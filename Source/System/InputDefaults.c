#include "game.h"

#if __APPLE__
	#define SC_KICK1 SDL_SCANCODE_LGUI
	#define SC_KICK2 SDL_SCANCODE_RGUI
	#define SC_PICK1 SDL_SCANCODE_LALT
	#define SC_PICK2 SDL_SCANCODE_RALT
#else
	#define SC_KICK1 SDL_SCANCODE_LCTRL
	#define SC_KICK2 SDL_SCANCODE_RCTRL
	#define SC_PICK1 SDL_SCANCODE_LALT
	#define SC_PICK2 SDL_SCANCODE_RALT
#endif

// Controller button/axis
#define CB(x)		{kInputTypeButton, SDL_CONTROLLER_BUTTON_##x}
#define CAPLUS(x)	{kInputTypeAxisPlus, SDL_CONTROLLER_AXIS_##x}
#define CAMINUS(x)	{kInputTypeAxisMinus, SDL_CONTROLLER_AXIS_##x}
#define CBNULL()	{kInputTypeUnbound, 0}

const InputBinding kDefaultInputBindings[NUM_CONTROL_NEEDS] =
{
	[kNeed_Forward] =
	{
		.key = { SDL_SCANCODE_UP, SDL_SCANCODE_W },
		.pad = { CAMINUS(LEFTY), CB(DPAD_UP) },
	},

	[kNeed_Backward] =
	{
		.key = { SDL_SCANCODE_DOWN, SDL_SCANCODE_S },
		.pad = { CAPLUS(LEFTY), CB(DPAD_DOWN) },
	},

	[kNeed_TurnLeft] =
	{
		.key = { SDL_SCANCODE_LEFT, SDL_SCANCODE_A },
		.pad = { CAMINUS(LEFTX), CB(DPAD_LEFT) },
	},

	[kNeed_TurnRight] =
	{
		.key = { SDL_SCANCODE_RIGHT, SDL_SCANCODE_D },
		.pad = { CAPLUS(LEFTX), CB(DPAD_RIGHT) },
	},

	[kNeed_AutoWalk] =
	{
		.key = { SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT },
	},

	[kNeed_Jump] =
	{
		.key = { SDL_SCANCODE_SPACE },
		.pad = { CB(A) },
		.mouseButton = SDL_BUTTON_RIGHT,
	},

	[kNeed_Kick] =
	{
		.key = { SC_KICK1, SC_KICK2 },
		.pad = { CB(X) },
		.mouseButton = SDL_BUTTON_LEFT,
	},

	[kNeed_PickupDrop] =
	{
		.key = { SC_PICK1, SC_PICK2 },
		.pad = { CB(B) },
		.mouseButton = SDL_BUTTON_MIDDLE,
	},

	[kNeed_LaunchBuddy] =
	{
		.key = { SDL_SCANCODE_TAB },
		.pad = { CB(Y) },
		.mouseButton = SDL_BUTTON_X1,
	},

	[kNeed_CameraMode] =
	{
		.key = { SDL_SCANCODE_RETURN },
		.pad = { CB(RIGHTSTICK) },
		.mouseButton = SDL_BUTTON_X2,
	},

	[kNeed_CameraLeft] = { .key = { SDL_SCANCODE_COMMA } },
	[kNeed_CameraRight] = { .key = { SDL_SCANCODE_PERIOD } },

	[kNeed_CameraLeftPrecise] = { .pad = { CAMINUS(RIGHTX) } },
	[kNeed_CameraRightPrecise] = { .pad = { CAPLUS(RIGHTX) } },

	// -----------------------------------------------------------
	// Non-remappable UI bindings below

	[kNeed_UIUp] =
	{
		.key = { SDL_SCANCODE_UP, SDL_SCANCODE_W },
		.pad = { CB(DPAD_UP), CAMINUS(LEFTY) },
	},

	[kNeed_UIDown] =
	{
		.key = { SDL_SCANCODE_DOWN, SDL_SCANCODE_S },
		.pad = { CB(DPAD_DOWN), CAPLUS(LEFTY) },
	},

	[kNeed_UIPrev] =
	{
		.key = { SDL_SCANCODE_LEFT, SDL_SCANCODE_A },
		.pad = { CB(DPAD_LEFT), CAMINUS(LEFTX), CB(LEFTSHOULDER) },
	},

	[kNeed_UINext] =
	{
		.key = { SDL_SCANCODE_RIGHT, SDL_SCANCODE_D },
		.pad = { CB(DPAD_RIGHT), CAPLUS(LEFTX), CB(RIGHTSHOULDER) },
	},

	[kNeed_UIConfirm] =
	{
		.key = { SDL_SCANCODE_RETURN, SDL_SCANCODE_SPACE, SDL_SCANCODE_KP_ENTER },
		.pad = { CB(A) },
	},

	[kNeed_UIDelete] =
	{
		.key = { SDL_SCANCODE_DELETE, SDL_SCANCODE_BACKSPACE },
		.pad = { CB(X) },
	},

	[kNeed_UIStart] =
	{
		.pad = { CB(START) },
	},

	[kNeed_UIBack] =
	{
		.key = { SDL_SCANCODE_ESCAPE, SDL_SCANCODE_BACKSPACE },
		.pad = { CB(B), CB(BACK) },
		.mouseButton = SDL_BUTTON_X1
	},

	[kNeed_UIPause] =
	{
		.key = { SDL_SCANCODE_ESCAPE },
		.pad = { CB(START) },
	},

	[kNeed_UITextLeft]		= { .key = { SDL_SCANCODE_LEFT  }, .pad = { CB(DPAD_LEFT) , CB(LEFTSHOULDER) , CB(B), CAMINUS(LEFTX) }},
	[kNeed_UITextRight]		= { .key = { SDL_SCANCODE_RIGHT }, .pad = { CB(DPAD_RIGHT), CB(RIGHTSHOULDER), CB(A), CAPLUS(LEFTX) }},
	[kNeed_UITextRightOrDone]= { .pad = { CB(RIGHTSHOULDER), CB(A) }},
	[kNeed_UITextBksp]		= { .key = { SDL_SCANCODE_BACKSPACE }, .pad = { CB(X) } },
	[kNeed_UITextDone]		= { .key = { SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER }, .pad = { CB(START) } },
	[kNeed_UITextNextCh]	= { .pad = { CB(DPAD_UP), CAPLUS(LEFTY)} },
	[kNeed_UITextPrevCh]	= { .pad = { CB(DPAD_DOWN), CAMINUS(LEFTY) } },
};
