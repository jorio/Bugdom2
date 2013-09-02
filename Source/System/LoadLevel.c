/****************************/
/*      LOAD LEVEL.C        */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include 	"windows.h"
#include 	"tunnel.h"

extern	short			gCurrentSong;
extern	short			gNumTerrainItems;
extern	short			gPrefsFolderVRefNum;
extern	long			gPrefsFolderDirID;
extern	long			gTerrainTileWidth,gTerrainTileDepth,gTerrainUnitWidth,gTerrainUnitDepth,gNumUniqueSuperTiles;
extern	long			gNumSuperTilesDeep,gNumSuperTilesWide;
extern	FSSpec			gDataSpec;
extern	u_long			gScore,gLoadedScore;
extern	SpriteType		*gSpriteGroupList[];
extern	float			**gMapYCoords,**gMapYCoordsOriginal;
extern	Byte			**gMapSplitMode;
extern	TerrainItemEntryType 	**gMasterItemList;
extern	short			**gSuperTileTextureGrid;
extern	FenceDefType	*gFenceList;
extern	long			gNumFences,gNumSplines,gNumWaterPatches;
extern	int				gLevelNum,gNumTunnelItems,gNumTunnelSplinePoints,gNumTunnelSections;
extern	PrefsType			gGamePrefs;
extern	AGLContext		gAGLContext;
extern	AGLDrawable		gAGLWin;
extern	Boolean			gMuteMusicFlag,gMuteMusicFlag,gLoadedDrawSprocket;
extern	WaterDefType	**gWaterListHandle, *gWaterList;
extern	Boolean			gPlayingFromSavedGame,gG4,gTunnelIsFullPipe,gSlowCPU;
extern	MOMaterialObject	*gTunnelTextureObj;
extern	TunnelItemDefType	*gTunnelItemList;
extern	MOVertexArrayObject	*gTunnelSectionObjects[];
extern	MOVertexArrayObject	*gTunnelSectionWaterObjects[];


/****************************/
/*    PROTOTYPES            */
/****************************/

static void LoadLevelArt_Playroom(OGLSetupOutputType *setupInfo);
static void LoadLevelArt_Garden(OGLSetupOutputType *setupInfo);
static void LoadLevelArt_Sidewalk(OGLSetupOutputType *setupInfo);
static void LoadLevelArt_Fido(OGLSetupOutputType *setupInfo);
static void LoadLevelArt_Closet(OGLSetupOutputType *setupInfo);
static void LoadLevelArt_Balsa(OGLSetupOutputType *setupInfo);
static void LoadLevelArt_Park(OGLSetupOutputType *setupInfo);
static void LoadLevelArt_Garbage(OGLSetupOutputType *setupInfo);


/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/



/************************** LOAD LEVEL ART: EXPLORE ***************************/

void LoadLevelArt_Explore(OGLSetupOutputType *setupInfo)
{
FSSpec	spec;

const Str63	terrainFiles[NUM_LEVELS] =
{
	"\p:Terrain:Level1_Garden.ter",
	"\p:Terrain:Level2_SideWalk.ter",
	"\p:Terrain:Level3_DogHair.ter",
	"\p",
	"\p:Terrain:Level5_PlayRoom.ter",

	"\p:Terrain:Level6_Closet.ter",
	"\p",
	"\p:Terrain:Level8_Garbage.ter",
	"\p:Terrain:Level9_Balsa.ter",
	"\p:Terrain:Level10_Park.ter",
};

 const Str63	levelModelFiles[NUM_LEVELS] =
{
	"\p:Models:Level1_Garden.bg3d",
	"\p:Models:Level2_Sidewalk.bg3d",
	"\p:Models:Level1_Garden.bg3d",
	"\p",
	"\p:Models:Level5_Playroom.bg3d",

	"\p:Models:Level6_Closet.bg3d",
	"\p",
	"\p:Models:Level8_Garbage.bg3d",
	"\p:Models:Level9_Balsa.bg3d",
	"\p:Models:Level10_Park.bg3d"
};

 const Str63	levelSpriteFiles[NUM_LEVELS] =
{
	"\p:Sprites:Level1_Garden.sprites",
	"\p:Sprites:Level2_Sidewalk.sprites",
	"\p:Sprites:Level3_DogHair.sprites",
	"\p",
	"\p:Sprites:Level5_Playroom.sprites",

	"\p:Sprites:Level6_Closet.sprites",
	"\p",
	"\p:Sprites:Level8_Garbage.sprites",
	"\p:Sprites:Level9_Balsa.sprites",
	"\p:Sprites:Level10_Park.sprites",
};


 const Str63	levelSoundFiles[NUM_LEVELS] =
{
	"\p:Audio:Garden.sounds",
	"\p:Audio:Garden.sounds",
	"\p:Audio:Fido.sounds",
	"\p",
	"\p:Audio:Playroom.sounds",

	"\p:Audio:Closet.sounds",
	"\p",
	"\p:Audio:Garbage.sounds",
	"\p:Audio:Balsa.sounds",
	"\p:Audio:Park.sounds",
};


			/*********************/
			/* LOAD COMMNON DATA */
			/*********************/

				/* LOAD AUDIO */

	if (levelSoundFiles[gLevelNum][0] > 0)
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, levelSoundFiles[gLevelNum], &spec);
		LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);
	}


			/* LOAD GLOBAL BG3D GEOMETRY */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL, setupInfo);

	if (!gSlowCPU)									// no reflection mapping when speed is a problem
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_Acorn,
										0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_HealthPOW,
										0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_FlightPOW,
										0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	}

			/* LOAD LEVEL SPECIFIC BG3D GEOMETRY */

	if (levelModelFiles[gLevelNum][0] > 0)
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, levelModelFiles[gLevelNum], &spec);
		ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC, setupInfo);
	}


			/* LOAD SPRITES */

	if (levelSpriteFiles[gLevelNum][0] > 0)
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, levelSpriteFiles[gLevelNum], &spec);
		LoadSpriteFile(&spec, SPRITE_GROUP_LEVELSPECIFIC, setupInfo);
	}

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:infobar.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_INFOBAR, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:global.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS, setupInfo);


			/* LOAD PLAYER SKELETON */

	LoadASkeleton(SKELETON_TYPE_SKIP_EXPLORE, setupInfo);

	if (!gSlowCPU)									// no reflection mapping when speed is a problem
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SKIP_EXPLORE, 0,				// set sphere map on geometry texture
										1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

		SetSphereMapInfoOnMaterialObject(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_SkipBlink].materialObject,	// also set sphere map on blink sprite texture
										MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

		SetSphereMapInfoOnMaterialObject(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_SkipDead].materialObject,	// also set sphere map on death sprite texture
										MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	}

			/* LOAD SNAIL */

	LoadASkeleton(SKELETON_TYPE_SNAIL, setupInfo);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SNAIL, 0,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_SnailShell,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

			/* LOAD CHIPMUNK */

	LoadASkeleton(SKELETON_TYPE_CHIPMUNK, setupInfo);


			/* LOAD BUDDYBUG */

	LoadASkeleton(SKELETON_TYPE_BUDDYBUG, setupInfo);

				/* HOUSEFLY */

	LoadASkeleton(SKELETON_TYPE_HOUSEFLY, setupInfo);


				/* FLEA */

#if !DEMO
	LoadASkeleton(SKELETON_TYPE_FLEA, setupInfo);
#endif

				/* BUMBLE BEE */

	LoadASkeleton(SKELETON_TYPE_BUMBLEBEE, setupInfo);

	LoadASkeleton(SKELETON_TYPE_HOBOBAG, setupInfo);


			/* LOAD CHECKPOINT */

	LoadASkeleton(SKELETON_TYPE_CHECKPOINT, setupInfo);
	if (gG4)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_CHECKPOINT, 0,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);

	}


			/* LOAD MOUSETRAP */

	LoadASkeleton(SKELETON_TYPE_MOUSETRAP, setupInfo);
	LoadASkeleton(SKELETON_TYPE_MOUSE, setupInfo);



			/* LOAD TERRAIN */
			//
			// must do this after creating the view!
			//

	if (terrainFiles[gLevelNum][0] > 0)
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, terrainFiles[gLevelNum], &spec);
		LoadPlayfield(&spec, setupInfo);
	}



			/* MISC REFLECTION MAPS */

	if (gG4)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_BottleCap,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_RoachSpear,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

	}

			/*****************************/
			/* DO LEVEL-SPECIFIC LOADING */
			/*****************************/

	switch(gLevelNum)
	{
		case	LEVEL_NUM_GNOMEGARDEN:
				LoadLevelArt_Garden(setupInfo);
				break;

		case	LEVEL_NUM_SIDEWALK:
				LoadLevelArt_Sidewalk(setupInfo);
				break;

		case	LEVEL_NUM_FIDO:
				LoadLevelArt_Fido(setupInfo);
				break;

		case	LEVEL_NUM_PLAYROOM:
				LoadLevelArt_Playroom(setupInfo);
				break;

		case	LEVEL_NUM_CLOSET:
				LoadLevelArt_Closet(setupInfo);
				break;

		case	LEVEL_NUM_GARBAGE:
				LoadLevelArt_Garbage(setupInfo);
				break;

		case	LEVEL_NUM_BALSA:
				LoadLevelArt_Balsa(setupInfo);
				break;

		case	LEVEL_NUM_PARK:
				LoadLevelArt_Park(setupInfo);
				break;

		default:
				DoFatalAlert("\pSorry, Bud!  That level doesn't exist yet.");
	}
}


/********************** LOAD FOLIAGE **************************/

void LoadFoliage(OGLSetupOutputType *setupInfo)
{
FSSpec	spec;

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Models:Foliage.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_FOLIAGE, setupInfo);

	if (gG4)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_FOLIAGE, FOLIAGE_ObjType_Tulip1,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_FOLIAGE, FOLIAGE_ObjType_Tulip2,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_FOLIAGE, FOLIAGE_ObjType_Tulip3,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_FOLIAGE, FOLIAGE_ObjType_Tulip4,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);

	}

}


/********************* LOAD LEVEL ART: GARDEN **********************/

static void LoadLevelArt_Garden(OGLSetupOutputType *setupInfo)
{

	LoadASkeleton(SKELETON_TYPE_GNOME, setupInfo);
	if (!gSlowCPU)									// no reflection mapping when speed is a problem
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_GNOME, 0,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
	}



									/* EVIL PLANT */

	LoadASkeleton(SKELETON_TYPE_EVILPLANT, setupInfo);

	if (!gSlowCPU)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_EVILPLANT, 0,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
	}





							/* LOAD THE FOLIAGE MODELS */

	LoadFoliage(setupInfo);



				/* MAKE HEAD SHINEY */

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARDEN_ObjType_ScarecrowHead,
									0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);



				/* MAKE ROCKS SHINEY */

	if (gG4)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARDEN_ObjType_LargeStone,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARDEN_ObjType_MediumStone,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARDEN_ObjType_SmallStone,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARDEN_ObjType_SprinklerBase,
										0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARDEN_ObjType_SprinklerPost,
										0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	}
}


/********************* LOAD LEVEL ART: SIDEWALK **********************/

static void LoadLevelArt_Sidewalk(OGLSetupOutputType *setupInfo)
{
int	i;

	LoadASkeleton(SKELETON_TYPE_GNOME, setupInfo);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_GNOME, 0,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);

								/* LOAD SNAKE HEAD */

	LoadASkeleton(SKELETON_TYPE_SNAKEHEAD, setupInfo);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SNAKEHEAD, 0,
									0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);




							/* LOAD THE FOLIAGE MODELS */

	LoadFoliage(setupInfo);


							/* MAKE LOCAL STUFF SHINEY */

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SIDEWALK_ObjType_RideBall,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SIDEWALK_ObjType_SquishBerry,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);


	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SIDEWALK_ObjType_BeachBall,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SIDEWALK_ObjType_ChlorineFloat,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SIDEWALK_ObjType_PoolRingFloat,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

	for (i = 0; i < 5; i++)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SIDEWALK_ObjType_Bottle+i,
										-1, MULTI_TEXTURE_COMBINE_ADDALPHA, SPHEREMAP_SObjType_SheenAlpha);
	}


	if (gG4)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SIDEWALK_ObjType_LargeStone,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SIDEWALK_ObjType_MediumStone,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SIDEWALK_ObjType_SmallStone,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SIDEWALK_ObjType_Grate,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	}

}


/********************* LOAD LEVEL ART: FIDO **********************/

static void LoadLevelArt_Fido(OGLSetupOutputType *setupInfo)
{


	LoadASkeleton(SKELETON_TYPE_TICK, setupInfo);
	if (!gSlowCPU)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_TICK, 0,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	}


}


/********************* LOAD LEVEL ART: GARBAGE **********************/

static void LoadLevelArt_Garbage(OGLSetupOutputType *setupInfo)
{

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARBAGE_ObjType_Can,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARBAGE_ObjType_Tab,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARBAGE_ObjType_Cap,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

	if (gG4)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARBAGE_ObjType_Banana,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARBAGE_ObjType_Onion,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARBAGE_ObjType_Tomato,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARBAGE_ObjType_Zuke,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
	}

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARBAGE_ObjType_Jar,
									0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, GARBAGE_ObjType_Jar,
									1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);



				/* ROACH */

	LoadASkeleton(SKELETON_TYPE_ROACH, setupInfo);


				/* FLEA */

	LoadASkeleton(SKELETON_TYPE_FLEA, setupInfo);


				/* DRAGONFLY */

	LoadASkeleton(SKELETON_TYPE_DRAGONFLY, setupInfo);

}


/********************* LOAD LEVEL ART: PARK **********************/

static void LoadLevelArt_Park(OGLSetupOutputType *setupInfo)
{
int		i;

							/* LOAD THE FOLIAGE MODELS */

	LoadFoliage(setupInfo);


				/* ANT */

	LoadASkeleton(SKELETON_TYPE_ANT, setupInfo);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_ANT, 0,				// set sphere map on geometry texture
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);


				/* FISH */


	LoadASkeleton(SKELETON_TYPE_FISH, setupInfo);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_FISH, 0,				// set sphere map on geometry texture
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);


				/* FROG */

	LoadASkeleton(SKELETON_TYPE_FROG, setupInfo);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_FROG, 0,				// set sphere map on geometry texture
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);


	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PARK_ObjType_Fork,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PARK_ObjType_CheeseBit,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PARK_ObjType_Cherry,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PARK_ObjType_Olive,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);


	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PARK_ObjType_Lure,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

	for (i = 0; i < 5; i++)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PARK_ObjType_Bottle+i,
										-1, MULTI_TEXTURE_COMBINE_ADDALPHA, SPHEREMAP_SObjType_SheenAlpha);
	}

}


/********************* LOAD LEVEL ART: PLAYROOM **********************/

static void LoadLevelArt_Playroom(OGLSetupOutputType *setupInfo)
{

	LoadASkeleton(SKELETON_TYPE_OTTO, setupInfo);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_OTTO,
								 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);



				/* SOLDIER */

	LoadASkeleton(SKELETON_TYPE_TOYSOLDIER, setupInfo);
	if (!gSlowCPU)									// no reflection mapping when speed is a problem
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_TOYSOLDIER, 0,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_Grenade,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

	}

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_SlotCarRed,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_SlotCarYellow,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);



				/* MAKE MARBLE SHINEY */

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_MarbleShell,
									-1, MULTI_TEXTURE_COMBINE_ADDALPHA, SPHEREMAP_SObjType_SheenAlpha);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_Battery,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_DCell,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);


	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_BeachBall,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_LetterBlock1,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_LetterBlock2,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_LetterBlock3,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);


	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_BlockPost,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_LegoWall,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_LegoBrick_Red,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_LegoBrick_Orange,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_LegoBrick_Green,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_LegoBrick_Blue,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_LegoBrick_Yellow,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLAYROOM_ObjType_Crayon,
										1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);


}


/********************* LOAD LEVEL ART: CLOSET **********************/

static void LoadLevelArt_Closet(OGLSetupOutputType *setupInfo)
{
				/* ROACH */

	LoadASkeleton(SKELETON_TYPE_ROACH, setupInfo);


				/* MOTH */

	LoadASkeleton(SKELETON_TYPE_MOTH, setupInfo);


				/* COMPUTER BUG */

	LoadASkeleton(SKELETON_TYPE_COMPUTERBUG, setupInfo);

	if (gG4)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_COMPUTERBUG, 0,				// set sphere map on geometry texture
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);

		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, CLOSET_ObjType_SiliconDoor,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, CLOSET_ObjType_Chip1,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, CLOSET_ObjType_Chip2,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, CLOSET_ObjType_Battery,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, CLOSET_ObjType_Hanger,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);

		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, CLOSET_ObjType_Vacuume,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);

	}

				/* SOLDIER */

	LoadASkeleton(SKELETON_TYPE_TOYSOLDIER, setupInfo);
	if (!gSlowCPU)									// no reflection mapping when speed is a problem
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_TOYSOLDIER, 0,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_Grenade,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	}





	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, CLOSET_ObjType_FlashLight,				// set sphere map on geometry texture
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);



			/* SKIP USES DIFFERENT SHINE HERE */

#if 0
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SKIP_EXPLORE, 0,				// set sphere map on geometry texture
									1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);

	SetSphereMapInfoOnMaterialObject(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_SkipBlink].materialObject,	// also set sphere map on blink sprite texture
									MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);

	SetSphereMapInfoOnMaterialObject(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_SkipDead].materialObject,	// also set sphere map on death sprite texture
									MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
#endif
}

/********************* LOAD LEVEL ART: BALSA **********************/

static void LoadLevelArt_Balsa(OGLSetupOutputType *setupInfo)
{
				/* DRAGONFLY */

	LoadASkeleton(SKELETON_TYPE_DRAGONFLY, setupInfo);

				/* FROG */

	LoadASkeleton(SKELETON_TYPE_FROG, setupInfo);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_FROG, 0,				// set sphere map on geometry texture
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

				/* LOAD THE FOLIAGE MODELS */

	LoadFoliage(setupInfo);


}


#pragma mark -


/********************* LOAD LEVEL ART: TUNNEL **********************/

void LoadLevelArt_Tunnel(OGLSetupOutputType *setupInfo)
{
FSSpec	spec, bg3dSpec;

				/* LOAD AUDIO */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Audio:Plumbing.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);


			/* LOAD SPRITES */

	if (gLevelNum == LEVEL_NUM_PLUMBING)
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:Level4_Plumbing.sprites", &spec);
		LoadSpriteFile(&spec, SPRITE_GROUP_LEVELSPECIFIC, setupInfo);
	}
	else
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:Level7_Gutter.sprites", &spec);
		LoadSpriteFile(&spec, SPRITE_GROUP_LEVELSPECIFIC, setupInfo);
	}

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:infobar.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_INFOBAR, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:global.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS, setupInfo);


			/* LOAD PLAYER SKELETON */

	LoadASkeleton(SKELETON_TYPE_SKIP_TUNNEL, setupInfo);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SKIP_TUNNEL, 0,				// set sphere map on geometry texture
									1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkYosemite);

	SetSphereMapInfoOnMaterialObject(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_SkipBlink].materialObject,	// also set sphere map on blink sprite texture
									MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkYosemite);

	SetSphereMapInfoOnMaterialObject(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_SkipDead].materialObject,	// also set sphere map on death sprite texture
									MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkYosemite);


			/************************************************/
			/* LOAD THE TUNNEL & ASSOCIATED BG3D MODEL FILE */
			/************************************************/

					/* PLUMBING */

	if (gLevelNum == LEVEL_NUM_PLUMBING)
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Models:Level4_Plumbing.bg3d", &bg3dSpec);
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Tunnels:Plumbing.tun", &spec);

		LoadTunnel(&spec, &bg3dSpec, setupInfo);

		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLUMBING_ObjType_Nail,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
	}

					/* GUTTER */

	else
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Models:Level7_Gutter.bg3d", &bg3dSpec);
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Tunnels:Gutter.tun", &spec);
		LoadTunnel(&spec, &bg3dSpec, setupInfo);

//		SetSphereMapInfoOnMaterialObject(gTunnelTextureObj,	MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkYosemite);

	}


}



