/******************************/
/*	LEVEL INTRO: PLAYROOM.C   */
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
static void MoveIntroSoldier(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/



/********************** DO LEVELINTRO SCREEN **************************/

void DoLevelIntroScreen_Playroom(void)
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
ObjNode	*newObj, *block;
int		i,x;

	InitShardSystem();

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.1;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 3800;

	viewDef.styles.useFog			= true;
	viewDef.styles.fogStart			= viewDef.camera.yon * .6f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * 1.1f;

	viewDef.view.clearBackBuffer	= true;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.y		= 400;
	viewDef.camera.from.z		= 800;

	viewDef.camera.to.y 		= 200.0f;

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

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:global.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS);



			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:LevelIntro.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELINTRO);

	for (i = 0; i < 7; i++)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELINTRO, LEVELINTRO_ObjType_LetterBlockA+i,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	}

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Level5_Playroom.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);


				/* SKELETONS */

	LoadASkeleton(SKELETON_TYPE_TOYSOLDIER);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_TOYSOLDIER, 0,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);





			/*******************/
			/* MAKE BACKGROUND */
			/*******************/

				/* GROUND */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= LEVELINTRO_ObjType_PlayroomGround;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= -1800;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 5;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 5.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CType 			= CTYPE_BLOCKSHADOW;

	SetObjectCollisionBounds(newObj, 0, -100, -2000, 2000, 2000, -2000);


			/* ARMY MEN */

	x = -1300;
	for (i = 0; i < 8; i++)
	{
		const short blocks[] =
		{
			LEVELINTRO_ObjType_LetterBlockP,
			LEVELINTRO_ObjType_LetterBlockL,
			LEVELINTRO_ObjType_LetterBlockA,
			LEVELINTRO_ObjType_LetterBlockY,
			LEVELINTRO_ObjType_LetterBlockR,
			LEVELINTRO_ObjType_LetterBlockO,
			LEVELINTRO_ObjType_LetterBlockO,
			LEVELINTRO_ObjType_LetterBlockM
		};

				/* SOLIDER */

		gNewObjectDefinition.type 		= SKELETON_TYPE_TOYSOLDIER;
		gNewObjectDefinition.animNum	= TOYSOLDIER_ANIM_CARRY;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= -200;
		gNewObjectDefinition.coord.y 	= 190;
		gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.moveCall	= MoveIntroSoldier;
		gNewObjectDefinition.rot 		= -PI/2;
		gNewObjectDefinition.scale 		= 3.5;
		newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

		newObj->Skeleton->CurrentAnimTime = RandomFloat() * 20.0f;

		AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 7, 7, true);



				/* BLOCK */

		gNewObjectDefinition.group		= MODEL_GROUP_LEVELINTRO;
		gNewObjectDefinition.type 		= blocks[7-i];
		gNewObjectDefinition.coord	 	= newObj->Coord;
		gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
		gNewObjectDefinition.slot 		= newObj->Slot+1;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= 1.5;
		block = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->ChainNode = block;

		AttachShadowToObject(newObj, SHADOW_TYPE_SQUARE, 6, 6, true);


		x -= 230.0f;
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
	OGL_DisposeWindowSetup(&gGameView);
}



/**************** PROCESS LEVELINTRO ********************/

static void ProcessLevelIntro(void)
{
float	timer;

	CalcFramesPerSecond();
	UpdateInput();

	timer = 11.0f;

	while(!UserWantsOut())
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		gGameView->cameraPlacement.cameraLocation.x += 20.0f * fps;
		gGameView->cameraPlacement.pointOfInterest.x += 20.0f * fps;

		MoveObjects();
		MoveShards();

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

/************************** MOVE INTRO SOLDIER ****************************/

static void MoveIntroSoldier(ObjNode *theNode)
{
ObjNode			*block = theNode->ChainNode;
OGLMatrix4x4	m,jm;

	GetObjectInfo(theNode);

	gCoord.x += 400.0f * gFramesPerSecondFrac;

	UpdateObject(theNode);


				/* ALIGN BLOCK */

	OGLMatrix4x4_SetRotate_XYZ(&m, PI, 0, -PI/4);

	FindJointFullMatrix(theNode, TOYSOLDIER_JOINTNUM_RIGHTHAND, &jm);
	OGLMatrix4x4_Multiply(&m, &jm, &block->BaseTransformMatrix);
	SetObjectTransformMatrix(block);

	UpdateShadow(theNode);
}








