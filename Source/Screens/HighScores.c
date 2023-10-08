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
static void DoTextEntry(char* myName);
static void DrawHighScoresCallback(ObjNode* theNode);
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



#define MYSCORE_DIGIT_SPACING 	30.0f



/***************************/
/*    VARIABLES            */
/***************************/

static const char*	gHighScoresFileName = ":" PROJECT_NAME ":HighScores4";

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

	MakeNewDriverObject(PARTICLE_SLOT-1, DrawHighScoresCallback, NULL);

			/* CLEAR NAME WITH SPACES */

	SDL_memset(gHighScores[gNewScoreSlot].name, ' ', MAX_NAME_LENGTH);
	gHighScores[gNewScoreSlot].name[MAX_NAME_LENGTH] = '\0';

			/* LOOP */
	
	ResetFramesPerSecond();

	while(!gExitHighScores)
	{
		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(DrawObjects);

				/*****************************/
				/* SEE IF USER ENTERING NAME */
				/*****************************/

		if (!gDrawScoreVerbage)
		{
			DoTextEntry(gHighScores[gNewScoreSlot].name);
		}
	}


		/* CLEANUP */

	if (gNewScoreSlot != -1)						// if a new score was added then update the high scores file
		SaveHighScores();

	OGL_FadeOutScene(DrawObjects, NULL);

	FreeScoreScreen();

	gAllowAudioKeys = true;
}



/********************* DO TEXT ENTRY **********************/

static void DoTextEntry(char* myName)
{
	static	float		gTimeSinceKeyRepeat = 0;
	static const char*	kGamepadTextEntryCharset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	gTimeSinceKeyRepeat += gFramesPerSecondFrac;

	if (IsNeedDown(kNeed_UITextDone)
		|| (IsNeedDown(kNeed_UITextRightOrDone) && gCursorIndex == MAX_NAME_LENGTH-1))
	{
		PlayEffect(EFFECT_GETPOW);
		gExitHighScores = true;
	}
	else if (IsNeedDown(kNeed_UITextLeft))
	{
		if (gCursorIndex > 0)
			gCursorIndex--;
	}
	else if (IsNeedDown(kNeed_UITextRight))
	{
		if (gCursorIndex < (MAX_NAME_LENGTH-1))
		{
			gCursorIndex++;
		}
	}
	else if (IsNeedDown(kNeed_UITextBksp))
	{
		if (gCursorIndex > 0)
		{
			gCursorIndex--;

			for (int i = gCursorIndex; i < MAX_NAME_LENGTH; i++)
				myName[i] = myName[i+1];

			myName[MAX_NAME_LENGTH-1] = ' ';

			PlayEffect(EFFECT_FLYGOTKICKED);
		}
	}
	else if (gTextInput[0]
			 && gTextInput[0] >= ' '
			 && gTextInput[0] <= '~'						// only ASCII to avoid dealing with utf-8
			 && gCursorIndex < MAX_NAME_LENGTH)			// dont add anything more if maxxed out now
	{
		PlayEffect_Parms(EFFECT_ACORNKICKED, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE + (RandomFloat2() * 0x3000));
		char theChar = gTextInput[0];
		if ((theChar >= 'a') && (theChar <= 'z'))					// see if convert lower case to upper case a..z
			theChar = 'A' + (theChar-'a');
		myName[gCursorIndex] = theChar;
		gCursorIndex++;
	}
	else if (IsNeedDown(kNeed_UITextNextCh) ||
			 (IsNeedHeld(kNeed_UITextNextCh) && gTimeSinceKeyRepeat > .125f))
	{
		if (gCursorIndex < MAX_NAME_LENGTH)
		{
			gTimeSinceKeyRepeat = 0;

			char c = myName[gCursorIndex];

			const char* posInCharset = strchr(kGamepadTextEntryCharset, c);

			if (!posInCharset)
			{
				c = kGamepadTextEntryCharset[0];		// fall back to first allowed char
			}
			else
			{
				c = *(posInCharset + 1);				// advance to next allowed char
				if (!c)									// reached end of allowed charset
					c = kGamepadTextEntryCharset[0];	// fall back to first allowed char
			}

			myName[gCursorIndex] = c;
			gCursorFlux = -PI/2;

			PlayEffect_Parms(EFFECT_ACORNKICKED, FULL_CHANNEL_VOLUME/2, FULL_CHANNEL_VOLUME/2, NORMAL_CHANNEL_RATE + (RandomFloat2() * 0x3000));
		}
	}
	else if (IsNeedDown(kNeed_UITextPrevCh) ||
			 (IsNeedHeld(kNeed_UITextPrevCh) && gTimeSinceKeyRepeat > .125f))
	{
		if (gCursorIndex < MAX_NAME_LENGTH)
		{
			gTimeSinceKeyRepeat = 0;

			char c = myName[gCursorIndex];

			const char* posInCharset = strchr(kGamepadTextEntryCharset, c);

			if (!posInCharset)
			{
				c = kGamepadTextEntryCharset[0];		// fall back to first allowed char
			}
			else
			{
				if (posInCharset == kGamepadTextEntryCharset)
					posInCharset = kGamepadTextEntryCharset + strlen(kGamepadTextEntryCharset) - 1;
				else
					posInCharset--;

				c = *posInCharset;
			}

			myName[gCursorIndex] = c;
			gCursorFlux = -PI/2;

			PlayEffect_Parms(EFFECT_ACORNKICKED, FULL_CHANNEL_VOLUME/2, FULL_CHANNEL_VOLUME/2, NORMAL_CHANNEL_RATE + (RandomFloat2() * 0x3000));
		}
	}
}



/********************* SETUP SCORE SCREEN **********************/

static void SetupScoreScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
ObjNode				*newObj;

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

	InitEffects();

			/* LOAD SPRITES */

	LoadSpriteGroupFromSeries(SPRITE_GROUP_LEVELSPECIFIC, BONUS_SObjType_COUNT, "Bonus");


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:HighScores.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_HIGHSCORES);


			/* CYC */

	gNewObjectDefinition.group		= MODEL_GROUP_HIGHSCORES;
	gNewObjectDefinition.type 		= HIGHSCORES_ObjType_Cyc;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot 		= CYC_SLOT;
	gNewObjectDefinition.moveCall 	= MoveScoreCyc;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= gGameView.yon * .99f / 100.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawCyclorama;



}




/********************** FREE SCORE SCREEN **********************/

static void FreeScoreScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeEffects();
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);
	DisposeAllBG3DContainers();
}



/***************** DRAW HIGHSCORES CALLBACK *******************/

static void DrawHighScoresCallback(ObjNode* theNode)
{
	(void) theNode;


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
	DrawInfobarSprite2(320-150, 110, 300, SPRITE_GROUP_LEVELSPECIFIC, BONUS_SObjType_Score);


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




			/****************************/
			/* DRAW ENTER NAME VERBAGE */
			/****************************/

			/* DRAW TEXT */

	gGlobalTransparency = gFinalScoreAlpha;
	DrawInfobarSprite2(320-250, 10, 500, SPRITE_GROUP_LEVELSPECIFIC, BONUS_SObjType_EnterNameText);


	gGlobalTransparency = gFinalScoreAlpha;

	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 0;

			/*****************/
			/* DRAW THE TEXT */
			/*****************/

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// make glow

	y = 120;
	cursorX = 150.0f;
	cursorY = y;
	for (int i = 0; i < NUM_SCORES; i++)
	{
		if (i == gNewScoreSlot)								// see if cursor will go on this line
		{
			cursorY = y;

			// calc cursor position
			for (int j = 0; j < gCursorIndex && gHighScores[i].name[j]; j++)
			{
				const AtlasGlyph* g = GetAtlasSpriteInfo(ATLAS_GROUP_FONT1, gHighScores[i].name[j]);
				if (!g)
					g = GetAtlasSpriteInfo(ATLAS_GROUP_FONT1, '?');
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
		gGlobalTransparency = (.3f + ((sinf(gCursorFlux) + 1.0f) * .5f) * .699f) * gFinalScoreAlpha;
		DrawInfobarSprite2(cursorX, cursorY, SCORE_TEXT_SPACING * 1.5f, SPRITE_GROUP_LEVELSPECIFIC, BONUS_SObjType_Cursor);
	}



			/***********/
			/* CLEANUP */
			/***********/

	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 1;

	gGlobalTransparency = 1;

	gCursorFlux += gFramesPerSecondFrac * 10.0f;
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





