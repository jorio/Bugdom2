/****************************/
/*   	HIGHSCORES.C    	*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float			gFramesPerSecond,gFramesPerSecondFrac,gGlobalTransparency;
extern	short	gPrefsFolderVRefNum;
extern	long	gPrefsFolderDirID;
extern	FSSpec	gDataSpec;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	u_long			gScore,gGlobalMaterialFlags,gLoadedScore;
extern	Boolean			gPlayingFromSavedGame,gAllowAudioKeys;
extern	AGLContext		gAGLContext;
extern	OGLColorRGB			gGlobalColorFilter;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupScoreScreen(void);
static void FreeScoreScreen(void);
static void DrawHighScoresCallback(OGLSetupOutputType *info);
static void DrawScoreVerbage(OGLSetupOutputType *info);
static void DrawHighScoresAndCursor(OGLSetupOutputType *info);
static void SetHighScoresSpriteState(void);
static void StartEnterName(void);
static Boolean IsThisScoreInList(u_long score);
static short AddNewScore(u_long newScore);
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
	HIGHSCORES_SObjType_ScoreText
};


#define MYSCORE_DIGIT_SPACING 	30.0f



/***************************/
/*    VARIABLES            */
/***************************/

static Str32	gHighScoresFileName = ":Bugdom2:HighScores";

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
		OGL_DrawScene(gGameViewInfoPtr, DrawHighScoresCallback);

				/*****************************/
				/* SEE IF USER ENTERING NAME */
				/*****************************/

		if (!gDrawScoreVerbage)
		{
			IMPLEMENT_ME();
#if 0
			EventRecord 	theEvent;

			GetNextEvent(keyDownMask|autoKeyMask, &theEvent);							// poll event queue
			if ((theEvent.what == keyDown) || (theEvent.what == autoKeyMask))			// see if key pressed
			{
				char	theChar = theEvent.message & charCodeMask;						// extract key
				int		i;

				switch(theChar)
				{
					case	CHAR_RETURN:
					case	CHAR_ENTER:
							gExitHighScores = true;
							break;

					case	CHAR_LEFT:
							if (gCursorIndex > 0)
								gCursorIndex--;
							break;

					case	CHAR_RIGHT:
							if (gCursorIndex < (MAX_NAME_LENGTH-1))
								gCursorIndex++;
							else
								gCursorIndex = MAX_NAME_LENGTH-1;
							break;

					case	CHAR_DELETE:
							if (gCursorIndex > 0)
							{
								gCursorIndex--;
								for (i = gCursorIndex+1; i < MAX_NAME_LENGTH; i++)
									gHighScores[gNewScoreSlot].name[i] = gHighScores[gNewScoreSlot].name[i+1];
								gHighScores[gNewScoreSlot].name[MAX_NAME_LENGTH] = ' ';
							}
							break;

					default:
							if (gCursorIndex < MAX_NAME_LENGTH)								// dont add anything more if maxxed out now
							{
								if ((theChar >= 'a') && (theChar <= 'z'))					// see if convert lower case to upper case a..z
									theChar = 'A' + (theChar-'a');
								gHighScores[gNewScoreSlot].name[gCursorIndex+1] = theChar;
								gCursorIndex++;
							}
				}

			}
#endif
		}
	}


		/* CLEANUP */

	if (gNewScoreSlot != -1)						// if a new score was added then update the high scores file
		SaveHighScores();

	FreeScoreScreen();

	GammaFadeOut();

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

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

	InitSparkles();


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:particle.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_PARTICLES, gGameViewInfoPtr);
	BlendAllSpritesInGroup(SPRITE_GROUP_PARTICLES);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:Dialog.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_DIALOG, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:HighScores.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_HIGHSCORES, gGameViewInfoPtr);


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:highscores.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_HIGHSCORES, gGameViewInfoPtr);


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
	gNewObjectDefinition.scale 		= gGameViewInfoPtr->yon * .99f / 100.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawCyclorama;



}




/********************** FREE SCORE SCREEN **********************/

static void FreeScoreScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUND_BANK_BONUS);
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}



/***************** DRAW HIGHSCORES CALLBACK *******************/

static void DrawHighScoresCallback(OGLSetupOutputType *info)
{
	DrawObjects(info);
	DrawSparkles(info);											// draw light sparkles


			/* DRAW SPRITES */

	OGL_PushState();

	SetHighScoresSpriteState();

	if (gDrawScoreVerbage)
		DrawScoreVerbage(info);
	else
		DrawHighScoresAndCursor(info);


	OGL_PopState();
	gGlobalMaterialFlags = 0;
	gGlobalTransparency = 1.0;
}


/********************* SET HIGHSCORES SPRITE STATE ***************/

static void SetHighScoresSpriteState(void)
{
	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);								// no z-buffer

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_CLAMP_U;	// clamp all textures


			/* INIT MATRICES */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/********************* DRAW SCORE VERBAGE ****************************/

static void DrawScoreVerbage(OGLSetupOutputType *info)
{
Str32	s;
int		texNum,n,i;
float	x;

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
	DrawInfobarSprite2(320-150, 110, 300, SPRITE_GROUP_HIGHSCORES, HIGHSCORES_SObjType_ScoreText, info);


			/**************/
			/* DRAW SCORE */
			/**************/

	NumToString(gScore, s);
	n = s[0];										// get str len

	x = 320.0f - ((float)n / 2.0f) * MYSCORE_DIGIT_SPACING - (MYSCORE_DIGIT_SPACING/2);	// calc starting x


	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 0;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	for (i = 1; i <= n; i++)
	{
		texNum = CharToSprite(s[i]);				// get texture #

		DrawInfobarSprite2(x, 240, MYSCORE_DIGIT_SPACING * 1.9f, SPRITE_GROUP_DIALOG, texNum, info);
		x += MYSCORE_DIGIT_SPACING;
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	gGlobalTransparency = 1.0f;
	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 1;
}


/****************** DRAW HIGH SCORES AND CURSOR ***********************/

static void DrawHighScoresAndCursor(OGLSetupOutputType *info)
{
float	y,cursorY,cursorX;
int		i,j,n;
Str32	s;

	gFinalScoreAlpha += gFramesPerSecondFrac;						// fade in
	if (gFinalScoreAlpha > .99f)
		gFinalScoreAlpha = .99f;


 	gCursorFlux += gFramesPerSecondFrac * 10.0f;


			/****************************/
			/* DRAW ENTER NAME VERBAGE */
			/****************************/

			/* DRAW TEXT */

	gGlobalTransparency = gFinalScoreAlpha;
	DrawInfobarSprite2(320-250, 10, 500, SPRITE_GROUP_HIGHSCORES, HIGHSCORES_SObjType_EnterNameText, info);


	gGlobalTransparency = gFinalScoreAlpha;

	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 0;

			/*****************/
			/* DRAW THE TEXT */
			/*****************/

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// make glow

	y = 120;
	for (i = 0; i < NUM_SCORES; i++)
	{
		if (i == gNewScoreSlot)								// see if cursor will go on this line
		{
			cursorY = y;
			cursorX = 150.0f;

			n = gHighScores[i].name[0];						// get str len
			for (j = 1; j <= gCursorIndex; j++)
				cursorX += GetCharSpacing(gHighScores[i].name[j], SCORE_TEXT_SPACING);	// calc cursor position

		}

				/* DRAW NAME */

		DrawScoreText(gHighScores[i].name, 150,y,info);

				/* DRAW SCORE */

		NumToString(gHighScores[i].score, s);	// convert score to a text string
		if (s[0] < SCORE_DIGITS)				// pad 0's
		{
			n = SCORE_DIGITS-s[0];
			BlockMove(&s[1],&s[1+n], 20);		// shift existing data over

			for (j = 0; j < n; j++)				// pad with 0's
				s[1+j] = '0';

			s[0] = SCORE_DIGITS;
		}
		DrawScoreText(s, 350, y, info);

		y += SCORE_TEXT_SPACING * 1.3f;
	}

		/*******************/
		/* DRAW THE CURSOR */
		/*******************/

	if (gCursorIndex < MAX_NAME_LENGTH)						// dont draw if off the right side
	{
		gGlobalTransparency = (.3f + ((sin(gCursorFlux) + 1.0f) * .5f) * .699f) * gFinalScoreAlpha;
		DrawInfobarSprite2(cursorX, cursorY, SCORE_TEXT_SPACING * 1.5f, SPRITE_GROUP_DIALOG, DIALOG_SObjType_Cursor, info);
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
	if (iErr)
		DoFatalAlert("LoadHighScores: Error opening High Scores file!");
	else
	{
		count = sizeof(HighScoreType) * NUM_SCORES;
		iErr = FSRead(refNum, &count,  &gHighScores[0]);								// read data from file
		if (iErr)
		{
			FSClose(refNum);
			FSpDelete(&file);												// file is corrupt, so delete
			return;
		}
		FSClose(refNum);
	}
}


/************************ SAVE HIGH SCORES ******************************/

static void SaveHighScores(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

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
	FSWrite(refNum, &count, &gHighScores[0]);
	FSClose(refNum);

}


/**************** CLEAR HIGH SCORES **********************/

void ClearHighScores(void)
{
short				i,j;
char				blank[MAX_NAME_LENGTH] = "               ";


			/* INIT SCORES */

	for (i=0; i < NUM_SCORES; i++)
	{
		gHighScores[i].name[0] = MAX_NAME_LENGTH;
		for (j=0; j < MAX_NAME_LENGTH; j++)
			gHighScores[i].name[j+1] = blank[j];
		gHighScores[i].score = 0;
	}

	SaveHighScores();
}


/*************************** ADD NEW SCORE ****************************/
//
// Returns high score slot that score was inserted to or -1 if none
//

static short AddNewScore(u_long newScore)
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
	gHighScores[slot].name[0] = MAX_NAME_LENGTH;				// clear name
	for (i = 1; i <= MAX_NAME_LENGTH; i++)
		gHighScores[slot].name[i] = ' ';						// clear to all spaces
	return(slot);
}


#pragma mark -

/****************** IS THIS SCORE IN LIST *********************/
//
// Returns True if this score value is anywhere in the high scores already
//

static Boolean IsThisScoreInList(u_long score)
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





