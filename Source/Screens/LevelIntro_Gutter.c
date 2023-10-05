/******************************/
/*	LEVEL INTRO: GUTTER.C   */
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
static void MoveIntroPinecone(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

static ObjNode	*gPinecone = nil;

/********************** DO LEVELINTRO SCREEN **************************/

void DoLevelIntroScreen_Gutter(void)
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
FSSpec				spec;
OGLSetupInputType	viewDef;
static const OGLVector3D	fillDirection1 = { -1.0, -.6, -.7 };

	InitEffects();

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= .9;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 3000;

	viewDef.styles.useFog			= true;
	viewDef.view.clearBackBuffer	= true;
	viewDef.view.clearColor.r 		= 1;
	viewDef.view.clearColor.g 		= 1;
	viewDef.view.clearColor.b		= .9;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.y		= -20;
	viewDef.camera.from.z		= 150;

	viewDef.camera.to.y 		= 0.0f;

	viewDef.lights.ambientColor.r = .3;
	viewDef.lights.ambientColor.g = .3;
	viewDef.lights.ambientColor.b = .3;

	viewDef.lights.numFillLights 	= 1;

	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillColor[0].r 	= .9;
	viewDef.lights.fillColor[0].g 	= .9;
	viewDef.lights.fillColor[0].b 	= .8;

	OGL_SetupWindow(&viewDef, &gGameView);


				/************/
				/* LOAD ART */
				/************/

			/* LOAD SPRITES */


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:LevelIntro.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELINTRO);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Level7_Gutter.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);



			/* LOAD AUDIO */
	// Note: preloaded in main.c



				/*****************/
				/* MAKE PINECONE */
				/*****************/

	NewObjectDefinitionType def =
	{
		.group		= MODEL_GROUP_LEVELSPECIFIC,
		.type		= GUTTER_ObjType_PineCone,
		.coord		= {0,200,0},
		.flags		= STATUS_BIT_NOTEXTUREWRAP,
		.slot		= 100,
		.moveCall	= MoveIntroPinecone,
		.scale		= 1.5f,
	};
	gPinecone = MakeNewDisplayGroupObject(&def);


			/* CYC */

	def = (NewObjectDefinitionType)
	{
		.group		= MODEL_GROUP_LEVELSPECIFIC,
		.type 		= GUTTER_ObjType_Cyc,
		.flags 		= STATUS_BIT_DONTCULL | STATUS_BIT_NOLIGHTING,
		.slot 		= CYC_SLOT,
		.scale 		= gGameView.yon * .995f / 100.0f,
		.drawCall	= DrawCyclorama,
	};
	MakeNewDisplayGroupObject(&def);
}


/********************** FREE LEVELINTRO SCREEN **********************/

static void FreeLevelIntroScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeEffects();
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);
	DisposeAllBG3DContainers();
}



/**************** PROCESS LEVELINTRO ********************/

static void ProcessLevelIntro(void)
{
OGLMatrix4x4	m;
OGLPoint3D		p;

	float timer = 14.0f;

	while(!UserWantsOut())
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();

				/* SPIN CAMERA */

		OGLMatrix4x4_SetRotateAboutPoint(&m, &gPinecone->Coord, 0, .3f * gFramesPerSecondFrac, 0);
		OGLPoint3D_Transform(&gGameView.cameraPlacement.cameraLocation, &m, &p);
		OGL_UpdateCameraFromTo(&p, &gPinecone->Coord);


				/* DRAW */

		OGL_DrawScene(DrawObjects);

		timer -= fps;
		if (timer < 0.0f)
			break;
	}
}


#pragma mark -

/************************** MOVE INTRO PINECONE ****************************/

static void MoveIntroPinecone(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	theNode->Rot.y += fps * 1.2f;
	theNode->Rot.x += fps * 1.9f;


	gCoord.y -= 30.0f * fps;
	if (gCoord.y < -20.0f)
	{
		ObjNode	*newObj;

		PlayEffect(EFFECT_HITPINECONE);
		ExplodeGeometry(theNode, 200, SHARD_MODE_FROMORIGIN , 1, .4);
		theNode->StatusBits |= STATUS_BIT_NOMOVE | STATUS_BIT_HIDDEN;

				/* LEVEL NAME TEXT */

		NewObjectDefinitionType def =
		{
			.group		= MODEL_GROUP_LEVELINTRO,
			.type 		= LEVELINTRO_ObjType_GutterText,
			.scale 		= .3f,
			.coord		= gCoord,
			.flags 		= STATUS_BIT_NOLIGHTING | STATUS_BIT_DONTCULL | STATUS_BIT_NOTEXTUREWRAP,
			.slot 		= 700,
		};
		newObj = MakeNewDisplayGroupObject(&def);

		TurnObjectTowardTarget(newObj, &gCoord, gGameView.cameraPlacement.cameraLocation.x, gGameView.cameraPlacement.cameraLocation.z, 2000, false);
		newObj->Rot.y -= 2.0;
		UpdateObjectTransforms(newObj);
		return;
	}

	UpdateObject(theNode);
}








