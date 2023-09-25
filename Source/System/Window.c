/****************************/
/*        WINDOW            */
/* By Brian Greenstone      */
/* (c)2002 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include	"game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/


/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/

Boolean				gRealCursorVisible = true;

int				gGameWindowWidth, gGameWindowHeight;

short			g2DStackDepth = 0;


/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
	gGameWindowWidth = 640;
	gGameWindowHeight = 480;
	SDL_GL_GetDrawableSize(gSDLWindow, &gGameWindowWidth, &gGameWindowHeight);
}



#pragma mark -


/**************** GAMMA FADE IN *************************/

void GammaFadeIn(void)
{
	IMPLEMENT_ME_SOFT();
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


#pragma mark -


/******************** GET DEFAULT WINDOW SIZE *******************/

void GetDefaultWindowSize(int display, int* width, int* height)
{
	const float aspectRatio = 4.0 / 3.0f;
	const float screenCoverage = 2.0f / 3.0f;

	SDL_Rect displayBounds = { .x = 0, .y = 0, .w = 640, .h = 480 };
	SDL_GetDisplayUsableBounds(display, &displayBounds);

	if (displayBounds.w > displayBounds.h)
	{
		*width = displayBounds.h * screenCoverage * aspectRatio;
		*height = displayBounds.h * screenCoverage;
	}
	else
	{
		*width = displayBounds.w * screenCoverage;
		*height = displayBounds.w * screenCoverage / aspectRatio;
	}
}

/******************** MOVE WINDOW TO PREFERRED DISPLAY *******************/
//
// This works best in windowed mode.
// Turn off fullscreen before calling this!
//

static void MoveToPreferredDisplay(void)
{
	int currentDisplay = SDL_GetWindowDisplayIndex(gSDLWindow);

	if (currentDisplay != gGamePrefs.monitorNum)
	{
		int w = 640;
		int h = 480;
		GetDefaultWindowSize(gGamePrefs.monitorNum, &w, &h);
		SDL_SetWindowSize(gSDLWindow, w, h);

		SDL_SetWindowPosition(
			gSDLWindow,
			SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.monitorNum),
			SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.monitorNum));
	}
}

/*********************** SET FULLSCREEN MODE **********************/

void SetFullscreenMode(bool enforceDisplayPref)
{
	if (!gGamePrefs.fullscreen)
	{
		SDL_SetWindowFullscreen(gSDLWindow, 0);

		if (enforceDisplayPref)
		{
			MoveToPreferredDisplay();
		}
	}
	else
	{
		if (enforceDisplayPref)
		{
			int currentDisplay = SDL_GetWindowDisplayIndex(gSDLWindow);

			if (currentDisplay != gGamePrefs.monitorNum)
			{
				// We must switch back to windowed mode for the preferred monitor to take effect
				SDL_SetWindowFullscreen(gSDLWindow, 0);
				MoveToPreferredDisplay();
			}
		}

		// Enter fullscreen mode
		SDL_SetWindowFullscreen(gSDLWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
}
