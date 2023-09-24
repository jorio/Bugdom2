/****************************/
/*   	DIALOG.C		    */
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

static void CreateDialogMessage(int messNum, int priority, float duration, OGLPoint3D *fromWhere);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	LETTER_SIZE			9.0f
#define	LETTER_SPACING		(LETTER_SIZE * 1.15f)

#define	DIALOG_ICON_WIDTH	50.0f




#define	DIALOG_FRAME_WIDTH		530

enum
{
	DIALOG_MODE_APPEAR,
	DIALOG_MODE_DISAPPEAR,
	DIALOG_MODE_STAY,
	DIALOG_MODE_NONE
};



static const int gMessageIcon[MAX_DIALOG_MESSAGES][2] =					// Sprite Group, Sprite #
{
	[DIALOG_MESSAGE_NEEDSNAILSHELL]				= { SPRITE_GROUP_LEVELSPECIFIC, GARDEN_SObjType_SnailShellIcon },
	[DIALOG_MESSAGE_GOTSNAILSHELL]				= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_RedKeyIcon },
	[DIALOG_MESSAGE_FINDSCARECROWHEAD]			= { SPRITE_GROUP_LEVELSPECIFIC, GARDEN_SObjType_ScarecrowHeadIcon },
	[DIALOG_MESSAGE_PUTSCARECROWHEAD]			= { SPRITE_GROUP_LEVELSPECIFIC, GARDEN_SObjType_ScarecrowHeadIcon },
	[DIALOG_MESSAGE_ATTACHEDSCARECROWHEAD]		= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_GreenKeyIcon },
	[DIALOG_MESSAGE_FINDMARBLE]					= { SPRITE_GROUP_LEVELSPECIFIC, PLAYROOM_SObjType_MarbleIcon },
	[DIALOG_MESSAGE_BOWLMARBLE]					= { SPRITE_GROUP_LEVELSPECIFIC, PLAYROOM_SObjType_MarbleIcon },
	[DIALOG_MESSAGE_DONEBOWLING]				= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_BlueKeyIcon },
	[DIALOG_MESSAGE_CHIPMUNK_MAP4ACORN]			= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_AcornIcon },
	[DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT4ACORN]	= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_AcornIcon },
	[DIALOG_MESSAGE_CHIPMUNK_MOUSETRAP]			= { SPRITE_GROUP_LEVELSPECIFIC, GARDEN_SObjType_Mouse },
	[DIALOG_MESSAGE_CHIPMUNK_THANKS]			= { -1,-1 },
	[DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT]		= { -1,-1 },
	[DIALOG_MESSAGE_POOLWATER]					= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_RedKeyIcon },
	[DIALOG_MESSAGE_SMASHBERRIES]				= { SPRITE_GROUP_LEVELSPECIFIC, SIDEWALK_SObjType_SquishBerry },
	[DIALOG_MESSAGE_SQUISHMORE]					= { SPRITE_GROUP_LEVELSPECIFIC, SIDEWALK_SObjType_SquishBerry },
	[DIALOG_MESSAGE_SQUISHDONE]					= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_GreenKeyIcon },
	[DIALOG_MESSAGE_DOGHOUSE]					= { -1,-1 },
	[DIALOG_MESSAGE_GOTFLEAS]					= { SPRITE_GROUP_LEVELSPECIFIC, FIDO_SObjType_Tick },
	[DIALOG_MESSAGE_GOTTICKS]					= { SPRITE_GROUP_LEVELSPECIFIC, FIDO_SObjType_Flea },
	[DIALOG_MESSAGE_HAPPYDOG]					= { -1, -1 },
	[DIALOG_MESSAGE_REMEMBERDOG]				= { SPRITE_GROUP_LEVELSPECIFIC, FIDO_SObjType_Flea },
	[DIALOG_MESSAGE_PLUMBINGINTRO]				= { -1,-1 },
	[DIALOG_MESSAGE_SLOTCAR]					= { -1,-1 },
	[DIALOG_MESSAGE_RESCUEMICE]					= { SPRITE_GROUP_LEVELSPECIFIC, GARDEN_SObjType_Mouse },
	[DIALOG_MESSAGE_RESCUEMICE2]				= { SPRITE_GROUP_LEVELSPECIFIC, GARDEN_SObjType_Mouse },
	[DIALOG_MESSAGE_MICESAVED]					= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_RedKeyIcon },
	[DIALOG_MESSAGE_SLOTCARPLAYERWON]			= { -1,-1 },
	[DIALOG_MESSAGE_SLOTCARTRYAGAIN]			= { -1,-1 },
	[DIALOG_MESSAGE_DOPUZZLE]					= { -1,-1 },
	[DIALOG_MESSAGE_DONEPUZZLE]					= { -1,-1 },
	[DIALOG_MESSAGE_BOMBHILLS]					= { SPRITE_GROUP_LEVELSPECIFIC, BALSA_SObjType_AntHillIcon },
	[DIALOG_MESSAGE_BOMBHILLS2]					= { SPRITE_GROUP_LEVELSPECIFIC, BALSA_SObjType_AntHillIcon },
	[DIALOG_MESSAGE_HILLSDONE]					= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_BeeIcon },
	[DIALOG_MESSAGE_MOTHBALL]					= { SPRITE_GROUP_LEVELSPECIFIC, CLOSET_SObjType_MothBallIcon },
	[DIALOG_MESSAGE_SILICONDOOR]				= { SPRITE_GROUP_LEVELSPECIFIC, CLOSET_SObjType_ChipIcon },
	[DIALOG_MESSAGE_GETREDCLOVERS]				= { SPRITE_GROUP_LEVELSPECIFIC, CLOSET_SObjType_RedClover },
	[DIALOG_MESSAGE_GOTREDCLOVERS]				= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_RedKeyIcon },
	[DIALOG_MESSAGE_GOFISHING]					= { SPRITE_GROUP_LEVELSPECIFIC, PARK_SObjType_BobberIcon },
	[DIALOG_MESSAGE_MOREFISH]					= { SPRITE_GROUP_LEVELSPECIFIC, PARK_SObjType_BobberIcon },
	[DIALOG_MESSAGE_THANKSFISH]					= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_RedKeyIcon },
	[DIALOG_MESSAGE_GETFOOD]					= { SPRITE_GROUP_LEVELSPECIFIC, PARK_SObjType_CheeseIcon },
	[DIALOG_MESSAGE_MOREFOOD]					= { SPRITE_GROUP_LEVELSPECIFIC, PARK_SObjType_CheeseIcon },
	[DIALOG_MESSAGE_THANKSFOOD]					= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_BlueKeyIcon },
	[DIALOG_MESSAGE_GETKINDLING]				= { SPRITE_GROUP_LEVELSPECIFIC, PARK_SObjType_KindlingIcon },
	[DIALOG_MESSAGE_MOREKINDLING]				= { SPRITE_GROUP_LEVELSPECIFIC, PARK_SObjType_KindlingIcon },
	[DIALOG_MESSAGE_LIGHTFIRE]					= { SPRITE_GROUP_LEVELSPECIFIC, PARK_SObjType_FireIcon },
	[DIALOG_MESSAGE_ENTERHIVE]					= { SPRITE_GROUP_LEVELSPECIFIC, PARK_SObjType_HiveIcon },
	[DIALOG_MESSAGE_BOTTLEKEY]					= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_GreenKeyIcon },
	[DIALOG_MESSAGE_MICEDROWN]					= { SPRITE_GROUP_LEVELSPECIFIC, GARBAGE_SObjType_Mouse },
	[DIALOG_MESSAGE_THANKSNODROWN]				= { SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_RedKeyIcon },
	[DIALOG_MESSAGE_GLIDER]						= { SPRITE_GROUP_LEVELSPECIFIC, GARBAGE_SObjType_PropIcon },
	[DIALOG_MESSAGE_SODACAN]					= { SPRITE_GROUP_LEVELSPECIFIC, GARBAGE_SObjType_CanIcon },
};

static const int gMessageSound[MAX_DIALOG_MESSAGES] =
{
	[DIALOG_MESSAGE_NEEDSNAILSHELL]				= EFFECT_SAM_FINDSHELL,
	[DIALOG_MESSAGE_GOTSNAILSHELL]				= EFFECT_SAM_GOTSHELL,
	[DIALOG_MESSAGE_FINDSCARECROWHEAD]			= EFFECT_SAM_FINDHEAD,
	[DIALOG_MESSAGE_PUTSCARECROWHEAD]			= EFFECT_SAM_PUTHEADON,
	[DIALOG_MESSAGE_ATTACHEDSCARECROWHEAD]		= EFFECT_SAM_FIXEDSCARECROW,
	[DIALOG_MESSAGE_FINDMARBLE]					= EFFECT_SAM_FINDMARBLE,
	[DIALOG_MESSAGE_BOWLMARBLE]					= EFFECT_SAM_KICKMARBLE,
	[DIALOG_MESSAGE_DONEBOWLING]				= EFFECT_SAM_PINSDOWN,
	[DIALOG_MESSAGE_CHIPMUNK_MAP4ACORN]			= EFFECT_CHIP_MAP4ACORN,
	[DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT4ACORN]	= EFFECT_CHIP_CHECKPOINT,
	[DIALOG_MESSAGE_CHIPMUNK_MOUSETRAP]			= EFFECT_CHIP_STUCKMOUSE,
	[DIALOG_MESSAGE_CHIPMUNK_THANKS]			= EFFECT_CHIP_THANKS,
	[DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT]		= EFFECT_CHIP_CHECKPOINTDONE,
	[DIALOG_MESSAGE_POOLWATER]					= EFFECT_SAM_POOLKEY,
	[DIALOG_MESSAGE_SMASHBERRIES]				= EFFECT_SAM_SQUASHBERRIES,
	[DIALOG_MESSAGE_SQUISHMORE]					= EFFECT_SAM_SQUISHMORE,
	[DIALOG_MESSAGE_SQUISHDONE]					= EFFECT_SAM_SQUISHDONE,
	[DIALOG_MESSAGE_DOGHOUSE]					= EFFECT_SAM_FIDO,
	[DIALOG_MESSAGE_GOTFLEAS]					= EFFECT_GOTFLEAS,
	[DIALOG_MESSAGE_GOTTICKS]					= EFFECT_GOTTICKS,
	[DIALOG_MESSAGE_HAPPYDOG]					= EFFECT_HAPPYDOG,
	[DIALOG_MESSAGE_REMEMBERDOG]				= EFFECT_REMEMBERDOG,
	[DIALOG_MESSAGE_PLUMBINGINTRO]				= EFFECT_PLUMBINGINTRO,
	[DIALOG_MESSAGE_SLOTCAR]					= EFFECT_CHIP_DORACE,
	[DIALOG_MESSAGE_RESCUEMICE]					= EFFECT_SAM_FREEMICE,
	[DIALOG_MESSAGE_RESCUEMICE2]				= EFFECT_SAM_FREEMICE2,
	[DIALOG_MESSAGE_MICESAVED]					= EFFECT_SAM_GOTMICE,
	[DIALOG_MESSAGE_SLOTCARPLAYERWON]			= EFFECT_CHIP_YOUWON,
	[DIALOG_MESSAGE_SLOTCARTRYAGAIN]			= EFFECT_CHIP_LOSTRACE,
	[DIALOG_MESSAGE_DOPUZZLE]					= EFFECT_SAM_DOPUZZLE,
	[DIALOG_MESSAGE_DONEPUZZLE]					= EFFECT_SAM_PUZZLEDONE,
	[DIALOG_MESSAGE_BOMBHILLS]					= EFFECT_SAM_DESTROYHILLS,
	[DIALOG_MESSAGE_BOMBHILLS2]					= EFFECT_SAM_HOWTOBOMB,
	[DIALOG_MESSAGE_HILLSDONE]					= EFFECT_SAM_HILLSDESTROYED,
	[DIALOG_MESSAGE_MOTHBALL]					= EFFECT_SAM_MOTHBALLS,
	[DIALOG_MESSAGE_SILICONDOOR]				= EFFECT_SAM_COMPUTERDOOR,
	[DIALOG_MESSAGE_GETREDCLOVERS]				= EFFECT_SAM_GETREDCLOVERS,
	[DIALOG_MESSAGE_GOTREDCLOVERS]				= EFFECT_SAM_GOTREDCLOVERS,
	[DIALOG_MESSAGE_GOFISHING]					= EFFECT_SAM_CATCHFISH,
	[DIALOG_MESSAGE_MOREFISH]					= EFFECT_SAM_KEEPFISHING,
	[DIALOG_MESSAGE_THANKSFISH]					= EFFECT_SAM_ANGLER,
	[DIALOG_MESSAGE_GETFOOD]					= EFFECT_SAM_GETFOOD,
	[DIALOG_MESSAGE_MOREFOOD]					= EFFECT_SAM_MOREFOOD,
	[DIALOG_MESSAGE_THANKSFOOD]					= EFFECT_SAM_GOTFOOD,
	[DIALOG_MESSAGE_GETKINDLING]				= EFFECT_SAM_GETKINDLING,
	[DIALOG_MESSAGE_MOREKINDLING]				= EFFECT_SAM_MOREKINDLING,
	[DIALOG_MESSAGE_LIGHTFIRE]					= EFFECT_SAM_SPARK,
	[DIALOG_MESSAGE_ENTERHIVE]					= EFFECT_SAM_ENTERHIVE,
	[DIALOG_MESSAGE_BOTTLEKEY]					= EFFECT_SAM_BOTTLEKEY,
	[DIALOG_MESSAGE_MICEDROWN]					= EFFECT_SAM_GUTTERWATER,
	[DIALOG_MESSAGE_THANKSNODROWN]				= EFFECT_SAM_FREED,
	[DIALOG_MESSAGE_GLIDER]						= EFFECT_SAM_GLIDER,
	[DIALOG_MESSAGE_SODACAN]					= EFFECT_SAM_SODA,
};



/*********************/
/*    VARIABLES      */
/*********************/

static float	gDialogAlpha;
static	int		gDialogMode;
static	float	gMessageReplayVoiceDelay[MAX_DIALOG_MESSAGES];


static	int			gCurrentDialogMessageNum;
static	char const *gCurrentDialogString;
static	int			gCurrentDialogIconGroup,gCurrentDialogIconFrame;
static	int			gNumLinesInCurrentDialog = 0;						// when 0, indicates no message active
short				gDialogSoundChannel;
int					gDialogSoundEffect;
static	OGLPoint3D	gDialogSoundWhere;

static	int		gNextMessageNum;
static	int		gNextMessagePriority,gCurrentDialogMessagePriority;
static	float	gNextMessageDuration, gMessageDuration;
static	OGLPoint3D	gNextMessageWhere;


/****************** INIT DIALOG MANAGER *************************/

void InitDialogManager(void)
{
FSSpec	spec;
int		i;

	gNumLinesInCurrentDialog = 0;
	gDialogAlpha			= 0;
	gNextMessageNum = gCurrentDialogMessageNum = -1;
	gNextMessagePriority = 100;
	gNextMessageDuration = 0;
	gDialogSoundChannel = -1;

	gCurrentDialogIconGroup = gCurrentDialogIconFrame = -1;

	for (i = 0; i < MAX_DIALOG_MESSAGES; i++)
		gMessageReplayVoiceDelay[i] = 0;

			/* LOAD THE SPRITES FOR THE DIALOGS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:Dialog.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_DIALOG);
}


/**************************** DO DIALOG MESSAGE *******************************/

void DoDialogMessage(int messNum, int priority, float duration, OGLPoint3D *fromWhere)
{
	if (messNum == gCurrentDialogMessageNum)						// if same message, then just reset timer
	{
		if (gDialogMode == DIALOG_MODE_DISAPPEAR)					// if was disappearing, then reverse
			gDialogMode = DIALOG_MODE_APPEAR;

		gMessageDuration = duration;
		return;
	}

			/* SEE IF THIS MESSAGE HAS HIGHER PRIORITY */

	if (gNumLinesInCurrentDialog == 0)								// if no dialog currently, then just do it
	{
		CreateDialogMessage(messNum, priority, duration, fromWhere);
		return;
	}

	if (priority < gCurrentDialogMessagePriority)					// if higher priority then make current message go away
		gDialogMode = DIALOG_MODE_DISAPPEAR;


				/* MAKE THIS THE NEXT MESSAGE */

	gNextMessageNum = messNum;
	gNextMessagePriority = priority;
	gNextMessageDuration = duration;

	if (fromWhere == nil)
		gNextMessageWhere.x = gNextMessageWhere.y = gNextMessageWhere.z = 0;
	else
		gNextMessageWhere = *fromWhere;

}


/********************* CREATE DIALOG MESSAGE **************************/

static void CreateDialogMessage(int messNum, int priority, float duration, OGLPoint3D *fromWhere)
{
int		i;
int		effect;

	gCurrentDialogIconGroup = gMessageIcon[messNum][0];								// get sprite group
	gCurrentDialogIconFrame = gMessageIcon[messNum][1];								// get sprite frame

	gCurrentDialogString = Localize(messNum);										// get ptr to string

				/* SEE HOW MANY LINES OF TEXT */

	i = 0;
	gNumLinesInCurrentDialog = 1;
	while (gCurrentDialogString[i] != 0x00)
	{
		if (gCurrentDialogString[i] == '\n')					// look for line feed
			gNumLinesInCurrentDialog++;
		i++;
	}

	gCurrentDialogMessageNum = messNum;
	gCurrentDialogMessagePriority = priority;
	gMessageDuration = duration;
	gDialogMode = DIALOG_MODE_APPEAR;

	gNextMessageNum = -1;										// no message waiting

			/* PLAY AUDIO */

	if (gMessageReplayVoiceDelay[messNum] == 0.0f)				// see if there's still a delay
	{
		effect = gMessageSound[messNum];
		if (effect != -1)
		{
			if (gDialogSoundChannel != -1)						// stop any existing audio
				StopAChannelIfEffectNum(&gDialogSoundChannel, gDialogSoundEffect);

			if (fromWhere)
			{
				gDialogSoundChannel = PlayEffect_Parms3D(effect, fromWhere, NORMAL_CHANNEL_RATE, 1.2);
				gDialogSoundWhere = *fromWhere;
			}
			else
			{
				gDialogSoundChannel = PlayEffect(effect);
				gDialogSoundWhere.x = gDialogSoundWhere.y = gDialogSoundWhere.z = 0.0;
			}

			gDialogSoundEffect = effect;

			gMessageReplayVoiceDelay[messNum] = 20.0f;			// set delay before can play this voice again
		}
	}
}


/************************ DRAW DIALOG MESSAGE ********************/

void DrawDialogMessage(void)
{
int		i;
float	x,y,leftX;

			/* UPDATE ANY CURRENT VOICE */

	if (gDialogSoundChannel != -1)
	{
		if ((gDialogSoundWhere.x != 0.0f) || (gDialogSoundWhere.y != 0.0f) || (gDialogSoundWhere.z != 0.0f))	// only update if not 0,0,0
			Update3DSoundChannel(gDialogSoundEffect, &gDialogSoundChannel, &gDialogSoundWhere);
	}

	for (i = 0; i < MAX_DIALOG_MESSAGES; i++)					// update delay timers while we're here
	{
		if (gMessageReplayVoiceDelay[i] > 0.0f)
		{
			gMessageReplayVoiceDelay[i] -= gFramesPerSecondFrac;
			if (gMessageReplayVoiceDelay[i] < 0.0f)
				gMessageReplayVoiceDelay[i] = 0.0f;
		}
	}

	if (gNumLinesInCurrentDialog == 0)
		return;

	SetColor4f(1,1,1,1);

		/***********************************/
		/* MOVE FRAME INTO POSTIONN & DRAW */
		/***********************************/

	switch(gDialogMode)
	{
		case	DIALOG_MODE_APPEAR:
				gDialogAlpha += gFramesPerSecondFrac * 2.0f;
				if (gDialogAlpha >= 1.0f)
				{
					gDialogAlpha = 1.0;
					gDialogMode = DIALOG_MODE_STAY;
				}
				break;

		case	DIALOG_MODE_DISAPPEAR:
				gDialogAlpha -= gFramesPerSecondFrac * 2.0f;
				if (gDialogAlpha <= 0.0f)
				{
					gDialogAlpha = 0.0f;
					gDialogMode = DIALOG_MODE_NONE;
					gCurrentDialogMessagePriority = 100;
					gCurrentDialogMessageNum = -1;
					if (gNextMessageNum != -1)
					{
						if ((gNextMessageWhere.x == 0.0f) && (gNextMessageWhere.y == 0.0f) && (gNextMessageWhere.z == 0.0f))		// see if has coord or not
							CreateDialogMessage(gNextMessageNum, gNextMessagePriority, gNextMessageDuration, nil);
						else
							CreateDialogMessage(gNextMessageNum, gNextMessagePriority, gNextMessageDuration, &gNextMessageWhere);
					}
				}
				break;

		case	DIALOG_MODE_STAY:
				gMessageDuration -= gFramesPerSecondFrac;
				if (gMessageDuration <= 0.0f)
					gDialogMode = DIALOG_MODE_DISAPPEAR;
				break;

		case	DIALOG_MODE_NONE:
				break;

	}


			/* SET FADE IN/OUT BASED ON DISTANCE FROM STOP POSITION */

	gGlobalTransparency = gDialogAlpha;


			/* DRAW FRAME FIRST */

	x = (640-DIALOG_FRAME_WIDTH)/2 + 5.0f;
	y = 410.0f;
	DrawInfobarSprite2(x, y, DIALOG_FRAME_WIDTH, SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_Frame);


			/* DRAW ICON */

	if (gCurrentDialogIconGroup != -1)						// see if has icon
	{
		DrawInfobarSprite2(x+6, y+6, DIALOG_ICON_WIDTH, gCurrentDialogIconGroup, gCurrentDialogIconFrame);
	}


			/*************/
			/* DRAW TEXT */
			/*************/

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// make glow

	gGlobalColorFilter.r = 1.0f;
	gGlobalColorFilter.g = 1.0f;
	gGlobalColorFilter.b = .8f;

	leftX = x += 80.0f;

			/* SET VERTICAL CENTERING */

	y += 10;

	if (gNumLinesInCurrentDialog == 1)						// center based on # of lines of text
		y += LETTER_SIZE * 1.3f;
	else
	if (gNumLinesInCurrentDialog == 2)
		y += LETTER_SIZE * 1.3f * .5f;


			/* DRAW EACH CHAR */

	i = 0;
	while (gCurrentDialogString[i] != 0x00)
	{
			/* NEXT LINE */

		if (gCurrentDialogString[i] == '\n')					// look for line feed
		{
			y += LETTER_SIZE * 1.3f;
			x = leftX;
		}

			/* DRAW LETTER */
		else
		{
			char	c = gCurrentDialogString[i];
			int	texNum = CharToSprite(c);
			if (texNum != -1)
				DrawInfobarSprite2(x, y, LETTER_SIZE * 1.8f, SPRITE_GROUP_DIALOG, texNum);

			x += GetCharSpacing(c, LETTER_SPACING);
		}
		i++;
	}

	gGlobalColorFilter.r = 1.0f;
	gGlobalColorFilter.g = 1.0f;
	gGlobalColorFilter.b = 1.0f;

	gGlobalTransparency = 1.0f;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}




#pragma mark -


/***************** CHAR TO SPRITE **********************/

int CharToSprite(char c)
{

	if ((c >= 'a') && (c <= 'z'))
	{
		return(DIALOG_SObjType_a + (c - 'a'));
	}
	else
	if ((c >= 'A') && (c <= 'Z'))
	{
		return(DIALOG_SObjType_A + (c - 'A'));
	}
	else
	if ((c >= '0') && (c <= '9'))
	{
		return(DIALOG_SObjType_0 + (c - '0'));
	}
	else
	{
		short	s;
		switch(c)
		{
			case	'.':
					s = DIALOG_SObjType_Period;
					break;

			case	',':
					s = DIALOG_SObjType_Comma;
					break;

			case	'-':
					s = DIALOG_SObjType_Dash;
					break;

			case	'?':
					s = DIALOG_SObjType_QuestionMark;
					break;

			case	'!':
					s = DIALOG_SObjType_ExclamationMark;
					break;

			case	'�':
					s = DIALOG_SObjType_ExclamationMark2;
					break;

			case	'�':
					s = DIALOG_SObjType_UU;
					break;

			case	'�':
					s = DIALOG_SObjType_uu;
					break;

			case	'�':
					s = DIALOG_SObjType_ua;
					break;

			case	'�':
					s = DIALOG_SObjType_OO;
					break;

			case	'�':
					s = DIALOG_SObjType_oo;
					break;

			case	'�':
					s = DIALOG_SObjType_AA;
					break;

			case	'�':
					s = DIALOG_SObjType_AO;
					break;

			case	'�':
					s = DIALOG_SObjType_av;
					break;

			case	'�':
					s = DIALOG_SObjType_au;
					break;

			case	'�':
					s = DIALOG_SObjType_aa;
					break;

			case	'�':
					s = DIALOG_SObjType_NN;
					break;

			case	'�':
					s = DIALOG_SObjType_nn;
					break;

			case	'�':
					s = DIALOG_SObjType_EE;
					break;

			case	'�':
					s = DIALOG_SObjType_ee;
					break;

			case	'�':
					s = DIALOG_SObjType_ee;
					break;

			case	'�':
					s = DIALOG_SObjType_ev;
					break;

			case	'�':
					s = DIALOG_SObjType_EE;
					break;

			case	'�':
					s = DIALOG_SObjType_E;
					break;

			case	'�':
					s = DIALOG_SObjType_Ax;
					break;

			case	'�':
					s = DIALOG_SObjType_ax;
					break;

			case	'�':
					s = DIALOG_SObjType_ao;
					break;


			case	'�':
					s = DIALOG_SObjType_Ox;
					break;

			case	'�':
					s = DIALOG_SObjType_Oa;
					break;

			case	'�':
					s = DIALOG_SObjType_oa;
					break;

			case	'�':
					s = DIALOG_SObjType_beta;
					break;

			case	'�':
					s = DIALOG_SObjType_ia;
					break;

			case	'�':
					s = DIALOG_SObjType_C;
					break;

			case	'�':
					s = DIALOG_SObjType_c;
					break;

			case	CHAR_APOSTROPHE:
					s = DIALOG_SObjType_Apostrophe;
					break;


			default:
					s = -1;

		}
	return(s);
	}

}


/******************** GET CHAR SPACING *************************/

float GetCharSpacing(char c, float spacingScale)
{
float	s;

	switch(c)
	{
		case	'j':
		case	'9':
		case	'�':
				s = .6;
				break;

		case	' ':
		case	'f':
		case	't':
		case	'r':
				s = .5f;
				break;

		case	'I':
		case	'i':
		case	'�':
		case	'l':
		case	'.':
		case	',':
		case	'!':
		case	'�':
				s = .4;
				break;

		case	CHAR_APOSTROPHE:
				s = .3;
				break;

		case	'a':
		case	'�':
		case	'�':
		case	'�':
		case	'�':
		case	'b':
		case	'c':
		case	'�':
		case	'd':
		case	'e':
		case	'�':
		case	'�':
		case	'g':
		case	'h':
		case	'k':
		case	'n':
		case	'�':
		case	'o':
		case	'�':
		case	'�':
		case	'p':
		case	'q':
		case	's':
		case	'u':
		case	'�':
		case	'�':
		case	'v':
		case	'x':
		case	'y':
		case	'z':
		case	'J':
		case	'S':
		case	'T':
		case	'Y':
		case	'0':
		case	'1':
		case	'2':
		case	'3':
		case	'4':
		case	'5':
		case	'6':
		case	'7':
		case	'8':
		case	'L':
				s = .7f;
				break;

		case	'N':
		case	'P':
		case	'B':
		case	'C':
		case	'�':
		case	'V':
		case	'F':
		case	'H':
		case	'�':
				s = .8f;
				break;


		case	'M':
		case	'm':
				s = 1.0f;
				break;

		default:
				s = .9f;
	}





	return(spacingScale * s);


}



