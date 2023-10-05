/******************************/
/*	LEVEL INTRO: FRONT YARD.C */
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
static void MoveLevelIntroSprinklerHead(ObjNode *base);
static void MoveMud(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/



enum
{
	LEVELINTRO_ObjType_Cyc
};



//enum
//{
//	LEVELINTRO_SObjType_LevelIntroText,
//};



/*********************/
/*    VARIABLES      */
/*********************/

#define	WaveXIndex	SpecialF[0]
#define	WaveZIndex	SpecialF[1]


/********************** DO LEVELINTRO SCREEN **************************/

void DoLevelIntroScreen_FrontYard(void)
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
static const OGLVector3D	fillDirection1 = { -.7, -.5, -1.0 };
ObjNode	*newObj, *base, *head;
int		i;

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= .95;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 2500;

	viewDef.styles.useFog			= true;
	viewDef.styles.fogStart			= viewDef.camera.yon * .6f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * 1.5f;

	viewDef.view.clearBackBuffer	= true;
	viewDef.view.clearColor.r 		= 1;
	viewDef.view.clearColor.g 		= 1;
	viewDef.view.clearColor.b		= 1;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.y		= 450;
	viewDef.camera.from.z		= 1000;

	viewDef.camera.to.y 		= 350.0f;

	viewDef.lights.ambientColor.r = .3;
	viewDef.lights.ambientColor.g = .3;
	viewDef.lights.ambientColor.b = .3;

	viewDef.lights.numFillLights 	= 1;

	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillColor[0].r 	= 1.0;
	viewDef.lights.fillColor[0].g 	= .9;
	viewDef.lights.fillColor[0].b 	= .9;

	OGL_SetupWindow(&viewDef, &gGameView);


				/************/
				/* LOAD ART */
				/************/

			/* LOAD SPRITES */



			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:LevelIntro.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELINTRO);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Level1_Garden.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARDEN_ObjType_SprinklerBase,
									0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARDEN_ObjType_SprinklerPost,
									0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);


				/* LOAD AUDIO */
	// Note: preloaded in main.c



			/*******************/
			/* MAKE BACKGROUND */
			/*******************/

				/* GROUND */

	def = (NewObjectDefinitionType)
	{
		.group		= MODEL_GROUP_LEVELINTRO,
		.type		= LEVELINTRO_ObjType_Level1Ground,
		.flags		= STATUS_BIT_DONTCULL,
		.slot		= 2,
		.scale		= 3.0f,
	};
	MakeNewDisplayGroupObject(&def);

			/* WIDESCREEN FILL LEFT */

	def.coord.x		= -2250;
	def.coord.y		= -20;
	MakeNewDisplayGroupObject(&def);

			/* WIDESCREEN FILL RIGHT */

	def.coord.x		= 2250;
	def.coord.y		= -20;
	MakeNewDisplayGroupObject(&def);

			/* CYC */

	def = (NewObjectDefinitionType)
	{
		.group		= MODEL_GROUP_LEVELSPECIFIC,
		.type		= GARDEN_ObjType_Cyclorama,
		.flags		= STATUS_BIT_DONTCULL | STATUS_BIT_NOLIGHTING,
		.slot		= CYC_SLOT,
		.scale		= gGameView.yon * .995f / 100.0f,
		.drawCall	= DrawCyclorama,
	};
	newObj = MakeNewDisplayGroupObject(&def);
	newObj->TargetOff.y = -300.0f;


			/* SPRINKLER */

	def = (NewObjectDefinitionType)
	{
		.group 		= MODEL_GROUP_LEVELSPECIFIC,
		.type 		= GARDEN_ObjType_SprinklerBase,
		.scale 		= 2.5f,
		.slot 		= 300,
		.moveCall 	= MoveLevelIntroSprinklerHead,
	};
	base = MakeNewDisplayGroupObject(&def);
	base->Timer = 1.0;			// set delay till pop-up


	def.type 		= GARDEN_ObjType_SprinklerPost;
	def.moveCall 	= nil;
	head = MakeNewDisplayGroupObject(&def);

	base->ChainNode = head;

	gSprinklerMode = SPRINKLER_MODE_OFF;
	gSprinklerPopUpOffset = gSprinklerTimer = 0;


			/* LEVEL NAME TEXT */

	def = (NewObjectDefinitionType)
	{
		.group		= MODEL_GROUP_LEVELINTRO,
		.type 		= LEVELINTRO_ObjType_Level1Text,
		.scale 		= 1.7f,
		.coord		= {0,490,0},
		.flags 		= STATUS_BIT_NOLIGHTING | STATUS_BIT_DONTCULL | STATUS_BIT_NOTEXTUREWRAP,
		.slot 		= 70,
	};
	MakeNewDisplayGroupObject(&def);


			/* MUD SPLOTCHES */

	for (i = 0; i < 100; i++)
	{
		def = (NewObjectDefinitionType)
		{
			.group		= MODEL_GROUP_LEVELINTRO,
			.type 		= LEVELINTRO_ObjType_MudSplotch,
			.coord.x 	= RandomFloat2() * 400.0f,
			.coord.y 	= 500.0f + RandomFloat2() * 50.0f,
			.coord.z 	= 0,
			.flags 		= STATUS_BIT_NOLIGHTING | STATUS_BIT_DONTCULL | STATUS_BIT_NOZBUFFER,
			.slot 		= 90,
			.moveCall 	= MoveMud,
			.scale 		= 1.0f + RandomFloat() * .5f,
		};
		newObj = MakeNewDisplayGroupObject(&def);

		newObj->ColorFilter.a = 1.5;
		newObj->SpecialF[0] = RandomFloat2() * .9f;		// target Delta Rot Z
		newObj->Rot.z = RandomFloat() * PI2;
		UpdateObjectTransforms(newObj);

		newObj->Timer = 2.0f + RandomFloat() * 3.0f;
	}
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


				/* MOVE CAMERA */

		gGameView.cameraPlacement.cameraLocation.z -= 30.0f * fps;



				/* MOVE */

		MoveObjects();




				/* DRAW */

		OGL_DrawScene(DrawObjects);

		timer -= fps;
		if (timer < 0.0f)
			break;
	}
}



#pragma mark -


/***************** MOVE LEVEL INTRO: SPRINKLER HEAD **********************/

static void MoveLevelIntroSprinklerHead(ObjNode *base)
{
ObjNode	*head = base->ChainNode;
ObjNode	*spray = head->ChainNode;
float	fps = gFramesPerSecondFrac;

			/*************/
			/* MOVE HEAD */
			/*************/

	switch(gSprinklerMode)
	{
				/* OFF */

		case	SPRINKLER_MODE_OFF:
				base->Timer -= fps;
				if (base->Timer <= 0.0f)
					gSprinklerMode = SPRINKLER_MODE_UP;
				break;

				/* MOVE UP */

		case	SPRINKLER_MODE_UP:
				gSprinklerPopUpOffset += fps * 700.0f;
				if (gSprinklerPopUpOffset >= 240.0f)						// see if all the way up
				{
					gSprinklerPopUpOffset = 240.0f;
					gSprinklerMode = SPRINKLER_MODE_ON;
					gSprinklerTimer = 5.0f;
				}
				break;
	}

			/* SET HEAD'S POSITION */

	head->Coord.y = head->InitCoord.y + gSprinklerPopUpOffset;
	UpdateObjectTransforms(head);


			/***********************/
			/* SEE IF UPDATE SPRAY */
			/***********************/

	if (gSprinklerMode == SPRINKLER_MODE_ON)
	{

				/* MAKE SPRAY */

		if (spray == nil)
		{
			NewObjectDefinitionType def =
			{
				.group		= MODEL_GROUP_LEVELSPECIFIC,
				.type		= GARDEN_ObjType_SprinklerSpray,
				.coord		= head->Coord,
				.flags		= STATUS_BIT_DOUBLESIDED | STATUS_BIT_NOZWRITES | STATUS_BIT_NOLIGHTING,
				.slot		= SLOT_OF_DUMB,
				.scale		= head->Scale.x,
			};
			spray = MakeNewDisplayGroupObject(&def);

			head->ChainNode = spray;
		}
		spray->Rot.y = RandomFloat()*PI2;
		UpdateObjectTransforms(spray);


				/* SOUND */

		if (base->EffectChannel == -1)
			base->EffectChannel = PlayEffect_Parms3D(EFFECT_SPRINKLER, &base->Coord, NORMAL_CHANNEL_RATE + (MyRandomLong()&0x3fff), 1.0);
		else
			Update3DSoundChannel(EFFECT_SPRINKLER, &base->EffectChannel, &base->Coord);
	}

				/* IF NOT ON, THEN MAKE SURE SPRAY IS GONE */

	else
	{
		if (spray)
		{
			DeleteObject(spray);
			spray = nil;
			head->ChainNode = nil;
		}
		StopAChannel(&base->EffectChannel);
	}

}


#pragma mark -

/************************** MOVE MUD ********************************/

static void MoveMud(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Timer -= fps;
	if (theNode->Timer <= 0.0f)
	{
		GetObjectInfo(theNode);

		if (theNode->DeltaRot.z < theNode->SpecialF[0])
			theNode->DeltaRot.z += fps * .5f;
		else
		if (theNode->DeltaRot.z > theNode->SpecialF[0])
			theNode->DeltaRot.z -= fps * .5f;

		theNode->Rot.z += theNode->DeltaRot.z * fps;


				/* MOVE */

		gDelta.y -= 80.0f * fps;
		gCoord.y += gDelta.y * fps;


				/* FADE */

		theNode->ColorFilter.a -= fps * .8f;
		if (theNode->ColorFilter.a <= 0.0f)
		{
			DeleteObject(theNode);
			return;
		}

				/* UPDATE */

		UpdateObject(theNode);
	}

}

















