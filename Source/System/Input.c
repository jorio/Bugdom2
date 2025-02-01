// SDL INPUT
// (C) 2023 Iliyas Jorio
// This file is part of Bugdom 2. https://github.com/jorio/Bugdom2

#include "game.h"

/***************/
/* CONSTANTS   */
/***************/

#define MOUSE_SMOOTHING 1

#define kJoystickDeadZoneFrac			(.33f)
#define kJoystickDeadZoneFrac_UI		(.66f)

#if MOUSE_SMOOTHING
static const int kMouseSmoothingAccumulatorWindowNanoseconds = 10 * 1e6;  // 10 milliseconds
#define DELTA_MOUSE_MAX_SNAPSHOTS 64
#endif

/**********************/
/*     PROTOTYPES     */
/**********************/

typedef uint8_t KeyState;

typedef struct Gamepad
{
	bool					open;
	bool					fallbackToKeyboard;
	bool					hasRumble;
	SDL_Gamepad*			sdlGamepad;
	KeyState				needStates[NUM_CONTROL_NEEDS];
	float					needAnalog[NUM_CONTROL_NEEDS];
	float					needAnalogRaw[NUM_CONTROL_NEEDS];		// no dead zone
} Gamepad;

Boolean				gUserPrefersGamepad = false;

Gamepad				gGamepad = {0};

static KeyState		gKeyboardStates[SDL_SCANCODE_COUNT];
static KeyState		gMouseButtonStates[NUM_SUPPORTED_MOUSE_BUTTONS];
static KeyState		gNeedStates[NUM_CONTROL_NEEDS];

Boolean				gMouseMotionNow = false;
char				gTextInput[64];

#if MOUSE_SMOOTHING
static struct MouseSmoothingState
{
	bool initialized;
	struct
	{
		uint64_t timestamp;	// nanoseconds
		float dx;
		float dy;
	} snapshots[DELTA_MOUSE_MAX_SNAPSHOTS];
	int ringStart;
	int ringLength;
	float dxAccu;
	float dyAccu;
} gMouseSmoothing = {0};

static void MouseSmoothing_ResetState(void);
static void MouseSmoothing_StartFrame(void);
static void MouseSmoothing_OnMouseMotion(const SDL_MouseMotionEvent* motion);
#endif

static void OnJoystickRemoved(SDL_JoystickID which);
static SDL_Gamepad* TryOpenGamepadFromJoystick(SDL_JoystickID joystickID);
static SDL_Gamepad* TryOpenAnyUnusedGamepad(bool showMessage);
static void SetPlayerAxisControls(void);
static void CloseGamepad(void);

static SDL_Cursor* gSystemCursors[SDL_SYSTEM_CURSOR_COUNT];

#pragma mark -
/**********************/

void InitInput(void)
{
	TryOpenAnyUnusedGamepad(false);
}

void DisposeInput(void)
{
	if (gGamepad.open)
	{
		CloseGamepad();
	}

	for (int i = 0; i < SDL_SYSTEM_CURSOR_COUNT; i++)
	{
		SDL_DestroyCursor(gSystemCursors[i]);
		gSystemCursors[i] = NULL;
	}
}

static inline void UpdateKeyState(KeyState* state, bool downNow)
{
	switch (*state)	// look at prev state
	{
		case KEYSTATE_HELD:
		case KEYSTATE_DOWN:
			*state = downNow ? KEYSTATE_HELD : KEYSTATE_UP;
			break;

		case KEYSTATE_OFF:
		case KEYSTATE_UP:
		default:
			*state = downNow ? KEYSTATE_DOWN : KEYSTATE_OFF;
			break;

		case KEYSTATE_IGNOREHELD:
			*state = downNow ? KEYSTATE_IGNOREHELD : KEYSTATE_OFF;
			break;
	}
}

void InvalidateNeedState(int need)
{
	gNeedStates[need] = KEYSTATE_IGNOREHELD;
}

void InvalidateAllInputs(void)
{
	_Static_assert(1 == sizeof(KeyState), "sizeof(KeyState) has changed -- Rewrite this function without memset()!");

	SDL_memset(gNeedStates, KEYSTATE_IGNOREHELD, NUM_CONTROL_NEEDS);
	SDL_memset(gKeyboardStates, KEYSTATE_IGNOREHELD, SDL_SCANCODE_COUNT);
	SDL_memset(gMouseButtonStates, KEYSTATE_IGNOREHELD, NUM_SUPPORTED_MOUSE_BUTTONS);

	SDL_memset(gGamepad.needStates, KEYSTATE_IGNOREHELD, NUM_CONTROL_NEEDS);
}

static void UpdateRawKeyboardStates(void)
{
	int numkeys = 0;
	const bool* keystate = SDL_GetKeyboardState(&numkeys);

	int minNumKeys = SDL_min(numkeys, SDL_SCANCODE_COUNT);

	for (int i = 0; i < minNumKeys; i++)
		UpdateKeyState(&gKeyboardStates[i], keystate[i]);

	// fill out the rest
	for (int i = minNumKeys; i < SDL_SCANCODE_COUNT; i++)
		UpdateKeyState(&gKeyboardStates[i], false);
}

static void ProcessAltEnter(void)
{
	if (IsKeyDown(SDL_SCANCODE_RETURN)
		&& (IsKeyHeld(SDL_SCANCODE_LALT) || IsKeyHeld(SDL_SCANCODE_RALT)))
	{
		gGamePrefs.fullscreen = !gGamePrefs.fullscreen;
		SetFullscreenMode(false);

		InvalidateAllInputs();
	}
}

#if __APPLE__
static void ProcessCmdQ(void)
{
	if ((IsKeyHeld(SDL_SCANCODE_LGUI) || IsKeyHeld(SDL_SCANCODE_RGUI))
		&& IsKeyDown(SDL_GetScancodeFromKey(SDLK_Q, NULL)))
	{
		if (gInGameNow && !gGamePaused)
		{
			UpdateKeyState(&gNeedStates[kNeed_UIPause], true);
		}
		else
		{
			CleanQuit();
		}
	}
}
#endif

static void UpdateMouseButtonStates(int mouseWheelDeltaX, int mouseWheelDeltaY)
{
	uint32_t mouseButtons = SDL_GetMouseState(NULL, NULL);

	for (int i = 1; i < NUM_SUPPORTED_MOUSE_BUTTONS_PURESDL; i++)	// SDL buttons start at 1!
	{
		bool buttonBit = 0 != (mouseButtons & SDL_BUTTON_MASK(i));
		UpdateKeyState(&gMouseButtonStates[i], buttonBit);
	}

	// Fake buttons for mouse wheel up/down
	UpdateKeyState(&gMouseButtonStates[SDL_BUTTON_WHEELUP], mouseWheelDeltaX > 0);
	UpdateKeyState(&gMouseButtonStates[SDL_BUTTON_WHEELDOWN], mouseWheelDeltaX < 0);
	UpdateKeyState(&gMouseButtonStates[SDL_BUTTON_WHEELLEFT], mouseWheelDeltaY < 0);
	UpdateKeyState(&gMouseButtonStates[SDL_BUTTON_WHEELRIGHT], mouseWheelDeltaY > 0);
}

static void UpdateKeyboardMouseInputNeeds(void)
{
	for (int need = 0; need < NUM_CONTROL_NEEDS; need++)
	{
		const InputBinding* kb = &gGamePrefs.bindings[need];

		bool pressed = false;

		for (int j = 0; j < MAX_BINDINGS_PER_NEED; j++)
		{
			int16_t scancode = kb->key[j];
			if (scancode && scancode < SDL_SCANCODE_COUNT)
			{
				pressed |= gKeyboardStates[scancode] & KEYSTATE_ACTIVE_BIT;
			}
		}

		pressed |= gMouseButtonStates[kb->mouseButton] & KEYSTATE_ACTIVE_BIT;

		UpdateKeyState(&gNeedStates[need], pressed);
	}
}

static void UpdateControllerInputNeeds(void)
{
	Gamepad* controller = &gGamepad;

	if (!controller->open)
	{
		return;
	}

	SDL_Gamepad* sdlGamepad = controller->sdlGamepad;

	for (int needNum = 0; needNum < NUM_CONTROL_NEEDS; needNum++)
	{
		const InputBinding* kb = &gGamePrefs.bindings[needNum];

		float deadZoneFrac = needNum >= NUM_REMAPPABLE_NEEDS
						   ? kJoystickDeadZoneFrac_UI
						   : kJoystickDeadZoneFrac;

		bool pressed = false;
		float actuation = 0;
		float analogRaw = 0;

		for (int buttonNum = 0; buttonNum < MAX_BINDINGS_PER_NEED; buttonNum++)
		{
			const PadBinding* pb = &kb->pad[buttonNum];
			int type = pb->type;

			if (type == kInputTypeButton)
			{
				if (0 != SDL_GetGamepadButton(sdlGamepad, pb->id))
				{
					pressed = true;
					actuation = 1;
				}
			}
			else if (type == kInputTypeAxisPlus || type == kInputTypeAxisMinus)
			{
				int16_t axis = SDL_GetGamepadAxis(sdlGamepad, pb->id);
				if (type == kInputTypeAxisPlus)
					analogRaw = axis * (1.0f / 32767.0f);
				else
					analogRaw = axis * (1.0f / -32768.0f);

				if (analogRaw < 0)
					analogRaw = 0;

				GAME_DEBUGASSERT(analogRaw >= 0);

				if (analogRaw >= deadZoneFrac)
				{
					pressed = true;
					float pastDeadZone = analogRaw - deadZoneFrac / (1.0f - deadZoneFrac);
					actuation = SDL_max(actuation, pastDeadZone);
				}
			}
		}

		controller->needAnalog[needNum] = actuation;
		controller->needAnalogRaw[needNum] = analogRaw;

		UpdateKeyState(&controller->needStates[needNum], pressed);
	}
}

#pragma mark -

/**********************/
/* PUBLIC FUNCTIONS   */
/**********************/

void UpdateInput(void)		// Also called DoSDLMaintenance in other ports
{
	gTextInput[0] = '\0';
	gMouseMotionNow = false;
	int mouseWheelDeltaX = 0;
	int mouseWheelDeltaY = 0;

			/**********************/
			/* DO SDL MAINTENANCE */
			/**********************/

#if MOUSE_SMOOTHING
	MouseSmoothing_StartFrame();
#endif

	SDL_PumpEvents();
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_EVENT_QUIT:
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				CleanQuit();			// throws Pomme::QuitRequest
				return;

			case SDL_EVENT_KEY_DOWN:
				gUserPrefersGamepad = false;
				break;

			case SDL_EVENT_TEXT_INPUT:
				SDL_snprintf(gTextInput, sizeof(gTextInput), "%s", event.text.text);
				break;

			case SDL_EVENT_MOUSE_MOTION:
				gMouseMotionNow = true;
				gUserPrefersGamepad = false;
#if MOUSE_SMOOTHING
				MouseSmoothing_OnMouseMotion(&event.motion);
#endif
				break;

			case SDL_EVENT_MOUSE_WHEEL:
				gUserPrefersGamepad = false;
				mouseWheelDeltaX += event.wheel.y;
				mouseWheelDeltaY += event.wheel.x;
				break;

			case SDL_EVENT_GAMEPAD_ADDED:
				TryOpenGamepadFromJoystick(event.gdevice.which);
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				OnJoystickRemoved(event.gdevice.which);
				break;

			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				gUserPrefersGamepad = true;
				break;
		}
	}


	// Refresh the state of each individual keyboard key
	UpdateRawKeyboardStates();

	// On ALT+ENTER, toggle fullscreen, and ignore ENTER until keyup.
	ProcessAltEnter();

	// Refresh the state of each mouse button
	UpdateMouseButtonStates(mouseWheelDeltaX, mouseWheelDeltaY);

	// Refresh the state of each input need
	UpdateKeyboardMouseInputNeeds();
	UpdateControllerInputNeeds();

#if __APPLE__
	// Check for Cmd-Q (must be after UpdateInputNeeds because it may update kNeed_UIPause)
	ProcessCmdQ();
#endif

	// Bugdom 2 player analog controls
	SetPlayerAxisControls();
}

#pragma mark - Keyboard states

int GetKeyState(uint16_t sdlScancode)
{
	if (sdlScancode >= SDL_SCANCODE_COUNT)
		return KEYSTATE_OFF;
	return gKeyboardStates[sdlScancode];
}

#pragma mark - Click states

int GetClickState(int mouseButton)
{
	if (mouseButton >= NUM_SUPPORTED_MOUSE_BUTTONS)
		return KEYSTATE_OFF;
	return gMouseButtonStates[mouseButton];
}

#pragma mark - Need states

int GetNeedState(int needID)
{
	GAME_ASSERT(needID >= 0);
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);

	const Gamepad* controller = &gGamepad;

	if (controller->open && controller->needStates[needID])
	{
		return controller->needStates[needID];
	}

	// Fallback to KB/M
	return gNeedStates[needID];

//	return KEYSTATE_OFF;
}

static float GetAnalogValue(int needID, bool raw)
{
	GAME_ASSERT(needID >= 0);
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);

	// Get KB/M first
	if (gNeedStates[needID] & KEYSTATE_ACTIVE_BIT)
	{
		return 1;
	}

	const Gamepad* controller = &gGamepad;

	if (controller->open && controller->needAnalogRaw[needID] != 0.0f)
	{
		float value = controller->needAnalogRaw[needID];
		GAME_DEBUGASSERT(value >= 0);
		GAME_DEBUGASSERT(value <= 1);

		if (!raw)
		{
			// Adjust for dead zone
			float deadZone = needID < NUM_REMAPPABLE_NEEDS ? kJoystickDeadZoneFrac : kJoystickDeadZoneFrac_UI;
			value = (value - deadZone) / (1.0f - deadZone);
			value = SDL_max(0, value);		// clamp to 0 if within dead zone
		}

		return value;
	}

	return 0;
}

float GetNeedAxis1D(int negativeNeedID, int positiveNeedID)
{
	float neg = GetAnalogValue(negativeNeedID, false);
	float pos = GetAnalogValue(positiveNeedID, false);

	if (neg > pos)
	{
		return -neg;
	}
	else
	{
		return pos;
	}
}

OGLPolar2D GetNeedAxis2D(int negXID, int posXID, int negYID, int posYID)
{
	float deadZone = negXID < NUM_REMAPPABLE_NEEDS ? kJoystickDeadZoneFrac : kJoystickDeadZoneFrac_UI;

	float negX = GetAnalogValue(negXID, true);
	float posX = GetAnalogValue(posXID, true);
	float x = negX > posX? -negX: posX;

	float negY = GetAnalogValue(negYID, true);
	float posY = GetAnalogValue(posYID, true);
	float y = negY > posY? -negY: posY;

	float mag = SDL_sqrtf(SQUARED(x) + SQUARED(y));
	if (mag < deadZone)
	{
		mag = 0;
	}
	else
	{
		mag = (mag - deadZone) / (1.0f - deadZone);
		mag = SDL_min(mag, 1);		// clamp to 0..1
	}

	float angle = SDL_atan2f(y, x);
	return (OGLPolar2D) { .a=angle, .r=mag };
}

Boolean UserWantsOut(void)
{
	if (gGammaFadeFrac < 1)				// disallow skipping during fade-in
		return false;

	return IsNeedDown(kNeed_UIConfirm)
		|| IsNeedDown(kNeed_UIBack)
		|| IsNeedDown(kNeed_UIPause)
		|| IsClickDown(SDL_BUTTON_LEFT);
}

Boolean IsCheatKeyComboDown(void)
{
	return (IsKeyHeld(SDL_SCANCODE_B) && IsKeyHeld(SDL_SCANCODE_U) && IsKeyHeld(SDL_SCANCODE_G))
		|| ((IsKeyHeld(SDL_SCANCODE_LGUI) || IsKeyHeld(SDL_SCANCODE_RGUI)) && (IsKeyHeld(SDL_SCANCODE_HELP) || IsKeyHeld(SDL_SCANCODE_INSERT)));
}

#pragma mark - Controller mapping

/****************************** SDL JOYSTICK FUNCTIONS ********************************/

int GetNumControllers(void)
{
	return gGamepad.open? 1: 0;
}

static SDL_Gamepad* TryOpenGamepadFromJoystick(SDL_JoystickID joystickID)
{
	// First, check that it's not in use already
	if (gGamepad.open && SDL_GetGamepadID(gGamepad.sdlGamepad) == joystickID)
	{
		return gGamepad.sdlGamepad;
	}

	// Don't open if we already have a gamepad
	if (gGamepad.open)
	{
		return NULL;
	}

	// If we can't get an SDL_Gamepad from that joystick, don't bother
	if (!SDL_IsGamepad(joystickID))
	{
		return NULL;
	}

	// Use this one
	SDL_Gamepad* sdlGamepad = SDL_OpenGamepad(joystickID);

	SDL_PropertiesID props = SDL_GetGamepadProperties(sdlGamepad);

	gGamepad = (Gamepad)
	{
		.open = true,
		.sdlGamepad = sdlGamepad,
		.hasRumble = SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false),
	};

	SDL_Log("Opened joystick %d as gamepad: \"%s\" - Rumble support: %d",
		joystickID,
		SDL_GetGamepadName(gGamepad.sdlGamepad),
		gGamepad.hasRumble);

	gUserPrefersGamepad = true;

	return gGamepad.sdlGamepad;
}

static SDL_Gamepad* TryOpenAnyUnusedGamepad(bool showMessage)
{
	int numJoysticks = 0;
	int numJoysticksAlreadyInUse = 0;

	SDL_JoystickID* joysticks = SDL_GetJoysticks(&numJoysticks);
	SDL_Gamepad* newGamepad = NULL;

	for (int i = 0; i < numJoysticks; ++i)
	{
		SDL_JoystickID joystickID = joysticks[i];

		// Usable as an SDL_Gamepad?
		if (!SDL_IsGamepad(joystickID))
		{
			continue;
		}

		// Already in use?
		if (gGamepad.open && SDL_GetGamepadID(gGamepad.sdlGamepad) == joystickID)
		{
			numJoysticksAlreadyInUse++;
			continue;
		}

		// Use this one
		newGamepad = TryOpenGamepadFromJoystick(joystickID);
		if (newGamepad)
		{
			break;
		}
	}

	if (newGamepad)
	{
		// OK
	}
	else if (numJoysticksAlreadyInUse == numJoysticks)
	{
		// No-op; All joysticks already in use (or there might be zero joysticks)
	}
	else
	{
		SDL_Log("%d joysticks found, but none is suitable as an SDL_Gamepad.", numJoysticks);
		if (showMessage)
		{
			char messageBuf[1024];
			SDL_snprintf(messageBuf, sizeof(messageBuf),
						 "The game does not support your controller yet (\"%s\").\n\n"
						 "You can play with the keyboard and mouse instead. Sorry!",
						 SDL_GetJoystickNameForID(joysticks[0]));
			SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_WARNING,
				"Controller not supported",
				messageBuf,
				gSDLWindow);
		}
	}

	SDL_free(joysticks);

	return newGamepad;
}

void Rumble(float lowFrequencyStrength, float highFrequencyStrength, uint32_t ms)
{
	static const float kRumbleIntensities[] =
	{
		0.0f,
		0.33f,
		0.66f,
		1.0f,
	};

	// Don't bother if rumble turned off in prefs
	if (!gGamePrefs.gamepadRumbleLevel)
	{
		return;
	}

	int level = gGamePrefs.gamepadRumbleLevel;
	_Static_assert(MAX_GAMEPAD_RUMBLE_LEVEL+1 == SDL_arraysize(kRumbleIntensities), "incorrect MAX_GAMEPAD_RUMBLE_LEVEL");
	level = SDL_clamp(level, 0, MAX_GAMEPAD_RUMBLE_LEVEL);

	float rumbleIntensityFrac = kRumbleIntensities[level];
	lowFrequencyStrength *= rumbleIntensityFrac;
	highFrequencyStrength *= rumbleIntensityFrac;

	const Gamepad* gamepad = &gGamepad;

	// Gotta have a valid SDL_Gamepad instance
	if (!gamepad->hasRumble || !gamepad->sdlGamepad)
	{
		return;
	}

	SDL_RumbleGamepad(
		gamepad->sdlGamepad,
		(Uint16)(lowFrequencyStrength * 65535),
		(Uint16)(highFrequencyStrength * 65535),
		(Uint32)((float)ms * rumbleIntensityFrac));

	// Prevent jetpack effect from kicking in while we're playing this
//	gPlayerInfo[playerID].jetpackRumbleCooldown = ms * (1.0f / 1000.0f);
}

static void CloseGamepad(void)
{
	GAME_ASSERT(gGamepad.open);
	GAME_ASSERT(gGamepad.sdlGamepad);

	SDL_CloseGamepad(gGamepad.sdlGamepad);
	gGamepad.open = false;
	gGamepad.sdlGamepad = NULL;
	gGamepad.hasRumble = false;
}

static void OnJoystickRemoved(SDL_JoystickID joystickID)
{
	if (gGamepad.open && SDL_GetGamepadID(gGamepad.sdlGamepad) == joystickID)		// we're using this joystick
	{
		SDL_Log("Joystick %d was removed, was in use by game", joystickID);

		// Nuke reference to this SDL_Gamepad
		CloseGamepad();
	}

	// Fill up any controller slots that are vacant
	TryOpenAnyUnusedGamepad(false);

	// Disable gUserPrefersGamepad if there are no controllers connected
	if (0 == GetNumControllers())
		gUserPrefersGamepad = false;
}

#pragma mark - Reset bindings

void ResetDefaultKeyboardBindings(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		SDL_memcpy(gGamePrefs.bindings[i].key, kDefaultInputBindings[i].key, sizeof(gGamePrefs.bindings[i].key));
	}
}

void ResetDefaultGamepadBindings(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		SDL_memcpy(gGamePrefs.bindings[i].pad, kDefaultInputBindings[i].pad, sizeof(gGamePrefs.bindings[i].pad));
	}

	gGamePrefs.gamepadRumbleLevel = MAX_GAMEPAD_RUMBLE_LEVEL;
}

void ResetDefaultMouseBindings(void)
{
	gGamePrefs.mouseSensitivityLevel = DEFAULT_MOUSE_SENSITIVITY_LEVEL;

	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		gGamePrefs.bindings[i].mouseButton = kDefaultInputBindings[i].mouseButton;
	}
}

#pragma mark - Mouse smoothing

#if MOUSE_SMOOTHING
static void MouseSmoothing_PopOldestSnapshot(void)
{
	struct MouseSmoothingState* state = &gMouseSmoothing;

	state->dxAccu -= state->snapshots[state->ringStart].dx;
	state->dyAccu -= state->snapshots[state->ringStart].dy;

	state->ringStart = (state->ringStart + 1) % DELTA_MOUSE_MAX_SNAPSHOTS;
	state->ringLength--;

	GAME_ASSERT(state->ringLength != 0 || (state->dxAccu == 0 && state->dyAccu == 0));
}

static void MouseSmoothing_ResetState(void)
{
	struct MouseSmoothingState* state = &gMouseSmoothing;
	state->ringLength = 0;
	state->ringStart = 0;
	state->dxAccu = 0;
	state->dyAccu = 0;
}

static void MouseSmoothing_StartFrame(void)
{
	struct MouseSmoothingState* state = &gMouseSmoothing;

	if (!state->initialized)
	{
		MouseSmoothing_ResetState();
		state->initialized = true;
	}

	uint64_t now = SDL_GetTicksNS();
	uint64_t cutoffTimestamp = now - kMouseSmoothingAccumulatorWindowNanoseconds;

	// Purge old snapshots
	while (state->ringLength > 0 &&
		   state->snapshots[state->ringStart].timestamp < cutoffTimestamp)
	{
		MouseSmoothing_PopOldestSnapshot();
	}
}

static void MouseSmoothing_OnMouseMotion(const SDL_MouseMotionEvent* motion)
{
	struct MouseSmoothingState* state = &gMouseSmoothing;

	// ignore mouse input if user has alt-tabbed away from the game
	if (!(SDL_GetWindowFlags(gSDLWindow) & SDL_WINDOW_INPUT_FOCUS))
	{
		return;
	}

	if (state->ringLength == DELTA_MOUSE_MAX_SNAPSHOTS)
	{
//		SDL_Log("%s: buffer full!!", __func__);
		MouseSmoothing_PopOldestSnapshot();				// make room at start of ring buffer
	}

	int i = (state->ringStart + state->ringLength) % DELTA_MOUSE_MAX_SNAPSHOTS;
	state->ringLength++;

	state->snapshots[i].timestamp = motion->timestamp;
	state->snapshots[i].dx = motion->xrel;
	state->snapshots[i].dy = motion->yrel;

	state->dxAccu += motion->xrel;
	state->dyAccu += motion->yrel;
}
#endif

OGLVector2D GetMouseDelta(void)
{
	float sensitivity = (float)(1 + gGamePrefs.mouseSensitivityLevel) / (NUM_MOUSE_SENSITIVITY_LEVELS);

	// Tweak range so that default sensitivity --> 1.0f
	sensitivity *= NUM_MOUSE_SENSITIVITY_LEVELS / (1.0f + DEFAULT_MOUSE_SENSITIVITY_LEVEL);

#if MOUSE_SMOOTHING
	struct MouseSmoothingState* state = &gMouseSmoothing;

	GAME_ASSERT(state->ringLength != 0 || (state->dxAccu == 0 && state->dyAccu == 0));

	return (OGLVector2D) {state->dxAccu * sensitivity, state->dyAccu * sensitivity};
#else
	static float timeSinceLastCall = 0;
	static OGLVector2D lastDelta = { 0, 0 };

	timeSinceLastCall += gFramesPerSecondFrac;

	// Mouse sensitivity settings are calibrated to feel good at 60 FPS,
	// so we mustn't poll GetRelativeMouseState any faster than 60 Hz.
	if (timeSinceLastCall >= (1.0f / 60.0f))
	{
		float x = 0;
		float y = 0;
		SDL_GetRelativeMouseState(&x, &y);
		lastDelta = (OGLVector2D){x * sensitivity, y * sensitivity};
		timeSinceLastCall = 0;
	}

	return lastDelta;
#endif
}

OGLPoint2D GetMouseCoordsIn2DLogicalRect(void)
{
	// On macOS, the mouse position is relative to the window's "point size" on Retina screens.
	float mx, my;
	int ww, wh;
	SDL_GetMouseState(&mx, &my);
	SDL_GetWindowSize(gSDLWindow, &ww, &wh);		// IMPORTANT: Do NOT use SDL_GetWindowSizeInPixels here

//	OGLRect r = Get2DLogicalRect(GetOverlayPaneNumber(), 1);
	OGLRect r = g2DLogicalRect;

	float screenToPaneX = (r.right - r.left) / (float)ww;
	float screenToPaneY = (r.bottom - r.top) / (float)wh;

	OGLPoint2D p =
	{
		mx * screenToPaneX + r.left,
		my * screenToPaneY + r.top
	};

	return p;
}

void BackupRestoreCursorCoord(Boolean backup)
{
	static OGLPoint2D cursorCoordBackup = { -1, -1 };

	if (backup)
	{
//		cursorCoordBackup = gCursorCoord;
		cursorCoordBackup = GetMouseCoordsIn2DLogicalRect();
	}
	else if (cursorCoordBackup.x >= 0)
	{
//		gCursorCoord = cursorCoordBackup;
		OGLPoint2D cursorCoord = cursorCoordBackup;

//		OGLRect r = Get2DLogicalRect(GetOverlayPaneNumber(), 1);
		OGLRect r = g2DLogicalRect;

		int ww, wh;
		SDL_GetWindowSize(gSDLWindow, &ww, &wh);		// IMPORTANT: Do NOT use SDL_GetWindowSizeInPixels here!

		float screenToPaneX = (r.right - r.left) / (float)ww;
		float screenToPaneY = (r.bottom - r.top) / (float)wh;

		float mx = (cursorCoord.x - r.left) / screenToPaneX;
		float my = (cursorCoord.y - r.top) / screenToPaneY;
		SDL_WarpMouseInWindow(gSDLWindow, mx, my);
	}
}

void GrabMouse(Boolean capture)
{
#if SKIPFLUFF
	(void) capture;
#else
	if (capture)
	{
		BackupRestoreCursorCoord(true);
	}

	SDL_SetWindowMouseGrab(gSDLWindow, capture);
	SDL_SetWindowRelativeMouseMode(gSDLWindow, capture);
	SetMacLinearMouse(capture);

	if (!capture)
	{
		BackupRestoreCursorCoord(false);
	}
#endif
}

void SetSystemCursor(int sdlSystemCursor)
{
	if (sdlSystemCursor < 0)
	{
		SDL_HideCursor();
		return;
	}

	GAME_ASSERT(sdlSystemCursor < SDL_SYSTEM_CURSOR_COUNT);

	SDL_Cursor* cursor = gSystemCursors[sdlSystemCursor];

	if (!cursor)
	{
		cursor = SDL_CreateSystemCursor(sdlSystemCursor);
		gSystemCursors[sdlSystemCursor] = cursor;
	}

	if (cursor != NULL && SDL_GetCursor() != cursor)
	{
		SDL_SetCursor(cursor);
	}

	SDL_ShowCursor();
}

SDL_Gamepad* GetGamepad(void)
{
	if (gGamepad.open)
		return gGamepad.sdlGamepad;
	
	return NULL;
}

static float SnapAngle(float angle, float snap)
{
	if (angle >= -snap && angle <= snap)							// east (-0...0)
		return 0;
	else if (angle >= PI/2 - snap && angle <= PI/2 + snap)			// south
		return PI/2;
	else if (angle >= PI - snap || angle <= -PI + snap)				// west (180...-180)
		return PI;
	else if (angle >= -PI/2 - snap && angle <= -PI/2 + snap)		// north
		return -PI/2;
	else
		return angle;
}

static void SetPlayerAxisControls(void)
{
	gPlayerInfo.analogIsMouse = false;

		/* FIRST CHECK ANALOG AXES */

	OGLPolar2D polar = GetNeedAxis2D(kNeed_TurnLeft, kNeed_TurnRight, kNeed_Forward, kNeed_Backward);

	float magnitude = polar.r;
	float angle = polar.a;

	GAME_DEBUGASSERT(magnitude >= 0);
	GAME_DEBUGASSERT(magnitude <= 1);

	angle = SnapAngle(angle, OGLMath_DegreesToRadians(10));
	float x = magnitude * SDL_cosf(angle);
	float z = magnitude * SDL_sinf(angle);

#if 0
	if (magnitude != 0.0f)
	{
		SDL_Log("%f x %d deg", magnitude, (int) OGLMath_RadiansToDegrees(angle));
	}
#endif

	gPlayerInfo.analogControlX = x;
	gPlayerInfo.analogControlZ = z;

		/* AND FINALLY SEE IF MOUSE DELTAS ARE BEST */

	if (!gGamePrefs.mouseControlsSkip)
		return;

	OGLVector2D mouseDelta = GetMouseDelta();

	float mouseDX = mouseDelta.x * 0.015f;						// scale down deltas for our use
	float mouseDY = mouseDelta.y * 0.015f;

	mouseDX = SDL_clamp(mouseDX, -1.0f, 1.0f);					// keep x values pinned
	mouseDY = SDL_clamp(mouseDY, -1.0f, 1.0f);					// keep y values pinned

	if (fabs(mouseDX) > fabs(gPlayerInfo.analogControlX))		// is the mouse delta better than what we've got from the other devices?
	{
		gPlayerInfo.analogControlX = mouseDX;
		gPlayerInfo.analogIsMouse = true;
	}

	if (fabs(mouseDY) > fabs(gPlayerInfo.analogControlZ))		// is the mouse delta better than what we've got from the other devices?
	{
		gPlayerInfo.analogControlZ = mouseDY;
		gPlayerInfo.analogIsMouse = true;
	}
}
