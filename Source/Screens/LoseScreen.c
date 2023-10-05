/****************************/
/*   	LOSE SCREEN.C	*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupLoseScreen(void);
static void FreeLoseScreen(void);
static void ProcessLose(void);
static void MoveMenuFlower(ObjNode *theNode);
static void MoveLoseBee(ObjNode *bee);
static void MoveGameOverText(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/



enum
{
	LOSE_ObjType_Cyc
};



enum
{
	LOSE_SObjType_GameOver,
	LOSE_SObjType_COUNT,
};



/*********************/
/*    VARIABLES      */
/*********************/

#define	WaveXIndex	SpecialF[0]
#define	WaveZIndex	SpecialF[1]


/********************** DO LOSE SCREEN **************************/

void DoLoseScreen(void)
{

			/* SETUP */

	SetupLoseScreen();
	MakeFadeEvent(true, 1);

	ProcessLose();


			/* CLEANUP */

	OGL_FadeOutScene(DrawObjects, NULL);
	FreeLoseScreen();
}



/********************* SETUP LOSE SCREEN **********************/

static void SetupLoseScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
static const OGLVector3D	fillDirection1 = { -.7, -.5, -1.0 };
static const OGLVector3D	fillDirection2 = { .3, .8, 1.0 };
ObjNode	*newObj, *bee, *bag;
int			i;

			/* SET ANAGLYPH INFO */

	if (gGamePrefs.anaglyph)
	{
		gAnaglyphScaleFactor 	= 1.0f;
		gAnaglyphFocallength	= 200.0f * gAnaglyphScaleFactor;	// set camera info
		gAnaglyphEyeSeparation 	= 25.0f * gAnaglyphScaleFactor;
	}


			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.0;
	viewDef.camera.hither 		= 30;
	viewDef.camera.yon 			= 9000;

	viewDef.styles.useFog			= false;
	viewDef.view.clearBackBuffer	= true;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.y		= 0;
	viewDef.camera.from.z		= 800;

	viewDef.camera.to.y 		= 70.0f;

	viewDef.lights.ambientColor.r = .15;
	viewDef.lights.ambientColor.g = .15;
	viewDef.lights.ambientColor.b = .15;

	viewDef.lights.numFillLights 	= 2;

	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillColor[0].r 	= .3;
	viewDef.lights.fillColor[0].g 	= .3;
	viewDef.lights.fillColor[0].b 	= .3;

	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[1].r 	= 1.0;
	viewDef.lights.fillColor[1].g 	= .8;
	viewDef.lights.fillColor[1].b 	= .1;

	OGL_SetupWindow(&viewDef, &gGameView);


				/************/
				/* LOAD ART */
				/************/

	InitEffects();

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:LoseScreen.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LOSESCREEN);

	LoadFoliage();


			/* LOAD SPRITES */

	LoadSpriteGroupFromSeries(SPRITE_GROUP_LEVELSPECIFIC, LOSE_SObjType_COUNT, "LoseScreen");


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_BUMBLEBEE);
	LoadASkeleton(SKELETON_TYPE_HOBOBAG);


				/* LOAD AUDIO */



			/*******************/
			/* MAKE BACKGROUND */
			/*******************/

			/* MAKE TULIPS */

	for (i = 0; i < 20; i++)
	{

		gNewObjectDefinition.group 		= MODEL_GROUP_FOLIAGE;
		gNewObjectDefinition.type 		= FOLIAGE_ObjType_Rose; // + (MyRandomLong()&0x3);
		gNewObjectDefinition.coord.x	= RandomFloat2() * 1100.0f;
		gNewObjectDefinition.coord.y 	= -800.0f;
		gNewObjectDefinition.coord.z 	= -300.0f - RandomFloat() * 300.0f;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= 5;
		gNewObjectDefinition.moveCall 	= MoveMenuFlower;
		gNewObjectDefinition.rot 		= RandomFloat()*PI2;
		gNewObjectDefinition.scale 		= 2.0 + RandomFloat() * 2.0f;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->WaveXIndex = newObj->Coord.x * .004f;
		newObj->WaveZIndex = newObj->Coord.z * .004f;
	}

			/* CYC */

	gNewObjectDefinition.group		= MODEL_GROUP_LOSESCREEN;
	gNewObjectDefinition.type 		= LOSE_ObjType_Cyc;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot 		= CYC_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= gGameView.yon * .99f / 100.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawCyclorama;


		/*************************/
		/* MAKE BUMBLE BEE & BAG */
		/*************************/

			/* MAKE BEE */

	gNewObjectDefinition.type 		= SKELETON_TYPE_BUMBLEBEE;
	gNewObjectDefinition.animNum 	= 0;
	gNewObjectDefinition.scale 		= 3.0;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 200;
	gNewObjectDefinition.coord.z 	= 900;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveLoseBee;
	gNewObjectDefinition.rot 		= 0;
	bee = MakeNewSkeletonObject(&gNewObjectDefinition);

	bee->Delta.y = 100.0f;


			/* MAKE BAG */

	gNewObjectDefinition.type 		= SKELETON_TYPE_HOBOBAG;
	gNewObjectDefinition.animNum 	= 0;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	bag = MakeNewSkeletonObject(&gNewObjectDefinition);

	bee->ChainNode = bag;


			/* MAKE TEXT SPRITE */

	gNewObjectDefinition.group 		= SPRITE_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= LOSE_SObjType_GameOver;
	gNewObjectDefinition.coord.x 	= (640/2);
	gNewObjectDefinition.coord.y 	= 150;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	gNewObjectDefinition.moveCall 	= MoveGameOverText;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 500;
	newObj = MakeSpriteObject(&gNewObjectDefinition);

	newObj->Timer = 3.0f;
	newObj->ColorFilter.a = 0;

	PlaySong(EFFECT_SONG_LOSE, false);

}



/********************** FREE LOSE SCREEN **********************/

static void FreeLoseScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeEffects();
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);
	DisposeAllBG3DContainers();
}



/**************** PROCESS MAIN MENU ********************/

static void ProcessLose(void)
{
float	timer = 25.0f;

	CalcFramesPerSecond();
	UpdateInput();

	while(!UserWantsOut())
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();
		OGL_MoveCameraFromTo(0, 0, -35.0f * fps, 0, 0, -35.0f * fps);


				/* DRAW */

		OGL_DrawScene(DrawObjects);


		timer -= fps;
		if (timer <= 0.0f)
			break;
	}
}


#pragma mark -

/********************* MOVE WAVING FLOWER **********************/

static void MoveMenuFlower(ObjNode *theNode)
{
	theNode->WaveXIndex += gFramesPerSecondFrac * 1.5f;
	theNode->WaveZIndex += gFramesPerSecondFrac * 1.5f;

	theNode->Rot.x = sin(theNode->WaveXIndex) * .04f;
	theNode->Rot.z = sin(theNode->WaveZIndex) * .04f;

	UpdateObjectTransforms(theNode);
}


/********************* MOVE LOSE BEE ****************************/

static void MoveLoseBee(ObjNode *bee)
{
OGLMatrix4x4	m,rm;
ObjNode			*bag = bee->ChainNode;
float			fps = gFramesPerSecondFrac;
float			r;

	GetObjectInfo(bee);


	r = bee->Rot.y;
	gCoord.x += -sin(r) * (700.0f * fps);
	gCoord.z += -cos(r) * (700.0f * fps);

	gDelta.y -= 50.0f * fps;
	gCoord.y += gDelta.y * fps;

	if (gCoord.z < -800.0f)
	{
		bee->Rot.y += fps * .2f;
	}


	UpdateObject(bee);

				/* UPDATE EFFECT */

	if (bee->EffectChannel == -1)
		bee->EffectChannel = PlayEffect_Parms3D(EFFECT_BUMBLERUMBLE, &bee->Coord, NORMAL_CHANNEL_RATE, 1.0);
	else
		Update3DSoundChannel(EFFECT_BUMBLERUMBLE, &bee->EffectChannel, &bee->Coord);


			/* UPDATE HOBO BAG */

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


/********************* MOVE GAME OVER TEXT ***********************/

static void MoveGameOverText(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Timer -= fps;
	if (theNode->Timer <= 0.0f)
	{
		theNode->ColorFilter.a += fps;
		if (theNode->ColorFilter.a > 1.0f)
			theNode->ColorFilter.a = 1.0f;
	}



}








