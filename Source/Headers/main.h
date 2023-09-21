//
// main.h
//

#pragma once

#define	GAME_FOV		1.2f

#define	DEFAULT_ANAGLYPH_R	0xd8
#define	DEFAULT_ANAGLYPH_G	0x90
#define	DEFAULT_ANAGLYPH_B	0xff


enum
{
	LEVEL_NUM_GNOMEGARDEN = 0,
	LEVEL_NUM_SIDEWALK,
	LEVEL_NUM_FIDO,
	LEVEL_NUM_PLUMBING,
	LEVEL_NUM_PLAYROOM,

	LEVEL_NUM_CLOSET,
	LEVEL_NUM_GUTTER,
	LEVEL_NUM_GARBAGE,
	LEVEL_NUM_BALSA,
	LEVEL_NUM_PARK,

	NUM_LEVELS
};


enum
{
	LANGUAGE_ENGLISH = 0,
	LANGUAGE_FRENCH,
	LANGUAGE_GERMAN,
	LANGUAGE_SPANISH,
	LANGUAGE_ITALIAN,
	LANGUAGE_SWEDISH,
	LANGUAGE_DUTCH,

	MAX_LANGUAGES
};

//=================================================

void GameMain(void);
void ToolBoxInit(void);
void MoveEverything(void);
void InitDefaultPrefs(void);
void DrawArea(OGLSetupOutputType *setupInfo);
void StartLevelCompletion(float coolDownTimer);
