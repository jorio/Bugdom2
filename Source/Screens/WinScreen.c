/****************************/
/*   	WIN SCREEN.C		*/
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

static void SetupWinScreen(void);
static void FreeWinScreen(void);
static void DrawWinCallback(void);
static void ProcessWin(void);
static void MakeWinConfetti(ObjNode *player);
static void MoveWinText(ObjNode *theNode);
static void MoveWinSkip(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	WIN_ObjType_Cyc,
	WIN_ObjType_Hive
};



enum
{
	WIN_SObjType_YouWin
};


#define	JOINT_NUM_RIGHTWRIST	25



/*********************/
/*    VARIABLES      */
/*********************/

#define	PickupNow 	Flag[0]

static	ObjNode *gHive, *gBag;

/********************** DO WIN SCREEN **************************/

void DoWinScreen(void)
{

	GammaFadeOut();

			/* SETUP */

	SetupWinScreen();
	MakeFadeEvent(true, 1);

	ProcessWin();


			/* CLEANUP */

	GammaFadeOut();
	FreeWinScreen();
}



/********************* SETUP WIN SCREEN **********************/

static void SetupWinScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
const static OGLVector3D	fillDirection1 = { -.1, -.1, 1.0 };
const static OGLVector3D	fillDirection2 = {0, -.4, -1.0 };
ObjNode	*newObj;

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

	viewDef.camera.fov 			= 1.1;
	viewDef.camera.hither 		= 4;
	viewDef.camera.yon 			= 2000;

	viewDef.styles.useFog			= false;
	viewDef.view.clearBackBuffer	= false;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;

	viewDef.camera.from.x		= -200;
	viewDef.camera.from.y		= 130;
	viewDef.camera.from.z		= 250;

	viewDef.camera.to.y 		= 100.0f;

	viewDef.lights.ambientColor.r = .2;
	viewDef.lights.ambientColor.g = .2;
	viewDef.lights.ambientColor.b = .2;

	viewDef.lights.numFillLights 	= 2;

	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillColor[0].r 	= .9;
	viewDef.lights.fillColor[0].g 	= .9;
	viewDef.lights.fillColor[0].b 	= .9;

	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[1].r 	= 1.0;
	viewDef.lights.fillColor[1].g 	= 1.0;
	viewDef.lights.fillColor[1].b 	= .7;

	OGL_SetupWindow(&viewDef, &gGameView);


				/************/
				/* LOAD ART */
				/************/

	InitSparkles();
	InitEffects();

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:winscreen.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_WINSCREEN);


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:WinScreen.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_WIN);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:global.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL);


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_SKIP_TITLE);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SKIP_TITLE, 0,				// set sphere map on geometry texture
									1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

	LoadASkeleton(SKELETON_TYPE_HOBOBAG);


			/*******************/
			/* MAKE BACKGROUND */
			/*******************/

			/* CYC */

	gNewObjectDefinition.group		= MODEL_GROUP_WINSCREEN;
	gNewObjectDefinition.type 		= WIN_ObjType_Cyc;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot 		= TERRAIN_SLOT+1;					// draw after terrain for better performance since terrain blocks much of the pixels
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= gGameView->yon * .99f / 100.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawCyclorama;


			/* HIVE */

	gNewObjectDefinition.group		= MODEL_GROUP_WINSCREEN;
	gNewObjectDefinition.type 		= WIN_ObjType_Hive;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.5;
	gHive = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* MAKE SKIP */

	gNewObjectDefinition.type 		= SKELETON_TYPE_SKIP_TITLE;
	gNewObjectDefinition.animNum 	= PLAYER_TITLE_ANIM_LOOK;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 80;
	gNewObjectDefinition.coord.z 	= -330;
	gNewObjectDefinition.slot		= 600;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.moveCall 	= MoveWinSkip;
	gNewObjectDefinition.rot 		= PI;
	gNewObjectDefinition.scale 		= 1.7;
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	newObj->PickupNow = false;


			/* MAKE BAG */

	gNewObjectDefinition.type 		= SKELETON_TYPE_HOBOBAG;
	gNewObjectDefinition.animNum 	= 3;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 2.0;
	gNewObjectDefinition.scale 		= 1.7;
	gBag = MakeNewSkeletonObject(&gNewObjectDefinition);


			/* MAKE BUGDOM TEXT SPRITE */

	gNewObjectDefinition.group 		= SPRITE_GROUP_WIN;
	gNewObjectDefinition.type 		= WIN_SObjType_YouWin;
	gNewObjectDefinition.coord.x 	= (640/2);
	gNewObjectDefinition.coord.y 	= 100;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	gNewObjectDefinition.moveCall 	= MoveWinText;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 500;
	newObj = MakeSpriteObject(&gNewObjectDefinition);

	newObj->Timer = 7.0f;
	newObj->ColorFilter.a = 0;

	PlaySong(SONG_WIN, false);

}



/********************** FREE WIN SCREEN **********************/

static void FreeWinScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	OGL_DisposeWindowSetup(&gGameView);
}



/**************** PROCESS WIN SCREEN ********************/

static void ProcessWin(void)
{
float	timer = 0.0f;

	CalcFramesPerSecond();
	UpdateInput();

	while(true)
	{
		OGLMatrix4x4	m;
		OGLPoint3D		p;
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();

				/* SPIN CAMERA */

		OGLMatrix4x4_SetRotateAboutPoint(&m, &gHive->Coord, 0, .1f * gFramesPerSecondFrac, 0);
		OGLPoint3D_Transform(&gGameView->cameraPlacement.cameraLocation, &m, &p);
		OGL_UpdateCameraFromTo(&p, nil);


				/* DRAW */

		OGL_DrawScene(DrawWinCallback);


		timer += fps;
		if (timer >= 9.0f)
		{
			if (UserWantsOut())
				break;
		}
		if (timer >= 30.0f)
			break;
	}

}


/***************** DRAW WIN CALLBACK *******************/

static void DrawWinCallback(void)
{
	DrawObjects();
	DrawSparkles();											// draw light sparkles

}


#pragma mark -


/********************** MOVE WIN SKIP ***********************/

static void MoveWinSkip(ObjNode *theNode)
{
ObjNode	*bag;
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	switch(theNode->Skeleton->AnimNum)
	{
		case	PLAYER_TITLE_ANIM_LOOK:
				if (theNode->Skeleton->AnimHasStopped)
					MorphToSkeletonAnim(theNode->Skeleton, PLAYER_TITLE_ANIM_WINWALK, 7);
				break;

		case	PLAYER_TITLE_ANIM_WINWALK:
				theNode->Skeleton->AnimSpeed = 2.0f;
				gCoord.z += 200.0f * fps;
				if (gCoord.z >= -20.0f)
				{
					gCoord.z = -20.0f;
					MorphToSkeletonAnim(theNode->Skeleton, PLAYER_TITLE_ANIM_PICKUP, 5);
				}
				break;

		default:

				/* SEE IF GET HOBO BAG NOW */

				if (theNode->PickupNow)
				{
					theNode->PickupNow = false;

					SetSkeletonAnim(gBag->Skeleton, 0);			// set bad anim to hold
					theNode->ChainNode = gBag;
					gBag->MoveCall = nil;
				}

						/* UPDATE HOBO BAG */

				bag = theNode->ChainNode;
				if (bag)
				{
					OGLMatrix4x4	rm,m;

					FindJointFullMatrix(theNode, JOINT_NUM_RIGHTWRIST, &m);
					OGLMatrix4x4_SetRotate_XYZ(&rm, 4.0f, -1.0, 0);					// rotate to align bag
					rm.value[M03] = 0;												// also offset to align to hand
					rm.value[M13] = -5;
					rm.value[M23] = 0;

					OGLMatrix4x4_Multiply(&rm, &m, &bag->BaseTransformMatrix);		// calc bag's matrix
					SetObjectTransformMatrix(bag);


					bag->Coord.x = bag->BaseTransformMatrix.value[M03];					// extract coords of bag
					bag->Coord.y = bag->BaseTransformMatrix.value[M13];
					bag->Coord.z = bag->BaseTransformMatrix.value[M23];

					theNode->Rot.y -= fps * 1.6f;
				}


				MakeWinConfetti(theNode);
	}


	UpdateObject(theNode);
}


/*********************** MAKE WIN CONFETTI ****************************/

static void MakeWinConfetti(ObjNode *player)
{
long					i;
OGLVector3D				delta = {0,0,0};
OGLPoint3D				pt;
NewConfettiDefType		newConfettiDef;
int						particleGroup,magicNum;

	player->Timer -= gFramesPerSecondFrac;
	if (player->Timer <= 0.0f)
		player->Timer += 0.04;
	else
		return;

	particleGroup 	= player->ParticleGroup;
	magicNum 		= player->ParticleMagicNum;

	if ((particleGroup == -1) || (!VerifyConfettiGroupMagicNum(particleGroup, magicNum)))
	{

		player->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

		gNewConfettiGroupDef.magicNum				= magicNum;
		gNewConfettiGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
		gNewConfettiGroupDef.gravity				= 100;
		gNewConfettiGroupDef.baseScale				= 3;
		gNewConfettiGroupDef.decayRate				= 0;
		gNewConfettiGroupDef.fadeRate				= 2.0;
		gNewConfettiGroupDef.confettiTextureNum		= PARTICLE_SObjType_PurpleDiasyConfetti;

		particleGroup = player->ParticleGroup = NewConfettiGroup(&gNewConfettiGroupDef);
	}

	if (particleGroup != -1)
	{
		for (i = 0; i < 4; i++)
		{
			pt.x = RandomFloat2() * 230.0f;
			pt.y = 400.0f + RandomFloat() * 200.0f;
			pt.z = RandomFloat2() * 230.0f;

			newConfettiDef.groupNum		= particleGroup;
			newConfettiDef.where		= &pt;
			newConfettiDef.delta		= &delta;
			newConfettiDef.scale		= 1.0f + RandomFloat()  * .5f;
			newConfettiDef.rot.x		= RandomFloat()*PI2;
			newConfettiDef.rot.y		= RandomFloat()*PI2;
			newConfettiDef.rot.z		= RandomFloat()*PI2;
			newConfettiDef.deltaRot.x	= RandomFloat2()*5.0f;
			newConfettiDef.deltaRot.y	= RandomFloat2()*5.0f;
			newConfettiDef.deltaRot.z	= RandomFloat2()*15.0f;
			newConfettiDef.alpha		= FULL_ALPHA;
			newConfettiDef.fadeDelay	= 4.5f + RandomFloat() * 2.0f;
			if (AddConfettiToGroup(&newConfettiDef))
			{
				player->ParticleGroup = -1;
				break;
			}
		}
	}
}




/********************* MOVE WIN TEXT ***********************/

static void MoveWinText(ObjNode *theNode)
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











