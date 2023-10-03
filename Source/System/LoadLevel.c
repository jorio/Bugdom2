/****************************/
/*      LOAD LEVEL.C        */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include 	"game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void LoadLevelArt_Playroom(void);
static void LoadLevelArt_Garden(void);
static void LoadLevelArt_Sidewalk(void);
static void LoadLevelArt_Fido(void);
static void LoadLevelArt_Closet(void);
static void LoadLevelArt_Balsa(void);
static void LoadLevelArt_Park(void);
static void LoadLevelArt_Garbage(void);


/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/



/************************** LOAD LEVEL ART: EXPLORE ***************************/

void LoadLevelArt_Explore(void)
{
FSSpec	spec;

const char*	terrainFiles[NUM_LEVELS] =
{
	":Terrain:Level1_Garden.ter",
	":Terrain:Level2_SideWalk.ter",
	":Terrain:Level3_DogHair.ter",
	"",
	":Terrain:Level5_Playroom.ter",

	":Terrain:Level6_Closet.ter",
	"",
	":Terrain:Level8_Garbage.ter",
	":Terrain:Level9_Balsa.ter",
	":Terrain:Level10_Park.ter",
};

const char*	levelModelFiles[NUM_LEVELS] =
{
	":Models:Level1_Garden.bg3d",
	":Models:Level2_Sidewalk.bg3d",
	":Models:Level1_Garden.bg3d",
	"",
	":Models:Level5_Playroom.bg3d",

	":Models:Level6_Closet.bg3d",
	"",
	":Models:Level8_Garbage.bg3d",
	":Models:Level9_Balsa.bg3d",
	":Models:Level10_Park.bg3d"
};

const char*	levelSpriteFiles[NUM_LEVELS] =
{
	"Level1_Garden",
	"Level2_Sidewalk",
	"Level3_DogHair",
	"",
	"Level5_Playroom",
	"Level6_Closet",
	"",
	"Level8_Garbage",
	"Level9_Balsa",
	"Level10_Park",
};

const int levelSpriteCount[NUM_LEVELS] =
{
	GARDEN_SObjType_COUNT,
	SIDEWALK_SObjType_COUNT,
	FIDO_SObjType_COUNT,
	0,
	PLAYROOM_SObjType_COUNT,
	CLOSET_SObjType_COUNT,
	0,
	GARBAGE_SObjType_COUNT,
	BALSA_SObjType_COUNT,
	PARK_SObjType_COUNT,
};


const char*	levelSoundFiles[NUM_LEVELS] =
{
	":Audio:Garden.sounds",
	":Audio:Garden.sounds",
	":Audio:Fido.sounds",
	"",
	":Audio:Playroom.sounds",

	":Audio:Closet.sounds",
	"",
	":Audio:Garbage.sounds",
	":Audio:Balsa.sounds",
	":Audio:Park.sounds",
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

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL);

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
		ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);
	}


			/* LOAD SPRITES */

	if (levelSpriteCount[gLevelNum] > 0)
	{
		LoadSpriteGroupFromSeries(SPRITE_GROUP_LEVELSPECIFIC, levelSpriteCount[gLevelNum], levelSpriteFiles[gLevelNum]);
	}

	LoadSpriteGroupFromSeries(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_COUNT, "Infobar");


			/* LOAD PLAYER SKELETON */

	LoadASkeleton(SKELETON_TYPE_SKIP_EXPLORE);

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

	LoadASkeleton(SKELETON_TYPE_SNAIL);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SNAIL, 0,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_SnailShell,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

			/* LOAD CHIPMUNK */

	LoadASkeleton(SKELETON_TYPE_CHIPMUNK);


			/* LOAD BUDDYBUG */

	LoadASkeleton(SKELETON_TYPE_BUDDYBUG);

				/* HOUSEFLY */

	LoadASkeleton(SKELETON_TYPE_HOUSEFLY);


				/* FLEA */

	LoadASkeleton(SKELETON_TYPE_FLEA);

				/* BUMBLE BEE */

	LoadASkeleton(SKELETON_TYPE_BUMBLEBEE);

	LoadASkeleton(SKELETON_TYPE_HOBOBAG);


			/* LOAD CHECKPOINT */

	LoadASkeleton(SKELETON_TYPE_CHECKPOINT);
	if (gG4)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_CHECKPOINT, 0,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);

	}


			/* LOAD MOUSETRAP */

	LoadASkeleton(SKELETON_TYPE_MOUSETRAP);
	LoadASkeleton(SKELETON_TYPE_MOUSE);



			/* LOAD TERRAIN */
			//
			// must do this after creating the view!
			//

	if (terrainFiles[gLevelNum][0] > 0)
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, terrainFiles[gLevelNum], &spec);
		LoadPlayfield(&spec);
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
				LoadLevelArt_Garden();
				break;

		case	LEVEL_NUM_SIDEWALK:
				LoadLevelArt_Sidewalk();
				break;

		case	LEVEL_NUM_FIDO:
				LoadLevelArt_Fido();
				break;

		case	LEVEL_NUM_PLAYROOM:
				LoadLevelArt_Playroom();
				break;

		case	LEVEL_NUM_CLOSET:
				LoadLevelArt_Closet();
				break;

		case	LEVEL_NUM_GARBAGE:
				LoadLevelArt_Garbage();
				break;

		case	LEVEL_NUM_BALSA:
				LoadLevelArt_Balsa();
				break;

		case	LEVEL_NUM_PARK:
				LoadLevelArt_Park();
				break;

		default:
				DoFatalAlert("Sorry, Bud!  That level doesn't exist yet.");
	}
}


/********************** LOAD FOLIAGE **************************/

void LoadFoliage(void)
{
FSSpec	spec;

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Foliage.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_FOLIAGE);

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

static void LoadLevelArt_Garden(void)
{

	LoadASkeleton(SKELETON_TYPE_GNOME);
	if (!gSlowCPU)									// no reflection mapping when speed is a problem
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_GNOME, 0,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
	}



									/* EVIL PLANT */

	LoadASkeleton(SKELETON_TYPE_EVILPLANT);

	if (!gSlowCPU)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_EVILPLANT, 0,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
	}





							/* LOAD THE FOLIAGE MODELS */

	LoadFoliage();



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

static void LoadLevelArt_Sidewalk(void)
{
int	i;

	LoadASkeleton(SKELETON_TYPE_GNOME);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_GNOME, 0,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);

								/* LOAD SNAKE HEAD */

	LoadASkeleton(SKELETON_TYPE_SNAKEHEAD);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SNAKEHEAD, 0,
									0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);




							/* LOAD THE FOLIAGE MODELS */

	LoadFoliage();


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

static void LoadLevelArt_Fido(void)
{


	LoadASkeleton(SKELETON_TYPE_TICK);
	if (!gSlowCPU)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_TICK, 0,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);
	}


}


/********************* LOAD LEVEL ART: GARBAGE **********************/

static void LoadLevelArt_Garbage(void)
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

	LoadASkeleton(SKELETON_TYPE_ROACH);


				/* FLEA */

	LoadASkeleton(SKELETON_TYPE_FLEA);


				/* DRAGONFLY */

	LoadASkeleton(SKELETON_TYPE_DRAGONFLY);

}


/********************* LOAD LEVEL ART: PARK **********************/

static void LoadLevelArt_Park(void)
{
int		i;

							/* LOAD THE FOLIAGE MODELS */

	LoadFoliage();


				/* ANT */

	LoadASkeleton(SKELETON_TYPE_ANT);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_ANT, 0,				// set sphere map on geometry texture
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);


				/* FISH */


	LoadASkeleton(SKELETON_TYPE_FISH);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_FISH, 0,				// set sphere map on geometry texture
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);


				/* FROG */

	LoadASkeleton(SKELETON_TYPE_FROG);
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

static void LoadLevelArt_Playroom(void)
{

	LoadASkeleton(SKELETON_TYPE_OTTO);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_OTTO,
								 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);



				/* SOLDIER */

	LoadASkeleton(SKELETON_TYPE_TOYSOLDIER);
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

static void LoadLevelArt_Closet(void)
{
				/* ROACH */

	LoadASkeleton(SKELETON_TYPE_ROACH);


				/* MOTH */

	LoadASkeleton(SKELETON_TYPE_MOTH);


				/* COMPUTER BUG */

	LoadASkeleton(SKELETON_TYPE_COMPUTERBUG);

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

	LoadASkeleton(SKELETON_TYPE_TOYSOLDIER);
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

static void LoadLevelArt_Balsa(void)
{
				/* DRAGONFLY */

	LoadASkeleton(SKELETON_TYPE_DRAGONFLY);

				/* FROG */

	LoadASkeleton(SKELETON_TYPE_FROG);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_FROG, 0,				// set sphere map on geometry texture
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

				/* LOAD THE FOLIAGE MODELS */

	LoadFoliage();


}


#pragma mark -


/********************* LOAD LEVEL ART: TUNNEL **********************/

void LoadLevelArt_Tunnel(void)
{
FSSpec	spec;

				/* LOAD AUDIO */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Plumbing.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);


			/* LOAD SPRITES */

	if (gLevelNum == LEVEL_NUM_PLUMBING)
	{
		LoadSpriteGroupFromSeries(SPRITE_GROUP_LEVELSPECIFIC, PLUMBING_SObjType_COUNT, "Level4_Plumbing");
	}
	else
	{
		LoadSpriteGroupFromSeries(SPRITE_GROUP_LEVELSPECIFIC, GUTTER_SObjType_COUNT, "Level7_Gutter");
	}


	LoadSpriteGroupFromSeries(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_COUNT, "Infobar");


			/* LOAD PLAYER SKELETON */

	LoadASkeleton(SKELETON_TYPE_SKIP_TUNNEL);

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
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Level4_Plumbing.bg3d", &spec);
		ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);

		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Tunnels:Plumbing.tun", &spec);
		LoadTunnel(&spec);

		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, PLUMBING_ObjType_Nail,
										-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);
	}

					/* GUTTER */

	else
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Level7_Gutter.bg3d", &spec);
		ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);

		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Tunnels:Gutter.tun", &spec);
		LoadTunnel(&spec);

//		SetSphereMapInfoOnMaterialObject(gTunnelTextureObj,	MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkYosemite);
	}
}



