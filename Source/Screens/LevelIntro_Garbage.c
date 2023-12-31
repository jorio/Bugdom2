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
static const OGLVector3D	fillDirection1 = { -1.0, -.6, -.7 };
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

	OGL_SetupWindow(&viewDef, &gGameView);


				/************/
				/* LOAD ART */
				/************/

			/* LOAD SPRITES */


				/* LOAD AUDIO */

	LoadSoundEffect(EFFECT_TITLEFLYBUZZ);


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:LevelIntro.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELINTRO);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Level10_Park.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL);


				/* SKELETONS */

	LoadASkeleton(SKELETON_TYPE_HOUSEFLY);



			/*******/
			/* CYC */
			/*******/
	
	def = (NewObjectDefinitionType)
	{
		.group		= MODEL_GROUP_LEVELSPECIFIC,
		.type 		= PARK_ObjType_Cyclorama,
		.flags 		= STATUS_BIT_DONTCULL | STATUS_BIT_NOLIGHTING,
		.slot 		= CYC_SLOT,
		.scale 		= gGameView.yon * .995f / 100.0f,
	};
	newObj = MakeNewDisplayGroupObject(&def);

	newObj->CustomDrawFunction = DrawCyclorama;

	newObj->TargetOff.y = -130.0f;


				/* GROUND */
	
	def = (NewObjectDefinitionType)
	{
		.group		= MODEL_GROUP_LEVELINTRO,
		.type		= LEVELINTRO_ObjType_GarbageGround,
		.flags		= STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_DONTCULL,
		.slot		= 5,
		.scale		= 4.0,
	};
	MakeNewDisplayGroupObject(&def);

				/* MAKE GARBAGE CAN */

	def.type 		= LEVELINTRO_ObjType_GarbageCan;
	def.scale 		= 1.3;
	gCan = MakeNewDisplayGroupObject(&def);


			/* LEVEL NAME TEXT */

	def = (NewObjectDefinitionType)
	{
		.group		= MODEL_GROUP_LEVELINTRO,
		.type 		= LEVELINTRO_ObjType_GarbageText,
		.scale 		= 1.0f,
		.coord		= {0,350,0},
		.flags 		= STATUS_BIT_NOLIGHTING | STATUS_BIT_DONTCULL,
		.slot 		= 70,
		.moveCall 	= MoveGarbageTitle,
	};
	MakeNewDisplayGroupObject(&def);




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
		def = (NewObjectDefinitionType)
		{
			.type 		= SKELETON_TYPE_HOUSEFLY,
			.animNum 	= 2,						// flying
			.coord.x 	= RandomFloat2() * 70.0f,
			.coord.y 	= 230.0f + RandomFloat() * 80.0f,
			.coord.z 	= RandomFloat2() * 70.0f,
			.scale 		= .1,
			.flags 		= STATUS_BIT_NOTEXTUREWRAP,
			.slot 		= 499,
			.moveCall 	= MoveIntroFly,
			.rot 		= RandomFloat() * PI2,
		};
		newObj = MakeNewSkeletonObject(&def);

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
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);
	DisposeAllBG3DContainers();
	DisposeSoundEffect(EFFECT_TITLEFLYBUZZ);
}



/**************** PROCESS LEVELINTRO ********************/

static void ProcessLevelIntro(void)
{
OGLMatrix4x4	m;
OGLPoint3D		p;
OGLVector2D		v;

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

		OGLMatrix4x4_SetRotateAboutPoint(&m, &gCan->Coord, 0, .2 * fps, 0);
		OGLPoint3D_Transform(&gGameView.cameraPlacement.cameraLocation, &m, &p);

		v.x = gCan->Coord.x - p.x;										// also move toward can
		v.y = gCan->Coord.z - p.z;
		FastNormalizeVector2D(v.x, v.y, &v, true);

		p.x += v.x * (fps * 25.0f);
		p.z += v.y * (fps * 25.0f);

		OGL_UpdateCameraFromTo(&p, nil);


				/* DRAW */

		OGL_DrawScene(DrawObjects);

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
	theNode->Rot.y = PI+CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->Coord.x, theNode->Coord.z, gGameView.cameraPlacement.cameraLocation.x, gGameView.cameraPlacement.cameraLocation.z);

	UpdateObjectTransforms(theNode);

}

