/****************************/
/*   	PAUSED.C			*/
/* By Brian Greenstone      */
/* (c)2002 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "utf8.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static Boolean NavigatePausedMenu(void);
static void DrawPaused(void);



/****************************/
/*    CONSTANTS             */
/****************************/


#define	PAUSED_FRAME_WIDTH	190

#define	LETTER_SIZE			12.0f
#define	LETTER_SPACING		(LETTER_SIZE * 1.15f)
#define	LETTER_SPACING_Y	(LETTER_SPACING * 1.3f)


/*********************/
/*    VARIABLES      */
/*********************/

Boolean			gGamePaused = false;
static short	gPausedMenuSelection;


/********************** DO PAUSED **************************/

void DoPaused(void)
{
	gGamePaused = true;
	gPausedMenuSelection = 0;

	PauseAllChannels(true);


				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	UpdateInput();

	while(true)
	{
			/* SEE IF MAKE SELECTION */

		if (NavigatePausedMenu())
			break;

			/* DRAW STUFF */

		CalcFramesPerSecond();
		UpdateInput();
		DoPlayerTerrainUpdate(gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z);		// need to call this to keep supertiles active
		OGL_DrawScene(DrawPaused);
	}

	PauseAllChannels(false);
	gGamePaused = false;
}


/*********************** DRAW PAUSED ***************************/

static void DrawPaused(void)
{
float	x,y,leftX;
float	dotX,dotY;
static float	dotAlpha = 1.0f;

			/* DRAW THE BACKGROUND */

	switch(gLevelNum)
	{
		case	LEVEL_NUM_PLUMBING:
		case	LEVEL_NUM_GUTTER:
				DrawTunnel();
				break;

		default:
				DrawArea();

	}


			/*************************/
			/* DRAW THE PAUSED STUFF */
			/*************************/

	OGL_PushState();
	SetInfobarSpriteState();

	SetColor4f(1,1,1,1);
	gGlobalTransparency = 1.0f;


			/* DRAW FRAME FIRST */

	x = (640-PAUSED_FRAME_WIDTH)/2;
	y = 210;
	DrawInfobarSprite2(x, y, PAUSED_FRAME_WIDTH, SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_PausedFrame);


				/* DRAW TEXT */

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// make glow

	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = .2;


	leftX = x + 25.0f;
	y += 13;

	for (int j = 0; j < 3; j++)											// 3 lines of text
	{
		x = leftX;

		if (j == gPausedMenuSelection)								// remember where to put dot
		{
			dotX = x - 15.0f;
			dotY = y + 3.0f;
		}

		const char* caption = Localize(STR_PAUSEOPTION1 + j);

		while (*caption)
		{
			uint32_t c = ReadNextCodepointFromUTF8(&caption);		// get char
			int		texNum = CharToSprite(c);						// convert to texture #
			if (texNum != -1)
				DrawInfobarSprite2(x, y, LETTER_SIZE * 1.8f, SPRITE_GROUP_DIALOG, texNum);

			x += GetCharSpacing(c, LETTER_SPACING);
		}
		y += LETTER_SPACING_Y;
	}

	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 1;

			/* DRAW SELECT DOT */

	dotAlpha += gFramesPerSecondFrac * 18.0f;					// occilate the dot
	if (dotAlpha > PI2)
		dotAlpha -= PI2;
	gGlobalTransparency = (1.0f + sin(dotAlpha)) * .8f;

	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	DrawInfobarSprite2(dotX, dotY, 16, SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_PausedDot);

	gGlobalTransparency = 1.0f;


	OGL_PopState();
}








/*********************** NAVIGATE PAUSED MENU **************************/

static Boolean NavigatePausedMenu(void)
{
Boolean	continueGame = false;


		/* SEE IF CHANGE SELECTION */

	if (IsNeedDown(kNeed_UIUp) && (gPausedMenuSelection > 0))
	{
		gPausedMenuSelection--;
		PlayEffect_Parms(EFFECT_CHANGESELECT,FULL_CHANNEL_VOLUME/3,FULL_CHANNEL_VOLUME/4,NORMAL_CHANNEL_RATE);
	}
	else
	if (IsNeedDown(kNeed_UIDown) && (gPausedMenuSelection < 2))
	{
		gPausedMenuSelection++;
		PlayEffect_Parms(EFFECT_CHANGESELECT,FULL_CHANNEL_VOLUME/3,FULL_CHANNEL_VOLUME/4,NORMAL_CHANNEL_RATE);
	}

			/***************************/
			/* SEE IF MAKE A SELECTION */
			/***************************/

	if (IsNeedDown(kNeed_UIConfirm))
	{
		PlayEffect_Parms(EFFECT_CHANGESELECT,FULL_CHANNEL_VOLUME/3,FULL_CHANNEL_VOLUME/2,NORMAL_CHANNEL_RATE);

		switch(gPausedMenuSelection)
		{
			case	0:								// RESUME
					continueGame = true;
					break;

			case	1:								// EXIT
					gGameOver = true;
					continueGame = true;
					break;

			case	2:								// QUIT
					CleanQuit();
					break;

		}
	}


			/*****************************/
			/* SEE IF CANCEL A SELECTION */
			/*****************************/

	else
	if (IsNeedDown(kNeed_UIPause))
	{
		continueGame = true;
	}


	return(continueGame);
}



