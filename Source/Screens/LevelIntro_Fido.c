/******************************/
/*	LEVEL INTRO: FIDE.C 	  */
/* (c)2002 Pangea Software    */
/* By Brian Greenstone        */
/******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupLevelIntroScreen(void);
static void FreeLevelIntroScreen(void);
static void DrawLevelIntroCallback(void);
static void ProcessLevelIntro(void);
static void MoveDogBone(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

/*********************/
/*    VARIABLES      */
/*********************/



/********************** DO LEVELINTRO SCREEN **************************/

void DoLevelIntroScreen_Fido(void)
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
const static OGLVector3D	fillDirection1 = { -1.0, -.7, -.7 };
ObjNode	*newObj;

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.0;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 4000;

	viewDef.styles.useFog			= true;
	viewDef.styles.fogStart			= viewDef.camera.yon * .7f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * .9f;

	viewDef.view.clearBackBuffer	= true;
	viewDef.view.clearColor.r 		= 1;
	viewDef.view.clearColor.g 		= 1;
	viewDef.view.clearColor.b		= 1;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.y		= 400;
	viewDef.camera.from.z		= 1100;

	viewDef.camera.to.y 		= 400.0f;

	viewDef.lights.ambientColor.r = .2;
	viewDef.lights.ambientColor.g = .2;
	viewDef.lights.ambientColor.b = .2;

	viewDef.lights.numFillLights 	= 1;

	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillColor[0].r 	= .9;
	viewDef.lights.fillColor[0].g 	= .9;
	viewDef.lights.fillColor[0].b 	= .8;

	OGL_SetupWindow(&viewDef, &gGameView);


				/************/
				/* LOAD ART */
				/************/


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:LevelIntro.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELINTRO);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Level2_Sidewalk.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);

				/* LOAD AUDIO */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Fido.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);


			/*******************/
			/* MAKE BACKGROUND */
			/*******************/

				/* GROUND */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= LEVELINTRO_ObjType_Level2Ground;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= -700;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 5;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 11.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* CYC */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SIDEWALK_ObjType_Cyclorama;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot 		= TERRAIN_SLOT+1;					// draw after terrain for better performance since terrain blocks much of the pixels
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= gGameView->yon * .995f / 100.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawCyclorama;


			/* DOG HOUSE */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SIDEWALK_ObjType_DogHouse;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= -1500;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 5;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* DOG BONE */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= LEVELINTRO_ObjType_DogBone;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 2600;
	gNewObjectDefinition.coord.z 	= -300;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 5;
	gNewObjectDefinition.moveCall 	= MoveDogBone;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .9;
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
	OGL_DisposeWindowSetup(&gGameView);
}



/**************** PROCESS LEVELINTRO ********************/

static void ProcessLevelIntro(void)
{
float	timer;

	CalcFramesPerSecond();
	UpdateInput();

	timer = 9.0f;

	while(!AreAnyNewKeysPressed())
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();

		OGL_MoveCameraFrom(0, 0, -65.0f * fps);


				/* DRAW */

		OGL_DrawScene(DrawLevelIntroCallback);

		timer -= fps;
		if (timer < 0.0f)
			break;
	}
}


/***************** DRAW LEVELINTRO CALLBACK *******************/

static void DrawLevelIntroCallback(void)
{
	DrawObjects();
}

#pragma mark -

/************************** MOVE DOG BONE ****************************/

static void MoveDogBone(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	gDelta.y -= 1400.0f * fps;
	gCoord.y += gDelta.y * fps;

	if (gCoord.y <= 100.0f)
	{
		gCoord.y = 100.0f;
		gDelta.y *= -.5f;

		if (fabs(gDelta.y) > 50.0f)			// do thud sfx
			PlayEffect_Parms(EFFECT_BONEHIT, FULL_CHANNEL_VOLUME/3, FULL_CHANNEL_VOLUME/3,NORMAL_CHANNEL_RATE);
	}

	UpdateObject(theNode);
}








