/******************************/
/*	LEVEL INTRO: CLOSET.C   */
/* (c)2002 Pangea Software    */
/* By Brian Greenstone        */
/******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gGlobalTransparency;
extern	FSSpec		gDataSpec;
extern	Boolean		gGameOver;
extern	KeyMap gKeyMap,gNewKeys;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,gDisableAnimSounds;
extern	PrefsType	gGamePrefs;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	int				gLevelNum;
extern	OGLPoint3D	gCoord;
extern	OGLVector3D	gDelta;
extern	u_long			gScore,gGlobalMaterialFlags;
extern	Byte	gSprinklerMode;
extern	float	gSprinklerPopUpOffset,gSprinklerTimer;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupLevelIntroScreen(void);
static void FreeLevelIntroScreen(void);
static void DrawLevelIntroCallback(OGLSetupOutputType *info);
static void ProcessLevelIntro(void);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/



/********************** DO LEVELINTRO SCREEN **************************/

void DoLevelIntroScreen_Closet(void)
{
	GammaFadeOut();

			/* SETUP */

	SetupLevelIntroScreen();
	MakeFadeEvent(true, 1);

	ProcessLevelIntro();


			/* CLEANUP */

	GammaFadeOut();
	FreeLevelIntroScreen();
}



/********************* SETUP LEVELINTRO SCREEN **********************/

static void SetupLevelIntroScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
const static OGLVector3D	fillDirection1 = { -.8, -.6, -1.0 };
ObjNode	*newObj;

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.3;
	viewDef.camera.hither 		= 2;
	viewDef.camera.yon 			= 12000;

	viewDef.styles.useFog			= true;
	viewDef.styles.fogStart			= viewDef.camera.yon * 0.1f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * 1.1f;

	viewDef.view.clearBackBuffer	= true;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.y		= 3;
	viewDef.camera.from.z		= 1000;

	viewDef.camera.to.x 		= 0.0f;
	viewDef.camera.to.y 		= 50.0f;
	viewDef.camera.to.z 		= -700.0f;

	viewDef.lights.ambientColor.r = .15;
	viewDef.lights.ambientColor.g = .15;
	viewDef.lights.ambientColor.b = .15;

	viewDef.lights.numFillLights 	= 1;

	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillColor[0].r 	= .7;
	viewDef.lights.fillColor[0].g 	= .7;
	viewDef.lights.fillColor[0].b 	= .8;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Models:LevelIntro.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELINTRO, gGameViewInfoPtr);


			/*******************/
			/* MAKE BACKGROUND */
			/*******************/

				/* GROUND */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= LEVELINTRO_ObjType_ClosetRoom;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 5;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 40.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);



}


/********************** FREE LEVELINTRO SCREEN **********************/

static void FreeLevelIntroScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUND_BANK_LEVELSPECIFIC);
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}



/**************** PROCESS LEVELINTRO ********************/

static void ProcessLevelIntro(void)
{
float	timer;

	CalcFramesPerSecond();
	UpdateInput();

	timer = 10.0f;

	while(!AreAnyNewKeysPressed())
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();
		OGL_MoveCameraFrom(gGameViewInfoPtr, 0, 0, -160.0f * fps);



				/* DRAW */

		OGL_DrawScene(gGameViewInfoPtr, DrawLevelIntroCallback);

		timer -= fps;
		if (timer < 0.0f)
			break;
	}
}


/***************** DRAW LEVELINTRO CALLBACK *******************/

static void DrawLevelIntroCallback(OGLSetupOutputType *info)
{
	DrawObjects(info);
}








