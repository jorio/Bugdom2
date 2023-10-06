/****************************/
/*     SOUND ROUTINES       */
/* By Brian Greenstone      */
/* (c)2002 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static short FindSilentChannel(void);
static void Calc3DEffectVolume(int effectNum, OGLPoint3D *where, float volAdjust, uint32_t *leftVolOut, uint32_t *rightVolOut);
static void UpdateGlobalVolume(void);


/****************************/
/*    CONSTANTS             */
/****************************/


#define		MAX_CHANNELS			40


typedef struct
{
	const char* bank;
	const char* name;
	int refDistance;
}EffectType;

typedef struct
{
	float			lowFrequencyStrength;
	float			highFrequencyStrength;
	uint16_t		duration;
} AutoRumbleDef;



#define	VOLUME_DISTANCE_FACTOR	.004f		// bigger == sound decays FASTER with dist, smaller = louder far away

/**********************/
/*     VARIABLES      */
/**********************/

float						gGlobalVolume = 0.8f;

static OGLPoint3D			gEarCoords;										// coord of camera plus a tad to get pt in front of camera
static	OGLVector3D			gEyeVector;


static	SndListHandle		gSndHandles[NUM_EFFECTS];		// handles to ALL sounds
static  long				gSndOffsets[NUM_EFFECTS];

static	SndChannelPtr		gSndChannel[MAX_CHANNELS];
ChannelInfoType				gChannelInfo[MAX_CHANNELS];

static short				gMaxChannels = 0;

static short				gMostRecentChannel = -1;

Boolean						gAllowAudioKeys = true;

int							gCurrentSong = -1;
short						gSongChannel = -1;


		/*****************/
		/* EFFECTS TABLE */
		/*****************/

static const char* kBankNames[NUM_SOUND_BANKS] =
{
	[SOUND_BANK_MUSIC]		= "Music",
	[SOUND_BANK_MAIN]		= "Main",
	[SOUND_BANK_TITLE]		= "Title",
	[SOUND_BANK_BONUS]		= "Bonus",
	[SOUND_BANK_GARDEN]		= "Garden",
	[SOUND_BANK_FIDO]		= "Fido",
	[SOUND_BANK_PLUMBING]	= "Plumbing",
	[SOUND_BANK_PLAYROOM]	= "Playroom",
	[SOUND_BANK_CLOSET]		= "Closet",
	[SOUND_BANK_GARBAGE]	= "Garbage",
	[SOUND_BANK_BALSA]		= "Balsa",
	[SOUND_BANK_PARK]		= "Park",
};

static EffectType	gEffectsTable[] =
{
	[EFFECT_SONG_TITLE]				= {"Music",		"TitleSong",			2000},
	[EFFECT_SONG_THEME]				= {"Music",		"ThemeSong",			2000},
	[EFFECT_SONG_GARDEN]			= {"Music",		"GardenSong",			2000},
	[EFFECT_SONG_POOL]				= {"Music",		"PoolSong",				2000},
	[EFFECT_SONG_FIDO]				= {"Music",		"FidoSong",				2000},
	[EFFECT_SONG_PLUMBING]			= {"Music",		"PlumbingSong",			2000},
	[EFFECT_SONG_PLAYROOM]			= {"Music",		"PlayroomSong",			2000},
	[EFFECT_SONG_CLOSET]			= {"Music",		"ClosetSong",			2000},
	[EFFECT_SONG_GARBAGE]			= {"Music",		"GarbageSong",			2000},
	[EFFECT_SONG_BALSA]				= {"Music",		"BalsaSong",			2000},
	[EFFECT_SONG_PARK]				= {"Music",		"ParkSong",				2000},
	[EFFECT_SONG_BONUS]				= {"Music",		"BonusSong",			2000},
	[EFFECT_SONG_WIN]				= {"Music",		"WinSong",				2000},
	[EFFECT_SONG_LOSE]				= {"Music",		"LoseSong",				2000},

	[EFFECT_CHANGESELECT]			= {"Main",		"ChangeSelect",			2000},
	[EFFECT_JUMP]					= {"Main",		"Jump",					500},
	[EFFECT_SKIPGLIDE]				= {"Main",		"SkipGlide",			1000},
	[EFFECT_SKIPKICK]				= {"Main",		"SkipKick",				500},
	[EFFECT_SKIPLAND]				= {"Main",		"SkipLand",				300},
	[EFFECT_ACORNKICKED]			= {"Main",		"AcornKicked",			300},
	[EFFECT_FLYGOTKICKED]			= {"Main",		"FlyGotKicked",			500},
	[EFFECT_GETPOW]					= {"Main",		"GetPOW",				500},
	[EFFECT_BUTTERFLYBOOM]			= {"Main",		"ButterflyBoom",		100},
	[EFFECT_FLYWALKBUZZ]			= {"Main",		"FlyWalkBuzz",			200},
	[EFFECT_SMACK]					= {"Main",		"Smack",				500},
	[EFFECT_SPLASH]					= {"Main",		"Splash",				500},
	[EFFECT_BUDDYLAUNCH]			= {"Main",		"BuddyLaunch",			700},
	[EFFECT_BUDDYBUZZ]				= {"Main",		"BuddyBuzz",			700},
	[EFFECT_DOORCREAK]				= {"Main",		"DoorCreak",			800},
	[EFFECT_THROWBOTTLECAP]			= {"Main",		"ThrowBottleCap",		500},
	[EFFECT_BOTTLECAPBOUNCE]		= {"Main",		"BottleCapBounce",		10},
	[EFFECT_MOUSETRAP]				= {"Main",		"MouseTrap",			1000},
	[EFFECT_BUDDYBOOM]				= {"Main",		"BuddyBoom",			300},
	[EFFECT_SHIELD]					= {"Main",		"Shield",				300},
	[EFFECT_FIRECRACKER]			= {"Main",		"Firecracker",			600},
	[EFFECT_BUMBLERUMBLE]			= {"Main",		"BumbleRumble",			100},
	[EFFECT_GRENADEBOOM]			= {"Main",		"GrenadeBoom",			500},
	[EFFECT_PLANECRASH]				= {"Main",		"PlaneCrash",			6000},
	[EFFECT_DRAGONFLYBUZZ]			= {"Main",		"DragonFlyBuzz",		2000},
	[EFFECT_BOTTLECRACK]			= {"Main",		"BottleCrack",			700},
	[EFFECT_BOTTLESHATTER]			= {"Main",		"BottleShatter",		700},
	[EFFECT_PULLTRAP]				= {"Main",		"PullTrap",				1000},
	[EFFECT_SNAPTRAP]				= {"Main",		"SnapTrap",				1000},
	[EFFECT_POPACORN]				= {"Main",		"PopAcorn",				100},
	[EFFECT_FOOTSTEP]				= {"Main",		"Footstep",				100},
	[EFFECT_GRENADETHROW]			= {"Main",		"GrenadeThrow",			400},
	[EFFECT_CHIP_CHECKPOINT]		= {"Main",		"ChipCheckpoint1",		600},
	[EFFECT_CHIP_CHECKPOINTDONE]	= {"Main",		"ChipCheckpoint2",		100},
	[EFFECT_CHIP_MAP4ACORN]			= {"Main",		"ChipMap1",				600},
	[EFFECT_CHIP_THANKS]			= {"Main",		"ChipMap2",				100},

	[EFFECT_GNOMESTEP]				= {"Garden",	"GnomeStep",			500},
	[EFFECT_GNOMEGOTKICKED]			= {"Garden",	"GnomeGotKicked",		400},
	[EFFECT_SPRINKLER]				= {"Garden",	"Sprinkler",			300},
	[EFFECT_SQUISHBERRY]			= {"Garden",	"SquishBerry",			900},
	[EFFECT_EVILPLANTSHOOT]			= {"Garden",	"EvilPlantShoot",		700},
	[EFFECT_CHIP_STUCKMOUSE]		= {"Garden",	"ChipStuckMouse",		600},
	[EFFECT_SAM_FREEMICE]			= {"Garden",	"SamFreeMice1",			600},
	[EFFECT_SAM_FREEMICE2]			= {"Garden",	"SamFreeMice2",			600},
	[EFFECT_SAM_GOTMICE]			= {"Garden",	"SamFreeMice3",			600},
	[EFFECT_SAM_FINDSHELL]			= {"Garden",	"SamFindShell1",		600},
	[EFFECT_SAM_GOTSHELL]			= {"Garden",	"SamFindShell2",		600},
	[EFFECT_SAM_FINDHEAD]			= {"Garden",	"SamScarecrow1",		600},
	[EFFECT_SAM_PUTHEADON]			= {"Garden",	"SamScarecrow2",		600},
	[EFFECT_SAM_FIXEDSCARECROW]		= {"Garden",	"SamScarecrow3",		600},
	[EFFECT_SAM_SQUASHBERRIES]		= {"Garden",	"SamBerries1",			600},
	[EFFECT_SAM_SQUISHMORE]			= {"Garden",	"SamBerries2",			600},
	[EFFECT_SAM_SQUISHDONE]			= {"Garden",	"SamBerries3",			600},
	[EFFECT_SAM_POOLKEY]			= {"Garden",	"SamPoolKey",			600},
	[EFFECT_SAM_FIDO]				= {"Garden",	"SamFido",				600},

	[EFFECT_GOTFLEAS]				= {"Fido",		"SamGotFleas",			700},
	[EFFECT_GOTTICKS]				= {"Fido",		"SamGotTicks",			700},
	[EFFECT_HAPPYDOG]				= {"Fido",		"SamHappyDog",			700},
	[EFFECT_REMEMBERDOG]			= {"Fido",		"SamRemember",			700},
	[EFFECT_TICKSUCK]				= {"Fido",		"TickSuck",				700},
	[EFFECT_TICKSPIT]				= {"Fido",		"TickSpit",				400},
	[EFFECT_TICKSTEP]				= {"Fido",		"TickStep",				400},
	[EFFECT_TICKDIE]				= {"Fido",		"TickDie",				500},
	[EFFECT_BONEHIT]				= {"Fido",		"BoneHit",				500},

	[EFFECT_PLUMBINGINTRO]			= {"Plumbing",	"SamSewerIntro",		700},
	[EFFECT_TUNNELWATER]			= {"Plumbing",	"GutterWater",			1000},
	[EFFECT_METALSCRAPE]			= {"Plumbing",	"MetalScrape",			1000},
	[EFFECT_SLUDGEHIT]				= {"Plumbing",	"HitSludge",			700},
	[EFFECT_NAILHIT]				= {"Plumbing",	"HitNail",				700},
	[EFFECT_HITPINECONE]			= {"Plumbing",	"HitPineCone",			1000},
	[EFFECT_HITLEAF]				= {"Plumbing",	"HitLeaf",				1000},

	[EFFECT_LASERBOOM]				= {"Playroom",	"LaserBoom",			400},
	[EFFECT_BOWLINGHIT]				= {"Playroom",	"BowlingHit",			500},
	[EFFECT_KICKMARBLE]				= {"Playroom",	"KickMarble",			900},
	[EFFECT_SLOTCAR]				= {"Playroom",	"SlotCar",				400},
	[EFFECT_OTTOMOTOR]				= {"Playroom",	"OttoMotor",			10},
	[EFFECT_OTTOSHOOT]				= {"Playroom",	"OttoShoot",			400},
	[EFFECT_OTTOFALL]				= {"Playroom",	"OttoFall",				1000},
	[EFFECT_SAM_FINDMARBLE]			= {"Playroom",	"SamMarble1",			600},
	[EFFECT_SAM_KICKMARBLE]			= {"Playroom",	"SamMarble2",			600},
	[EFFECT_SAM_PINSDOWN]			= {"Playroom",	"SamMarble3",			600},
	[EFFECT_SAM_DOPUZZLE]			= {"Playroom",	"SamPuzzle1",			610},
	[EFFECT_SAM_PUZZLEDONE]			= {"Playroom",	"SamPuzzle2",			610},
	[EFFECT_CHIP_DORACE]			= {"Playroom",	"ChipDoRace",			610},
	[EFFECT_CHIP_READY]				= {"Playroom",	"ChipReady",			1500},
	[EFFECT_CHIP_SET]				= {"Playroom",	"ChipSet",				1500},
	[EFFECT_CHIP_GO]				= {"Playroom",	"ChipGo",				1500},
	[EFFECT_CHIP_WINNING]			= {"Playroom",	"ChipWinning",			1500},
	[EFFECT_CHIP_SAMWINNING]		= {"Playroom",	"ChipSamWinning",		1500},
	[EFFECT_CHIP_LOSTRACE]			= {"Playroom",	"ChipLostRace",			1500},
	[EFFECT_CHIP_YOUWON]			= {"Playroom",	"ChipYouWon",			1500},

	[EFFECT_PROPELLER]				= {"Balsa",		"Propeller",			400},
	[EFFECT_HILLBOOM]				= {"Balsa",		"AntHillBoom",			4000},
	[EFFECT_BOMBBOOM]				= {"Balsa",		"BombBoom",				3000},
	[EFFECT_PLANEHIT]				= {"Balsa",		"PlaneHit",				1500},
	[EFFECT_FROGJUMP]				= {"Balsa",		"FrogJump",				6000},
	[EFFECT_DIVEBOMB]				= {"Balsa",		"DiveBomb",				6000},
	[EFFECT_BOMBFALL]				= {"Balsa",		"BombFall",				6000},
	[EFFECT_BALSASHOOT]				= {"Balsa",		"BalsaShoot",			2000},
	[EFFECT_DRAGONFLYHIT]			= {"Balsa",		"DragonFlyHit",			2000},
	[EFFECT_SAM_DESTROYHILLS]		= {"Balsa",		"SamAntHills1",			2000},
	[EFFECT_SAM_HOWTOBOMB]			= {"Balsa",		"SamAntHills2",			2000},
	[EFFECT_SAM_HILLSDESTROYED]		= {"Balsa",		"SamAntHills3",			2000},

	[EFFECT_SERVO]					= {"Closet",	"Servo1",				10},
	[EFFECT_SERVO2]					= {"Closet",	"Servo2",				300},
	[EFFECT_MINEBOOM]				= {"Closet",	"MineBoom",				500},
	[EFFECT_CHIPCLICK]				= {"Closet",	"ChipClick",			500},
	[EFFECT_SILICONDOOROPEN]		= {"Closet",	"SiliconDoorOpen",		500},
	[EFFECT_VACUUME]				= {"Closet",	"Vacuum",				1500},
	[EFFECT_VACUUMECRUMCH]			= {"Closet",	"VacuumCrunch",			3000},
	[EFFECT_MOTHFLAP]				= {"Closet",	"MothFlap",				400},
	[EFFECT_SAM_GETREDCLOVERS]		= {"Closet",	"SamRedClovers1",		2000},
	[EFFECT_SAM_GOTREDCLOVERS]		= {"Closet",	"SamRedClovers2",		2000},
	[EFFECT_SAM_MOTHBALLS]			= {"Closet",	"SamMoths",				2000},
	[EFFECT_SAM_COMPUTERDOOR]		= {"Closet",	"SamComputer",			2000},

	[EFFECT_CANOPEN]				= {"Garbage",	"CanOpen",				2000},
	[EFFECT_SODASPRAY]				= {"Garbage",	"SodaSpray",			10},
	[EFFECT_PROP2]					= {"Garbage",	"Propeller2",			1000},
	[EFFECT_SAM_GUTTERWATER]		= {"Garbage",	"SamFlood1",			2000},
	[EFFECT_SAM_FREED]				= {"Garbage",	"SamFlood2",			2000},
	[EFFECT_SAM_GLIDER]				= {"Garbage",	"SamGlider",			2000},
	[EFFECT_SAM_SODA]				= {"Garbage",	"SamSoda",				2000},

	[EFFECT_FROGJUMP2]				= {"Park",		"FrogJump",				600},
	[EFFECT_TONGUEHIT]				= {"Park",		"TongueHit",			1000},
	[EFFECT_TONGUESWOOSH]			= {"Park",		"TongueSwoosh",			500},
	[EFFECT_ANTBITE]				= {"Park",		"AntBite",				200},
	[EFFECT_FISHFLOP]				= {"Park",		"FishFlop",				200},
	[EFFECT_SAM_BOTTLEKEY]			= {"Park",		"SamBottleKey",			2000},
	[EFFECT_SAM_CATCHFISH]			= {"Park",		"SamFish1",				2000},
	[EFFECT_SAM_KEEPFISHING]		= {"Park",		"SamFish2",				2000},
	[EFFECT_SAM_ANGLER]				= {"Park",		"SamFish3",				2000},
	[EFFECT_SAM_GETFOOD]			= {"Park",		"SamFood1",				2000},
	[EFFECT_SAM_MOREFOOD]			= {"Park",		"SamFood2",				2000},
	[EFFECT_SAM_GOTFOOD]			= {"Park",		"SamFood3",				2000},
	[EFFECT_SAM_GETKINDLING]		= {"Park",		"SamHive1",				2000},
	[EFFECT_SAM_MOREKINDLING]		= {"Park",		"SamHive2",				2000},
	[EFFECT_SAM_SPARK]				= {"Park",		"SamHive3",				2000},
	[EFFECT_SAM_ENTERHIVE]			= {"Park",		"SamHive4",				2000},

	[EFFECT_LOGOBOUNCE]				= {"Title",		"LogoBounce",			400},
	[EFFECT_FLYSWATTER]				= {"Title",		"FlySwatter",			400},
	[EFFECT_LOGOVANISH]				= {"Title",		"LogoVanish",			400},
	[EFFECT_TITLEFLYBUZZ]			= {"Title",		"FlyBuzz",				400},
	[EFFECT_SMACKDOWN]				= {"Title",		"SmackDown",			400},
	[EFFECT_STOMP]					= {"Title",		"Stomp",				400},

	[EFFECT_CLOVERBONUS]			= {"Bonus",		"CloverBonus",			400},
	[EFFECT_MOUSEBONUS]				= {"Bonus",		"MouseBonus",			400},
};

static const AutoRumbleDef kAutoRumbleTable[NUM_EFFECTS] =
{
	[EFFECT_GETPOW]					= {0.0f, 0.7f, 150},
	[EFFECT_BUTTERFLYBOOM]			= {0.7f, 0.0f, 150},
	[EFFECT_SMACK]					= {1.0f, 1.0f, 300},
	[EFFECT_ACORNKICKED]			= {1.0f, 0.0f, 150},
	[EFFECT_FLYGOTKICKED]			= {1.0f, 0.0f, 150},
	[EFFECT_GNOMEGOTKICKED]			= {0.0f, 1.0f, 150},
	[EFFECT_KICKMARBLE]				= {1.0f, 0.0f, 150},
	[EFFECT_SNAPTRAP]				= {1.0f, 0.0f, 200},
	[EFFECT_SQUISHBERRY]			= {1.0f, 0.0f, 200},
	[EFFECT_BALSASHOOT]				= {0.0f, 0.9f, 50},
	[EFFECT_HILLBOOM]				= {0.5f, 0.0f, 750},
	[EFFECT_SKIPGLIDE]				= {0.0f, 0.5f,  60},
	[EFFECT_STOMP]					= {0.0f, 1.0f, 200},
	[EFFECT_LOGOBOUNCE]				= {0.0f, 0.6f, 150},
	[EFFECT_FLYSWATTER]				= {0.0f, 1.0f, 150},
};


/********************* INIT SOUND TOOLS ********************/

void InitSoundTools(void)
{
OSErr			iErr;

	gMaxChannels = 0;
	gMostRecentChannel = -1;

			/* INIT BANK INFO */

	SDL_zeroa(gSndHandles);
	SDL_zeroa(gSndOffsets);

			/******************/
			/* ALLOC CHANNELS */
			/******************/

	for (gMaxChannels = 0; gMaxChannels < MAX_CHANNELS; gMaxChannels++)
	{
			/* NEW SOUND CHANNEL */

		iErr = SndNewChannel(&gSndChannel[gMaxChannels], sampledSynth, initStereo, nil);
		if (iErr)												// if err, stop allocating channels
			break;
	}


		/***********************/
		/* LOAD DEFAULT SOUNDS */
		/***********************/

	LoadSoundBank(SOUND_BANK_MAIN);
}


/******************** SHUTDOWN SOUND ***************************/
//
// Called at Quit time
//

void ShutdownSound(void)
{
			/* STOP ANY PLAYING AUDIO */

	StopAllEffectChannels();
	KillSong();


		/* DISPOSE OF CHANNELS */

	for (int i = 0; i < gMaxChannels; i++)
	{
		SndDisposeChannel(gSndChannel[i], true);
	}
	gMaxChannels = 0;


		/* DISPOSE OF SOUND BANKS */

	for (int i = 0; i < NUM_EFFECTS; i++)
	{
		DisposeSoundEffect(i);
	}
}

#pragma mark -

/******************* LOAD 1 EFFECT ************************/

void LoadSoundEffect(int i)
{
	OSErr iErr;

	GAME_ASSERT(i >= 0);
	GAME_ASSERT(i < NUM_EFFECTS);

	GAME_ASSERT(!gSndHandles[i]);
	GAME_ASSERT(!gSndOffsets[i]);

		/* LOAD SND REZ */

	char path[64];
	SDL_snprintf(path, sizeof(path), ":Audio:%s:%s.aiff", gEffectsTable[i].bank, gEffectsTable[i].name);

	FSSpec aiffSpec;
	iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &aiffSpec);
	GAME_ASSERT_MESSAGE(!iErr, path);

	short fRefNum;
	iErr = FSpOpenDF(&aiffSpec, fsRdPerm, &fRefNum);
	GAME_ASSERT(!iErr);

	gSndHandles[i] = Pomme_SndLoadFileAsResource(fRefNum);
	GAME_ASSERT(gSndHandles[i]);

		/* GET OFFSET INTO IT */

	iErr = GetSoundHeaderOffset(gSndHandles[i], &gSndOffsets[i]);
	GAME_ASSERT(!iErr);

		/* PRE-DECOMPRESS IT */

	Pomme_DecompressSoundResource(&gSndHandles[i], &gSndOffsets[i]);

		/* CLOSE THE FILE */

	FSClose(fRefNum);
}

/******************* LOAD 1 EFFECT ************************/

void DisposeSoundEffect(int i)
{
	GAME_ASSERT(i >= 0);
	GAME_ASSERT(i < NUM_EFFECTS);

	if (gSndHandles[i])
	{
		DisposeHandle((Handle) gSndHandles[i]);
		gSndHandles[i] = NULL;
		gSndOffsets[i] = 0;
	}
}

/******************* LOAD SOUND BANK ************************/

void LoadSoundBank(int bankNum)
{
	StopAllEffectChannels();

	GAME_ASSERT(bankNum >= 0);
	GAME_ASSERT(bankNum < NUM_SOUND_BANKS);

	const char* bankName = kBankNames[bankNum];
	GAME_ASSERT(bankName);

			/****************************/
			/* LOAD ALL EFFECTS IN BANK */
			/****************************/

	for (int i = 0; i < NUM_EFFECTS; i++)
	{
				/* SCAN FOR EFFECTS IN THIS BANK */

		if (0 == SDL_strcmp(bankName, gEffectsTable[i].bank))
		{
			LoadSoundEffect(i);
		}
	}
}


/******************** DISPOSE SOUND BANK **************************/

void DisposeSoundBank(int bankNum)
{
	GAME_ASSERT(bankNum >= 0);
	GAME_ASSERT(bankNum < NUM_SOUND_BANKS);

	const char* bankName = kBankNames[bankNum];
	GAME_ASSERT(bankName);

	StopAllEffectChannels();									// make sure all sounds are stopped before nuking any banks

	for (int i = 0; i < NUM_EFFECTS; i++)
	{
			/* SCAN FOR EFFECTS IN THIS BANK */

		if (0 == SDL_strcmp(bankName, gEffectsTable[i].bank))
		{
			DisposeSoundEffect(i);
		}
	}
}



/********************* STOP A CHANNEL **********************/
//
// Stops the indicated sound channel from playing.
//

void StopAChannel(short *channelNum)
{
SndCommand 	mySndCmd;
short		c = *channelNum;

	if ((c < 0) || (c >= gMaxChannels))		// make sure its a legal #
		return;

	mySndCmd.cmd = flushCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	SndDoImmediate(gSndChannel[c], &mySndCmd);

	mySndCmd.cmd = quietCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	SndDoImmediate(gSndChannel[c], &mySndCmd);

	*channelNum = -1;

	gChannelInfo[c].effectNum = -1;
}


/********************* STOP A CHANNEL IF EFFECT NUM **********************/
//
// Stops the indicated sound channel from playing if it is still this effect #
//

Boolean StopAChannelIfEffectNum(short *channelNum, short effectNum)
{
short		c = *channelNum;

	if (gChannelInfo[c].effectNum != effectNum)		// make sure its the right effect
		return(false);

	StopAChannel(channelNum);

	return(true);
}



/********************* STOP ALL EFFECT CHANNELS **********************/

void StopAllEffectChannels(void)
{
short		i;

	for (i=0; i < gMaxChannels; i++)
	{
		short	c;

		if (i != gSongChannel)							// don't stop the music channel!
		{
			c = i;
			StopAChannel(&c);
		}
	}
}

#pragma mark -

/******************** PLAY SONG ***********************/
//
// if songNum == -1, then play existing open song
//
// INPUT: loopFlag = true if want song to loop
//

void PlaySong(int songEffectNum, Boolean loopFlag)
{
OSErr 	iErr;
int		volume;

const float	volumeTweaks[] =
{
	[EFFECT_SONG_TITLE]		= 1.9,
	[EFFECT_SONG_THEME]		= 1.8,
	[EFFECT_SONG_GARDEN]	= 1.2,
	[EFFECT_SONG_POOL]		= 1.8,
	[EFFECT_SONG_FIDO]		= 1.2,
	[EFFECT_SONG_PLUMBING]	= 1.3,
	[EFFECT_SONG_PLAYROOM]	= 1.4,
	[EFFECT_SONG_CLOSET]	= 1.1,
	[EFFECT_SONG_GARBAGE]	= 1.3,
	[EFFECT_SONG_BALSA]		= 1.4,
	[EFFECT_SONG_PARK]		= 1.3,
	[EFFECT_SONG_BONUS]		= 1.6,
	[EFFECT_SONG_WIN]		= 1.3,
	[EFFECT_SONG_LOSE]		= 1.3,
};

	if (songEffectNum == gCurrentSong)					// see if this is already playing
		return;


		/* ZAP ANY EXISTING SONG */

	KillSong();

			/******************************/
			/* OPEN APPROPRIATE SND FILE */
			/******************************/

	LoadSoundEffect(songEffectNum);



				/*****************/
				/* START PLAYING */
				/*****************/



	gCurrentSong 	= songEffectNum;
	volume = FULL_CHANNEL_VOLUME * volumeTweaks[songEffectNum];
	gSongChannel = PlayEffect_Parms(songEffectNum, volume, volume, NORMAL_CHANNEL_RATE);
	if (gSongChannel < 0)
		return;


				/* SET LOOP FLAG ON STREAM */

	SndCommand loopCommand = {.cmd=pommeSetLoopCmd, .param1=loopFlag? 1: 0};
	iErr = SndDoImmediate(gSndChannel[gSongChannel], &loopCommand);
	GAME_ASSERT(!iErr);


				/* SEE IF WANT TO MUTE THE MUSIC */

	if (!gGamePrefs.music)
	{
		SndPauseFilePlay(gSndChannel[gSongChannel]);			// pause it
	}
}



/*********************** KILL SONG *********************/

void KillSong(void)
{
	if (gCurrentSong < 0)
		return;

	gCurrentSong = -1;

	StopAChannel(&gSongChannel);

	DisposeSoundBank(SOUND_BANK_MUSIC);
}


/******************** TOGGLE MUSIC *********************/

void EnforceMusicPausePref(void)
{
	if (gGamePaused)
		return;

	SCStatus	theStatus;

	SndChannelStatus(gSndChannel[gSongChannel], sizeof(SCStatus), &theStatus);	// get channel info

	if (gGamePrefs.music == theStatus.scChannelPaused)
		SndPauseFilePlay(gSndChannel[gSongChannel]);
}


#pragma mark -

/***************************** PLAY EFFECT 3D ***************************/
//
// NO SSP
//
// OUTPUT: channel # used to play sound
//

short PlayEffect3D(int effectNum, OGLPoint3D *where)
{
short					theChan;
uint32_t				leftVol, rightVol;

	GAME_ASSERT(effectNum >= 0);
	GAME_ASSERT(effectNum < NUM_EFFECTS);

				/* CALC VOLUME */

	Calc3DEffectVolume(effectNum, where, 1.0, &leftVol, &rightVol);
	if ((leftVol+rightVol) == 0)
		return(-1);


	theChan = PlayEffect_Parms(effectNum, leftVol, rightVol, NORMAL_CHANNEL_RATE);

	if (theChan != -1)
		gChannelInfo[theChan].volumeAdjust = 1.0;			// full volume adjust

	return(theChan);									// return channel #
}



/***************************** PLAY EFFECT PARMS 3D ***************************/
//
// Plays an effect with parameters in 3D
//
// OUTPUT: channel # used to play sound
//

short PlayEffect_Parms3D(int effectNum, OGLPoint3D *where, uint32_t rateMultiplier, float volumeAdjust)
{
short			theChan;
uint32_t		leftVol, rightVol;

	GAME_ASSERT(effectNum >= 0);
	GAME_ASSERT(effectNum < NUM_EFFECTS);

				/* CALC VOLUME */

	Calc3DEffectVolume(effectNum, where, volumeAdjust, &leftVol, &rightVol);
	if ((leftVol+rightVol) == 0)
		return(-1);


				/* PLAY EFFECT */

	theChan = PlayEffect_Parms(effectNum, leftVol, rightVol, rateMultiplier);

	if (theChan != -1)
		gChannelInfo[theChan].volumeAdjust = volumeAdjust;	// remember volume adjuster

	return(theChan);									// return channel #
}


/************************* UPDATE 3D SOUND CHANNEL ***********************/
//
// Returns TRUE if effectNum was a mismatch or something went wrong
//

Boolean Update3DSoundChannel(int effectNum, short *channel, OGLPoint3D *where)
{
SCStatus		theStatus;
uint32_t			leftVol,rightVol;
short			c;

	c = *channel;

	if (c == -1)
		return(true);

			/* MAKE SURE THE SAME SOUND IS STILL ON THIS CHANNEL */

	if (effectNum != gChannelInfo[c].effectNum)
	{
		*channel = -1;
		return(true);
	}


			/* SEE IF SOUND HAS COMPLETED */

	SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
	if (!theStatus.scChannelBusy)									// see if channel not busy
	{
		StopAChannel(channel);							// make sure it's really stopped (OS X sound manager bug)
		return(true);
	}

			/* UPDATE THE THING */

	if (where)
	{
		Calc3DEffectVolume(gChannelInfo[c].effectNum, where, gChannelInfo[c].volumeAdjust, &leftVol, &rightVol);
		if ((leftVol+rightVol) == 0)										// if volume goes to 0, then kill channel
		{
			StopAChannel(channel);
			return(false);
		}

		ChangeChannelVolume(c, leftVol, rightVol);
	}
	return(false);
}

/******************** CALC 3D EFFECT VOLUME *********************/

static void Calc3DEffectVolume(int effectNum, OGLPoint3D *where, float volAdjust, uint32_t *leftVolOut, uint32_t *rightVolOut)
{
float	dist;
float	refDist,volumeFactor;
uint32_t	volume,left,right;
uint32_t	maxLeft,maxRight;

	dist 	= OGLPoint3D_Distance(where, &gEarCoords);		// calc dist to sound for pane 0

			/* DO VOLUME CALCS */

	refDist = gEffectsTable[effectNum].refDistance;			// get ref dist

	dist -= refDist;
	if (dist <= EPS)
		volumeFactor = 1.0f;
	else
	{
		volumeFactor = 1.0f / (dist * VOLUME_DISTANCE_FACTOR);
		if (volumeFactor > 1.0f)
			volumeFactor = 1.0f;
	}

	volume = (float)FULL_CHANNEL_VOLUME * volumeFactor * volAdjust;


	if (volume < 6)							// if really quiet, then just turn it off
	{
		*leftVolOut = *rightVolOut = 0;
		return;
	}

			/************************/
			/* DO STEREO SEPARATION */
			/************************/

	else
	{
		float		volF = (float)volume;
		OGLVector2D	earToSound,lookVec;
		float		dot,cross;

		maxLeft = maxRight = 0;

			/* CALC VECTOR TO SOUND */

		earToSound.x = where->x - gEarCoords.x;
		earToSound.y = where->z - gEarCoords.z;
		FastNormalizeVector2D(earToSound.x, earToSound.y, &earToSound, true);


			/* CALC EYE LOOK VECTOR */

		FastNormalizeVector2D(gEyeVector.x, gEyeVector.z, &lookVec, true);


			/* DOT PRODUCT  TELLS US HOW MUCH STEREO SHIFT */

		dot = 1.0f - fabs(OGLVector2D_Dot(&earToSound,  &lookVec));
		if (dot < 0.0f)
			dot = 0.0f;
		else
		if (dot > 1.0f)
			dot = 1.0f;


			/* CROSS PRODUCT TELLS US WHICH SIDE */

		cross = OGLVector2D_Cross(&earToSound,  &lookVec);


				/* DO LEFT/RIGHT CALC */

		if (cross > 0.0f)
		{
			left 	= volF + (volF * dot);
			right 	= volF - (volF * dot);
		}
		else
		{
			right 	= volF + (volF * dot);
			left 	= volF - (volF * dot);
		}


				/* KEEP MAX */

		if (left > maxLeft)
			maxLeft = left;
		if (right > maxRight)
			maxRight = right;

	}

	*leftVolOut = maxLeft;
	*rightVolOut = maxRight;
}



#pragma mark -

/******************* UPDATE LISTENER LOCATION ******************/
//
// Get ear coord for all local players
//

void UpdateListenerLocation(void)
{
OGLVector3D	v;

	v.x = gGameView.cameraPlacement.pointOfInterest.x - gGameView.cameraPlacement.cameraLocation.x;	// calc line of sight vector
	v.y = gGameView.cameraPlacement.pointOfInterest.y - gGameView.cameraPlacement.cameraLocation.y;
	v.z = gGameView.cameraPlacement.pointOfInterest.z - gGameView.cameraPlacement.cameraLocation.z;
	FastNormalizeVector(v.x, v.y, v.z, &v);

	gEarCoords.x = gGameView.cameraPlacement.cameraLocation.x + (v.x * 300.0f);			// put ear coord in front of camera
	gEarCoords.y = gGameView.cameraPlacement.cameraLocation.y + (v.y * 300.0f);
	gEarCoords.z = gGameView.cameraPlacement.cameraLocation.z + (v.z * 300.0f);

	gEyeVector = v;
}


/***************************** PLAY EFFECT ***************************/
//
// OUTPUT: channel # used to play sound
//

short PlayEffect(int effectNum)
{
	return(PlayEffect_Parms(effectNum,FULL_CHANNEL_VOLUME,FULL_CHANNEL_VOLUME,NORMAL_CHANNEL_RATE));

}

/***************************** PLAY EFFECT PARMS ***************************/
//
// Plays an effect with parameters
//
// OUTPUT: channel # used to play sound
//

short  PlayEffect_Parms(int effectNum, uint32_t leftVolume, uint32_t rightVolume, unsigned long rateMultiplier)
{
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
short			theChan;
OSErr			myErr;
uint32_t		lv2,rv2;

	GAME_ASSERT(effectNum >= 0);
	GAME_ASSERT(effectNum < NUM_EFFECTS);
	GAME_ASSERT_MESSAGE(gSndHandles[effectNum], "Sound effect not loaded");

			/* LOOK FOR FREE CHANNEL */

	theChan = FindSilentChannel();
	if (theChan == -1)
	{
		return(-1);
	}

	lv2 = (float)leftVolume * gGlobalVolume;							// amplify by global volume
	rv2 = (float)rightVolume * gGlobalVolume;


					/* GET IT GOING */

	chanPtr = gSndChannel[theChan];

	mySndCmd.cmd = flushCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = quietCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = volumeCmd;										// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);


	mySndCmd.cmd = bufferCmd;										// make it play
	mySndCmd.param1 = 0;
	mySndCmd.ptr = (Ptr)(*gSndHandles[effectNum]) + gSndOffsets[effectNum];	// pointer to SoundHeader
    SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd 		= rateMultiplierCmd;						// modify the rate to change the frequency
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= rateMultiplier;
	SndDoImmediate(chanPtr, &mySndCmd);


			/* SET MY INFO */

	gChannelInfo[theChan].effectNum 	= effectNum;		// remember what effect is playing on this channel
	gChannelInfo[theChan].leftVolume 	= leftVolume;		// remember requested volume (not the adjusted volume!)
	gChannelInfo[theChan].rightVolume 	= rightVolume;
	return(theChan);										// return channel #
}


#pragma mark -


/****************** UPDATE GLOBAL VOLUME ************************/
//
// Call this whenever gGlobalVolume is changed.  This will update
// all of the sounds with the correct volume.
//

static void UpdateGlobalVolume(void)
{
			/* ADJUST VOLUMES OF ALL CHANNELS REGARDLESS IF THEY ARE PLAYING OR NOT */

	for (int c = 0; c < gMaxChannels; c++)
	{
		ChangeChannelVolume(c, gChannelInfo[c].leftVolume, gChannelInfo[c].rightVolume);
	}
}

/*************** CHANGE CHANNEL VOLUME **************/
//
// Modifies the volume of a currently playing channel
//

void ChangeChannelVolume(short channel, float leftVol, float rightVol)
{
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
uint32_t			lv2,rv2;

	if (channel < 0)									// make sure it's valid
		return;

	lv2 = leftVol * gGlobalVolume;				// amplify by global volume
	rv2 = rightVol * gGlobalVolume;

	chanPtr = gSndChannel[channel];						// get the actual channel ptr

	mySndCmd.cmd = volumeCmd;							// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;			// set volume left & right
	SndDoImmediate(chanPtr, &mySndCmd);

	gChannelInfo[channel].leftVolume = leftVol;				// remember requested volume (not the adjusted volume!)
	gChannelInfo[channel].rightVolume = rightVol;
}



/*************** CHANGE CHANNEL RATE **************/
//
// Modifies the frequency of a currently playing channel
//
// The Input Freq is a fixed-point multiplier, not the static rate via rateCmd.
// This function uses rateMultiplierCmd, so a value of 0x00020000 is x2.0
//

void ChangeChannelRate(short channel, long rateMult)
{
static	SndCommand 		mySndCmd;
SndChannelPtr			chanPtr;

	if (channel < 0)									// make sure it's valid
		return;

	chanPtr = gSndChannel[channel];						// get the actual channel ptr

	mySndCmd.cmd 		= rateMultiplierCmd;						// modify the rate to change the frequency
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= rateMult;
	SndDoImmediate(chanPtr, &mySndCmd);
}




/******************** FIND SILENT CHANNEL *************************/

static short FindSilentChannel(void)
{
short		theChan, startChan;
OSErr		myErr;
SCStatus	theStatus;

	theChan = gMostRecentChannel + 1;					// start on channel after the most recently acquired one - assuming it has the best chance of being silent
	if (theChan >= gMaxChannels)
		theChan = 0;
	startChan = theChan;

	do
	{
		myErr = SndChannelStatus(gSndChannel[theChan],sizeof(SCStatus),&theStatus);	// get channel info
		if (myErr)
			goto next;

		if (theStatus.scChannelBusy)					// see if channel busy
			goto next;

		gMostRecentChannel = theChan;
		return(theChan);

next:
		theChan++;										// try next channel
		if (theChan >= gMaxChannels)
			theChan = 0;

	}while(theChan != startChan);

			/* NO FREE CHANNELS */

	return(-1);
}


/********************** IS EFFECT CHANNEL PLAYING ********************/

Boolean IsEffectChannelPlaying(short chanNum)
{
SCStatus	theStatus;

	SndChannelStatus(gSndChannel[chanNum],sizeof(SCStatus),&theStatus);	// get channel info
	return (theStatus.scChannelBusy);
}


/*************** PAUSE ALL SOUND CHANNELS **************/

void PauseAllChannels(Boolean pause)
{
	SndCommand cmd = { .cmd = pause ? pommePausePlaybackCmd : pommeResumePlaybackCmd };

	for (int c = 0; c < gMaxChannels; c++)
	{
		SndDoImmediate(gSndChannel[c], &cmd);
	}

//	SndDoImmediate(gMusicChannel, &cmd);
}


#pragma mark -

/********************** PLAY RUMBLE EFFECT ***************************/

void PlayRumbleEffect(int effectNum)
{
	if (effectNum < 0 || effectNum >= NUM_EFFECTS)
		return;

	const AutoRumbleDef* rumbleEffect = &kAutoRumbleTable[effectNum];

	if (rumbleEffect->duration > 0)
	{
		Rumble(rumbleEffect->lowFrequencyStrength, rumbleEffect->highFrequencyStrength, rumbleEffect->duration);
	}
}
