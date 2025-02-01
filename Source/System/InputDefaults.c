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
#define GB(x)		{kInputTypeButton, SDL_GAMEPAD_BUTTON_##x}
#define GAPLUS(x)	{kInputTypeAxisPlus, SDL_GAMEPAD_AXIS_##x}
#define GAMINUS(x)	{kInputTypeAxisMinus, SDL_GAMEPAD_AXIS_##x}
#define GBNULL()	{kInputTypeUnbound, 0}

const InputBinding kDefaultInputBindings[NUM_CONTROL_NEEDS] =
{
	[kNeed_Forward] =
	{
		.key = { SDL_SCANCODE_UP, SDL_SCANCODE_W },
		.pad = { GAMINUS(LEFTY), GB(DPAD_UP) },
	},

	[kNeed_Backward] =
	{
		.key = { SDL_SCANCODE_DOWN, SDL_SCANCODE_S },
		.pad = { GAPLUS(LEFTY), GB(DPAD_DOWN) },
	},

	[kNeed_TurnLeft] =
	{
		.key = { SDL_SCANCODE_LEFT, SDL_SCANCODE_A },
		.pad = { GAMINUS(LEFTX), GB(DPAD_LEFT) },
	},

	[kNeed_TurnRight] =
	{
		.key = { SDL_SCANCODE_RIGHT, SDL_SCANCODE_D },
		.pad = { GAPLUS(LEFTX), GB(DPAD_RIGHT) },
	},

	[kNeed_AutoWalk] =
	{
		.key = { SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT },
	},

	[kNeed_Jump] =
	{
		.key = { SDL_SCANCODE_SPACE },
		.pad = { GB(SOUTH) },
		.mouseButton = SDL_BUTTON_RIGHT,
	},

	[kNeed_Kick] =
	{
		.key = { SC_KICK1, SC_KICK2 },
		.pad = { GB(WEST) },
		.mouseButton = SDL_BUTTON_LEFT,
	},

	[kNeed_PickupDrop] =
	{
		.key = { SC_PICK1, SC_PICK2 },
		.pad = { GB(EAST) },
		.mouseButton = SDL_BUTTON_MIDDLE,
	},

	[kNeed_LaunchBuddy] =
	{
		.key = { SDL_SCANCODE_TAB },
		.pad = { GB(NORTH) },
		.mouseButton = SDL_BUTTON_X1,
	},

	[kNeed_CameraMode] =
	{
		.key = { SDL_SCANCODE_RETURN },
		.pad = { GB(RIGHT_STICK) },
		.mouseButton = SDL_BUTTON_X2,
	},

	[kNeed_CameraLeft] = { .key = { SDL_SCANCODE_COMMA } },
	[kNeed_CameraRight] = { .key = { SDL_SCANCODE_PERIOD } },

	[kNeed_CameraLeftPrecise] = { .pad = { GAMINUS(RIGHTX) } },
	[kNeed_CameraRightPrecise] = { .pad = { GAPLUS(RIGHTX) } },

	// -----------------------------------------------------------
	// Non-remappable UI bindings below

	[kNeed_UIUp] =
	{
		.key = { SDL_SCANCODE_UP, SDL_SCANCODE_W },
		.pad = { GB(DPAD_UP), GAMINUS(LEFTY) },
	},

	[kNeed_UIDown] =
	{
		.key = { SDL_SCANCODE_DOWN, SDL_SCANCODE_S },
		.pad = { GB(DPAD_DOWN), GAPLUS(LEFTY) },
	},

	[kNeed_UIPrev] =
	{
		.key = { SDL_SCANCODE_LEFT, SDL_SCANCODE_A },
		.pad = { GB(DPAD_LEFT), GAMINUS(LEFTX), GB(LEFT_SHOULDER) },
	},

	[kNeed_UINext] =
	{
		.key = { SDL_SCANCODE_RIGHT, SDL_SCANCODE_D },
		.pad = { GB(DPAD_RIGHT), GAPLUS(LEFTX), GB(RIGHT_SHOULDER) },
	},

	[kNeed_UIConfirm] =
	{
		.key = { SDL_SCANCODE_RETURN, SDL_SCANCODE_SPACE, SDL_SCANCODE_KP_ENTER },
		.pad = { GB(SOUTH) },
	},

	[kNeed_UIDelete] =
	{
		.key = { SDL_SCANCODE_DELETE, SDL_SCANCODE_BACKSPACE },
		.pad = { GB(WEST) },
	},

	[kNeed_UIStart] =
	{
		.pad = { GB(START) },
	},

	[kNeed_UIBack] =
	{
		.key = { SDL_SCANCODE_ESCAPE, SDL_SCANCODE_BACKSPACE },
		.pad = { GB(EAST), GB(BACK) },
		.mouseButton = SDL_BUTTON_X1
	},

	[kNeed_UIPause] =
	{
		.key = { SDL_SCANCODE_ESCAPE },
		.pad = { GB(START) },
	},

	[kNeed_UITextLeft]		= { .key = { SDL_SCANCODE_LEFT  }, .pad = { GB(DPAD_LEFT) , GB(LEFT_SHOULDER) , GB(EAST), GAMINUS(LEFTX) }},
	[kNeed_UITextRight]		= { .key = { SDL_SCANCODE_RIGHT }, .pad = { GB(DPAD_RIGHT), GB(RIGHT_SHOULDER), GB(SOUTH), GAPLUS(LEFTX) }},
	[kNeed_UITextRightOrDone]={ .pad = { GB(RIGHT_SHOULDER), GB(SOUTH) }},
	[kNeed_UITextBksp]		= { .key = { SDL_SCANCODE_BACKSPACE }, .pad = { GB(WEST) } },
	[kNeed_UITextDone]		= { .key = { SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER }, .pad = { GB(START) } },
	[kNeed_UITextNextCh]	= { .pad = { GB(DPAD_UP), GAPLUS(LEFTY)} },
	[kNeed_UITextPrevCh]	= { .pad = { GB(DPAD_DOWN), GAMINUS(LEFTY) } },
};
