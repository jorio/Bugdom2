// OTTO MATIC FILE SELECT SCREEN
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"
#include "menu.h"

bool DoFileScreen(int fileScreenType, void (*backgroundDrawRoutine)(void))
{
	bool isSave = fileScreenType == FILE_SCREEN_TYPE_SAVE;

	const int maxLabelLength = 256;
	char* labelBuffer = AllocPtrClear(maxLabelLength * (NUM_SAVE_SLOTS + 1));
	char* labelBufferPos = labelBuffer;

	MenuItem menu[NUM_SAVE_SLOTS+5+2];
	SDL_memset(menu, 0, sizeof(menu));

	int menuIndex = 0;

	menu[menuIndex++] = (MenuItem) { .id='file' };
	menu[menuIndex++] = (MenuItem) { .type=kMITitle, .text=(isSave ? STR_SAVE_GAME : STR_LOAD_GAME) };

	if (isSave)
	{
		LocalizeWithPlaceholder(STR_SAVE_WHERE, labelBufferPos, maxLabelLength, "%d", gLevelNum+2);
		menu[menuIndex++] = (MenuItem){ .type = kMISubtitle, .rawText=labelBufferPos };
		labelBufferPos += maxLabelLength;
	}

	menu[menuIndex++] = (MenuItem) { .type=kMISpacer };

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
				mi->type = kMIPick;
				mi->id = i;
				mi->next = 'EXIT';
			}
			else
			{
				mi->type = kMILabel;
			}
		}
		else
		{
			char dateBuf[64];
			SDL_DateTime dt = {0};
			SDL_TimeToDateTime(saveData.timestamp * 1e9, &dt, true);
			const char* month = Localize(dt.month - 1 + STR_JANUARY);
			int day = dt.day;
			int year = dt.year;

			switch (gGamePrefs.language)
			{
				case LANGUAGE_ENGLISH:
					SDL_snprintf(dateBuf, sizeof(dateBuf), "%s %d, %d", month, day, year);
					break;
				case LANGUAGE_GERMAN:
					SDL_snprintf(dateBuf, sizeof(dateBuf), "%d. %s %d", day, month, year);
					break;
				default:
					SDL_snprintf(dateBuf, sizeof(dateBuf), "%d %s %d", day, month, year);
					break;
			}

			SDL_snprintf(labelBufferPos, maxLabelLength,
				"%s %c:\t\t%s %d\t\t%s",
				Localize(STR_FILE), 'A' + i,
				Localize(STR_LEVEL), saveData.realLevel+1,
				dateBuf);

			mi->type = kMIPick;
			mi->id = i;
			mi->text = -1;
			mi->rawText = labelBufferPos;
			mi->next = 'EXIT';

			labelBufferPos += maxLabelLength;
		}
	}

	menu[menuIndex++].type = kMISpacer;

	menu[menuIndex++] = (MenuItem) {.type=kMIPick, .next='EXIT', .id=-1,
									.text = isSave ? STR_CONTINUE_WITHOUT_SAVING : STR_BACK };

	menu[menuIndex++] = (MenuItem) {.id=0};

	GAME_ASSERT(menuIndex <= (int) SDL_arraysize(menu));

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
