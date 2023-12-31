/****************************/
/*   	SPLINE ITEMS.C      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static Boolean NilPrime(int splineNum, SplineItemType *itemPtr);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SPLINE_OBJECTS		100



/**********************/
/*     VARIABLES      */
/**********************/

SplineDefType	*gSplineList = NULL;
int				gNumSplines = 0;

int				gNumSplineObjects = 0;
static ObjNode	*gSplineObjectList[MAX_SPLINE_OBJECTS];


/**********************/
/*     TABLES         */
/**********************/

#define	MAX_SPLINE_ITEM_NUM		68				// for error checking!

Boolean (*gSplineItemPrimeRoutines[MAX_SPLINE_ITEM_NUM+1])(int, SplineItemType *) =
{
		NilPrime,							// My Start Coords
		NilPrime,
		NilPrime,
		NilPrime,
		PrimeEnemy_Gnome,					// gnome
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		PrimeEnemy_HouseFly,				// 10: housefly
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,							// 20:
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		PrimeBumbleBee,						// 25: bumble bee
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,							// 30:
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		PrimeEnemy_Flea,					// 38: flea
		PrimeEnemy_Tick,					// 39: tick
		PrimeSlotCar,						// 40: slot car
		NilPrime,
		NilPrime,
		PrimeEnemy_ToySoldier,				// 43: toy solider
		NilPrime,							// 44:
		PrimeEnemy_Otto,					// 45: otto enemy
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		PrimeEnemy_Dragonfly,				// 52: dragonfly
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		NilPrime,
		PrimeVacuume,						// 58: vacuume
		NilPrime,
		PrimeMothPath,						// 60: moth enemy
		PrimeEnemy_ComputerBug,				// 61: computer bug enemy
		NilPrime,
		PrimeHanger,						// 63: hanger
		NilPrime,
		PrimeEnemy_Roach,					// 65: roach enemy
		NilPrime,
		NilPrime,
		PrimeEnemy_Ant,						// 68: ant enemy
};



/********************* PRIME SPLINES ***********************/
//
// Called during terrain prime function to initialize
// all items on the splines and recalc spline coords
//

void PrimeSplines(void)
{
			/* ADJUST SPLINE TO GAME COORDINATES */

	for (int s = 0; s < gNumSplines; s++)
	{
		for (int i = 0; i < gSplineList[s].numPoints; i++)
		{
			gSplineList[s].pointList[i].x *= gMapToUnitValue;
			gSplineList[s].pointList[i].z *= gMapToUnitValue;
		}
	}


				/* CLEAR SPLINE OBJECT LIST */

	gNumSplineObjects = 0;										// no items in spline object node list yet

	for (int s = 0; s < gNumSplines; s++)
	{
		SplineDefType* spline = &gSplineList[s];						// point to this spline

				/* SCAN ALL ITEMS ON THIS SPLINE */

		for (int i = 0; i < spline->numItems; i++)
		{
			SplineItemType* itemPtr = &spline->itemList[i];				// point to this item
			int type = itemPtr->type;									// get item type
			GAME_ASSERT(type <= MAX_SPLINE_ITEM_NUM);

			Boolean flag = gSplineItemPrimeRoutines[type](s,itemPtr); 	// call item's Prime routine
			if (flag)
				itemPtr->flags |= ITEM_FLAGS_INUSE;						// set in-use flag
		}
	}
}


/******************** NIL PRIME ***********************/
//
// nothing prime
//

static Boolean NilPrime(int splineNum, SplineItemType *itemPtr)
{
	(void) splineNum;
	(void) itemPtr;
	return(false);
}


/*********************** GET COORD ON SPLINE FROM INDEX **********************/

void GetCoordOnSplineFromIndex(SplineDefType *splinePtr, float findex, float *x, float *z)
{
SplinePointType	*points;
int				i,i2, numPointsInSpline;
float			ratio,oneMinusRatio;

			/* CALC INDEX OF THIS PT AND NEXT */

	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline
	i = findex;													// round down to int
	i2 = i+1;
	if (i2 >= numPointsInSpline)								// make sure not go too far
		i2 = numPointsInSpline-1;

	points = splinePtr->pointList;								// point to point list


			/* INTERPOLATE */

	ratio = findex - (float)i;									// calc 0.0 - .999 remainder for weighing the points
	oneMinusRatio = 1.0f - ratio;

	*x = points[i].x * oneMinusRatio + points[i2].x * ratio;	// calc interpolated coord
	*z = points[i].z * oneMinusRatio + points[i2].z * ratio;
}


/*********************** GET COORD ON SPLINE **********************/

void GetCoordOnSpline(SplineDefType *splinePtr, float placement, float *x, float *z)
{
int				numPointsInSpline;
float			findex;

	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline
	findex = (float)numPointsInSpline * placement;				// calc float index

	GetCoordOnSplineFromIndex(splinePtr, findex, x, z);
}


/*********************** GET NEXT COORD ON SPLINE **********************/
//
// Same as above except returns coord of the next point on the spline instead of the exact
// current one.
//

void GetNextCoordOnSpline(SplineDefType *splinePtr, float placement, float *x, float *z)
{
float			numPointsInSpline;
float			findex;

	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline

	findex = (float)numPointsInSpline * placement;				// get index
	findex += 1.0f;												// bump it up +1

	if (findex >= (float)numPointsInSpline)						// see if wrap around
		findex = 0;

	GetCoordOnSplineFromIndex(splinePtr, findex, x, z);
}


/*********************** GET COORD ON SPLINE 2 **********************/
//
// Same as above except takes in input spline index offset
//

void GetCoordOnSpline2(SplineDefType *splinePtr, float placement, float offset, float *x, float *z)
{
float			numPointsInSpline;
int				findex;

	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline

	findex = (float)numPointsInSpline * placement;				// get index
	findex += offset;											// bump it up

	if (findex >= (float)numPointsInSpline)						// see if wrap around
		findex -= (float)numPointsInSpline;

	GetCoordOnSplineFromIndex(splinePtr, findex, x, z);
}


/********************* IS SPLINE ITEM ON ACTIVE TERRAIN ********************/
//
// Returns true if the input objnode is in visible range.
// Also, this function handles the attaching and detaching of the objnode
// as needed.
//

Boolean IsSplineItemOnActiveTerrain(ObjNode *theNode)
{
Boolean	visible = true;
long	row,col;


			/* IF IS ON AN ACTIVE SUPERTILE, THEN ASSUME VISIBLE */

	row = theNode->Coord.z * gTerrainSuperTileUnitSizeFrac;	// calc supertile row,col
	col = theNode->Coord.x * gTerrainSuperTileUnitSizeFrac;

	if ((row < 0) || (row >= gNumSuperTilesDeep) || (col < 0) || (col >= gNumSuperTilesWide))		// make sure in bounds
		visible = false;
	else
	{
		if (gSuperTileStatusGrid[row][col].playerHereFlag)
			visible = true;
		else
			visible = false;
	}
			/* HANDLE OBJNODE UPDATES */

	if (visible)
	{
		if (theNode->StatusBits & STATUS_BIT_DETACHED)			// see if need to insert into linked list
		{
			AttachObject(theNode, true);
		}
	}
	else
	{
		if (!(theNode->StatusBits & STATUS_BIT_DETACHED))		// see if need to remove from linked list
		{
			DetachObject(theNode, true);
		}
	}

	return(visible);
}


#pragma mark ======= SPLINE OBJECTS ================

/******************* ADD TO SPLINE OBJECT LIST ***************************/
//
// Called by object's primer function to add the detached node to the spline item master
// list so that it can be maintained.
//

void AddToSplineObjectList(ObjNode *theNode, Boolean setAim)
{
	GAME_ASSERT(gNumSplineObjects < MAX_SPLINE_OBJECTS);

	theNode->SplineObjectIndex = gNumSplineObjects;					// remember where in list this is

	gSplineObjectList[gNumSplineObjects++] = theNode;


			/* SET INITIAL AIM */

	if (setAim)
		SetSplineAim(theNode);
}

/************************ SET SPLINE AIM ***************************/

void SetSplineAim(ObjNode *theNode)
{
float	x,z;

	GetCoordOnSpline2(&gSplineList[theNode->SplineNum], theNode->SplinePlacement, 3, &x, &z);			// get coord of next point on spline
	theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->Coord.x, theNode->Coord.z,x,z);	// calc y rot aim


}

/****************** REMOVE FROM SPLINE OBJECT LIST **********************/
//
// OUTPUT:  true = the obj was on a spline and it was removed from it
//			false = the obj was not on a spline.
//

Boolean RemoveFromSplineObjectList(ObjNode *theNode)
{
	theNode->StatusBits &= ~STATUS_BIT_ONSPLINE;		// make sure this flag is off

	if (theNode->SplineObjectIndex != -1)
	{
		GAME_DEBUGASSERT(theNode == gSplineObjectList[theNode->SplineObjectIndex]);
		gSplineObjectList[theNode->SplineObjectIndex] = nil;			// nil out the entry into the list
		theNode->SplineObjectIndex = -1;
		theNode->SplineItemPtr = nil;
		theNode->SplineMoveCall = nil;
		return(true);
	}
	else
	{
		return(false);
	}
}


/**************** EMPTY SPLINE OBJECT LIST ***********************/
//
// Called by level cleanup to dispose of the detached ObjNode's in this list.
//

void EmptySplineObjectList(void)
{
	for (int i = 0; i < gNumSplineObjects; i++)
	{
		ObjNode	*o = gSplineObjectList[i];
		if (o)
		{
			GAME_DEBUGASSERT(o->SplineObjectIndex == i);
			DeleteObject(o);			// This will dispose of all memory used by the node.
										// RemoveFromSplineObjectList will be called by it.
			gSplineObjectList[i] = NULL;
		}
	}
	gNumSplineObjects = 0;
}


/******************* MOVE SPLINE OBJECTS **********************/

void MoveSplineObjects(void)
{
long	i;
ObjNode	*theNode;

	for (i = 0; i < gNumSplineObjects; i++)
	{
		theNode = gSplineObjectList[i];
		if (theNode)
		{
			if (theNode->SplineMoveCall)
			{
				KeepOldCollisionBoxes(theNode);					// keep old boxes & other stuff
				theNode->SplineMoveCall(theNode);				// call object's spline move routine
			}
		}
	}
}


/*********************** GET OBJECT COORD ON SPLINE **********************/
//
// OUTPUT: 	x,y = coords
//

void GetObjectCoordOnSpline(ObjNode *theNode)
{
float			placement;
SplineDefType	*splinePtr;

	placement = theNode->SplinePlacement;						// get placement
	if (placement < 0.0f)
		placement = 0;
	else
	if (placement >= 1.0f)
		placement = .999f;

	splinePtr = &gSplineList[theNode->SplineNum];			// point to the spline

	GetCoordOnSpline(splinePtr, placement, &theNode->Coord.x, &theNode->Coord.z);		// get coord

	theNode->Delta.x = (theNode->Coord.x - theNode->OldCoord.x) * gFramesPerSecond;	// calc delta
	theNode->Delta.z = (theNode->Coord.z - theNode->OldCoord.z) * gFramesPerSecond;

}


/*********************** GET OBJECT COORD ON SPLINE 2 **********************/

void GetObjectCoordOnSpline2(ObjNode *theNode, float *x, float *z)
{
float			placement;
SplineDefType	*splinePtr;

	placement = theNode->SplinePlacement;						// get placement
	if (placement < 0.0f)
		placement = 0;
	else
	if (placement >= 1.0f)
		placement = .999f;

	splinePtr = &gSplineList[theNode->SplineNum];			// point to the spline

	GetCoordOnSpline(splinePtr, placement, x, z);		// get coord
}



/******************* INCREASE SPLINE INDEX *********************/
//
// Moves objects on spline at given speed
//
// Returns true if increase caused item to wrap to beginning of spline
//

Boolean IncreaseSplineIndex(ObjNode *theNode, float speed)
{
SplineDefType	*splinePtr;
float			numPointsInSpline;

	speed *= gFramesPerSecondFrac;

	splinePtr = &gSplineList[theNode->SplineNum];			// point to the spline
	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline

	theNode->SplinePlacement += speed / numPointsInSpline;
	if (theNode->SplinePlacement > 1.0f)
	{
		theNode->SplinePlacement -= 1.0f;
		if (theNode->SplinePlacement > 1.0f)			// see if it wrapped somehow
			theNode->SplinePlacement = 0;
		return(true);
	}
	return(false);
}


/******************* INCREASE SPLINE INDEX ZIGZAG *********************/
//
// Moves objects on spline at given speed, but zigzags
//

void IncreaseSplineIndexZigZag(ObjNode *theNode, float speed)
{
SplineDefType	*splinePtr;
float			numPointsInSpline;

	speed *= gFramesPerSecondFrac;

	splinePtr = &gSplineList[theNode->SplineNum];			// point to the spline
	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline

			/* GOING BACKWARD */

	if (theNode->StatusBits & STATUS_BIT_REVERSESPLINE)			// see if going backward
	{
		theNode->SplinePlacement -= speed / numPointsInSpline;
		if (theNode->SplinePlacement <= 0.0f)
		{
			theNode->SplinePlacement = 0;
			theNode->StatusBits ^= STATUS_BIT_REVERSESPLINE;	// toggle direction
		}
	}

		/* GOING FORWARD */

	else
	{
		theNode->SplinePlacement += speed / numPointsInSpline;
		if (theNode->SplinePlacement >= .999f)
		{
			theNode->SplinePlacement = .999f;
			theNode->StatusBits ^= STATUS_BIT_REVERSESPLINE;	// toggle direction
		}
	}
}


/******************** DETACH OBJECT FROM SPLINE ***********************/

void DetachObjectFromSpline(ObjNode *theNode, void (*moveCall)(ObjNode*))
{

	if (!(theNode->StatusBits & STATUS_BIT_ONSPLINE))
		return;

		/***********************************************/
		/* MAKE SURE ALL COMPONENTS ARE IN LINKED LIST */
		/***********************************************/

	AttachObject(theNode, true);


			/* REMOVE FROM SPLINE */

	RemoveFromSplineObjectList(theNode);

	theNode->InitCoord  = theNode->Coord;			// remember where started

	theNode->MoveCall = moveCall;

}


/******************* DRAW SPLINES FOR DEBUGGING *********************/

void DrawSplines(void)
{
	const float kMapToUnit = ((float)DEFAULT_TERRAIN_SCALE/OREOMAP_TILE_SIZE);
	const int kLineStripInc = 32;

	for (int i = 0; i < gNumSplines; i++)
	{
		const SplineDefType* spline = &gSplineList[i];

		if (spline->numPoints == 0)
		{
			continue;
		}

		const SplinePointType* points = spline->pointList;
		const SplinePointType* nubs = spline->nubList;
		const int halfway = spline->numPoints / 2;

		if (SeeIfCoordsOutOfRange(points[0].x, points[0].z)
			&& SeeIfCoordsOutOfRange(points[halfway].x, points[halfway].z))
		{
			continue;
		}

		glBegin(GL_LINE_STRIP);
		for (int j = 0; j < spline->numPoints; j += kLineStripInc)
		{
			float x = points[j].x;
			float z = points[j].z;
			float y = GetTerrainY(x, z) + 15;
			glVertex3f(x, y, z);
		}
		if ((spline->numPoints % kLineStripInc) != 0)
		{
			float x = points[spline->numPoints-1].x;
			float z = points[spline->numPoints-1].z;
			float y = GetTerrainY(x, z) + 15;
			glVertex3f(x, y, z);
		}
		glEnd();

		glBegin(GL_LINES);
		for (int nub = 0; nub < spline->numNubs; nub++)
		{
			float flagSize = nub == 0 ? 150 : 30;
			float poleSize = flagSize;

			float x = nubs[nub].x * kMapToUnit;
			float z = nubs[nub].z * kMapToUnit;
			float y1 = GetTerrainY(x, z) + 17;
			float y2 = y1 + poleSize;
			glVertex3f(x, y1, z);
			glVertex3f(x, y2, z);

			if (nub < spline->numNubs - 1)
			{
				OGLVector3D flagDir = { nubs[nub+1].x * kMapToUnit - x, 0, nubs[nub+1].z * kMapToUnit - z };
				FastNormalizeVector(flagDir.x, flagDir.y, flagDir.z, &flagDir);

				float y3 = y2 - flagSize * 0.25f;
				float y4 = y2 - flagSize * 0.5f;

				glVertex3f(x, y2, z);
				glVertex3f(x + flagDir.x * flagSize, y3, z + flagDir.z * flagSize);
				glVertex3f(x + flagDir.x * flagSize, y3, z + flagDir.z * flagSize);
				glVertex3f(x, y4, z);

				if (nub == 0)
				{
					for (int baton = 0; baton < i + 1; baton++)
					{
						float x2 = x + ((2 + baton) * 4) * flagDir.x;
						float z2 = z + ((2 + baton) * 4) * flagDir.z;
						glVertex3f(x2, y3-10, z2);
						glVertex3f(x2, y3+10, z2);
					}
				}
			}
		}
		glEnd();
	}
}
