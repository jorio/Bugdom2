/******************************/
/*	LEVEL INTRO: BALSA.C   */
/* (c)2002 Pangea Software    */
/* By Brian Greenstone        */
/******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gGlobalTransparency;
extern	FSSpec		gDataSpec;
extern	Boolean		gGameOver;
extern	KeyMap gKeyMap,gNewKeys;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,gDisableAnimSounds;
extern	PrefsType	gGamePrefs;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	int				gLevelNum;
extern	OGLPoint3D	gCoord;
extern	OGLVector3D	gDelta;
extern	u_long			gScore,gGlobalMaterialFlags;
extern	Byte	gSprinklerMode;
extern	float	gSprinklerPopUpOffset,gSprinklerTimer;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupLevelIntroScreen(void);
static void FreeLevelIntroScreen(void);
static void DrawLevelIntroCallback(OGLSetupOutputType *info);
static void ProcessLevelIntro(void);
static void MoveIntroPlane(ObjNode *theNode);
static void MoveBalsaSky(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/



enum
{
	LEVELINTRO_ObjType_Cyc
};


#define	 WobbleZ	SpecialF[0]
#define	 WobbleX	SpecialF[1]


/*********************/
/*    VARIABLES      */
/*********************/

static ObjNode *gBanner;


/********************** DO LEVELINTRO SCREEN **************************/

void DoLevelIntroScreen_Balsa(void)
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
const static OGLVector3D	fillDirection1 = { -.6, -.9, -1.0 };
ObjNode	*newObj, *prop, *band;

	InitShardSystem();

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.1;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 3000;

	viewDef.styles.useFog			= true;
	viewDef.view.clearBackBuffer	= true;
	viewDef.view.clearColor.r 		= 1;
	viewDef.view.clearColor.g 		= 1;
	viewDef.view.clearColor.b		= .9;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.y		= 200;
	viewDef.camera.from.z		= 500;

	viewDef.camera.to.y 		= 0.0f;

	viewDef.lights.ambientColor.r = .35;
	viewDef.lights.ambientColor.g = .35;
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


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Models:LevelIntro.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELINTRO, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Models:Level9_Balsa.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC, gGameViewInfoPtr);




				/* LOAD AUDIO */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Audio:Balsa.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);


				/*********************/
				/* MAKE THE SKY DOME */
				/*********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= LEVELINTRO_ObjType_GliderSky;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|STATUS_BIT_ROTYZX;
	gNewObjectDefinition.slot 		= TERRAIN_SLOT+1;					// draw after terrain for better performance since terrain blocks much of the pixels
	gNewObjectDefinition.moveCall 	= MoveBalsaSky;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= gGameViewInfoPtr->yon * .85f / 100.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawCyclorama;

	newObj->TargetOff.y = -300.0f;

	newObj->Rot.x = -.4;

				/************************/
				/* MAKE THE BALSA PLANE */
				/************************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BALSA_ObjType_Plane;
	gNewObjectDefinition.coord.x 	= 1100;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= -150;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_ROTZXY;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveIntroPlane;
	gNewObjectDefinition.rot 		= PI/2;
	gNewObjectDefinition.scale 		= 1.5f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->EffectChannel = PlayEffect_Parms3D(EFFECT_PROPELLER, &newObj->Coord, NORMAL_CHANNEL_RATE, 1.0);


					/********************/
					/* ATTACH PROPELLER */
					/********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BALSA_ObjType_Prop;
	gNewObjectDefinition.coord	 	= newObj->Coord;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_DOUBLESIDED;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	prop = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ChainNode = prop;

					/**********************/
					/* ATTACH RUBBER BAND */
					/**********************/

	gNewObjectDefinition.type 		= BALSA_ObjType_RubberBand;
	gNewObjectDefinition.coord	 	= prop->Coord;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot++;
	band = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	prop->ChainNode = band;


				/*******************/
				/* MAKE THE BANNER */
				/*******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= LEVELINTRO_ObjType_PlaneBanner;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_DOUBLESIDED;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.scale 		= 2.0;
	gNewObjectDefinition.rot 		= -PI/2;
	gBanner = MakeNewDisplayGroupObject(&gNewObjectDefinition);

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

	CalcFramesPerSecond();
	UpdateInput();

	timer = 10.0f;

	while(!AreAnyNewKeysPressed())
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();
		MoveShards();

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

/************************** MOVE INTRO PLANE ****************************/

static void MoveIntroPlane(ObjNode *plane)
{
ObjNode	*prop, *band;
float	fps = gFramesPerSecondFrac;
OGLMatrix4x4	m;

	GetObjectInfo(plane);

	gCoord.x -= 300.0f * fps;

			/* DO ROT WOBBLE */

	plane->WobbleZ += fps * 4.0f;
	plane->Rot.z = sin(plane->WobbleZ) * .2f;

	UpdateObject(plane);
	Update3DSoundChannel(EFFECT_PROPELLER, &plane->EffectChannel, &gCoord);


				/* ALIGN PROP */

	prop = plane->ChainNode;															// get prop obj

	prop->Rot.z -= fps * 60.0f;															// spin prop

	OGLMatrix4x4_SetRotate_Z(&m, prop->Rot.z);											// set rot matrix
	m.value[M03] = 0;
	m.value[M13] = -16;
	m.value[M23] = -132;																// offset to nose of plane
	OGLMatrix4x4_Multiply(&m, &plane->BaseTransformMatrix, &prop->BaseTransformMatrix); // set matrix
	SetObjectTransformMatrix(prop);


				/* ALIGN RUBBER BAND */

	band = prop->ChainNode;

	band->Rot.z -= fps * 60.0f;															// spin prop

	OGLMatrix4x4_SetRotate_Z(&m, band->Rot.z);											// set rot matrix
	m.value[M03] = 0;
	m.value[M13] = -15;
	m.value[M23] = -30;																// offset to nose of plane
	OGLMatrix4x4_Multiply(&m, &plane->BaseTransformMatrix, &band->BaseTransformMatrix); // set matrix
	SetObjectTransformMatrix(band);


			/* ALIGN BANNER */

	gBanner->Coord.x = gCoord.x + 250.0f;
	gBanner->Coord.y = gCoord.y - 20.0f;
	gBanner->Coord.z = gCoord.z;

	UpdateObjectTransforms(gBanner);



}


/***************** MOVE BALSA SKY ***********************/

static void MoveBalsaSky(ObjNode *theNode)
{
	theNode->Rot.y -= gFramesPerSecondFrac * .3f;

}









