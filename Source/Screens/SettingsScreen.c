// OTTO MATIC SETTINGS
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

/****************************/
/*    EXTERNALS            */
/****************************/

#include "game.h"
#include "menu.h"

/***************************************************************/
/*                       CALLBACKS                             */
/***************************************************************/

static void cb_SetLanguage(void)
{
	LoadLocalizedStrings(gGamePrefs.language);
	LayoutCurrentMenuAgain();
}

static void cb_SetRumble(void)
{
	if (gGamePrefs.gamepadRumble)
	{
		Rumble(1.0f, 1.0f, 500);
	}
}

static void cb_ResetKeyBindings(void)
{
	for (int i = 0; i < NUM_REMAPPABLE_NEEDS; i++)
	{
		SDL_memcpy(gGamePrefs.bindings[i].key, kDefaultInputBindings[i].key, sizeof(gGamePrefs.bindings[i].key));
	}

	MyFlushEvents();
	PlayEffect(EFFECT_PLANECRASH);
	LayoutCurrentMenuAgain();
}

static void cb_ResetPadBindings(void)
{
	for (int i = 0; i < NUM_REMAPPABLE_NEEDS; i++)
	{
		SDL_memcpy(gGamePrefs.bindings[i].pad, kDefaultInputBindings[i].pad, sizeof(gGamePrefs.bindings[i].pad));
	}

	MyFlushEvents();
	PlayEffect(EFFECT_PLANECRASH);
	LayoutCurrentMenuAgain();
}

static void cb_ResetMouseBindings(void)
{
	for (int i = 0; i < NUM_REMAPPABLE_NEEDS; i++)
	{
		gGamePrefs.bindings[i].mouseButton = kDefaultInputBindings[i].mouseButton;
	}

	MyFlushEvents();
	PlayEffect(EFFECT_PLANECRASH);
	LayoutCurrentMenuAgain();
}

static const char* GenerateGamepadLabel(void)
{
	SDL_GameController* controller = GetController();
	if (controller)
		return SDL_GameControllerName(controller);
	else
		return Localize(STR_NO_GAMEPAD_DETECTED);
}

static uint8_t GenerateNumDisplays(void)
{
	int numDisplays = SDL_GetNumVideoDisplays();
	return SDL_clamp(numDisplays, 1, 255);
}

static const char* GenerateDisplayName(char* buf, int bufSize, Byte value)
{
	SDL_snprintf(buf, bufSize, "%s %d", Localize(STR_DISPLAY), 1 + (int)value);
	return buf;
}

static const char* GenerateCurrentLanguageName(char* buf, int bufSize, Byte value)
{
	(void) buf;
	(void) bufSize;
	(void) value;
	return Localize(STR_LANGUAGE_NAME);
}

static void cb_ChangeAnaglyphMode(void)
{
	gAnaglyphPass = 0;
	for (int i = 0; i < 4; i++)
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClearColor(0,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		SDL_GL_SwapWindow(gSDLWindow);
	}
}

static void SetFullscreenModeFromPrefs(void)
{
	SetFullscreenMode(true);
}

/***************************************************************/
/*                     MENU DEFINITIONS                        */
/***************************************************************/

static const MenuItem gKeybindingMenu[] =
{
	{.type = kMenuItem_Title, .text = STR_CONFIGURE_KEYBOARD},
	{.type = kMenuItem_Subtitle, .text = STR_CONFIGURE_KEYBOARD_HELP},
	{.type = kMenuItem_Spacer},

	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Forward },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Backward },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_TurnLeft },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_TurnRight },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_AutoWalk },
	
	{ .type = kMenuItem_Spacer },

	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Jump },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Kick },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_PickupDrop },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_LaunchBuddy },

	{ .type = kMenuItem_Spacer },

	{ .type = kMenuItem_KeyBinding, .kb = kNeed_CameraLeft },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_CameraRight },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_CameraMode },

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_RESET_KEYBINDINGS,
		.action = { .callback = cb_ResetKeyBindings },
	},

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_BACK,
		.action = { .callback = MenuCallback_Back },
	},

	{ .type = kMenuItem_END_SENTINEL }
};

static const MenuItem gGamepadMenu[] =
{
	{.type = kMenuItem_Title, .text = STR_CONFIGURE_GAMEPAD},
	{.type = kMenuItem_Subtitle, .generateText = GenerateGamepadLabel },
	{.type = kMenuItem_Subtitle, .text = STR_CONFIGURE_GAMEPAD_HELP},
	
	{.type = kMenuItem_Spacer },

	{ .type = kMenuItem_PadBinding, .kb = kNeed_Forward },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_Backward },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_TurnLeft },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_TurnRight },
	
	{.type = kMenuItem_Spacer },

	{ .type = kMenuItem_PadBinding, .kb = kNeed_Jump },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_Kick },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_LaunchBuddy },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_PickupDrop },
	
	
	{ .type = kMenuItem_Spacer },

	{ .type = kMenuItem_PadBinding, .kb = kNeed_CameraLeft },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_CameraRight },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_CameraMode },

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_RESET_KEYBINDINGS,
		.action = { .callback = cb_ResetPadBindings },
	},

#if 0
	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Cycler,
		.text = STR_GAMEPAD_RUMBLE,
		.cycler =
		{
			.callback = cb_SetRumble,
			.valuePtr = &gGamePrefs.gamepadRumble,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},
#endif

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_BACK,
		.action = { .callback = MenuCallback_Back },
	},

	{ .type = kMenuItem_END_SENTINEL }
};

static const MenuItem gMouseMenu[] =
{
	{.type = kMenuItem_Title, .text = STR_CONFIGURE_MOUSE},
	{.type = kMenuItem_Spacer},

	/*
	{
		.type = kMenuItem_Cycler,
		.text = STR_MOUSE_CONTROL_TYPE,
		.cycler =
		{
			.valuePtr = &gGamePrefs.mouseControlsOtto,
			.numChoices = 2,
			.choices = {STR_MOUSE_CONTROLS_CAMERA, STR_MOUSE_CONTROLS_OTTO},
		},
	},

	{
		.type = kMenuItem_Cycler,
		.text = STR_MOUSE_SENSITIVITY,
		.cycler =
		{
			.valuePtr = &gGamePrefs.mouseSensitivityLevel,
			.numChoices = NUM_MOUSE_SENSITIVITY_LEVELS,
			.choices =
			{
				STR_MOUSE_SENSITIVITY_1,
				STR_MOUSE_SENSITIVITY_2,
				STR_MOUSE_SENSITIVITY_3,
				STR_MOUSE_SENSITIVITY_4,
				STR_MOUSE_SENSITIVITY_5,
				STR_MOUSE_SENSITIVITY_6,
				STR_MOUSE_SENSITIVITY_7,
				STR_MOUSE_SENSITIVITY_8,
			},
		},
	},

	{ .type = kMenuItem_Spacer },
	 */

	{ .type = kMenuItem_MouseBinding, .kb = kNeed_Jump },
	{ .type = kMenuItem_MouseBinding, .kb = kNeed_Kick },
	{ .type = kMenuItem_MouseBinding, .kb = kNeed_PickupDrop },
	{ .type = kMenuItem_MouseBinding, .kb = kNeed_LaunchBuddy },
	{ .type = kMenuItem_MouseBinding, .kb = kNeed_AutoWalk },
	{ .type = kMenuItem_MouseBinding, .kb = kNeed_CameraMode },
	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_RESET_KEYBINDINGS,
		.action = { .callback = cb_ResetMouseBindings },
	},

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_BACK,
		.action = { .callback = MenuCallback_Back },
	},

	{ .type = kMenuItem_END_SENTINEL }
};

static const MenuItem gSettingsMenu[] =
{
	{.type = kMenuItem_Title, .text = STR_SETTINGS},
	{.type = kMenuItem_Spacer},


	{
		.type = kMenuItem_Submenu,
		.text = STR_CONFIGURE_KEYBOARD,
		.submenu = {.menu = gKeybindingMenu},
	},

	{
		.type = kMenuItem_Submenu,
		.text = STR_CONFIGURE_MOUSE,
		.submenu = {.menu = gMouseMenu},
	},

	{
		.type = kMenuItem_Submenu,
		.text = STR_CONFIGURE_GAMEPAD,
		.submenu = {.menu = gGamepadMenu},
	},

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Cycler,
		.text = STR_DIFFICULTY,
		.cycler =
		{
			.valuePtr = &gGamePrefs.kiddieMode,
			.numChoices = 2,
			.choices = {STR_DIFFICULTY_NORMAL, STR_DIFFICULTY_EASY},
		},
	},
	{
		.type = kMenuItem_Cycler,
		.text = STR_MUSIC,
		.cycler =
		{
			.callback = EnforceMusicPausePref,
			.valuePtr = &gGamePrefs.music,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},

	{
		.type = kMenuItem_Cycler,
		.text= STR_UI_SPACING,
		.cycler =
		{
			.valuePtr = &gGamePrefs.uiCentering,
			.numChoices = 2,
			.choices = {STR_UI_SPREAD, STR_UI_CENTERED},
		},
	},
/*
	{
		.type = kMenuItem_Cycler,
		.text = STR_UI_SCALE,
		.cycler =
		{
			.valuePtr = &gGamePrefs.uiScaleLevel,
			.numChoices = NUM_UI_SCALE_LEVELS,
			.choices =
			{
				STR_MOUSE_SENSITIVITY_1,
				STR_MOUSE_SENSITIVITY_2,
				STR_MOUSE_SENSITIVITY_3,
				STR_MOUSE_SENSITIVITY_4,
				STR_MOUSE_SENSITIVITY_5,
				STR_MOUSE_SENSITIVITY_6,
				STR_MOUSE_SENSITIVITY_7,
				STR_MOUSE_SENSITIVITY_8,
			},
		},
	},
*/

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Cycler,
		.text = STR_FULLSCREEN,
		.cycler =
		{
			.callback = SetFullscreenModeFromPrefs,
			.valuePtr = &gGamePrefs.fullscreen,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},

	{
		.type = kMenuItem_Cycler,
		.text = STR_VSYNC,
		.cycler =
		{
			.callback = SetFullscreenModeFromPrefs,
			.valuePtr = &gGamePrefs.vsync,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},

#if !(__APPLE__ && __x86_64__)		// On macOS, don't expose AA to old machines
	{
		.type = kMenuItem_Cycler,
		.text = STR_ANTIALIASING,
		.cycler =
		{
			.valuePtr = &gGamePrefs.antialiasingLevel,
			.numChoices = 4,
			.choices = {STR_OFF, STR_MSAA_2X, STR_MSAA_4X, STR_MSAA_8X},
		}
	},
#endif

	{
		.type = kMenuItem_Cycler,
		.text = STR_PREFERRED_DISPLAY,
		.cycler =
		{
			.callback = SetFullscreenModeFromPrefs,
			.valuePtr = &gGamePrefs.monitorNum,
			.generateNumChoices = GenerateNumDisplays,
			.generateChoiceString = GenerateDisplayName,
		},
	},

	{
		.type = kMenuItem_Cycler,
		.text = STR_ANAGLYPH,
		.cycler =
		{
			.callback = cb_ChangeAnaglyphMode,
			.valuePtr = &gGamePrefs.anaglyph,
			.numChoices = 3,
			.choices = {STR_OFF, STR_ANAGLYPH_COLOR, STR_ANAGLYPH_MONOCHROME},
		},
	},

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Cycler,
		.text = STR_LANGUAGE,
		.cycler =
		{
			.callback = cb_SetLanguage,
			.valuePtr = &gGamePrefs.language,
			.numChoices = NUM_LANGUAGES,
			.generateChoiceString = GenerateCurrentLanguageName,
		},
	},


	{ .type = kMenuItem_Spacer },
	{
		.type = kMenuItem_Action,
		.text = STR_BACK,
		.action = { .callback = MenuCallback_Back },
	},

	{ .type = kMenuItem_END_SENTINEL }
};

static const MenuItem kAntialiasingWarning[] =
{
	{ .type = kMenuItem_Label, .text = STR_ANTIALIASING_CHANGE_WARNING },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Action, .text = STR_OK, .action = { .callback = MenuCallback_Back } },
	{ .type = kMenuItem_END_SENTINEL },
};

static const MenuItem kAnaglyphWarning[] =
{
	{ .type = kMenuItem_Label, .text = STR_ANAGLYPH_TOGGLE_WARNING },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Action, .text = STR_OK, .action = { .callback = MenuCallback_Back } },
	{ .type = kMenuItem_END_SENTINEL },
};

#pragma mark -

/***************************************************************/
/*                          RUNNER                             */
/***************************************************************/

void DoSettingsOverlay(void (*updateRoutine)(void),
					   void (*backgroundDrawRoutine)(void))
{
	gAllowAudioKeys = false;					// don't interfere with keyboard binding

//	PlayEffect(MyRandomLong()&1? EFFECT_ACCENTDRONE1: EFFECT_ACCENTDRONE2);

	PrefsType gPreviousPrefs = gGamePrefs;

	StartMenu(gSettingsMenu, nil, updateRoutine, backgroundDrawRoutine);

	// Save prefs if any changes
	if (0 != SDL_memcmp(&gGamePrefs, &gPreviousPrefs, sizeof(gGamePrefs)))
		SavePrefs();

	gAllowAudioKeys = true;

	// If user changed antialiasing setting, show warning
	if (gPreviousPrefs.antialiasingLevel != gGamePrefs.antialiasingLevel)
	{
		StartMenu(kAntialiasingWarning, nil, updateRoutine, backgroundDrawRoutine);
	}

	// If user changed anaglyph setting, show warning
	if (gPreviousPrefs.anaglyph != gGamePrefs.anaglyph)
	{
		StartMenu(kAnaglyphWarning, nil, updateRoutine, backgroundDrawRoutine);
	}
}
