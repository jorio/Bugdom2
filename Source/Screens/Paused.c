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
#include "menu.h"


/****************************/
/*    PROTOTYPES            */
/****************************/


/****************************/
/*    CONSTANTS             */
/****************************/


#define	PAUSED_FRAME_WIDTH	190


static const MenuItem gPauseMenu[] =
{
	{.id='paus'},
	{.type = kMIPick, .id = 0, .next = 'EXIT', .text = STR_RESUME },
	{.type = kMIPick, .id = 1, .next = 'EXIT', .text = STR_SETTINGS },
	{.type = kMIPick, .id = 2, .next = 'EXIT', .text = STR_RETIRE },
	{.id=0},
};



/*********************/
/*    VARIABLES      */
/*********************/

Boolean			gGamePaused = false;


/***************** KEEP TERRAIN ALIVE WHILE PAUSED ******************/

void KeepTerrainAlive(void)
{
	// need to call this to keep supertiles active
	DoPlayerTerrainUpdate(gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z);
}

/********************** DO PAUSED **************************/

void DoPaused(void)
{
	gGamePaused = true;

	PauseAllChannels(true);
	GrabMouse(false);
	InvalidateAllInputs();		// flush ESC keypress

	NewObjectDefinitionType frameDef =
	{
		.group = SPRITE_GROUP_INFOBAR,
		.type = INFOBAR_SObjType_PausedFrame,
		.coord.x = 640/2, //(640-PAUSED_FRAME_WIDTH)/2,
		.coord.y = 480/2, //210,
		.scale = PAUSED_FRAME_WIDTH,
		.slot = INFOBAR_SLOT,
	};
	ObjNode* frame = MakeSpriteObject(&frameDef);

	MenuStyle pauseMenuStyle = kDefaultMenuStyle;
	pauseMenuStyle.offset.x += 120;
	pauseMenuStyle.offset.y += 5;
	pauseMenuStyle.darkenPaneOpacity = 0;
	pauseMenuStyle.fadeInSpeed = 999;
	pauseMenuStyle.fadeOutSpeed = 999;
	pauseMenuStyle.startButtonExits = true;

	void (*moveCall)(void) = IsTunnelLevel()? NULL: KeepTerrainAlive;
	void (*drawCall)(void) = IsTunnelLevel()? DrawTunnel: DrawObjects;

				/*************/
				/* MAIN LOOP */
				/*************/

	while (1)
	{
		frame->StatusBits &= ~STATUS_BIT_HIDDEN;

		int pick = StartMenu(gPauseMenu, &pauseMenuStyle, moveCall, drawCall);

		frame->StatusBits |= STATUS_BIT_HIDDEN;

		if (pick == 1)
		{
			DoSettingsOverlay(moveCall, drawCall);
			continue;		// do it again
		}

		if (pick == 2)
		{
			gGameOver = true;

			frame->StatusBits &= ~STATUS_BIT_HIDDEN;
			OGL_FadeOutScene(drawCall, moveCall);
		}

		break;
	}

	DeleteObject(frame);

	gGamePaused = false;

	if (!gGameOver)
	{
		PauseAllChannels(false);
		EnforceMusicPausePref();
	}

	GrabMouse(true);
}
