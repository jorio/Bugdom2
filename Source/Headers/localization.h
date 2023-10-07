#pragma once

typedef enum GameLanguageID
{
	LANGUAGE_ILLEGAL = -1,
	LANGUAGE_ENGLISH = 0,
	LANGUAGE_FRENCH,
	LANGUAGE_GERMAN,
	LANGUAGE_SPANISH,
	LANGUAGE_ITALIAN,
	LANGUAGE_SWEDISH,
	LANGUAGE_DUTCH,
	NUM_LANGUAGES
} GameLanguageID;

typedef enum LocStrID
{
	STR_NULL					= 0,

	DIALOG_MESSAGE_NEEDSNAILSHELL,
	DIALOG_MESSAGE_GOTSNAILSHELL,

	DIALOG_MESSAGE_FINDSCARECROWHEAD,
	DIALOG_MESSAGE_PUTSCARECROWHEAD,
	DIALOG_MESSAGE_ATTACHEDSCARECROWHEAD,

	DIALOG_MESSAGE_FINDMARBLE,
	DIALOG_MESSAGE_BOWLMARBLE,
	DIALOG_MESSAGE_DONEBOWLING,

	DIALOG_MESSAGE_CHIPMUNK_MAP4ACORN,
	DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT4ACORN,
	DIALOG_MESSAGE_CHIPMUNK_MOUSETRAP,
	DIALOG_MESSAGE_CHIPMUNK_THANKS,
	DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT,

	DIALOG_MESSAGE_POOLWATER,
	DIALOG_MESSAGE_SMASHBERRIES,
	DIALOG_MESSAGE_SQUISHMORE,
	DIALOG_MESSAGE_SQUISHDONE,

	DIALOG_MESSAGE_DOGHOUSE,

	DIALOG_MESSAGE_GOTFLEAS,
	DIALOG_MESSAGE_GOTTICKS,
	DIALOG_MESSAGE_HAPPYDOG,
	DIALOG_MESSAGE_REMEMBERDOG,

	DIALOG_MESSAGE_PLUMBINGINTRO,

	DIALOG_MESSAGE_SLOTCAR,
	DIALOG_MESSAGE_RESCUEMICE,
	DIALOG_MESSAGE_RESCUEMICE2,
	DIALOG_MESSAGE_MICESAVED,
	DIALOG_MESSAGE_SLOTCARPLAYERWON,
	DIALOG_MESSAGE_SLOTCARTRYAGAIN,

	DIALOG_MESSAGE_DOPUZZLE,
	DIALOG_MESSAGE_DONEPUZZLE,

	DIALOG_MESSAGE_BOMBHILLS,
	DIALOG_MESSAGE_BOMBHILLS2,
	DIALOG_MESSAGE_HILLSDONE,

	DIALOG_MESSAGE_MOTHBALL,
	DIALOG_MESSAGE_SILICONDOOR,
	DIALOG_MESSAGE_GETREDCLOVERS,
	DIALOG_MESSAGE_GOTREDCLOVERS,

	DIALOG_MESSAGE_GOFISHING,
	DIALOG_MESSAGE_MOREFISH,
	DIALOG_MESSAGE_THANKSFISH,
	DIALOG_MESSAGE_GETFOOD,
	DIALOG_MESSAGE_MOREFOOD,
	DIALOG_MESSAGE_THANKSFOOD,

	DIALOG_MESSAGE_GETKINDLING,
	DIALOG_MESSAGE_MOREKINDLING,
	DIALOG_MESSAGE_LIGHTFIRE,
	DIALOG_MESSAGE_ENTERHIVE,
	DIALOG_MESSAGE_BOTTLEKEY,

	DIALOG_MESSAGE_MICEDROWN,
	DIALOG_MESSAGE_THANKSNODROWN,
	DIALOG_MESSAGE_GLIDER,
	DIALOG_MESSAGE_SODACAN,
	MAX_DIALOG_MESSAGES,

	STR_RESUME,
	STR_RETIRE,
	STR_QUIT,

	STR_LOAD_GAME,
	STR_SAVE_GAME,
	STR_SAVE_WHERE,
	STR_CONTINUE_WITHOUT_SAVING,
	STR_EMPTY_SLOT,
	STR_LEVEL,
	STR_FILE,

	STR_JANUARY,
	STR_FEBRUARY,
	STR_MARCH,
	STR_APRIL,
	STR_MAY,
	STR_JUNE,
	STR_JULY,
	STR_AUGUST,
	STR_SEPTEMBER,
	STR_OCTOBER,
	STR_NOVEMBER,
	STR_DECEMBER,

	STR_LEVEL1,
	STR_LEVEL2,
	STR_LEVEL3,
	STR_LEVEL4,
	STR_LEVEL5,
	STR_LEVEL6,
	STR_LEVEL7,
	STR_LEVEL8,
	STR_LEVEL9,
	STR_LEVEL10,

	STR_LANGUAGE_NAME,
	STR_OK,
	STR_BACK,
	STR_ON,
	STR_OFF,
	STR_SETTINGS,
	STR_VIDEO_SETTINGS,
	STR_DIFFICULTY,
	STR_DIFFICULTY_NORMAL,
	STR_DIFFICULTY_EASY,
	STR_MUSIC,
	STR_LANGUAGE,
	STR_UI_SPACING,
	STR_UI_SPREAD,
	STR_UI_CENTERED,
	STR_PREFERRED_DISPLAY,
	STR_DISPLAY,
	STR_FULLSCREEN,
	STR_ANTIALIASING_CHANGE_WARNING,
	STR_VSYNC,
	STR_ANTIALIASING,
	STR_MSAA_2X,
	STR_MSAA_4X,
	STR_MSAA_8X,
	STR_ANAGLYPH,
	STR_ANAGLYPH_COLOR,
	STR_ANAGLYPH_MONOCHROME,
	STR_ANAGLYPH_TOGGLE_WARNING,
	STR_BUDDY_BUG_SOUND,
	STR_BUDDY_BUG_BUZZ,
	STR_BUDDY_BUG_SILENT,
	STR_UNBOUND_PLACEHOLDER,
	STR_PRESS,
	STR_CLICK,
	STR_BUTTON,
	STR_MOUSE_BUTTON_LEFT,
	STR_MOUSE_BUTTON_MIDDLE,
	STR_MOUSE_BUTTON_RIGHT,
	STR_MOUSE_WHEEL_UP,
	STR_MOUSE_WHEEL_DOWN,
	STR_MOUSE_WHEEL_LEFT,
	STR_MOUSE_WHEEL_RIGHT,
	STR_CONFIGURE_KEYBOARD,
	STR_CONFIGURE_KEYBOARD_HELP,
	STR_CONFIGURE_KEYBOARD_HELP_CANCEL,
	STR_CONFIGURE_GAMEPAD,
	STR_CONFIGURE_GAMEPAD_HELP,
	STR_CONFIGURE_GAMEPAD_HELP_CANCEL,
	STR_CONFIGURE_MOUSE,
	STR_NO_GAMEPAD_DETECTED,
	STR_GAMEPAD_RUMBLE,
	STR_GAMEPAD_RUMBLE_OFF,
	STR_GAMEPAD_RUMBLE_LOW,
	STR_GAMEPAD_RUMBLE_MID,
	STR_GAMEPAD_RUMBLE_HIGH,
	STR_MOUSE_CONTROL_TYPE,
	STR_MOUSE_CONTROLS_CAMERA,
	STR_MOUSE_CONTROLS_SKIP,
	STR_MOUSE_AUTOWALK_HINT,
	STR_MOUSE_SENSITIVITY,
	STR_MOUSE_SENSITIVITY_1,
	STR_MOUSE_SENSITIVITY_2,
	STR_MOUSE_SENSITIVITY_3,
	STR_MOUSE_SENSITIVITY_4,
	STR_MOUSE_SENSITIVITY_5,
	STR_MOUSE_SENSITIVITY_6,
	STR_MOUSE_SENSITIVITY_7,
	STR_MOUSE_SENSITIVITY_8,
	STR_RESET_KEYBINDINGS,
	STR_KEYBINDING_DESCRIPTION_0,
	STR_KEYBINDING_DESCRIPTION_1,
	STR_KEYBINDING_DESCRIPTION_2,
	STR_KEYBINDING_DESCRIPTION_3,
	STR_KEYBINDING_DESCRIPTION_4,
	STR_KEYBINDING_DESCRIPTION_5,
	STR_KEYBINDING_DESCRIPTION_6,
	STR_KEYBINDING_DESCRIPTION_7,
	STR_KEYBINDING_DESCRIPTION_8,
	STR_KEYBINDING_DESCRIPTION_9,
	STR_KEYBINDING_DESCRIPTION_10,
	STR_KEYBINDING_DESCRIPTION_11,

	NUM_LOCALIZED_STRINGS,
} LocStrID;

void LoadLocalizedStrings(GameLanguageID languageID);
void DisposeLocalizedStrings(void);

const char* Localize(LocStrID stringID);
int LocalizeWithPlaceholder(LocStrID stringID, char* buf, size_t bufSize, const char* format, ...);

bool IsNativeEnglishSystem(void);
GameLanguageID GetBestLanguageIDFromSystemLocale(void);
