//
// sobjtypes.h
//


enum
{
	SPRITE_GROUP_SPHEREMAPS 		=	0,
	SPRITE_GROUP_GLOBAL				=	1,
	SPRITE_GROUP_PARTICLES			=	2,
	SPRITE_GROUP_INFOBAR			=	3,
	SPRITE_GROUP_LEVELSPECIFIC		=	4,
	MAX_SPRITE_GROUPS
};

		/* GLOBAL SPRITES */

enum
{
	GLOBAL_SObjType_Shadow_Circular,
	GLOBAL_SObjType_Shadow_BalsaPlane,
	GLOBAL_SObjType_Shadow_CircularDark,
	GLOBAL_SObjType_Shadow_Square,

	GLOBAL_SObjType_RainDrop,
	GLOBAL_SObjType_WaterRipple,

	GLOBAL_SObjType_Water,
	GLOBAL_SObjType_PoolWater,
	GLOBAL_SObjType_GarbageWater,

	GLOBAL_SObjType_SkipBlink,
	GLOBAL_SObjType_SkipDead,
	GLOBAL_SObjType_HouseFlyBlink,

	GLOBAL_SObjType_Fence_Grass,
	GLOBAL_SObjType_Fence_Brick,
	GLOBAL_SObjType_COUNT,
};





		/* SPHEREMAP SPRITES */

enum
{
	SPHEREMAP_SObjType_Satin,
	SPHEREMAP_SObjType_Sea,
	SPHEREMAP_SObjType_DarkDusk,
	SPHEREMAP_SObjType_Medow,
	SPHEREMAP_SObjType_Sheen,
	SPHEREMAP_SObjType_DarkYosemite,
	SPHEREMAP_SObjType_Red,
	SPHEREMAP_SObjType_Tundra,
	SPHEREMAP_SObjType_SheenAlpha,
	SPHEREMAP_SObjType_COUNT,
};



		/* PARTICLE SPRITES */

enum
{
	PARTICLE_SObjType_WhiteSpark,
	PARTICLE_SObjType_WhiteSpark2,
	PARTICLE_SObjType_WhiteSpark3,
	PARTICLE_SObjType_WhiteSpark4,
	PARTICLE_SObjType_WhiteGlow,

	PARTICLE_SObjType_RedGlint,
	PARTICLE_SObjType_GreenGlint,
	PARTICLE_SObjType_BlueGlint,
	PARTICLE_SObjType_YellowGlint,

	PARTICLE_SObjType_RedSpark,
	PARTICLE_SObjType_GreenSpark,
	PARTICLE_SObjType_BlueSpark,

	PARTICLE_SObjType_GreySmoke,
	PARTICLE_SObjType_BlackSmoke,
	PARTICLE_SObjType_RedFumes,
	PARTICLE_SObjType_GreenFumes,
	PARTICLE_SObjType_CokeSpray,

	PARTICLE_SObjType_Splash,
	PARTICLE_SObjType_SnowFlakes,
	PARTICLE_SObjType_Fire,
	PARTICLE_SObjType_Bubble,

	PARTICLE_SObjType_YwllowDiasyConfetti,
	PARTICLE_SObjType_PurpleDiasyConfetti,

	PARTICLE_SObjType_LensFlare0,
	PARTICLE_SObjType_LensFlare1,
	PARTICLE_SObjType_LensFlare2,
	PARTICLE_SObjType_LensFlare3,

	PARTICLE_SObjType_COUNT,
};

/******************* INFOBAR SOBJTYPES *************************/

enum
{
	INFOBAR_SObjType_LeftCorner,
	INFOBAR_SObjType_RightCorner,

	INFOBAR_SObjType_Health,

	INFOBAR_SObjType_Flight,
	INFOBAR_SObjType_Life,

	INFOBAR_SObjType_MapDot,

	INFOBAR_SObjType_PausedFrame,
	INFOBAR_SObjType_PausedDot,

	INFOBAR_SObjType_Mouse,

	INFOBAR_SObjType_BlueClover1,
	INFOBAR_SObjType_BlueClover2,
	INFOBAR_SObjType_BlueClover3,
	INFOBAR_SObjType_BlueClover4,
	INFOBAR_SObjType_GoldClover1,
	INFOBAR_SObjType_GoldClover2,
	INFOBAR_SObjType_GoldClover3,
	INFOBAR_SObjType_GoldClover4,

	INFOBAR_SObjTypes_DialogFrame,
	INFOBAR_SObjTypes_AcornIcon,
	INFOBAR_SObjTypes_RedKeyIcon,
	INFOBAR_SObjTypes_GreenKeyIcon,
	INFOBAR_SObjTypes_BlueKeyIcon,
	INFOBAR_SObjTypes_BeeIcon,

	INFOBAR_SObjType_COUNT,
};


/******************* GARDEN LEVEL *************************/

enum
{
	GARDEN_SObjType_Map,
	GARDEN_SObjType_Mouse,
	GARDEN_SObjType_SnailShellIcon,
	GARDEN_SObjType_ScarecrowHeadIcon,

	GARDEN_SObjType_Fence_Edging,

	GARDEN_SObjType_COUNT,
};



/******************* SIDEWALK LEVEL *************************/

enum
{
	SIDEWALK_SObjType_SnakeSkin,
	SIDEWALK_SObjType_SquishBerry,
	SIDEWALK_SObjType_COUNT,
};


/******************* FIDO LEVEL *************************/

enum
{
	FIDO_SObjType_Flea,
	FIDO_SObjType_Tick,

	FIDO_SObjType_Fence_DogHair,
	FIDO_SObjType_Fence_DogCollar,
	FIDO_SObjType_Fence_DogHairDense,

	FIDO_SObjType_COUNT,
};


/******************* PLUMBING LEVEL *************************/

enum
{
	PLUMBING_SObjType_Water0 = 0,
	PLUMBING_SObjType_Water1,
	PLUMBING_SObjType_Water2,
	PLUMBING_SObjType_Water3,
	PLUMBING_SObjType_Water4,
	PLUMBING_SObjType_Water5,
	PLUMBING_SObjType_COUNT,
};


/******************* GUTTER LEVEL *************************/

enum
{
	GUTTER_SObjType_Water0 = 0,
	GUTTER_SObjType_COUNT,
};


/******************* PLAYROOM LEVEL *************************/

enum
{
	PLAYROOM_SObjType_Map = 0,

	PLAYROOM_SObjType_MarbleIcon,

	PLAYROOM_SObjType_Fence_Cards,
	PLAYROOM_SObjType_Fence_Blocks,

	PLAYROOM_SObjType_COUNT,
};


/******************* CLOSET LEVEL *************************/

enum
{
	CLOSET_SObjType_Map = 0,

	CLOSET_SObjType_Fence_Cloth,
	CLOSET_SObjType_Fence_Books,
	CLOSET_SObjType_Fence_Computer,
	CLOSET_SObjType_Fence_ShoeBox,

	CLOSET_SObjType_RedClover,

	CLOSET_SObjType_ChipIcon,
	CLOSET_SObjType_MothBallIcon,

	CLOSET_SObjType_COUNT,
};

/******************* GARBAGE LEVEL *************************/

enum
{
	GARBAGE_SObjType_Fence_GarbageCan,
	GARBAGE_SObjType_Fence_Box,
	GARBAGE_SObjType_Mouse,
	GARBAGE_SObjType_CanIcon,
	GARBAGE_SObjType_PropIcon,

	GARBAGE_SObjType_COUNT,
};



/******************* BALSA LEVEL *************************/

enum
{
	BALSA_SObjType_AntHillIcon,

	BALSA_SObjType_Fence_Balsa,

	BALSA_SObjType_COUNT,
};

/******************* PARK LEVEL *************************/

enum
{
	PARK_SObjType_WaterGrassFence,
	PARK_SObjType_Map,

	PARK_SObjType_FishIcon,
	PARK_SObjType_CheeseIcon,
	PARK_SObjType_CherryIcon,
	PARK_SObjType_OliveIcon,
	PARK_SObjType_KindlingIcon,

	PARK_SObjType_BobberIcon,
	PARK_SObjType_FireIcon,
	PARK_SObjType_HiveIcon,

	PARK_SObjType_COUNT,
};




