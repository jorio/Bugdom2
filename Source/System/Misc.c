/****************************/
/*      MISC ROUTINES       */
/* By Brian Greenstone      */
/* (c)2002 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include <stdarg.h>
#include "game.h"


/****************************/
/*    CONSTANTS             */
/****************************/

#define	DEFAULT_FPS			10
#define	MAX_FPS				300

#define	USE_MALLOC		1



/**********************/
/*     VARIABLES      */
/**********************/


uint32_t 	gSeed0 = 0, gSeed1 = 0, gSeed2 = 0;

float	gFramesPerSecond, gFramesPerSecondFrac;

int		gNumPointers = 0;



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

	SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, PROJECT_FULL_NAME " Alert: %s\n", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, PROJECT_FULL_NAME, message, gSDLWindow);

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

	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, PROJECT_FULL_NAME " Fatal Alert: %s\n", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, PROJECT_FULL_NAME, message, gSDLWindow);

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

		DeleteAllObjects();
		DisposeTerrain();								// dispose of any memory allocated by terrain manager
		DisposeAllBG3DContainers();						// nuke all models
		DisposeAllSpriteGroups();						// nuke all sprites
		DisposeAllSpriteAtlases();						// nuke all atlases

		if (gGameView)							// see if need to dispose this
			OGL_DisposeWindowSetup(&gGameView);

		ShutdownSound();								// cleanup sound stuff
	}

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
uint32_t	*cookiePtr;

	size += 16;								// make room for our cookie & whatever else (also keep to 16-byte alignment!)

#if USE_MALLOC
	pr = malloc(size);
#else
	pr = NewPtr(size);
#endif
	if (pr == nil)
		DoFatalAlert("AllocPtr: NewPtr failed");

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
	pr = calloc(1, size);
#else
	pr = NewPtrClear(size);						// alloc in Application
#endif

	if (pr == nil)
		DoFatalAlert("AllocPtr: NewPtr failed");

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


/************** CALC FRAMES PER SECOND *****************/
//
// This version uses UpTime() which is only available on PCI Macs.
//

void CalcFramesPerSecond(void)
{
static UnsignedWide time;
UnsignedWide currTime;
unsigned long deltaTime;
float		fps;

wait:
	Microseconds(&currTime);

	deltaTime = currTime.lo - time.lo;

	if (deltaTime == 0)
	{
		fps = DEFAULT_FPS;
	}
	else
	{
		fps = 1000000.0f / deltaTime;

		if (fps < DEFAULT_FPS)					// (avoid divide by 0's later)
		{
			fps = DEFAULT_FPS;
		}
		else if (fps > MAX_FPS)					// limit to avoid issue
		{
			if (fps - MAX_FPS > 1000)			// try to sneak in some sleep if we have 1 ms to spare
			{
				SDL_Delay(1);
			}
			goto wait;
		}
	}

#if _DEBUG
	if (GetKeyState(SDL_SCANCODE_BACKSLASH))	// debug speed-up with backslash key
		fps = DEFAULT_FPS;
#endif

//	SDL_Log("FPS: %f\n", fps);

	gFramesPerSecond = fps;
	gFramesPerSecondFrac = 1.0f / fps;

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
	IMPLEMENT_ME_SOFT();
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







