// OTTO MATIC LEVEL SELECT SCREEN
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"
#include "menu.h"

int DoLevelCheatDialog(void (*drawCall)(void))
{
	static const MenuItem kLevelCheatMenu[] =
	{
		{.type=kMenuItem_Title,		.rawText="Select a level"},
		{.type=kMenuItem_Spacer},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL1,		.pick=0},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL2,		.pick=1},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL3,		.pick=2},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL4,		.pick=3},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL5,		.pick=4},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL6,		.pick=5},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL7,		.pick=6},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL8,		.pick=7},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL9,		.pick=8},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL10,		.pick=9},
		{.type=kMenuItem_END_SENTINEL},
	};

	return StartMenu(kLevelCheatMenu, nil, nil, drawCall);
}
