/****************************/
/*   	MISCSCREENS.C	    */
/* By Brian Greenstone      */
/* (c)2002 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/


#pragma mark -

/************** DO LEGAL SCREEN *********************/

void DoLegalScreen(void)
{
	OGLSetupInputType	viewDef;
	float	timeout = 8.0f;


			/* SETUP VIEW */

	OGL_NewViewDef(&viewDef);

	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;
	viewDef.styles.useFog			= false;

	OGL_SetupWindow(&viewDef, &gGameView);


			/* CREATE BACKGROUND OBJECT */

	LoadSpriteGroupFromSeries(SPRITE_GROUP_LEVELSPECIFIC, 1, "Pangea");

	NewObjectDefinitionType def =
	{
		.coord = {320, 240, 0},
		.group = SPRITE_GROUP_LEVELSPECIFIC,
		.type = 0,
		.scale = 640,
		.slot = SPRITE_SLOT,
	};
	ObjNode* imageNode = MakeSpriteObject(&def);
	GAME_ASSERT(imageNode);

	def.scale = 0.18f;
	def.coord.y = 470;
	def.group = ATLAS_GROUP_FONT1;
	ObjNode* widescreenText = TextMesh_New(
		"© 2002 Pangea Software, Inc. All rights reserved. Bugdom is a registered trademark of Pangea Software, Inc.",
		kTextMeshAlignBottom, &def);
	widescreenText->ColorFilter = (OGLColorRGBA) {.48f, .2f, .15f, 1};


		/***********/
		/* SHOW IT */
		/***********/


	MakeFadeEvent(true, 1);

		/* MAIN LOOP */

	while(!UserWantsOut())
	{
		CalcFramesPerSecond();

		imageNode->Coord = (OGLPoint3D) {320,240,0};
		imageNode->Scale = (OGLVector3D) {640,640,0};
		if (gCurrentAspectRatio > 1.59f)
		{
			imageNode->Scale.x *= 1.33f;
			imageNode->Scale.y *= 1.33f;
			imageNode->Coord.y += 20;
			widescreenText->StatusBits &= ~STATUS_BIT_HIDDEN;
		}
		else
			widescreenText->StatusBits |= STATUS_BIT_HIDDEN;
		UpdateObjectTransforms(imageNode);

		MoveObjects();
		OGL_DrawScene(DrawObjects);

		UpdateInput();
		if (UserWantsOut())
			break;

		timeout -= gFramesPerSecondFrac;
		if (timeout < 0.0f)
			break;
	}

		/* FADE OUT */

	OGL_FadeOutScene(DrawObjects, NULL);

		/* CLEANUP */

	DeleteAllObjects();
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);
}


#pragma mark -


/********************* LEVEL INTRO SUBTITLE ***********************/

static void DrawLevelIntroSubtitle(ObjNode* objNode)
{
	objNode->Timer += gFramesPerSecondFrac;

	if (objNode->Timer < 0)
		return;

	OGL_PushState();
	SetInfobarSpriteState();
	gGlobalTransparency = SDL_min(1, 2 * objNode->Timer);
	GameFont_DrawString(Localize(STR_LEVEL1 + gLevelNum), 320, 400, 0.6f, kTextMeshAlignCenter);
	OGL_PopState();

	gGlobalTransparency = 1;		// restore this
}

/*********************** DO LEVEL INTRO ***************************/

void DoLevelIntro(void)
{
			/* SET ANAGLYPH INFO */

	if (gGamePrefs.anaglyph)
	{
		gAnaglyphScaleFactor 	= 1.0f;
		gAnaglyphFocallength	= 200.0f * gAnaglyphScaleFactor;	// set camera info
		gAnaglyphEyeSeparation 	= 25.0f * gAnaglyphScaleFactor;
	}

			/* SHOW NON-ENGLISH SUBTITLE */

	if (gGamePrefs.language != LANGUAGE_ENGLISH
		&& Localize(STR_LEVEL1 + gLevelNum)[0] != '-')		// don't show untranslated subtitles
	{
		static const float timers[] =
		{
			[LEVEL_NUM_GNOMEGARDEN]	= 5,
			[LEVEL_NUM_SIDEWALK]	= 12,
			[LEVEL_NUM_FIDO]		= 3,
			[LEVEL_NUM_PLUMBING]	= 7,
			[LEVEL_NUM_PLAYROOM]	= 5,
			[LEVEL_NUM_CLOSET]		= 6.5f,
			[LEVEL_NUM_GUTTER]		= 8,
			[LEVEL_NUM_GARBAGE]		= 2,
			[LEVEL_NUM_BALSA]		= 5,
			[LEVEL_NUM_PARK]		= 5.5f,
		};
		ObjNode* subtitle = MakeNewDriverObject(INFOBAR_SLOT, DrawLevelIntroSubtitle, 0);
		subtitle->Timer = -timers[gLevelNum];
	}

			/* DO INTRO SCENE */

	switch(gLevelNum)
	{
		case	LEVEL_NUM_GNOMEGARDEN:
				DoLevelIntroScreen_FrontYard();
				break;

		case	LEVEL_NUM_SIDEWALK:
				DoLevelIntroScreen_BackYard();
				break;

		case	LEVEL_NUM_PLUMBING:
				DoLevelIntroScreen_Sewer();
				break;

		case	LEVEL_NUM_PLAYROOM:
				DoLevelIntroScreen_Playroom();
				break;

		case	LEVEL_NUM_CLOSET:
				DoLevelIntroScreen_Closet();
				break;

		case	LEVEL_NUM_BALSA:
				DoLevelIntroScreen_Balsa();
				break;

		case	LEVEL_NUM_FIDO:
				DoLevelIntroScreen_Fido();
				break;

		case	LEVEL_NUM_GUTTER:
				DoLevelIntroScreen_Gutter();
				break;

		case	LEVEL_NUM_GARBAGE:
				DoLevelIntroScreen_Garbage();
				break;

		case	LEVEL_NUM_PARK:
				DoLevelIntroScreen_Park();
				break;
	}

}
