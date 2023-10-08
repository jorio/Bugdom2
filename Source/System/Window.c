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

int				gGameWindowWidth, gGameWindowHeight;


/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
	gGameWindowWidth = 640;
	gGameWindowHeight = 480;
	SDL_GL_GetDrawableSize(gSDLWindow, &gGameWindowWidth, &gGameWindowHeight);
}



#pragma mark -


/************************** ENTER 2D *************************/

void Enter2D(void)
{
	GrabMouse(false);
	SDL_ShowCursor(SDL_ENABLE);
	MyFlushEvents();
}


/************************** EXIT 2D *************************/

void Exit2D(void)
{
}



#pragma mark -


/******************** GET DEFAULT WINDOW SIZE *******************/

void GetDefaultWindowSize(int display, int* width, int* height)
{
	const float aspectRatio = 16.0 / 9.0f;
	const float screenCoverage = .8f;

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

	SDL_GL_SetSwapInterval(gGamePrefs.vsync);
}
