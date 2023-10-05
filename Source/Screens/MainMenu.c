/****************************/
/*   	MAINMENU SCREEN.C	*/
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

static void SetupMainMenuScreen(void);
static void FreeMainMenuScreen(void);
static void DrawMainMenuCallback(void);
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
static void DrawDarkenPane(ObjNode *theNode);
static void MoveDarkenPane(ObjNode *theNode);
static void DrawHighScores(void);


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

	MAINMENU_SObjType_COUNT,
};

#define	ICON_SCALE		120.0f

/*********************/
/*    VARIABLES      */
/*********************/

static 	ObjNode *gMenuLogo;

#define	WaveXIndex	SpecialF[0]
#define	WaveZIndex	SpecialF[1]

static	int		gSelectedIcon = 0;

static Boolean	gPlayNow = false;

static	float	gInactivityTimer;

static 	Boolean	gHideFlowerMenu;
static	Boolean	gDrawHighScores;
static	float	gScoreFadeAlpha;


/********************** DO MAINMENU SCREEN **************************/

void DoMainMenuScreen(void)
{

			/* SETUP */

	SetupMainMenuScreen();
	MakeFadeEvent(true, 1);

	ProcessMainMenu();


			/* CLEANUP */

	OGL_FadeOutScene(DrawMainMenuCallback, NULL);
	FreeMainMenuScreen();
}



/********************* SETUP MAINMENU SCREEN **********************/

static void SetupMainMenuScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
static const OGLVector3D	fillDirection1 = { -.7, .9, -1.0 };
static const OGLVector3D	fillDirection2 = { .3, .8, 1.0 };
ObjNode	*newObj;
int		i;

	PlaySong(EFFECT_SONG_THEME, true);

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

	OGL_SetupWindow(&viewDef, &gGameView);
	OGL_CheckError();


				/************/
				/* LOAD ART */
				/************/

	InitEffects();

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:MainMenu.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_MAINMENU);

	LoadFoliage();


			/* LOAD SPRITES */

	LoadSpriteGroupFromSeries(SPRITE_GROUP_LEVELSPECIFIC, MAINMENU_SObjType_COUNT, "MainMenu");

			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_SKIP_EXPLORE);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SKIP_EXPLORE, 0,				// set sphere map on geometry texture
									1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkYosemite);


	LoadASkeleton(SKELETON_TYPE_MOUSE);

	LoadASkeleton(SKELETON_TYPE_GNOME);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_GNOME, 0,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Satin);



			/*******************/
			/* MAKE BACKGROUND */
			/*******************/


			/* CYC */

	gNewObjectDefinition.group		= MODEL_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_ObjType_Cyc;
	gNewObjectDefinition.coord		= viewDef.camera.from;
	gNewObjectDefinition.coord.y	+= 50.0f;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|STATUS_BIT_NOZBUFFER|STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot 		= CYC_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= gGameView.yon * .90f / 100.0f;
	MakeNewDisplayGroupObject(&gNewObjectDefinition);



		/* FLOWER WHEEL */

	gNewObjectDefinition.group 		= SPRITE_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= MAINMENU_SObjType_MenuFlower;
	gNewObjectDefinition.coord.x 	= 640/2 + 0.0f;
	gNewObjectDefinition.coord.y 	= 480/2 + 0.0f;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	gNewObjectDefinition.moveCall 	= MoveMenuIcon;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 480;
	newObj = MakeSpriteObject(&gNewObjectDefinition);
	newObj->Kind = -1;



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
		newObj = MakeSpriteObject(&gNewObjectDefinition);

		newObj->Kind = i;
	}



			/* LOGO */

	gNewObjectDefinition.group 		= SPRITE_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= MAINMENU_SObjType_MenuLogo;
	gNewObjectDefinition.coord.x 	= 80;
	gNewObjectDefinition.coord.y 	= 50;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	gNewObjectDefinition.moveCall 	= MoveMenuLogo;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 150;
	gMenuLogo = MakeSpriteObject(&gNewObjectDefinition);

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
	DisposeEffects();
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);
	DisposeAllBG3DContainers();
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

		OGL_DrawScene(DrawMainMenuCallback);

				/* DO USER INPUT */

		DoMenuControls();
	}


}


/*********************** DO MENU CONTROLS ***********************/

static void DoMenuControls(void)
{
	gHideFlowerMenu = false;

	if (IsNeedDown(kNeed_UIUp) || IsNeedDown(kNeed_UINext))
	{
		gInactivityTimer = 0;
		gSelectedIcon--;
		if (gSelectedIcon < 0)
			gSelectedIcon = 5;

		PlayEffect_Parms(EFFECT_CHANGESELECT,FULL_CHANNEL_VOLUME/3,FULL_CHANNEL_VOLUME/4,NORMAL_CHANNEL_RATE);
	}
	else if (IsNeedDown(kNeed_UIDown) || IsNeedDown(kNeed_UIPrev))
	{
		gInactivityTimer = 0;
		gSelectedIcon++;
		if (gSelectedIcon > 5)
			gSelectedIcon = 0;

		PlayEffect_Parms(EFFECT_CHANGESELECT,FULL_CHANNEL_VOLUME/3,FULL_CHANNEL_VOLUME/4,NORMAL_CHANNEL_RATE);
	}

			/* SEE IF SELECT */

	else if (IsNeedDown(kNeed_UIConfirm))
	{
		PlayEffect_Parms(EFFECT_CHANGESELECT,FULL_CHANNEL_VOLUME/4,FULL_CHANNEL_VOLUME/3,NORMAL_CHANNEL_RATE * 3/2);

		gInactivityTimer = 0;
		gHideFlowerMenu = true;

		switch(gSelectedIcon)
		{
			case	0:
					if (IsKeyActive(SDL_SCANCODE_F10))
					{
						int pick = DoLevelCheatDialog(DrawMainMenuCallback);
						if (pick >= 0)
						{
							gPlayNow = true;
							gPlayingFromSavedGame = false;
							gLevelNum = pick;
						}
					}
					else
					{
						gPlayNow = true;
						gPlayingFromSavedGame = false;
						gLevelNum = 0;
						for (int i = 0; i < 10; i++)		// quick n dirty level cheat
						{
							if (IsKeyActive(SDL_SCANCODE_1+i))
								gLevelNum = i;
						}
					}
					break;

			case	1:
					if (DoFileScreen(FILE_SCREEN_TYPE_LOAD, DrawObjects))
					{
						gPlayNow = true;
						gPlayingFromSavedGame = true;
					}
					break;

			case	2:
					DoSettingsOverlay(NULL, DrawMainMenuCallback);
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

static void DrawMainMenuCallback(void)
{
	DrawObjects();

			/* DRAW HIGH SCORES */

	if (gDrawHighScores)
		DrawHighScores();

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


	gNewObjectDefinition.coord.x 	= -g2DLogicalRect.right * 2;		// was -1390 in 4:3 original
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

		default:
				gNewObjectDefinition.type 		= SKELETON_TYPE_GNOME;
				gNewObjectDefinition.animNum	= 1;
				gNewObjectDefinition.scale 		= 12.0;
				gNewObjectDefinition.coord.y 	= -100;
				gNewObjectDefinition.coord.z 	= -700 - RandomFloat() * 300.0f;
				gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;

				gNewObjectDefinition.coord.x	*= 1.25f;		// spawn gnome a little more offscreen

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

			/* FADE IN */

	newObj->ColorFilter.a = 0.0f;
}


/***************** MOVE MENU CHARACTER ************************/

static void MoveMenuCharacter(ObjNode *theNode)
{
	GetObjectInfo(theNode);

			/* WALK */

	gCoord.x += gDelta.x * gFramesPerSecondFrac;

			/* FADE IN/OUT */

	float limit = g2DLogicalRect.right * 2;
	Boolean fadeOut = (gDelta.x > 0 && gCoord.x > limit)
					|| (gDelta.x < 0 && gCoord.x < -limit);

	if (fadeOut)
	{
		theNode->ColorFilter.a -= gFramesPerSecondFrac;
		if (theNode->ColorFilter.a < 0)		// kill when faded out completely
		{
			DeleteObject(theNode);
			return;
		}
	}
	else
	{
		theNode->ColorFilter.a += gFramesPerSecondFrac;
		theNode->ColorFilter.a = SDL_min(1.0f, theNode->ColorFilter.a);
	}

	UpdateObject(theNode);
}


/******************* MOVE MENU ICON **************************/

static void MoveMenuIcon(ObjNode *theNode)
{
float	s;
float	fps = gFramesPerSecondFrac;

	if (gHideFlowerMenu)
	{
		theNode->ColorFilter.a -= gFramesPerSecondFrac * 2;
		theNode->ColorFilter.a = SDL_max(0, theNode->ColorFilter.a);
	}
	else
	{
		float maxAlpha = theNode->Kind == -1? 1.0f: 0.9f;
		theNode->ColorFilter.a += gFramesPerSecondFrac * 2;
		theNode->ColorFilter.a = SDL_min(maxAlpha, theNode->ColorFilter.a);
	}

			/* IS THIS THE SELECTED ICON? */

	if (theNode->Kind == gSelectedIcon)
	{

		theNode->SpecialF[0] += fps * 10.0f;
		s = ICON_SCALE + sin(theNode->SpecialF[0]) * (ICON_SCALE * .2f);

		theNode->Scale.x =
		theNode->Scale.y = s;

	}

		/* NOT SELECTED */

	else if (theNode->Kind != -1)		// -1 is flower
	{

		theNode->Scale.x =
		theNode->Scale.y = ICON_SCALE;
	}
}


/*********************** MOVE MENU LOGO *****************************/

static void MoveMenuLogo(ObjNode *theNode)
{
	theNode->Coord.x = 80;
	if (!gGamePrefs.uiCentering)
		theNode->Coord.x += g2DLogicalRect.left;
	UpdateObjectTransforms(theNode);

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
	static const char* credits[][2] =
	{
		{"Programming:", "Brian Greenstone"},
		{"Art:", "Duncan Knarr"},
		{"Music:", "Aleksander Dimitrijevic"},
		{"Animation:", "Peter Greenstone"},
		{"Modern version:", "Iliyas Jorio"},
	};

	const float LH = 30;
	const float X2 = 300;

	ObjNode* pane = MakeDarkenPane();
	ObjNode* creditObjs[20] = {NULL};
	int n = 0;

	NewObjectDefinitionType def =
	{
		.coord = {640/2, 80, 0},
		.slot = INFOBAR_SLOT,
		.scale = 0.5f,
		.group = ATLAS_GROUP_FONT1,
	};

	creditObjs[n++] = TextMesh_New("Bugdom 2 Credits", kTextMeshAlignCenter, &def);
	def.coord.y += LH * 2;
	def.scale = 0.33f;

	for (int i = 0; i < (int)SDL_arraysize(credits); i++)
	{
		def.coord.x = X2-30;
		ObjNode* role = TextMesh_New(credits[i][0], kTextMeshAlignRight, &def);
		role->ColorFilter = (OGLColorRGBA) {1,0.8f,0.2f,1};
		def.coord.x = X2;
		ObjNode* name = TextMesh_New(credits[i][1], kTextMeshAlignLeft, &def);
		def.coord.y += LH;
		creditObjs[n++] = role;
		creditObjs[n++] = name;
	}

	def.coord.y += LH * 1.5f;
	def.coord.x = 640/2;
	creditObjs[n] = TextMesh_New("pangeasoft.net/bug2", kTextMeshAlignCenter, &def);
	creditObjs[n]->ColorFilter = (OGLColorRGBA){ .4, .8, 1, 1 };
	def.coord.y += LH;
	n++;
	creditObjs[n] = TextMesh_New("jorio.itch.io/bugdom2", kTextMeshAlignCenter, &def);
	creditObjs[n]->ColorFilter = (OGLColorRGBA){ .4, .8, 1, 1 };
	n++;

	char systemInfo[512];
	SDL_snprintf(systemInfo, sizeof(systemInfo), "%s v%s - %s (%s)\nOpenGL %s (%s)",
		PROJECT_NAME,
		PROJECT_VERSION,
		SDL_GetRevision(),
		SDL_GetCurrentVideoDriver(),
		(const char*)glGetString(GL_VERSION),
		(const char*)glGetString(GL_RENDERER));
	
	def.scale = .2f;
	def.coord.y = 475;
	creditObjs[n] = TextMesh_New(systemInfo, kTextMeshAlignCenter | kTextMeshAlignBottom, &def);
	creditObjs[n]->ColorFilter = (OGLColorRGBA){ .5,.5,.5, 1 };
	n++;

	//-----------------------------

	GAME_ASSERT(n <= (int) SDL_arraysize(creditObjs));

	for (int i = 0; i < n; i++)
	{
		creditObjs[i]->MoveCall = MoveCredits;
		creditObjs[i]->ColorFilter.a = 0;
		creditObjs[i]->Mode = 0;
	}


	UpdateInput();
	while(!UserWantsOut())
	{
		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(DrawMainMenuCallback);
	}

			/* FADE OUT EVERYTHING */

	pane->Mode = 1;
	for (int i = 0; i < n; i++)
	{
		creditObjs[i]->Mode = 1;
	}
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

	gDrawHighScores = true;
	gScoreFadeAlpha = 0;


		/* MAKE DARKEN PANE */

	pane = MakeDarkenPane();


		/*************************/
		/* SHOW IN ANIMATED LOOP */
		/*************************/

	UpdateInput();
	while(!UserWantsOut())
	{
		UpdateInput();


		CalcFramesPerSecond();
		MoveObjects();

		gScoreFadeAlpha += gFramesPerSecondFrac * 3.0f;		// fade in text
		if (gScoreFadeAlpha > 1.0f)
			gScoreFadeAlpha = 1.0f;

			/* DRAW STUFF */

		OGL_DrawScene(DrawMainMenuCallback);
	}


		/* CLEANUP */

	pane->Mode = 1;								// fade out pane
	UpdateInput();
	gDrawHighScores = false;
}

/****************** DRAW HIGH SCORES ***********************/

static void DrawHighScores(void)
{
float	y;
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
	for (int i = 0; i < NUM_SCORES; i++)
	{
				/* DRAW NAME */

		DrawScoreText(gHighScores[i].name, 150, y);

				/* DRAW SCORE */

		SDL_snprintf(s, sizeof(s), "%09d", gHighScores[i].score);
		DrawScoreText(s, 350, y);

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


/***************** DRAW SCORE TEXT (ASSUMES INFOBAR MODE) ***********************/

void DrawScoreText(const char* s, float x, float y)
{
	GameFont_DrawString(s, x, y, .32f, kTextMeshAlignLeft | kTextMeshAlignTop);
}



#pragma mark -


/******************** MAKE DARKEN PANE **************************/

ObjNode *MakeDarkenPane(void)
{
	NewObjectDefinitionType def =
	{
		.genre		= CUSTOM_GENRE,
		.flags		= STATUS_BIT_NOZWRITES | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOFOG | STATUS_BIT_NOTEXTUREWRAP,
		.slot		= DARKENPANE_SLOT,
		.moveCall	= MoveDarkenPane,
		.drawCall	= DrawDarkenPane,
	};

	ObjNode* pane = MakeNewObject(&def);
	pane->Mode = 0;							// make lighten
	pane->ColorFilter = (OGLColorRGBA){0,0,0,0};

	return pane;
}


/********************* MOVE DARKEN PANE ******************************/

static void MoveDarkenPane(ObjNode *theNode)
{
	const float targetOpacity = .75f;

	if (theNode->Mode == 0)
	{
		theNode->ColorFilter.a += gFramesPerSecondFrac * 3.0f;
		if (theNode->ColorFilter.a > targetOpacity)
			theNode->ColorFilter.a = targetOpacity;
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

static void DrawDarkenPane(ObjNode *theNode)
{
	OGL_PushState();
	SetInfobarSpriteState();

	glDisable(GL_TEXTURE_2D);
	SetColor4fv(&theNode->ColorFilter.r);
	glEnable(GL_BLEND);

	glBegin(GL_QUADS);
	glVertex3f(g2DLogicalRect.right,	g2DLogicalRect.top, 0);
	glVertex3f(g2DLogicalRect.left,		g2DLogicalRect.top, 0);
	glVertex3f(g2DLogicalRect.left,		g2DLogicalRect.bottom, 0);
	glVertex3f(g2DLogicalRect.right,	g2DLogicalRect.bottom, 0);
	glEnd();

	OGL_PopState();
}


