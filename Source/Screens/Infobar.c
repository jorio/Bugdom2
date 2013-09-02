/****************************/
/*   	INFOBAR.C		    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include	"infobar.h"
#include <AGL/aglmacro.h>
#include	"dialog.h"
#include	"3dmath.h"


extern	float					gCurrentAspectRatio,gGlobalTransparency,gFramesPerSecondFrac;
extern	int						gLevelNum,gSlotCarRacingMode,gNumAntHills, gNumAntHillsDestroyed;
extern	FSSpec					gDataSpec;
extern	long					gTerrainUnitWidth,gTerrainUnitDepth;
extern	OGLColorRGB				gGlobalColorFilter;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	PrefsType			gGamePrefs;
extern	u_long					gGlobalMaterialFlags;
extern	SpriteType	*gSpriteGroupList[];
extern	AGLContext		gAGLContext;
extern	short					gTotalTicks, gTotalFleas, gNumKilledFleas, gNumKilledTicks;
extern	float					gSlotCarStartTimer;
extern	int						gTotalRedClovers, gGatheredRedClovers, gNumCaughtFish, gNumFoodOnBasket, gNumMice;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawInfobarSprite_Rotated(float x, float y, float size, short texNum, float rot, const OGLSetupOutputType *setupInfo);
static void DrawInfobarSprite_Centered(float x, float y, float size, short texNum, const OGLSetupOutputType *setupInfo);
static void Infobar_DrawHealth(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawFlight(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawLives(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawKey(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawMap(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawTickAndFleaCount(const OGLSetupOutputType *setupInfo);
//static void Infobar_DrawRacingInfo(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawMice(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawClovers(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawAntHills(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawRedClovers(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawFish(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawFood(const OGLSetupOutputType *setupInfo);


/****************************/
/*    CONSTANTS             */
/****************************/

#define MAP_SCALE	150.0f
#define	MAP_X		410.0f			//(640.0f - MAP_SCALE)
#define	MAP_Y		0.0f

#define	HEALTH_X		22.0f
#define	HEALTH_Y		20.0f
#define	HEALTH_SCALE	16.0f

#define	LIVES_X			170.0f
#define	LIVES_Y			0.0f
#define	LIVES_SCALE		30.0f

#define	FLIGHT_X		88.0f
#define	FLIGHT_Y		20.0f
#define	FLIGHT_SCALE	16.0f

#define	COLORKEY_SCALE	40.0f
#define	COLORKEY_X		143.0f
#define	COLORKEY_Y		32.0f


#define	MOUSE_SCALE		20.0f
#define	MOUSE_X			0.0f
#define	MOUSE_Y			100.0f

#define	TICK_X			0.0f
#define	TICK_Y			MOUSE_Y
#define	TICK_SCALE		30.0f

#define	FLEA_X			(TICK_X + TICK_SCALE)
#define	FLEA_Y			TICK_Y
#define	FLEA_SCALE		TICK_SCALE


#define	ANTHILL_SCALE	22.0f
#define	ANTHILL_X		0.0f
#define	ANTHILL_Y		MOUSE_Y

#define	CLOVERS_SCALE	40.0f
#define	CLOVERS_X		595.0f
#define	CLOVERS_Y		0.0f

#define	REDCLOVER_SCALE		20.0f
#define	REDCLOVERS_X		(640.0f - REDCLOVER_SCALE)
#define	REDCLOVERS_Y		100.0f

#define	FISH_X			0.0f
#define	FISH_Y			170.0f
#define	FISH_SCALE		30.0f

#define	FOOD_X			0.0f
#define	FOOD_Y			170.0f
#define	FOOD_SCALE		30.0f


/*********************/
/*    VARIABLES      */
/*********************/

Boolean			gHideInfobar = false;
static float	gRedCloverAlpha, gFishAlpha, gFoodAlpha;
Boolean			gShowRedClovers, gShowFish, gShowFood;
Byte			gFoodTypes[FOOD_TO_GET];

/********************* INIT INFOBAR ****************************/
//
// Called at beginning of level
//

void InitInfobar(OGLSetupOutputType *setupInfo)
{

#pragma unused(setupInfo)

	gShowRedClovers = false;
	gFishAlpha = gFoodAlpha = 1.0f;
	gShowFish = gShowFood = true;

}


/****************** DISPOSE INFOBAR **********************/

void DisposeInfobar(void)
{

}


/***************** SET INFOBAR SPRITE STATE *******************/

void SetInfobarSpriteState(void)
{
AGLContext agl_ctx = gAGLContext;

	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);								// no z-buffer

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_CLAMP_U;	// clamp all textures


			/* INIT MATRICES */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


/********************** DRAW INFOBAR ****************************/

void DrawInfobar(OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;

	if (gHideInfobar)
		return;

		/************/
		/* SET TAGS */
		/************/

	OGL_PushState();

	if (setupInfo->useFog)
		glDisable(GL_FOG);

	SetInfobarSpriteState();



		/***************/
		/* DRAW THINGS */
		/***************/


			/* DRAW CORNER LEAVES */

	DrawInfobarSprite(0, -7, 230, INFOBAR_SObjType_LeftCorner, setupInfo);
	DrawInfobarSprite(CLOVERS_X-26, -7, CLOVERS_SCALE * 2, INFOBAR_SObjType_RightCorner, setupInfo);



		/* DRAW STUFF */

	Infobar_DrawFlight(setupInfo);
	Infobar_DrawHealth(setupInfo);
	Infobar_DrawLives(setupInfo);
	Infobar_DrawKey(setupInfo);
	Infobar_DrawMap(setupInfo);
	Infobar_DrawClovers(setupInfo);

	switch(gLevelNum)
	{
		case	LEVEL_NUM_GNOMEGARDEN:
		case	LEVEL_NUM_SIDEWALK:
		case	LEVEL_NUM_GARBAGE:
		case	LEVEL_NUM_PLAYROOM:
				Infobar_DrawMice(setupInfo);
				break;

		case	LEVEL_NUM_FIDO:
				Infobar_DrawTickAndFleaCount(setupInfo);
				break;

		case	LEVEL_NUM_CLOSET:
				Infobar_DrawMice(setupInfo);
				Infobar_DrawRedClovers(setupInfo);
				break;

		case	LEVEL_NUM_BALSA:
				Infobar_DrawAntHills(setupInfo);
				break;

		case	LEVEL_NUM_PARK:
				Infobar_DrawFish(setupInfo);
				Infobar_DrawFood(setupInfo);
				Infobar_DrawMice(setupInfo);
				break;
	}

	DrawDialogMessage(setupInfo);


			/***********/
			/* CLEANUP */
			/***********/

	OGL_PopState();
	gGlobalMaterialFlags = 0;
	if (setupInfo->useFog)
		glEnable(GL_FOG);
}

#pragma mark -


/******************** DRAW INFOBAR SPRITE **********************/

void DrawInfobarSprite(float x, float y, float size, short texNum, const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo, setupInfo);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 		y);
	glTexCoord2f(1,1);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,0);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,0);	glVertex2f(x,		y+(size*aspect));
	glEnd();
}

/******************** DRAW INFOBAR SPRITE: CENTERED **********************/
//
// Coords are for center of sprite, not upper left
//

static void DrawInfobarSprite_Centered(float x, float y, float size, short texNum, const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo, setupInfo);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

	x -= size*.5f;
	y -= (size*aspect)*.5f;

			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 		y);
	glTexCoord2f(1,1);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,0);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,0);	glVertex2f(x,		y+(size*aspect));
	glEnd();
}


/******************** DRAW INFOBAR SPRITE 2 **********************/
//
// This version lets user pass in the sprite group
//

void DrawInfobarSprite2(float x, float y, float size, short group, short texNum, const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[group][texNum].materialObject;
	MO_DrawMaterial(mo, setupInfo);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 		y);
	glTexCoord2f(1,1);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,0);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,0);	glVertex2f(x,		y+(size*aspect));
	glEnd();
}

/******************** DRAW INFOBAR SPRITE 2: CENTERED **********************/
//
// This version lets user pass in the sprite group
//

void DrawInfobarSprite2_Centered(float x, float y, float size, short group, short texNum, const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[group][texNum].materialObject;
	MO_DrawMaterial(mo, setupInfo);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

	x -= size*.5f;
	y -= (size*aspect)*.5f;

			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 		y);
	glTexCoord2f(1,1);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,0);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,0);	glVertex2f(x,		y+(size*aspect));
	glEnd();
}



/******************** DRAW INFOBAR SPRITE: ROTATED **********************/

static void DrawInfobarSprite_Rotated(float x, float y, float size, short texNum, float rot, const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect, xoff, yoff;
OGLPoint2D			p[4];
OGLMatrix3x3		m;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo, setupInfo);

				/* SET COORDS */

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

	xoff = size*.5f;
	yoff = (size*aspect)*.5f;

	p[0].x = -xoff;		p[0].y = -yoff;
	p[1].x = xoff;		p[1].y = -yoff;
	p[2].x = xoff;		p[2].y = yoff;
	p[3].x = -xoff;		p[3].y = yoff;

	OGLMatrix3x3_SetRotate(&m, rot);
	OGLPoint2D_TransformArray(p, &m, p, 4);


			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(p[0].x + x, p[0].y + y);
	glTexCoord2f(1,1);	glVertex2f(p[1].x + x, p[1].y + y);
	glTexCoord2f(1,0);	glVertex2f(p[2].x + x, p[2].y + y);
	glTexCoord2f(0,0);	glVertex2f(p[3].x + x, p[3].y + y);
	glEnd();
}



#pragma mark -


/********************** DRAW LIVES *************************/

static void Infobar_DrawLives(const OGLSetupOutputType *setupInfo)
{
float	x = LIVES_X;
int		i, n = gPlayerInfo.lives-1;

	for (i = 0; i < n; i++)
	{
		DrawInfobarSprite(x, LIVES_Y, LIVES_SCALE, INFOBAR_SObjType_Life, setupInfo);
		x += LIVES_SCALE * .8f;
	}
}


/********************** DRAW CLOVERS *************************/

static void Infobar_DrawClovers(const OGLSetupOutputType *setupInfo)
{
	if (gPlayerInfo.numBlueClovers)
		DrawInfobarSprite(CLOVERS_X, CLOVERS_Y, CLOVERS_SCALE, INFOBAR_SObjType_BlueClover1-1 + gPlayerInfo.numBlueClovers, setupInfo);

	if (gPlayerInfo.numGoldClovers)
		DrawInfobarSprite(CLOVERS_X - 24, CLOVERS_Y + (CLOVERS_SCALE * .64), CLOVERS_SCALE, INFOBAR_SObjType_GoldClover1-1 + gPlayerInfo.numGoldClovers, setupInfo);
}


#define	NUM_HEALTH_DOTS	9

/********************** DRAW HEALTH *************************/

static void Infobar_DrawHealth(const OGLSetupOutputType *setupInfo)
{
int			i, n;
float		x, a, f;
OGLPoint2D	p[NUM_HEALTH_DOTS], o;
OGLMatrix3x3	m;
static float wiggle = 0;

			/* CALC HEALTH DOT COORDS */

	wiggle += gFramesPerSecondFrac * 1.5f;
	a = wiggle;

	x = HEALTH_X;

	for (i = 0; i < NUM_HEALTH_DOTS; i++)
	{
		p[i].x = x;
		p[i].y = HEALTH_Y + sin(a) * 7.0f;

		x += HEALTH_SCALE * .5f;
		a += 1.1f;
	}

	o.x = HEALTH_X;																	// rotate into position
	o.y = HEALTH_Y;
	OGLMatrix3x3_SetRotateAboutPoint(&m, &o, 1.1f);
	OGLPoint2D_TransformArray(p, &m, p, NUM_HEALTH_DOTS);


			/* DRAW LEAF */

//	DrawInfobarSprite(HEALTH_X-25, HEALTH_Y-23, 90, INFOBAR_SObjType_HealthLeaf, setupInfo);


			/*************/
			/* DRAW DOTS */
			/*************/

	f = gPlayerInfo.health * (float)NUM_HEALTH_DOTS;
	n = (int)f;
	f -= (float)n;								// f is now just the remaining fraction

	for (i = 0; i < NUM_HEALTH_DOTS; i++)
	{
		if (i == n)								// do fractional fade of remainder
			gGlobalTransparency = .2f + (f * .8f);
		else
		if (i > n)								// fade the non-active ones
			gGlobalTransparency = .2;

		DrawInfobarSprite_Centered(p[i].x, p[i].y, HEALTH_SCALE, INFOBAR_SObjType_Health, setupInfo);

	}


	gGlobalTransparency = 1.0f;
}

#define	NUM_FLIGHT_DOTS	9


/********************** DRAW FLIGHT *************************/

static void Infobar_DrawFlight(const OGLSetupOutputType *setupInfo)
{
int		n,i;
float	x, a, f;
OGLPoint2D	p[NUM_FLIGHT_DOTS], o;
OGLMatrix3x3	m;
static float wiggle = 0;

			/* CALC HEALTH DOT COORDS */

	wiggle += gFramesPerSecondFrac * 1.5f;
	a = wiggle;

	x = FLIGHT_X;

	for (i = 0; i < NUM_FLIGHT_DOTS; i++)
	{
		p[i].x = x;
		p[i].y = FLIGHT_Y + sin(a) * 4.0f;

		x += FLIGHT_SCALE * .5f;
		a += 1.1f;
	}

	o.x = FLIGHT_X;																	// rotate into position
	o.y = FLIGHT_Y;
	OGLMatrix3x3_SetRotateAboutPoint(&m, &o, 1.5f);
	OGLPoint2D_TransformArray(p, &m, p, NUM_FLIGHT_DOTS);



			/*************/
			/* DRAW DOTS */
			/*************/

	f = gPlayerInfo.glidePower * (float)NUM_FLIGHT_DOTS;
	n = (int)f;
	f -= (float)n;								// f is now just the remaining fraction


	for (i = 0; i < NUM_FLIGHT_DOTS; i++)
	{
		if (i == n)								// do fractional fade of remainder
			gGlobalTransparency = .2f + (f * .8f);
		else
		if (i > n)								// fade the non-active ones
			gGlobalTransparency = .2;

		DrawInfobarSprite_Centered(p[i].x, p[i].y, FLIGHT_SCALE, INFOBAR_SObjType_Flight, setupInfo);
	}


	gGlobalTransparency = 1.0f;
}



/********************** DRAW KEY *************************/

static void Infobar_DrawKey(const OGLSetupOutputType *setupInfo)
{
int		i;
static float wobble = 0;
float	scale;

	wobble += gFramesPerSecondFrac * 3.0f;
	scale = COLORKEY_SCALE + sin(wobble) * (COLORKEY_SCALE * .15f);


	for (i = 0; i < NUM_KEY_TYPES; i++)
	{
		if (gPlayerInfo.hasKey[i])
		{
			DrawInfobarSprite2_Centered(COLORKEY_X,COLORKEY_Y, scale, SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_RedKeyIcon + i, setupInfo);
			return;
		}
	}

}


/********************** DRAW MAP *************************/

static void Infobar_DrawMap(const OGLSetupOutputType *setupInfo)
{
float	x,y;

	if (!gPlayerInfo.hasMap)
		return;

	switch(gLevelNum)
	{
		case	LEVEL_NUM_GNOMEGARDEN:
				gGlobalTransparency = .90f;
				DrawInfobarSprite2(MAP_X, MAP_Y, MAP_SCALE, SPRITE_GROUP_LEVELSPECIFIC, GARDEN_SObjType_Map, setupInfo);

				x = (gPlayerInfo.coord.x / (float)gTerrainUnitWidth) * (MAP_SCALE/2) + 38.0f;
				y = (gPlayerInfo.coord.z / (float)gTerrainUnitDepth) * (MAP_SCALE/2);

				x += MAP_X;
				y += MAP_Y;

				DrawInfobarSprite_Rotated(x, y, MAP_SCALE/15, INFOBAR_SObjType_MapDot, -gPlayerInfo.objNode->Rot.y, setupInfo);
				break;


		case	LEVEL_NUM_PLAYROOM:
				gGlobalTransparency = .90f;

				DrawInfobarSprite2(MAP_X, MAP_Y, MAP_SCALE, SPRITE_GROUP_LEVELSPECIFIC, PLAYROOM_SObjType_Map, setupInfo);

				x = (gPlayerInfo.coord.x / (float)gTerrainUnitWidth) * (MAP_SCALE * .655f) + 24.0f;
				y = (gPlayerInfo.coord.z / (float)gTerrainUnitDepth) * (MAP_SCALE * .43f) + 5.0f;

				x += MAP_X;
				y += MAP_Y;

				DrawInfobarSprite_Rotated(x, y, MAP_SCALE/15, INFOBAR_SObjType_MapDot, -gPlayerInfo.objNode->Rot.y, setupInfo);
				break;

		case	LEVEL_NUM_CLOSET:

				DrawInfobarSprite2(MAP_X, MAP_Y, MAP_SCALE, SPRITE_GROUP_LEVELSPECIFIC, CLOSET_SObjType_Map, setupInfo);

				x = (gPlayerInfo.coord.x / (float)gTerrainUnitWidth) * (MAP_SCALE * .55f) + 34.0f;
				y = (gPlayerInfo.coord.z / (float)gTerrainUnitDepth) * (MAP_SCALE * .44f) + 1.0f;

				x += MAP_X;
				y += MAP_Y;

				DrawInfobarSprite_Rotated(x, y, MAP_SCALE/15, INFOBAR_SObjType_MapDot, -gPlayerInfo.objNode->Rot.y, setupInfo);
				break;


		case	LEVEL_NUM_PARK:

				DrawInfobarSprite2(MAP_X, MAP_Y, MAP_SCALE, SPRITE_GROUP_LEVELSPECIFIC, PARK_SObjType_Map, setupInfo);

				x = (gPlayerInfo.coord.x / (float)gTerrainUnitWidth) * (MAP_SCALE * .65f) + 25.0f;
				y = (gPlayerInfo.coord.z / (float)gTerrainUnitDepth) * (MAP_SCALE * .485f) + 1.0f;

				x += MAP_X;
				y += MAP_Y;

				DrawInfobarSprite_Rotated(x, y, MAP_SCALE/15, INFOBAR_SObjType_MapDot, -gPlayerInfo.objNode->Rot.y, setupInfo);
				break;


	}
	gGlobalTransparency = 1.0f;
}


/********************** DRAW TICK AND FLEA COUNT *************************/

static void Infobar_DrawTickAndFleaCount(const OGLSetupOutputType *setupInfo)
{
float	x,y;
int		i;
			/**************/
			/* DRAW TICKS */
			/**************/

	x = TICK_X;
	y = TICK_Y;

	for (i = 0; i < gTotalTicks; i++)
	{
		if (gNumKilledTicks <= i)
			gGlobalTransparency = .3f;
		else
			gGlobalTransparency = 1.0f;

		DrawInfobarSprite2(x, y, TICK_SCALE, SPRITE_GROUP_LEVELSPECIFIC, FIDO_SObjType_Tick, setupInfo);

		y += TICK_SCALE;

	}



			/**************/
			/* DRAW FLEAS */
			/**************/

	x = FLEA_X;
	y = FLEA_Y;

	for (i = 0; i < gTotalFleas; i++)
	{
		if (gNumKilledFleas <= i)
			gGlobalTransparency = .3f;
		else
			gGlobalTransparency = 1.0f;

		DrawInfobarSprite2(x, y, FLEA_SCALE, SPRITE_GROUP_LEVELSPECIFIC, FIDO_SObjType_Flea, setupInfo);

		y += FLEA_SCALE;

	}


	gGlobalTransparency = 1.0f;

}


/********************** DRAW MICE *************************/

static void Infobar_DrawMice(const OGLSetupOutputType *setupInfo)
{
float	x,y;
int		i;

			/* DRAW THE MOUSE SPRITE */

	x = MOUSE_X;
	y = MOUSE_Y;

	for (i = 0; i < gNumMice; i++)
	{
		if (gPlayerInfo.numMiceRescued <= i)
			gGlobalTransparency = .3f;
		else
			gGlobalTransparency = 1.0f;

		DrawInfobarSprite2(x, y, MOUSE_SCALE, SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_Mouse, setupInfo);

		if (i == 9)									// see if next col
		{
			y = MOUSE_Y;
			x += MOUSE_SCALE;
		}
		else
			y += MOUSE_SCALE;

	}

	gGlobalTransparency = 1.0f;
}


/********************** DRAW ANT HILLS *************************/

static void Infobar_DrawAntHills(const OGLSetupOutputType *setupInfo)
{
float	x, y;
int		i;

			/* DRAW THE ANTHILL SPRITE */

	x = ANTHILL_X;
	y = ANTHILL_Y;

	for (i = 0; i < gNumAntHills; i++)
	{
		if (gNumAntHillsDestroyed <= i)
			gGlobalTransparency = .3f;
		else
			gGlobalTransparency = 1.0f;


		DrawInfobarSprite2(x, y, ANTHILL_SCALE, SPRITE_GROUP_LEVELSPECIFIC, BALSA_SObjType_AntHillIcon, setupInfo);

		if (i == 7)									// see if go to next col
		{
			y = ANTHILL_Y;
			x += ANTHILL_SCALE;
		}
		else
			y += ANTHILL_SCALE;
	}

	gGlobalTransparency = 1.0f;
}


/********************** DRAW RED CLOVERS *************************/

static void Infobar_DrawRedClovers(const OGLSetupOutputType *setupInfo)
{
float	x, y;
int		i;

			/* FADE IN/OUT */

	if (gShowRedClovers)
	{
		gRedCloverAlpha += gFramesPerSecondFrac;
		if (gRedCloverAlpha > 1.0f)
			gRedCloverAlpha = 1.0f;
	}
	else
	{
		gRedCloverAlpha -= gFramesPerSecondFrac;
		if (gRedCloverAlpha < 0.0f)
			gRedCloverAlpha = 0.0f;
	}

	if (gRedCloverAlpha <= 0.0f)
		return;


			/* DRAW THE ANTHILL SPRITE */

	x = REDCLOVERS_X;
	y = REDCLOVERS_Y;

	for (i = 0; i < gTotalRedClovers; i++)
	{
		if (gGatheredRedClovers <= i)									// did we already get this one?
			gGlobalTransparency = .3f * gRedCloverAlpha;
		else
			gGlobalTransparency = 1.0f * gRedCloverAlpha;

		DrawInfobarSprite2(x, y, REDCLOVER_SCALE, SPRITE_GROUP_LEVELSPECIFIC, CLOSET_SObjType_RedClover, setupInfo);

		y += REDCLOVER_SCALE;
	}

	gGlobalTransparency = 1.0f;
}


/********************** DRAW FISH *************************/

static void Infobar_DrawFish(const OGLSetupOutputType *setupInfo)
{
float	y;
int		i;

			/* SEE IF FADE OUT */

	if (!gShowFish)
	{
		gFishAlpha -= gFramesPerSecondFrac;
		if (gFishAlpha <= 0.0f)
		{
			gFishAlpha = 0;
			return;
		}
	}

			/* DRAW THE FISH SPRITE */

	y = FISH_Y;

	gGlobalTransparency = gFishAlpha;

	for (i = 0; i < gNumCaughtFish; i++)
	{
		DrawInfobarSprite2(FISH_X, y, FISH_SCALE, SPRITE_GROUP_LEVELSPECIFIC, PARK_SObjType_FishIcon, setupInfo);
		y += FISH_SCALE * .8f;
	}

	gGlobalTransparency = 1.0f;
}


/********************** DRAW FOOD *************************/

static void Infobar_DrawFood(const OGLSetupOutputType *setupInfo)
{
float	y;
int		i;

			/* SEE IF FADE OUT */

	if (!gShowFood)
	{
		gFoodAlpha -= gFramesPerSecondFrac;
		if (gFoodAlpha <= 0.0f)
		{
			gFoodAlpha = 0;
			return;
		}
	}

			/* DRAW THE FOOD SPRITE */

	y = FOOD_Y;

	gGlobalTransparency = gFoodAlpha;

	for (i = 0; i < gNumFoodOnBasket; i++)
	{
		DrawInfobarSprite2(FOOD_X, y, FOOD_SCALE, SPRITE_GROUP_LEVELSPECIFIC, PARK_SObjType_CheeseIcon + gFoodTypes[i], setupInfo);
		y += FOOD_SCALE;
	}

	gGlobalTransparency = 1.0f;
}


