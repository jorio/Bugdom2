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
	Rumble(1.0f, 1.0f, 300);
}

static void cb_ResetKeyBindings(void)
{
	for (int i = 0; i < NUM_REMAPPABLE_NEEDS; i++)
	{
		SDL_memcpy(gGamePrefs.bindings[i].key, kDefaultInputBindings[i].key, sizeof(gGamePrefs.bindings[i].key));
	}

	MyFlushEvents();
	PlayEffect(EFFECT_BOTTLECRACK);
	LayoutCurrentMenuAgain();
}

static void cb_ResetPadBindings(void)
{
	for (int i = 0; i < NUM_REMAPPABLE_NEEDS; i++)
	{
		SDL_memcpy(gGamePrefs.bindings[i].pad, kDefaultInputBindings[i].pad, sizeof(gGamePrefs.bindings[i].pad));
	}

	MyFlushEvents();
	PlayEffect(EFFECT_BOTTLECRACK);
	LayoutCurrentMenuAgain();
}

static void cb_ResetMouseBindings(void)
{
	for (int i = 0; i < NUM_REMAPPABLE_NEEDS; i++)
	{
		gGamePrefs.bindings[i].mouseButton = kDefaultInputBindings[i].mouseButton;
	}

	MyFlushEvents();
	PlayEffect(EFFECT_BOTTLECRACK);
	LayoutCurrentMenuAgain();
}

//static const char* GenerateVideoLabel(void)
//{
//	return glGetString(GL_RENDERER);
//}

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

static const MenuItem kSettingsMenuTree[] =
{
	{.id='sett'},
	{.type = kMITitle, .text = STR_SETTINGS},
	{.type = kMISpacer},
	{.type = kMIPick, .text = STR_VIDEO_SETTINGS, .next = 'vide' },
	{.type = kMISpacer},
	{.type = kMIPick, .text = STR_CONFIGURE_KEYBOARD, .next = 'keyb' },
	{.type = kMIPick, .text = STR_CONFIGURE_MOUSE, .next = 'mous' },
	{.type = kMIPick, .text = STR_CONFIGURE_GAMEPAD, .next = 'gpad' },
	{.type = kMISpacer},
	{
		.type = kMICycler,
		.text = STR_DIFFICULTY,
		.cycler =
		{
			.valuePtr = &gGamePrefs.kiddieMode,
			.numChoices = 2,
			.choices = {STR_DIFFICULTY_NORMAL, STR_DIFFICULTY_EASY},
		},
	},
	{
		.type = kMICycler,
		.text = STR_MUSIC,
		.callback = EnforceMusicPausePref,
		.cycler =
		{
			.valuePtr = &gGamePrefs.music,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},
	{
		.type = kMICycler,
		.text = STR_BUDDY_BUG_SOUND,
		.cycler =
		{
			.valuePtr = &gGamePrefs.buddyBugBuzz,
			.numChoices = 2,
			.choices = {STR_BUDDY_BUG_SILENT, STR_BUDDY_BUG_BUZZ},
		},
	},
	{
		.type = kMICycler,
		.text= STR_UI_SPACING,
		.cycler =
		{
			.valuePtr = &gGamePrefs.uiCentering,
			.numChoices = 2,
			.choices = {STR_UI_SPREAD, STR_UI_CENTERED},
		},
	},
	{
		.type = kMICycler,
		.text = STR_LANGUAGE,
		.callback = cb_SetLanguage,
		.cycler =
		{
			.valuePtr = &gGamePrefs.language,
			.numChoices = NUM_LANGUAGES,
			.generateChoiceString = GenerateCurrentLanguageName,
		},
	},
/*
	{
		.type = kMICycler,
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
	{.type = kMISpacer},
	{.type = kMIPick, .text = STR_BACK, .next = 'BACK'},

	{.id='vide'},
	{.type = kMITitle, .text = STR_VIDEO_SETTINGS},
//	{.type = kMISubtitle, .generateText = GenerateVideoLabel},
	{.type = kMISpacer},
	{
		.type = kMICycler,
		.text = STR_FULLSCREEN,
		.callback = SetFullscreenModeFromPrefs,
		.cycler =
		{
			.valuePtr = &gGamePrefs.fullscreen,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},
	{
		.type = kMICycler,
		.text = STR_VSYNC,
		.callback = SetFullscreenModeFromPrefs,
		.cycler =
		{
			.valuePtr = &gGamePrefs.vsync,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},
#if !(__APPLE__ && __x86_64__)		// On macOS, don't expose AA to old machines
	{
		.type = kMICycler,
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
		.type = kMICycler,
		.text = STR_PREFERRED_DISPLAY,
		.callback = SetFullscreenModeFromPrefs,
		.cycler =
		{
			.valuePtr = &gGamePrefs.monitorNum,
			.generateNumChoices = GenerateNumDisplays,
			.generateChoiceString = GenerateDisplayName,
		},
	},
	{
		.type = kMICycler,
		.text = STR_ANAGLYPH,
		.callback = cb_ChangeAnaglyphMode,
		.cycler =
		{
			.valuePtr = &gGamePrefs.anaglyph,
			.numChoices = 3,
			.choices = {STR_OFF, STR_ANAGLYPH_COLOR, STR_ANAGLYPH_MONOCHROME},
		},
	},
	{.type = kMISpacer},
	{.type = kMIPick, .text = STR_BACK, .next = 'BACK'},

	{.id='keyb'},
	{.type = kMITitle, .text = STR_CONFIGURE_KEYBOARD},
	{.type = kMISubtitle, .text = STR_CONFIGURE_KEYBOARD_HELP},
	{.type = kMISpacer},
	{.type = kMIKeyBinding, .kb = kNeed_Forward},
	{.type = kMIKeyBinding, .kb = kNeed_Backward},
	{.type = kMIKeyBinding, .kb = kNeed_TurnLeft},
	{.type = kMIKeyBinding, .kb = kNeed_TurnRight},
	{.type = kMIKeyBinding, .kb = kNeed_AutoWalk},
	{.type = kMISpacer},
	{.type = kMIKeyBinding, .kb = kNeed_Jump},
	{.type = kMIKeyBinding, .kb = kNeed_Kick},
	{.type = kMIKeyBinding, .kb = kNeed_PickupDrop},
	{.type = kMIKeyBinding, .kb = kNeed_LaunchBuddy},
	{.type = kMISpacer},
	{.type = kMIKeyBinding, .kb = kNeed_CameraLeft},
	{.type = kMIKeyBinding, .kb = kNeed_CameraRight},
	{.type = kMIKeyBinding, .kb = kNeed_CameraMode},
	{.type = kMISpacer},
	{.type = kMIPick, .text = STR_RESET_KEYBINDINGS, .callback = cb_ResetKeyBindings},
	{.type = kMISpacer},
	{.type = kMIPick, .text = STR_BACK, .next = 'BACK'},

	{.id='gpad'},
//	{.type = kMITitle, .text = STR_CONFIGURE_GAMEPAD},
//	{.type = kMISubtitle, .generateText = GenerateGamepadLabel },
	{.type = kMITitle, .generateText = GenerateGamepadLabel },
	{.type = kMISubtitle, .text = STR_CONFIGURE_GAMEPAD_HELP},
	{.type = kMISpacer },
	{.type = kMIPadBinding, .kb = kNeed_Forward },
	{.type = kMIPadBinding, .kb = kNeed_Backward },
	{.type = kMIPadBinding, .kb = kNeed_TurnLeft },
	{.type = kMIPadBinding, .kb = kNeed_TurnRight },
	{.type = kMISpacer },
	{.type = kMIPadBinding, .kb = kNeed_Jump },
	{.type = kMIPadBinding, .kb = kNeed_Kick },
	{.type = kMIPadBinding, .kb = kNeed_LaunchBuddy },
	{.type = kMIPadBinding, .kb = kNeed_PickupDrop },
	{.type = kMISpacer },
	{.type = kMIPadBinding, .kb = kNeed_CameraLeft },
	{.type = kMIPadBinding, .kb = kNeed_CameraRight },
	{.type = kMIPadBinding, .kb = kNeed_CameraMode },
	{.type = kMISpacer },
	{.type = kMIPick, .text = STR_RESET_KEYBINDINGS, .callback = cb_ResetPadBindings},
	{.type = kMISpacer },
	{
		.type = kMICycler,
		.text = STR_GAMEPAD_RUMBLE,
		.callback = cb_SetRumble,
		.cycler =
		{
			.valuePtr = &gGamePrefs.gamepadRumbleLevel,
			.numChoices = 1 + MAX_GAMEPAD_RUMBLE_LEVEL,
			.choices = {STR_GAMEPAD_RUMBLE_OFF, STR_GAMEPAD_RUMBLE_LOW, STR_GAMEPAD_RUMBLE_MID, STR_GAMEPAD_RUMBLE_HIGH},
		},
	},
	{.type = kMISpacer },
	{.type = kMIPick, .text = STR_BACK,  .next = 'BACK'},

	{.id='mous'},
	{.type = kMITitle, .text = STR_CONFIGURE_MOUSE},
	{.type = kMISpacer},
	/*
	{
		.type = kMICycler,
		.text = STR_MOUSE_CONTROL_TYPE,
		.cycler =
		{
			.valuePtr = &gGamePrefs.mouseControlsOtto,
			.numChoices = 2,
			.choices = {STR_MOUSE_CONTROLS_CAMERA, STR_MOUSE_CONTROLS_OTTO},
		},
	},
	{
		.type = kMICycler,
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
	{ .type = kMISpacer },
	 */
	{.type = kMIMouseBinding, .kb = kNeed_Jump },
	{.type = kMIMouseBinding, .kb = kNeed_Kick },
	{.type = kMIMouseBinding, .kb = kNeed_PickupDrop },
	{.type = kMIMouseBinding, .kb = kNeed_LaunchBuddy },
	{.type = kMIMouseBinding, .kb = kNeed_AutoWalk },
	{.type = kMIMouseBinding, .kb = kNeed_CameraMode },
	{.type = kMISpacer },
	{.type = kMIPick, .text = STR_RESET_KEYBINDINGS, .callback = cb_ResetMouseBindings },
	{.type = kMISpacer },
	{.type = kMIPick, .text = STR_BACK, .next = 'BACK'},
	{ .id=0 }		// end of menu tree
};

static const MenuItem kAntialiasingWarning[] =
{
	{.id='w_aa'},
	{.type = kMILabel, .text = STR_ANTIALIASING_CHANGE_WARNING },
	{.type = kMISpacer },
	{.type = kMISpacer },
	{.type = kMISpacer },
	{.type = kMIPick, .text = STR_OK, .next = 'BACK' },
	{.id = 0 },
};

static const MenuItem kAnaglyphWarning[] =
{
	{.id='w_ag'},
	{.type = kMILabel, .text = STR_ANAGLYPH_TOGGLE_WARNING },
	{.type = kMISpacer },
	{.type = kMISpacer },
	{.type = kMISpacer },
	{.type = kMIPick, .text = STR_OK, .next = 'BACK' },
	{.id = 0 },
};

#pragma mark -

/***************************************************************/
/*                          RUNNER                             */
/***************************************************************/

void DoSettingsOverlay(void (*moveCall)(void), void (*drawCall)(void))
{
	gAllowAudioKeys = false;					// don't interfere with keyboard binding

//	PlayEffect(MyRandomLong()&1? EFFECT_ACCENTDRONE1: EFFECT_ACCENTDRONE2);

	PrefsType gPreviousPrefs = gGamePrefs;

	StartMenu(kSettingsMenuTree, nil, moveCall, drawCall);

	// Save prefs if any changes
	if (0 != SDL_memcmp(&gGamePrefs, &gPreviousPrefs, sizeof(gGamePrefs)))
		SavePrefs();

	gAllowAudioKeys = true;

	// If user changed antialiasing setting, show warning
	if (gPreviousPrefs.antialiasingLevel != gGamePrefs.antialiasingLevel)
	{
		StartMenu(kAntialiasingWarning, nil, moveCall, drawCall);
	}

	// If user changed anaglyph setting, show warning
	if (gPreviousPrefs.anaglyph != gGamePrefs.anaglyph)
	{
		StartMenu(kAnaglyphWarning, nil, moveCall, drawCall);
	}
}
