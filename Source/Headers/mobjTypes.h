//
// mobjtypes.h
//

#ifndef __MOBJT
#define __MOBJT

enum
{
	MODEL_GROUP_GLOBAL		=	0,
	MODEL_GROUP_LEVELSPECIFIC =	1,
	MODEL_GROUP_TITLE		=	1,
	MODEL_GROUP_LEVELINTRO =	2,
	MODEL_GROUP_WINNERS		= 	2,
	MODEL_GROUP_BONUS		= 	2,
	MODEL_GROUP_MAINMENU	= 	2,
	MODEL_GROUP_HIGHSCORES	=	2,
	MODEL_GROUP_LOSESCREEN	=	2,
	MODEL_GROUP_WINSCREEN	=	2,
	MODEL_GROUP_FOLIAGE		 = 	3,

	MODEL_GROUP_SKELETONBASE				// skeleton files' models are attached here
};




/******************* GLOBAL *************************/

enum
{
	GLOBAL_ObjType_Shield = 0,

	GLOBAL_ObjType_RightWing,
	GLOBAL_ObjType_LeftWing,
	GLOBAL_ObjType_WingBlur1,
	GLOBAL_ObjType_WingBlur2,
	GLOBAL_ObjType_WingBlur3,
	GLOBAL_ObjType_WingBlur4,
	GLOBAL_ObjType_WingBlur5,
	GLOBAL_ObjType_WingBlur6,

	GLOBAL_ObjType_WaterSpat,

	GLOBAL_ObjType_ButterflyBody,
	GLOBAL_ObjType_ButterflyLeftWing,
	GLOBAL_ObjType_ButterflyRightWing,

	GLOBAL_ObjType_HealthPOW,
	GLOBAL_ObjType_FlightPOW,
	GLOBAL_ObjType_MapPOW,
	GLOBAL_ObjType_FreeLife,
	GLOBAL_ObjType_RamGrainPOW,
	GLOBAL_ObjType_RedKeyPOW,
	GLOBAL_ObjType_GreenKeyPOW,
	GLOBAL_ObjType_BlueKeyPOW,
	GLOBAL_ObjType_GreenCloverPOW,
	GLOBAL_ObjType_BlueCloverPOW,
	GLOBAL_ObjType_GoldCloverPOW,
	GLOBAL_ObjType_ShieldPOW,

	GLOBAL_ObjType_Acorn,

	GLOBAL_ObjType_SnailShell,

	GLOBAL_ObjType_BottleCap,
	GLOBAL_ObjType_TickSpit,
	GLOBAL_ObjType_RoachSpear,

	GLOBAL_ObjType_Firecracker,
	GLOBAL_ObjType_Grenade

};


/******************* FOLIAGE *************************/

enum
{
	FOLIAGE_ObjType_Daisy1 = 0,
	FOLIAGE_ObjType_Daisy2,
	FOLIAGE_ObjType_Daisy3,

	FOLIAGE_ObjType_Tulip1,
	FOLIAGE_ObjType_Tulip2,
	FOLIAGE_ObjType_Tulip3,
	FOLIAGE_ObjType_Tulip4,

	FOLIAGE_ObjType_Grass1,
	FOLIAGE_ObjType_Grass2,
	FOLIAGE_ObjType_Grass3,

	FOLIAGE_ObjType_ShrubRoot,

	FOLIAGE_ObjType_Rose
};


/******************* LEVEL 1: GARDEN *************************/

enum
{
	GARDEN_ObjType_Cyclorama = 0,
	GARDEN_ObjType_SprinklerBase,
	GARDEN_ObjType_SprinklerPost,
	GARDEN_ObjType_SprinklerSpray,
	GARDEN_ObjType_PollenSpore,

	GARDEN_ObjType_RedDoor,
	GARDEN_ObjType_GreenDoor,
	GARDEN_ObjType_BlueDoor,

	GARDEN_ObjType_ScarecrowBody,
	GARDEN_ObjType_ScarecrowShirt,
	GARDEN_ObjType_ScarecrowHead,

	GARDEN_ObjType_Brick,

	GARDEN_ObjType_Post_Brick,
	GARDEN_ObjType_Post_Edge,
	GARDEN_ObjType_Post_Grass,

	GARDEN_ObjType_LargeStone,
	GARDEN_ObjType_MediumStone,
	GARDEN_ObjType_SmallStone

};


/******************* LEVEL 2: GARDEN *************************/

enum
{
	SIDEWALK_ObjType_Cyclorama = 0,
	SIDEWALK_ObjType_SprinklerBase,
	SIDEWALK_ObjType_SprinklerPost,
	SIDEWALK_ObjType_SprinklerSpray,
	SIDEWALK_ObjType_PollenSpore,

	SIDEWALK_ObjType_RedDoor,
	SIDEWALK_ObjType_GreenDoor,
	SIDEWALK_ObjType_BlueDoor,

	SIDEWALK_ObjType_Brick,
	SIDEWALK_ObjType_LargeStone,
	SIDEWALK_ObjType_MediumStone,
	SIDEWALK_ObjType_SmallStone,

	SIDEWALK_ObjType_Post_Brick,
	SIDEWALK_ObjType_Post_Edge,
	SIDEWALK_ObjType_Post_Grass,

	SIDEWALK_ObjType_RideBall,

	SIDEWALK_ObjType_PoolLeaf1,
	SIDEWALK_ObjType_PoolLeaf2,
	SIDEWALK_ObjType_PoolLeaf3,
	SIDEWALK_ObjType_PoolLeaf4,

	SIDEWALK_ObjType_Coping,
	SIDEWALK_ObjType_CornerCoping,
	SIDEWALK_ObjType_BeachBall,
	SIDEWALK_ObjType_ChlorineFloat,
	SIDEWALK_ObjType_PoolRingFloat,

	SIDEWALK_ObjType_SquishBerry,
	SIDEWALK_ObjType_SquishSplat,

	SIDEWALK_ObjType_DogHouse,

	SIDEWALK_ObjType_WindmillBase,
	SIDEWALK_ObjType_WindmillBlades,

	SIDEWALK_ObjType_TulipPot,
	SIDEWALK_ObjType_DrainPipe,
	SIDEWALK_ObjType_Grate,
	SIDEWALK_ObjType_Bottle,
	SIDEWALK_ObjType_BottleCrack1,
	SIDEWALK_ObjType_BottleCrack2,
	SIDEWALK_ObjType_BottleCrack3,
	SIDEWALK_ObjType_BottleCrack4
};


/******************* LEVEL 4: PLUMBING *************************/

enum
{
	PLUMBING_ObjType_Nail = 0,
	PLUMBING_ObjType_Blob,
	PLUMBING_ObjType_HealthPOW,
	PLUMBING_ObjType_Ring,

	PLUMBING_ObjType_Spray
};



/******************* LEVEL 5: PLAYROOM **********************/

enum
{
	PLAYROOM_ObjType_Cyclorama = 0,
	PLAYROOM_ObjType_LetterBlock1,
	PLAYROOM_ObjType_LetterBlock2,
	PLAYROOM_ObjType_LetterBlock3,

	PLAYROOM_ObjType_BlockPost,
	PLAYROOM_ObjType_CardPost,

	PLAYROOM_ObjType_MarbleShell,
	PLAYROOM_ObjType_MarbleGuts,
	PLAYROOM_ObjType_Battery,
	PLAYROOM_ObjType_DCell,

	PLAYROOM_ObjType_FinishLine,
	PLAYROOM_ObjType_SlotCarRed,
	PLAYROOM_ObjType_SlotCarYellow,
	PLAYROOM_ObjType_FrontWheels,
	PLAYROOM_ObjType_RearWheels,

	PLAYROOM_ObjType_OttoKey,
	PLAYROOM_ObjType_OttoStunPulse,

	PLAYROOM_ObjType_BeachBall,
	PLAYROOM_ObjType_Baseball,
	PLAYROOM_ObjType_Crayon,

	PLAYROOM_ObjType_PuzzleMain,
	PLAYROOM_ObjType_PuzzlePiece1,
	PLAYROOM_ObjType_PuzzlePiece2,
	PLAYROOM_ObjType_PuzzlePiece3,

	PLAYROOM_ObjType_LegoWall,
	PLAYROOM_ObjType_LegoBrick_Red,
	PLAYROOM_ObjType_LegoBrick_Orange,
	PLAYROOM_ObjType_LegoBrick_Green,
	PLAYROOM_ObjType_LegoBrick_Blue,
	PLAYROOM_ObjType_LegoBrick_Yellow,

	PLAYROOM_ObjType_RedDoor,
	PLAYROOM_ObjType_GreenDoor,
	PLAYROOM_ObjType_BlueDoor
};


/******************* LEVEL 6: CLOSET *************************/

enum
{
	CLOSET_ObjType_FlashLight = 0,
	CLOSET_ObjType_LightBeam,

	CLOSET_ObjType_CardboardBox1,
	CLOSET_ObjType_CardboardBox2,
	CLOSET_ObjType_CardboardBox3,
	CLOSET_ObjType_CardboardBox4,

	CLOSET_ObjType_ShoeBox,

	CLOSET_ObjType_TrampolineBase,
	CLOSET_ObjType_TrampolineWebDown,
	CLOSET_ObjType_TrampolineWebUp,

	CLOSET_ObjType_MothBall,
	CLOSET_ObjType_Vacuume,
	CLOSET_ObjType_Light,

	CLOSET_ObjType_PCICard,
	CLOSET_ObjType_OpenBook,
	CLOSET_ObjType_ClosedBook,
	CLOSET_ObjType_FlatBook,
	CLOSET_ObjType_BookStack,
	CLOSET_ObjType_TallBookStack,
	CLOSET_ObjType_DiaryDoor,

	CLOSET_ObjType_Virus,

	CLOSET_ObjType_SiliconDoor,
	CLOSET_ObjType_Chip1,
	CLOSET_ObjType_Chip2,
	CLOSET_ObjType_Battery,

	CLOSET_ObjType_Hanger,
	CLOSET_ObjType_RedClover,
	CLOSET_ObjType_PaperMap,

	CLOSET_ObjType_PictureFrame_Brian,
	CLOSET_ObjType_PictureFrame_Peter,
	CLOSET_ObjType_PictureFrame_Tuncer

};



/******************* LEVEL 7: GUTTER *************************/

enum
{
	GUTTER_ObjType_PineCone = 0,
	GUTTER_ObjType_Leaf,
	GUTTER_ObjType_Tree,
	GUTTER_ObjType_Chimney,

	GUTTER_ObjType_Cyc,
	GUTTER_ObjType_Spray
};


/******************* LEVEL 8: GARBAGE *************************/

enum
{
	GARBAGE_ObjType_Cyc = 0,
	GARBAGE_ObjType_Can,
	GARBAGE_ObjType_Tab,
	GARBAGE_ObjType_Cap,

	GARBAGE_ObjType_Banana,
	GARBAGE_ObjType_Onion,
	GARBAGE_ObjType_Tomato,
	GARBAGE_ObjType_Zuke,

	GARBAGE_ObjType_Jar,
	GARBAGE_ObjType_BrokenJar,

	GARBAGE_ObjType_TinCan,
	GARBAGE_ObjType_Detergent,
	GARBAGE_ObjType_BoxWall,

	GARBAGE_ObjType_RedDoor,
	GARBAGE_ObjType_GreenDoor,

	GARBAGE_ObjType_Glider,
	GARBAGE_ObjType_Wheel,
	GARBAGE_ObjType_Propeller,
	GARBAGE_ObjType_Rubber,

	GARBAGE_ObjType_TPpost
};


/******************* LEVEL 9: BALSA *************************/

enum
{
	BALSA_ObjType_Plane,
	BALSA_ObjType_Prop,
	BALSA_ObjType_RubberBand,
	BALSA_ObjType_AntHill,
	BALSA_ObjType_Cloud,

	BALSA_ObjType_Bomb,
	BALSA_ObjType_Shockwave,
	BALSA_ObjType_Bullet
};


/******************* LEVEL 10: PARK **********************/

enum
{
	PARK_ObjType_Cyclorama = 0,
	PARK_ObjType_CheeseBit,
	PARK_ObjType_Cherry,
	PARK_ObjType_Olive,

	PARK_ObjType_LilyPad,
	PARK_ObjType_CatTail,

	PARK_ObjType_ShortFlower,
	PARK_ObjType_MediumFlower,
	PARK_ObjType_TallFlower,

	PARK_ObjType_Lure,

	PARK_ObjType_Fork,
	PARK_ObjType_Spoon,
	PARK_ObjType_Knife,

	PARK_ObjType_PicnicBasket,

	PARK_ObjType_RedDoor,
	PARK_ObjType_GreenDoor,
	PARK_ObjType_BlueDoor,

	PARK_ObjType_GrassPost,

	PARK_ObjType_Bottle,
	PARK_ObjType_BottleCrack1,
	PARK_ObjType_BottleCrack2,
	PARK_ObjType_BottleCrack3,
	PARK_ObjType_BottleCrack4,

	PARK_ObjType_Leaf,
	PARK_ObjType_Twig,

	PARK_ObjType_Hive,
	PARK_ObjType_HiveDoor
};

#endif









