/****************************/
/*   	ENEMY: MOTH.C		*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLPoint3D				gCoord;
extern	int						gNumEnemies;
extern	float					gFramesPerSecondFrac,gGlobalTransparency;
extern	OGLVector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];
extern	u_long		gAutoFadeStatusBits;
extern	SparkleType	gSparkles[];
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	int					gLevelNum,gMaxEnemies;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TerrainItemEntryType 	**gMasterItemList;
extern	short					gNumTerrainItems,gNumTicks;
extern	Boolean					gPlayerIsDead;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveMoth(ObjNode *theNode);
static void OrbitMoth(ObjNode *theNode);
static void MothChasePlayer(ObjNode *theNode);
static Boolean MothGoAway(ObjNode *theNode);
static void MothCarryPlayer(ObjNode *theNode);
static void UpdateMoth(ObjNode *theNode);
static void FindMothTarget(ObjNode *moth);
static Boolean PlayerHasMothBall(void);
static float FindClosestPointOnMothPath(float x, float z, int pathNum);


/****************************/
/*    CONSTMOTHS             */
/****************************/

#define	MAX_MOTHS				8

#define	MOTH_SCALE				.7f

#define	MOTH_CHASE_RANGE		250.0f

#define MOTH_TURN_SPEED			6.0f

#define	NAB_HEIGHT			120.0f
#define CARRY_HEIGHT		650.0f
#define	MOTH_FLIGHT_Y		300.0f
#define	MOTH_ORBIT_RADIUS	130.0f


		/* ANIMS */

enum
{
	MOTH_ANIM_FLY
};



enum
{
	MOTH_MODE_ORBIT,
	MOTH_MODE_CHASE,
	MOTH_MODE_CARRY,
	MOTH_MODE_DONE
};


#define	MAX_MOTH_PATHS	10


/*********************/
/*    VARIABLES      */
/*********************/

ObjNode	*gCurrentCarryingMoth = nil;

float	gMothTargetX,gMothTargetZ;

#define	MothTargetID	Special[0]

static	int	gMothPaths[MAX_MOTH_PATHS];


/************************ ADD MOTH ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Moth(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;
int		i,j;

	if (itemPtr->parm[3] & 1)								// see if target
		return(true);


				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_MOTH;
	gNewObjectDefinition.animNum 	= MOTH_ANIM_FLY;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + MOTH_FLIGHT_Y + (RandomFloat() * 100.0f);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-1;
	gNewObjectDefinition.moveCall 	= MoveMoth;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	gNewObjectDefinition.scale 		= MOTH_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;

	newObj->StatusBits |= STATUS_BIT_NOTEXTUREWRAP;

	newObj->Coord.y += MOTH_ORBIT_RADIUS;							// set orbital radius


				/* SET BETTER INFO */

	newObj->Kind 		= ENEMY_KIND_MOTH;
	newObj->Mode 	= MOTH_MODE_ORBIT;
	newObj->MothTargetID = itemPtr->parm[0];						// get target ID #


	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 4, 4, true);


			/******************/
			/* MAKE EYES GLOW */
			/******************/

	for (j = 0; j < 2; j++)
	{

				/* CREATE EYE LIGHTS */

		i = newObj->Sparkles[j] = GetFreeSparkle(newObj);				// get free sparkle slot
		if (i != -1)
		{
			gSparkles[i].flags = 0;
			gSparkles[i].where.x = newObj->Coord.x;
			gSparkles[i].where.y = newObj->Coord.y;
			gSparkles[i].where.z = newObj->Coord.z;

			gSparkles[i].aim.x =
			gSparkles[i].aim.y =
			gSparkles[i].aim.z = 1;

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = 1;

			gSparkles[i].scale = 20.0f;
			gSparkles[i].separation = 10.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_YellowGlint;
		}
	}


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_MOTH]++;


	return(true);
}





/********************* MOVE MOTH **************************/

static void MoveMoth(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	switch(theNode->Mode)
	{
		case	MOTH_MODE_ORBIT:
				OrbitMoth(theNode);
				break;

		case	MOTH_MODE_CHASE:
				MothChasePlayer(theNode);
				break;

		case	MOTH_MODE_CARRY:
				MothCarryPlayer(theNode);
				break;

		case	MOTH_MODE_DONE:
				if (MothGoAway(theNode))
					return;
				break;
	}

	UpdateMoth(theNode);
}



/***************** ORBIT MOTH ****************/

static void OrbitMoth(ObjNode *theNode)
{
float		fps = gFramesPerSecondFrac;
OGLMatrix4x4	m;

			/* SEE IF NEW RANDOM ORBIT */

	theNode->Timer -= fps;
	if (theNode->Timer <= 0.0f)
	{
		theNode->Timer = 1.0f + RandomFloat() * 2.0f;

		theNode->DeltaRot.x = RandomFloat2() * 9.0f;					// random orbital spin
		theNode->DeltaRot.y = RandomFloat2() * 13.0f;
		theNode->DeltaRot.z = RandomFloat2() * 9.0f;
	}


	OGLMatrix4x4_SetRotateAboutPoint(&m, &theNode->InitCoord,
									theNode->DeltaRot.x * fps,
									theNode->DeltaRot.y * fps,
									theNode->DeltaRot.z * fps);

	OGLPoint3D_Transform(&gCoord, &m, &gCoord);


	theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z, gCoord.x, gCoord.z);



			/* SEE IF AVOID MOTH BALL */

	if (PlayerHasMothBall())
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < MOTH_CHASE_RANGE)
		{
			theNode->TerrainItemPtr = nil;					// dont ever come back
			theNode->Mode = MOTH_MODE_DONE;
		}
	}

		/* SEE IF CLOSE ENOUGH TO ATTACK */

	else
	if (!gCurrentCarryingMoth)								// only if not being carried
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < MOTH_CHASE_RANGE)
		{
			theNode->Mode = MOTH_MODE_CHASE;
		}
	}
}



/***************** MOTH CHASE PLAYER ****************/

static void MothChasePlayer(ObjNode *theNode)
{
float		fps = gFramesPerSecondFrac;
float		d,y;
const OGLPoint3D	myCoord = gPlayerInfo.coord;

			/* SEE IF AVOID MOTH BALL OR IF SOMEONE ELSE ALREADY GOT PLAYER */

	if (PlayerHasMothBall() || gCurrentCarryingMoth)
	{
		theNode->Mode = MOTH_MODE_DONE;
		return;
	}


	TurnObjectTowardTarget(theNode, &gCoord, myCoord.x, myCoord.z, MOTH_TURN_SPEED, false);

			/* MOVE TOWARD PLAYER */

	d =  OGLPoint3D_Distance(&gCoord, &myCoord) * fps * .02f;

	if (gCoord.x < myCoord.x)
	{
		gDelta.x += (myCoord.x - gCoord.x) * d;
		gCoord.x += gDelta.x * fps;
		if (gCoord.x > myCoord.x)
		{
			gCoord.x = myCoord.x;
			gDelta.x = 0;
		}
	}
	else
	{
		gDelta.x += (myCoord.x - gCoord.x) * d;
		gCoord.x += gDelta.x * fps;
		if (gCoord.x < myCoord.x)
		{
			gCoord.x = myCoord.x;
			gDelta.x = 0;
		}
	}

	if (gCoord.z < myCoord.z)
	{
		gDelta.z += (myCoord.z - gCoord.z) * d;
		gCoord.z += gDelta.z * fps;
		if (gCoord.z > myCoord.z)
		{
			gCoord.z = myCoord.z;
			gDelta.z = 0;
		}
	}
	else
	{
		gDelta.z += (myCoord.z - gCoord.z) * d;
		gCoord.z += gDelta.z * fps;
		if (gCoord.z < myCoord.z)
		{
			gCoord.z = myCoord.z;
			gDelta.z = 0;
		}
	}

	y = myCoord.y + NAB_HEIGHT;
	if (gCoord.y < y)
	{
		gDelta.y += (y - gCoord.y) * d;
		gCoord.y += gDelta.y * fps;
		if (gCoord.y > y)
		{
			gCoord.y = y;
			gDelta.y = 0;
		}
	}
	else
	{
		gDelta.y += (y - gCoord.y) * d;
		gCoord.y += gDelta.y * fps;
		if (gCoord.y < y)
		{
			gCoord.y = y;
			gDelta.y = 0;
		}
	}


				/*********************/
				/* SEE IF NAB PLAYER */
				/*********************/

	if (gPlayerIsDead || (gPlayerInfo.objNode->Skeleton->AnimNum == PLAYER_ANIM_CARRIED))		// no nabbing in this condition
		return;


		/* CHECK OTHER CONDITIONS */

	if (gCurrentCarryingMoth == nil)
	{
		if (OGLPoint3D_Distance(&gCoord, &myCoord) < 150.0f)				// see if can nab now
		{
			ObjNode *player = gPlayerInfo.objNode;

			theNode->Mode = MOTH_MODE_CARRY;

				/* SET PLAYER ANIM */

			MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_CARRIED, 7);

					/* SEE IF DROP HELD OBJECT */

			if (gPlayerInfo.heldObject)
			{
				ObjNode *held = gPlayerInfo.heldObject;
				ReleaseHeldObject(player, held);
				held->Delta.x =
				held->Delta.y =
				held->Delta.z = 0;							// make sure it doesn't go zooming away
			}

			gCurrentCarryingMoth = theNode;
			FindMothTarget(theNode);

			gDelta.y = 0;
		}
	}

}


/***************** MOTH GO AWAY ****************/

static Boolean MothGoAway(ObjNode *theNode)
{
float		fps = gFramesPerSecondFrac;

	if (theNode->StatusBits & STATUS_BIT_ISCULLED)			// once culled, make it go away
	{
		DeleteEnemy(theNode);
		return(true);
	}

	gDelta.y += 1600.0f * fps;

	gCoord.x += gDelta.x * fps;						// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	return(false);
}




/***************** MOTH CARRY PLAYER ****************/

static void MothCarryPlayer(ObjNode *theNode)
{
float		fps = gFramesPerSecondFrac;
float		r,y;
float		targetX, targetZ, dist;
OGLVector2D	v;


		/* MOVE INDEX ALONG THE SPLINE */

	if (IncreaseSplineIndex(theNode, 250))									// inc index & see if @ end
	{
		gCurrentCarryingMoth = nil;											// not chasing or carrying anymore
		theNode->Mode = MOTH_MODE_DONE;
		return;
	}
	GetObjectCoordOnSpline2(theNode, &targetX, &targetZ);					// get coords of it


			/* MOVE TOWARD TARGET */

	v.x = targetX - gCoord.x;												// calc vector to target
	v.y = targetZ - gCoord.z;
	FastNormalizeVector2D(v.x, v.y, &v, true);

	dist = CalcDistance(targetX, targetZ, gCoord.x, gCoord.z);				// calc dist to target coord
	dist *= 3.0f * fps;														// move just a fraction of the dist
	v.x *= dist;															// calc delta vector
	v.y *= dist;

	gCoord.x += v.x;														// move moth
	gCoord.z += v.y;


			/* AIM  */

	r = theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z, gCoord.x, gCoord.z);



			/* CHECK Y */

	y = GetTerrainY(gCoord.x, gCoord.z);			// get ground y here
	if (gCoord.y < (y + CARRY_HEIGHT))				// see if below hover height
	{
		gDelta.y += 700.0f * fps;
	}
	else											// above hover height
	{
		if (gDelta.y > 1.0f)
			gDelta.y *= .5f;						// slow it down
		else
			gDelta.y -= 500.0f * fps;
	}

	gCoord.y += gDelta.y * fps;						// move on y

}



/***************** UPDATE MOTH ************************/

static void UpdateMoth(ObjNode *theNode)
{
const static OGLPoint3D	leftEye = {-8, -1, -23};
const static OGLPoint3D	rightEye = {8, -1, -23};
float	r, aimX, aimZ;
OGLMatrix4x4	m;
int		i;

	UpdateEnemy(theNode);

			/*********/
			/* SOUND */
			/*********/

	if (theNode->EffectChannel == -1)
		theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_MOTHFLAP, &theNode->Coord, NORMAL_CHANNEL_RATE - (MyRandomLong() & 0xfff), .2);
	else
		Update3DSoundChannel(EFFECT_MOTHFLAP, &theNode->EffectChannel, &theNode->Coord);


		/***********************/
		/* UPDATE EYE SPARKLES */
		/***********************/

	r = theNode->Rot.y;
	aimX = -sin(r);
	aimZ = -cos(r);

	FindJointFullMatrix(theNode, 6, &m);						// get head matrix


		/* UPDATE RIGHT EYE */

	i = theNode->Sparkles[0];												// get sparkle index
	if (i != -1)
	{
		OGLPoint3D_Transform(&rightEye, &m, &gSparkles[i].where);			// calc coord of right eye
		gSparkles[i].aim.x = aimX;											// update aim vector
		gSparkles[i].aim.z = aimZ;
	}


		/* UPDATE LEFT EYE */

	i = theNode->Sparkles[1];												// get sparkle index
	if (i != -1)
	{
		OGLPoint3D_Transform(&leftEye, &m, &gSparkles[i].where);			// calc coord of right eye
		gSparkles[i].aim.x = aimX;											// update aim vector
		gSparkles[i].aim.z = aimZ;
	}


}


#pragma mark -



/***************** PLAYER HAS MOTH BALL **************************/

static Boolean PlayerHasMothBall(void)
{
ObjNode *held = gPlayerInfo.heldObject;

	if (held == nil)
		return(false);

	if (held->Kind == PICKUP_KIND_MOTHBALL)
		return(true);

	return(false);

}


#pragma mark -

/******************** FIND MOTH TARGET *******************/

static void FindMothTarget(ObjNode *moth)
{
	moth->SplineNum = gMothPaths[moth->MothTargetID];						// get spline #
	moth->SplinePlacement = FindClosestPointOnMothPath(gCoord.x, gCoord.z, moth->MothTargetID);	// find spline start placement
}



/************************ PRIME MOTH PATH *************************/
//
// The moth paths are simply the shared paths that moths will travel on when carrying Skip
//

Boolean PrimeMothPath(long splineNum, SplineItemType *itemPtr)
{
int	pathNum = itemPtr->parm[0];

	if (pathNum >= MAX_MOTH_PATHS)
		DoFatalAlert("PrimeMothPath: pathNum > MAX_MOTH_PATHS");

	gMothPaths[pathNum] = splineNum;					// remember which spline this moth path is

	return(true);
}


/******************** FIND CLOSEST POINT ON MOTH PATH ***********************/

static float FindClosestPointOnMothPath(float x, float z, int pathNum)
{
int				splineNum;
SplineDefType	*splinePtr;
int				numPointsInSpline, i, bestIndex;
float			dist, bestDist;
SplinePointType	*points;


	splineNum = gMothPaths[pathNum];						// get spline #
	splinePtr = &(*gSplineList)[splineNum];					// point to the spline
	numPointsInSpline = splinePtr->numPoints;				// get # points in the spline
	points = *splinePtr->pointList;							// point to point list

			/* SCAN POINTS ON SPLINE FOR CLOSEST */

	bestIndex = -1;
	bestDist = 100000000;

	for (i = 0; i < numPointsInSpline; i += 100)
	{
		dist = CalcQuickDistance(x, z, points[i].x, points[i].z);		// calc dist to this spline pt
		if (dist < bestDist)											// see if it's the closest so far
		{
			bestDist = dist;
			bestIndex = i;
		}
	}

	return((float)bestIndex / (float)numPointsInSpline);				// return the placement 0.0 -> 1.0 value
}




