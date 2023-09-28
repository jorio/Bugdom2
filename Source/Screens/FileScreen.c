// OTTO MATIC FILE SELECT SCREEN
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"
#include "menu.h"
#include <time.h>

bool DoFileScreen(int fileScreenType, void (*backgroundDrawRoutine)(void))
{
	bool isSave = fileScreenType == FILE_SCREEN_TYPE_SAVE;

	const int maxLabelLength = 256;
	char* labelBuffer = AllocPtrClear(maxLabelLength * (NUM_SAVE_SLOTS + 1));
	char* labelBufferPos = labelBuffer;

	MenuItem menu[NUM_SAVE_SLOTS+5+1];
	memset(menu, 0, sizeof(menu));

	int menuIndex = 0;

	menu[menuIndex++] = (MenuItem) { .type=kMenuItem_Title, .text=(isSave ? STR_SAVE_GAME : STR_LOAD_GAME) };

	if (isSave)
	{
		LocalizeWithPlaceholder(STR_SAVE_WHERE, labelBufferPos, maxLabelLength, "%d", gLevelNum+2);
		menu[menuIndex++] = (MenuItem){ .type = kMenuItem_Subtitle, .rawText=labelBufferPos };
		labelBufferPos += maxLabelLength;
	}

	menu[menuIndex++] = (MenuItem) { .type=kMenuItem_Spacer };

	for (int i = 0; i < NUM_SAVE_SLOTS; i++)
	{
		SaveGameType saveData;

		MenuItem* mi = &menu[menuIndex++];

		if (!LoadSavedGameStruct(i, &saveData))
		{
			SDL_snprintf(labelBufferPos, maxLabelLength,
				"%s %c:\t\t%s", Localize(STR_FILE), 'A' + i, Localize(STR_EMPTY_SLOT));

			mi->rawText = labelBufferPos;
			labelBufferPos += maxLabelLength;

			if (isSave)
			{
				mi->type = kMenuItem_Pick;
				mi->pick = i;
			}
			else
			{
				mi->type = kMenuItem_Label;
			}
		}
		else
		{
			int month = 0;
			int day = 0;
			int year = 0;

			time_t timestamp = (time_t) saveData.timestamp;
			struct tm *timeinfo = localtime(&timestamp);

			if (timeinfo)
			{
				month = timeinfo->tm_mon;
				day = timeinfo->tm_mday;
				year = 1900 + timeinfo->tm_year;
			}

			SDL_snprintf(labelBufferPos, maxLabelLength,
				"%s %c:\t\t%s %d\t\t%d %s %d",
				Localize(STR_FILE), 'A' + i,
				Localize(STR_LEVEL), saveData.realLevel+1,
				day, Localize(month + STR_JANUARY), year);

			mi->type = kMenuItem_Pick;
			mi->pick = i;
			mi->text = -1;
			mi->rawText = labelBufferPos;

			labelBufferPos += maxLabelLength;
		}
	}

	menu[menuIndex++].type = kMenuItem_Spacer;

	menu[menuIndex++] = (MenuItem) {.type=kMenuItem_Action, .action.callback = MenuCallback_Back,
									.text = isSave ? STR_CONTINUE_WITHOUT_SAVING : STR_BACK };

	menu[menuIndex++].type = kMenuItem_END_SENTINEL;

	GAME_ASSERT(menuIndex <= sizeof(menu)/sizeof(menu[0]));

	//-----------------------------------

	int pickedSlot = StartMenu(menu, nil, nil, backgroundDrawRoutine);

	SafeDisposePtr(labelBuffer);

	if (pickedSlot >= 0)
	{
		if (isSave)
			SaveGame(pickedSlot);
		else
			LoadSavedGame(pickedSlot);
		return true;
	}

	return false;
}