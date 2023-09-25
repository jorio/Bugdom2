/****************************/
/*   	HIGHSCORES.C    	*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupScoreScreen(void);
static void FreeScoreScreen(void);
static void DrawHighScoresCallback(void);
static void DrawScoreVerbage(void);
static void DrawHighScoresAndCursor(void);
static void StartEnterName(void);
static Boolean IsThisScoreInList(uint32_t score);
static short AddNewScore(uint32_t newScore);
static void SaveHighScores(void);
static void MoveScoreCyc(ObjNode *theNode);

/***************************/
/*    CONSTANTS            */
/***************************/

enum
{
	HIGHSCORES_ObjType_Cyc
};


enum
{
	HIGHSCORES_SObjType_EnterNameText,
	HIGHSCORES_SObjType_ScoreText,
	HIGHSCORES_SObjType_Cursor,
	HIGHSCORES_SObjType_COUNT,
};


#define MYSCORE_DIGIT_SPACING 	30.0f



/***************************/
/*    VARIABLES            */
/***************************/

static Str32	gHighScoresFileName = ":" PROJECT_NAME ":HighScores4";

HighScoreType	gHighScores[NUM_SCORES];

static	float	gFinalScoreTimer,gFinalScoreAlpha, gCursorFlux = 0;

static	short	gNewScoreSlot,gCursorIndex;

static	Boolean	gDrawScoreVerbage,gExitHighScores;


/*********************** NEW SCORE ***********************************/

void NewScore(void)
{
	if (gScore == 0)
		return;

	gAllowAudioKeys = false;					// dont interfere with name editing

			/* INIT */

	LoadHighScores();										// make sure current scores are loaded
	SetupScoreScreen();										// setup OGL
	MakeFadeEvent(true, 1);


			/* LOOP */

	CalcFramesPerSecond();
	UpdateInput();

	while(!gExitHighScores)
	{

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(DrawHighScoresCallback);

				/*****************************/
				/* SEE IF USER ENTERING NAME */
				/*****************************/

		if (!gDrawScoreVerbage)
		{
			if (IsKeyDown(SDL_SCANCODE_RETURN) || IsKeyDown(SDL_SCANCODE_KP_ENTER))
			{
				gExitHighScores = true;
			}
			else if (IsKeyDown(SDL_SCANCODE_LEFT))
			{
				if (gCursorIndex > 0)
					gCursorIndex--;
			}
			else if (IsKeyDown(SDL_SCANCODE_RIGHT))
			{
				if (gCursorIndex < (MAX_NAME_LENGTH-1)
					&& gHighScores[gNewScoreSlot].name[gCursorIndex])
				{
					gCursorIndex++;
				}
			}
			else if (IsKeyDown(SDL_SCANCODE_BACKSPACE))
			{
				if (gCursorIndex > 0)
				{
					gCursorIndex--;
					for (int i = gCursorIndex; i < MAX_NAME_LENGTH; i++)
						gHighScores[gNewScoreSlot].name[i] = gHighScores[gNewScoreSlot].name[i+1];
					gHighScores[gNewScoreSlot].name[MAX_NAME_LENGTH] = '\0';
				}
			}
			else if (gTextInput[0] && gCursorIndex < MAX_NAME_LENGTH)			// dont add anything more if maxxed out now
			{
				char theChar = gTextInput[0];
				if ((theChar >= 'a') && (theChar <= 'z'))					// see if convert lower case to upper case a..z
					theChar = 'A' + (theChar-'a');
				gHighScores[gNewScoreSlot].name[gCursorIndex] = theChar;
				gCursorIndex++;
			}
		}
	}


		/* CLEANUP */

	if (gNewScoreSlot != -1)						// if a new score was added then update the high scores file
		SaveHighScores();

	OGL_FadeOutScene(DrawHighScoresCallback, NULL);

	FreeScoreScreen();

	gAllowAudioKeys = true;
}


/********************* SETUP SCORE SCREEN **********************/

static void SetupScoreScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
ObjNode				*newObj;

//	PlaySong(SONG_HIGHSCORE, true);

	gDrawScoreVerbage = true;
	gExitHighScores = false;
	gFinalScoreAlpha = 1.0f;


		/* IF THIS WAS A SAVED GAME AND SCORE HASN'T CHANGED AND IS ALREADY IN LIST THEN DON'T ADD TO HIGH SCORES */

	if (gPlayingFromSavedGame && (gScore == gLoadedScore) && IsThisScoreInList(gScore))
	{
		gFinalScoreTimer = 4.0f;
		gNewScoreSlot = -1;
	}

			/* NOT SAVED GAME OR A BETTER SCORE THAN WAS LOADED OR ISN'T IN LIST YET */
	else
	{
		gNewScoreSlot = AddNewScore(gScore);				// try to add new score to high scores list
		if (gNewScoreSlot == -1)							// -1 if not added
			gFinalScoreTimer = 4.0f;
		else
			gFinalScoreTimer = 3.0f;
	}



			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.0;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 5000;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.z		= 800;
	viewDef.camera.from.y		= -350;

	OGL_SetupWindow(&viewDef, &gGameView);


				/************/
				/* LOAD ART */
				/************/

	InitSparkles();


			/* LOAD SPRITES */

	LoadSpriteGroupFromSeries(SPRITE_GROUP_LEVELSPECIFIC, HIGHSCORES_SObjType_COUNT, "HighScores");


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:highscores.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_HIGHSCORES);


			/* CYC */

	gNewObjectDefinition.group		= MODEL_GROUP_HIGHSCORES;
	gNewObjectDefinition.type 		= HIGHSCORES_ObjType_Cyc;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot 		= TERRAIN_SLOT+1;					// draw after terrain for better performance since terrain blocks much of the pixels
	gNewObjectDefinition.moveCall 	= MoveScoreCyc;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= gGameView->yon * .99f / 100.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawCyclorama;



}




/********************** FREE SCORE SCREEN **********************/

static void FreeScoreScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUND_BANK_BONUS);
}



/***************** DRAW HIGHSCORES CALLBACK *******************/

static void DrawHighScoresCallback(void)
{
	DrawObjects();
	DrawSparkles();											// draw light sparkles


			/* DRAW SPRITES */

	OGL_PushState();

	SetInfobarSpriteState();

	if (gDrawScoreVerbage)
		DrawScoreVerbage();
	else
		DrawHighScoresAndCursor();


	OGL_PopState();
	gGlobalMaterialFlags = 0;
	gGlobalTransparency = 1.0;
}


/********************* DRAW SCORE VERBAGE ****************************/

static void DrawScoreVerbage(void)
{
				/* SEE IF DONE */

	gFinalScoreTimer -= gFramesPerSecondFrac;
	if (gFinalScoreTimer <= 0.0f)
	{
		if (gNewScoreSlot != -1)							// see if bail or if let player enter name for high score
		{
			StartEnterName();
		}
		else
			gExitHighScores = true;
		return;
	}
	if (gFinalScoreTimer < 1.0f)							// fade out
		gFinalScoreAlpha = gFinalScoreTimer;


			/****************************/
			/* DRAW BONUS TOTAL VERBAGE */
			/****************************/

	gGlobalTransparency = gFinalScoreAlpha;
	DrawInfobarSprite2(320-150, 110, 300, SPRITE_GROUP_LEVELSPECIFIC, HIGHSCORES_SObjType_ScoreText);


			/**************/
			/* DRAW SCORE */
			/**************/

	char s[32];
	SDL_snprintf(s, sizeof(s), "%d", gScore);

	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 0;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	GameFont_DrawString(s, 320, 240, .8f, kTextMeshAlignCenter | kTextMeshAlignTop);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gGlobalTransparency = 1.0f;
	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 1;
}


/****************** DRAW HIGH SCORES AND CURSOR ***********************/

static void DrawHighScoresAndCursor(void)
{
float	y,cursorY,cursorX;
char	s[33];

	gFinalScoreAlpha += gFramesPerSecondFrac;						// fade in
	if (gFinalScoreAlpha > .99f)
		gFinalScoreAlpha = .99f;


 	gCursorFlux += gFramesPerSecondFrac * 10.0f;


			/****************************/
			/* DRAW ENTER NAME VERBAGE */
			/****************************/

			/* DRAW TEXT */

	gGlobalTransparency = gFinalScoreAlpha;
	DrawInfobarSprite2(320-250, 10, 500, SPRITE_GROUP_LEVELSPECIFIC, HIGHSCORES_SObjType_EnterNameText);


	gGlobalTransparency = gFinalScoreAlpha;

	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 0;

			/*****************/
			/* DRAW THE TEXT */
			/*****************/

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// make glow

	y = 120;
	for (int i = 0; i < NUM_SCORES; i++)
	{
		if (i == gNewScoreSlot)								// see if cursor will go on this line
		{
			cursorY = y;
			cursorX = 150.0f;

			// calc cursor position
			for (int j = 0; j < gCursorIndex && gHighScores[i].name[j]; j++)
			{
				const AtlasGlyph* g = GetAtlasSpriteInfo(ATLAS_GROUP_FONT1, gHighScores[i].name[j]);
				cursorX += g->xadv * .32f;
			}
		}

				/* DRAW NAME */

		DrawScoreText(gHighScores[i].name, 150, y);

				/* DRAW SCORE */

		SDL_snprintf(s, sizeof(s), "%09d", gHighScores[i].score);
		DrawScoreText(s, 350, y);

		y += SCORE_TEXT_SPACING * 1.3f;
	}

		/*******************/
		/* DRAW THE CURSOR */
		/*******************/

	if (gCursorIndex < MAX_NAME_LENGTH)						// dont draw if off the right side
	{
		gGlobalTransparency = (.3f + ((sin(gCursorFlux) + 1.0f) * .5f) * .699f) * gFinalScoreAlpha;
		DrawInfobarSprite2(cursorX, cursorY, SCORE_TEXT_SPACING * 1.5f, SPRITE_GROUP_LEVELSPECIFIC, HIGHSCORES_SObjType_Cursor);
	}



			/***********/
			/* CLEANUP */
			/***********/

	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 1;

	gGlobalTransparency = 1;
}



/************************* START ENTER NAME **********************************/

static void StartEnterName(void)
{
	gFinalScoreAlpha = 0;
	gDrawScoreVerbage = false;
	gCursorIndex = 0;
	MyFlushEvents();
}




#pragma mark -


/*********************** LOAD HIGH SCORES ********************************/

void LoadHighScores(void)
{
OSErr				iErr;
short				refNum;
FSSpec				file;
long				count;

				/* OPEN FILE */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gHighScoresFileName, &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);
	if (iErr == fnfErr)
		ClearHighScores();
	else
		GAME_ASSERT(!iErr);

	count = sizeof(HighScoreType) * NUM_SCORES;
	iErr = FSRead(refNum, &count, (Ptr) gHighScores);					// read data from file
	FSClose(refNum);

	if (iErr)
	{
		FSpDelete(&file);												// file is corrupt, so delete
		return;
	}

				/* SAFETY */

	for (int i = 0; i < NUM_SCORES; i++)
	{
		gHighScores[i].name[MAX_NAME_LENGTH] = '\0';
	}
}


/************************ SAVE HIGH SCORES ******************************/

static void SaveHighScores(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

	CheckPrefsFolder(true);

				/* CREATE BLANK FILE */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gHighScoresFileName, &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, kGameID, 'Skor', smSystemScript);					// create blank file
	if (iErr)
		goto err;


				/* OPEN FILE */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gHighScoresFileName, &file);
	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
err:
		DoAlert("Unable to Save High Scores file!");
		return;
	}

				/* WRITE DATA */

	count = sizeof(HighScoreType) * NUM_SCORES;
	FSWrite(refNum, &count, (Ptr) gHighScores);
	FSClose(refNum);

}


/**************** CLEAR HIGH SCORES **********************/

void ClearHighScores(void)
{
	SDL_memset(gHighScores, 0, sizeof(gHighScores));

	SaveHighScores();
}


/*************************** ADD NEW SCORE ****************************/
//
// Returns high score slot that score was inserted to or -1 if none
//

static short AddNewScore(uint32_t newScore)
{
short	slot,i;

			/* FIND INSERT SLOT */

	for (slot=0; slot < NUM_SCORES; slot++)
	{
		if (newScore > gHighScores[slot].score)
			goto	got_slot;
	}
	return(-1);


got_slot:
			/* INSERT INTO LIST */

	for (i = NUM_SCORES-1; i > slot; i--)						// make hole
		gHighScores[i] = gHighScores[i-1];
	gHighScores[slot].score = newScore;							// set score in structure
	SDL_zeroa(gHighScores[slot].name);							// clear name
	return(slot);
}


#pragma mark -

/****************** IS THIS SCORE IN LIST *********************/
//
// Returns True if this score value is anywhere in the high scores already
//

static Boolean IsThisScoreInList(uint32_t score)
{
short	slot;

	for (slot=0; slot < NUM_SCORES; slot++)
	{
		if (gHighScores[slot].score == score)
			return(true);
	}

	return(false);
}



/******************* MOVE SCORE CYC ************************/

static void MoveScoreCyc(ObjNode *theNode)
{
	theNode->Rot.y -= gFramesPerSecondFrac * .05f;

	UpdateObjectTransforms(theNode);


}





