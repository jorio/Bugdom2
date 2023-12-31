/******************************/
/*	LEVEL INTRO: PARK.C   */
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
static void MoveIntroHive(ObjNode *theNode);
static void MoveIntroBee(ObjNode *bee);

/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

static ObjNode	*gHive = nil;
static ObjNode *gText = nil;

#define	CloseDoor	Flag[0]


/********************** DO LEVELINTRO SCREEN **************************/

void DoLevelIntroScreen_Park(void)
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
ObjNode	*newObj, *bag, *door;
static OGLPoint3D doorOff = {-135, 350, -50};

	gText = nil;

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

	viewDef.camera.from.x		= -200;
	viewDef.camera.from.y		= -300;
	viewDef.camera.from.z		= 700;

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

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Level10_Park.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);


				/* BUMBLE BEE */

	LoadASkeleton(SKELETON_TYPE_BUMBLEBEE);
	LoadASkeleton(SKELETON_TYPE_HOBOBAG);



			/*******/
			/* CYC */
			/*******/

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PARK_ObjType_Cyclorama;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= CYC_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= gGameView.yon * .995f / 100.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawCyclorama;


				/*************/
				/* MAKE HIVE */
				/*************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PARK_ObjType_Hive;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= -500;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveIntroHive;
	gNewObjectDefinition.rot 		= PI/2;
	gNewObjectDefinition.scale 		= 1.0;
	gHive = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			/* MAKE DOOR */

	OGLPoint3D_Transform(&doorOff, &gHive->BaseTransformMatrix, &gNewObjectDefinition.coord);

	gNewObjectDefinition.type 		= PARK_ObjType_HiveDoor;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot		-= PI/2.05f;
	door = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gHive->ChainNode = door;



				/*******************/
				/* MAKE BEE OBJECT */
				/*******************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_BUMBLEBEE;
	gNewObjectDefinition.animNum 	= 0;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= -200;
	gNewObjectDefinition.coord.z 	= 800;
	gNewObjectDefinition.scale 		= .5;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 499;
	gNewObjectDefinition.moveCall 	= MoveIntroBee;
	gNewObjectDefinition.rot 		= 0;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);


				/* MAKE HOBO BAG */

	gNewObjectDefinition.type 		= SKELETON_TYPE_HOBOBAG;
	gNewObjectDefinition.animNum 	= 1;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	bag = MakeNewSkeletonObject(&gNewObjectDefinition);

	newObj->ChainNode = bag;
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

	float timer = 9.0f;

	ResetFramesPerSecond();

	while(!UserWantsOut())
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();

				/* SPIN CAMERA */

		OGLMatrix4x4_SetRotateAboutPoint(&m, &gHive->Coord, 0, .05f * gFramesPerSecondFrac, 0);
		OGLPoint3D_Transform(&gGameView.cameraPlacement.cameraLocation, &m, &p);
		OGL_UpdateCameraFromTo(&p, nil);


				/* DRAW */

		OGL_DrawScene(DrawObjects);

		timer -= fps;
		if (timer < 0.0f)
			break;
	}
}


#pragma mark -

/************************** MOVE INTRO HIVE ****************************/

static void MoveIntroHive(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*door = theNode->ChainNode;


			/* SEE IF CLOSE DOOR */

	if (theNode->CloseDoor)
	{
		door->Rot.y += fps * 2.5f;
		if (door->Rot.y > (PI/2))
		{
			door->Rot.y = PI/2;

			if (gText == nil)
			{
						/* LEVEL NAME TEXT */

				gNewObjectDefinition.group		= MODEL_GROUP_LEVELINTRO;
				gNewObjectDefinition.type 		= LEVELINTRO_ObjType_ParkText;
				gNewObjectDefinition.scale 		= 1.2;
				gNewObjectDefinition.coord.x	= theNode->Coord.x;
				gNewObjectDefinition.coord.y	= theNode->Coord.y + 430.0;
				gNewObjectDefinition.coord.z	= theNode->Coord.z + 180.0f;
				gNewObjectDefinition.flags 		= STATUS_BIT_NOLIGHTING | STATUS_BIT_DONTCULL;
				gNewObjectDefinition.slot 		= 1000;
				gNewObjectDefinition.moveCall 	= nil;
				gNewObjectDefinition.rot 		= 0;
				gText = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				gText->ColorFilter.a = 0;
			}
			else
			{
				gText->ColorFilter.a += fps;					// fade in text
				if (gText->ColorFilter.a > 1.0f)
					gText->ColorFilter.a = 1.0f;

			}
		}
	}

	UpdateObjectTransforms(door);
}


/******************** MOVE INTRO BEE ***************************/

static void MoveIntroBee(ObjNode *bee)
{
float fps = gFramesPerSecondFrac;
float	r;
ObjNode	*bag = bee->ChainNode;

	GetObjectInfo(bee);

	r = bee->Rot.y;

	gDelta.x = -sin(r) * 200.0f;
	gDelta.z = -cos(r) * 200.0f;

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;

	if (gCoord.z < -100.0f)										// see if in hive
	{
		PlayEffect(EFFECT_DOORCREAK);
		DeleteObject(bee);
		gHive->CloseDoor = true;
		return;
	}

	UpdateObject(bee);

		/* ALIGN BAG */

	if (bag)
	{
		OGLMatrix4x4	m,rm;
		FindJointFullMatrix(bee, BUMBLEBEE_JOINTNUM_HAND, &m);
		OGLMatrix4x4_SetRotate_X(&rm, 0.5f);							// rotate to position
		rm.value[M03] = 0;												// also offset to align to hand
		rm.value[M13] = -5;
		rm.value[M23] = 0;
		OGLMatrix4x4_Multiply(&rm, &m, &bag->BaseTransformMatrix);		// calc bag's matrix
		SetObjectTransformMatrix(bag);

		bag->Coord.x = bag->BaseTransformMatrix.value[M03];					// extract coords of bag
		bag->Coord.y = bag->BaseTransformMatrix.value[M13];
		bag->Coord.z = bag->BaseTransformMatrix.value[M23];
	}


				/* UPDATE EFFECT */

	if (bee->EffectChannel == -1)
		bee->EffectChannel = PlayEffect_Parms3D(EFFECT_BUMBLERUMBLE, &bee->Coord, NORMAL_CHANNEL_RATE + (MyRandomLong() & 0x1fff), 1.0);
	else
		Update3DSoundChannel(EFFECT_BUMBLERUMBLE, &bee->EffectChannel, &bee->Coord);
}





