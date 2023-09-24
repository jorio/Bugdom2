/****************************/
/*   	ENEMY.C  			*/
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



/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

int			gNumEnemyOfKind[NUM_ENEMY_KINDS];
int			gNumEnemies;
int			gMaxEnemies;


/*********************  INIT ENEMY MANAGER **********************/

void InitEnemyManager(void)
{
short	i;

	gNumEnemies = 0;

	if (gG4)					// tweak based on horsepower
		gMaxEnemies = 20;
	else
		gMaxEnemies = 16;

	for (i=0; i < NUM_ENEMY_KINDS; i++)
		gNumEnemyOfKind[i] = 0;


	gNumViri = 0;
	gCurrentCarryingMoth = nil;
	gKillerDragonFly = nil;

}


/********************** DELETE ENEMY **************************/

void DeleteEnemy(ObjNode *theEnemy)
{
	if (!(theEnemy->StatusBits & STATUS_BIT_ONSPLINE))		// spline enemies dont factor into the enemy counts!
	{
		gNumEnemyOfKind[theEnemy->Kind]--;					// dec kind count
		if (gNumEnemyOfKind[theEnemy->Kind] < 0)
		{
			DoAlert("DeleteEnemy: < 0");
			gNumEnemyOfKind[theEnemy->Kind] = 0;
		}

		gNumEnemies--;										// dec global count
	}


	DeleteObject(theEnemy);								// nuke the obj
}



/*********************** UPDATE ENEMY ******************************/

void UpdateEnemy(ObjNode *theNode)
{
	theNode->Speed3D = CalcVectorLength(&gDelta);

	UpdateObject(theNode);
}



/******************* MAKE ENEMY SKELETON *********************/
//
// This routine creates a non-character skeleton which is an enemy.
//
// INPUT:	itemPtr->parm[0] = skeleton type 0..n
//
// OUTPUT:	ObjNode or nil if err.
//

ObjNode *MakeEnemySkeleton(Byte skeletonType, short animNum, float x, float z, float scale, float rot, void *moveCall)
{
ObjNode	*newObj;

			/****************************/
			/* MAKE NEW SKELETON OBJECT */
			/****************************/

	gNewObjectDefinition.type 		= skeletonType;
	gNewObjectDefinition.animNum 	= animNum;							// assume default anim is #0
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= ENEMY_SLOT + skeletonType;
	gNewObjectDefinition.moveCall 	= moveCall;
	gNewObjectDefinition.rot 		= rot;
	gNewObjectDefinition.scale 		= scale;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);



				/* SET DEFAULT COLLISION INFO */

	newObj->CType = CTYPE_ENEMY|CTYPE_BLOCKCAMERA|CTYPE_AUTOTARGETWEAPON|CTYPE_KICKABLE|CTYPE_BUDDYATTRACT;
	newObj->CBits = CBITS_ALLSOLID;

	newObj->GotKickedCallback = DefaultGotKickedCallback;			// set callback for being kicked


	newObj->Coord.y -= newObj->BBox.min.y;						// offset so bottom touches ground
	UpdateObjectTransforms(newObj);

	return(newObj);
}




/***************** DETACH ENEMY FROM SPLINE *****************/
//
// OUTPUT: true if was on spline, false if wasnt
//

void DetachEnemyFromSpline(ObjNode *theNode, void *moveCall)
{
	if (!(theNode->StatusBits & STATUS_BIT_ONSPLINE))	// must be on spline
		return;

	DetachObjectFromSpline(theNode, moveCall);

	gNumEnemies++;									// count as a normal enemy now
	gNumEnemyOfKind[theNode->Kind]++;
}


#pragma mark -



/********************* FIND CLOSEST ENEMY *****************************/
//
// OUTPUT: nil if no enemies
//

ObjNode *FindClosestEnemy(OGLPoint3D *pt, float *dist)
{
ObjNode		*thisNodePtr,*best = nil;
float	d,minDist = 10000000;


	thisNodePtr = gFirstNodePtr;

	do
	{
		if (thisNodePtr->Slot >= SLOT_OF_DUMB)					// see if reach end of usable list
			break;

		if (thisNodePtr->CType & CTYPE_ENEMY)
		{
			d = CalcQuickDistance(pt->x,pt->z,thisNodePtr->Coord.x, thisNodePtr->Coord.z);
			if (d < minDist)
			{
				minDist = d;
				best = thisNodePtr;
			}
		}
		thisNodePtr = thisNodePtr->NextNode;		// next node
	}
	while (thisNodePtr != nil);

	*dist = minDist;
	return(best);
}



/************************* IS WATER IN FRONT OF ENEMY *****************************/
//
// coord is in gCoord
//

Boolean	IsWaterInFrontOfEnemy(float r)
{
float	x,z;

	x = gCoord.x - sin(r) * 30.0f;
	z = gCoord.z - cos(r) * 30.0f;

	return(IsXZOverWater(x,z));
}




/****************** DO ENEMY COLLISION DETECT ***************************/
//
// For use by non-skeleton enemies.
//
// OUTPUT: true = was deleted
//

Boolean DoEnemyCollisionDetect(ObjNode *theEnemy, uint32_t ctype, Boolean useBBoxBottom)
{
float	terrainY,distToFloor,bottomOff;
int		i;

			/* AUTOMATICALLY HANDLE THE BORING STUFF */

	HandleCollisions(theEnemy, ctype, -.9);


			/******************************/
			/* SCAN FOR INTERESTING STUFF */
			/******************************/

	for (i=0; i < gNumCollisions; i++)
	{
		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
			ObjNode	*hitObj = gCollisionList[i].objectPtr;		// get ObjNode of this collision
			ctype = hitObj->CType;

			if (ctype == INVALID_NODE_FLAG)						// see if has since become invalid
				continue;

					/* HURT */

			if (ctype & CTYPE_HURTENEMY)
			{
				if (theEnemy->HurtCallback != nil)							// if has a hurt callback
					if (theEnemy->HurtCallback(theEnemy, hitObj->Damage))	// handle hit (returns true if was deleted)
						return(true);
			}

				/* TOUCHED PLAYER */

			else
			if (ctype & CTYPE_PLAYER)
			{
				EnemyTouchedPlayer(theEnemy, hitObj);
			}
		}
	}



				/* CHECK PARTICLE COLLISION */

	if (theEnemy->HurtCallback != nil)							// if has a hurt callback
	{
		if (ParticleHitObject(theEnemy, PARTICLE_FLAGS_HURTENEMY))
		{

			if (theEnemy->HurtCallback(theEnemy, .3))			// handle hit (returns true if was deleted)
				return(true);
		}
	}

			/*************************************/
			/* CHECK & HANDLE TERRAIN  COLLISION */
			/*************************************/

	if (useBBoxBottom)
		bottomOff = theEnemy->BBox.min.y;						// use bbox's bottom
	else
		bottomOff = theEnemy->BottomOff;						// use collision box's bottom


	terrainY =  GetTerrainY(gCoord.x, gCoord.z);				// get terrain Y
	distToFloor = (gCoord.y + bottomOff) - terrainY;			// calc amount I'm above or under

	if (distToFloor <= 0.0f)									// see if on or under floor
	{
		gCoord.y = terrainY - bottomOff;
		gDelta.y = 0;
		theEnemy->StatusBits |= STATUS_BIT_ONGROUND;

				/* DEAL WITH SLOPES */
				//
				// Using the floor normal here, apply some deltas to it.
				// Only apply slopes when on the ground (or really close to it)
				//

//		gDelta.x += gRecentTerrainNormal.x * (gFramesPerSecondFrac * ENEMY_SLOPE_ACCEL);
//		gDelta.z += gRecentTerrainNormal.z * (gFramesPerSecondFrac * ENEMY_SLOPE_ACCEL);
	}
	return(false);
}


/********************** ENEMY TOUCHED PLAYER ***************************/

void EnemyTouchedPlayer(ObjNode *enemy, ObjNode *player)
{
	switch(enemy->Kind)
	{
		case	ENEMY_KIND_COMPUTERBUG:
				ComputerBugTouchedPlayer(enemy, player);
				break;

	}

}



#pragma mark -

/********************** MOVE ENEMY SKIP CHUNK *************************/

void MoveEnemySkipChunk(ObjNode *chunk)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(chunk);

	gDelta.y -= 2000.0f * fps;								// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	if (gCoord.y < (GetTerrainY(gCoord.x, gCoord.z) -  600.0f))	// see if gone
	{
		DeleteObject(chunk);
		return;
	}


	chunk->Rot.x += chunk->DeltaRot.x * fps;
	chunk->Rot.y += chunk->DeltaRot.y * fps;
	chunk->Rot.z += chunk->DeltaRot.z * fps;

	UpdateObject(chunk);
}











