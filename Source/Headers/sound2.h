//
// Sound2.h
//


typedef struct
{
	short	effectNum;
	float	volumeAdjust;
	float	leftVolume, rightVolume;
}ChannelInfoType;



#define		BASE_EFFECT_RESOURCE	10000

#define		FULL_CHANNEL_VOLUME		kFullVolume
#define		NORMAL_CHANNEL_RATE		0x10000

enum
{
	SOUND_BANK_MUSIC,
	SOUND_BANK_MAIN,
	SOUND_BANK_TITLE,
	SOUND_BANK_BONUS,
	SOUND_BANK_GARDEN,
	SOUND_BANK_FIDO,
	SOUND_BANK_PLUMBING,
	SOUND_BANK_PLAYROOM,
	SOUND_BANK_CLOSET,
	SOUND_BANK_GARBAGE,
	SOUND_BANK_BALSA,
	SOUND_BANK_PARK,
	NUM_SOUND_BANKS
};


/***************** EFFECTS *************************/

enum
{
		/* SONG */

	EFFECT_SONG_TITLE,
	EFFECT_SONG_THEME,
	EFFECT_SONG_GARDEN,
	EFFECT_SONG_POOL,
	EFFECT_SONG_FIDO,
	EFFECT_SONG_PLUMBING,
	EFFECT_SONG_PLAYROOM,
	EFFECT_SONG_CLOSET,
	EFFECT_SONG_GARBAGE,
	EFFECT_SONG_BALSA,
	EFFECT_SONG_PARK,
	EFFECT_SONG_BONUS,
	EFFECT_SONG_WIN,
	EFFECT_SONG_LOSE,

		/* DEFAULT */

	EFFECT_CHANGESELECT,
	EFFECT_JUMP,
	EFFECT_SKIPGLIDE,
	EFFECT_SKIPKICK,
	EFFECT_ACORNKICKED,
	EFFECT_SKIPLAND,
	EFFECT_FLYGOTKICKED,
	EFFECT_GETPOW,
	EFFECT_BUTTERFLYBOOM,
	EFFECT_FLYWALKBUZZ,
	EFFECT_SMACK,
	EFFECT_SPLASH,
	EFFECT_BUDDYLAUNCH,
	EFFECT_BUDDYBUZZ,
	EFFECT_CHIP_CHECKPOINT,
	EFFECT_DOORCREAK,
	EFFECT_THROWBOTTLECAP,
	EFFECT_BOTTLECAPBOUNCE,
	EFFECT_MOUSETRAP,
	EFFECT_BUDDYBOOM,
	EFFECT_CHIP_MAP4ACORN,
	EFFECT_SHIELD,
	EFFECT_FIRECRACKER,
	EFFECT_BUMBLERUMBLE,
	EFFECT_CHIP_CHECKPOINTDONE,
	EFFECT_CHIP_THANKS,
	EFFECT_GRENADEBOOM,
	EFFECT_PLANECRASH,
	EFFECT_DRAGONFLYBUZZ,
	EFFECT_BOTTLECRACK,
	EFFECT_BOTTLESHATTER,
	EFFECT_PULLTRAP,
	EFFECT_SNAPTRAP,
	EFFECT_POPACORN,
	EFFECT_FOOTSTEP,
	EFFECT_GRENADETHROW,


		/* GARDEN */

	EFFECT_GNOMESTEP,
	EFFECT_SPRINKLER,
	EFFECT_GNOMEGOTKICKED,
	EFFECT_SQUISHBERRY,
	EFFECT_CHIP_STUCKMOUSE,
	EFFECT_EVILPLANTSHOOT,
	EFFECT_SAM_FREEMICE,
	EFFECT_SAM_FREEMICE2,
	EFFECT_SAM_FINDSHELL,
	EFFECT_SAM_GOTSHELL,
	EFFECT_SAM_FINDHEAD,
	EFFECT_SAM_PUTHEADON,
	EFFECT_SAM_FIXEDSCARECROW,
	EFFECT_SAM_SQUASHBERRIES,
	EFFECT_SAM_SQUISHMORE,
	EFFECT_SAM_SQUISHDONE,
	EFFECT_SAM_GOTMICE,
	EFFECT_SAM_POOLKEY,
	EFFECT_SAM_FIDO,

			/* FIDO */

	EFFECT_GOTFLEAS,
	EFFECT_GOTTICKS,
	EFFECT_HAPPYDOG,
	EFFECT_REMEMBERDOG,
	EFFECT_TICKSUCK,
	EFFECT_TICKSPIT,
	EFFECT_TICKSTEP,
	EFFECT_TICKDIE,
	EFFECT_BONEHIT,


		/* PLUMBING */

	EFFECT_PLUMBINGINTRO,
	EFFECT_SLUDGEHIT,
	EFFECT_NAILHIT,
	EFFECT_TUNNELWATER,
	EFFECT_METALSCRAPE,
	EFFECT_HITPINECONE,
	EFFECT_HITLEAF,


		/* PLAYROOM */

	EFFECT_LASERBOOM,
	EFFECT_SAM_FINDMARBLE,
	EFFECT_SAM_KICKMARBLE,
	EFFECT_SAM_PINSDOWN,
	EFFECT_BOWLINGHIT,
	EFFECT_KICKMARBLE,
	EFFECT_OTTOMOTOR,
	EFFECT_CHIP_DORACE,
	EFFECT_CHIP_LOSTRACE,
	EFFECT_CHIP_WINNING,
	EFFECT_CHIP_YOUWON,
	EFFECT_CHIP_SAMWINNING,
	EFFECT_SAM_DOPUZZLE,
	EFFECT_SAM_PUZZLEDONE,
	EFFECT_OTTOSHOOT,
	EFFECT_SLOTCAR,
	EFFECT_OTTOFALL,
	EFFECT_CHIP_READY,
	EFFECT_CHIP_SET,
	EFFECT_CHIP_GO,

			/* BALSA */

	EFFECT_PROPELLER,
	EFFECT_HILLBOOM,
	EFFECT_BOMBBOOM,
	EFFECT_PLANEHIT,
	EFFECT_FROGJUMP,
	EFFECT_DIVEBOMB,
	EFFECT_BOMBFALL,
	EFFECT_BALSASHOOT,
	EFFECT_DRAGONFLYHIT,
	EFFECT_SAM_DESTROYHILLS,
	EFFECT_SAM_HILLSDESTROYED,
	EFFECT_SAM_HOWTOBOMB,

		/* CLOSET */

	EFFECT_SERVO,
	EFFECT_SERVO2,
	EFFECT_MINEBOOM,
	EFFECT_CHIPCLICK,
	EFFECT_SILICONDOOROPEN,
	EFFECT_SAM_GETREDCLOVERS,
	EFFECT_SAM_GOTREDCLOVERS,
	EFFECT_VACUUME,
	EFFECT_VACUUMECRUMCH,
	EFFECT_MOTHFLAP,
	EFFECT_SAM_MOTHBALLS,
	EFFECT_SAM_COMPUTERDOOR,

		/* GARBAGE */

	EFFECT_CANOPEN,
	EFFECT_SODASPRAY,
	EFFECT_SAM_GUTTERWATER,
	EFFECT_SAM_FREED,
	EFFECT_SAM_GLIDER,
	EFFECT_SAM_SODA,
	EFFECT_PROP2,

		/* PARK */

	EFFECT_FROGJUMP2,
	EFFECT_TONGUEHIT,
	EFFECT_TONGUESWOOSH,
	EFFECT_SAM_CATCHFISH,
	EFFECT_SAM_ANGLER,
	EFFECT_SAM_GETFOOD,
	EFFECT_SAM_GOTFOOD,
	EFFECT_ANTBITE,
	EFFECT_FISHFLOP,
	EFFECT_SAM_ENTERHIVE,
	EFFECT_SAM_KEEPFISHING,
	EFFECT_SAM_MOREFOOD,
	EFFECT_SAM_GETKINDLING,
	EFFECT_SAM_MOREKINDLING,
	EFFECT_SAM_SPARK,
	EFFECT_SAM_BOTTLEKEY,


		/* TITLE */

	EFFECT_LOGOBOUNCE,
	EFFECT_FLYSWATTER,
	EFFECT_LOGOVANISH,
	EFFECT_TITLEFLYBUZZ,
	EFFECT_SMACKDOWN,
	EFFECT_STOMP,


		/* BONUS */

	EFFECT_CLOVERBONUS,
	EFFECT_MOUSEBONUS,

	NUM_EFFECTS
};



//===================== PROTOTYPES ===================================


extern void	InitSoundTools(void);
void ShutdownSound(void);

void StopAChannel(short *channelNum);
void StopAllEffectChannels(void);
void PlaySong(int songEffectNum, Boolean loopFlag);
void KillSong(void);
short PlayEffect(int effectNum);
short PlayEffect_Parms3D(int effectNum, OGLPoint3D *where, uint32_t rateMultiplier, float volumeAdjust);
void EnforceMusicPausePref(void);
void LoadSoundBank(int bankNum);
void DisposeSoundBank(int bankNum);
void LoadSoundEffect(int effectNum);
void DisposeSoundEffect(int effectNum);
short PlayEffect_Parms(int effectNum, uint32_t leftVolume, uint32_t rightVolume, unsigned long rateMultiplier);
void ChangeChannelVolume(short channel, float leftVol, float rightVol);
short PlayEffect3D(int effectNum, OGLPoint3D *where);
Boolean Update3DSoundChannel(int effectNum, short *channel, OGLPoint3D *where);
Boolean IsEffectChannelPlaying(short chanNum);
void UpdateListenerLocation(void);
void ChangeChannelRate(short channel, long rateMult);
Boolean StopAChannelIfEffectNum(short *channelNum, short effectNum);
void PauseAllChannels(Boolean pause);
void PlayRumbleEffect(int effectNum);
