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
			/* SETUP */

	SetupLevelIntroScreen();
	MakeFadeEvent(true, 1);

	ProcessLevelIntro();


			/* CLEANUP */

	OGL_FadeOutScene(DrawObjects, NULL);
	FreeLevelIntroScreen();
}



/********************* SETUP LEVELINTRO SCREEN **********************/

static void SetupLevelIntroScreen(void)
{
NewObjectDefinitionType def;
FSSpec				spec;
OGLSetupInputType	viewDef;
static const OGLVector3D	fillDirection1 = { -1.0, -.7, -.7 };

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
	// Note: preloaded in main.c


			/*******************/
			/* MAKE BACKGROUND */
			/*******************/

				/* GROUND */

	def = (NewObjectDefinitionType)
	{
		.group		= MODEL_GROUP_LEVELINTRO,
		.type		= LEVELINTRO_ObjType_Level2Ground,
		.coord		= { 0, 0, -700 },
		.flags		= STATUS_BIT_DONTCULL,
		.slot		= 5,
		.scale		= 11.0,
	};
	MakeNewDisplayGroupObject(&def);


			/* CYC */

	def = (NewObjectDefinitionType)
	{
		.group		= MODEL_GROUP_LEVELSPECIFIC,
		.type		= SIDEWALK_ObjType_Cyclorama,
		.coord		= {0,0,0},
		.flags		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG,
		.slot		= TERRAIN_SLOT+1,		// draw after terrain for better performance since terrain blocks much of the pixels
		.drawCall	= DrawCyclorama,
		.scale		= gGameView.yon * .995f / 100.0f,
	};
	MakeNewDisplayGroupObject(&def);


			/* DOG HOUSE */

	def = (NewObjectDefinitionType)
	{
		.group		= MODEL_GROUP_LEVELSPECIFIC,
		.type		= SIDEWALK_ObjType_DogHouse,
		.coord		= {0,0,-1500},
		.flags		= STATUS_BIT_DONTCULL,
		.slot		= 5,
		.scale		= 2.0f,
	};
	MakeNewDisplayGroupObject(&def);


			/* DOG BONE */

	def = (NewObjectDefinitionType)
	{
		.group		= MODEL_GROUP_LEVELINTRO,
		.type		= LEVELINTRO_ObjType_DogBone,
		.coord		= {0,2600,-300},
		.flags		= STATUS_BIT_DONTCULL,
		.slot		= 5,
		.moveCall 	= MoveDogBone,
		.scale 		= .9,
	};
	MakeNewDisplayGroupObject(&def);
}


/********************** FREE LEVELINTRO SCREEN **********************/

static void FreeLevelIntroScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);
	DisposeAllBG3DContainers();
}



/**************** PROCESS LEVELINTRO ********************/

static void ProcessLevelIntro(void)
{
float	timer;

	CalcFramesPerSecond();
	UpdateInput();

	timer = 9.0f;

	while(!UserWantsOut())
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();

		OGL_MoveCameraFrom(0, 0, -65.0f * fps);


				/* DRAW */

		OGL_DrawScene(DrawObjects);

		timer -= fps;
		if (timer < 0.0f)
			break;
	}
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








