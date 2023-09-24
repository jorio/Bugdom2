/****************************/
/*        WINDOWS           */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include	"game.h"



/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveFadeEvent(ObjNode *theNode);

static void CreateDisplayModeList(void);
static void CalcVRAMAfterBuffers(void);
static void GetDisplayVRAM(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define		DO_GAMMA	0

/**********************/
/*     VARIABLES      */
/**********************/

Boolean				gRealCursorVisible = true;


uint32_t			gDisplayVRAM = 0;
uint32_t			gVRAMAfterBuffers = 0;

long					gScreenXOffset,gScreenYOffset;

float		gGammaFadePercent = 1.0;

int				gGameWindowWidth, gGameWindowHeight;

short			g2DStackDepth = 0;

Boolean			gPlayFullScreen;

#if 0
CGGammaValue gOriginalRedTable[256];
CGGammaValue gOriginalGreenTable[256];
CGGammaValue gOriginalBlueTable[256];

static CGGammaValue gGammaRedTable[256];
static CGGammaValue gGammaGreenTable[256];
static CGGammaValue gGammaBlueTable[256];
#endif

float		gGammaTweak = 1.0f;


/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
	gPlayFullScreen = true;				//!gGamePrefs.playInWindow;

	IMPLEMENT_ME_SOFT();
	gGameWindowWidth = 640;
	gGameWindowHeight = 480;
	SDL_GL_GetDrawableSize(gSDLWindow, &gGameWindowWidth, &gGameWindowHeight);
#if 0
	if (gPlayFullScreen)
	{

		CGDisplayCapture(gCGDisplayID);

				/* FIND BEST MATCH */

		refDisplayMode = CGDisplayBestModeForParametersAndRefreshRate(gCGDisplayID, gGamePrefs.depth,
																	gGamePrefs.screenWidth, gGamePrefs.screenHeight,
																	gGamePrefs.hz,
																	NULL);
		if (refDisplayMode == nil)
			DoFatalAlert("InitWindowStuff: CGDisplayBestModeForParameters failed!");


				/* SWITCH TO IT */

		CGDisplaySwitchToMode (gCGDisplayID, refDisplayMode);


					/* GET GDEVICE & INFO */

		DMGetGDeviceByDisplayID ((DisplayIDType)gCGDisplayID, &gGDevice, false);

		w = gGamePrefs.screenWidth;
		h = gGamePrefs.screenHeight;

		r.top  		= (short) ((**gGDevice).gdRect.top + ((**gGDevice).gdRect.bottom - (**gGDevice).gdRect.top) / 2);	  	// h center
		r.top  		-= (short) (h / 2);
		r.left  	= (short) ((**gGDevice).gdRect.left + ((**gGDevice).gdRect.right - (**gGDevice).gdRect.left) / 2);		// v center
		r.left  	-= (short) (w / 2);
		r.right 	= (short) (r.left + w);
		r.bottom 	= (short) (r.top + h);




					/* GET ORIGINAL GAMMA TABLE */

		CGGetDisplayTransferByTable(gCGDisplayID, 256, gOriginalRedTable, gOriginalGreenTable, gOriginalBlueTable, &sampleCount);

	}


				/********************/
				/* INIT SOME WINDOW */
				/********************/
	else
	{
		int			w;
		gGDevice = GetMainDevice();

				/* SIZE WINDOW TO HALF SCREEN SIZE */

		w = (**gGDevice).gdRect.right;				// get width/height of display
		h = (**gGDevice).gdRect.bottom;

		r.left = r.top = 0;
//		r.right = w / 2;
//		r.bottom = h / 2;
		r.right = w * .99;
		r.bottom = h * .95;

		if (r.right < 640)			// keep minimum at 640 wide
		{
			r.right = 640;
			r.bottom = 480;
		}

		gGameWindow = NewCWindow(nil, &r, "", false, plainDBox, (WindowPtr)-1L, false, 0);


		gGameWindowGrafPtr = GetWindowPort(gGameWindow);

				/* MOVE WINDOW TO CENTER OF SCREEN */

//		MoveWindow(gGameWindow, r.right/2, r.bottom/2, true);
		MoveWindow(gGameWindow, 50, 30, true);
		ShowWindow(gGameWindow);
	}



	gGameWindowWidth = r.right - r.left;
	gGameWindowHeight = r.bottom - r.top;
#endif
}



#pragma mark -


/**************** GAMMA FADE IN *************************/

void GammaFadeIn(void)
{
#if DO_GAMMA
int			i;

	if (!gPlayFullScreen)							// only do gamma in full-screen mode
		return;


	while(gGammaFadePercent < 1.0f)
	{
		gGammaFadePercent += .07f;
		if (gGammaFadePercent > 1.0f)
			gGammaFadePercent = 1.0f;

	    for (i = 0; i < 256 ; i++)
	    {
	        gGammaRedTable[i] 	= gOriginalRedTable[i] * gGammaFadePercent * gGammaTweak;
	        gGammaGreenTable[i] = gOriginalGreenTable[i] * gGammaFadePercent * gGammaTweak;
	        gGammaBlueTable[i] 	= gOriginalBlueTable[i] * gGammaFadePercent * gGammaTweak;
	    }

		Wait(1);

		CGSetDisplayTransferByTable( 0, 256, gGammaRedTable, gGammaGreenTable, gGammaBlueTable);
	}
#endif
}


/**************** GAMMA FADE OUT *************************/

void GammaFadeOut(void)
{
#if DO_GAMMA

int			i;

	if (!gPlayFullScreen)							// only do gamma in full-screen mode
		return;

	while(gGammaFadePercent > 0.0f)
	{
		gGammaFadePercent -= .07f;
		if (gGammaFadePercent < 0.0f)
			gGammaFadePercent = 0.0f;

	    for (i = 0; i < 256 ; i++)
	    {
	        gGammaRedTable[i] 	= gOriginalRedTable[i] * gGammaFadePercent * gGammaTweak;
	        gGammaGreenTable[i] = gOriginalGreenTable[i] * gGammaFadePercent * gGammaTweak;
	        gGammaBlueTable[i] 	= gOriginalBlueTable[i] * gGammaFadePercent * gGammaTweak;
	    }

		Wait(1);

		CGSetDisplayTransferByTable( 0, 256, gGammaRedTable, gGammaGreenTable, gGammaBlueTable);

	}
#endif
}

/********************** GAMMA ON *********************/

void GammaOn(void)
{
#if DO_GAMMA

	if (!gPlayFullScreen)
		return;

	if (gGammaFadePercent != 1.0f)
	{
		gGammaFadePercent = 1.0f;

	 	CGSetDisplayTransferByTable(0, 256, gOriginalRedTable, gOriginalGreenTable, gOriginalBlueTable);
	}
#endif
}



/********************** GAMMA OFF *********************/

void GammaOff(void)
{
#if DO_GAMMA

	if (!gPlayFullScreen)
		return;

	if (gGammaFadePercent != 0.0f)
	{
		int			i;

		gGammaFadePercent = 0.0f;

	    for (i = 0; i < 256 ; i++)
	    {
	        gGammaRedTable[i] 	= gOriginalRedTable[i] * gGammaFadePercent * gGammaTweak;
	        gGammaGreenTable[i] = gOriginalGreenTable[i] * gGammaFadePercent * gGammaTweak;
	        gGammaBlueTable[i] 	= gOriginalBlueTable[i] * gGammaFadePercent * gGammaTweak;
	    }

		CGSetDisplayTransferByTable( 0, 256, gGammaRedTable, gGammaGreenTable, gGammaBlueTable);
	}
#endif
}




/****************** CLEANUP DISPLAY *************************/

void CleanupDisplay(void)
{
}


/******************** MAKE FADE EVENT *********************/
//
// INPUT:	fadeIn = true if want fade IN, otherwise fade OUT.
//

void MakeFadeEvent(Boolean fadeIn, float fadeSpeed)
{
ObjNode	*newObj;
ObjNode		*thisNodePtr;

		/* SCAN FOR OLD FADE EVENTS STILL IN LIST */

	thisNodePtr = gFirstNodePtr;

	while (thisNodePtr)
	{
		if (thisNodePtr->MoveCall == MoveFadeEvent)
		{
			thisNodePtr->Flag[0] = fadeIn;								// set new mode
			return;
		}
		thisNodePtr = thisNodePtr->NextNode;							// next node
	}


		/* MAKE NEW FADE EVENT */

	gNewObjectDefinition.genre = EVENT_GENRE;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = SLOT_OF_DUMB + 1000;
	gNewObjectDefinition.moveCall = MoveFadeEvent;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->Flag[0] = fadeIn;

	newObj->Speed = fadeSpeed;
}


/***************** MOVE FADE EVENT ********************/

static void MoveFadeEvent(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	speed = theNode->Speed * fps;

			/* SEE IF FADE IN */

	if (theNode->Flag[0])
	{
		gGammaFadePercent += speed;
		if (gGammaFadePercent >= 1.0f)										// see if @ 100%
		{
			gGammaFadePercent = 1.0f;
			DeleteObject(theNode);
		}
	}

			/* FADE OUT */
	else
	{
		gGammaFadePercent -= speed;
		if (gGammaFadePercent <= 0.0f)													// see if @ 0%
		{
			gGammaFadePercent = 0;
			DeleteObject(theNode);
		}
	}


	if (gPlayFullScreen)
	{
		IMPLEMENT_ME_SOFT();
#if 0
		int			i;

	    for (i = 0; i < 256 ; i++)
	    {
	        gGammaRedTable[i] 	= gOriginalRedTable[i] * gGammaFadePercent * gGammaTweak;
	        gGammaGreenTable[i] = gOriginalGreenTable[i] * gGammaFadePercent * gGammaTweak;
	        gGammaBlueTable[i] 	= gOriginalBlueTable[i] * gGammaFadePercent * gGammaTweak;
	    }

		CGSetDisplayTransferByTable( 0, 256, gGammaRedTable, gGammaGreenTable, gGammaBlueTable);
#endif
	}
}



/************************ GAME SCREEN TO BLACK ************************/

void GameScreenToBlack(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
Rect	r;

	if (!gDisplayContextGrafPtr)
		return;

	SetPort(gDisplayContextGrafPtr);
	BackColor(blackColor);

	GetPortBounds(gDisplayContextGrafPtr, &r);
	EraseRect(&r);
#endif
}


/************************** ENTER 2D *************************/
//
// For OS X - turn off DSp when showing 2D
//

void Enter2D(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
	ShowRealCursor();
	MyFlushEvents();

	g2DStackDepth++;
	if (g2DStackDepth > 1)						// see if already in 2D
	{
		GammaOn();
		return;
	}

	if (gPlayFullScreen)
	{
		GammaOff();

		if (gAGLContext)
		{
			glFlush();
			glFinish();

			aglSetDrawable(gAGLContext, nil);		// diable GL so our dialogs will show up
			glFlush();
			glFinish();
		}

			/* NEED TO UN-CAPTURE THE CG DISPLAY */

		CGDisplayRelease(gCGDisplayID);
	}

	GammaOn();
#endif
}


/************************** EXIT 2D *************************/
//
// For OS X - turn ON DSp when NOT 2D
//

void Exit2D(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
	g2DStackDepth--;
	if (g2DStackDepth > 0)			// don't exit unless on final exit
		return;

	HideRealCursor();


	if (gPlayFullScreen)
	{
//		if (gAGLContext)
		{
			CGDisplayCapture(gCGDisplayID);
			if (gAGLContext)
				aglSetFullScreen(gAGLContext, 0, 0, 0, 0);		//re-enable GL
		}
	}
#endif
}


/********************** WAIT **********************/

void Wait(uint32_t ticks)
{
uint32_t	start;

	start = TickCount();

	while (TickCount()-start < ticks)
		MyFlushEvents();

}


#pragma mark -

/*********************** CALC VRAM AFTER BUFFERS ***********************/
//
// CALC HOW MUCH VRAM REMAINS AFTER OUR BUFFERS
//

static void CalcVRAMAfterBuffers(void)
{
int	bufferSpace;

	bufferSpace = gGameWindowWidth * gGameWindowHeight * 2 * 2;		// calc main pixel/z buffers @ 16-bit
	bufferSpace *= 2;	// 32-bit

	gVRAMAfterBuffers = gDisplayVRAM - bufferSpace;
}


#pragma mark -

/********************* HIDE REAL CURSOR *********************/

void HideRealCursor(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
	if (gRealCursorVisible)
	{
		CGDisplayHideCursor(gCGDisplayID);
		gRealCursorVisible = false;
	}
#endif
}


/********************* SHOW REAL CURSOR *********************/

void ShowRealCursor(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
	if (!gRealCursorVisible)
	{
		CGDisplayShowCursor(gCGDisplayID);
		gRealCursorVisible = true;
	}
#endif
}





