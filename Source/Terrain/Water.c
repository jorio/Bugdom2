/**********************/
/*   	WATER.C      */
/**********************/


#include "game.h"

/***************/
/* EXTERNALS   */
/***************/


/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawWater(ObjNode *theNode);
static void MakeWaterGeometry(void);

static void DrawRainEffect(ObjNode *theNode);
static void MoveRainEffect(ObjNode *theNode);

static void InitRipples(void);
static void DrawRipples(ObjNode *theNode);
static void MoveRippleEvent(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_WATER			60
#define	MAX_NUBS_IN_WATER	80

enum
{
	RAIN_MODE_OFF,
	RAIN_MODE_RAMPUP,
	RAIN_MODE_ON,
	RAIN_MODE_RAMPDOWN
};

#define	MAX_SCREEN_DROPS	100

typedef struct
{
	Boolean		isUsed;
	int			type;
	OGLPoint2D	coord;
	float		speed;
	float		size;
}ScreenDropType;


#define	MAX_GROUND_DROPS	40

typedef struct
{
	Boolean		isUsed;
	int			type;
	OGLPoint3D	coord;
	float		alpha;
	float		scale;
}GroundDropType;

#define	MAX_RIPPLES	100

typedef struct
{
	Boolean		isUsed;
	OGLPoint3D	coord;
	float		alpha, fadeRate;
	float		scale,scaleSpeed;
}RippleType;



/**********************/
/*     VARIABLES      */
/**********************/

int				gNumWaterPatches = 0;
int				gNumWaterDrawn;
WaterDefType	**gWaterListHandle = nil;
WaterDefType	*gWaterList;

static float					gWaterInitY[MAX_WATER];

static MOVertexArrayData		gWaterTriMeshData[MAX_WATER];
static MOTriangleIndecies		gWaterTriangles[MAX_WATER][MAX_NUBS_IN_WATER*2];
static OGLPoint3D				gWaterPoints[MAX_WATER][MAX_NUBS_IN_WATER*2];
static OGLTextureCoord			gWaterUVs[MAX_WATER][MAX_NUBS_IN_WATER*2];
static OGLTextureCoord			gWaterUVs2[MAX_WATER][MAX_NUBS_IN_WATER*2];
OGLBoundingBox			gWaterBBox[MAX_WATER];

static const float gWaterTransparency[NUM_WATER_TYPES] =
{
	.5,				// blue water
	.6,				// pool water
	.5,				// garbage water
};

static const Boolean gWaterGlow[NUM_WATER_TYPES] =
{
	false,				// blue water
	false,				// pool water
	true,				// garbage water
};

static const OGLTextureCoord	gWaterScrollUVDeltas[NUM_WATER_TYPES][2] =
{
	.05,.07,		.03, .06,			// blue water
	.03,.04,		.02, .03,			// pool water
	.06,.08,		.04, .06,			// garbage water
};


static const float	gWaterFixedYCoord[] =
{
	400.0f,					// #0 swimming pool

};


		/* RAIN */

static 	Byte	gRainMode;
static	float	gRainRampFactor;					// basically an alpha value
static	float	gRainScreenDropTimer, gRainGroundDropTimer;
static	int		gNumRainScreenDrops, gNumRainGroundDrops;

static	ScreenDropType	gRainScreenDrops[MAX_SCREEN_DROPS];
static	GroundDropType	gRainGroundDrops[MAX_GROUND_DROPS];

static	ObjNode		*gRainEventObj = nil;


		/* RIPPLES */

static	int			gNumRipples;
static	ObjNode		*gRippleEventObj = nil;

static	RippleType	gRippleList[MAX_RIPPLES];


/********************** DISPOSE WATER *********************/

void DisposeWater(void)
{

	if (!gWaterListHandle)
		return;

	DisposeHandle((Handle)gWaterListHandle);
	gWaterListHandle = nil;
	gWaterList = nil;
	gNumWaterPatches = 0;
}



/********************* PRIME WATER ***********************/
//
// Called during terrain prime function to initialize
//

void PrimeTerrainWater(void)
{
int						numNubs;
OGLPoint2D				*nubs;
ObjNode					*obj;
float					y,centerX,centerZ;

	InitRipples();

			/******************************/
			/* ADJUST TO GAME COORDINATES */
			/******************************/

	GAME_ASSERT(gNumWaterPatches <= MAX_WATER);

	for (int f = 0; f < gNumWaterPatches; f++)
	{
		nubs 				= &gWaterList[f].nubList[0];				// point to nub list
		numNubs 			= gWaterList[f].numNubs;					// get # nubs in water

		if (numNubs == 1)
			DoFatalAlert("PrimeTerrainWater: numNubs == 1");

		if (numNubs > MAX_NUBS_IN_WATER)
			DoFatalAlert("PrimeTerrainWater: numNubs > MAX_NUBS_IN_WATER");


				/* IF FIRST AND LAST NUBS ARE SAME, THEN ELIMINATE LAST */

		if ((nubs[0].x == nubs[numNubs-1].x) &&
			(nubs[0].y == nubs[numNubs-1].y))
		{
			numNubs--;
			gWaterList[f].numNubs = numNubs;
		}


				/* CONVERT TO WORLD COORDS */

		for (int i = 0; i < numNubs; i++)
		{
			nubs[i].x *= gMapToUnitValue;
			nubs[i].y *= gMapToUnitValue;
		}


				/***********************/
				/* CREATE VERTEX ARRAY */
				/***********************/

				/* FIND HARD-WIRED Y */

		if (gWaterList[f].flags & WATER_FLAG_FIXEDHEIGHT)
		{
			y = gWaterFixedYCoord[gWaterList[f].height];
		}
				/* FIND Y @ HOT SPOT */
		else
		{
			gWaterList[f].hotSpotX *= gMapToUnitValue;
			gWaterList[f].hotSpotZ *= gMapToUnitValue;

			y =  GetTerrainY(gWaterList[f].hotSpotX, gWaterList[f].hotSpotZ);

			switch(gLevelNum)
			{
				case	LEVEL_NUM_GARBAGE:					// water starts shallow for flooding
						y += 10.0f;
						break;

				case	LEVEL_NUM_PARK:						// deep water on Park
						y += 325.0f;
						break;

				default:
						y += 75.0f;
			}
		}

		gWaterInitY[f] = y;									// save water's y coord


		for (int i = 0; i < numNubs; i++)
		{
			gWaterPoints[f][i].x = nubs[i].x;
			gWaterPoints[f][i].y = y;
			gWaterPoints[f][i].z = nubs[i].y;
		}

			/* APPEND THE CENTER POINT TO THE POINT LIST */

		centerX = centerZ = 0;											// calc average of points
		for (int i = 0; i < numNubs; i++)
		{
			centerX += gWaterPoints[f][i].x;
			centerZ += gWaterPoints[f][i].z;
		}
		centerX /= (float)numNubs;
		centerZ /= (float)numNubs;

		gWaterPoints[f][numNubs].x = centerX;
		gWaterPoints[f][numNubs].z = centerZ;
		gWaterPoints[f][numNubs].y = y;
	}

			/***********************/
			/* MAKE WATER GEOMETRY */
			/***********************/

	MakeWaterGeometry();


		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE WATER DRAWING AT THE DESIRED TIME */
		/*************************************************************************/

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.slot 		= WATER_SLOT;
	if (gLevelNum == LEVEL_NUM_PARK)						// in park, need to do water *after* particles so that bubbles are seen
		gNewObjectDefinition.slot += 2;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED|STATUS_BIT_NOLIGHTING|STATUS_BIT_DONTCULL;

	obj = MakeNewObject(&gNewObjectDefinition);
	obj->CustomDrawFunction = DrawWater;
}


/*************** MAKE WATER GEOMETRY *********************/

static void MakeWaterGeometry(void)
{
float					minX,minY,minZ,maxX,maxY,maxZ;

	for (int f = 0; f < gNumWaterPatches; f++)
	{
				/* GET WATER INFO */

		WaterDefType* water = &gWaterList[f];				// point to this water
		uint32_t numNubs = water->numNubs;					// get # nubs in water (note:  this is the # from the file, not including the extra center point we added earlier!)
		if (numNubs < 3)
			DoFatalAlert("MakeWaterGeometry: numNubs < 3");
		int type = water->type;								// get water type


					/***************************/
					/* SET VERTEX ARRAY HEADER */
					/***************************/

		gWaterTriMeshData[f].points 					= &gWaterPoints[f][0];
		gWaterTriMeshData[f].triangles					= &gWaterTriangles[f][0];
		gWaterTriMeshData[f].uvs[0]						= &gWaterUVs[f][0];
		gWaterTriMeshData[f].uvs[1]						= &gWaterUVs2[f][0];
		gWaterTriMeshData[f].normals					= nil;
		gWaterTriMeshData[f].colorsByte					= nil;
		gWaterTriMeshData[f].colorsFloat				= nil;
		gWaterTriMeshData[f].numPoints 					= numNubs+1;					// +1 is to include the extra center point
		gWaterTriMeshData[f].numTriangles 				= numNubs;


				/* BUILD TRIANGLE INFO */

		for (int i = 0; i < gWaterTriMeshData[f].numTriangles; i++)
		{
			gWaterTriangles[f][i].vertexIndices[0] = numNubs;							// vertex 0 is always the radial center that we appended to the end of the list
			gWaterTriangles[f][i].vertexIndices[1] = i + 0;
			gWaterTriangles[f][i].vertexIndices[2] = i + 1;

			if (gWaterTriangles[f][i].vertexIndices[2] == numNubs)						// check for wrap back
				 gWaterTriangles[f][i].vertexIndices[2] = 0;
		}


				/* SET TEXTURE */

		gWaterTriMeshData[f].numMaterials	= 2;
		gWaterTriMeshData[f].materials[0] 	= 											// set illegal ref to material
		gWaterTriMeshData[f].materials[1] 	= gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_Water+type].materialObject;


				/*************/
				/* CALC BBOX */
				/*************/

		maxX = maxY = maxZ = -1000000;									// build new bboxes while we do this
		minX = minY = minZ = -maxX;

		for (uint32_t i = 0; i < numNubs; i++)
		{

					/* GET COORDS */

			float x = gWaterPoints[f][i].x;
			float y = gWaterPoints[f][i].y;
			float z = gWaterPoints[f][i].z;

					/* CHECK BBOX */

			if (x < minX)	minX = x;									// find min/max bounds for bbox
			if (x > maxX)	maxX = x;
			if (z < minZ)	minZ = z;
			if (z > maxZ)	maxZ = z;
			if (y < minY)	minY = y;
			if (y > maxY)	maxY = y;
		}

				/* SET CALCULATED BBOX */

		gWaterBBox[f].min.x = minX;
		gWaterBBox[f].max.x = maxX;
		gWaterBBox[f].min.y = minY;
		gWaterBBox[f].max.y = maxY;
		gWaterBBox[f].min.z = minZ;
		gWaterBBox[f].max.z = maxZ;
		gWaterBBox[f].isEmpty = false;


				/**************/
				/* BUILD UV's */
				/**************/

		for (int i = 0; i <= (int)numNubs; i++)
		{
			float x = gWaterPoints[f][i].x;
			float z = gWaterPoints[f][i].z;

			gWaterUVs[f][i].u 	= x * .002;
			gWaterUVs[f][i].v 	= z * .002;
			gWaterUVs2[f][i].u 	= x * .0015;
			gWaterUVs2[f][i].v 	= z * .0015;
		}
	}
}


#pragma mark -


/********************* DRAW WATER ***********************/

static void DrawWater(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	ud1, uv1, ud2, uv2;

	(void) theNode;

			/*******************/
			/* DRAW EACH WATER */
			/*******************/

	gNumWaterDrawn = 0;

	for (int f = 0; f < gNumWaterPatches; f++)
	{
		short	waterType = gWaterList[f].type;

				/* DO BBOX CULLING */

		if (OGL_IsBBoxVisible(&gWaterBBox[f], nil))
		{
			gGlobalTransparency = gWaterTransparency[waterType];

			if (gWaterGlow[waterType])								// set glow
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			else
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			MO_DrawGeometry_VertexArray(&gWaterTriMeshData[f]);
			gNumWaterDrawn++;
		}


		/********************************/
		/* ANIMATE UVS WHILE WE'RE HERE */
		/********************************/
		//
		// Unfortunately, we need to do UV animation on all water patches regardless if they are drawn or not
		// because the edges of adjacent patches must always match / be synchronized.
		//

		ud1 = gWaterScrollUVDeltas[waterType][0].u * fps;
		uv1 = gWaterScrollUVDeltas[waterType][0].v * fps;
		ud2 = gWaterScrollUVDeltas[waterType][1].u * fps;
		uv2 = gWaterScrollUVDeltas[waterType][1].v * fps;

		for (int i = 0; i <= gWaterList[f].numNubs; i++)
		{
			gWaterUVs[f][i].u 	+= ud1;
			gWaterUVs[f][i].v 	+= uv1;

			gWaterUVs2[f][i].u 	-= ud2;
			gWaterUVs2[f][i].v 	-= uv2;
		}
	}

	gGlobalTransparency = 1.0;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


/********************* RAISE WATER ***********************/

void RaiseWater(void)
{
float		y = 0;
float		fps = gFramesPerSecondFrac;

	for (int f = 0; f < gNumWaterPatches; f++)
	{
		int n = gWaterTriMeshData[f].numPoints;		// get # points in geometry

		for (int i = 0; i < n; i++)
		{
			y = gWaterTriMeshData[f].points[i].y;	// get y
			y += fps * 3.0f;

			if (y > (gWaterInitY[f] + 270.0f))		// see if over max
			{
				y = gWaterInitY[f] + 270.0f;

				if (gNumDrowningMiceRescued < gNumDrowingMiceToRescue)		// if didn't rescue all the mice then we're toast!
					SpawnKillerDragonfly();

			}

			gWaterTriMeshData[f].points[i].y = y;	// save y
		}

		gWaterBBox[f].max.y = y;					// update bbox
	}
}


/******************* RESET RISING WATER ****************************/
//
// When player fails to rescue the mice and dies, this resets water to starting level
//

void ResetRisingWater(void)
{
	for (int f = 0; f < gNumWaterPatches; f++)
	{
		float y = gWaterInitY[f];					// get init Y
		int n = gWaterTriMeshData[f].numPoints;		// get # points in geometry

		for (int i = 0; i < n; i++)
		{
			gWaterTriMeshData[f].points[i].y = y;	// reset Y
		}

		gWaterBBox[f].max.y = y;					// update bbox
	}
}



#pragma mark -

/**************** DO WATER COLLISION DETECT ********************/

Boolean DoWaterCollisionDetect(ObjNode *theNode, float x, float y, float z, int *patchNum)
{
int	i;

	for (i = 0; i < gNumWaterPatches; i++)
	{
				/* QUICK CHECK TO SEE IF IS IN BBOX */

		if ((x < gWaterBBox[i].min.x) || (x > gWaterBBox[i].max.x) ||
			(z < gWaterBBox[i].min.z) || (z > gWaterBBox[i].max.z) ||
			(y > gWaterBBox[i].max.y))
			continue;

					/* NOW CHECK IF INSIDE THE POLYGON */
					//
					// note: this really isn't necessary since the bbox should
					// 		be accurate enough
					//

//		if (!IsPointInPoly2D(x, z, gWaterList[i].numNubs, gWaterList[i].nubList))
//			continue;


					/* WE FOUND A HIT */

		theNode->StatusBits |= STATUS_BIT_UNDERWATER;
		if (patchNum)
			*patchNum = i;
		return(true);
	}

				/* NOT IN WATER */

	theNode->StatusBits &= ~STATUS_BIT_UNDERWATER;
	if (patchNum)
		*patchNum = 0;
	return(false);
}


/*********************** IS XZ OVER WATER **************************/
//
// Returns true if x/z coords are over a water bbox
//

Boolean IsXZOverWater(float x, float z)
{
int	i;

	for (i = 0; i < gNumWaterPatches; i++)
	{
				/* QUICK CHECK TO SEE IF IS IN BBOX */

		if ((x > gWaterBBox[i].min.x) && (x < gWaterBBox[i].max.x) &&
			(z > gWaterBBox[i].min.z) && (z < gWaterBBox[i].max.z))
			return(true);
	}

	return(false);
}



/**************** GET WATER Y  ********************/
//
// returns TRUE if over water.
//

Boolean GetWaterY(float x, float z, float *y)
{
int	i;

	for (i = 0; i < gNumWaterPatches; i++)
	{
				/* QUICK CHECK TO SEE IF IS IN BBOX */

		if ((x < gWaterBBox[i].min.x) || (x > gWaterBBox[i].max.x) ||
			(z < gWaterBBox[i].min.z) || (z > gWaterBBox[i].max.z))
			continue;

					/* NOW CHECK IF INSIDE THE POLYGON */

//		if (!IsPointInPoly2D(x, z, gWaterList[i].numNubs, gWaterList[i].nubList))
//			continue;


					/* WE FOUND A HIT */

		*y = gWaterBBox[i].max.y;						// return y
		return(true);
	}

				/* NOT IN WATER */

	*y = 0;
	return(false);
}

#pragma mark -
#pragma mark ======= RAIN ===========
#pragma mark -


/*************************** INIT RAIN EFFECT **********************************/

void InitRainEffect(void)
{
int		i;

	gRainEventObj = nil;

	gRainMode = RAIN_MODE_OFF;
	gRainRampFactor = 0;
	gNumRainScreenDrops = 0;
	gNumRainGroundDrops = 0;

	for (i = 0; i < MAX_SCREEN_DROPS; i++)
		gRainScreenDrops[i].isUsed = false;
}


/*************************** START RAIN EFFECT ******************************/

void StartRainEffect(void)
{
	if (gRainMode == RAIN_MODE_RAMPDOWN)		// see if stop ramping down
	{
		gRainMode = RAIN_MODE_RAMPUP;
		return;
	}


	if (gRainMode != RAIN_MODE_OFF)				// see if already going in some way
		return;


	gRainMode = RAIN_MODE_RAMPUP;

	gRainRampFactor = 0;
	gRainScreenDropTimer = 0;
	gRainGroundDropTimer = 0;


		/* MAKE EVENT */

	if (gRainEventObj)
		DoFatalAlert("StartRainEffect: gRainEventObj != nil");

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED | STATUS_BIT_NOZWRITES | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+40;
	gNewObjectDefinition.moveCall 	= MoveRainEffect;
	gRainEventObj = MakeNewObject(&gNewObjectDefinition);

	gRainEventObj->CustomDrawFunction = DrawRainEffect;



}


/********************* STOP RAIN EFFECT **************************/

void StopRainEffect(void)
{
	gRainMode = RAIN_MODE_RAMPDOWN;
}


/********************* MOVE RAIN EFFECT ****************************/

static void MoveRainEffect(ObjNode *theNode)
{
int		i,j;
float	fps = gFramesPerSecondFrac;
Boolean	makeNewDrops = false;

	switch(gRainMode)
	{
		case	RAIN_MODE_OFF:
				return;

		case	RAIN_MODE_ON:
				makeNewDrops = true;
				break;

			/* RAMP UP */

		case	RAIN_MODE_RAMPUP:
				gRainRampFactor += fps;
				if (gRainRampFactor >= 1.0f)
				{
					gRainRampFactor = 1.0f;
					gRainMode = RAIN_MODE_ON;
				}

				makeNewDrops = true;
				break;


			/* RAMP DOWN */

		case	RAIN_MODE_RAMPDOWN:
				gRainRampFactor -= fps;
				if (gRainRampFactor <= 0.0f)
				{
					gRainRampFactor = 0.0f;
					if (gNumRainScreenDrops <= 0)						// once all drops gone, turn off
					{
						gRainMode = RAIN_MODE_OFF;
						DeleteObject(theNode);
						gRainEventObj = nil;
						return;
					}
				}
				else
					makeNewDrops = true;
				break;

	}



	if (makeNewDrops)
	{
			/***********************************/
			/* GENERATE MORE RAIN SCREEN DROPS */
			/***********************************/

		gRainScreenDropTimer -= fps;
		if (gRainScreenDropTimer <= 0.0f)
		{
			gRainScreenDropTimer += .04f;							// reset timer

			for (j = 0; j < 2; j++)									// n drops per pass
			{
				for (i = 0; i < MAX_SCREEN_DROPS; i++)					// look for a free slot
					if (!gRainScreenDrops[i].isUsed)
						break;

				if (i < MAX_SCREEN_DROPS)								// see if got one
				{
					gRainScreenDrops[i].isUsed = true;

					gRainScreenDrops[i].type = 0;

					gRainScreenDrops[i].coord.x = -50.0f + RandomFloat() * 700.0f;
					gRainScreenDrops[i].coord.y = -30;

					gRainScreenDrops[i].size = 7.0f + RandomFloat() * 7.0f;
					gRainScreenDrops[i].speed = 1500.0f + RandomFloat() * 400.0f;

					gNumRainScreenDrops++;
				}
			}
		}

			/******************************/
			/* GENERATE MORE GROUND DROPS */
			/******************************/

		gRainGroundDropTimer -= fps;
		if (gRainGroundDropTimer <= 0.0f)
		{
			float			centerX, centerZ;
			OGLVector2D		v;

			gRainGroundDropTimer += .03f;							// reset timer

				/* CALC CENTER POINT WHERE CAN ADD DROPS */

			centerX = gGameView->cameraPlacement.pointOfInterest.x - gGameView->cameraPlacement.cameraLocation.x;
			centerZ = gGameView->cameraPlacement.pointOfInterest.z - gGameView->cameraPlacement.cameraLocation.z;
			FastNormalizeVector2D(centerX, centerZ, &v, false);
			centerX = gGameView->cameraPlacement.cameraLocation.x + v.x * 1000.0f;
			centerZ = gGameView->cameraPlacement.cameraLocation.z + v.y * 1000.0f;


			for (j = 0; j < 4; j++)									// n drops per pass
			{
				for (i = 0; i < MAX_GROUND_DROPS; i++)					// look for a free slot
					if (!gRainGroundDrops[i].isUsed)
						break;

				if (i < MAX_GROUND_DROPS)								// see if got one
				{
					gRainGroundDrops[i].isUsed = true;

					gRainGroundDrops[i].type = 0;

					gRainGroundDrops[i].coord.x = centerX + RandomFloat2() * 900.0f;
					gRainGroundDrops[i].coord.z = centerZ + RandomFloat2() * 900.0f;
					gRainGroundDrops[i].coord.y = GetTerrainY(gRainGroundDrops[i].coord.x, gRainGroundDrops[i].coord.z);

					gRainGroundDrops[i].alpha = .99;
					gRainGroundDrops[i].scale = .3f;

					gNumRainGroundDrops++;
				}
			}
		}

	}

			/*********************/
			/* MOVE SCREEN DROPS */
			/*********************/

	for (i = 0; i < MAX_SCREEN_DROPS; i++)
	{
		gRainScreenDrops[i].coord.y += gRainScreenDrops[i].speed * fps;

		if (gRainScreenDrops[i].coord.y > 480.0f)					// see if gone
		{
			gRainScreenDrops[i].isUsed = false;
			gNumRainScreenDrops--;
		}
	}

			/*********************/
			/* MOVE GROUND DROPS */
			/*********************/

	for (i = 0; i < MAX_GROUND_DROPS; i++)
	{
		gRainGroundDrops[i].scale += fps * 10.0f;
		gRainGroundDrops[i].alpha -= fps * 6.0f;

		if (gRainGroundDrops[i].alpha <= 0.0f)					// see if gone
		{
			gRainGroundDrops[i].isUsed = false;
			gNumRainGroundDrops--;
		}
	}

}


/************************* DRAW RAIN EFFECT ***************************/

static void DrawRainEffect(ObjNode *theNode)
{
float		x,y,size;
OGLMatrix4x4	m;

	(void) theNode;

	if (gRainMode == RAIN_MODE_OFF)
		return;

	OGL_PushState();

			/*********************/
			/* DRAW GROUND DROPS */
			/*********************/

	for (int i = 0; i < MAX_GROUND_DROPS; i++)
	{
		if (!gRainGroundDrops[i].isUsed)
			continue;

		size = gRainGroundDrops[i].scale;
		gGlobalTransparency = gRainGroundDrops[i].alpha;

		glPushMatrix();
		OGLMatrix4x4_SetTranslate(&m, gRainGroundDrops[i].coord.x, gRainGroundDrops[i].coord.y, gRainGroundDrops[i].coord.z);
		glMultMatrixf((GLfloat *)&m);
		OGLMatrix4x4_SetScale(&m, size, size, size);
		glMultMatrixf((GLfloat *)&m);

		MO_DrawObject(gBG3DGroupList[MODEL_GROUP_GLOBAL][GLOBAL_ObjType_WaterSpat]);

		glPopMatrix();

	}



			/*********************/
			/* DRAW SCREEN DROPS */
			/*********************/

	SetInfobarSpriteState();

	gGlobalTransparency = gRainRampFactor * .99f;

	for (int i = 0; i < MAX_SCREEN_DROPS; i++)
	{
		if (!gRainScreenDrops[i].isUsed)
			continue;

		size = gRainScreenDrops[i].size;
		x = gRainScreenDrops[i].coord.x;
		y = gRainScreenDrops[i].coord.y;

		DrawInfobarSprite2(x, y, size, SPRITE_GROUP_GLOBAL, GLOBAL_SObjType_RainDrop);
	}

	gGlobalTransparency = 1.0f;
	gGlobalMaterialFlags = 0;
	OGL_PopState();
}

#pragma mark -
#pragma mark ======= RIPPLE ===========
#pragma mark -


/************************** INIT RIPPLES ****************************/

static void InitRipples(void)
{
	gNumRipples = 0;
	gRippleEventObj = nil;

	for (int i = 0; i < MAX_RIPPLES; i++)
		gRippleList[i].isUsed = false;
}


/********************** CREATE NEW RIPPLE ************************/

void CreateNewRipple(float x, float z, float baseScale, float scaleSpeed, float fadeRate)
{
float	y2,y;
int		i;

			/* GET Y COORD FOR WATER */

	if (!GetWaterY(x, z, &y2))
		return;													// bail if not actually on water

	y = y2+.5f;													// raise ripple off water

		/* CREATE RIPPLE EVENT OBJECT */

	if (gRippleEventObj == nil)
	{
		gNewObjectDefinition.genre		= EVENT_GENRE;
		gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED | STATUS_BIT_NOZWRITES | STATUS_BIT_NOLIGHTING | STATUS_BIT_GLOW | STATUS_BIT_NOFOG;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB+1;
		gNewObjectDefinition.moveCall 	= MoveRippleEvent;
		gRippleEventObj = MakeNewObject(&gNewObjectDefinition);

		gRippleEventObj->CustomDrawFunction = DrawRipples;
	}

		/**********************/
		/* ADD TO RIPPLE LIST */
		/**********************/

		/* SCAN FOR FREE RIPPLE SLOT */

	for (i = 0; i < MAX_RIPPLES; i++)
	{
		if (!gRippleList[i].isUsed)
			goto got_it;
	}
	return;												// no free slots

got_it:

	gRippleList[i].isUsed = true;
	gRippleList[i].coord.x = x;
	gRippleList[i].coord.y = y;
	gRippleList[i].coord.z = z;

	gRippleList[i].scale = baseScale + RandomFloat() * 30.0f;
	gRippleList[i].scaleSpeed = scaleSpeed;
	gRippleList[i].alpha = .999f - (RandomFloat() * .2f);
	gRippleList[i].fadeRate = fadeRate;

	gNumRipples++;
}


/******************** MOVE RIPPLE EVENT ****************************/

static void MoveRippleEvent(ObjNode *theNode)
{
int		i;
float	fps = gFramesPerSecondFrac;

	for (i = 0; i < MAX_RIPPLES; i++)
	{
		if (!gRippleList[i].isUsed)									// see if this ripple slot active
			continue;

		gRippleList[i].scale += fps * gRippleList[i].scaleSpeed;
		gRippleList[i].alpha -= fps * gRippleList[i].fadeRate;
		if (gRippleList[i].alpha <= 0.0f)							// see if done
		{
			gRippleList[i].isUsed = false;							// kill this slot
			gNumRipples--;
		}
	}

	if (gNumRipples <= 0)											// see if all done
	{
		DeleteObject(theNode);
		gRippleEventObj = nil;
	}

}


/******************** DRAW RIPPLES ***************************/

static void DrawRipples(ObjNode *theNode)
{
int			i;
float		s,x,y,z;

	(void) theNode;

		/* ACTIVATE MATERIAL */

	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_WaterRipple].materialObject);


		/* DRAW EACH RIPPLE */

	for (i = 0; i < MAX_RIPPLES; i++)
	{
		if (!gRippleList[i].isUsed)									// see if this ripple slot active
			continue;

		x = gRippleList[i].coord.x;									// get coord
		y = gRippleList[i].coord.y;
		z = gRippleList[i].coord.z;

		s = gRippleList[i].scale;									// get scale
		glColor4f(1,1,1,gRippleList[i].alpha);						// get/set alpha

		glBegin(GL_QUADS);
		glTexCoord2f(0,0);	glVertex3f(x - s, y, z - s);
		glTexCoord2f(1,0);	glVertex3f(x + s, y, z - s);
		glTexCoord2f(1,1);	glVertex3f(x + s, y, z + s);
		glTexCoord2f(0,1);	glVertex3f(x - s, y, z + s);
		glEnd();
	}

	glColor4f(1,1,1,1);
	gGlobalTransparency = 1.0f;
}








