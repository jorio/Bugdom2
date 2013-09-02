/****************************/
/*   	MAINMENU SCREEN.C	*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"
#include "dialog.h"
#include "infobar.h"
#include <AGL/aglmacro.h>

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gGlobalTransparency;
extern	FSSpec		gDataSpec;
extern	Boolean		gGameOver;
extern	KeyMap gKeyMap,gNewKeys;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,DisableAnimSounds, gPlayingFromSavedGame;
extern	PrefsType	gGamePrefs;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	int				gLevelNum;
extern	SparkleType	gSparkles[MAX_SPARKLES];
extern	OGLPoint3D	gCoord;
extern	OGLVector3D	gDelta;
extern	u_long			gScore,gGlobalMaterialFlags;
extern	HighScoreType	gHighScores[NUM_SCORES];
extern	OGLColorRGB			gGlobalColorFilter;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupMainMenuScreen(void);
static void FreeMainMenuScreen(void);
static void DrawMainMenuCallback(OGLSetupOutputType *info);
static void ProcessMainMenu(void);
static void MoveMenuFlower(ObjNode *theNode);
static void MoveMenuCharacter(ObjNode *theNode);
static void MakeMenuCharacter(void);
static void MoveMenuIcon(ObjNode *theNode);
static void DoMenuControls(void);
static void MoveMenuLogo(ObjNode *theNode);
static void DoCredits(void);
static void MoveCredits(ObjNode *theNode);
static void DoHighScores(void);
static void DrawDarkenPane(ObjNode *theNode, const OGLSetupOutputType *setupInfo);
static void MoveDarkenPane(ObjNode *theNode);
static void DrawHighScores(OGLSetupOutputType *info);


/****************************/
/*    CONSTANTS             */
/****************************/



enum
{
	MAINMENU_ObjType_Cyc,
	MAINMENU_ObjType_Web
};



enum
{
	MAINMENU_SObjType_MenuFlower,
	MAINMENU_SObjType_MenuLogo,

	MAINMENU_SObjType_PlayIcon,
	MAINMENU_SObjType_SavedIcon,
	MAINMENU_SObjType_SettingsIcon,
	MAINMENU_SObjType_HighScoreIcon,
	MAINMENU_SObjType_HelpIcon,
	MAINMENU_SObjType_QuitIcon,

	MAINMENU_SObjType_Credits
};

#define	ICON_SCALE		120.0f

/*********************/
/*    VARIABLES      */
/*********************/

static 	ObjNode	*gMenuFlower, *gMenuLogo;

#define	WaveXIndex	SpecialF[0]
#define	WaveZIndex	SpecialF[1]

static	int		gSelectedIcon = 0;

static	Boolean	gPlayNow;

static	float	gInactivityTimer;

static 	Boolean	gFadeInText;
static	Boolean	gDrawHighScores;
static	float	gScoreFadeAlpha;


/********************** DO MAINMENU SCREEN **************************/

void DoMainMenuScreen(void)
{

	GammaFadeOut();

			/* SETUP */

	SetupMainMenuScreen();
	MakeFadeEvent(true, 1);

	ProcessMainMenu();


			/* CLEANUP */

	GammaFadeOut();
	FreeMainMenuScreen();
}



/********************* SETUP MAINMENU SCREEN **********************/

static void SetupMainMenuScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
const static OGLVector3D	fillDirection1 = { -.7, .9, -1.0 };
const static OGLVector3D	fillDirection2 = { .3, .8, 1.0 };
ObjNode	*newObj;
int		i;

	PlaySong(SONG_THEME, true);

	gLevelNum 		= -1;
	gSelectedIcon 	= 0;
	gPlayNow 		= false;
	gInactivityTimer = 0;
	gDrawHighScores	= false;

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.0;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 2500;

	viewDef.styles.useFog			= true;
	viewDef.styles.fogStart			= viewDef.camera.yon * .6f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * .9f;

	viewDef.view.clearBackBuffer	= true;
	viewDef.view.clearColor.r 		= .7;
	viewDef.view.clearColor.g 		= .4;
	viewDef.view.clearColor.b		= 0;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.y		= 50;
	viewDef.camera.from.z		= 500;

	viewDef.camera.to.y 		= 100.0f;

	viewDef.lights.ambientColor.r = .2;
	viewDef.lights.ambientColor.g = .2;
	viewDef.lights.ambientColor.b = .2;

	viewDef.lights.numFillLights 	= 2;

	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillColor[0].r 	= .8;
	viewDef.lights.fillColor[0].g 	= .8;
	viewDef.lights.fillColor[0].b 	= .6;

	viewDef.lights.fillDirection[1] = fillDirection2;
	viewDef.lights.fillColor[1].r 	= .5;
	viewDef.lights.fillColor[1].g 	= .5;
	viewDef.lights.fillColor[1].b 	= .0;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

	InitSparkles();

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Models:mainmenu.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_MAINMENU, gGameViewInfoPtr);

	LoadFoliage(gGameViewInfoPtr);


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:particle.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_PARTICLES, gGameViewInfoPtr);
	BlendAllSpritesInGroup(SPRITE_GROUP_PARTICLES);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:global.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:MainMenu.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_MAINMENU, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:Dialog.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_DIALOG, gGameViewInfoPtr);


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_SKIP_EXPLORE, gGameViewInfoPtr);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SKIP_EXPLORE, 0,				// set sphere map on geometry texture
									1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkYosemite);


	LoadASkeleton(SKELETON_TYPE_MOUSE, gGameViewInfoPtr);

	LoadASkeleton(SKELETON_TYPE_GNOME, gGameViewInfoPtr);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_GNOME, 0,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);


				/* LOAD AUDIO */

//	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:audio:Bonus.sounds", &spec);
//	LoadSoundBank(&spec, SOUND_BANK_MAINMENU);



			/*******************/
			/* MAKE BACKGROUND */
			/*******************/


			/* CYC */

	gNewObjectDefinition.group		= MODEL_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_ObjType_Cyc;
	gNewObjectDefinition.coord		= viewDef.camera.from;
	gNewObjectDefinition.coord.y	+= 50.0f;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot 		= TERRAIN_SLOT+1;					// draw after terrain for better performance since terrain blocks much of the pixels
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= gGameViewInfoPtr->yon * .90f / 100.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);



		/* FLOWER WHEEL */

	gNewObjectDefinition.group 		= SPRITE_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_SObjType_MenuFlower;
	gNewObjectDefinition.coord.x 	= 640/2 + 0.0f;
	gNewObjectDefinition.coord.y 	= 480/2 + 0.0f;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 480;
	gMenuFlower = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);



			/* ICONS */

	for (i = 0; i < 6; i++)
	{
		const OGLPoint3D iconCoords[6] =
		{
			320,	430, 	0,
			160,	340, 	0,
			160,	165, 	0,

			325,	60, 	0,
			470,	165, 	0,
			470,	330, 	0,
		};

		gNewObjectDefinition.type 		= MAINMENU_SObjType_PlayIcon + i;
		gNewObjectDefinition.coord		= iconCoords[i];
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= SPRITE_SLOT;
		gNewObjectDefinition.moveCall 	= MoveMenuIcon;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 	    = ICON_SCALE;
		newObj = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);

		newObj->Kind = i;
	}



			/* LOGO */

	gNewObjectDefinition.group 		= SPRITE_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_SObjType_MenuLogo;
	gNewObjectDefinition.coord.x 	= 80;
	gNewObjectDefinition.coord.y 	= 50;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	gNewObjectDefinition.moveCall 	= MoveMenuLogo;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 150;
	gMenuLogo = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);

	gMenuLogo->ColorFilter.a = 0;						// hide for now


			/* MAKE GRASS */

	for (i = 0; i < 180; i++)
	{

		gNewObjectDefinition.group 		= MODEL_GROUP_FOLIAGE;
		gNewObjectDefinition.type 		= FOLIAGE_ObjType_Grass1 + RandomRange(0,2);
		gNewObjectDefinition.coord.x	= RandomFloat2() * 1500.0f;
		gNewObjectDefinition.coord.y 	= -600.0f;
		gNewObjectDefinition.coord.z 	= -200.0f - RandomFloat() * 1200.0f;
		gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot 		= 5;
		gNewObjectDefinition.moveCall 	= MoveMenuFlower;
		gNewObjectDefinition.rot 		= RandomFloat()*PI2;
		gNewObjectDefinition.scale 		= 4.0 + RandomFloat() * 2.0f;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->WaveXIndex = newObj->Coord.x * .004f;
		newObj->WaveZIndex = newObj->Coord.z * .004f;
	}

}


/********************** FREE MAINMENU SCREEN **********************/

static void FreeMainMenuScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUND_BANK_MAINMENU);
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}

#pragma mark -


/**************** PROCESS MAIN MENU ********************/

static void ProcessMainMenu(void)
{
float	charTimer = 2.0f;

	CalcFramesPerSecond();
	UpdateInput();

	while(!gPlayNow)
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();

				/* SEE IF MAKE SOMEONE WALK ACROSS */

		charTimer -= fps;
		if (charTimer <= 0.0f)
		{
			charTimer = 4.0f + RandomFloat() * 6.0f;
			MakeMenuCharacter();
		}


				/* DRAW */

		OGL_DrawScene(gGameViewInfoPtr, DrawMainMenuCallback);

				/* DO USER INPUT */

		DoMenuControls();
	}


}


/*********************** DO MENU CONTROLS ***********************/

static void DoMenuControls(void)
{
	if (GetNewKeyState(KEY_RIGHT) || GetNewKeyState(KEY_UP))
	{
		gInactivityTimer = 0;
		gSelectedIcon--;
		if (gSelectedIcon < 0)
			gSelectedIcon = 5;

		PlayEffect_Parms(EFFECT_CHANGESELECT,FULL_CHANNEL_VOLUME/3,FULL_CHANNEL_VOLUME/4,NORMAL_CHANNEL_RATE);
	}
	else
	if (GetNewKeyState(KEY_LEFT) || GetNewKeyState(KEY_DOWN))
	{
		gInactivityTimer = 0;
		gSelectedIcon++;
		if (gSelectedIcon > 5)
			gSelectedIcon = 0;

		PlayEffect_Parms(EFFECT_CHANGESELECT,FULL_CHANNEL_VOLUME/3,FULL_CHANNEL_VOLUME/4,NORMAL_CHANNEL_RATE);
	}

			/* SEE IF SELECT */

	else
	if (GetNewKeyState(KEY_SPACE) || GetNewKeyState(KEY_RETURN))
	{
		PlayEffect_Parms(EFFECT_CHANGESELECT,FULL_CHANNEL_VOLUME/4,FULL_CHANNEL_VOLUME/3,NORMAL_CHANNEL_RATE * 3/2);

		gInactivityTimer = 0;
		switch(gSelectedIcon)
		{
			case	0:
					gPlayNow = true;
					break;

			case	1:
#if !DEMO
					if (LoadSavedGame())
					{
						gPlayNow = true;
						gPlayingFromSavedGame = true;
						GameScreenToBlack();
					}
#endif
					break;

			case	2:
					DoGameOptionsDialog();
					break;

			case	3:
					DoHighScores();
					break;

			case	4:
					DoCredits();
					break;

			case	5:
					CleanQuit();
					break;
		}
	}

			/* NO ACTIVITY */
	else
		gInactivityTimer += gFramesPerSecondFrac;


}



/***************** DRAW MAINMENU CALLBACK *******************/

static void DrawMainMenuCallback(OGLSetupOutputType *info)
{
	DrawObjects(info);
	DrawSparkles(info);											// draw light sparkles

			/* DRAW HIGH SCORES */

	if (gDrawHighScores)
		DrawHighScores(info);

}


#pragma mark -

/********************* MOVE WAVING FLOWER **********************/

static void MoveMenuFlower(ObjNode *theNode)
{
	theNode->WaveXIndex += gFramesPerSecondFrac * 1.5f;
	theNode->WaveZIndex += gFramesPerSecondFrac * 1.5f;

	theNode->Rot.x = sin(theNode->WaveXIndex) * .02f;
	theNode->Rot.z = sin(theNode->WaveZIndex) * .02f;

	UpdateObjectTransforms(theNode);
}

/******************* MAKE MENU CHARACTER ********************/

static void MakeMenuCharacter(void)
{
ObjNode	*newObj;


	gNewObjectDefinition.coord.x 	= -1390;
	gNewObjectDefinition.coord.z 	= -500 - RandomFloat() * 300.0f;
	gNewObjectDefinition.moveCall	= MoveMenuCharacter;
	gNewObjectDefinition.slot 		= 10;
	gNewObjectDefinition.rot 		= -PI/2;

	switch(RandomRange(0,2))
	{
							/* MAKE SKIP */

		case	0:
				gNewObjectDefinition.type 		= SKELETON_TYPE_SKIP_EXPLORE;
				gNewObjectDefinition.animNum	= PLAYER_ANIM_WALK;
				gNewObjectDefinition.scale 		= 5.0;
				gNewObjectDefinition.coord.y 	= -400;
				gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;

				newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
				newObj->Skeleton->AnimSpeed = 1.5f;

				newObj->Delta.x = 300.0f;
				break;


							/* MAKE MOUSE */

		case	1:
				gNewObjectDefinition.type 		= SKELETON_TYPE_MOUSE;
				gNewObjectDefinition.animNum	= 1;
				gNewObjectDefinition.scale 		= 6.0;
				gNewObjectDefinition.coord.y 	= -300;
				gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;

				newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
				newObj->Skeleton->AnimSpeed = .7f;

				newObj->Delta.x = 500.0f;
				break;

							/* MAKE GNOME */

		case	2:
				gNewObjectDefinition.type 		= SKELETON_TYPE_GNOME;
				gNewObjectDefinition.animNum	= 1;
				gNewObjectDefinition.scale 		= 12.0;
				gNewObjectDefinition.coord.y 	= -100;
				gNewObjectDefinition.coord.z 	= -700 - RandomFloat() * 300.0f;
				gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;

				newObj = MakeNewSkeletonObject(&gNewObjectDefinition);
				newObj->Skeleton->AnimSpeed = .8f;

				newObj->Delta.x = 350.0f;
				break;


		}


			/* SEE IF LEFT TO RIGHT */

	if (MyRandomLong() & 1)
	{
		newObj->Delta.x *= -1.0f;
		newObj->Coord.x *= -1.0f;

		newObj->Rot.y -= PI;

		UpdateObjectTransforms(newObj);
	}

}


/***************** MOVE MENU CHARACTER ************************/

static void MoveMenuCharacter(ObjNode *theNode)
{
	GetObjectInfo(theNode);

	gCoord.x += gDelta.x * gFramesPerSecondFrac;

	if (fabs(gCoord.x) > 1400.0f)
	{
		DeleteObject(theNode);
		return;
	}


	UpdateObject(theNode);
}


/******************* MOVE MENU ICON **************************/

static void MoveMenuIcon(ObjNode *theNode)
{
float	s;
float	fps = gFramesPerSecondFrac;

	theNode->ColorFilter.a = .9f;

			/* IS THIS THE SELECTED ICON? */

	if (theNode->Kind == gSelectedIcon)
	{

		theNode->SpecialF[0] += fps * 10.0f;
		s = ICON_SCALE + sin(theNode->SpecialF[0]) * (ICON_SCALE * .2f);

		theNode->Scale.x =
		theNode->Scale.y = s;

	}

		/* NOT SELECTED */

	else
	{

		theNode->Scale.x =
		theNode->Scale.y = ICON_SCALE;
	}
}


/*********************** MOVE MENU LOGO *****************************/

static void MoveMenuLogo(ObjNode *theNode)
{
			/* SHOW LOGO IF USER IS INACTIVE */

	if (gInactivityTimer > 10.0f)
	{
		theNode->ColorFilter.a += gFramesPerSecondFrac;
		if (theNode->ColorFilter.a > 1.0f)
			theNode->ColorFilter.a = 1.0f;
	}
	else
	{
		theNode->ColorFilter.a -= gFramesPerSecondFrac;
		if (theNode->ColorFilter.a < 0.0f)
			theNode->ColorFilter.a = 0.0f;
	}

}


#pragma mark -


/**************** DO CREDITS ********************/

static void DoCredits(void)
{
ObjNode	*newObj, *pane;

	pane = MakeDarkenPane();

			/* MAKE CREDITS SPRITE */

	gNewObjectDefinition.group 		= SPRITE_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_SObjType_Credits;
	gNewObjectDefinition.coord.x 	= 640/2;
	gNewObjectDefinition.coord.y 	= 480/2;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	gNewObjectDefinition.moveCall 	= MoveCredits;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 500;
	newObj = MakeSpriteObject(&gNewObjectDefinition, gGameViewInfoPtr);

	newObj->ColorFilter.a = 0;						// dim out
	newObj->Mode = 0;								// fade-in mode

	UpdateInput();
	while(!AreAnyNewKeysPressed())
	{
		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();

				/* DRAW */

		OGL_DrawScene(gGameViewInfoPtr, DrawMainMenuCallback);
	}

	newObj->Mode = 1;								// fade out mode
	pane->Mode = 1;
}


/*********************** MOVE CREDITS *****************************/

static void MoveCredits(ObjNode *theNode)
{

	if (theNode->Mode == 0)
	{
		theNode->ColorFilter.a += gFramesPerSecondFrac * 3.0f;
		if (theNode->ColorFilter.a > 1.0f)
			theNode->ColorFilter.a = 1.0f;
	}
	else
	{
		theNode->ColorFilter.a -= gFramesPerSecondFrac * 3.0f;
		if (theNode->ColorFilter.a < 0.0f)
		{
			DeleteObject(theNode);
			return;
		}
	}

}


#pragma mark -


/********************** DO HIGH SCORES ***********************/

static void DoHighScores(void)
{
ObjNode	*pane;

			/* LOAD HIGH SCORES */

	LoadHighScores();

	gFadeInText = true;
	gDrawHighScores = true;
	gScoreFadeAlpha = 0;


		/* MAKE DARKEN PANE */

	pane = MakeDarkenPane();


		/*************************/
		/* SHOW IN ANIMATED LOOP */
		/*************************/

	UpdateInput();
	while(!AreAnyNewKeysPressed())
	{
		UpdateInput();


		CalcFramesPerSecond();
		MoveObjects();

		gScoreFadeAlpha += gFramesPerSecondFrac * 3.0f;		// fade in text
		if (gScoreFadeAlpha > 1.0f)
			gScoreFadeAlpha = 1.0f;

			/* DRAW STUFF */

		OGL_DrawScene(gGameViewInfoPtr, DrawMainMenuCallback);
	}


		/* CLEANUP */

	pane->Mode = 1;								// fade out pane
	UpdateInput();
	gDrawHighScores = false;
}

/****************** DRAW HIGH SCORES ***********************/

static void DrawHighScores(OGLSetupOutputType *info)
{
AGLContext agl_ctx = info->drawContext;
float	y;
int		i,j,n;
Str32	s;

			/* SET STATE */

	OGL_PushState();

	SetInfobarSpriteState();

	gGlobalTransparency = gScoreFadeAlpha;
	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = .3;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// make glow
	glEnable(GL_BLEND);

			/*****************/
			/* DRAW THE TEXT */
			/*****************/

	y = 120;
	for (i = 0; i < NUM_SCORES; i++)
	{
				/* DRAW NAME */

		DrawScoreText(gHighScores[i].name, 150,y,info);

				/* DRAW SCORE */

		NumToString(gHighScores[i].score, s);	// convert score to a text string
		if (s[0] < SCORE_DIGITS)				// pad 0's
		{
			n = SCORE_DIGITS-s[0];
			BlockMove(&s[1],&s[1+n], 20);		// shift existing data over

			for (j = 0; j < n; j++)				// pad with 0's
				s[1+j] = '0';

			s[0] = SCORE_DIGITS;
		}
		DrawScoreText(s, 350,y,info);

		y += SCORE_TEXT_SPACING * 1.3f;
	}

			/***********/
			/* CLEANUP */
			/***********/

	gGlobalTransparency = 1;
	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 1;
	OGL_PopState();

}


/***************** DRAW SCORE TEXT ***********************/

void DrawScoreText(unsigned char *s, float x, float y, OGLSetupOutputType *info)
{
int	n,i,texNum;

	n = s[0];										// get str len

	for (i = 1; i <= n; i++)
	{
		texNum = CharToSprite(s[i]);				// get texture #
		if (texNum != -1)
			DrawInfobarSprite2(x, y, SCORE_TEXT_SPACING * 1.5f, SPRITE_GROUP_DIALOG, texNum, info);

		x += GetCharSpacing(s[i], SCORE_TEXT_SPACING);
	}



}



#pragma mark -


/******************** MAKE DARKEN PANE **************************/

ObjNode *MakeDarkenPane(void)
{
ObjNode *pane;

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|STATUS_BIT_NOTEXTUREWRAP|
										STATUS_BIT_DOUBLESIDED;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+100;
	gNewObjectDefinition.moveCall 	= MoveDarkenPane;
	pane = MakeNewObject(&gNewObjectDefinition);

	pane->Mode = 0;							// make lighten

	pane->CustomDrawFunction = DrawDarkenPane;

	pane->ColorFilter.r = 0;
	pane->ColorFilter.g = 0;
	pane->ColorFilter.b = 0;
	pane->ColorFilter.a = 0;

	return(pane);
}


/********************* MOVE DARKEN PANE ******************************/

static void MoveDarkenPane(ObjNode *theNode)
{
	if (theNode->Mode == 0)
	{
		theNode->ColorFilter.a += gFramesPerSecondFrac * 3.0f;
		if (theNode->ColorFilter.a > .6f)
			theNode->ColorFilter.a = .6f;
	}
	else
	{
		theNode->ColorFilter.a -= gFramesPerSecondFrac * 3.0f;
		if (theNode->ColorFilter.a < 0.0f)
		{
			DeleteObject(theNode);
			return;
		}
	}
}



/********************** DRAW DARKEN PANE *****************************/

static void DrawDarkenPane(ObjNode *theNode, const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = setupInfo->drawContext;


	glDisable(GL_TEXTURE_2D);
	SetColor4fv((GLfloat *)&theNode->ColorFilter);
	glEnable(GL_BLEND);

	glBegin(GL_QUADS);
	glVertex3f(-1000,-1000,DARKEN_PANE_Z);
	glVertex3f(1000,-1000,DARKEN_PANE_Z);
	glVertex3f(1000,1000,DARKEN_PANE_Z);
	glVertex3f(-1000,1000,DARKEN_PANE_Z);
	glEnd();

	glDisable(GL_BLEND);
}












