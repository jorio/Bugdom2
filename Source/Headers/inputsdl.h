//
// input.h
//

#pragma once

#define ANY_PLAYER (-1)

#define MAX_USER_BINDINGS_PER_NEED			2
#define MAX_BINDINGS_PER_NEED				4

#define NUM_SUPPORTED_MOUSE_BUTTONS			31
#define NUM_SUPPORTED_MOUSE_BUTTONS_PURESDL	(NUM_SUPPORTED_MOUSE_BUTTONS-4)		// make room for fake buttons below
#define SDL_BUTTON_WHEELLEFT				(NUM_SUPPORTED_MOUSE_BUTTONS-4)		// make wheelup look like it's a button
#define SDL_BUTTON_WHEELRIGHT				(NUM_SUPPORTED_MOUSE_BUTTONS-3)		// make wheelup look like it's a button
#define SDL_BUTTON_WHEELUP					(NUM_SUPPORTED_MOUSE_BUTTONS-2)		// make wheelup look like it's a button
#define SDL_BUTTON_WHEELDOWN				(NUM_SUPPORTED_MOUSE_BUTTONS-1)		// make wheeldown look like it's a button

#define NUM_MOUSE_SENSITIVITY_LEVELS		8
#define DEFAULT_MOUSE_SENSITIVITY_LEVEL		(NUM_MOUSE_SENSITIVITY_LEVELS/2)
#define MAX_GAMEPAD_RUMBLE_LEVEL			3

enum
{
	KEYSTATE_ACTIVE_BIT		= 0b001,
	KEYSTATE_CHANGE_BIT		= 0b010,
	KEYSTATE_IGNORE_BIT		= 0b100,

	KEYSTATE_OFF			= 0b000,
	KEYSTATE_DOWN			= KEYSTATE_ACTIVE_BIT | KEYSTATE_CHANGE_BIT,
	KEYSTATE_HELD			= KEYSTATE_ACTIVE_BIT,
	KEYSTATE_UP				= KEYSTATE_OFF | KEYSTATE_CHANGE_BIT,
	KEYSTATE_IGNOREHELD		= KEYSTATE_OFF | KEYSTATE_IGNORE_BIT,
};

typedef struct
{
	int8_t		type;
	int8_t		id;
} PadBinding;

typedef struct
{
	int16_t			key[MAX_BINDINGS_PER_NEED];
	PadBinding		pad[MAX_BINDINGS_PER_NEED];
	int8_t			mouseButton;
} InputBinding;

enum
{
	kInputTypeUnbound = 0,
	kInputTypeButton,
	kInputTypeAxisPlus,
	kInputTypeAxisMinus,
};


//============================================================================================

void InitInput(void);
void DisposeInput(void);
void UpdateInput(void);
void InvalidateAllInputs(void);
void InvalidateNeedState(int need);

int GetKeyState(uint16_t sdlScancode);
int GetClickState(int mouseButton);
int GetNeedState(int needID);
float GetNeedAxis1D(int negativeNeedID, int positiveNeedID);
OGLPolar2D GetNeedAxis2D(int xNegativeNeedID, int xPositiveNeedID, int yNegativeNeedID, int yPositiveNeedID);

#define IsKeyDown(scancode) (KEYSTATE_DOWN == GetKeyState((scancode)))
#define IsKeyHeld(scancode) (KEYSTATE_HELD == GetKeyState((scancode)))
#define IsKeyActive(scancode) (KEYSTATE_ACTIVE_BIT & GetKeyState((scancode)))
#define IsKeyUp(scancode) (KEYSTATE_UP == GetKeyState((scancode)))

#define IsClickDown(button) (KEYSTATE_DOWN == GetClickState((button)))
#define IsClickHeld(button) (KEYSTATE_HELD == GetClickState((button)))
#define IsClickUp(button) (KEYSTATE_UP == GetClickState((button)))

#define IsNeedDown(needID) (KEYSTATE_DOWN == GetNeedState((needID)))
#define IsNeedHeld(needID) (KEYSTATE_HELD == GetNeedState((needID)))
#define IsNeedActive(needID) (KEYSTATE_ACTIVE_BIT & GetNeedState((needID)))
#define IsNeedUp(needID) (KEYSTATE_UP == GetNeedState((needID)))

Boolean UserWantsOut(void);
Boolean IsCheatKeyComboDown(void);

int GetNumControllers(void);
SDL_Gamepad* GetGamepad(void);
void SetMainController(int oldControllerSlot);
void Rumble(float lowFrequencyStrength, float highFrequencyStrength, uint32_t ms);

void ResetDefaultKeyboardBindings(void);
void ResetDefaultGamepadBindings(void);
void ResetDefaultMouseBindings(void);

OGLVector2D GetMouseDelta(void);
OGLPoint2D GetMouseCoordsIn2DLogicalRect(void);
void GrabMouse(Boolean capture);
void SetSystemCursor(int sdlSystemCursor);
void SetMacLinearMouse(Boolean linear);

void BackupRestoreCursorCoord(Boolean backup);
