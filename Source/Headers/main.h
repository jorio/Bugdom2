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

//=================================================

OSErr CheckPrefsFolder(bool createIt);
void GameMain(void);
void MoveEverything(void);
void InitDefaultPrefs(void);
void DrawArea(void);
void StartLevelCompletion(float coolDownTimer);
