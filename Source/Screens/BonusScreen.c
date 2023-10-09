/****************************/
/*   	BONUS SCREEN.C		*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupBonusScreen(void);
static void FreeBonusScreen(void);
static void DrawBonusCallback(void);
static void DoSkipWalkOn(void);
static void DoSkipThrowClovers(void);
static Boolean TossClover(void);
static void MoveTossedClover(ObjNode *theNode);
static void DoMouseBonus(void);
static void MoveBonusMouse(ObjNode *theNode);
static void MoveBonusSkipDancing(ObjNode *theNode);
static void MoveBonusFlower(ObjNode *theNode);
static void DrawBonusScore(void);
static void DoSaveSelect(void);
static void DrawSave(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	POINTS_LEVEL		15000
#define	POINTS_GREENCLOVER	175
#define	POINTS_BLUECLOVER	20000
#define	POINTS_GOLDCLOVER	250000
#define	POINTS_MOUSE		425

static const OGLPoint2D kSaveIconPos[2] = {{320-80, 300}, {320+80, 300}};


enum
{
	BONUS_ObjType_GreenClover,
	BONUS_ObjType_BlueClover,
	BONUS_ObjType_GoldClover,

	BONUS_ObjType_Brick,
};





/*********************/
/*    VARIABLES      */
/*********************/

static ObjNode *gSkip = nil;

#define	ThrowNow	Flag[0]


#define	WaveXIndex	SpecialF[0]
#define	WaveZIndex	SpecialF[1]

static	float	gBonusScoreAlpha, gSaveAlpha;

static	float	gSaveWobble = 0;

static	Boolean	gSaveGame = true;

static	int		gNumFullGoldClovers;


/********************** DO BONUS SCREEN **************************/

void DoBonusScreen(void)
{
	PlaySong(EFFECT_SONG_BONUS, false);

	gScore += POINTS_LEVEL;							// get level completion bonus

			/* SETUP */

	SetupBonusScreen();
	MakeFadeEvent(true, 1);
	ResetFramesPerSecond();

	DoSkipWalkOn();
	DoSkipThrowClovers();
	DoMouseBonus();

			/* DO DELAY */

	{
		float	timer = 2.0f;
		do
		{
			CalcFramesPerSecond();
			UpdateInput();
			MoveObjects();
			OGL_DrawScene(DrawBonusCallback);
			timer -= gFramesPerSecondFrac;
		}while(timer > 0.0f);
	}

	if (gLevelNum < LEVEL_NUM_PARK)					// dont do Save Game if just won
		DoSaveSelect();


			/* CLEANUP */

	OGL_FadeOutScene(DrawObjects, NULL);
	FreeBonusScreen();
}



/********************* SETUP BONUS SCREEN **********************/

static void SetupBonusScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
static const OGLVector3D	fillDirection1 = { -.7, -.5, -1.0 };
int					i;
ObjNode	*newObj;

	gBonusScoreAlpha = -.7f;
	gSaveAlpha = 0;

			/* SET ANAGLYPH INFO */

	if (gGamePrefs.anaglyph)
	{
		gAnaglyphScaleFactor 	= 1.0f;
		gAnaglyphFocallength	= 200.0f * gAnaglyphScaleFactor;	// set camera info
		gAnaglyphEyeSeparation 	= 25.0f * gAnaglyphScaleFactor;
	}

			/* FIX BLUE CLOVER COUNT */

//	gPlayerInfo.numMiceRescued = 2;	//-------
//	gPlayerInfo.numGreenClovers = 5;	//---------
//	gPlayerInfo.numBlueClovers = 1;	//---------
//	gPlayerInfo.numGoldClovers = 4;		//---------


	gPlayerInfo.numBlueClovers /= 4;						// divide by 4 since it takes 4 blue clovers to get 1 bonus
	gNumFullGoldClovers = gPlayerInfo.numGoldClovers/4;		// similar for gold except we dont want to modify the value since we need it for later

	if (gNumFullGoldClovers > 0)							// if we got it then clear the master value
		gPlayerInfo.numGoldClovers = 0;

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= .95;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 2500;

	viewDef.styles.useFog			= true;
	viewDef.styles.fogStart			= viewDef.camera.yon * .5f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * 1.0f;
	viewDef.view.clearColor.r 		= .5;
	viewDef.view.clearColor.g 		= .5;
	viewDef.view.clearColor.b		= .8;
	viewDef.view.clearBackBuffer	= true;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.y		= 50;
	viewDef.camera.from.z		= 800;

	viewDef.camera.to.y 		= 50.0f;

	viewDef.lights.ambientColor.r = .2;
	viewDef.lights.ambientColor.g = .2;
	viewDef.lights.ambientColor.b = .15;
	viewDef.lights.fillDirection[0] = fillDirection1;

	OGL_SetupWindow(&viewDef, &gGameView);


				/************/
				/* LOAD ART */
				/************/

	InitEffects();

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Bonus.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_BONUS);

	LoadFoliage();


			/* LOAD SPRITES */

	LoadSpriteGroupFromSeries(SPRITE_GROUP_LEVELSPECIFIC, BONUS_SObjType_COUNT, "Bonus");


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_SKIP_EXPLORE);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SKIP_EXPLORE, 0,				// set sphere map on geometry texture
									1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkYosemite);


	LoadASkeleton(SKELETON_TYPE_MOUSE);


				/* LOAD AUDIO */

	LoadSoundBank(SOUND_BANK_BONUS);



			/*******************/
			/* MAKE BACKGROUND */
			/*******************/

			/* MAKE TULIPS */

	for (i = 0; i < 20; i++)
	{

		gNewObjectDefinition.group 		= MODEL_GROUP_FOLIAGE;
		gNewObjectDefinition.type 		= FOLIAGE_ObjType_Tulip1 + (MyRandomLong()&0x3);
		gNewObjectDefinition.coord.x	= RandomFloat2() * 1400.0f;
		gNewObjectDefinition.coord.y 	= -600.0f;
		gNewObjectDefinition.coord.z 	= -680.0f - RandomFloat() * 1000.0f;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= 5;
		gNewObjectDefinition.moveCall 	= MoveBonusFlower;
		gNewObjectDefinition.rot 		= RandomFloat()*PI2;
		gNewObjectDefinition.scale 		= 5.0 + RandomFloat() * 3.0f;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->WaveXIndex = newObj->Coord.x * .003f;
		newObj->WaveZIndex = newObj->Coord.z * .003f;
	}


			/* BRICK */

	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
	gNewObjectDefinition.type 		= BONUS_ObjType_Brick;
	gNewObjectDefinition.coord.x	= 0;
	gNewObjectDefinition.coord.y 	= -169.0f;
	gNewObjectDefinition.coord.z 	= 250.0f;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 20;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 10.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	CreateCollisionBoxFromBoundingBox(newObj, 1, 1);
	newObj->CType = CTYPE_BLOCKSHADOW;
}


/********************* MOVE WAVING FLOWER **********************/

static void MoveBonusFlower(ObjNode *theNode)
{
	theNode->WaveXIndex += gFramesPerSecondFrac * 1.5f;
	theNode->WaveZIndex += gFramesPerSecondFrac * 1.5f;

	theNode->Rot.x = sin(theNode->WaveXIndex) * .04f;
	theNode->Rot.z = sin(theNode->WaveZIndex) * .04f;

	UpdateObjectTransforms(theNode);
}



/********************** FREE BONUS SCREEN **********************/

static void FreeBonusScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeEffects();
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUND_BANK_BONUS);
}

#pragma mark -


/**************** DO SKIP WALK ON ********************/

static void DoSkipWalkOn(void)
{

			/* MAKE SKIP */

	gNewObjectDefinition.type 		= SKELETON_TYPE_SKIP_EXPLORE;
	gNewObjectDefinition.animNum	= PLAYER_ANIM_WALK;
	gNewObjectDefinition.coord.x 	= -600.0f;
	gNewObjectDefinition.coord.y 	= -100;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_DONTCULL|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall	= nil;
	gNewObjectDefinition.rot 		= -2.5f;
	gNewObjectDefinition.scale 		= 3.0;

	gSkip = MakeNewSkeletonObject(&gNewObjectDefinition);
	gSkip->Skeleton->AnimSpeed = 1.8f;

	gSkip->ThrowNow = false;

	AttachShadowToObject(gSkip, SHADOW_TYPE_CIRCULAR, 7,7, true);


			/****************/
			/* WALK SKIP ON */
			/****************/

	while(gSkip->Coord.x < -150.0f)
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();
		gSkip->Rot.y += fps * .3f;
		gSkip->Coord.x -= sin(gSkip->Rot.y) * 200.0f * fps;
		gSkip->Coord.z -= cos(gSkip->Rot.y) * 200.0f * fps;
		UpdateObjectTransforms(gSkip);
		UpdateShadow(gSkip);

				/* DRAW */

		OGL_DrawScene(DrawBonusCallback);
	}


}



/**************** DO SKIP THROW CLOVERS ********************/

static void DoSkipThrowClovers(void)
{
Boolean	done = false;

			/* SEE IF WE EVEN HAVE ANY CLOVERS */

	if ((gPlayerInfo.numGreenClovers + gPlayerInfo.numBlueClovers + gNumFullGoldClovers) <= 0)
	{
		MorphToSkeletonAnim(gSkip->Skeleton, PLAYER_ANIM_PERSONALITY3_DANCE, 6);
		return;
	}
	else
	{
		MorphToSkeletonAnim(gSkip->Skeleton, PLAYER_ANIM_CLOVERTOSS, 4);
		gSkip->Skeleton->AnimSpeed = 1.5f;
	}


	while(!done)
	{
		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE */

		MoveObjects();

		if (gSkip->ThrowNow)							// see if throw clover now
		{
			done = TossClover();
			gSkip->ThrowNow = false;
		}


				/* DRAW */

		OGL_DrawScene(DrawBonusCallback);
	}
}


/************************ TOSS CLOVER *****************************/

static Boolean TossClover(void)
{
const OGLPoint3D handOff = {0, -15, -15};
ObjNode	*newObj;
int		type;


			/* SEE WHICH CLOVER WE'VE GOT */

	if (gPlayerInfo.numGreenClovers)							// see if have green
	{
		gPlayerInfo.numGreenClovers--;
		type = 0;
		gScore += POINTS_GREENCLOVER;

		PlayEffect_Parms(EFFECT_CLOVERBONUS, FULL_CHANNEL_VOLUME/2, FULL_CHANNEL_VOLUME/2, NORMAL_CHANNEL_RATE*3/2);
	}
	else
	if (gPlayerInfo.numBlueClovers)								// no green, so see if blue
	{
		gPlayerInfo.numBlueClovers--;
		type = 1;
		gScore += POINTS_BLUECLOVER;
		PlayEffect_Parms(EFFECT_CLOVERBONUS, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME/2, NORMAL_CHANNEL_RATE);
	}
	else														// must be a gold
	{
		gNumFullGoldClovers--;
		type = 2;
		gScore += POINTS_GOLDCLOVER;
		PlayEffect_Parms(EFFECT_CLOVERBONUS, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE*2);
	}



			/* MAKE CLOVER */

	FindCoordOnJoint(gSkip, PLAYER_JOINT_UPPER_RIGHT_ELBOW, &handOff, &gNewObjectDefinition.coord);

	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
	gNewObjectDefinition.type 		= BONUS_ObjType_GreenClover + type;
	gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED;
	gNewObjectDefinition.slot 		= 200;
	gNewObjectDefinition.moveCall 	= MoveTossedClover;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->Delta.x = 500.0f;
	newObj->Delta.y = 400.0f;

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 2.5,2.5, true);


			/* SEE IF THAT WAS ALL */

	if ((gPlayerInfo.numGreenClovers + gPlayerInfo.numBlueClovers + gNumFullGoldClovers) <= 0)
	{

		MorphToSkeletonAnim(gSkip->Skeleton, PLAYER_ANIM_PERSONALITY3_DANCE, 6);
		gSkip->MoveCall = MoveBonusSkipDancing;
		return(true);
	}


	return(false);
}

/******************** MOVE BONUS SKIP DANCING ************************/

static void MoveBonusSkipDancing(ObjNode *theNode)
{
	theNode->Rot.y -= gFramesPerSecondFrac;
	if (theNode->Rot.y <= -PI)
		theNode->Rot.y = -PI;


	UpdateObjectTransforms(theNode);


}


/******************** MOVE TOSSED CLOVER ************************/

static void MoveTossedClover(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (theNode->StatusBits & STATUS_BIT_ISCULLED)			// see if gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

			/* MOVE */

	gDelta.y -= 2000.0f * fps;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	if (gCoord.y < -150.0f)
	{
		gCoord.y = -150.0f;
		gDelta.y = -gDelta.y * .5f;

	}

			/* SPIN */

	theNode->Rot.x += fps;
	theNode->Rot.z += fps * 2.0f;


			/* UPDATE */

	UpdateObject(theNode);
}


#pragma mark -


/**************** DO MOUSE BONUS ********************/

static void DoMouseBonus(void)
{
ObjNode	*mouse;
float	timer;
int		i;


	if (gPlayerInfo.numMiceRescued == 0)
		return;


	for (i = 0; i < gPlayerInfo.numMiceRescued; i++)
	{

				/* MAKE MOUSE */

		gNewObjectDefinition.type 		= SKELETON_TYPE_MOUSE;
		gNewObjectDefinition.animNum	= 1;
		gNewObjectDefinition.coord.x 	= 1300.0f;
		gNewObjectDefinition.coord.y 	= -10;
		gNewObjectDefinition.coord.z 	= -550;
		gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot 		= 500;
		gNewObjectDefinition.moveCall	= MoveBonusMouse;
		gNewObjectDefinition.rot 		= 2.2;
		gNewObjectDefinition.scale 		= 3.2;

		mouse = MakeNewSkeletonObject(&gNewObjectDefinition);

		mouse->Skeleton->AnimSpeed = .8f;

		AttachShadowToObject(mouse, SHADOW_TYPE_CIRCULAR, 7,7, true);

		gScore += POINTS_MOUSE;

		PlayEffect(EFFECT_MOUSEBONUS);

		for (timer = 0; timer < 1.2f; timer += gFramesPerSecondFrac)		// delay to next mouse
		{
			CalcFramesPerSecond();
			UpdateInput();
			MoveObjects();
			OGL_DrawScene(DrawBonusCallback);
		}
	}



			/* EXIT DELAY */

	for (timer = 0; timer < 3.0f; timer += gFramesPerSecondFrac)
	{
		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(DrawBonusCallback);
	}


}

/******************** MOVE BONUS MOUSE *****************************/

static void MoveBonusMouse(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	r;

	GetObjectInfo(theNode);

	if ((theNode->StatusBits & STATUS_BIT_ISCULLED)	&& (gCoord.x < 0.0f))		// see if gone
	{
		DeleteObject(theNode);
		return;
	}


			/* MOVE */

	r = theNode->Rot.y;
	gCoord.x -= sin(r) * 400.0f * fps;
	gCoord.z -= cos(r) * 400.0f * fps;


			/* UPDATE */

	UpdateObject(theNode);

}


#pragma mark -

static void SelectIcon(int iconNum)
{
	bool wantSave = iconNum == 0;

	if (wantSave != gSaveGame)
	{
		gSaveGame = wantSave;
		gSaveWobble = 0;

		int lv = wantSave? FULL_CHANNEL_VOLUME/3: FULL_CHANNEL_VOLUME/5;
		int rv = wantSave? FULL_CHANNEL_VOLUME/5: FULL_CHANNEL_VOLUME/3;
		PlayEffect_Parms(EFFECT_CHANGESELECT, lv, rv, NORMAL_CHANNEL_RATE);
	}
}

/**************** DO SAVE SELECT ********************/

static void DoSaveSelect(void)
{
	gSaveGame = true;
	MakeDarkenPane();

	bool validMouse = false;

	while (!IsNeedDown(kNeed_UIConfirm)
		&& !(IsClickDown(SDL_BUTTON_LEFT) && validMouse))
	{
		CalcFramesPerSecond();
		UpdateInput();

				/* SEE IF USER CHANGE SELECTION */

		if (IsNeedDown(kNeed_UIPrev) && !gSaveGame)
		{
			SelectIcon(0);
			SetSystemCursor(-1);
			validMouse = false;
		}
		else if (IsNeedDown(kNeed_UINext) && gSaveGame)
		{
			SelectIcon(1);
			SetSystemCursor(-1);
			validMouse = false;
		}
		else if (gMouseMotionNow)
		{
			OGLPoint2D mouse = GetMouseCoordsIn2DLogicalRect();
			float d0 = OGLPoint2D_Distance(&mouse, &kSaveIconPos[0]);
			float d1 = OGLPoint2D_Distance(&mouse, &kSaveIconPos[1]);

			if (SDL_min(d0, d1) < 60)
			{
				SelectIcon(d0 < d1 ? 0: 1);
				SetSystemCursor(SDL_SYSTEM_CURSOR_HAND);
				validMouse = true;
			}
			else
			{
				SetSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
				validMouse = false;
			}
		}


				/* MOVE & DRAW */

		MoveObjects();
		OGL_DrawScene(DrawBonusCallback);

				/* FADE IN ICONS */

		gSaveAlpha += gFramesPerSecondFrac;
		if (gSaveAlpha > 1.0f)
			gSaveAlpha = 1.0f;

		gSaveWobble += gFramesPerSecondFrac * 10.0f;
	}

	PlayEffect_Parms(EFFECT_CHANGESELECT,FULL_CHANNEL_VOLUME/4,FULL_CHANNEL_VOLUME/4,NORMAL_CHANNEL_RATE*3/2);

		/* SAVE GAME */

	if (gSaveGame)
	{
		DoFileScreen(FILE_SCREEN_TYPE_SAVE, DrawObjects);
	}
}


#pragma mark -

/***************** DRAW BONUS CALLBACK *******************/

static void DrawBonusCallback(void)
{
	DrawObjects();


			/* DRAW SPRITES */

	OGL_PushState();
	SetInfobarSpriteState();

	DrawBonusScore();
	DrawSave();

	OGL_PopState();
}


/******************** DRAW BONUS SCORE ************************/

#define	SCORE_SPACING	30.0f

static void DrawBonusScore(void)
{
Str255	s;
int		n,i,texNum;
float	x;



			/* FADE IN */

	gBonusScoreAlpha += gFramesPerSecondFrac * .5f;
	if (gBonusScoreAlpha > 1.0f)
		gBonusScoreAlpha = 1.0f;

	if (gBonusScoreAlpha <= 0.0f)
		goto bail;

	gGlobalTransparency = gBonusScoreAlpha;


			/* DRAW "SCORE" */

	DrawInfobarSprite2_Centered(640/2, 50, 200, SPRITE_GROUP_LEVELSPECIFIC, BONUS_SObjType_Score);

			/* DRAW SCORE */

	NumToString(gScore, s);							// convert score to a text string


	n = s[0];										// get str len
	x = 640/2 - (((n-1) * SCORE_SPACING) * .5f);
	for (i = 1; i <= n; i++)
	{
		texNum = BONUS_SObjType_0 + s[i] - '0';		// convert char to sprite
		if (texNum != -1)
			DrawInfobarSprite2_Centered(x, 115, SCORE_SPACING * 1.6f, SPRITE_GROUP_LEVELSPECIFIC, texNum);
		x += SCORE_SPACING;
	}


			/* CLEANUP */
bail:
	gGlobalTransparency = 1.0f;
}


/************************ DRAW SAVE *********************************/

static void DrawSave(void)
{
float	s, s0, s1;

	if (gSaveAlpha <= 0.0f)
		return;

	gGlobalTransparency = gSaveAlpha;

	s = 100 + (sinf(gSaveWobble) + 1.0f) * 20.0f;

	if (gSaveGame)
	{
		s0 = s;
		s1 = 100;
	}
	else
	{
		s0 = 100;
		s1 = s;
	}

	DrawInfobarSprite2_Centered(kSaveIconPos[0].x, kSaveIconPos[0].y, s0, SPRITE_GROUP_LEVELSPECIFIC, BONUS_SObjType_SaveIcon);
	DrawInfobarSprite2_Centered(kSaveIconPos[1].x, kSaveIconPos[1].y, s1, SPRITE_GROUP_LEVELSPECIFIC, BONUS_SObjType_NoSaveIcon);

	gGlobalTransparency = 1.0f;
}
