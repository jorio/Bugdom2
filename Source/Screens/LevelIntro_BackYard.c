/******************************/
/*	LEVEL INTRO: BACK YARD.C */
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
static void MoveIntroBottle(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

/*********************/
/*    VARIABLES      */
/*********************/



/********************** DO LEVELINTRO SCREEN **************************/

void DoLevelIntroScreen_BackYard(void)
{
			/* SETUP */

	SetupLevelIntroScreen();
	MakeFadeEvent(true, 1);

	ProcessLevelIntro();


			/* CLEANUP */

	OGL_FadeOutScene(DrawLevelIntroCallback, NULL);
	FreeLevelIntroScreen();
}



/********************* SETUP LEVELINTRO SCREEN **********************/

static void SetupLevelIntroScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
static const OGLVector3D	fillDirection1 = { -1.0, -.6, -.7 };
ObjNode	*newObj;
int		i,x,z;

	InitShardSystem();

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.1;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 3600;

	viewDef.styles.useFog			= true;
	viewDef.styles.fogStart			= viewDef.camera.yon * .5f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * .9f;

	viewDef.view.clearBackBuffer	= true;
	viewDef.view.clearColor.r 		= 1;
	viewDef.view.clearColor.g 		= 1;
	viewDef.view.clearColor.b		= 1;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.y		= 150;
	viewDef.camera.from.z		= 800;

	viewDef.camera.to.y 		= 400.0f;

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

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Level2_Sidewalk.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);

	for (i = 0; i < 5; i++)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SIDEWALK_ObjType_Bottle+i,
										-1, MULTI_TEXTURE_COMBINE_ADDALPHA, SPHEREMAP_SObjType_SheenAlpha);
	}



				/* LOAD AUDIO */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Garden.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);



			/*******************/
			/* MAKE BACKGROUND */
			/*******************/

				/* GROUND */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= LEVELINTRO_ObjType_Level2Ground;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= -480;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 5;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 8.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* CYC */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= GARDEN_ObjType_Cyclorama;
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

	newObj->TargetOff.y = -300.0f;

			/* BOTTLES */

	i = 0;
	for (z = -800; z < 300.0f; z += 300.0f)
	{
		for (x = -1100.0f + RandomFloat() * 200.0f; x < 1100.0f; x += 300.0f)
		{
			gNewObjectDefinition.group		= MODEL_GROUP_LEVELSPECIFIC;
			gNewObjectDefinition.type 		= SIDEWALK_ObjType_Bottle;
			gNewObjectDefinition.coord.x 	= x + RandomFloat2() * 20.0f;
			gNewObjectDefinition.coord.y 	= 0;
			gNewObjectDefinition.coord.z 	= z + RandomFloat2() * 20.0f;
			gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
			gNewObjectDefinition.slot 		= 90 + i;
			gNewObjectDefinition.moveCall 	= MoveIntroBottle;
			gNewObjectDefinition.rot 		= RandomFloat() * PI2;
			gNewObjectDefinition.scale 		= 2.0f;
			newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			newObj->Timer = 2.0f + RandomFloat() * 7.0f;
			newObj->Mode = 0;

			i++;
		}
	}

			/* LEVEL NAME TEXT */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= LEVELINTRO_ObjType_Level2Text;
	gNewObjectDefinition.scale 		= 5.0;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 260;
	gNewObjectDefinition.coord.z 	= -1000;
	gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED | STATUS_BIT_NOLIGHTING | STATUS_BIT_DONTCULL | STATUS_BIT_NOFOG | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 50;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


}


/********************** FREE LEVELINTRO SCREEN **********************/

static void FreeLevelIntroScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUND_BANK_LEVELSPECIFIC);
}



/**************** PROCESS LEVELINTRO ********************/

static void ProcessLevelIntro(void)
{
float	timer;

	CalcFramesPerSecond();
	UpdateInput();

	timer = 19.0f;

	while(!UserWantsOut())
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();
		MoveShards();

				/* MOVE CAMERA */

		gGameView->cameraPlacement.cameraLocation.y += 30.0f * fps;


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
	DrawShards();
}

#pragma mark -

/************************** MOVE INTRO BOTTLE ****************************/

static void MoveIntroBottle(ObjNode *theNode)
{
	theNode->Timer -= gFramesPerSecondFrac;
	if (theNode->Timer <= 0.0f)
	{
		theNode->Timer = .5f + RandomFloat() * 1.5f;

		theNode->Mode++;
		if (theNode->Mode >= 5)
		{
			PlayEffect_Parms3D(EFFECT_BOTTLESHATTER, &theNode->Coord, NORMAL_CHANNEL_RATE + (MyRandomLong() & 0x3ffff), 1.0);
			ExplodeGeometry(theNode, 300, SHARD_MODE_FROMORIGIN, 1, .5);
			DeleteObject(theNode);
			return;
		}

				/* UPDATE CRACK */

		theNode->Type++;
		ResetDisplayGroupObject(theNode);
		PlayEffect_Parms3D(EFFECT_BOTTLECRACK, &theNode->Coord, NORMAL_CHANNEL_RATE + (MyRandomLong() & 0x3ffff), 1.0);
	}
}








