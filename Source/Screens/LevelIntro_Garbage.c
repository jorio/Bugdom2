/******************************/
/*	LEVEL INTRO: GARBAGE.C   */
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
static void MoveGarbageTitle(ObjNode *theNode);
static void MoveIntroFly(ObjNode *fly);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

static	ObjNode	 *gCan;

/********************** DO LEVELINTRO SCREEN **************************/

void DoLevelIntroScreen_Garbage(void)
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
const static OGLVector3D	fillDirection1 = { -1.0, -.6, -.7 };
ObjNode	*newObj;
int			i,n;

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.0;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 2000;

	viewDef.styles.useFog			= false;
	viewDef.view.clearBackBuffer	= false;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.y		= 300;
	viewDef.camera.from.z		= 500;

	viewDef.camera.to.y 		= 200.0f;

	viewDef.lights.ambientColor.r = .3;
	viewDef.lights.ambientColor.g = .3;
	viewDef.lights.ambientColor.b = .3;

	viewDef.lights.numFillLights 	= 1;

	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillColor[0].r 	= 1.0;
	viewDef.lights.fillColor[0].g 	= 1.0;
	viewDef.lights.fillColor[0].b 	= .9;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

			/* LOAD SPRITES */

//	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:global.sprites", &spec);
//	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL, gGameViewInfoPtr);

//	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":sprite//s:spheremap.sprites", &spec);
//	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS, gGameViewInfoPtr);

				/* LOAD AUDIO */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":audio:Title.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_TITLE);


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:LevelIntro.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELINTRO, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Level10_Park.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL, gGameViewInfoPtr);


				/* SKELETONS */

	LoadASkeleton(SKELETON_TYPE_HOUSEFLY, gGameViewInfoPtr);



			/*******/
			/* CYC */
			/*******/

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PARK_ObjType_Cyclorama;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= TERRAIN_SLOT+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= gGameViewInfoPtr->yon * .995f / 100.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawCyclorama;

	newObj->TargetOff.y = -130.0f;


				/* GROUND */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= LEVELINTRO_ObjType_GarbageGround;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 5;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 4.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				/* MAKE GARBAGE CAN */

	gNewObjectDefinition.type 		= LEVELINTRO_ObjType_GarbageCan;
	gNewObjectDefinition.scale 		= 1.3;
	gCan = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* LEVEL NAME TEXT */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= LEVELINTRO_ObjType_GarbageText;
	gNewObjectDefinition.scale 		= 1.0;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 350;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED | STATUS_BIT_NOLIGHTING | STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveGarbageTitle;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);




				/*********************/
				/* MAKE FLIES OBJECT */
				/*********************/

	if (gG4)
		n = 30;
	else
	if (gSlowCPU)
		n = 10;
	else
		n = 15;

	for (i = 0; i < n; i++)
	{
		gNewObjectDefinition.type 		= SKELETON_TYPE_HOUSEFLY;
		gNewObjectDefinition.animNum 	= 2;						// flying
		gNewObjectDefinition.coord.x 	= RandomFloat2() * 70.0f;
		gNewObjectDefinition.coord.y 	= 230.0f + RandomFloat() * 80.0f;
		gNewObjectDefinition.coord.z 	= RandomFloat2() * 70.0f;
		gNewObjectDefinition.scale 		= .1;
		gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot 		= 499;
		gNewObjectDefinition.moveCall 	= MoveIntroFly;
		gNewObjectDefinition.rot 		= RandomFloat() * PI2;

		newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

		newObj->Skeleton->CurrentAnimTime = RandomFloat() * .1f;

		newObj->Coord.x += RandomFloat2() * 80.0f;
		newObj->Coord.z += RandomFloat2() * 80.0f;
		newObj->Coord.y += RandomFloat2() * 10.0f;

		newObj->DeltaRot.y = RandomFloat2() * 8.0f;
		if (newObj->DeltaRot.y < 0.0f)
			newObj->DeltaRot.y -= 5.0f;
		else
			newObj->DeltaRot.y += 5.0f;

	}
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
OGLMatrix4x4	m;
OGLPoint3D		p;
OGLVector2D		v;

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

				/* SPIN CAMERA */

		OGLMatrix4x4_SetRotateAboutPoint(&m, &gCan->Coord, 0, .2 * fps, 0);
		OGLPoint3D_Transform(&gGameViewInfoPtr->cameraPlacement.cameraLocation, &m, &p);

		v.x = gCan->Coord.x - p.x;										// also move toward can
		v.y = gCan->Coord.z - p.z;
		FastNormalizeVector2D(v.x, v.y, &v, true);

		p.x += v.x * (fps * 25.0f);
		p.z += v.y * (fps * 25.0f);

		OGL_UpdateCameraFromTo(gGameViewInfoPtr, &p, nil);


				/* DRAW */

		OGL_DrawScene(gGameViewInfoPtr, DrawObjects);

		timer -= fps;
		if (timer < 0.0f)
			break;
	}
}

#pragma mark -

/******************** MOVE INTRO FLY ***************************/

static void MoveIntroFly(ObjNode *fly)
{
float fps = gFramesPerSecondFrac;
OGLMatrix4x4	m;

	GetObjectInfo(fly);

	OGLMatrix4x4_SetRotateAboutPoint(&m, &fly->InitCoord, 0, fly->DeltaRot.y * fps, 0);
	OGLPoint3D_Transform(&gCoord, &m, &gCoord);

	TurnObjectTowardTarget(fly, &fly->OldCoord, gCoord.x, gCoord.z, 30.0f, false);

	UpdateObject(fly);



				/* UPDATE EFFECT */

	if (fly->EffectChannel == -1)
		fly->EffectChannel = PlayEffect_Parms3D(EFFECT_TITLEFLYBUZZ, &gCoord, NORMAL_CHANNEL_RATE + (MyRandomLong() & 0x3fff), .1);
	else
		Update3DSoundChannel(EFFECT_TITLEFLYBUZZ, &fly->EffectChannel, &gCoord);
}


/**************** MOVE GARBAGE TITLE **********************/

static void MoveGarbageTitle(ObjNode *theNode)
{
	theNode->Rot.y = PI+CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->Coord.x, theNode->Coord.z, gGameViewInfoPtr->cameraPlacement.cameraLocation.x, gGameViewInfoPtr->cameraPlacement.cameraLocation.z);

	UpdateObjectTransforms(theNode);

}

