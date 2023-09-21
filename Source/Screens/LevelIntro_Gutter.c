/******************************/
/*	LEVEL INTRO: GUTTER.C   */
/* (c)2002 Pangea Software    */
/* By Brian Greenstone        */
/******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gGlobalTransparency;
extern	FSSpec				gDataSpec;
extern	Boolean				gGameOver;
extern	KeyMap 				gKeyMap,gNewKeys;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean				gSongPlayingFlag,gDisableAnimSounds;
extern	PrefsType			gGamePrefs;
extern	OGLSetupOutputType	*gGameViewInfoPtr;
extern	int				gLevelNum;
extern	OGLPoint3D	gCoord;
extern	OGLVector3D	gDelta;
extern	uint32_t			gScore,gGlobalMaterialFlags;
extern	Byte	gSprinklerMode;
extern	float	gSprinklerPopUpOffset,gSprinklerTimer;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupLevelIntroScreen(void);
static void FreeLevelIntroScreen(void);
static void DrawLevelIntroCallback(OGLSetupOutputType *info);
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

	InitShardSystem();

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

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

			/* LOAD SPRITES */

//	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:global.sprites", &spec);
//	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL, gGameViewInfoPtr);

//	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:spheremap.sprites", &spec);
//	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS, gGameViewInfoPtr);



			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:LevelIntro.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELINTRO, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Level7_Gutter.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC, gGameViewInfoPtr);



			/* LOAD AUDIO */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Plumbing.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);



				/*****************/
				/* MAKE PINECONE */
				/*****************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= GUTTER_ObjType_PineCone;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 200;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveIntroPinecone;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.5f;
	gPinecone = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* CYC */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= GUTTER_ObjType_Cyc;
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

	CalcFramesPerSecond();
	UpdateInput();

	timer = 14.0f;

	while(!AreAnyNewKeysPressed())
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();
		MoveShards();

				/* SPIN CAMERA */

		OGLMatrix4x4_SetRotateAboutPoint(&m, &gPinecone->Coord, 0, .3f * gFramesPerSecondFrac, 0);
		OGLPoint3D_Transform(&gGameViewInfoPtr->cameraPlacement.cameraLocation, &m, &p);
		OGL_UpdateCameraFromTo(gGameViewInfoPtr, &p, &gPinecone->Coord);


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
	DrawShards(info);
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

		gNewObjectDefinition.group		= MODEL_GROUP_LEVELINTRO;
		gNewObjectDefinition.type 		= LEVELINTRO_ObjType_GutterText;
		gNewObjectDefinition.scale 		= .3;
		gNewObjectDefinition.coord		= gCoord;
		gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED | STATUS_BIT_NOLIGHTING | STATUS_BIT_DONTCULL | STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot 		= 700;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		TurnObjectTowardTarget(newObj, &gCoord, gGameViewInfoPtr->cameraPlacement.cameraLocation.x, gGameViewInfoPtr->cameraPlacement.cameraLocation.z, 2000, false);
		newObj->Rot.y -= 2.0;
		UpdateObjectTransforms(newObj);
		return;
	}

	UpdateObject(theNode);
}








