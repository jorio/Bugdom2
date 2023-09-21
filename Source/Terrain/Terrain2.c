/****************************/
/*   	TERRAIN2.C 	        */
/****************************/


#include "game.h"

/***************/
/* EXTERNALS   */
/***************/

extern	long	gTerrainTileWidth,gTerrainTileDepth;
extern	long	gNumSuperTilesDeep,gNumSuperTilesWide;
extern	long	gTerrainUnitWidth,gTerrainUnitDepth,gNumUniqueSuperTiles;
extern	OGLPoint3D	gCoord;
extern	PlayerInfoType	gPlayerInfo;
extern	OGLVector3D				gRecentTerrainNormal;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	SuperTileStatus			**gSuperTileStatusGrid;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float					gFramesPerSecondFrac,gTerrainSuperTileUnitSize,gMapToUnitValue;
extern	SuperTileMemoryType	gSuperTileMemoryList[];
extern	float					gTerrainSuperTileUnitSizeFrac,gTerrainPolygonSize;
extern	float			**gVertexShading;
extern	int				gLevelNum;


/****************************/
/*    PROTOTYPES            */
/****************************/

static Boolean NilAdd(TerrainItemEntryType *itemPtr,float x, float z);


/****************************/
/*    CONSTANTS             */
/****************************/



/**********************/
/*     VARIABLES      */
/**********************/

short	  				gNumTerrainItems;
TerrainItemEntryType 	**gMasterItemList = nil;

float					**gMapYCoords = nil;			// 2D array of map vertex y coords
float					**gMapYCoordsOriginal = nil;	// copy of gMapYCoords data as it was when file was loaded
Byte					**gMapSplitMode = nil;

SuperTileItemIndexType	**gSuperTileItemIndexGrid = nil;

int						gNumLineMarkers;
LineMarkerDefType		gLineMarkerList[MAX_LINEMARKERS];


/**********************/
/*     TABLES         */
/**********************/

#define	MAX_ITEM_NUM	85					// for error checking!

static Boolean (*gTerrainItemAddRoutines[MAX_ITEM_NUM+1])(TerrainItemEntryType *itemPtr, float x, float z) =
{
		NilAdd,								// My Start Coords
		AddSnail,							// snail
		AddSprinklerHead,					// sprinkler head
		AddButterfly,						// butterfly pow
		AddEnemy_Gnome,						// gnome
		AddDaisy,							// daisy
		AddGrass,							// grass
		AddSnailShell,						// snail shell
		AddTulip,							// tulip
		AddAcorn,							// acorn
		AddEnemy_HouseFly,					// 10: housefly
		AddScarecrow,						// 11: scarecrow
		AddEnemy_EvilPlant,					// 12: evil plant
		AddDoor,						// 13: lawn door
		AddRideBall,						// 14: ride ball
		AddBowlingMarble,					// 15: bowling marble
		AddBowlingPins,						// 16: bowling pins
		AddBrick,							// 17: brick
		AddPost,							// 18: post
		AddChipmunk,						// 19: chipmunk
		AddShrubRoot,						// 20: shrub root
		AddPebble,							// 21: pebble
		AddSnakeGenerator,					// 22: snake generator
		AddPoolCoping,						// 23: pool coping
		AddPoolLeaf,						// 24: pool leaf
		NilAdd,								// 25: ??????
		AddSquishBerry,						// 26: squish berry
		AddDogHouse,						// 27: dog house
		AddWindmill,						// 28: windmill
		AddRose,							// 29: rose
		AddTulipPot,						// 30: tulip pot
		AddBeachBall,						// 31: beach ball
		AddChlorineFloat,					// 32: chlorine float
		AddPoolRingFloat,					// 33: pool ring float
		AddDrainPipe,						// 34: drain pipe
		AddPOW,								// 35: powerup
		AddFirecracker,						// 36: firecracker
		AddGlassBottle,						// 37: glass bottle
		AddEnemy_Flea,						// 38: flea enemy
		AddEnemy_Tick,						// 39: tick enemy
		NilAdd,								// 40: slot car
		AddLetterBlock,						// 41: letter block
		AddMouseTrap,						// 42: mouse trap
		AddEnemy_ToySoldier,				// 43: toy solider
		AddFinishLine,						// 44: finish line
		AddEnemy_Otto,						// 45: otto enemy
		AddPuzzle,							// 46: puzzle
		AddLegoWall,						// 47: lego wall
		AddFlashLight,						// 48: flashlight
		AddDCell,							// 49: d-cell
		AddCrayon,							// 50: crayon
		AddAntHill,							// 51: ant hill
		AddEnemy_Dragonfly,					// 52: dragonfly
		AddCloud,							// 53: cloud
		AddEnemy_Frog,						// 54: frog
		AddCardboardBox,								// 55: box
		AddTrampoline,						// 56: trampoline
		AddMothBall,						// 57: moth ball
		NilAdd,								// 58: vacuume
		AddClosetWall,						// 59: pci card
		AddEnemy_Moth,						// 60: moth
		AddEnemy_ComputerBug,				// 61: computer bug
		AddSiliconPart,						// 62: silicon part
		NilAdd,
		AddBookStack,						// 64: book stack
		AddEnemy_Roach,						// 65: roach enemy
		AddShoeBox,							// 66: shoe box
		AddPictureFrame,					// 67: picture frame
		AddEnemy_Ant,						// 68: ant enemy
		AddEnemy_PondFish,					// 69: fish enemy
		AddLilyPad,							// 70: lily pad
		AddCatTail,							// 71: cat tail
		AddBubbler,							// 72: bubbler
		AddPlatformFlower,					// 73: platform flower
		AddFishingLure,						// 74: fishing lure
		AddSilverware,						// 75: silvereware
		AddPicnicBasket,					// 76: picnic basket
		AddKindling,						// 77: kindling
		AddBeeHive,							// 78: bee hive
		AddSodaCan,							// 79: soda can
		AddVeggie,							// 80: veggies
		AddJar,								// 81: jar
		AddTinCan,							// 82: tin can
		AddDetergent,						// 83: detergent
		AddBoxWall,							// 84: box wall
		AddGliderPart,						// 85: glider part
};



/********************* BUILD TERRAIN ITEM LIST ***********************/
//
// This takes the input item list and resorts it according to supertile grid number
// such that the items on any supertile are all sequential in the list instead of scattered.
//

void BuildTerrainItemList(void)
{
TerrainItemEntryType	**tempItemList;
TerrainItemEntryType	*srcList,*newList;
int						row,col,i;
int						itemX, itemZ;
int						total;

			/* ALLOC MEMORY FOR SUPERTILE ITEM INDEX GRID */

	Alloc_2d_array(SuperTileItemIndexType, gSuperTileItemIndexGrid, gNumSuperTilesDeep, gNumSuperTilesWide);

	if (gNumTerrainItems == 0)
		DoFatalAlert("\pBuildTerrainItemList: there must be at least 1 terrain item!");


			/* ALLOC MEMORY FOR NEW LIST */

	tempItemList = (TerrainItemEntryType **)AllocHandle(sizeof(TerrainItemEntryType) * gNumTerrainItems);
	if (tempItemList == nil)
		DoFatalAlert("\pBuildTerrainItemList: AllocPtr failed!");

	HLock((Handle)gMasterItemList);
	HLock((Handle)tempItemList);

	srcList = *gMasterItemList;
	newList = *tempItemList;


			/************************/
			/* SCAN ALL SUPERTILES  */
			/************************/

	total = 0;

	for (row = 0; row < gNumSuperTilesDeep; row++)
	{
		for (col = 0; col < gNumSuperTilesWide; col++)
		{
			gSuperTileItemIndexGrid[row][col].numItems = 0;			// no items on this supertile yet


			/* FIND ALL ITEMS ON THIS SUPERTILE */

			for (i = 0; i < gNumTerrainItems; i++)
			{
				itemX = srcList[i].x;								// get pixel coords of item
				itemZ = srcList[i].y;

				itemX /= gTerrainSuperTileUnitSize;				// convert to supertile row
				itemZ /= gTerrainSuperTileUnitSize;				// convert to supertile column

				if ((itemX == col) && (itemZ == row))				// see if its on this supertile
				{
					if (gSuperTileItemIndexGrid[row][col].numItems == 0)		// see if this is the 1st item
						gSuperTileItemIndexGrid[row][col].itemIndex = total;	// set starting index

					newList[total] = srcList[i];					// copy into new list
					total++;										// inc counter
					gSuperTileItemIndexGrid[row][col].numItems++;	// inc # items on this supertile

				}
				else
				if (itemX > col)									// since original list is sorted, we can know when we are past the usable edge
					break;
			}
		}
	}


		/* NUKE THE ORIGINAL ITEM LIST AND REASSIGN TO THE NEW SORTED LIST */

	DisposeHandle((Handle)gMasterItemList);						// nuke old list
	gMasterItemList = tempItemList;								// reassign



			/* FIGURE OUT WHERE THE STARTING POINT IS */

	FindPlayerStartCoordItems();										// look thru items for my start coords
}



/******************** FIND PLAYER START COORD ITEM *******************/
//
// Scans thru item list for item type #14 which is a teleport reciever / start coord,
//

void FindPlayerStartCoordItems(void)
{
long					i;
TerrainItemEntryType	*itemPtr;
Boolean                 flags = false;


	itemPtr = *gMasterItemList; 												// get pointer to data inside the LOCKED handle

				/* SCAN FOR "START COORD" ITEM */

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_MYSTARTCOORD)						// see if it's a MyStartCoord item
		{

					/* CHECK FOR BIT INFO */


			gPlayerInfo.coord.x = gPlayerInfo.startX = itemPtr[i].x;
			gPlayerInfo.coord.z = gPlayerInfo.startZ = itemPtr[i].y;
			gPlayerInfo.startRotY = (float)itemPtr[i].parm[0] * (PI2/8.0f);	// calc starting rotation aim

			break;

//			if (flags)                      								// if we already got a coord for this player then err
  //              DoFatalAlert("\pFindPlayerStartCoordItems:  duplicate start item for player #n");
	        flags = true;
		}
	}
}


/****************** ADD TERRAIN ITEMS ON SUPERTILE *******************/
//
// Called by DoPlayerTerrainUpdate() per each supertile needed.
// This scans all of the items on this supertile and attempts to add them.
//

void AddTerrainItemsOnSuperTile(long row, long col)
{
TerrainItemEntryType *itemPtr;
long			type,numItems,startIndex,i;
Boolean			flag;


	numItems = gSuperTileItemIndexGrid[row][col].numItems;		// see how many items are on this supertile
	if (numItems == 0)
		return;

	startIndex = gSuperTileItemIndexGrid[row][col].itemIndex;	// get starting index into item list
	itemPtr = &(*gMasterItemList)[startIndex];					// get pointer to 1st item on this supertile


			/*****************************/
			/* SCAN ALL ITEMS UNDER HERE */
			/*****************************/

	for (i = 0; i < numItems; i++)
	{
		float	x,z;

		if (itemPtr[i].flags & ITEM_FLAGS_INUSE)				// see if item available
			continue;

		x = itemPtr[i].x;										// get item coords
		z = itemPtr[i].y;

		if (SeeIfCoordsOutOfRange(x,z))						// only add if this supertile is active by player
			continue;

		type = itemPtr[i].type;									// get item #
		if (type > MAX_ITEM_NUM)								// error check!
		{
			DoAlert("\pIllegal Map Item Type!");
			ShowSystemErr(type);
		}

		flag = gTerrainItemAddRoutines[type](&itemPtr[i],itemPtr[i].x, itemPtr[i].y); // call item's ADD routine
		if (flag)
			itemPtr[i].flags |= ITEM_FLAGS_INUSE;				// set in-use flag
	}
}



/******************** NIL ADD ***********************/
//
// nothing add
//

static Boolean NilAdd(TerrainItemEntryType *itemPtr,float x, float z)
{
#pragma unused (itemPtr, x, z)
	return(false);
}


/***************** TRACK TERRAIN ITEM ******************/
//
// Returns true if theNode is out of range
//

Boolean TrackTerrainItem(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_DONTPURGE)			// see if non-purgable
		return(false);

	return(SeeIfCoordsOutOfRange(theNode->Coord.x,theNode->Coord.z));		// see if out of range of all players
}


/***************** TRACK TERRAIN ITEM: FROM INIT ******************/
//
// Returns true if theNode is out of range
//

Boolean TrackTerrainItem_FromInit(ObjNode *theNode)
{
	return(SeeIfCoordsOutOfRange(theNode->InitCoord.x,theNode->InitCoord.z));
}


/********************* SEE IF COORDS OUT OF RANGE RANGE *********************/
//
// Returns true if the given x/z coords are outside the item delete
// window of any of the players.
//

Boolean SeeIfCoordsOutOfRange(float x, float z)
{
int			row,col;

			/* SEE IF OUT OF RANGE */

	if ((x < 0) || (z < 0))
		return(true);
	if ((x >= gTerrainUnitWidth) || (z >= gTerrainUnitDepth))
		return(true);


		/* SEE IF A PLAYER USES THIS SUPERTILE */

	col = x * gTerrainSuperTileUnitSizeFrac;						// calc supertile relative row/col that the coord lies on
	row = z * gTerrainSuperTileUnitSizeFrac;

	if (gSuperTileStatusGrid[row][col].playerHereFlag)				// if a player is using this supertile, then coords are in range
		return(false);
	else
		return(true);												// otherwise, out of range since no players can see this supertile
}


/*************************** ROTATE ON TERRAIN ***************************/
//
// Rotates an object's x & z such that it's lying on the terrain.
//
// INPUT: surfaceNormal == optional input normal to use.
//

void RotateOnTerrain(ObjNode *theNode, float yOffset, OGLVector3D *surfaceNormal)
{
OGLVector3D		up;
float			r,x,z,y;
OGLPoint3D		to;
OGLMatrix4x4	*m,m2;

			/* GET CENTER Y COORD & TERRAIN NORMAL */

	x = theNode->Coord.x;
	z = theNode->Coord.z;
	y = theNode->Coord.y = GetTerrainY(x, z) + yOffset;

	if (surfaceNormal)
		up = *surfaceNormal;
	else
		up = gRecentTerrainNormal;


			/* CALC "TO" COORD */

	r = theNode->Rot.y;
	to.x = x + sin(r) * -30.0f;
	to.z = z + cos(r) * -30.0f;
	to.y = GetTerrainY(to.x, to.z) + yOffset;


			/* CREATE THE MATRIX */

	m = &theNode->BaseTransformMatrix;
	SetLookAtMatrix(m, &up, &theNode->Coord, &to);


		/* POP IN THE TRANSLATE INTO THE MATRIX */

	m->value[M03] = x;
	m->value[M13] = y;
	m->value[M23] = z;


			/* SET SCALE */

	OGLMatrix4x4_SetScale(&m2, theNode->Scale.x,				// make scale matrix
							 	theNode->Scale.y,
							 	theNode->Scale.z);
	OGLMatrix4x4_Multiply(&m2, m, m);
}



/*************************** ROTATE ON TERRAIN: WIDE AREA ***************************/
//
// Same as above except it averages normals around the center.
//

void RotateOnTerrain_WideArea(ObjNode *theNode, float yOffset, float radius)
{
OGLVector3D		up;
float			r,x,z,x2,z2;


			/* GET CENTER Y COORD & TERRAIN NORMAL */

	x = theNode->Coord.x;
	z = theNode->Coord.z;
	GetTerrainY(x, z);
	up = gRecentTerrainNormal;

			/* AVERAGE IN THE RADIAL NORMALS */

	for (r = 0; r < PI2; r += (PI/8))
	{
		x2 = x + sin(r) * radius;
		z2 = z + cos(r) * radius;

		GetTerrainY(x, z);
		up.x += gRecentTerrainNormal.x;
		up.y += gRecentTerrainNormal.y;
		up.z += gRecentTerrainNormal.z;
	}
	OGLVector3D_Normalize(&up, &up);


	RotateOnTerrain(theNode, yOffset, &up);
}


#pragma mark ======= TERRAIN PRE-CONSTRUCTION =========




/********************** CALC TILE NORMALS *****************************/
//
// Given a row, col coord, calculate the face normals for the 2 triangles.
//

void CalcTileNormals(long row, long col, OGLVector3D *n1, OGLVector3D *n2)
{
static OGLPoint3D	p1 = {0,0,0};
static OGLPoint3D	p2 = {0,0,0};
static OGLPoint3D	p3 = {0,0,0};
static OGLPoint3D	p4 = {0,0,0};
//static OGLPoint3D	p2 = {0,0,gTerrainPolygonSize};
//static OGLPoint3D	p3 = {gTerrainPolygonSize,0,gTerrainPolygonSize};
//static OGLPoint3D	p4 = {gTerrainPolygonSize, 0, 0};


	p2.z =
	p3.x =
	p3.z =
	p4.x = gTerrainPolygonSize;

		/* MAKE SURE ROW/COL IS IN RANGE */

	if ((row >= gTerrainTileDepth) || (row < 0) ||
		(col >= gTerrainTileWidth) || (col < 0))
	{
		n1->x = n2->x = 0;						// pass back up vector by default since our of range
		n1->y = n2->y = 1;
		n1->z = n2->z = 0;
		return;
	}

	p1.y = gMapYCoords[row][col];		// far left
	p2.y = gMapYCoords[row+1][col];		// near left
	p3.y = gMapYCoords[row+1][col+1];	// near right
	p4.y = gMapYCoords[row][col+1];		// far right


		/* CALC NORMALS BASED ON SPLIT */

	if (gMapSplitMode[row][col] == SPLIT_BACKWARD)
	{
		CalcFaceNormal(&p2, &p3, &p1, n1);		// fl, nl, nr
		CalcFaceNormal(&p3, &p4, &p1, n2);		// fl, nr, fr
	}
	else
	{
		CalcFaceNormal(&p4, &p1, &p2, n1);		// fl, nl, fr
		CalcFaceNormal(&p3, &p4, &p2, n2);		// fr, nl, nr
	}
}


/********************** CALC TILE NORMALS: NOT NORMALIZED *****************************/
//
// Given a row, col coord, calculate the face normals for the 2 triangles.
//

void CalcTileNormals_NotNormalized(long row, long col, OGLVector3D *n1, OGLVector3D *n2)
{
static OGLPoint3D	p1 = {0,0,0};
static OGLPoint3D	p2 = {0,0,0};
static OGLPoint3D	p3 = {0,0,0};
static OGLPoint3D	p4 = {0,0,0};
//static OGLPoint3D	p2 = {0,0,gTerrainPolygonSize};
//static OGLPoint3D	p3 = {gTerrainPolygonSize,0,gTerrainPolygonSize};
//static OGLPoint3D	p4 = {gTerrainPolygonSize, 0, 0};

	p2.z =
	p3.x =
	p3.z =
	p4.x = gTerrainPolygonSize;


		/* MAKE SURE ROW/COL IS IN RANGE */

	if ((row >= gTerrainTileDepth) || (row < 0) ||
		(col >= gTerrainTileWidth) || (col < 0))
	{
		n1->x = n2->x = 0;						// pass back up vector by default since our of range
		n1->y = n2->y = 1;
		n1->z = n2->z = 0;
		return;
	}

	p1.y = gMapYCoords[row][col];		// far left
	p2.y = gMapYCoords[row+1][col];		// near left
	p3.y = gMapYCoords[row+1][col+1];	// near right
	p4.y = gMapYCoords[row][col+1];		// far right


		/* CALC NORMALS BASED ON SPLIT */

	if (gMapSplitMode[row][col] == SPLIT_BACKWARD)
	{
		CalcFaceNormal_NotNormalized(&p2, &p3, &p1, n1);		// fl, nl, nr
		CalcFaceNormal_NotNormalized(&p3, &p4, &p1, n2);		// fl, nr, fr
	}
	else
	{
		CalcFaceNormal_NotNormalized(&p4, &p1, &p2, n1);		// fl, nl, fr
		CalcFaceNormal_NotNormalized(&p3, &p4, &p2, n2);		// fr, nl, nr
	}
}


#pragma mark -

/****************** DO ITEM SHADOW CASTING **********************/
//
// Scans thru item list and casts a shadown onto the terrain
// by darkening the vertex colors of the terrain.
//

void DoItemShadowCasting(OGLSetupOutputType *setupInfo)
{
long				i;
static OGLVector3D up = {0,1,0};
float				height,dot,length;
OGLVector2D			lightVector;
OGLPoint2D			from,to;
float				x,z,t;
long				row,col;
Byte				**shadowFlags;
float				shadeFactor;

	if (gLevelNum == LEVEL_NUM_BALSA)
		shadeFactor = .9;
	else
		shadeFactor = .7;

				/* INIT SHADING GRID */

	Alloc_2d_array(float, gVertexShading, gTerrainTileDepth+1, gTerrainTileWidth+1);	// alloc 2D array for map
	for (row = 0; row <= gTerrainTileDepth; row++)
		for (col = 0; col <= gTerrainTileWidth; col++)
			gVertexShading[row][col] = 1.0;


			/* INIT SHADOW FLAGS TEMP BUFFER */

	Alloc_2d_array(Byte, shadowFlags, gTerrainTileDepth+1, gTerrainTileWidth+1);

	for (row = 0; row <= gTerrainTileDepth; row++)
		for (col = 0; col <= gTerrainTileWidth; col++)
			shadowFlags[row][col] = 0;


			/* GET MAIN LIGHT VECTOR INFO */

	lightVector.x = setupInfo->lightList.fillDirection[0].x;
	lightVector.y = setupInfo->lightList.fillDirection[0].z;
	OGLVector2D_Normalize(&lightVector, &lightVector);

	dot = OGLVector3D_Dot(&up,&setupInfo->lightList.fillDirection[0]);
	dot = 1.0f - dot;

			/***********************/
			/* SCAN THRU ITEM LIST */
			/***********************/

	for (i = 0; i < gNumTerrainItems; i++)
	{
			/* SEE WHICH THINGS WE SUPPORT & GET PARMS */

		switch((*gMasterItemList)[i].type)
		{
			case	2:						// sprinkler head
					height = 50;
					break;

			case	5:						// daisy
					height = 200;
					break;

			case	6:						// grass
					height = 200;
					break;

			case	8:						// tulip
					height = 200;
					break;

			case	18:						// post
					height = 300;
					break;

			case	19:						// chipmunk
					height = 200;
					break;

			case	20:						// shrub root
					height = 700;
					break;

			case	21:						// pebble
					height = 20;
					break;

			case	29:						// rose
					height = 300;
					break;

			case	48:						// flashlight
					height = 350;
					break;

			case	64:						// tall book stack
					if ((*gMasterItemList)[i].parm[0] == 2)	// only shadow tall stack
						height = 500;
					else
						continue;
					break;

			default:
					continue;
		}

			/* CALCULATE LINE TO DRAW SHADOW ALONG */

		from.x = (*gMasterItemList)[i].x;
		from.y = (*gMasterItemList)[i].y;

		to.x = from.x + lightVector.x * (height * dot);
		to.y = from.y + lightVector.y * (height * dot);

		length = OGLPoint2D_Distance(&from, &to);


			/***************************************/
			/* SCAN ALONG LIGHT AND SHADE VERTICES */
			/***************************************/

		for (t = 1.0; t > 0.0f; t -= 1.0f / (length/gTerrainPolygonSize))
		{
			float	oneMinusT = 1.0f - t;
			float	ro,co;

			x = (from.x * oneMinusT) + (to.x * t);			// calc center x
			z = (from.y * oneMinusT) + (to.y * t);

			for (ro = -.5; ro <= .5; ro += .5)
			{
				for (co = -.5; co <= .5; co += .5)
				{
					row = z / gTerrainPolygonSize + ro;			// calc row/col
					col = x / gTerrainPolygonSize + co;

					if ((row < 0) || (col < 0))						// check for out of bounds
						continue;
					if ((row >= gTerrainTileDepth) || (col >= gTerrainTileWidth))
						continue;

					if (shadowFlags[row][col])						// see if this already shadowed
						continue;

					shadowFlags[row][col] = 1;						// set flag


					gVertexShading[row][col] = shadeFactor;			// set shading

				}// co
			} // ro
		}
	}


			/* CLEANUP */

	Free_2d_array(shadowFlags);
}


#pragma mark -
#pragma mark ====== LINE MARKERS =========
#pragma mark -


/*********************** SEE IF CROSSED LINE MARKER ************************/

Boolean SeeIfCrossedLineMarker(ObjNode *theNode, int *whichLine)
{
float	fromX, fromZ, toX, toZ;
short	c;
float	intersectX, intersectZ;


			/* GET PLAYER'S MOVEMENT LINE SEGMENT */

	fromX 	= theNode->OldCoord.x;
	fromZ 	= theNode->OldCoord.z;
	toX 	= gCoord.x;
	toZ 	= gCoord.z;


			/**********************************/
			/* SEE IF CROSSED ANY LINE MARKERS */
			/**********************************/

	for (c = 0; c < gNumLineMarkers; c++)
	{
		float	x1,z1,x2,z2;

		x1 = gLineMarkerList[c].x[0];													// get endpoints of line marker
		z1 = gLineMarkerList[c].z[0];
        x2 = gLineMarkerList[c].x[1];
        z2 = gLineMarkerList[c].z[1];

		if (IntersectLineSegments(fromX, fromZ, toX, toZ,x1,z1,x2,z2,&intersectX, &intersectZ))
    	{
    		*whichLine = c;
    		return(true);
		}
	}


			/* NOTHING */

	*whichLine = -1;
	return(false);
}



