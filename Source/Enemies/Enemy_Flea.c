/****************************/
/*   ENEMY: FLEA.C	*/
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

static ObjNode *MakeFlea(float x, float z, short animNum);
static void MoveFlea(ObjNode *theNode);
static void MoveFlea_Standing(ObjNode *theNode);
static void  MoveFlea_Walk(ObjNode *theNode);
static void  MoveFlea_PickUp(ObjNode *theNode);
static void  MoveFlea_Death(ObjNode *theNode);
static void MoveFleaOnSpline(ObjNode *theNode);
static void UpdateFlea(ObjNode *theNode);
static void  MoveFlea_GotHit(ObjNode *theNode);
static Boolean HurtFlea(ObjNode *enemy, float damage);
static void  MoveFlea_Throw(ObjNode *theNode);
static void  MoveFlea_Hop(ObjNode *theNode);
static Boolean SeeIfFleaAttack(ObjNode *theNode, float angleToTarget, float dist);
static void UpdateFleaEyes(ObjNode *theNode);

static void KillFlea(ObjNode *enemy);
static void FleaGotKickedCallback(ObjNode *player, ObjNode *kickedObj);

static void GiveFleaACap(ObjNode *enemy);
static void UpdateFleaCap(ObjNode *flea);
static void FleaThrowCap(ObjNode *theEnemy);
static void MoveFleaCap(ObjNode *theNode);

/****************************/
/*    CONSTFLEAS             */
/****************************/

#define	MAX_FLEAS				8

#define	FLEA_SCALE				1.8f

#define	FLEA_CHASE_DIST_MIN 	250.0f
#define	FLEA_DETACH_DIST		400.0f

#define	FLEA_CHASE_DIST_HOP		1400.0f					// dist to start hop chase
#define	FLEA_CHASE_DIST_WALK	350.0f					// how close to walk instead of hop

#define	FLEA_TARGET_OFFSET		20.0f

#define FLEA_TURN_SPEED			20.0f
#define FLEA_WALK_SPEED			300.0f

#define	FLEA_HEALTH				1.1f
#define	FLEA_DAMAGE				.2f


		/* ANIMS */

enum
{
	FLEA_ANIM_STAND,
	FLEA_ANIM_HOP,
	FLEA_ANIM_THROW,
	FLEA_ANIM_DEATH,
	FLEA_ANIM_PICKUP,
	FLEA_ANIM_WALK,
	FLEA_ANIM_GOTHIT
};


#define	FLEA_JOINTNUM_RIGHTHAND			10
#define	FLEA_JOINTNUM_HEAD				4



		/* MODES */

enum
{
	FLEA_MODE_NONE,
	FLEA_MODE_WATCHCAP,
	FLEA_MODE_GETCAP
};


		/* CAP */

#define	CAP_ATTACK_DIST				700.0f
#define	CAP_THROW_MIN_ANGLE			0.04f
#define	DIST_TO_RETRIVE_CAP			140.0f
#define	CAP_DAMAGE					.2f

#define	CAP_SPEED					800.0f


/*********************/
/*    VARIABLES      */
/*********************/

int	gTotalFleas = 0;
int	gNumKilledFleas = 0;

#define	ButtTimer			SpecialF[2]


#define	ThrowCap			Flag[0]					// set by animation when spear should be thrown
#define PickUpNow			Flag[0]					// set by anim when pickup should occur
#define	HopNow				Flag[2]					// set by anim when hop should occur

#define	HasCap				Flag[1]				// true if this guy has a bottle cap
#define	ThrownCap			SpecialObjPtr[0]		// objnode of thrown cap


		/* CAP */

#define CapOwner			SpecialObjPtr[0]		// objnode of ant who threw spear
#define	PrevSides			Special[1]


/************************ ADD FLEA ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Flea(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;

	if (gLevelNum != LEVEL_NUM_FIDO)								// ALWAYS ADD ALL FOR FIDO LEVEL
	{
		if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
			return(false);

		if (!(itemPtr->parm[3] & 1))								// see if always add
		{
			if (gNumEnemyOfKind[ENEMY_KIND_FLEA] >= MAX_FLEAS)
				return(false);
		}
	}

	newObj = MakeFlea(x, z, FLEA_ANIM_STAND);

	newObj->TerrainItemPtr = itemPtr;

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_FLEA]++;


	return(true);
}


/************************* MAKE FLEA ****************************/

static ObjNode *MakeFlea(float x, float z, short animNum)
{
ObjNode	*newObj;
short	i,j;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_FLEA,animNum, x,z, FLEA_SCALE, 0, MoveFlea);



				/* SET BETTER INFO */

	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync

	newObj->StatusBits |= STATUS_BIT_NOTEXTUREWRAP;

	newObj->Health 		= FLEA_HEALTH;
	newObj->Damage 		= FLEA_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_FLEA;
	newObj->Mode		= FLEA_MODE_NONE;


				/* SET COLLISION INFO */

	newObj->ForceLookAtDist	= 800.0f;

	CreateCollisionBoxFromBoundingBox(newObj, .7,1);
	CalcNewTargetOffsets(newObj,FLEA_TARGET_OFFSET);


	newObj->HurtCallback 		= HurtFlea;							// set hurt callback function
	newObj->GotKickedCallback 	= FleaGotKickedCallback;			// set callback for being kicked

	newObj->Damage = FLEA_DAMAGE;


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 4, 6,false);


		/* GIVE THE FLEA A BOTTLE CAP */

	GiveFleaACap(newObj);


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
			gSparkles[i].color.g = 0;
			gSparkles[i].color.b = 0;
			gSparkles[i].color.a = 1;

			gSparkles[i].scale = 90.0f;
			gSparkles[i].separation = 10.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark3;
		}
	}


	return(newObj);

}





/********************* MOVE FLEA **************************/

static void MoveFlea(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveFlea_Standing,
					MoveFlea_Hop,
					MoveFlea_Throw,
					MoveFlea_Death,
					MoveFlea_PickUp,
					MoveFlea_Walk,
					MoveFlea_GotHit,
				};

	if (gLevelNum != LEVEL_NUM_FIDO)						// don't bother deleting on Fido level
	{
		if (TrackTerrainItem(theNode))						// just check to see if it's gone
		{
			DeleteEnemy(theNode);
			return;
		}
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE FLEA: STANDING ******************************/

static void  MoveFlea_Standing(ObjNode *theNode)
{
float	angleToTarget;
ObjNode	*cap;
float	dist;

	switch(theNode->Mode)
	{
				/****************************************/
				/* HANDLE WAITING FOR BOTTLE CAP TO HIT */
				/****************************************/

		case	FLEA_MODE_WATCHCAP:

						/* VERIFY CAP */

				cap = theNode->ThrownCap;														// get objnode of cap
				if ((cap->CapOwner != theNode) || (cap->CType == INVALID_NODE_FLAG))			// see if isnt valid anymore
				{
					theNode->Mode = FLEA_MODE_NONE;
					break;
				}

					/* SEE IF CAP HAS IMPACTED */

				if (cap->StatusBits & STATUS_BIT_ONGROUND)
				{
					theNode->Mode = FLEA_MODE_GETCAP;
					MorphToSkeletonAnim(theNode->Skeleton, FLEA_ANIM_WALK, 8);
				}
				break;


				/**************************/
				/* HANDLE ALL OTHER MODES */
				/**************************/

		default:

							/* TURN TOWARDS ME */

				angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, FLEA_TURN_SPEED, false);

				dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);		// calc dist to player

				if (!gGamePrefs.kiddieMode)
				{
							/* SEE IF THROW CAP */

					if (theNode->HasCap)
					{
						if (SeeIfFleaAttack(theNode, angleToTarget, dist))
							break;
					}

						/* SEE IF START CHASING PLAYER */

					if ((dist < FLEA_CHASE_DIST_HOP) && (dist > FLEA_CHASE_DIST_MIN))
					{
						gDelta.x = gDelta.z = 0;

						if (!SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))	// dont chase thru things
						{
							if (dist < FLEA_CHASE_DIST_WALK)				// see if close enough to walk
								MorphToSkeletonAnim(theNode->Skeleton, FLEA_ANIM_WALK, 5);
							else
								MorphToSkeletonAnim(theNode->Skeleton, FLEA_ANIM_HOP, 9);
						}
					}
				}
	}

				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateFlea(theNode);
}




/********************** MOVE FLEA: WALKING ******************************/

static void  MoveFlea_Walk(ObjNode *theNode)
{
float		r,fps,dist;
ObjNode		*cap;

	fps = gFramesPerSecondFrac;

	switch(theNode->Mode)
	{
				/*************************/
				/* HANDLE RETREIVING CAP */
				/*************************/

		case	FLEA_MODE_GETCAP:

						/* VERIFY CAP */

				cap = theNode->ThrownCap;														// get objnode of spear
				if ((cap->CapOwner != theNode) || (cap->CType == INVALID_NODE_FLAG))			// see if isnt valid anymore
				{
					theNode->Mode = FLEA_MODE_NONE;
					break;
				}

						/* MOVE TOWARD CAP */

				TurnObjectTowardTarget(theNode, &gCoord, cap->Coord.x, cap->Coord.z, FLEA_TURN_SPEED, false);

				r = theNode->Rot.y;
				gDelta.x = -sin(r) * FLEA_WALK_SPEED;
				gDelta.z = -cos(r) * FLEA_WALK_SPEED;
				gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity

				gCoord.x += gDelta.x * fps;
				gCoord.y += gDelta.y * fps;
				gCoord.z += gDelta.z * fps;


						/* SEE IF GET CAP */

				if (CalcQuickDistance(gCoord.x, gCoord.z, cap->Coord.x, cap->Coord.z) < DIST_TO_RETRIVE_CAP)
				{
					theNode->Mode = FLEA_MODE_NONE;
					theNode->PickUpNow = false;
					MorphToSkeletonAnim(theNode->Skeleton, FLEA_ANIM_PICKUP, 4);
				}

				break;


			/********************************/
			/* HANDLE NORMAL WALKING AROUND */
			/********************************/

		default:
						/* MOVE TOWARD PLAYER */

				if (SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))	// dont chase thru things
					goto stand;

				TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, FLEA_TURN_SPEED, false);

				r = theNode->Rot.y;
				gDelta.x = -sin(r) * FLEA_WALK_SPEED;
				gDelta.z = -cos(r) * FLEA_WALK_SPEED;
				gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
				gCoord.x += gDelta.x * fps;
				gCoord.y += gDelta.y * fps;
				gCoord.z += gDelta.z * fps;


						/* SEE IF STAND */

				dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
				if ((dist < FLEA_CHASE_DIST_MIN) || (dist > FLEA_CHASE_DIST_HOP) || gGamePrefs.kiddieMode)
				{
stand:
					MorphToSkeletonAnim(theNode->Skeleton, FLEA_ANIM_STAND, 2.0);
				}

	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	UpdateFlea(theNode);
}


/********************** MOVE FLEA: HOP ******************************/

static void  MoveFlea_Hop(ObjNode *theNode)
{
float		r,fps,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	ApplyFrictionToDeltas(1200.0,&gDelta);

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, FLEA_TURN_SPEED, false);
	r = theNode->Rot.y;

	if (theNode->HopNow)
	{
		gDelta.x = -sin(r) * 1000.0f;
		gDelta.z = -cos(r) * 1000.0f;

		theNode->HopNow = false;
	}

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* SEE IF CLOSE ENOUGH TO STOP OR WALK */

	dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);

	if (dist < FLEA_CHASE_DIST_MIN)
		MorphToSkeletonAnim(theNode->Skeleton, FLEA_ANIM_STAND, 4.0);
	else
	if (dist < FLEA_CHASE_DIST_WALK)
		MorphToSkeletonAnim(theNode->Skeleton, FLEA_ANIM_WALK, 4.0);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	UpdateFlea(theNode);
}



/********************** MOVE FLEA: THROW ******************************/

static void  MoveFlea_Throw(ObjNode *theNode)
{
float	angleToTarget;

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, FLEA_TURN_SPEED, false);

			/*************************/
			/* SEE IF LAUNCH THE CAP */
			/*************************/

	if (theNode->ThrowCap)
	{
		theNode->ThrowCap = false;

				/* MAKE SURE STILL AIMED AT ME */

		if (angleToTarget < CAP_THROW_MIN_ANGLE)
			FleaThrowCap(theNode);
	}


			/* SEE IF DONE WITH ANIM */

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, FLEA_ANIM_STAND, 5);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateFlea(theNode);
}


/********************** MOVE FLEA: PICKUP ******************************/

static void  MoveFlea_PickUp(ObjNode *theNode)
{
ObjNode	*spearObj;

	gDelta.x = gDelta.z = 0;						// make sure not moving during this anim

			/**********************/
			/* SEE IF GET THE CAP */
			/**********************/

	if (theNode->PickUpNow)
	{
		theNode->PickUpNow = false;

				/* VERIFY CAP */

		spearObj = theNode->ThrownCap;														// get objnode of spear
		if ((spearObj->CapOwner == theNode) && (spearObj->CType != INVALID_NODE_FLAG))		// make sure spear obj is valid
			DeleteObject(spearObj);															// delete the old spear

		GiveFleaACap(theNode);																// give it a new spear (even if old is invalid)
	}


			/* SEE IF DONE WITH ANIM */

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, FLEA_ANIM_STAND, 5);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateFlea(theNode);
}


/********************** MOVE FLEA: GOT HIT ******************************/

static void  MoveFlea_GotHit(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;

	ApplyFrictionToDeltas(1200.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*fps;			// add gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* SEE IF DONE */

	theNode->ButtTimer -= gFramesPerSecondFrac;
	if (theNode->ButtTimer <= 0.0)
	{
		MorphToSkeletonAnim(theNode->Skeleton, FLEA_ANIM_STAND, 2);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;

	UpdateFlea(theNode);
}




/********************** MOVE FLEA: DEATH ******************************/

static void  MoveFlea_Death(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF GONE */

	if (theNode->StatusBits & STATUS_BIT_ISCULLED)		// if was culled on last frame and is far enough away, then delete it
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) > 1000.0f)
		{
			DeleteEnemy(theNode);
			return;
		}
	}


	if (theNode->StatusBits & STATUS_BIT_ONGROUND)		// if on ground, add friction
		ApplyFrictionToDeltas(1200.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*fps;		// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES, true))
		return;


				/* UPDATE */

	UpdateFlea(theNode);


}




/***************** UPDATE FLEA ************************/

static void UpdateFlea(ObjNode *theNode)
{
	UpdateEnemy(theNode);

	UpdateFleaCap(theNode);

	UpdateFleaEyes(theNode);
}


/****************** UPDATE FLEA EYES *********************/

static void UpdateFleaEyes(ObjNode *theNode)
{
short			i;
float			r,aimX,aimZ;
static const OGLPoint3D	leftEye = {-10,1,-40};
static const OGLPoint3D	rightEye = {10,1,-40};
OGLMatrix4x4	m;



		/***********************/
		/* UPDATE EYE SPARKLES */
		/***********************/

	r = theNode->Rot.y;
	aimX = -sin(r);
	aimZ = -cos(r);

	FindJointFullMatrix(theNode,FLEA_JOINTNUM_HEAD,&m);						// get head matrix


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


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME FLEA ENEMY *************************/

Boolean PrimeEnemy_Flea(int splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE FLEA */

	newObj = MakeFlea(x,z, FLEA_ANIM_WALK);

	newObj->Skeleton->AnimSpeed = 1.5f;


				/* SET BETTER INFO */

	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveFleaOnSpline;					// set move call

	newObj->Coord.y 		-= newObj->BottomOff;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE FLEA ON SPLINE ***************************/

static void MoveFleaOnSpline(ObjNode *theNode)
{
Boolean isInRange;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 110);
	GetObjectCoordOnSpline(theNode);


			/* UPDATE STUFF IF IN RANGE */

	if (isInRange)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// calc y coord
		UpdateObjectTransforms(theNode);															// update transforms
		UpdateShadow(theNode);

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < FLEA_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveFlea);

		UpdateFleaCap(theNode);
		UpdateFleaEyes(theNode);
	}
}



#pragma mark -


/************************* FLEA GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

static void FleaGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	PlayEffect3D(EFFECT_FLYGOTKICKED, &kickedObj->Coord);

	kickedObj->Delta.x = -sin(r) * 800.0f;
	kickedObj->Delta.z = -cos(r) * 800.0f;
	kickedObj->Delta.y = 600.0f;

	HurtFlea(kickedObj, .3);
}




/*********************** HURT FLEA ***************************/

static Boolean HurtFlea(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveFlea);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillFlea(enemy);
		return(true);
	}
	else
	{
		MorphToSkeletonAnim(enemy->Skeleton, FLEA_ANIM_GOTHIT, 10);
		enemy->ButtTimer = 2.5;
	}

	return(false);
}


/****************** KILL FLEA ***********************/

static void KillFlea(ObjNode *enemy)
{
	enemy->CType 				= CTYPE_MISC;				// no longer an enemy
	enemy->HurtCallback 		= nil;
	enemy->GotKickedCallback 	= nil;

	MorphToSkeletonAnim(enemy->Skeleton, FLEA_ANIM_DEATH, 4);

	enemy->TerrainItemPtr = nil;			// dont ever come back
	gNumKilledFleas++;


			/* SEE IF TALK ON FIDO LEVEL */

	if (gLevelNum == LEVEL_NUM_FIDO)
	{
		if (gNumKilledFleas >= gTotalFleas)
		{
			if (gNumKilledTicks < gTotalTicks)									// see if that was all the fleas but still have ticks
				DoDialogMessage(DIALOG_MESSAGE_GOTFLEAS, 1, 8.0, nil);
			else																// all ticks and fleas dead, so level done
			{
				StartLevelCompletion(4.0);
				DoDialogMessage(DIALOG_MESSAGE_HAPPYDOG, 1, 4.0, nil);
			}
		}
	}

}


#pragma mark -


/***************************** GIVE FLEA A BOTTLE CAP *********************************/

static void GiveFleaACap(ObjNode *enemy)
{
ObjNode	*cap;

			/* MAKE CAP OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_BottleCap;
	gNewObjectDefinition.coord		= enemy->Coord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= enemy->Slot+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= enemy->Scale.x * .9f;
	cap = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			/* ATTACH CAP TO ENEMY */

	enemy->ChainNode = cap;
	cap->ChainHead = enemy;

	enemy->HasCap = true;
}


/************************* UPDATE FLEA CAP ************************/
//
// Updated when is being held by flea.
//

static void UpdateFleaCap(ObjNode *flea)
{
ObjNode					*cap;
OGLMatrix4x4			m,m2,mst,rm;
static const OGLPoint3D	zero = {0,0,0};
float					scale;

			/* VERIFY */

	if (!flea->HasCap)
		return;
	if (!flea->ChainNode)
		return;

	cap = flea->ChainNode;


			/* CALC SCALE MATRIX */

	scale = cap->Scale.x / flea->Scale.x;					// to adjust from enemy's scale to cap's scale
	OGLMatrix4x4_SetScale(&mst, scale, scale, scale);

			/* CALC TRANSLATE MATRIX */

	mst.value[M03] = -17;												// set offset for hand
	mst.value[M13] = 0;
	mst.value[M23] = -12;


			/* CALC ROTATE MATRIX */

	OGLMatrix4x4_SetRotate_XYZ(&rm, .4, 0, .5); 					// set rotation to fit in hand
	OGLMatrix4x4_Multiply(&mst, &rm, &m2);

			/* GET ALIGNMENT MATRIX */

	FindJointFullMatrix(flea, FLEA_JOINTNUM_RIGHTHAND, &m);
	OGLMatrix4x4_Multiply(&m2, &m, &cap->BaseTransformMatrix);
	SetObjectTransformMatrix(cap);


			/* SET REAL POINT FOR CULLING */

	OGLPoint3D_Transform(&zero, &cap->BaseTransformMatrix, &cap->Coord);
}


/********************* THROW CAP ************************/

static void FleaThrowCap(ObjNode *enemy)
{
ObjNode		 			*cap;
static const OGLPoint3D zero = {0,0,0};
float					rot;

	enemy->HasCap = false;							// dont have it anymore

	cap = enemy->ChainNode;						// get spear obj
	if (cap == nil)
		return;


		/* SETUP NEW LINKS TO REMEMBER CAP */

	enemy->ThrownCap = cap;		// remember the ObjNode to the spear so I can go get it
	cap->CapOwner = enemy;		// remember node of ant


		/* DETACH FROM CHAIN */

	enemy->ChainNode = nil;
	cap->ChainHead = nil;
	cap->MoveCall = MoveFleaCap;


			/* CALC THROW START COORD */

	OGLPoint3D_Transform(&zero, &cap->BaseTransformMatrix, &cap->Coord);


		/* CALC THROW VECTOR */

	rot = enemy->Rot.y;
	cap->Delta.x = -sin(rot) * CAP_SPEED;
	cap->Delta.z = -cos(rot) * CAP_SPEED;
	cap->Delta.y = 0;


			/* GIVE CAP COLLISION INFO */

	cap->CType  = CTYPE_HURTME;
	cap->CBits 	= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(cap,1,1);
	cap->Damage = CAP_DAMAGE;


	enemy->Mode = FLEA_MODE_WATCHCAP;			// the ant is watching the spear go


			/* MAKE A SHADOW */

	AttachShadowToObject(cap, SHADOW_TYPE_CIRCULAR, 3, 3, false);


	PlayEffect_Parms3D(EFFECT_THROWBOTTLECAP, &cap->Coord, NORMAL_CHANNEL_RATE, 1.4);

}



/******************* MOVE FLEA CAP ****************/

static void MoveFleaCap(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
uint16_t	sides;

			/* SEE IF ITS STILL AROUND */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);


				/* MOVE IT */

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// do friction if on ground
		ApplyFrictionToDeltas(1000.0,&gDelta);
	else
		theNode->Rot.y -= fps * 13.0f;						// spin when in air


	gDelta.y -= 600.0f * fps;						// gravity
	gCoord.x += gDelta.x * fps;						// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);
	if (theNode->Speed2D < 10.0f)
	{
		theNode->CType = CTYPE_MISC;			// don't hurt once stopped
	}




				/* COLLISION */

	sides = HandleCollisions(theNode, CTYPE_MISC | CTYPE_FENCE | CTYPE_TERRAIN, .5);
	if (sides && (sides != theNode->PrevSides))							// if hit something new then make a sound
	{
		PlayEffect_Parms3D(EFFECT_BOTTLECAPBOUNCE, &theNode->Coord, NORMAL_CHANNEL_RATE, .3);
	}
	theNode->PrevSides = sides;


	UpdateObject(theNode);
}


/************************ SEE IF FLEA ATTACK ****************************/

static Boolean SeeIfFleaAttack(ObjNode *theNode, float angleToTarget, float dist)
{
	if ((angleToTarget < CAP_THROW_MIN_ANGLE) && (dist < CAP_ATTACK_DIST))
	{
		if (!SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))	// dont attack thru things
		{
			MorphToSkeletonAnim(theNode->Skeleton, FLEA_ANIM_THROW, 5);
			theNode->ThrowCap = false;
			return(true);
		}
	}
	return(false);
}



#pragma mark -


/**************************** COUNT FLEAS *********************************/

void CountFleas(void)
{
int						i;
TerrainItemEntryType 	*itemPtr;

	gTotalFleas = 0;

	itemPtr = *gMasterItemList; 											// get pointer to data inside the LOCKED handle

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_FLEA)						// see if it's a Squish Berry item
			gTotalFleas++;
	}

	gNumKilledFleas = 0;
}






