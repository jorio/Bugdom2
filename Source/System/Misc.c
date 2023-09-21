/****************************/
/*      MISC ROUTINES       */
/* (c)1996-2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/


#include "game.h"


extern	Boolean		gG4,gShareware;
extern	Boolean		gHIDInitialized;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	int			gPolysThisFrame;
extern	AGLContext		gAGLContext;
extern	AGLDrawable		gAGLWin;
extern	float			gDemoVersionTimer;
extern	short	gPrefsFolderVRefNum;
extern	long		gPrefsFolderDirID;
extern	PrefsType			gGamePrefs;
extern	FSSpec				gDataSpec;

/****************************/
/*    CONSTANTS             */
/****************************/

#define		ERROR_ALERT_ID		401

#define	DEFAULT_FPS			10

#define	USE_MALLOC		1



/**********************/
/*     VARIABLES      */
/**********************/


u_long 	gSeed0 = 0, gSeed1 = 0, gSeed2 = 0;

float	gFramesPerSecond, gFramesPerSecondFrac;

int		gNumPointers = 0;

Str255  gSerialFileName = ":Bugdom2:Info";

Boolean	gGameIsRegistered = false;

unsigned char	gRegInfo[64];

Boolean	gSlowCPU = false;


Boolean			gAltivec = false;


/**********************/
/*     PROTOTYPES     */
/**********************/

static Boolean ValidateSerialNumber(unsigned char *regInfo);
static void DoSerialDialog(void);
static pascal OSStatus SerialDialog_EventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData);


/****************** DO SYSTEM ERROR ***************/

void ShowSystemErr(long err)
{
Str255		numStr;
SInt16      alertItemHit;

	Enter2D();
	ConvertIntToPStr(err, numStr);

	StandardAlert(kAlertStopAlert, numStr, NULL, NULL, &alertItemHit);

	Exit2D();

	ExitToShell();
}

/****************** DO SYSTEM ERROR : NONFATAL ***************/
//
// nonfatal
//
void ShowSystemErr_NonFatal(long err)
{
Str255		numStr;
SInt16      alertItemHit;

	Enter2D();
	ConvertIntToPStr(err, numStr);

	StandardAlert(kAlertStopAlert, numStr, NULL, NULL, &alertItemHit);

	Exit2D();
}


/*********************** DO ALERT *******************/

void DoAlert(const char* s)
{
SInt16      alertItemHit;

	Enter2D();

	StandardAlert(kAlertStopAlert, s, NULL, NULL, &alertItemHit);

	Exit2D();
}




/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(const char* s)
{
SInt16      alertItemHit;

	Enter2D();

	StandardAlert(kAlertNoteAlert, s, NULL, NULL, &alertItemHit);

	Exit2D();
	ExitToShell();
}


/************ CLEAN QUIT ***************/

void CleanQuit(void)
{
static Boolean	beenHere = false;

	if (!beenHere)
	{
		beenHere = true;

		DeleteAllObjects();
		DisposeTerrain();								// dispose of any memory allocated by terrain manager
		DisposeAllBG3DContainers();						// nuke all models
		DisposeAllSpriteGroups();						// nuke all sprites

		if (gGameViewInfoPtr)							// see if need to dispose this
			OGL_DisposeWindowSetup(&gGameViewInfoPtr);

		if (DEMO || (!gGameIsRegistered))
		{
			GammaFadeOut();
			ShowDemoQuitScreen();

		}

		ShutdownSound();								// cleanup sound stuff
	}

	GameScreenToBlack();
	CleanupDisplay();								// unloads Draw Sprocket

//	if (gHIDInitialized)							// unload HID
//	{
//	    HIDReleaseDeviceList ();
//		TearDownHIDCFM ();
//	}


	MyFlushEvents();

	SavePrefs();							// save prefs before bailing

	ExitToShell();
}



#pragma mark -


/******************** MY RANDOM LONG **********************/
//
// My own random number generator that returns a LONG
//
// NOTE: call this instead of MyRandomShort if the value is going to be
//		masked or if it just doesnt matter since this version is quicker
//		without the 0xffff at the end.
//

unsigned long MyRandomLong(void)
{
  return gSeed2 ^= (((gSeed1 ^= (gSeed2>>5)*1568397607UL)>>7)+
                   (gSeed0 = (gSeed0+1)*3141592621UL))*2435386481UL;
}


/************************* RANDOM RANGE *************************/
//
// THE RANGE *IS* INCLUSIVE OF MIN AND MAX
//

u_short	RandomRange(unsigned short min, unsigned short max)
{
u_short		qdRdm;											// treat return value as 0-65536
u_long		range, t;

	qdRdm = MyRandomLong();
	range = max+1 - min;
	t = (qdRdm * range)>>16;	 							// now 0 <= t <= range

	return( t+min );
}



/************** RANDOM FLOAT ********************/
//
// returns a random float between 0 and 1
//

float RandomFloat(void)
{
unsigned long	r;
float	f;

	r = MyRandomLong() & 0xfff;
	if (r == 0)
		return(0);

	f = (float)r;							// convert to float
	f = f * (1.0f/(float)0xfff);			// get # between 0..1
	return(f);
}


/************** RANDOM FLOAT 2 ********************/
//
// returns a random float between -1 and +1
//

float RandomFloat2(void)
{
unsigned long	r;
float	f;

	r = MyRandomLong() & 0xfff;
	if (r == 0)
		return(0);

	f = (float)r;							// convert to float
	f = f * (2.0f/(float)0xfff);			// get # between 0..2
	f -= 1.0f;								// get -1..+1
	return(f);
}



/**************** SET MY RANDOM SEED *******************/

void SetMyRandomSeed(unsigned long seed)
{
	gSeed0 = seed;
	gSeed1 = 0;
	gSeed2 = 0;

}

/**************** INIT MY RANDOM SEED *******************/

void InitMyRandomSeed(void)
{
	gSeed0 = 0x2a80ce30;
	gSeed1 = 0;
	gSeed2 = 0;
}


#pragma mark -


/******************* FLOAT TO STRING *******************/

void FloatToString(float num, Str255 string)
{
Str255	sf;
long	i,f;

	i = num;						// get integer part


	f = (fabs(num)-fabs((float)i)) * 10000;		// reduce num to fraction only & move decimal --> 5 places

	if ((i==0) && (num < 0))		// special case if (-), but integer is 0
	{
		string[0] = 2;
		string[1] = '-';
		string[2] = '0';
	}
	else
		NumToString(i,string);		// make integer into string

	NumToString(f,sf);				// make fraction into string

	string[++string[0]] = '.';		// add "." into string

	if (f >= 1)
	{
		if (f < 1000)
			string[++string[0]] = '0';	// add 1000's zero
		if (f < 100)
			string[++string[0]] = '0';	// add 100's zero
		if (f < 10)
			string[++string[0]] = '0';	// add 10's zero
	}

	for (i = 0; i < sf[0]; i++)
	{
		string[++string[0]] = sf[i+1];	// copy fraction into string
	}
}

/*********************** STRING TO FLOAT *************************/

float StringToFloat(Str255 textStr)
{
short	i;
short	length;
Byte	mode = 0;
long	integer = 0;
long	mantissa = 0,mantissaSize = 0;
float	f;
float	tens[8] = {1,10,100,1000,10000,100000,1000000,10000000};
char	c;
float	sign = 1;												// assume positive

	length = textStr[0];										// get string length

	if (length== 0)												// quick check for empty
		return(0);


			/* SCAN THE NUMBER */

	for (i = 1; i <= length; i++)
	{
		c  = textStr[i];										// get this char

		if (c == '-')											// see if negative
		{
			sign = -1;
			continue;
		}
		else
		if (c == '.')											// see if hit the decimal
		{
			mode = 1;
			continue;
		}
		else
		if ((c < '0') || (c > '9'))								// skip all but #'s
			continue;


		if (mode == 0)
			integer = (integer * 10) + (c - '0');
		else
		{
			mantissa = (mantissa * 10) + (c - '0');
			mantissaSize++;
		}
	}

			/* BUILT A FLOAT FROM IT */

	f = (float)integer + ((float)mantissa/tens[mantissaSize]);
	f *= sign;

	return(f);
}





#pragma mark -

/****************** ALLOC HANDLE ********************/

Handle	AllocHandle(long size)
{
Handle	hand;
OSErr	err;

	hand = NewHandle(size);							// alloc in APPL
	if (hand == nil)
	{
		DoAlert("AllocHandle: using temp mem");
		hand = TempNewHandle(size,&err);			// try TEMP mem
		if (hand == nil)
		{
			DoAlert("AllocHandle: failed!");
			return(nil);
		}
		else
			return(hand);
	}
	return(hand);

}



/****************** ALLOC PTR ********************/

void *AllocPtr(long size)
{
Ptr	pr;
u_long	*cookiePtr;

	size += 16;								// make room for our cookie & whatever else (also keep to 16-byte alignment!)

#if USE_MALLOC
	pr = malloc(size);
#else
	pr = NewPtr(size);
#endif
	if (pr == nil)
		DoFatalAlert("AllocPtr: NewPtr failed");

	cookiePtr = (u_long *)pr;

	*cookiePtr++ = 'FACE';
	*cookiePtr++ = 'PTR2';
	*cookiePtr++ = 'PTR3';
	*cookiePtr = 'PTR4';

	pr += 16;

	gNumPointers++;

	return(pr);
}


/****************** ALLOC PTR CLEAR ********************/

void *AllocPtrClear(long size)
{
Ptr	pr;
u_long	*cookiePtr;

	size += 16;								// make room for our cookie & whatever else (also keep to 16-byte alignment!)

#if USE_MALLOC
	pr = calloc(1, size);
#else
	pr = NewPtrClear(size);						// alloc in Application
#endif

	if (pr == nil)
		DoFatalAlert("AllocPtr: NewPtr failed");

	cookiePtr = (u_long *)pr;

	*cookiePtr++ = 'FACE';
	*cookiePtr++ = 'PTC2';
	*cookiePtr++ = 'PTC3';
	*cookiePtr = 'PTC4';

	pr += 16;

	gNumPointers++;

	return(pr);
}


/***************** SAFE DISPOSE PTR ***********************/

void SafeDisposePtr(void *ptr)
{
u_long	*cookiePtr;
Ptr		p = ptr;

	p -= 16;					// back up to pt to cookie

	cookiePtr = (u_long *)p;

	if (*cookiePtr != 'FACE')
		DoFatalAlert("SafeSafeDisposePtr: invalid cookie!");

	*cookiePtr = 0;

#if USE_MALLOC
	free(p);
#else
	DisposePtr(p);
#endif

	gNumPointers--;
}



#pragma mark -

/******************* COPY P STRING ********************/

void CopyPString(Str255 from, Str255 to)
{
short	i,n;

	n = from[0];			// get length

	for (i = 0; i <= n; i++)
		to[i] = from[i];

}


/***************** P STRING TO C ************************/

void PStringToC(char *pString, char *cString)
{
Byte	pLength,i;

	pLength = pString[0];

	for (i=0; i < pLength; i++)					// copy string
		cString[i] = pString[i+1];

	cString[pLength] = 0x00;					// add null character to end of c string
}


/***************** DRAW C STRING ********************/

void DrawCString(char *string)
{
	while(*string != 0x00)
		DrawChar(*string++);
}


/******************* VERIFY SYSTEM ******************/

void VerifySystem(void)
{
OSErr	iErr;
long		 cpuSpeed;
NumVersion	vers;


			/* SEE IF PROCESSOR IS G4 OR NOT */

	gSlowCPU = false;														// assume not slow

	gG4 = true;

	if (!gG4)																// if not G4, check processor speed to see if on really fast G3
	{
		iErr = Gestalt(gestaltProcClkSpeed,&cpuSpeed);
		if (iErr != noErr)
			DoFatalAlert("VerifySystem: gestaltProcClkSpeed failed!");

		if ((cpuSpeed/1000000) >= 600)										// must be at least 600mhz G3 for us to treat it like a G4
			gG4 = true;
		else
		if ((cpuSpeed/1000000) <= 450)										// if 450 or less then it's a slow G3
			gSlowCPU = true;
	}


		/* DETERMINE IF RUNNING ON OS X */

	iErr = Gestalt(gestaltSystemVersion,(long *)&vers);
	if (iErr != noErr)
		DoFatalAlert("VerifySystem: gestaltSystemVersion failed!");

//	if (vers.stage >= 0x10)													// see if at least OS 10
//	{
//		if ((vers.stage == 0x10) && (vers.nonRelRev < 0x40))				// must be at least OS 10.1 !!!
//			DoFatalAlert("This game requires OS 10.4 or later to run on OS X.  Run Software Update in System Preferences to get the latest update.");
//	}
//	else
//	{
//		DoFatalAlert("This game requires at least Mac OS 10.4.");
//	}


#if 0
			/* CHECK TIME-BOMB */
	{
		unsigned long secs;
		DateTimeRec	d;

		GetDateTime(&secs);
		SecondsToDate(secs, &d);

		if ((d.year > 2002) ||
			((d.year == 2002) && (d.month > 10)))
		{
			DoFatalAlert("Sorry, but this beta has expired");
		}
	}
#endif


		/**********************/
		/* SEE IF HAS ALTIVEC */
		/**********************/

	gAltivec = false;									// assume no altivec

#if __BIG_ENDIAN__						// temp for now

	SInt32	response;
	long	flags;

	if (!Gestalt(gestaltNativeCPUtype, &response))
	{
		if (response > gestaltCPU750)					// skip if on G3 or go into this if > G3
		{
			iErr = Gestalt(gestaltPowerPCProcessorFeatures,(long *)&flags);		// see if AltiVec available
			if (iErr != noErr)
				DoFatalAlert("VerifySystem: gestaltPowerPCProcessorFeatures failed!");
			gAltivec = ((flags & (1<<gestaltPowerPCHasVectorInstructions)) != 0);
		}
	}
#endif




		/***************************************/
		/* SEE IF QUICKEN SCHEDULER IS RUNNING */
		/***************************************/

	{
		ProcessSerialNumber psn = {kNoProcess, kNoProcess};
		ProcessInfoRec	info;
		short			i;
		Str255		s;
		const char snitch[] = "Quicken Scheduler";

		info.processName = s;
		info.processInfoLength = sizeof(ProcessInfoRec);
		info.processAppSpec = nil;

		while(GetNextProcess(&psn) == noErr)
		{
			iErr = GetProcessInformation(&psn, &info);
			if (iErr)
				break;

			if (s[0] != snitch[0])					// see if string matches
				goto next_process2;

			for (i = 1; i <= s[0]; i++)
			{
				if (s[i] != snitch[i])
					goto next_process2;
			}

			DoAlert("IMPORTANT:  Quicken Scheduler is known to cause certain keyboard access functions in OS X to malfunction.  If the keyboard does not appear to be working in this game, quit Quicken Scheduler to fix it.");

next_process2:;
		}
	}

}


/******************** REGULATE SPEED ***************/

void RegulateSpeed(short fps)
{
u_long	n;
static u_long oldTick = 0;

	n = 60 / fps;
	while ((TickCount() - oldTick) < n) {}			// wait for n ticks
	oldTick = TickCount();							// remember current time
}


/************* COPY PSTR **********************/

void CopyPStr(ConstStr255Param	inSourceStr, StringPtr	outDestStr)
{
short	dataLen = inSourceStr[0] + 1;

	BlockMoveData(inSourceStr, outDestStr, dataLen);
	outDestStr[0] = dataLen - 1;
}





#pragma mark -



/************** CALC FRAMES PER SECOND *****************/
//
// This version uses UpTime() which is only available on PCI Macs.
//

void CalcFramesPerSecond(void)
{
AbsoluteTime currTime,deltaTime;
static AbsoluteTime time = {0,0};
Nanoseconds	nano;

limit_speed:

	currTime = UpTime();

	deltaTime = SubAbsoluteFromAbsolute(currTime, time);
	nano = AbsoluteToNanoseconds(deltaTime);

	gFramesPerSecond = 1000000.0f / (float)nano.lo;
	gFramesPerSecond *= 1000.0f;

	if (gFramesPerSecond < DEFAULT_FPS)			// (avoid divide by 0's later)
		gFramesPerSecond = DEFAULT_FPS;
	gFramesPerSecondFrac = 1.0f/gFramesPerSecond;		// calc fractional for multiplication


	if (gFramesPerSecond > 100)
		goto limit_speed;

	time = currTime;	// reset for next time interval
}


/********************* IS POWER OF 2 ****************************/

Boolean IsPowerOf2(int num)
{
int		i;

	i = 2;
	do
	{
		if (i == num)				// see if this power of 2 matches
			return(true);
		i *= 2;						// next power of 2
	}while(i <= num);				// search until power is > number

	return(false);
}

#pragma mark-

/******************* MY FLUSH EVENTS **********************/
//
// This call makes damed sure that there are no events anywhere in any event queue.
//

void MyFlushEvents(void)
{
//EventRecord 	theEvent;

	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());

#if 0
			/* POLL EVENT QUEUE TO BE SURE THINGS ARE FLUSHED OUT */

	while (GetNextEvent(mDownMask|mUpMask|keyDownMask|keyUpMask|autoKeyMask, &theEvent));


	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
#endif
}


#pragma mark -



/******************** PSTR CAT / COPY *************************/

StringPtr PStrCat(StringPtr dst, ConstStr255Param   src)
{
SInt16 size = src[0];

	if (0xff - dst[0] < size)
		size = 0xff - dst[0];

	BlockMoveData(&src[1], &dst[dst[0]], size);
	dst[0] = dst[0] + size;

	return dst;
}

StringPtr PStrCopy(StringPtr dst, ConstStr255Param   src)
{
	dst[0] = src[0]; BlockMoveData(&src[1], &dst[1], src[0]); return dst;
}




#pragma mark -


/********************** CHECK GAME SERIAL NUMBER *************************/

void CheckGameSerialNumber(void)
{
OSErr   iErr;
FSSpec  spec;
short		fRefNum;
long        	numBytes = SERIAL_LENGTH;

            /* GET SPEC TO REG FILE */

	iErr = FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gSerialFileName, &spec);
    if (iErr)
        goto game_not_registered;


            /*************************/
            /* VALIDATE THE SERIAL FILE */
            /*************************/

            /* READ SERIAL DATA */

    if (FSpOpenDF(&spec,fsRdPerm,&fRefNum) != noErr)
        goto game_not_registered;

	FSRead(fRefNum,&numBytes,gRegInfo);

    FSClose(fRefNum);

            /* VALIDATE IT */

    if (!ValidateSerialNumber(gRegInfo))
        goto game_not_registered;

    gGameIsRegistered = true;

    return;

        /* GAME IS NOT REGISTERED YET, SO DO DIALOG */

game_not_registered:

    DoSerialDialog();

    if (gGameIsRegistered)                                  // see if write out reg file
    {
	    FSpDelete(&spec);	                                // delete existing file if any
	    iErr = FSpCreate(&spec,kGameID,'xxxx',-1);
        if (iErr == noErr)
        {
        	numBytes = SERIAL_LENGTH;
			FSpOpenDF(&spec,fsRdWrPerm,&fRefNum);
			FSWrite(fRefNum,&numBytes,gRegInfo);
		    FSClose(fRefNum);

     	}
    }


}


/********************* VALIDATE REGISTRATION NUMBER ******************/
//
// Return true if is valid
//

static Boolean ValidateSerialNumber(unsigned char *regInfo)
{
#define	NUM_PIRATE_SERIALS	19
FSSpec	spec;
u_long	customerID, checksum, i;
u_long	seed0,seed1,seed2, validSerial, enteredSerial;
u_char	shift;
int		j,c;
Handle	hand;
const unsigned char pirateNumbers[NUM_PIRATE_SERIALS][SERIAL_LENGTH*2] =
{
	"A%A%A%A%A%A%M%M%M%M%M%M%",			// put "%" in there to confuse pirates scanning for this data
	"C%E%N%D%R%M%H%C%Q%G%P%R%",
	"G%C%R%H%K%J%Q%G%M%I%G%H%",
	"G%N%L%G%F%M%H%K%C%N%K%F%",
	"G%B%E%R%O%O%E%I%G%P%C%E%",
	"G%I%P%H%J%Q%D%D%K%H%N%N%",
	"G%D%H%Q%G%G%K%O%P%C%O%H%",
	"G%N%D%I%F%C%G%J%G%I%N%L%",
	"J%B%L%I%I%C%J%R%Q%J%C%G%",
	"M%J%E%L%P%C%C%D%I%G%P%I%",
	"P%A%B%L%K%4%9%H%E%Q%N%R%",
	"H%F%O%P%F%L%I%C%R%H%R%R%",
	"H%F%J%M%I%L%I%M%M%H%M%H%",
	"I%Y%J%T%K%O%M%P%N%L%K%Q%",
	"N%D%K%O%E%K%O%H%F%D%R%K%",
	"O%P%J%S%J%M%I%I%R%G%F%R%",
	"I%E%J%I%H%C%L%G%K%G%J%D%",
	"I%Y%J%T%K%O%M%P%N%L%K%Q%",
	"I%F%O%L%R%H%N%F%J%E%H%L%",
};


	if (gShareware)
	{

				/*************************/
	            /* VALIDATE ENTERED CODE */
	            /*************************/

			/* CONVERT TO UPPER CASE */

	    for (i = 0; i < SERIAL_LENGTH; i++)
	    {
	    	if ((regInfo[i] >= '0') && (regInfo[i] <= '9'))		// if any numbers then its a pirate number
	    		return(false);

			if ((regInfo[i] >= 'a') && (regInfo[i] <= 'z'))
				regInfo[i] = 'A' + (regInfo[i] - 'a');
		}


	    	/* THE FIRST 4 DIGITS ARE THE CUSTOMER INDEX */

	    customerID  = (regInfo[0] - 'G') * 0x1000;				// convert G,H,I,J, K,L,M,N, O,P,Q,R, S,T,U,V to 0x1000...0xf000
	    customerID += (regInfo[1] - 'A') * 0x0100;				// convert A,B,C,D, E,F,G,H, I,J,K,L, M,N,O,P to 0x0100...0x0f00
	    customerID += (regInfo[2] - 'C') * 0x0010;				// convert C,D,E,F, G,H,I,J, K,L,M,N, O,P,Q,R to 0x0010...0x00f0
	    customerID += (regInfo[3] - 'G') * 0x0001;				// convert G,H,I,J, K,L,M,N, O,P,Q,R, S,T,U,V to 0x0001...0x000f

		if (customerID < 200)									// there are no customers under 200 since we want to confuse pirates
			return(false);
		if (customerID > 40000)									// also assume not over 40,000
			return(false);

			/* NOW SEE WHAT THE SERIAL SHOULD BE */

		seed0 = 0x2a80ce30;										// init random seed
		seed1 = 0xf032343a;
		seed2 = 0x8CA77EE9;

		for (i = 0; i < customerID; i++)						// calculate the random serial
  			seed2 ^= (((seed1 ^= (seed2>>5)*1568397607UL)>>7)+(seed0 = (seed0+1)*3141592621UL))*2435386481UL;

		validSerial = seed2;


			/* CONVERT ENTERED SERIAL STRING TO NUMBER */

		shift = 0;
		enteredSerial = 0;
		for (i = SERIAL_LENGTH-1; i >= 4; i--)						// start @ last digit
		{
			u_long 	digit = regInfo[i] - 'C';					// convert C,D,E,F, G,H,I,J, K,L,M,N, O,P,Q,R to 0x0..0xf
			enteredSerial += digit << shift;					// shift digit into posiion
			shift += 4;											// set to insert next nibble
		}

				/* SEE IF IT MATCHES */

		if (enteredSerial != validSerial)
			return(false);



				/**********************************/
				/* CHECK FOR KNOWN PIRATE NUMBERS */
				/**********************************/

					/* FIRST VERIFY OUR TABLE */

		if ((pirateNumbers[0][1] != '%') ||									// we do this to see if priates have cleared out our table
			(pirateNumbers[2][0] != 'G') ||
			(pirateNumbers[2][2] != 'C') ||
			(pirateNumbers[1][0] != 'C') ||
			(pirateNumbers[5][16] != 'K') ||
			(pirateNumbers[2][12] != 'Q') ||
			(pirateNumbers[7][12] != 'G') ||
			(pirateNumbers[10][12] != '9') ||
			(pirateNumbers[3][20] != 'K'))
		{
corrupt:
			DoFatalAlert("This application is corrupt.  You should reinstall a fresh copy of the game.");
			return(false);
		}


				/* CHECKSUM */

		checksum = 0;
		for (i = 0; i < NUM_PIRATE_SERIALS; i++)
		{
			for (j = 0; j < (SERIAL_LENGTH*2); j++)
				checksum += (u_long)pirateNumbers[i][j];
		}

#if 0
		ShowSystemErr(checksum);							// each time we change the table we need to update the hard-coded checksum
#else
		if (checksum != 25314)								// does the checksum match?
			goto corrupt;

#endif



				/* THEN SEE IF THIS CODE IS IN THE TABLE */

		for (j = 0; j < NUM_PIRATE_SERIALS; j++)
		{
			for (i = 0; i < SERIAL_LENGTH; i++)
			{
				if (regInfo[i] != pirateNumbers[j][i*2])					// see if doesn't match
					goto next_code;
			}

					/* THIS CODE IS PIRATED */

			return(false);

	next_code:;
		}


				/*******************************/
				/* SECONDARY CHECK IN REZ FILE */
				/*******************************/
				//
				// The serials are stored in the Level 1 terrain file
				//

		if (FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Terrain:Level1_Garden.ter", &spec) == noErr)		// open rez fork
		{
			short fRefNum = FSpOpenResFile(&spec,fsRdPerm);

			UseResFile(fRefNum);						// set app rez

			c = Count1Resources('savs');						// count how many we've got stored
			for (j = 0; j < c; j++)
			{
				hand = GetResource('savs',128+j);			// read the #

				for (i = 0; i < SERIAL_LENGTH; i++)
				{
					if (regInfo[i] != (*hand)[i])			// see if doesn't match
						goto next2;
				}

						/* THIS CODE IS PIRATED */

				return(false);

		next2:
				ReleaseResource(hand);
			}

		}

	    return(true);
	}

			/******************************************************/
			/* THIS IS THE BOXED VERSION, SO VERIFY THE FAUX CODE */
			/******************************************************/
	else
	{
		int	i;
		static unsigned char fauxCode[SERIAL_LENGTH] = "PANG00195846";

		for (i = 0 ; i < SERIAL_LENGTH; i++)
		{
			if ((regInfo[i] >= 'a') && (regInfo[i] <= 'z'))		// conver to upper-case
				regInfo[i] = 'A' + (regInfo[i] - 'a');

			if (regInfo[i] != fauxCode[i])						// see if doesn't match
				return(false);
		}
	    return(true);
	}
}



/****************** DO SERIAL DIALOG *************************/

		/* CHANGE NAME TO CONFUSE HACKERS */

static void DoSerialDialog(void)
{
OSErr			err;
EventHandlerRef	ref;

EventTypeSpec	list[] = { { kEventClassCommand,  kEventProcessCommand } };

const char		*rezNames[MAX_LANGUAGES] =
{
	"Shareware_English",
	"Shareware_French",
	"Shareware_German",
	"Shareware_Spanish",
	"Shareware_Italian",
	"Shareware_Swedish",
	"Shareware_Dutch",
};

const char		*retailRezNames[MAX_LANGUAGES] =
{
	"Retail_English",
	"Retail_French",
	"Retail_German",
	"Retail_Spanish",
	"Retail_Italian",
	"Retail_Swedish",
	"Retail_Dutch",
};


	Enter2D();

    		/***************/
    		/* INIT DIALOG */
    		/***************/

	if (gGamePrefs.language >= MAX_LANGUAGES)			// check for corruption
		gGamePrefs.language = LANGUAGE_ENGLISH;


				/* CREATE WINDOW FROM THE NIB */

	if (gShareware)
	{
		err = CreateWindowFromNib(gNibs, CFStringCreateWithCString(nil, rezNames[gGamePrefs.language],
								kCFStringEncodingMacRoman), &gDialogWindow);
	}
	else
	{
		err = CreateWindowFromNib(gNibs, CFStringCreateWithCString(nil, retailRezNames[gGamePrefs.language],
								kCFStringEncodingMacRoman), &gDialogWindow);
	}
	if (err)
		DoFatalAlert("GamepadInit: CreateWindowFromNib failed!");


			/* CREATE NEW WINDOW EVENT HANDLER */

    gWinEvtHandler = NewEventHandlerUPP(SerialDialog_EventHandler);
    InstallWindowEventHandler(gDialogWindow, gWinEvtHandler, GetEventTypeCount(list), list, 0, &ref);



			/* PROCESS THE DIALOG */

    ShowWindow(gDialogWindow);
	RunAppModalLoopForWindow(gDialogWindow);


				/* CLEANUP */

	DisposeEventHandlerUPP (gWinEvtHandler);
	DisposeWindow (gDialogWindow);


	Exit2D();
}


/****************** DO SERIAL DIALOG EVENT HANDLER *************************/
//
// main window event handling
//

static pascal OSStatus SerialDialog_EventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData)
{
#pragma unused (myHandler, userData)
OSStatus			result = eventNotHandledErr;
ControlID 			idControl;
ControlRef 			control;
OSStatus 			err = noErr;
HICommand 			command;
Size				actualSize;

	switch(GetEventKind(event))
	{

				/*******************/
				/* PROCESS COMMAND */
				/*******************/

		case	kEventProcessCommand:
				GetEventParameter (event, kEventParamDirectObject, kEventParamHICommand, NULL, sizeof(command), NULL, &command);
				switch(command.commandID)
				{
							/******************/
							/* "ENTER" BUTTON */
							/******************/

					case	'ok  ':

								/* GET THE SERIAL STRING FROM THE TEXTEDIT CONTROL */

						    idControl.signature = 'serl';
						    idControl.id 		= 0;
						    err = GetControlByID(gDialogWindow, &idControl, &control);
							err |= GetControlData(control, 0, kControlEditTextTextTag, 64, gRegInfo, &actualSize);
							if (err)
						    	DoFatalAlert("GamepadInit_EventHandler: GetControlData failed!");


									/* VALIDATE THE NUMBER */

		                    if (ValidateSerialNumber(gRegInfo) == true)
		                    {
		                        gGameIsRegistered = true;
		                        QuitAppModalLoopForWindow(gDialogWindow);
		             		}
		                    else
		                    {
		                        DoAlert("Sorry, that serial number is not valid.  Please try again.");
		                    }
		                    break;


							/*****************/
							/* "QUIT" BUTTON */
							/*****************/

					case	'quit':
							ExitToShell();
							break;


							/*****************/
							/* "DEMO" BUTTON */
							/*****************/

					case	'demo':
	                        QuitAppModalLoopForWindow(gDialogWindow);
							break;


							/****************/
							/* "URL" BUTTON */
							/****************/

					case	'url ':
							if (LaunchURL("http://www.pangeasoft.net/bug2/serials.html") == noErr)
			                    ExitToShell();
			              	break;


				}
				break;
    }

    return (result);
}


#pragma mark -




/********************* CONVERT INT TO PSTR *************************/

void ConvertIntToPStr(int num, StringPtr s)
{
CFStringRef	cfstr;

	cfstr = CFStringCreateWithFormat(kCFAllocatorDefault, nil, CFSTR("%d"), num);		// creates a CFString based on printf style formatting

	if (!CFStringGetPascalString(cfstr, s, 255, kCFStringEncodingASCII))
		DoFatalAlert("ConvertIntToPStr:  CFStringGetPascalString() failed!");

}




#pragma mark -




/********************* SWIZZLE SHORT **************************/

short SwizzleShort(short *shortPtr)
{
short	theShort = *shortPtr;

#if __LITTLE_ENDIAN__

	Byte	b1 = theShort & 0xff;
	Byte	b2 = (theShort & 0xff00) >> 8;

	theShort = (b1 << 8) | b2;
#endif

	return(theShort);
}


/********************* SWIZZLE USHORT **************************/

u_short SwizzleUShort(u_short *shortPtr)
{
u_short	theShort = *shortPtr;

#if __LITTLE_ENDIAN__

	Byte	b1 = theShort & 0xff;
	Byte	b2 = (theShort & 0xff00) >> 8;

	theShort = (b1 << 8) | b2;
#endif

	return(theShort);
}



/********************* SWIZZLE LONG **************************/

long SwizzleLong(long *longPtr)
{
long	theLong = *longPtr;

#if __LITTLE_ENDIAN__

	Byte	b1 = theLong & 0xff;
	Byte	b2 = (theLong & 0xff00) >> 8;
	Byte	b3 = (theLong & 0xff0000) >> 16;
	Byte	b4 = (theLong & 0xff000000) >> 24;

	theLong = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;

#endif

	return(theLong);
}


/********************* SWIZZLE U LONG **************************/

u_long SwizzleULong(u_long *longPtr)
{
u_long	theLong = *longPtr;

#if __LITTLE_ENDIAN__

	Byte	b1 = theLong & 0xff;
	Byte	b2 = (theLong & 0xff00) >> 8;
	Byte	b3 = (theLong & 0xff0000) >> 16;
	Byte	b4 = (theLong & 0xff000000) >> 24;

	theLong = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;

#endif

	return(theLong);
}



/********************* SWIZZLE FLOAT **************************/

float SwizzleFloat(float *floatPtr)
{
float	*theFloat;
u_long	bytes = SwizzleULong((u_long *)floatPtr);

	theFloat = (float *)&bytes;

	return(*theFloat);
}


/************************ SWIZZLE POINT 3D ********************/

void SwizzlePoint3D(OGLPoint3D *pt)
{
	pt->x = SwizzleFloat(&pt->x);
	pt->y = SwizzleFloat(&pt->y);
	pt->z = SwizzleFloat(&pt->z);

}


/************************ SWIZZLE VECTOR 3D ********************/

void SwizzleVector3D(OGLVector3D *pt)
{
	pt->x = SwizzleFloat(&pt->x);
	pt->y = SwizzleFloat(&pt->y);
	pt->z = SwizzleFloat(&pt->z);

}

/************************ SWIZZLE UV ********************/

void SwizzleUV(OGLTextureCoord *pt)
{
	pt->u = SwizzleFloat(&pt->u);
	pt->v = SwizzleFloat(&pt->v);

}


#pragma mark -

/******************** RGB TO HSV **********************/
//
// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
// if s == 0, then h = -1 (undefined)
//

void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v )
{
float min, max, delta;

//	min = MIN( r, g, b );
	min = r;
	if (g < min)
		min = g;
	if (b < min)
		min = b;

//	max = MAX( r, g, b );
	max = r;
	if (g > max)
		max = g;
	if (b > max)
		max = b;

	*v = max;				// v
	delta = max - min;
	if( max != 0 )
		*s = delta / max;		// s
	else {
		// r = g = b = 0		// s = 0, v is undefined
		*s = 0;
		*h = -1;
		return;
	}
	if( r == max )
		*h = ( g - b ) / delta;		// between yellow & magenta
	else if( g == max )
		*h = 2 + ( b - r ) / delta;	// between cyan & yellow
	else
		*h = 4 + ( r - g ) / delta;	// between magenta & cyan
	*h *= 60;				// degrees
	if( *h < 0 )
		*h += 360;
}

/***************** HSV TO RGB ***************************/

void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
{
int		i;
float	f, p, q, t;

	if (s == 0)
	{
		// achromatic (grey)
		*r = *g = *b = v;
		return;
	}

	h /= 60;			// sector 0 to 5
	i = floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );
	switch( i )
	{
		case 0:
			*r = v;
			*g = t;
			*b = p;
			break;
		case 1:
			*r = q;
			*g = v;
			*b = p;
			break;
		case 2:
			*r = p;
			*g = v;
			*b = t;
			break;
		case 3:
			*r = p;
			*g = q;
			*b = v;
			break;
		case 4:
			*r = t;
			*g = p;
			*b = v;
			break;
		default:		// case 5:
			*r = v;
			*g = p;
			*b = q;
			break;
	}
}







