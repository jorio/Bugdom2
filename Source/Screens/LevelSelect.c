// OTTO MATIC LEVEL SELECT SCREEN
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"
#include "menu.h"

int DoLevelCheatDialog(void (*drawCall)(void))
{
	static const MenuItem kLevelCheatMenu[] =
	{
		{.id='lvlc'},
		{.type=kMITitle,	.rawText="Select a level"},
		{.type=kMISpacer},
		{.type=kMIPick,		.text=STR_LEVEL1,		.id=0,		.next = 'EXIT'},
		{.type=kMIPick,		.text=STR_LEVEL2,		.id=1,		.next = 'EXIT'},
		{.type=kMIPick,		.text=STR_LEVEL3,		.id=2,		.next = 'EXIT'},
		{.type=kMIPick,		.text=STR_LEVEL4,		.id=3,		.next = 'EXIT'},
		{.type=kMIPick,		.text=STR_LEVEL5,		.id=4,		.next = 'EXIT'},
		{.type=kMIPick,		.text=STR_LEVEL6,		.id=5,		.next = 'EXIT'},
		{.type=kMIPick,		.text=STR_LEVEL7,		.id=6,		.next = 'EXIT'},
		{.type=kMIPick,		.text=STR_LEVEL8,		.id=7,		.next = 'EXIT'},
		{.type=kMIPick,		.text=STR_LEVEL9,		.id=8,		.next = 'EXIT'},
		{.type=kMIPick,		.text=STR_LEVEL10,		.id=9,		.next = 'EXIT'},
		{.id=0},
	};

	return StartMenu(kLevelCheatMenu, nil, nil, drawCall);
}
