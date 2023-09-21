/****************************/
/*     SOUND ROUTINES       */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	FSSpec				gDataSpec;
extern	float		gFramesPerSecondFrac;
extern	PrefsType			gGamePrefs;


/****************************/
/*    PROTOTYPES            */
/****************************/

static short FindSilentChannel(void);
static void Calc3DEffectVolume(short effectNum, OGLPoint3D *where, float volAdjust, u_long *leftVolOut, u_long *rightVolOut);
static void UpdateGlobalVolume(void);


/****************************/
/*    CONSTANTS             */
/****************************/


#define FloatToFixed16(a)      ((Fixed)((float)(a) * 0x000100L))		// convert float to 16bit fixed pt


#define		MAX_CHANNELS			40

#define		MAX_EFFECTS				70


typedef struct
{
	Byte	bank,sound;
	long	refDistance;
}EffectType;



#define	VOLUME_DISTANCE_FACTOR	.004f		// bigger == sound decays FASTER with dist, smaller = louder far away

/**********************/
/*     VARIABLES      */
/**********************/

float						gGlobalVolume = .4;

static OGLPoint3D			gEarCoords;										// coord of camera plus a tad to get pt in front of camera
static	OGLVector3D			gEyeVector;


static	SndListHandle		gSndHandles[MAX_SOUND_BANKS][MAX_EFFECTS];		// handles to ALL sounds
static  long				gSndOffsets[MAX_SOUND_BANKS][MAX_EFFECTS];

static	SndChannelPtr		gSndChannel[MAX_CHANNELS];
ChannelInfoType				gChannelInfo[MAX_CHANNELS];

static short				gMaxChannels = 0;

static short				gMostRecentChannel = -1;

static short				gNumSndsInBank[MAX_SOUND_BANKS] = {0,0,0};


Boolean						gSongPlayingFlag = false;
Boolean						gLoopSongFlag = true;
Boolean						gAllowAudioKeys = true;

int							gNumLoopingEffects;

Boolean				gMuteMusicFlag = false;
short				gCurrentSong = -1;
short				gSongChannel = -1;


		/*****************/
		/* EFFECTS TABLE */
		/*****************/

static EffectType	gEffectsTable[] =
{
	SOUND_BANK_SONG,0,2000,									// EFFECT_SONG

	SOUND_BANK_MAIN,SOUND_DEFAULT_CHANGESELECT,2000,		// EFFECT_CHANGESELECT
	SOUND_BANK_MAIN,SOUND_DEFAULT_JUMP,500,					// EFFECT_JUMP
	SOUND_BANK_MAIN,SOUND_DEFAULT_SKIPGLIDE,1000,			// EFFECT_SKIPGLIDE
	SOUND_BANK_MAIN,SOUND_DEFAULT_SKIPKICK,500,				// EFFECT_SKIPKICK
	SOUND_BANK_MAIN,SOUND_DEFAULT_ACORNKICKED,300,			// EFFECT_ACORNKICKED
	SOUND_BANK_MAIN,SOUND_DEFAULT_SKIPLAND,300,				// EFFECT_SKIPLAND
	SOUND_BANK_MAIN,SOUND_DEFAULT_FLYGOTKICKED,500,			// EFFECT_FLYGOTKICKED
	SOUND_BANK_MAIN,SOUND_DEFAULT_GETPOT,500,				// EFFECT_GETPOW
	SOUND_BANK_MAIN,SOUND_DEFAULT_BUTTERFLYBOOM,100,		// EFFECT_BUTTERFLYBOOM
	SOUND_BANK_MAIN,SOUND_DEFAULT_FLYWALKBUZZ,200,			// EFFECT_FLYWALKBUZZ
	SOUND_BANK_MAIN,SOUND_DEFAULT_SMACK,500,				// EFFECT_SMACK
	SOUND_BANK_MAIN,SOUND_DEFAULT_SPLASH,500,				// EFFECT_SPLASH
	SOUND_BANK_MAIN,SOUND_DEFAULT_BUDDYLAUNCH,700,			// EFFECT_BUDDYLAUNCH
	SOUND_BANK_MAIN,SOUND_DEFAULT_BUDDYBUZZ,700,			// EFFECT_BUDDYBUZZ
	SOUND_BANK_MAIN,SOUND_DEFAULT_CHIP_CHECKPOINT,600,		// EFFECT_CHIP_CHECKPOINT
	SOUND_BANK_MAIN,SOUND_DEFAULT_DOORCREAK,800,			// EFFECT_DOORCREAK
	SOUND_BANK_MAIN,SOUND_DEFAULT_THROWBOTTLECAP,500,		// EFFECT_THROWBOTTLECAP
	SOUND_BANK_MAIN,SOUND_DEFAULT_BOTTLECAPBOUNCE,10,		// EFFECT_BOTTLECAPBOUNCE
	SOUND_BANK_MAIN,SOUND_DEFAULT_MOUSETRAP,1000,			// EFFECT_MOUSETRAP
	SOUND_BANK_MAIN,SOUND_DEFAULT_BUDDYBOOM,300,			// EFFECT_BUDDYBOOM
	SOUND_BANK_MAIN,SOUND_DEFAULT_CHIP_MAP4ACORN,600,		// EFFECT_CHIP_MAP4ACORN
	SOUND_BANK_MAIN,SOUND_DEFAULT_SHIELD,300,				// EFFECT_SHIELD
	SOUND_BANK_MAIN,SOUND_DEFAULT_FIRECRACKER,600,			// EFFECT_FIRECRACKER
	SOUND_BANK_MAIN,SOUND_DEFAULT_BUMBLERUMBLE,100,			// EFFECT_BUMBLERUMBLE
	SOUND_BANK_MAIN,SOUND_DEFAULT_CHIP_CHECKPOINTDONE,100,	// EFFECT_CHIP_CHECKPOINTDONE
	SOUND_BANK_MAIN,SOUND_DEFAULT_CHIP_THANKS,100,			// EFFECT_CHIP_THANKS
	SOUND_BANK_MAIN,SOUND_DEFAULT_GRENADEBOOM,500,			// EFFECT_GRENADEBOOM
	SOUND_BANK_MAIN,SOUND_DEFAULT_PLANECRASH,6000,			// EFFECT_PLANECRASH
	SOUND_BANK_MAIN,SOUND_DEFAULT_DRAGONFLYBUZZ,2000,		// EFFECT_DRAGONFLYBUZZ
	SOUND_BANK_MAIN,SOUND_DEFAULT_BOTTLECRACK,700,			// EFFECT_BOTTLECRACK
	SOUND_BANK_MAIN,SOUND_DEFAULT_BOTTLESHATTER,700,		// EFFECT_BOTTLESHATTER
	SOUND_BANK_MAIN,SOUND_DEFAULT_PULLTRAP,1000,			// EFFECT_PULLTRAP
	SOUND_BANK_MAIN,SOUND_DEFAULT_SNAPTRAP,1000,			// EFFECT_SNAPTRAP
	SOUND_BANK_MAIN,SOUND_DEFAULT_POPACORN,100,				// EFFECT_POPACORN
	SOUND_BANK_MAIN,SOUND_DEFAULT_FOOTSTEP,100,				// EFFECT_FOOTSTEP
	SOUND_BANK_MAIN,SOUND_DEFAULT_GRENADETHROW,400,			// EFFECT_GRENADETHROW

	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_GNOMESTEP,500,			// EFFECT_GNOMESTEP
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SPRINKLER,300,			// EFFECT_SPRINKLER
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_GNOMEGOTKICKED,400,		// EFFECT_GNOMEGOTKICKED
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SQUISHBERRY,900,			// EFFECT_SQUISHBERRY
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_CHIP_STUCKMOUSE,600,		// EFFECT_CHIP_STUCKMOUSE
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_EVILPLANTSHOOT,700,		// EFFECT_EVILPLANTSHOOT
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SAM_FREEMICE,600,			// EFFECT_SAM_FREEMICE
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SAM_FREEMICE2,600,		// EFFECT_SAM_FREEMICE2
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SAM_FINDSHELL,600,		// EFFECT_SAM_FINDSHELL
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SAM_GOTSHELL,600,			// EFFECT_SAM_GOTSHELL
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SAM_FINDHEAD,600,			// EFFECT_SAM_FINDHEAD
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SAM_PUTHEADON,600,		// EFFECT_SAM_PUTHEADON
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SAM_FIXEDSCARECROW,600,	// EFFECT_SAM_FIXEDSCARECROW
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SAM_SQUASHBERRIES,600,	// EFFECT_SAM_SQUASHBERRIES
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SAM_SQUISHMORE,600,		// EFFECT_SAM_SQUISHMORE
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SAM_SQUISHDONE,600,		// EFFECT_SAM_SQUISHDONE
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SAM_GOTMICE,600,			// EFFECT_SAM_GOTMICE
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SAM_POOLKEY,600,			// EFFECT_SAM_POOLKEY
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARDEN_SAM_FIDO,600,				// EFFECT_SAM_FIDO

	SOUND_BANK_LEVELSPECIFIC,SOUND_FIDO_GOTFLEAS,700,				// EFFECT_GOTFLEAS
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIDO_GOTTICKS,700,				// EFFECT_GOTTICKS
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIDO_HAPPYDOG,700,				// EFFECT_HAPPYDOG
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIDO_REMEMBER,700,				// EFFECT_REMEMBERDOG
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIDO_TICKSUCK,700,				// EFFECT_TICKSUCK
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIDO_TICKSPIT,400,				// EFFECT_TICKSPIT
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIDO_TICKSTEP,400,				// EFFECT_TICKSTEP
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIDO_TICKDIE,500,				// EFFECT_TICKDIE
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIDO_BONEHIT,500,				// EFFECT_BONEHIT

	SOUND_BANK_LEVELSPECIFIC,SOUND_PLUMBING_INTRO,700,				// EFFECT_PLUMBINGINTRO
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLUMBING_SLUDGEHIT,700,			// EFFECT_SLUDGEHIT
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLUMBING_NAILHIT,700,			// EFFECT_NAILHIT
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLUMBING_WATER,1000,				// EFFECT_TUNNELWATER
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLUMBING_METALSCRAPE,1000,		// EFFECT_METALSCRAPE
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLUMBING_PINECONE,1000,			// EFFECT_HITPINECONE
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLUMBING_LEAF,1000,				// EFFECT_HITLEAF

	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_LASERBOOM,400,			// EFFECT_LASERBOOM
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_SAM_FINDMARBLE,600,		// EFFECT_SAM_FINDMARBLE
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_SAM_KICKMARBLE,600,		// EFFECT_SAM_KICKMARBLE
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_SAM_PINSDOWN,600,		// EFFECT_SAM_PINSDOWN
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_BOWLINGHIT,500,			// EFFECT_BOWLINGHIT
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_KICKMARBLE,900,			// EFFECT_KICKMARBLE
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_OTTOMOTOR,10,			// EFFECT_OTTOMOTOR
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_CHIP_DORACE,610,		// EFFECT_CHIP_DORACE
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_CHIP_LOSTRACE,1500,		// EFFECT_CHIP_LOSTRACE
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_CHIP_WINNING,1500,		// EFFECT_CHIP_WINNING
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_CHIP_YOUWON,1500,		// EFFECT_CHIP_YOUWON
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_CHIP_SAMWINNING,1500,	// EFFECT_CHIP_SAMWINNING
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_SAM_DOPUZZLE,610,		// EFFECT_SAM_DOPUZZLE
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_SAM_PUZZLEDONE,610,		// EFFECT_SAM_PUZZLEDONE
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_OTTOSHOOT,400,			// EFFECT_OTTOSHOOT
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_SLOTCAR,400,			// EFFECT_SLOTCAR
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_OTTOFALL,1000,			// EFFECT_OTTOFALL
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_CHIP_READY,1500,		// EFFECT_CHIP_READY
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_CHIP_SET,1500,			// EFFECT_CHIP_SET
	SOUND_BANK_LEVELSPECIFIC,SOUND_PLAYROOM_CHIP_GO,1500,			// EFFECT_CHIP_GO

	SOUND_BANK_LEVELSPECIFIC,SOUND_BALSA_PROPELLER,400,				// EFFECT_PROPELLER
	SOUND_BANK_LEVELSPECIFIC,SOUND_BALSA_HILLBOOM,4000,				// EFFECT_HILLBOOM
	SOUND_BANK_LEVELSPECIFIC,SOUND_BALSA_BOMBBOOM,3000,				// EFFECT_BOMBBOOM
	SOUND_BANK_LEVELSPECIFIC,SOUND_BALSA_PLANEHIT,1500,				// EFFECT_PLANEHIT
	SOUND_BANK_LEVELSPECIFIC,SOUND_BALSA_FROGJUMP,6000,				// EFFECT_FROGJUMP
	SOUND_BANK_LEVELSPECIFIC,SOUND_BALSA_DIVEBOMB,6000,				// EFFECT_DIVEBOMB
	SOUND_BANK_LEVELSPECIFIC,SOUND_BALSA_BOMBFALL,6000,				// EFFECT_BOMBFALL
	SOUND_BANK_LEVELSPECIFIC,SOUND_BALSA_SHOOT,2000,				// EFFECT_BALSASHOOT
	SOUND_BANK_LEVELSPECIFIC,SOUND_DRAGONFLYHIT,2000,				// EFFECT_DRAGONFLYHIT
	SOUND_BANK_LEVELSPECIFIC,SOUND_SAM_DESTROYHILLS,2000,			// EFFECT_SAM_DESTROYHILLS
	SOUND_BANK_LEVELSPECIFIC,SOUND_SAM_HILLSDESTROYED,2000,			// EFFECT_SAM_HILLSDESTROYED
	SOUND_BANK_LEVELSPECIFIC,SOUND_SAM_HOWTOBOMB,2000,				// EFFECT_SAM_HOWTOBOMB

	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOSET_SERVO,10,					// EFFECT_SERVO
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOSET_SERVO2,300,				// EFFECT_SERVO2
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOSET_MINEBOOM,500,				// EFFECT_MINEBOOM
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOSET_CHIPCLICK,500,			// EFFECT_CHIPCLICK
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOSET_SILICONDOOROPEN,500,		// EFFECT_SILICONDOOROPEN
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOSET_SAM_GETREDCLOVERS,2000,	// EFFECT_SAM_GETREDCLOVERS
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOSET_SAM_GOTREDCLOVERS,2000,	// EFFECT_SAM_GOTREDCLOVERS
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOSET_VACUUME,1500,				// EFFECT_VACUUME
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOSET_VACUUMECRUNCH,3000,		// EFFECT_VACUUMECRUMCH
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOSET_MOTHFLAP,400,				// EFFECT_MOTHFLAP
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOSET_SAM_MOTHBALLS,2000,		// EFFECT_SAM_MOTHBALLS
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOSET_SAM_COMPUTERDOOR,2000,	// EFFECT_SAM_COMPUTERDOOR

	SOUND_BANK_LEVELSPECIFIC,SOUND_GARBAGE_CANOPEN,2000,			// EFFECT_CANOPEN
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARBAGE_SODASPRAY,10,			// EFFECT_SODASPRAY
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARBAGE_SAM_GUTTER,2000,			// EFFECT_SAM_GUTTERWATER
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARBAGE_SAM_FREED,2000,			// EFFECT_SAM_FREED
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARBAGE_SAM_GLIDER,2000,			// EFFECT_SAM_GLIDER
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARBAGE_SAM_SODA,2000,			// EFFECT_SAM_SODA
	SOUND_BANK_LEVELSPECIFIC,SOUND_GARBAGE_PROP,1000,				// EFFECT_PROP2

	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_FROGJUMP,600,				// EFFECT_FROGJUMP2
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_TONGUEHIT,1000,				// EFFECT_TONGUEHIT
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_TONGUESWOOSH,500,			// EFFECT_TONGUESWOOSH
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_SAM_CATCHFISH,2000,			// EFFECT_SAM_CATCHFISH
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_SAM_ANGLER,2000,			// EFFECT_SAM_ANGLER
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_SAM_GETFOOD,2000,			// EFFECT_SAM_GETFOOD
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_SAM_GOTFOOD,2000,			// EFFECT_SAM_GOTFOOD
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_ANTBITE,200,				// EFFECT_ANTBITE
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_FISHFLOP,200,				// EFFECT_FISHFLOP
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_SAM_ENTERHIVE,2000,			// EFFECT_SAM_ENTERHIVE
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_SAM_KEEPFISHING,2000,		// EFFECT_SAM_KEEPFISHING
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_SAM_MOREFOOD,2000,			// EFFECT_SAM_MOREFOOD
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_SAM_GETKINDLING,2000,		// EFFECT_SAM_GETKINDLING
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_SAM_MOREKINDLING,2000,		// EFFECT_SAM_MOREKINDLING
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_SAM_SPARK,2000,				// EFFECT_SAM_SPARK
	SOUND_BANK_LEVELSPECIFIC,SOUND_PARK_SAM_BOTTLEKEY,2000,			// EFFECT_SAM_BOTTLEKEY

	SOUND_BANK_TITLE,SOUND_TITLE_LOGOBOUNCE,400,			// EFFECT_LOGOBOUNCE
	SOUND_BANK_TITLE,SOUND_TITLE_FLYSWATTER,400,			// EFFECT_FLYSWATTER
	SOUND_BANK_TITLE,SOUND_TITLE_LOGOVANISH,400,			// EFFECT_LOGOVANISH
	SOUND_BANK_TITLE,SOUND_TITLE_FLYBUZZ,400,				// EFFECT_TITLEFLYBUZZ
	SOUND_BANK_TITLE,SOUND_TITLE_SMACKDOWN,400,				// EFFECT_SMACKDOWN
	SOUND_BANK_TITLE,SOUND_TITLE_STOMP,400,					// EFFECT_STOMP

	SOUND_BANK_BONUS,SOUND_BONUS_CLOVERBONUS,400,			// EFFECT_CLOVERBONUS
	SOUND_BANK_BONUS,SOUND_BONUS_MOUSEBONUS,400,			// EFFECT_MOUSEBONUS
};


/********************* INIT SOUND TOOLS ********************/

void InitSoundTools(void)
{
	IMPLEMENT_ME();
#if 0
OSErr			iErr;
short			i;
ExtSoundHeader	sndHdr;
const double	crap = rate44khz;
FSSpec			spec;

	gNumLoopingEffects = 0;

	gMaxChannels = 0;
	gMostRecentChannel = -1;

			/* INIT BANK INFO */

	for (i = 0; i < MAX_SOUND_BANKS; i++)
		gNumSndsInBank[i] = 0;

			/******************/
			/* ALLOC CHANNELS */
			/******************/

				/* MAKE DUMMY SOUND HEADER */

	sndHdr.samplePtr 		= nil;
    sndHdr.sampleRate		= rate44khz;
    sndHdr.loopStart		= 0;
    sndHdr.loopEnd			= 0;
    sndHdr.encode			= extSH;
    sndHdr.baseFrequency 	= 0;
    sndHdr.numFrames		= 0;
    sndHdr.numChannels		= 2;
   	dtox80(&crap, &sndHdr.AIFFSampleRate);
    sndHdr.markerChunk		= 0;
    sndHdr.instrumentChunks	= 0;
    sndHdr.AESRecording		= 0;
    sndHdr.sampleSize		= 16;
    sndHdr.futureUse1		= 0;
    sndHdr.futureUse2		= 0;
    sndHdr.futureUse3		= 0;
    sndHdr.futureUse4		= 0;
    sndHdr.sampleArea[0]		= 0;

	for (gMaxChannels = 0; gMaxChannels < MAX_CHANNELS; gMaxChannels++)
	{
			/* NEW SOUND CHANNEL */

		SndCommand 		mySndCmd;

		iErr = SndNewChannel(&gSndChannel[gMaxChannels],sampledSynth,initMono+initNoInterp,NewSndCallBackUPP(CallBackFn));
		if (iErr)												// if err, stop allocating channels
			break;

		gChannelInfo[gMaxChannels].isLooping = false;


			/* FOR POST- SM 3.6.5 DO THIS! */

		mySndCmd.cmd = soundCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = (long)&sndHdr;
		if ((iErr = SndDoImmediate(gSndChannel[gMaxChannels], &mySndCmd)) != noErr)
		{
			DoAlert("InitSoundTools: SndDoImmediate failed! %d", iErr);
		}


		mySndCmd.cmd = reInitCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = initNoInterp|initStereo;
		if ((iErr = SndDoImmediate(gSndChannel[gMaxChannels], &mySndCmd)) != noErr)
		{
			DoAlert("InitSoundTools: SndDoImmediate failed 2! %d", iErr);
		}
	}


		/***********************/
		/* LOAD DEFAULT SOUNDS */
		/***********************/

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Main.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_MAIN);
#endif
}


/******************** SHUTDOWN SOUND ***************************/
//
// Called at Quit time
//

void ShutdownSound(void)
{
int	i;

			/* STOP ANY PLAYING AUDIO */

	StopAllEffectChannels();
	KillSong();


		/* DISPOSE OF CHANNELS */

	for (i = 0; i < gMaxChannels; i++)
		SndDisposeChannel(gSndChannel[i], true);
	gMaxChannels = 0;


}

#pragma mark -

/******************* LOAD SOUND BANK ************************/

void LoadSoundBank(FSSpec *spec, long bankNum)
{
short			srcFile1,numSoundsInBank,i;
OSErr			iErr;

	StopAllEffectChannels();

	if (bankNum >= MAX_SOUND_BANKS)
		DoFatalAlert("LoadSoundBank: bankNum >= MAX_SOUND_BANKS");

			/* DISPOSE OF EXISTING BANK */

	DisposeSoundBank(bankNum);


			/* OPEN APPROPRIATE REZ FILE */

	srcFile1 = FSpOpenResFile(spec, fsCurPerm);
	if (srcFile1 == -1)
	{
		DoFatalAlert("LoadSoundBank: OpenResFile failed! %d", ResError());
	}

			/****************************/
			/* LOAD ALL EFFECTS IN BANK */
			/****************************/

	UseResFile( srcFile1 );												// open sound resource fork
	numSoundsInBank = Count1Resources('snd ');							// count # snd's in this bank
	if (numSoundsInBank > MAX_EFFECTS)
		DoFatalAlert("LoadSoundBank: numSoundsInBank > MAX_EFFECTS");

	for (i=0; i < numSoundsInBank; i++)
	{
				/* LOAD SND REZ */

		gSndHandles[bankNum][i] = (SndListResource **)GetResource('snd ',BASE_EFFECT_RESOURCE+i);
		if (gSndHandles[bankNum][i] == nil)
		{
			iErr = ResError();
			DoFatalAlert("LoadSoundBank: GetResource failed! %d", iErr);
		}
		DetachResource((Handle)gSndHandles[bankNum][i]);				// detach resource from rez file & make a normal Handle

		HNoPurge((Handle)gSndHandles[bankNum][i]);						// make non-purgeable
		HLockHi((Handle)gSndHandles[bankNum][i]);

				/* GET OFFSET INTO IT */

		GetSoundHeaderOffset(gSndHandles[bankNum][i], &gSndOffsets[bankNum][i]);
	}

	CloseResFile(srcFile1);

	gNumSndsInBank[bankNum] = numSoundsInBank;					// remember how many sounds we've got
}


/******************** DISPOSE SOUND BANK **************************/

void DisposeSoundBank(short bankNum)
{
short	i;


	if (bankNum > MAX_SOUND_BANKS)
		return;

	StopAllEffectChannels();									// make sure all sounds are stopped before nuking any banks

			/* FREE ALL SAMPLES */

	for (i=0; i < gNumSndsInBank[bankNum]; i++)
		DisposeHandle((Handle)gSndHandles[bankNum][i]);


	gNumSndsInBank[bankNum] = 0;
}



/********************* STOP A CHANNEL **********************/
//
// Stops the indicated sound channel from playing.
//

void StopAChannel(short *channelNum)
{
SndCommand 	mySndCmd;
OSErr 		myErr;
short		c = *channelNum;

	if ((c < 0) || (c >= gMaxChannels))		// make sure its a legal #
		return;

	mySndCmd.cmd = flushCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);

	mySndCmd.cmd = quietCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);

	*channelNum = -1;

	if (gChannelInfo[c].isLooping)
	{
		gNumLoopingEffects--;
		gChannelInfo[c].isLooping = false;
	}

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


/****************** WAIT EFFECTS SILENT *********************/

void WaitEffectsSilent(void)
{
short	i;
Boolean	isBusy;
SCStatus				theStatus;

	do
	{
		isBusy = 0;
		for (i=0; i < gMaxChannels; i++)
		{
			SndChannelStatus(gSndChannel[i],sizeof(SCStatus),&theStatus);	// get channel info
			isBusy |= theStatus.scChannelBusy;
		}
	}while(isBusy);
}

#pragma mark -

/******************** PLAY SONG ***********************/
//
// if songNum == -1, then play existing open song
//
// INPUT: loopFlag = true if want song to loop
//

void PlaySong(short songNum, Boolean loopFlag)
{
OSErr 	iErr;
FSSpec	spec;
int		volume;

Str32	songNames[] =
{
	":Audio:Intro.song",
	":Audio:Theme.song",
	":Audio:Level1_Garden.song",
	":Audio:Level2_Pool.song",
	":Audio:Level3_DogHouse.song",
	":Audio:Level4_Plumbing.song",
	":Audio:Level5_Playroom.song",
	":Audio:Level6_Closet.song",
	":Audio:Level8_Garbage.song",
	":Audio:Level9_Balsa.song",
	":Audio:Level10_Park.song",
	":Audio:Bonus.song",
	":Audio:Win.song",
	":Audio:Lose.song",
};

float	volumeTweaks[]=
{
	1.9,				// intro
	1.8,				// theme
	1.2,				// level 1 garden
	1.8,				// level 2 pool
	1.2,				// level 3 fido
	1.3,				// level 4 plumbing
	1.4,				// level 5 playroom
	1.1,				// level 6 closet
	1.3,				// level 8 garbage
	1.4,				// level 9 balsa
	1.3,				// level 10 park
	1.6,				// bonus
	1.3,				// win
	1.3,				// lose
};

	if (songNum == gCurrentSong)					// see if this is already playing
		return;


		/* ZAP ANY EXISTING SONG */

	KillSong();

			/******************************/
			/* OPEN APPROPRIATE SND FILE */
			/******************************/

	iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID,songNames[songNum], &spec);
	if (iErr)
		DoFatalAlert("PlaySong: song file not found");




				/*****************/
				/* START PLAYING */
				/*****************/

	LoadSoundBank(&spec, SOUND_BANK_SONG);								// load snd resource into song bank


	gCurrentSong 	= songNum;
	gLoopSongFlag 	= loopFlag;
	volume = FULL_CHANNEL_VOLUME * volumeTweaks[songNum];
	gSongChannel = PlayEffect_Parms(EFFECT_SONG, volume, volume, NORMAL_CHANNEL_RATE);

	gSongPlayingFlag = true;

			/* SEE IF WANT TO MUTE THE MUSIC */

	if (gMuteMusicFlag)
	{
		gMuteMusicFlag = false;			// set not muted
		ToggleMusic();					// and then mute it
	}
}



/*********************** KILL SONG *********************/

void KillSong(void)
{

	gCurrentSong = -1;

	if (!gSongPlayingFlag)
		return;

	gSongPlayingFlag = false;											// tell callback to do nothing

	StopAChannel(&gSongChannel);

	DisposeSoundBank(SOUND_BANK_SONG);

}

/******************** TOGGLE MUSIC *********************/

void ToggleMusic(void)
{
SndCommand 		mySndCmd;

	gMuteMusicFlag = !gMuteMusicFlag;

	if ((gSongChannel < 0) || (gSongChannel >= gMaxChannels))		// make sure its a legal #
		return;

	if (gMuteMusicFlag)
	{
		mySndCmd.cmd = quietCmd;									// stop it
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		SndDoImmediate(gSndChannel[gSongChannel], &mySndCmd);
	}
	else
	{
		mySndCmd.cmd = bufferCmd;									// start it
		mySndCmd.param1 = 0;
		mySndCmd.param2 = ((long)*gSndHandles[SOUND_BANK_SONG][0])+gSndOffsets[SOUND_BANK_SONG][0];
	    SndDoCommand(gSndChannel[gSongChannel], &mySndCmd, true);
	}
}



#pragma mark -

/***************************** PLAY EFFECT 3D ***************************/
//
// NO SSP
//
// OUTPUT: channel # used to play sound
//

short PlayEffect3D(short effectNum, OGLPoint3D *where)
{
short					theChan;
Byte					bankNum,soundNum;
u_long					leftVol, rightVol;

			/* GET BANK & SOUND #'S FROM TABLE */

	bankNum 	= gEffectsTable[effectNum].bank;
	soundNum 	= gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number! %d", effectNum);
	}

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

short PlayEffect_Parms3D(short effectNum, OGLPoint3D *where, u_long rateMultiplier, float volumeAdjust)
{
short			theChan;
Byte			bankNum,soundNum;
u_long			leftVol, rightVol;

			/* GET BANK & SOUND #'S FROM TABLE */

	bankNum 	= gEffectsTable[effectNum].bank;
	soundNum 	= gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoFatalAlert("Illegal sound number! %d", effectNum);
	}

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

Boolean Update3DSoundChannel(short effectNum, short *channel, OGLPoint3D *where)
{
SCStatus		theStatus;
u_long			leftVol,rightVol;
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

	if (!gChannelInfo[c].isLooping)										// loopers wont complete, duh.
	{
		SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
		if (!theStatus.scChannelBusy)									// see if channel not busy
		{
			StopAChannel(channel);							// make sure it's really stopped (OS X sound manager bug)
			return(true);
		}
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

static void Calc3DEffectVolume(short effectNum, OGLPoint3D *where, float volAdjust, u_long *leftVolOut, u_long *rightVolOut)
{
float	dist;
float	refDist,volumeFactor;
u_long	volume,left,right;
u_long	maxLeft,maxRight;

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

void UpdateListenerLocation(OGLSetupOutputType *setupInfo)
{
OGLVector3D	v;

	v.x = setupInfo->cameraPlacement.pointOfInterest.x - setupInfo->cameraPlacement.cameraLocation.x;	// calc line of sight vector
	v.y = setupInfo->cameraPlacement.pointOfInterest.y - setupInfo->cameraPlacement.cameraLocation.y;
	v.z = setupInfo->cameraPlacement.pointOfInterest.z - setupInfo->cameraPlacement.cameraLocation.z;
	FastNormalizeVector(v.x, v.y, v.z, &v);

	gEarCoords.x = setupInfo->cameraPlacement.cameraLocation.x + (v.x * 300.0f);			// put ear coord in front of camera
	gEarCoords.y = setupInfo->cameraPlacement.cameraLocation.y + (v.y * 300.0f);
	gEarCoords.z = setupInfo->cameraPlacement.cameraLocation.z + (v.z * 300.0f);

	gEyeVector = v;
}


/***************************** PLAY EFFECT ***************************/
//
// OUTPUT: channel # used to play sound
//

short PlayEffect(short effectNum)
{
	return(PlayEffect_Parms(effectNum,FULL_CHANNEL_VOLUME,FULL_CHANNEL_VOLUME,NORMAL_CHANNEL_RATE));

}

/***************************** PLAY EFFECT PARMS ***************************/
//
// Plays an effect with parameters
//
// OUTPUT: channel # used to play sound
//

short  PlayEffect_Parms(short effectNum, u_long leftVolume, u_long rightVolume, unsigned long rateMultiplier)
{
	IMPLEMENT_ME_SOFT();
	return -1;
#if 0
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
short			theChan;
Byte			bankNum,soundNum;
OSErr			myErr;
u_long			lv2,rv2;
static UInt32          loopStart, loopEnd;
SoundHeaderPtr   sndPtr;



			/* GET BANK & SOUND #'S FROM TABLE */

	bankNum = gEffectsTable[effectNum].bank;
	soundNum = gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoFatalAlert("Illegal sound number! %d", effectNum);
	}

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
	myErr = SndDoCommand(chanPtr, &mySndCmd, true);


	mySndCmd.cmd = bufferCmd;										// make it play
	mySndCmd.param1 = 0;
	mySndCmd.param2 = ((long)*gSndHandles[bankNum][soundNum])+gSndOffsets[bankNum][soundNum];	// pointer to SoundHeader
    SndDoCommand(chanPtr, &mySndCmd, true);
	if (myErr)
		return(-1);

	mySndCmd.cmd 		= rateMultiplierCmd;						// modify the rate to change the frequency
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= rateMultiplier;
	SndDoImmediate(chanPtr, &mySndCmd);



    		/* SEE IF THIS IS A LOOPING EFFECT */

    sndPtr = (SoundHeaderPtr)(((long)*gSndHandles[bankNum][soundNum])+gSndOffsets[bankNum][soundNum]);
    loopStart = sndPtr->loopStart;
    loopEnd = sndPtr->loopEnd;
    if ((loopStart + 1) < loopEnd)
    {
    	mySndCmd.cmd = callBackCmd;										// let us know when the buffer is done playing
    	mySndCmd.param1 = 0;
    	mySndCmd.param2 = ((long)*gSndHandles[bankNum][soundNum])+gSndOffsets[bankNum][soundNum];	// pointer to SoundHeader
    	SndDoCommand(chanPtr, &mySndCmd, true);

    	gChannelInfo[theChan].isLooping = true;
    	gNumLoopingEffects++;
	}
	else
		gChannelInfo[theChan].isLooping = false;


			/* SET MY INFO */

	gChannelInfo[theChan].effectNum 	= effectNum;		// remember what effect is playing on this channel
	gChannelInfo[theChan].leftVolume 	= leftVolume;		// remember requested volume (not the adjusted volume!)
	gChannelInfo[theChan].rightVolume 	= rightVolume;
	return(theChan);										// return channel #
#endif
}


#pragma mark -


/****************** UPDATE GLOBAL VOLUME ************************/
//
// Call this whenever gGlobalVolume is changed.  This will update
// all of the sounds with the correct volume.
//

static void UpdateGlobalVolume(void)
{
int		c;

			/* ADJUST VOLUMES OF ALL CHANNELS REGARDLESS IF THEY ARE PLAYING OR NOT */

	for (c = 0; c < gMaxChannels; c++)
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
u_long			lv2,rv2;

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




#pragma mark -


/******************** DO SOUND MAINTENANCE *************/
//
// 		UpdateInput() must have already been called
//

void DoSoundMaintenance(void)
{

	if (gAllowAudioKeys)
	{
					/* SEE IF TOGGLE MUSIC */

		if (GetNewKeyState(KEY_M))
		{
			ToggleMusic();
		}


				/* SEE IF CHANGE VOLUME */

		if (GetKeyState(KEY_PLUS))
		{
			gGlobalVolume += .5f * gFramesPerSecondFrac;
			UpdateGlobalVolume();
		}
		else
		if (GetKeyState(KEY_MINUS))
		{
			gGlobalVolume -= .5f * gFramesPerSecondFrac;
			if (gGlobalVolume < 0.0f)
				gGlobalVolume = 0.0f;
			UpdateGlobalVolume();
		}
	}


#if 0
		/* ALSO CHECK OPTIONS */

	if (GetNewKeyState(KEY_F1))
	{
		DoGameOptionsDialog();
	}

		/* AND CONTROL SETTINGS */

	else
	if (GetNewKeyState(KEY_F2))
	{
		DoInputConfigDialog();
	}
#endif
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
		if (gChannelInfo[theChan].isLooping)				// see if this channel is playing a looping effect
			goto next;

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


















