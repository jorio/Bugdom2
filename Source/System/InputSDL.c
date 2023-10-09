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
static const int kMouseSmoothingAccumulatorWindowTicks = 10;
#define DELTA_MOUSE_MAX_SNAPSHOTS 64
#endif

/**********************/
/*     PROTOTYPES     */
/**********************/

typedef uint8_t KeyState;

typedef struct Controller
{
	bool					open;
	bool					fallbackToKeyboard;
	bool					hasRumble;
	SDL_GameController*		controllerInstance;
	SDL_JoystickID			joystickInstance;
	KeyState				needStates[NUM_CONTROL_NEEDS];
	float					needAnalog[NUM_CONTROL_NEEDS];
	float					needAnalogRaw[NUM_CONTROL_NEEDS];		// no dead zone
} Controller;

Boolean				gUserPrefersGamepad = false;

Controller			gController = {0};

static KeyState		gKeyboardStates[SDL_NUM_SCANCODES];
static KeyState		gMouseButtonStates[NUM_SUPPORTED_MOUSE_BUTTONS];
static KeyState		gNeedStates[NUM_CONTROL_NEEDS];

Boolean				gMouseMotionNow = false;
char				gTextInput[SDL_TEXTINPUTEVENT_TEXT_SIZE];

#if MOUSE_SMOOTHING
static struct MouseSmoothingState
{
	bool initialized;
	struct
	{
		uint32_t timestamp;
		int dx;
		int dy;
	} snapshots[DELTA_MOUSE_MAX_SNAPSHOTS];
	int ringStart;
	int ringLength;
	int dxAccu;
	int dyAccu;
} gMouseSmoothing = {0};

static void MouseSmoothing_ResetState(void);
static void MouseSmoothing_StartFrame(void);
static void MouseSmoothing_OnMouseMotion(const SDL_MouseMotionEvent* motion);
#endif

static void OnJoystickRemoved(SDL_JoystickID which);
static SDL_GameController* TryOpenControllerFromJoystick(int joystickIndex);
static SDL_GameController* TryOpenAnyUnusedController(bool showMessage);
static void SetPlayerAxisControls(void);
static void CloseController(void);

static SDL_Cursor* gSystemCursors[SDL_NUM_SYSTEM_CURSORS];

#pragma mark -
/**********************/

void InitInput(void)
{
	TryOpenAnyUnusedController(false);
}

void DisposeInput(void)
{
	if (gController.open)
	{
		CloseController();
	}

	for (int i = 0; i < SDL_NUM_SYSTEM_CURSORS; i++)
	{
		SDL_FreeCursor(gSystemCursors[i]);
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
	SDL_memset(gKeyboardStates, KEYSTATE_IGNOREHELD, SDL_NUM_SCANCODES);
	SDL_memset(gMouseButtonStates, KEYSTATE_IGNOREHELD, NUM_SUPPORTED_MOUSE_BUTTONS);

	SDL_memset(gController.needStates, KEYSTATE_IGNOREHELD, NUM_CONTROL_NEEDS);
}

static void UpdateRawKeyboardStates(void)
{
	int numkeys = 0;
	const UInt8* keystate = SDL_GetKeyboardState(&numkeys);

	int minNumKeys = SDL_min(numkeys, SDL_NUM_SCANCODES);

	for (int i = 0; i < minNumKeys; i++)
		UpdateKeyState(&gKeyboardStates[i], keystate[i]);

	// fill out the rest
	for (int i = minNumKeys; i < SDL_NUM_SCANCODES; i++)
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
		&& IsKeyDown(SDL_GetScancodeFromKey(SDLK_q)))
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
		bool buttonBit = 0 != (mouseButtons & SDL_BUTTON(i));
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
			if (scancode && scancode < SDL_NUM_SCANCODES)
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
	Controller* controller = &gController;

	if (!controller->open)
	{
		return;
	}

	SDL_GameController* controllerInstance = controller->controllerInstance;

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
				if (0 != SDL_GameControllerGetButton(controllerInstance, pb->id))
				{
					pressed = true;
					actuation = 1;
				}
			}
			else if (type == kInputTypeAxisPlus || type == kInputTypeAxisMinus)
			{
				int16_t axis = SDL_GameControllerGetAxis(controllerInstance, pb->id);
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
			case SDL_QUIT:
				CleanQuit();			// throws Pomme::QuitRequest
				return;

			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_CLOSE)
				{
					CleanQuit();		// throws Pomme::QuitRequest
				}
				break;

			case SDL_KEYDOWN:
				gUserPrefersGamepad = false;
				break;

			case SDL_TEXTINPUT:
				SDL_memcpy(gTextInput, event.text.text, sizeof(gTextInput));
				_Static_assert(sizeof(gTextInput) == sizeof(event.text.text), "size mismatch: gTextInput/event.text.text");
				break;

			case SDL_MOUSEMOTION:
				gMouseMotionNow = true;
				gUserPrefersGamepad = false;
#if MOUSE_SMOOTHING
				MouseSmoothing_OnMouseMotion(&event.motion);
#endif
				break;

			case SDL_MOUSEWHEEL:
				gUserPrefersGamepad = false;
				mouseWheelDeltaX += event.wheel.y;
				mouseWheelDeltaY += event.wheel.x;
				break;

			case SDL_CONTROLLERDEVICEADDED:
				// event.cdevice.which is the joystick's DEVICE INDEX (not an instance id!)
				TryOpenControllerFromJoystick(event.cdevice.which);
				break;

			case SDL_CONTROLLERDEVICEREMOVED:
				// event.cdevice.which is the joystick's UNIQUE INSTANCE ID (not an index!)
				OnJoystickRemoved(event.cdevice.which);
				break;

			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
			case SDL_JOYBUTTONDOWN:
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
	if (sdlScancode >= SDL_NUM_SCANCODES)
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

	const Controller* controller = &gController;

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

	const Controller* controller = &gController;

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
	return gController.open? 1: 0;
}

static SDL_GameController* TryOpenControllerFromJoystick(int joystickIndex)
{
	// First, check that it's not in use already
	if (gController.open && gController.joystickInstance == joystickIndex)
	{
		return gController.controllerInstance;
	}

	// Don't open if we already have a controller
	if (gController.open)
	{
		return NULL;
	}

	// If we can't get an SDL_GameController from that joystick, don't bother
	if (!SDL_IsGameController(joystickIndex))
	{
		return NULL;
	}

	// Use this one
	SDL_GameController* controllerInstance = SDL_GameControllerOpen(joystickIndex);

	gController = (Controller)
	{
		.open = true,
		.controllerInstance = controllerInstance,
		.joystickInstance = SDL_JoystickGetDeviceInstanceID(joystickIndex),
#if SDL_VERSION_ATLEAST(2,0,18)
		.hasRumble = SDL_GameControllerHasRumble(controllerInstance),
#else
		#warning Rumble support requires SDL 2.0.18 later
		.hasRumble = false,
#endif
	};

	SDL_Log("Opened joystick %d as controller: \"%s\" - Rumble support: %d\n",
		gController.joystickInstance,
		SDL_GameControllerName(gController.controllerInstance),
		gController.hasRumble);

	gUserPrefersGamepad = true;

	return gController.controllerInstance;
}

static SDL_GameController* TryOpenAnyUnusedController(bool showMessage)
{
	int numJoysticks = SDL_NumJoysticks();
	int numJoysticksAlreadyInUse = 0;

	if (numJoysticks == 0)
	{
		return NULL;
	}

	for (int i = 0; i < numJoysticks; ++i)
	{
		// Usable as an SDL GameController?
		if (!SDL_IsGameController(i))
		{
			continue;
		}

		// Already in use?
		if (gController.open && gController.joystickInstance == i)
		{
			numJoysticksAlreadyInUse++;
			continue;
		}

		// Use this one
		SDL_GameController* newController = TryOpenControllerFromJoystick(i);
		if (newController)
		{
			return newController;
		}
	}

	if (numJoysticksAlreadyInUse == numJoysticks)
	{
		// All joysticks already in use
		return NULL;
	}

	SDL_Log("Joystick(s) found, but none is suitable as an SDL_GameController.\n");
	if (showMessage)
	{
		char messageBuf[1024];
		SDL_snprintf(messageBuf, sizeof(messageBuf),
					"The game does not support your controller yet (\"%s\").\n\n"
					"You can play with the keyboard and mouse instead. Sorry!",
					SDL_JoystickNameForIndex(0));
		SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_WARNING,
				"Controller not supported",
				messageBuf,
				gSDLWindow);
	}

	return NULL;
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

#if !(SDL_VERSION_ATLEAST(2,0,18))
	#warning Rumble support requires SDL 2.0.18 later
#else
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

	const Controller* controller = &gController;

	// Gotta have a valid SDL controller instance
	if (!controller->hasRumble || !controller->controllerInstance)
	{
		return;
	}

	SDL_GameControllerRumble(
		controller->controllerInstance,
		(Uint16)(lowFrequencyStrength * 65535),
		(Uint16)(highFrequencyStrength * 65535),
		(Uint32)((float)ms * rumbleIntensityFrac));

	// Prevent jetpack effect from kicking in while we're playing this
//	gPlayerInfo[playerID].jetpackRumbleCooldown = ms * (1.0f / 1000.0f);
#endif
}

static void CloseController(void)
{
	GAME_ASSERT(gController.open);
	GAME_ASSERT(gController.controllerInstance);

	SDL_GameControllerClose(gController.controllerInstance);
	gController.open = false;
	gController.controllerInstance = NULL;
	gController.joystickInstance = -1;
	gController.hasRumble = false;
}

static void OnJoystickRemoved(SDL_JoystickID joystickInstanceID)
{
	if (gController.open && gController.joystickInstance == joystickInstanceID)		// we're using this joystick
	{
		SDL_Log("Joystick %d was removed, was in use by game\n", joystickInstanceID);

		// Nuke reference to this controller+joystick
		CloseController();
	}

	// Fill up any controller slots that are vacant
	TryOpenAnyUnusedController(false);

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

	uint32_t now = SDL_GetTicks();
	uint32_t cutoffTimestamp = now - kMouseSmoothingAccumulatorWindowTicks;

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
//		printf("%s: buffer full!!\n", __func__);
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
		int x = 0;
		int y = 0;
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
	int mx, my;
	int ww, wh;
	SDL_GetMouseState(&mx, &my);
	SDL_GetWindowSize(gSDLWindow, &ww, &wh);

//	OGLRect r = Get2DLogicalRect(GetOverlayPaneNumber(), 1);
	OGLRect r = g2DLogicalRect;

	float screenToPaneX = (r.right - r.left) / (float)ww;
	float screenToPaneY = (r.bottom - r.top) / (float)wh;

	OGLPoint2D p =
	{
		(float) mx * screenToPaneX + r.left,
		(float) my * screenToPaneY + r.top
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
		SDL_GetWindowSize(gSDLWindow, &ww, &wh);

		float screenToPaneX = (r.right - r.left) / (float)ww;
		float screenToPaneY = (r.bottom - r.top) / (float)wh;

		int mx = (int) ((cursorCoord.x - r.left) / screenToPaneX);
		int my = (int) ((cursorCoord.y - r.top) / screenToPaneY);
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

	SDL_SetWindowGrab(gSDLWindow, capture);
	SDL_SetRelativeMouseMode(capture? SDL_TRUE: SDL_FALSE);
//	SDL_ShowCursor(capture? 0: 1);
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
		SDL_ShowCursor(SDL_DISABLE);
		return;
	}

	GAME_ASSERT(sdlSystemCursor < SDL_NUM_SYSTEM_CURSORS);

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

	SDL_ShowCursor(SDL_ENABLE);
}

SDL_GameController* GetController(void)
{
	if (gController.open)
		return gController.controllerInstance;
	
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
