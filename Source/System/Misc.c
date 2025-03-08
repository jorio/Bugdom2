/****************************/
/*      MISC ROUTINES       */
/* By Brian Greenstone      */
/* (c)2002 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/


uint32_t 	gSeed0 = 0, gSeed1 = 0, gSeed2 = 0;

float	gFramesPerSecond, gFramesPerSecondFrac;

int		gNumPointers = 0;

static int gDeltaTimeSampleIndex = -1;
static int gDeltaTimeSampleRing[STEADY_FPS_WINDOW] = { 0 };


/**********************/
/*     PROTOTYPES     */
/**********************/


/*********************** DO ALERT *******************/

void DoAlert(const char* format, ...)
{
	Enter2D();

	char message[1024];
	va_list args;
	va_start(args, format);
	SDL_vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Game Alert: %s\n", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GAME_FULL_NAME, message, gSDLWindow);

	Exit2D();
}


/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(const char* format, ...)
{
	Enter2D();

	char message[1024];
	va_list args;
	va_start(args, format);
	SDL_vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Game Fatal Alert: %s\n", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GAME_FULL_NAME, message, gSDLWindow);

	Exit2D();
	CleanQuit();
}


/************ CLEAN QUIT ***************/

void CleanQuit(void)
{
static Boolean	beenHere = false;

	if (!beenHere)
	{
		beenHere = true;

		SavePrefs();									// save prefs before bailing

		DisposeObjectManager();
		DisposeSkeletonManager();
		DisposeEffects();
		DisposeTunnelData();
		DisposeTerrain();								// dispose of any memory allocated by terrain manager
		DisposeWater();
		DisposeAllBG3DContainers();						// nuke all models
		DisposeAllSpriteGroups();						// nuke all sprites
		DisposeAllSpriteAtlases();						// nuke all atlases

		if (gGameView.isActive)							// see if need to dispose this
			OGL_DisposeWindowSetup(&gGameView);

		ShutdownSound();								// cleanup sound stuff
		DisposeLocalizedStrings();
		DisposeInput();
	}

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

uint32_t MyRandomLong(void)
{
  return gSeed2 ^= (((gSeed1 ^= (gSeed2>>5)*1568397607UL)>>7)+
                   (gSeed0 = (gSeed0+1)*3141592621UL))*2435386481UL;
}


/************************* RANDOM RANGE *************************/
//
// THE RANGE *IS* INCLUSIVE OF MIN AND MAX
//

uint16_t	RandomRange(unsigned short min, unsigned short max)
{
uint16_t		qdRdm;											// treat return value as 0-65536
uint32_t		range, t;

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
	gSeed0 = (uint32_t) seed;
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


/****************** ALLOC PTR ********************/

void *AllocPtr(long size)
{
Ptr	pr;
uint32_t	*cookiePtr;

	size += 16;								// make room for our cookie & whatever else (also keep to 16-byte alignment!)

#if USE_MALLOC
	pr = SDL_malloc(size);
#else
	pr = NewPtr(size);
#endif
	GAME_ASSERT(pr);

	cookiePtr = (uint32_t *)pr;

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
uint32_t	*cookiePtr;

	size += 16;								// make room for our cookie & whatever else (also keep to 16-byte alignment!)

#if USE_MALLOC
	pr = SDL_calloc(1, size);
#else
	pr = NewPtrClear(size);						// alloc in Application
#endif

	GAME_ASSERT(pr);

	cookiePtr = (uint32_t *)pr;

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
uint32_t	*cookiePtr;
Ptr		p = ptr;

	p -= 16;					// back up to pt to cookie

	cookiePtr = (uint32_t *)p;

	GAME_ASSERT(*cookiePtr == 'FACE');

	*cookiePtr = 0;

#if USE_MALLOC
	SDL_free(p);
#else
	DisposePtr(p);
#endif

	gNumPointers--;
}



#pragma mark -


/************** CALC FRAMES PER SECOND *****************/

void CalcFramesPerSecond(void)
{
static const int		kMaxDelta = TIME_UNIT / MIN_FPS;
static const int		kFallbackDelta = TIME_UNIT / DEFAULT_FPS;

UnsignedWide			currTime;
static UnsignedWide		time = {0};


		/* SEE IF RESET */

	if (gDeltaTimeSampleIndex < 0)
	{
		// Init sample ring with 60 fps deltas
		for (size_t i = 0; i < STEADY_FPS_WINDOW; i++)
		{
			gDeltaTimeSampleRing[i] = kFallbackDelta;
		}

		gDeltaTimeSampleIndex = 0;

		// Reset initial time
		Microseconds(&time);
	}


wait:
	Microseconds(&currTime);

	unsigned long deltaTime = currTime.lo - time.lo;

	if (deltaTime == 0)
	{
		// First call since reset - Assume default framerate
		deltaTime = kFallbackDelta;
	}
	else if (deltaTime > kMaxDelta)
	{
		// Pin to max delta (min FPS)
		deltaTime = kMaxDelta;
	}
	else
	{
#if !COOK_GPU
		float fps = (float)TIME_UNIT / deltaTime;
		if (fps > MAX_FPS)						// limit framerate to avoid frying the GPU and floating point issues
		{
			if (fps - MAX_FPS > 1000)			// do we have 1 millisecond to spare?
			{
				SDL_Delay(1);					// try to sneak in some sleep to let the CPU breathe
			}
			goto wait;
		}
#endif
	}

#if _DEBUG
	if (GetKeyState(SDL_SCANCODE_BACKSLASH))	// debug speed-up with backslash key
	{
		deltaTime = kMaxDelta;
	}
#endif


		/* ADD SAMPLE TO RING BUFFER */
		// For steadiness - Keep the delta, not the fps!
		// (During a framerate hiccup, the FPS value varies more wildly than the delta)
		
	gDeltaTimeSampleRing[gDeltaTimeSampleIndex] = deltaTime;
	gDeltaTimeSampleIndex++;
	gDeltaTimeSampleIndex %= STEADY_FPS_WINDOW;


		/* CALC AVERAGE OF ENTIRE RING BUFFER */

	gFramesPerSecondFrac = 0;
	for (int i = 0; i < STEADY_FPS_WINDOW; i++)
	{
		gFramesPerSecondFrac += gDeltaTimeSampleRing[i];
	}
	gFramesPerSecondFrac *= (1.0f / (TIME_UNIT * STEADY_FPS_WINDOW));	// convert to seconds and average

	GAME_DEBUGASSERT(gFramesPerSecondFrac != 0.0f);
	gFramesPerSecond = 1.0f / gFramesPerSecondFrac;


		/* RESET TIME FOR NEXT INTERVAL */

	time = currTime;
}


/************** RESET FRAMES PER SECOND *****************/

void ResetFramesPerSecond(void)
{
	gDeltaTimeSampleIndex = -1;
	gFramesPerSecond = DEFAULT_FPS;
	gFramesPerSecondFrac = 1.0f / DEFAULT_FPS;
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
	InvalidateAllInputs();
}


#pragma mark -


/********************* SWIZZLE SHORT **************************/

int16_t SwizzleShort(const int16_t *shortPtr)
{
	return (int16_t)SwizzleUShort((const uint16_t *)shortPtr);
}


/********************* SWIZZLE USHORT **************************/

uint16_t SwizzleUShort(const uint16_t *shortPtr)
{
uint16_t	theShort = *shortPtr;

#if __LITTLE_ENDIAN__

	uint32_t	b1 = theShort & 0xff;
	uint32_t	b2 = (theShort & 0xff00) >> 8;

	theShort = (b1 << 8) | b2;
#endif

	return(theShort);
}



/********************* SWIZZLE LONG **************************/

int32_t SwizzleLong(const int32_t *longPtr)
{
	return (int32_t) SwizzleULong((const uint32_t*) longPtr);
}


/********************* SWIZZLE U LONG **************************/

uint32_t SwizzleULong(const uint32_t *longPtr)
{
uint32_t	theLong = *longPtr;

#if __LITTLE_ENDIAN__

	uint32_t	b1 = theLong & 0xff;
	uint32_t	b2 = (theLong & 0xff00) >> 8;
	uint32_t	b3 = (theLong & 0xff0000) >> 16;
	uint32_t	b4 = (theLong & 0xff000000) >> 24;

	theLong = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;

#endif

	return(theLong);
}

/********************* SWIZZLE U LONG 64 **************************/


uint64_t SwizzleULong64(const uint64_t* u64Ptr)
{
	uint64_t	u64 = *u64Ptr;

#if __LITTLE_ENDIAN__
	uint64_t	b1 = (u64 >> 0) & 0xff;
	uint64_t	b2 = (u64 >> 8) & 0xff;
	uint64_t	b3 = (u64 >> 16) & 0xff;
	uint64_t	b4 = (u64 >> 24) & 0xff;
	uint64_t	b5 = (u64 >> 32) & 0xff;
	uint64_t	b6 = (u64 >> 40) & 0xff;
	uint64_t	b7 = (u64 >> 48) & 0xff;
	uint64_t	b8 = (u64 >> 56) & 0xff;

	u64 = (b1 << 56)
		| (b2 << 48)
		| (b3 << 40)
		| (b4 << 32)
		| (b5 << 24)
		| (b6 << 16)
		| (b7 << 8)
		| (b8);
#endif

	return u64;
}


/********************* SWIZZLE FLOAT **************************/

float SwizzleFloat(const float *floatPtr)
{
	const void* blob = floatPtr;

	uint32_t theLong = SwizzleULong((const uint32_t *) blob);

	blob = &theLong;

	return *((const float *) blob);
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



/************************ READ INTEGER THEN BYTESWAP ********************/

uint8_t FSReadByte(short refNum)
{
	uint8_t result = 0;
	long count = sizeof(result);
	OSErr err = FSRead(refNum, &count, (Ptr)&result);
	GAME_ASSERT(err == noErr);
	return result;
}

int16_t FSReadBEShort(short refNum)
{
	int16_t result = 0;
	long count = sizeof(result);
	OSErr err = FSRead(refNum, &count, (Ptr)&result);
	GAME_ASSERT(err == noErr);
	return SwizzleShort(&result);
}

uint16_t FSReadBEUShort(short refNum)
{
	uint16_t result = 0;
	long count = sizeof(result);
	OSErr err = FSRead(refNum, &count, (Ptr)&result);
	GAME_ASSERT(err == noErr);
	return SwizzleUShort(&result);
}

int32_t FSReadBELong(short refNum)
{
	int32_t result = 0;
	long count = sizeof(result);
	OSErr err = FSRead(refNum, &count, (Ptr)&result);
	GAME_ASSERT(err == noErr);
	return SwizzleLong(&result);
}

uint32_t FSReadBEULong(short refNum)
{
	uint32_t result = 0;
	long count = sizeof(result);
	OSErr err = FSRead(refNum, &count, (Ptr)&result);
	GAME_ASSERT(err == noErr);
	return SwizzleULong(&result);
}

float FSReadBEFloat(short refNum)
{
	float result = 0;
	long count = sizeof(result);
	OSErr err = FSRead(refNum, &count, (Ptr)&result);
	GAME_ASSERT(err == noErr);
	return SwizzleFloat(&result);
}
