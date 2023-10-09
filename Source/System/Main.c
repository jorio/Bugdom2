/****************************/
/*    BUGDOM 2 - MAIN 		*/
/* By Brian Greenstone      */
/* (c)2002 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void InitArea(void);
static void InitArea_Exploration(void);
static void CleanupLevel(void);
static void PlayArea(void);
static void PlayArea_Terrain(void);
static void PlayGame(void);
static void UpdateParkFog(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	NORMAL_GRAVITY	4500.0f


static const int kLevelSongs[NUM_LEVELS] =
{
	EFFECT_SONG_GARDEN,				// front yard
	EFFECT_SONG_POOL,				// back yard
	EFFECT_SONG_FIDO,				// fido
	EFFECT_SONG_PLUMBING,			// sewer
	EFFECT_SONG_PLAYROOM,			// playroom
	EFFECT_SONG_CLOSET,				// closet
	EFFECT_SONG_PLUMBING,			// gutter
	EFFECT_SONG_GARBAGE,			// garbage
	EFFECT_SONG_BALSA,				// balsa
	EFFECT_SONG_PARK,				// park
};

static const int kLevelSoundBanks[NUM_LEVELS] =
{
	SOUND_BANK_GARDEN,
	SOUND_BANK_GARDEN,
	SOUND_BANK_FIDO,
	SOUND_BANK_PLUMBING,
	SOUND_BANK_PLAYROOM,
	SOUND_BANK_CLOSET,
	SOUND_BANK_PLUMBING,
	SOUND_BANK_GARBAGE,
	SOUND_BANK_BALSA,
	SOUND_BANK_PARK,
};


/****************************/
/*    VARIABLES             */
/****************************/

short	gPrefsFolderVRefNum;
long	gPrefsFolderDirID;

Boolean				gG4 = true;
Boolean				gSlowCPU = false;

float				gGravity = NORMAL_GRAVITY;

Byte				gDebugMode = 0;				// 0 == none, 1 = fps, 2 = all

uint32_t				gAutoFadeStatusBits;

OGLSetupOutputType		gGameView = {0};

PrefsType			gGamePrefs;

OGLVector3D			gWorldSunDirection = { .5, -.35, .8};		// also serves as lense flare vector
OGLColorRGBA		gFillColor1 = { .6, .6, .6, 1};

uint32_t				gGameFrameNum = 0;
float				gGameLevelTimer = 0;

Boolean				gInGameNow = false;
Boolean				gPlayingFromSavedGame = false;
Boolean				gGameOver = false;
Boolean				gLevelCompleted = false;
float				gLevelCompletedCoolDownTimer = 0;

int					gLevelNum;

OGLPoint2D			gBestCheckpointCoord;
float				gBestCheckpointAim;

int					gScratch = 0;
float				gScratchF = 0;

uint32_t				gScore,gLoadedScore;


//======================================================================================
//======================================================================================
//======================================================================================


/****************** TOOLBOX INIT  *****************/

OSErr CheckPrefsFolder(bool createIt)
{
OSErr		iErr;
long		createdDirID;

	iErr = FindFolder(kOnSystemDisk,kPreferencesFolderType,kDontCreateFolder,			// locate the folder
					&gPrefsFolderVRefNum,&gPrefsFolderDirID);

	if (iErr != noErr)
	{
		DoAlert("Warning: Cannot locate the Preferences folder.");
		return iErr;
	}

	if (createIt)
	{
		iErr = DirCreate(gPrefsFolderVRefNum, gPrefsFolderDirID, PROJECT_NAME, &createdDirID);		// make folder in there
	}

	return iErr;
}


/************************* INIT DEFAULT PREFS **************************/

void InitDefaultPrefs(void)
{
	SDL_zero(gGamePrefs);

	gGamePrefs.language				= GetBestLanguageIDFromSystemLocale();
	gGamePrefs.version				= CURRENT_PREFS_VERS;
	gGamePrefs.kiddieMode			= false;
	gGamePrefs.anaglyph				= ANAGLYPH_OFF;
	gGamePrefs.monitorNum			= 0;
	gGamePrefs.antialiasingLevel	= 0;
	gGamePrefs.vsync				= true;
	gGamePrefs.fullscreen			= true;
	gGamePrefs.uiCentering			= false;
	gGamePrefs.music				= true;
	gGamePrefs.buddyBugBuzz			= true;
	gGamePrefs.gamepadRumbleLevel	= MAX_GAMEPAD_RUMBLE_LEVEL;
	gGamePrefs.mouseControlsSkip	= true;
	gGamePrefs.mouseSensitivityLevel= DEFAULT_MOUSE_SENSITIVITY_LEVEL;

	_Static_assert(sizeof(gGamePrefs.bindings) == sizeof(kDefaultInputBindings), "input binding size mismatch: prefs vs defaults");
	SDL_memcpy(&gGamePrefs.bindings, &kDefaultInputBindings, sizeof(kDefaultInputBindings));
}


#pragma mark -

/******************** PLAY GAME ************************/

static void PlayGame(void)
{
			/***********************/
			/* GAME INITIALIZATION */
			/***********************/

	InitPlayerInfo_Game();					// init player info for entire game


			/*********************************/
			/* PLAY THRU LEVELS SEQUENTIALLY */
			/*********************************/


	for (; gLevelNum < NUM_LEVELS; gLevelNum++)		// assume gLevelNum was initially set from menu
	{
		LoadSoundBank(kLevelSoundBanks[gLevelNum]);
		PlaySong(kLevelSongs[gLevelNum], true);

				/* DO LEVEL INTRO */

		DoLevelIntro();

		MyFlushEvents();


	        /* LOAD ALL OF THE ART & STUFF */

		InitArea();


			/***********/
	        /* PLAY IT */
	        /***********/

		gInGameNow = true;
		PlayArea();

			/* CLEANUP LEVEL */

		gInGameNow = false;
		MyFlushEvents();
		CleanupLevel();
		DisposeSoundBank(kLevelSoundBanks[gLevelNum]);

			/***************/
			/* SEE IF LOST */
			/***************/

		if (gGameOver)									// bail out if game has ended
		{
			DoLoseScreen();
			break;
		}


		/* DO END-LEVEL BONUS SCREEN */

		if (gLevelNum == LEVEL_NUM_PARK)				// if just won game then do win screen first!
			DoWinScreen();
		DoBonusScreen();

	}


			/* DO HIGH SCORES */

	NewScore();

	gPlayingFromSavedGame = false;
}

/************************* PLAY AREA *******************************/

static void PlayArea(void)
{
			/* PREP STUFF */

	MakeFadeEvent(true, 1);
	GrabMouse(true);
	InvalidateAllInputs();
	ResetFramesPerSecond();

	/***********************************************/
	/* PLAY BASED ON THE TYPE OF LEVEL WE'RE DOING */
	/***********************************************/

	switch(gLevelNum)
	{
			/* PLAY TUNNEL LEVELS */

		case	LEVEL_NUM_PLUMBING:
		case	LEVEL_NUM_GUTTER:
				PlayArea_Tunnel();
				if (!gGameOver)
					OGL_FadeOutScene(DrawTunnel, KeepTerrainAlive);
				break;


			/* PLAY EXPLORATION LEVELS */

		default:
				PlayArea_Terrain();
				if (!gGameOver)
					OGL_FadeOutScene(DrawObjects, KeepTerrainAlive);
	}
}



/**************** PLAY AREA: TERRAIN  ************************/
//
// The main loop for a terrain exploration level
//

static void PlayArea_Terrain(void)
{



		/******************/
		/* MAIN GAME LOOP */
		/******************/

	while(true)
	{
					/* GET CONTROL INFORMATION FOR THIS FRAME */

		UpdateInput();									// read local keys

				/* MOVE OBJECTS */

		MoveEverything();


			/* UPDATE THE TERRAIN */

		DoPlayerTerrainUpdate(gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z);


			/* DRAW IT ALL */


		OGL_DrawScene(DrawObjects);



			/**************/
			/* MISC STUFF */
			/**************/

			/* SEE IF PAUSED */

		if (IsNeedDown(kNeed_UIPause))
			DoPaused();

#if 0
		if (IsKeyDown(SDL_SCANCODE_F15))								// do screen-saver-safe paused mode
		{
			glFinish();

			do
			{
				EventRecord	e;
				WaitNextEvent(everyEvent,&e, 0, 0);
				UpdateInput();
			}while(!IsKeyDown(SDL_SCANCODE_F15));

			CalcFramesPerSecond();
		}
#endif


		CalcFramesPerSecond();

		gGameFrameNum++;
		gGameLevelTimer += gFramesPerSecondFrac;


				/* CHEATS */

#if !_DEBUG
		if (IsKeyActive(SDL_SCANCODE_LGUI) || IsKeyActive(SDL_SCANCODE_RGUI))
#endif
		{
			if (IsKeyDown(SDL_SCANCODE_F10))            // win level cheat
				break;
		}

		if (IsCheatKeyComboDown())						// get health & stuff
		{
			gPlayerInfo.health = 1.0;
			gPlayerInfo.glidePower = 1.0;
			if (gPlayerInfo.lives < 3)
				gPlayerInfo.lives = 3;
			gPlayerInfo.hasMap = true;
		}

		if (IsKeyActive(SDL_SCANCODE_F) && IsKeyActive(SDL_SCANCODE_N) && IsKeyActive(SDL_SCANCODE_C))
		{
			gNumFences = -gNumFences;					// toggle fences
			InvalidateAllInputs();
		}

				/* SEE IF LEVEL IS COMPLETED */

		if (gGameOver)													// if we need immediate abort, then bail now
			break;

		if (gLevelCompleted)
		{
			gLevelCompletedCoolDownTimer -= gFramesPerSecondFrac;		// game is done, but wait for cool-down timer before bailing
			if (gLevelCompletedCoolDownTimer <= 0.0f)
				break;
		}

				/* SEE IF RESET PLAYER NOW */

		if (gPlayerIsDead)
		{
			float	oldTimer = gDeathTimer;
			gDeathTimer -= gFramesPerSecondFrac;
			if (gDeathTimer <= 0.0f)
			{
				if (oldTimer > 0.0f)						// if just now crossed zero then start fade
					MakeFadeEvent(false, 1);
				else
				if (gGammaFadeFrac <= 0.0f)				// once fully faded out reset player @ checkpoint
					ResetPlayerAtBestCheckpoint();
			}
		}

		gDisableHiccupTimer = false;									// reenable this after the 1st frame
	}
}


/******************** MOVE EVERYTHING ************************/

void MoveEverything(void)
{
	MoveObjects();
	MoveSplineObjects();
	UpdateCamera();								// update camera

	/* LEVEL SPECIFIC UPDATES */

	switch(gLevelNum)
	{
		case	LEVEL_NUM_GNOMEGARDEN:
				UpdateSprinklerHeads();
				break;

		case	LEVEL_NUM_PLAYROOM:
				UpdateSlotCarRacing();
				break;

		case	LEVEL_NUM_GARBAGE:
				RaiseWater();
				break;

		case	LEVEL_NUM_PARK:
				UpdateParkFog();
				break;

	}

}


/******************* UPDATE PARK FOG ****************************/
//
// Varies the density of the fog based on terrain Y
//

static void UpdateParkFog(void)
{
float	y = GetTerrainY(gPlayerInfo.coord.x, gPlayerInfo.coord.z);
float	start,end;
float	decay = (1.0f / 2600.0f);			// decay based on upper margin

	y -= 600.0f;							// set lower margin of n
	if (y < 0.0f)
		y = 0.0f;


	start = .0f + (y * decay);
	start *= gGameView.yon;

	end = .8f + (y * decay);
	end *= gGameView.yon;

	if (start > end)
		start = end;

	glFogf(GL_FOG_START, start);
	glFogf(GL_FOG_END, end);

}


#pragma mark -

/***************** INIT AREA ************************/

static void InitArea(void)
{

		/*********************/
		/* INIT COMMON STUFF */
		/*********************/

	gGameFrameNum 		= 0;
	gGameLevelTimer 	= 0;
	gGameOver 			= false;
	gLevelCompleted 	= false;


	gGravity = NORMAL_GRAVITY;					// assume normal gravity


	gPlayerInfo.objNode = nil;

	/***********************************************/
	/* INIT BASED ON THE TYPE OF LEVEL WE'RE DOING */
	/***********************************************/

	switch(gLevelNum)
	{
			/* INIT TUNNEL LEVELS */

		case	LEVEL_NUM_PLUMBING:
		case	LEVEL_NUM_GUTTER:
				InitArea_Tunnels();
				break;

			/* INIT EXPLORATION LEVELS */

		default:
				InitArea_Exploration();
	}
}




/******************** INIT AREA:  EXPLORATION ***************************/
//
// This does the level initialization for the levels which are the regular "exploration" type.
//

static void InitArea_Exploration(void)
{
OGLSetupInputType	viewDef;


			/*************/
			/* MAKE VIEW */
			/*************/

	SetTerrainScale(DEFAULT_TERRAIN_SCALE);								// set scale to some default for now



			/* SETUP VIEW DEF */

	OGL_NewViewDef(&viewDef);

	viewDef.camera.hither 			= 10;
	viewDef.camera.fov 				= GAME_FOV;


			/* SET ANAGLYPH INFO */

	if (gGamePrefs.anaglyph)
	{
		gAnaglyphScaleFactor 	= 1.0f;
		gAnaglyphFocallength	= 130.0f * gAnaglyphScaleFactor;	// set camera info
		gAnaglyphEyeSeparation 	= 20.0f * gAnaglyphScaleFactor;
	}



	switch(gLevelNum)
	{
		case	LEVEL_NUM_SIDEWALK:
#if APPSTORE
				gSuperTileActiveRange = MAX_SUPERTILE_ACTIVE_RANGE;
#else
				if (gG4)												// better range if we can afford it
					gSuperTileActiveRange = 5;
#endif
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .9;
				viewDef.view.clearColor.g 		= .9;
				viewDef.view.clearColor.b		= .9;
				viewDef.view.clearBackBuffer	= true;
				viewDef.camera.yon 				= (gSuperTileActiveRange * SUPERTILE_SIZE * gTerrainPolygonSize) * .95f;
				viewDef.styles.fogStart			= viewDef.camera.yon * .5f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.2f;
				gDrawLensFlare = true;
				break;

		case	LEVEL_NUM_FIDO:
				viewDef.camera.yon 				*= .8f;

				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .12;
				viewDef.view.clearColor.g 		= .08;
				viewDef.view.clearColor.b		= .08;
				viewDef.view.clearBackBuffer	= true;
				viewDef.camera.yon 				= (gSuperTileActiveRange * SUPERTILE_SIZE * gTerrainPolygonSize) * .7f;
				viewDef.styles.fogStart			= viewDef.camera.yon * .1f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * .95f;
				gDrawLensFlare = false;
				break;

		case	LEVEL_NUM_PLAYROOM:
#if APPSTORE
				gSuperTileActiveRange = MAX_SUPERTILE_ACTIVE_RANGE;
#else
				if (gG4)												// better range if we can afford it
					gSuperTileActiveRange = 5;
#endif
				viewDef.styles.useFog			= false;
				viewDef.view.clearBackBuffer	= true;
				viewDef.camera.yon 				= (gSuperTileActiveRange * SUPERTILE_SIZE * gTerrainPolygonSize) * .7f;
				gDrawLensFlare = false;
				break;


		case	LEVEL_NUM_CLOSET:
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .0;
				viewDef.view.clearColor.g 		= .0;
				viewDef.view.clearColor.b		= .1;
				viewDef.view.clearBackBuffer	= true;
				viewDef.camera.yon 				= (gSuperTileActiveRange * SUPERTILE_SIZE * gTerrainPolygonSize) * .7f;
				viewDef.styles.fogStart			= viewDef.camera.yon * .2f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * .95f;
				gDrawLensFlare = false;
				break;

		case	LEVEL_NUM_GARBAGE:
#if APPSTORE
				gSuperTileActiveRange = MAX_SUPERTILE_ACTIVE_RANGE;
#else
				if (gG4)												// better range if we can afford it
					gSuperTileActiveRange = 5;
#endif
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .1;
				viewDef.view.clearColor.g 		= .2;
				viewDef.view.clearColor.b		= .1;
				viewDef.view.clearBackBuffer	= true;
				viewDef.camera.yon 				= (gSuperTileActiveRange * SUPERTILE_SIZE * gTerrainPolygonSize) * .7f;
				viewDef.styles.fogStart			= viewDef.camera.yon * .7f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.1f;
				gDrawLensFlare = true;
				break;



		case	LEVEL_NUM_BALSA:
				SetTerrainScale(DEFAULT_TERRAIN_SCALE*1.6);								// scale the terrain up for this level
				gSuperTileActiveRange = 3;

				viewDef.camera.fov 				= .9;
				viewDef.styles.useFog			= false;
				viewDef.view.clearBackBuffer	= true;
				viewDef.view.clearColor.r 		= 0;
				viewDef.view.clearColor.g 		= 0;
				viewDef.view.clearColor.b		= 0;
				viewDef.camera.hither			= 200;
				viewDef.camera.yon 				= 9000;
				gDrawLensFlare = false;

				if (gGamePrefs.anaglyph)
				{
					gAnaglyphScaleFactor 	= 1.0f;
					gAnaglyphFocallength	= 1300.0f * gAnaglyphScaleFactor;	// set camera info
					gAnaglyphEyeSeparation 	= 55.0f * gAnaglyphScaleFactor;
				}

				break;

		case	LEVEL_NUM_PARK:
#if APPSTORE
				gSuperTileActiveRange = MAX_SUPERTILE_ACTIVE_RANGE;
#else
				if (gG4)												// better range if we can afford it
					gSuperTileActiveRange = 5;
#endif
				viewDef.styles.useFog			= true;
				viewDef.view.clearBackBuffer	= true;
				viewDef.view.clearColor.r 		= 1;
				viewDef.view.clearColor.g 		= 1;
				viewDef.view.clearColor.b		= 1;
				gDrawLensFlare = true;
				break;

		default:
#if APPSTORE
				gSuperTileActiveRange = MAX_SUPERTILE_ACTIVE_RANGE;
#else
				if (gSlowCPU)
					gSuperTileActiveRange = 3;
#endif

				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= 0;
				viewDef.view.clearColor.g 		= .4;
				viewDef.view.clearColor.b		= 0;
				viewDef.view.clearBackBuffer	= true;
				viewDef.camera.yon 				= (gSuperTileActiveRange * SUPERTILE_SIZE * gTerrainPolygonSize) * .95f;
				viewDef.styles.fogStart			= viewDef.camera.yon * .7f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.0f;
				gDrawLensFlare = true;
	}




		/**************/
		/* SET LIGHTS */
		/**************/

	switch(gLevelNum)
	{
		case	LEVEL_NUM_FIDO:
				viewDef.lights.numFillLights 		= 2;

				viewDef.lights.ambientColor.r 		= .2;
				viewDef.lights.ambientColor.g 		= .1;
				viewDef.lights.ambientColor.b 		= .1;

				viewDef.lights.fillDirection[0].x 	= -.5;
				viewDef.lights.fillDirection[0].y 	= -.2;
				viewDef.lights.fillDirection[0].z 	= .5;
				viewDef.lights.fillColor[0].r 		= 1.0;
				viewDef.lights.fillColor[0].g 		= 1.0;
				viewDef.lights.fillColor[0].b 		= .8;

				viewDef.lights.fillDirection[1].x 	= .4;
				viewDef.lights.fillDirection[1].y 	= -.1;
				viewDef.lights.fillDirection[1].z 	= -.5;
				viewDef.lights.fillColor[1].r 		= .5;
				viewDef.lights.fillColor[1].g 		= .5;
				viewDef.lights.fillColor[1].b 		= .4;
				break;


		case	LEVEL_NUM_SIDEWALK:
				gWorldSunDirection.x = .8;
				gWorldSunDirection.y = -.5;
				gWorldSunDirection.z = -.8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .4;
				viewDef.lights.ambientColor.g 		= .4;
				viewDef.lights.ambientColor.b 		= .3;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
				break;

		case	LEVEL_NUM_PLAYROOM:
				gWorldSunDirection.x = .5;
				gWorldSunDirection.y = -.8;
				gWorldSunDirection.z = -.3;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 2;
				viewDef.lights.ambientColor.r 		= .35;
				viewDef.lights.ambientColor.g 		= .35;
				viewDef.lights.ambientColor.b 		= .30;

				viewDef.lights.fillDirection[0].x 	= 1;
				viewDef.lights.fillDirection[0].y 	= -.4;
				viewDef.lights.fillDirection[0].z 	= 1;
				viewDef.lights.fillColor[0].r 		= 1.0;
				viewDef.lights.fillColor[0].g 		= .7;
				viewDef.lights.fillColor[0].b 		= .7;

				viewDef.lights.fillDirection[1].x 	= -1;
				viewDef.lights.fillDirection[1].y 	= -.2;
				viewDef.lights.fillDirection[1].z 	= -.5;
				viewDef.lights.fillColor[1].r 		= .7;
				viewDef.lights.fillColor[1].g 		= .7;
				viewDef.lights.fillColor[1].b 		= 1.0;
				break;

		case	LEVEL_NUM_CLOSET:
				gWorldSunDirection.x = .3;
				gWorldSunDirection.y = -.6;
				gWorldSunDirection.z = -.5;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .2;
				viewDef.lights.ambientColor.g 		= .2;
				viewDef.lights.ambientColor.b 		= .3;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0].r 		= .25;
				viewDef.lights.fillColor[0].g 		= .25;
				viewDef.lights.fillColor[0].b 		= .4;
				break;

		case	LEVEL_NUM_GARBAGE:
				gWorldSunDirection.x = -.5;
				gWorldSunDirection.y = -.5;
				gWorldSunDirection.z = -.8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .3;
				viewDef.lights.ambientColor.g 		= .3;
				viewDef.lights.ambientColor.b 		= .3;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0].r 		= .8;
				viewDef.lights.fillColor[0].g 		= .9;
				viewDef.lights.fillColor[0].b 		= .8;
				break;


		case	LEVEL_NUM_BALSA:
				gWorldSunDirection.x = .8;
				gWorldSunDirection.y = -.6;
				gWorldSunDirection.z = -.9;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .3;
				viewDef.lights.ambientColor.g 		= .3;
				viewDef.lights.ambientColor.b 		= .3;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0].r 		= 1.0;
				viewDef.lights.fillColor[0].g 		= 1.0;
				viewDef.lights.fillColor[0].b 		= .9;
				break;


		case	LEVEL_NUM_PARK:
				gWorldSunDirection.x = .8;
				gWorldSunDirection.y = -.35;
				gWorldSunDirection.z = -.2;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .4;
				viewDef.lights.ambientColor.g 		= .4;
				viewDef.lights.ambientColor.b 		= .3;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0].r 		= .9;
				viewDef.lights.fillColor[0].g 		= .9;
				viewDef.lights.fillColor[0].b 		= .8;
				break;



		default:
				gWorldSunDirection.x = .4;
				gWorldSunDirection.y = -.5;
				gWorldSunDirection.z = -.8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .5;
				viewDef.lights.ambientColor.g 		= .5;
				viewDef.lights.ambientColor.b 		= .4;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
	}



	OGL_SetupWindow(&viewDef, &gGameView);


			/**********************/
			/* SET AUTO-FADE INFO */
			/**********************/

	switch(gLevelNum)
	{
		case	LEVEL_NUM_FIDO:
		case	LEVEL_NUM_BALSA:
		case	LEVEL_NUM_CLOSET:
				gAutoFadeStartDist	= 0;				// no auto-fading here
				break;

		default:
				gAutoFadeStartDist	= gGameView.yon * .80;
				gAutoFadeEndDist	= gGameView.yon * .95f;
	}


	if (!gG4)												// bring it in a little for slow machines
	{
		gAutoFadeStartDist *= .85f;
		gAutoFadeEndDist *= .85f;
	}

	gAutoFadeRange_Frac	= 1.0f / (gAutoFadeEndDist - gAutoFadeStartDist);

	if (gAutoFadeStartDist != 0.0f)
		gAutoFadeStatusBits = STATUS_BIT_AUTOFADE;
	else
		gAutoFadeStatusBits = 0;



			/**********************/
			/* LOAD ART & TERRAIN */
			/**********************/
			//
			// NOTE: only call this *after* draw context is created!
			//

	LoadLevelArt_Explore();
	InitInfobar();

			/* INIT OTHER MANAGERS */

	InitEnemyManager();
	InitEffects();
	InitItemsManager();
	InitDialogManager();


			/****************/
			/* INIT SPECIAL */
			/****************/

	gGravity = NORMAL_GRAVITY;					// assume normal gravity

	switch(gLevelNum)
	{
		case	LEVEL_NUM_SIDEWALK:
				InitSnakeStuff();
				CountSquishBerries();
				break;

		case	LEVEL_NUM_FIDO:
				CountTicks();
				CountFleas();
				DoDialogMessage(DIALOG_MESSAGE_REMEMBERDOG, 1, 11.0, nil);
				break;

		case	LEVEL_NUM_PLAYROOM:
				InitSlotCarRacing();
				break;

		case	LEVEL_NUM_GARBAGE:
				gNumDrowningMiceRescued = 0;				// no drowning mice rescued yet
				break;

		case	LEVEL_NUM_BALSA:
				DoDialogMessage(DIALOG_MESSAGE_BOMBHILLS, 0, 10.0, nil);
				DoDialogMessage(DIALOG_MESSAGE_BOMBHILLS2, 1, 6.0, nil);
				break;

		case	LEVEL_NUM_PARK:
				gNumFoodOnBasket = 0;
				gNumCaughtFish = 0;
				gKindlingCount = 0;
				gBurnKindling = false;
				gIgnoreBottleKeySnail = false;
				break;

	}

		/* INIT THE PLAYER & RELATED STUFF */

	PrimeTerrainWater();							// NOTE:  must do this before items get added since some items may be on the water
	InitPlayerAtStartOfLevel_Terrain();				// NOTE:  this will also cause the initial items in the start area to be created

	PrimeSplines();
	PrimeFences();

			/* INIT CAMERAS */

	InitCamera_Terrain();


 }


/***************** START LEVEL COMPLETION ****************/

void StartLevelCompletion(float coolDownTimer)
{
	if (!gLevelCompleted)
	{
		gLevelCompleted = true;
		gLevelCompletedCoolDownTimer = coolDownTimer;
	}
}

/**************** CLEANUP LEVEL **********************/

static void CleanupLevel(void)
{
	GrabMouse(false);

	StopAllEffectChannels();
	DisposeTunnelData();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeWater();
	DisposeTerrain();
	DisposeInfobar();
	DisposeEffects();
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);
	DisposeSpriteGroup(SPRITE_GROUP_INFOBAR);

	DisposeAllBG3DContainers();

	gPlayerInfo.objNode = NULL;
}


#pragma mark -

void PreloadGlobalSprites(void)
{
	static int spritesPreloadedForAnaglyphMode = -1;

	if (gGamePrefs.anaglyph == spritesPreloadedForAnaglyphMode)
		return;

	spritesPreloadedForAnaglyphMode = gGamePrefs.anaglyph;

	DisposeSpriteGroup(SPRITE_GROUP_PARTICLES);
	DisposeSpriteGroup(SPRITE_GROUP_SPHEREMAPS);
	DisposeSpriteGroup(SPRITE_GROUP_GLOBAL);
	DisposeSpriteAtlas(ATLAS_GROUP_FONT1);

	LoadSpriteGroupFromSeries(SPRITE_GROUP_PARTICLES, PARTICLE_SObjType_COUNT, "Particle");
	LoadSpriteGroupFromSeries(SPRITE_GROUP_SPHEREMAPS, SPHEREMAP_SObjType_COUNT, "SphereMap");
	LoadSpriteGroupFromSeries(SPRITE_GROUP_GLOBAL, GLOBAL_SObjType_COUNT, "Global");
	LoadSpriteAtlas(ATLAS_GROUP_FONT1, ":Sprites:Font:font", kAtlasLoadFont);

	BlendAllSpritesInGroup(SPRITE_GROUP_PARTICLES);
//	BlendASprite(SPRITE_GROUP_PARTICLES, PARTICLE_SObjType_Splash);
}

#pragma mark -

/************************************************************/
/******************** PROGRAM MAIN ENTRY  *******************/
/************************************************************/


void GameMain(void)
{
unsigned long	someLong;


				/**************/
				/* BOOT STUFF */
				/**************/

	LoadLocalizedStrings(gGamePrefs.language);
	InitInput();

	OGL_Boot();

	// Set fullscreen mode from prefs (*after* we have a GL context for Wayland)
	SetFullscreenMode(true);

	InitSpriteManager();
	InitBG3DManager();
	InitWindowStuff();
	InitTerrainManager();
	InitSkeletonManager();
	InitSoundTools();
	InitObjectManager();

	GetDateTime ((unsigned long *)(&someLong));		// init random seed
	SetMyRandomSeed(someLong);

	// Load some global sprites
	PreloadGlobalSprites();

#if !SKIPFLUFF
		/* SHOW TITLES */

	DoLegalScreen();
	DoTitleScreen();
#endif


		/* MAIN LOOP */

	while(true)
	{
		MyFlushEvents();
		DoMainMenuScreen();

		PlayGame();
	}

}
