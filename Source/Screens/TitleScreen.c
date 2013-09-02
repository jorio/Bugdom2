/****************************/
/*   	TITLE SCREEN.C		*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"
#include "terrain.h"
#include "infobar.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gGlobalTransparency;
extern	FSSpec		gDataSpec;
extern	Boolean		gGameOver;
extern	KeyMap gKeyMap,gNewKeys;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,gDisableAnimSounds;
extern	PrefsType	gGamePrefs;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	PlayerInfoType	gPlayerInfo;
extern	int				gLevelNum, gSuperTileActiveRange;
extern	SparkleType	gSparkles[MAX_SPARKLES];
extern	OGLPoint3D	gCoord;
extern	OGLVector3D	gDelta;
extern	u_long			gScore,gGlobalMaterialFlags;
extern	float			 gTerrainPolygonSize;
extern	OGLBoundingBox			gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	u_long				gAutoFadeStatusBits;
extern	float		gAutoFadeStartDist,gAutoFadeEndDist,gAutoFadeRange_Frac;
extern	ChannelInfoType				gChannelInfo[];

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupTitleScreen(void);
static void FreeTitleScreen(void);
static void DrawTitleCallback(OGLSetupOutputType *info);
static void DoBugdomEntrance(void);
static void MoveTitleFlies(ObjNode *fly);
static void DoSwatter(void);
static void DoFadeToScene(void);
static void MoveTitleSkip(ObjNode *skip);
static void MakeTitleSkip(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	SAVE_TEXT_SIZE		30.0f


enum
{
	TITLE_ObjType_Cyc
};

enum
{
	TITLE_SObjType_BugdomText,
	TITLE_SObjType_2Text,

	TITLE_SObjType_Fly,
	TITLE_SObjType_Back,
	TITLE_SObjType_Swatter
};

#define	 BUGDOM_TEXT_SCALE	500.0f
#define	BUGDOM_SHADOW_YOFF	5.0f

#define	FLY_SCALE	20.0f

#define	WaveXIndex	SpecialF[0]
#define	WaveZIndex	SpecialF[1]



#define	JOINT_NUM_RIGHTWRIST	25

/*********************/
/*    VARIABLES      */
/*********************/

static Boolean	gAbortTitle, gScatterFlies, gDeleteFlies;

static ObjNode *gBack, *gText, *g2;

/********************** DO TITLE SCREEN **************************/

void DoTitleScreen(void)
{

	GammaFadeOut();

			/* SETUP */

	SetupTitleScreen();
	MakeFadeEvent(true, 1);

	gAbortTitle = false;

	DoBugdomEntrance();
	if (gAbortTitle)
		goto bail;

	DoSwatter();
	if (gAbortTitle)
		goto bail;

	DoFadeToScene();
	if (gAbortTitle)
		goto bail;


			/* CLEANUP */

bail:
	GammaFadeOut();
	FreeTitleScreen();
}



/********************* SETUP TITLE SCREEN **********************/

static void SetupTitleScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
const static OGLVector3D	fillDirection1 = { -.7, -.5, -1.0 };
int				i;
ObjNode			*fly;

	gLevelNum = -1;

	SetTerrainScale(100);
	gSuperTileActiveRange = 5;

	gScatterFlies = false;
	gDeleteFlies = false;

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= .8;

	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= (gSuperTileActiveRange * SUPERTILE_SIZE * gTerrainPolygonSize) * .95f;

	viewDef.styles.useFog			= true;
	viewDef.styles.fogStart			= viewDef.camera.yon * .3f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * 1.2f;
	viewDef.view.clearColor.r 		= .8;
	viewDef.view.clearColor.g 		= .8;
	viewDef.view.clearColor.b		= .7;

	viewDef.view.clearBackBuffer	= false;

	viewDef.lights.ambientColor.r = .2;
	viewDef.lights.ambientColor.g = .2;
	viewDef.lights.ambientColor.b = .15;
	viewDef.lights.fillDirection[0] = fillDirection1;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);

	gAutoFadeStatusBits = STATUS_BIT_AUTOFADE;
	gAutoFadeStartDist	= gGameViewInfoPtr->yon * .80;
	gAutoFadeEndDist	= gGameViewInfoPtr->yon * .95f;
	gAutoFadeRange_Frac	= 1.0f / (gAutoFadeEndDist - gAutoFadeStartDist);


				/************/
				/* LOAD ART */
				/************/

				/* LOAD AUDIO */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:audio:Title.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_TITLE);

	InitSparkles();

			/* LOAD TERRAIN */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Terrain:Title.ter", &spec);
	LoadPlayfield(&spec, gGameViewInfoPtr);



			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Models:title.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_TITLE, gGameViewInfoPtr);

	LoadFoliage(gGameViewInfoPtr);


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:particle.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_PARTICLES, gGameViewInfoPtr);
	BlendAllSpritesInGroup(SPRITE_GROUP_PARTICLES);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:global.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:Title.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_TITLE, gGameViewInfoPtr);


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_SKIP_TITLE, gGameViewInfoPtr);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SKIP_TITLE, 0,				// set sphere map on geometry texture
									1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkYosemite);

	LoadASkeleton(SKELETON_TYPE_BUMBLEBEE, gGameViewInfoPtr);
	LoadASkeleton(SKELETON_TYPE_HOBOBAG, gGameViewInfoPtr);




			/*******************/
			/* MAKE BACKGROUND */
			/*******************/


			/* MAKE BACKGROUND */

	gNewObjectDefinition.group 		= SPRITE_GROUP_TITLE;
	gNewObjectDefinition.type 		= TITLE_SObjType_Back;
	gNewObjectDefinition.coord.x 	= 640/2;
	gNewObjectDefinition.coord.y 	= 480/2;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SPRITE_SLOT - 10;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1;
	gBack = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);

	gBack->Scale.x = 640;
	gBack->Scale.y = 640;


			/* MAKE FLIES */

	for (i = 0; i < 20; i++)
	{
		u_long	volL,volR;

		gNewObjectDefinition.group 		= SPRITE_GROUP_TITLE;
		gNewObjectDefinition.type 		= TITLE_SObjType_Fly;
		gNewObjectDefinition.coord.x 	= RandomFloat() * 640.0f;
		gNewObjectDefinition.coord.y 	= RandomFloat() * 480.0f;
		gNewObjectDefinition.coord.z 	= 0;
		gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot 		= SPRITE_SLOT + 2;
		gNewObjectDefinition.moveCall 	= MoveTitleFlies;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 	    = 1;
		fly = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);

		fly->Scale.x = FLY_SCALE;
		fly->Scale.y = fly->Scale.x;

		fly->Rot.y = RandomFloat() * PI2;

		fly->Timer = RandomFloat() * 4.0f;

		fly->TargetOff.x = RandomFloat() * 640.0f;
		fly->TargetOff.y = RandomFloat() * 480.0f;

		if (i & 1)			// do every other
		{
			volL = RandomRange(FULL_CHANNEL_VOLUME/20, FULL_CHANNEL_VOLUME/3);
			volR = (FULL_CHANNEL_VOLUME/3) - volL;
			fly->EffectChannel = PlayEffect_Parms(EFFECT_TITLEFLYBUZZ, volL, volR, NORMAL_CHANNEL_RATE + (MyRandomLong()&0x7fff));
		}
	}


	PlaySong(SONG_INTRO, true);


}

/****************** MOVE TITLE FLIES ***********************/

static void MoveTitleFlies(ObjNode *fly)
{
float	fps = gFramesPerSecondFrac;
float	r,speed;

	if (gDeleteFlies)
	{
		DeleteObject(fly);
		return;
	}

			/* TURN TOWARD TARGET */

	fly->Timer -= fps;								// see if pick new target
	if (fly->Timer <= 0.0f)
	{
		fly->Timer = 1.0f + RandomFloat() * 2.0f;

		fly->TargetOff.x = RandomFloat() * 640.0f;
		fly->TargetOff.y = RandomFloat() * 480.0f;
	}

	if (!gScatterFlies)
	{
		TurnObjectTowardTarget2D(fly, fly->Coord.x, fly->Coord.y, fly->TargetOff.x, fly->TargetOff.y, 5.0);
		speed = 450.0f;
	}
	else
	{
		if (fly->EffectChannel != -1)
		{
			u_long volL, volR;

			volL = gChannelInfo[fly->EffectChannel].leftVolume;							// get current left volume
			volL -= fps;																// dim
			if (volL < 0.0f)
				volL = 0.0f;

			volR = gChannelInfo[fly->EffectChannel].rightVolume;						// get current right volume
			volR -= fps;																// dim
			if (volR < 0.0f)
				volR = 0.0f;

			ChangeChannelVolume(fly->EffectChannel, volL, volR);						// lower volume of buzz
		}

		speed = 800.0f;

	}



			/* MOVE FORWARD */

	r = fly->Rot.y;
	fly->Coord.x += sin(r) * (speed * fps);
	fly->Coord.y += -cos(r) * (speed * fps);


}


/********************** FREE TITLE SCREEN **********************/

static void FreeTitleScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUND_BANK_TITLE);
	DisposeTerrain();

	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}

#pragma mark -


/**************** DO BUGDOM ENTRANCE ********************/
//
// Make the "Bugdom" part of the logo fly into place
//

static void DoBugdomEntrance(void)
{
float	timer;

			/* MAKE BUGDOM TEXT SPRITE */

	gNewObjectDefinition.group 		= SPRITE_GROUP_TITLE;
	gNewObjectDefinition.type 		= TITLE_SObjType_BugdomText;
	gNewObjectDefinition.coord.x 	= (640/2) - 30;
	gNewObjectDefinition.coord.y 	= -200;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1;
	gText = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);

	gText->Scale.x = BUGDOM_TEXT_SCALE;
	gText->Scale.y = BUGDOM_TEXT_SCALE;


			/************************/
			/* MOVE LOGO INTO PLACE */
			/************************/

	CalcFramesPerSecond();
	UpdateInput();

	timer = 3.0f;
	while(timer > 0.0f)
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();
		if (AreAnyNewKeysPressed())
		{
			gAbortTitle = true;
			break;
		}

				/* MOVE */

		MoveObjects();

		gText->Delta.y += 1600.0f * fps;			// gravity
		gText->Coord.y += gText->Delta.y * fps; 	// move down
		if (gText->Coord.y >= 200.0f)				// see if bounce
		{
			gText->Coord.y = 200.0f;
			gText->Delta.y *= -.5f;
			if (fabs(gText->Delta.y) > 50.0f)
				PlayEffect(EFFECT_LOGOBOUNCE);
		}

				/* DRAW */

		OGL_DrawScene(gGameViewInfoPtr, DrawTitleCallback);

		timer -= fps;
	}


}


/**************** DO SWATTER ********************/

static void DoSwatter(void)
{
ObjNode	*swatter;
float	timer;
Byte	swatMode = 0;

			/* MAKE BUGDOM TEXT SPRITE */

	gNewObjectDefinition.group 		= SPRITE_GROUP_TITLE;
	gNewObjectDefinition.type 		= TITLE_SObjType_Swatter;
	gNewObjectDefinition.coord.x 	= 260;
	gNewObjectDefinition.coord.y 	= 350;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1;
	swatter = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);

	swatter->ColorFilter.a = 0;
	swatter->Scale.x =
	swatter->Scale.y = 600;

	swatter->Rot.y = PI/5;

			/************************/
			/* MOVE LOGO INTO PLACE */
			/************************/

	PlayEffect(EFFECT_FLYSWATTER);

	CalcFramesPerSecond();
	UpdateInput();

	timer = 3.0f;
	while(timer > 0.0f)
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();
		if (AreAnyNewKeysPressed())
		{
			gAbortTitle = true;
			break;
		}

				/* MOVE */

		MoveObjects();

		switch(swatMode)
		{
			case	0:															// SWATTING
					swatter->ColorFilter.a += fps * 4.0f;						// fade in
					if (swatter->ColorFilter.a > 1.0f)
						swatter->ColorFilter.a = 1.0f;

					swatter->Scale.x = swatter->Scale.y -= fps * 1300.0f;		// scale down
					if (swatter->Scale.x < 250.0f)
					{
						swatter->Scale.x = swatter->Scale.y = 250.0f;
						swatMode = 1;
						gScatterFlies = true;
						swatter->Timer = 1.0f;

								/* MAKE THE 2 */

						gNewObjectDefinition.group 		= SPRITE_GROUP_TITLE;
						gNewObjectDefinition.type 		= TITLE_SObjType_2Text;
						gNewObjectDefinition.coord.x 	= 320;
						gNewObjectDefinition.coord.y 	= 220;
						gNewObjectDefinition.coord.z 	= 0;
						gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
						gNewObjectDefinition.slot 		= SPRITE_SLOT - 3;
						gNewObjectDefinition.moveCall 	= nil;
						gNewObjectDefinition.rot 		= 0;
						gNewObjectDefinition.scale 	    = 1;
						g2 = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);
						g2->Scale.x =
						g2->Scale.y = BUGDOM_TEXT_SCALE*.6;

					}
					break;

			case	1:															// WAIT
					swatter->Timer -= fps;
					if (swatter->Timer <= 0.0f)
					{
						swatMode = 2;
						swatter->Speed2D = 0;

					}
					break;

			case	2:								// move swatter up
					swatter->Speed2D += 3000.0f * fps;

					swatter->Coord.y += swatter->Speed2D * fps;
					swatter->Coord.x -= swatter->Speed2D * fps;
					swatter->Scale.x = swatter->Scale.y += fps * swatter->Speed2D;
					if (swatter->Scale.x > 500.0f)
					{
						swatter->ColorFilter.a -= fps * 4.0f;

						if (swatter->ColorFilter.a <= 0.0f)
						{
							DeleteObject(swatter);
							swatter = 0;
							swatMode = 3;
						}
					}
					break;
		}

				/* DRAW */

		OGL_DrawScene(gGameViewInfoPtr, DrawTitleCallback);

		timer -= fps;
	}


}



/**************** DO FADE TO SCENE ********************/

static void DoFadeToScene(void)
{
float	timer;
ObjNode	*newObj;

	gDeleteFlies = true;

		/* INIT THE TERRAIN */

	InitCurrentScrollSettings();								// this also creates the CUSTOM DRAW object for drawing the terrain
	DoPlayerTerrainUpdate(gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z);

	PrimeTerrainWater();
	PrimeSplines();
	PrimeFences();


		/* MAKE CYC */

	gNewObjectDefinition.group	= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 	= 0;								// cyc is always 1st model in level bg3d files
	gNewObjectDefinition.coord.x = 0;
	gNewObjectDefinition.coord.y = 0;
	gNewObjectDefinition.coord.z = 0;
	gNewObjectDefinition.flags 	= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 	= TERRAIN_SLOT+1;					// draw after terrain for better performance since terrain blocks much of the pixels
	gNewObjectDefinition.moveCall = nil;
	gNewObjectDefinition.rot 	= 0;
	gNewObjectDefinition.scale 	= gGameViewInfoPtr->yon * .995f / 100.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawCyclorama;

			/* MAKE SKIP */

	MakeTitleSkip();


			/************************/
			/* MOVE LOGO INTO PLACE */
			/************************/

	CalcFramesPerSecond();
	UpdateInput();

	timer = 22.0f;
	while(timer > 0.0f)
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();
		if (AreAnyNewKeysPressed())
		{
			gAbortTitle = true;
			break;
		}

				/* MOVE */

		MoveObjects();
		MoveSplineObjects();
		DoPlayerTerrainUpdate(gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z);


				/* FADE OUT BACK */

		if (gBack)
		{
			gBack->ColorFilter.a -= fps * 1.0f;
			if (gBack->ColorFilter.a <= 0.0f)
			{
				PlayEffect(EFFECT_LOGOVANISH);
				DeleteObject(gBack);
				gBack = nil;
			}
		}

				/* MOVE LOGO */
		else
		{
			gText->Delta.y -= fps * 400.0f;
			gText->Coord.y += gText->Delta.y * fps;

			g2->Coord.y -= gText->Delta.y * fps;
		}



				/* DRAW */

		OGL_DrawScene(gGameViewInfoPtr, DrawTitleCallback);

		timer -= fps;
	}

}


#pragma mark -


/****************** MAKE TITLE SKIP ***********************/

static void MakeTitleSkip(void)
{
ObjNode	 *skip, *bag;

				/* MAKE SKIP */

	gNewObjectDefinition.type 		= SKELETON_TYPE_SKIP_TITLE;
	gNewObjectDefinition.animNum	= PLAYER_TITLE_ANIM_WALK;
	gNewObjectDefinition.scale 		= PLAYER_DEFAULT_SCALE;
	gNewObjectDefinition.coord.x 	= gPlayerInfo.coord.x;
	gNewObjectDefinition.coord.z 	= gPlayerInfo.coord.z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(gPlayerInfo.coord.x, gPlayerInfo.coord.z) + 62.0f;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_DONTCULL|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= PLAYER_SLOT;
	gNewObjectDefinition.moveCall	= MoveTitleSkip;
	gNewObjectDefinition.rot 		= -PI/2;

	skip = MakeNewSkeletonObject(&gNewObjectDefinition);
	skip->Skeleton->AnimSpeed = 1.0f;

	gPlayerInfo.objNode = skip;

	AttachShadowToObject(skip, 0, 3,3, false);


				/* MAKE HOBO BAG */

	gNewObjectDefinition.type 		= SKELETON_TYPE_HOBOBAG;
	gNewObjectDefinition.animNum 	= 0;
	gNewObjectDefinition.scale 		= 1.3;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	bag = MakeNewSkeletonObject(&gNewObjectDefinition);


	skip->ChainNode = bag;

}


/********************* MOVE TITLE SKIP ******************************/

static void MoveTitleSkip(ObjNode *skip)
{
float			fps = gFramesPerSecondFrac;
OGLPoint3D		from, to;
ObjNode			*bag;
OGLMatrix4x4	m,rm;

	GetObjectInfo(skip);


			/* MOVE */

	switch(skip->Skeleton->AnimNum)
	{
		case	PLAYER_TITLE_ANIM_WALK:
				gCoord.x += 100.0f * fps;
				break;

		case	PLAYER_TITLE_ANIM_ROBBED:
				skip->Timer -= fps;
				if (skip->Timer <= 0.0f)
					SetSkeletonAnim(skip->Skeleton, PLAYER_TITLE_ANIM_GETUP);
				break;

		case	PLAYER_TITLE_ANIM_GETUP:
				if (skip->Skeleton->AnimHasStopped)
				{
					MorphToSkeletonAnim(skip->Skeleton, PLAYER_TITLE_ANIM_GETMAD, 5);
					skip->Timer = 6.0f;
				}
				break;

		case	PLAYER_TITLE_ANIM_GETMAD:
				skip->Timer -= fps;
				if (skip->Timer <= 0.0f)
				{
					MorphToSkeletonAnim(skip->Skeleton, PLAYER_TITLE_ANIM_THINK, 4);
				}
				break;

		case	PLAYER_TITLE_ANIM_THINK:
				if (skip->Skeleton->AnimHasStopped)
				{
					SetSkeletonAnim(skip->Skeleton, PLAYER_TITLE_ANIM_JUMP);
					skip->Flag[0] = false;
					skip->Flag[1] = false;
				}
				break;

		case	PLAYER_TITLE_ANIM_JUMP:
				if (skip->Flag[0])								// see if jump now
				{
					skip->Flag[0] = false;
					skip->Flag[1] = true;
					gDelta.x = 700.0f;
					gDelta.y = 1100.0f;
					PlayEffect(EFFECT_JUMP);
				}
				else
				if (skip->Flag[1])								// see if already leapt
				{
					gDelta.y -= 4000.0f * fps;
					gCoord.x += gDelta.x * fps;
					gCoord.y += gDelta.y * fps;

				}
				break;
	}

			/* UPDATE SKIP */

	UpdateObject(skip);
	gPlayerInfo.coord = gCoord;


			/* UPDATE HOBO BAG */

	bag = skip->ChainNode;
	if (bag)
	{
		FindJointFullMatrix(skip, JOINT_NUM_RIGHTWRIST, &m);
		OGLMatrix4x4_SetRotate_X(&rm, 4.0f);							// rotate to align bag
		rm.value[M03] = 0;												// also offset to align to hand
		rm.value[M13] = -5;
		rm.value[M23] = 0;

		OGLMatrix4x4_Multiply(&rm, &m, &bag->BaseTransformMatrix);		// calc bag's matrix
		SetObjectTransformMatrix(bag);


		bag->Coord.x = bag->BaseTransformMatrix.value[M03];					// extract coords of bag
		bag->Coord.y = bag->BaseTransformMatrix.value[M13];
		bag->Coord.z = bag->BaseTransformMatrix.value[M23];
	}


			/* UPDATE CAMERA */

	if (skip->Skeleton->AnimNum != PLAYER_TITLE_ANIM_JUMP)				// camera doesnt follow @ the end
	{
		from = skip->Coord;
		from.x += 350.0f;
		from.z += 170.0f;
		from.y += 120.0f;

		to = skip->Coord;
		to.y += 70.0f;
		OGL_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);
	}

}


/******************** MOVE BUMBLEBEE ON SPLINE: TITLE ***************************/

void MoveBumbleBeeOnSpline_Title(ObjNode *bee)
{
float	fps = gFramesPerSecondFrac;
ObjNode *bag, *player;

	if (bee->Skeleton->AnimNum != 0)
		SetSkeletonAnim(bee->Skeleton, 0);

		/* MOVE ALONG THE SPLINE */

	IsSplineItemOnActiveTerrain(bee);						// update its visibility
	if (IncreaseSplineIndex(bee, 420.0f))					// move and see if reached end of spline
	{
		DeleteObject(bee);
		return;
	}
	GetObjectCoordOnSpline(bee);
	SetSplineAim(bee);										// update aim



	switch(bee->Mode)
	{
				/****************/
				/* CHASE PLAYER */
				/****************/

		case	0:
				bee->Coord.y = GetTerrainY(gCoord.x, gCoord.z) + 90.0f;					// keep above player
				if (OGLPoint3D_Distance(&gPlayerInfo.coord, &bee->Coord) < 130.0f)		// see if close enough to grab bag
				{
					player = gPlayerInfo.objNode;

					bee->Mode = 1;								// bee has bag
					MorphToSkeletonAnim(player->Skeleton, PLAYER_TITLE_ANIM_ROBBED, 8);
					player->Timer = 3.5f;

					bag = player->ChainNode;					// get bag obj
					player->ChainNode = nil;					// take away from player
					bee->ChainNode = bag;						// give bag to bee
					MorphToSkeletonAnim(bag->Skeleton, 2, 4);

					PlayEffect3D(EFFECT_SMACKDOWN, &player->Coord);
				}
				break;


				/************/
				/* FLY AWAY */
				/************/

		case	1:
				bee->Coord.y += 25.0f * fps;
				break;

	}

				/* UPDATE */

	UpdateObjectTransforms(bee);							// update transforms
	UpdateShadow(bee);


			/*******************/
			/* UPDATE HOBO BAG */
			/*******************/

	bag = bee->ChainNode;
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
		bee->EffectChannel = PlayEffect_Parms3D(EFFECT_BUMBLERUMBLE, &bee->Coord, NORMAL_CHANNEL_RATE, 1.0);
	else
		Update3DSoundChannel(EFFECT_BUMBLERUMBLE, &bee->EffectChannel, &bee->Coord);


}


#pragma mark -

/***************** DRAW BONUS CALLBACK *******************/

static void DrawTitleCallback(OGLSetupOutputType *info)
{
	DrawObjects(info);
	DrawSparkles(info);											// draw light sparkles

}









