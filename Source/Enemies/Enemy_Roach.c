/****************************/
/*   ENEMY: ROACH.C	*/
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

static ObjNode *MakeRoach(float x, float z, short animNum);
static void MoveRoach(ObjNode *theNode);
static void MoveRoach_Standing(ObjNode *theNode);
static void  MoveRoach_WalkSpear(ObjNode *theNode);
static void  MoveRoach_WalkEmpty(ObjNode *theNode);
static void  MoveRoach_PickUp(ObjNode *theNode);
static void  MoveRoach_Death(ObjNode *theNode);
static void MoveRoachOnSpline(ObjNode *theNode);
static void UpdateRoach(ObjNode *theNode);
static void  MoveRoach_GotHit(ObjNode *theNode);
static Boolean HurtRoach(ObjNode *enemy, float damage);
static void  MoveRoach_Throw(ObjNode *theNode);
static Boolean SeeIfRoachAttack(ObjNode *theNode, float angleToTarget, float dist);

static void KillRoach(ObjNode *enemy);
static void RoachGotKickedCallback(ObjNode *player, ObjNode *kickedObj);

static void GiveRoachASpear(ObjNode *enemy);
static void UpdateRoachSpear(ObjNode *roach);
static void RoachThrowSpear(ObjNode *theEnemy);
static void MoveRoachSpear(ObjNode *theNode);

/****************************/
/*    CONSTROACHS             */
/****************************/

#define	MAX_ROACHS				7

#define	ROACH_SCALE				1.7f

#define	ROACH_DETACH_DIST		500.0f

#define	ROACH_CHASE_DIST_MAX	900.0f
#define	ROACH_CHASE_DIST_MIN	300.0f

#define	ROACH_TARGET_OFFSET		20.0f

#define ROACH_TURN_SPEED			20.0f
#define ROACH_WALK_SPEED			450.0f

#define	ROACH_HEALTH				1.1f
#define	ROACH_DAMAGE				.2f


		/* ANIMS */

enum
{
	ROACH_ANIM_STAND,
	ROACH_ANIM_WALKSPEAR,
	ROACH_ANIM_THROW,
	ROACH_ANIM_PICKUP,
	ROACH_ANIM_GOTHIT,
	ROACH_ANIM_DEATH,
	ROACH_ANIM_WALKEMPTY
};


#define	WALK_ANIM_SPEED					1.8f

#define	ROACH_JOINTNUM_RIGHTHAND		19
#define	ROACH_JOINTNUM_HEAD				0



		/* MODES */

enum
{
	ROACH_MODE_NONE,
	ROACH_MODE_WATCHSPEAR,
	ROACH_MODE_GETSPEAR
};


		/* SPEAR */

#define	SPEAR_ATTACK_DIST				500.0f
#define	SPEAR_THROW_MIN_ANGLE			0.03f
#define	DIST_TO_RETRIVE_SPEAR			140.0f
#define	SPEAR_DAMAGE					.2f



/*********************/
/*    VARIABLES      */
/*********************/

#define	ButtTimer			SpecialF[2]


#define	ThrowSpear			Flag[0]				// set by animation when spear should be thrown
#define PickUpNow			Flag[0]				// set by anim when pickup should occur

#define	HasSpear			Flag[1]				// true if this guy has a bottle spear
#define	ThrownSpear			SpecialObjPtr[0]	// objnode of thrown spear


		/* SPEAR */

#define SpearOwner			SpecialObjPtr[0]	// objnode of ant who threw spear
#define	PrevSides			Special[1]


/************************ ADD ROACH ENEMY *************************/

Boolean AddEnemy_Roach(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_ROACH] >= MAX_ROACHS)
			return(false);
	}

	newObj = MakeRoach(x, z, ROACH_ANIM_STAND);

	newObj->TerrainItemPtr = itemPtr;

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_ROACH]++;


	return(true);
}


/************************* MAKE ROACH ****************************/

static ObjNode *MakeRoach(float x, float z, short animNum)
{
ObjNode	*newObj;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_ROACH,animNum, x,z, ROACH_SCALE, 0, MoveRoach);



				/* SET BETTER INFO */

	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync

	newObj->StatusBits |= STATUS_BIT_NOTEXTUREWRAP;

	newObj->Health 		= ROACH_HEALTH;
	newObj->Damage 		= ROACH_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_ROACH;
	newObj->Mode		= ROACH_MODE_NONE;


				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, .7,1);
	CalcNewTargetOffsets(newObj,ROACH_TARGET_OFFSET);


	newObj->HurtCallback 		= HurtRoach;							// set hurt callback function
	newObj->GotKickedCallback 	= RoachGotKickedCallback;			// set callback for being kicked

	newObj->Damage = ROACH_DAMAGE;


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 4, 6,false);


		/* GIVE THE ROACH A BOTTLE SPEAR */

	GiveRoachASpear(newObj);

#if 0
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
#endif

	return(newObj);

}





/********************* MOVE ROACH **************************/

static void MoveRoach(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveRoach_Standing,
					MoveRoach_WalkSpear,
					MoveRoach_Throw,
					MoveRoach_PickUp,
					MoveRoach_GotHit,
					MoveRoach_Death,
					MoveRoach_WalkEmpty,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE ROACH: STANDING ******************************/

static void  MoveRoach_Standing(ObjNode *theNode)
{
float	angleToTarget;
ObjNode	*spear;
float	dist;

	switch(theNode->Mode)
	{
				/***********************************/
				/* HANDLE WAITING FOR SPEAR TO HIT */
				/***********************************/

		case	ROACH_MODE_WATCHSPEAR:

						/* VERIFY SPEAR */

				spear = theNode->ThrownSpear;												// get objnode of spear
				if ((spear->SpearOwner != theNode) || (spear->CType == INVALID_NODE_FLAG))			// see if isnt valid anymore
				{
					theNode->Mode = ROACH_MODE_NONE;
					break;
				}

					/* SEE IF SPEAR HAS IMPACTED */

				if (spear->StatusBits & STATUS_BIT_ONGROUND)
				{
					theNode->Mode = ROACH_MODE_GETSPEAR;
					MorphToSkeletonAnim(theNode->Skeleton, ROACH_ANIM_WALKEMPTY, 8);
				}
				break;


				/**************************/
				/* HANDLE ALL OTHER MODES */
				/**************************/

		default:

							/* TURN TOWARDS ME */

				angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, ROACH_TURN_SPEED, false);
				dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);		// calc dist to player

				if (!gGamePrefs.kiddieMode)
				{
							/* SEE IF THROW SPEAR */

					if (SeeIfRoachAttack(theNode, angleToTarget, dist))
						break;


						/* SEE IF START CHASING PLAYER */

					if ((dist < ROACH_CHASE_DIST_MAX) && (dist > ROACH_CHASE_DIST_MIN))
					{
						gDelta.x = gDelta.z = 0;

						MorphToSkeletonAnim(theNode->Skeleton, ROACH_ANIM_WALKSPEAR, 5);
					}
				}
	}

				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateRoach(theNode);
}




/********************** MOVE ROACH: WALKING WITH SPEAR ******************************/

static void  MoveRoach_WalkSpear(ObjNode *theNode)
{
float		r,fps,aim,dist;

	theNode->Skeleton->AnimSpeed = WALK_ANIM_SPEED;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	aim = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, ROACH_TURN_SPEED, false);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * ROACH_WALK_SPEED;
	gDelta.z = -cos(r) * ROACH_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* SEE IF STAND */

	dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	if ((dist > ROACH_CHASE_DIST_MAX) || (dist < ROACH_CHASE_DIST_MIN) || gGamePrefs.kiddieMode)
	{
		MorphToSkeletonAnim(theNode->Skeleton, ROACH_ANIM_STAND, 2.0);
	}

	SeeIfRoachAttack(theNode, aim, dist);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	UpdateRoach(theNode);
}




/********************** MOVE ROACH: WALKING EMPTY ******************************/

static void  MoveRoach_WalkEmpty(ObjNode *theNode)
{
float		r,fps;
ObjNode		*spear;

	theNode->Skeleton->AnimSpeed = WALK_ANIM_SPEED;

	fps = gFramesPerSecondFrac;

			/* VERIFY SPEAR */

	spear = theNode->ThrownSpear;												// get objnode of spear
	if ((spear->SpearOwner != theNode) || (spear->CType == INVALID_NODE_FLAG))			// see if isnt valid anymore
	{
		theNode->Mode = ROACH_MODE_NONE;
	}

			/* MOVE TOWARD SPEAR */

	TurnObjectTowardTarget(theNode, &gCoord, spear->Coord.x, spear->Coord.z, ROACH_TURN_SPEED, false);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * ROACH_WALK_SPEED;
	gDelta.z = -cos(r) * ROACH_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* SEE IF GET SPEAR */

	if (CalcQuickDistance(gCoord.x, gCoord.z, spear->Coord.x, spear->Coord.z) < DIST_TO_RETRIVE_SPEAR)
	{
		theNode->Mode = ROACH_MODE_NONE;
		theNode->PickUpNow = false;
		MorphToSkeletonAnim(theNode->Skeleton, ROACH_ANIM_PICKUP, 4);
	}


			/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	UpdateRoach(theNode);
}



/********************** MOVE ROACH: THROW ******************************/

static void  MoveRoach_Throw(ObjNode *theNode)
{
float	angleToTarget;

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, ROACH_TURN_SPEED, false);

			/*************************/
			/* SEE IF LAUNCH THE SPEAR */
			/*************************/

	if (theNode->ThrowSpear)
	{
		theNode->ThrowSpear = false;

				/* MAKE SURE STILL AIMED AT ME */

		if (angleToTarget < SPEAR_THROW_MIN_ANGLE)
			RoachThrowSpear(theNode);
	}


			/* SEE IF DONE WITH ANIM */

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, ROACH_ANIM_STAND, 5);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateRoach(theNode);
}


/********************** MOVE ROACH: PICKUP ******************************/

static void  MoveRoach_PickUp(ObjNode *theNode)
{
ObjNode	*spearObj;

	gDelta.x = gDelta.z = 0;						// make sure not moving during this anim

			/**********************/
			/* SEE IF GET THE SPEAR */
			/**********************/

	if (theNode->PickUpNow)
	{
		theNode->PickUpNow = false;

				/* VERIFY SPEAR */

		spearObj = theNode->ThrownSpear;													// get objnode of spear
		if ((spearObj->SpearOwner == theNode) && (spearObj->CType != INVALID_NODE_FLAG))	// make sure spear obj is valid
			DeleteObject(spearObj);																// delete the old spear

		GiveRoachASpear(theNode);																// give it a new spear (even if old is invalid)
	}


			/* SEE IF DONE WITH ANIM */

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, ROACH_ANIM_STAND, 5);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateRoach(theNode);
}


/********************** MOVE ROACH: GOT HIT ******************************/

static void  MoveRoach_GotHit(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	ApplyFrictionToDeltas(1200.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*fps;			// add gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* SEE IF DONE */

	theNode->ButtTimer -= gFramesPerSecondFrac;
	if (theNode->ButtTimer <= 0.0)
	{
		if (theNode->ChainNode)								// if has spear then stand
			MorphToSkeletonAnim(theNode->Skeleton, ROACH_ANIM_STAND, 2);
		else
			MorphToSkeletonAnim(theNode->Skeleton, ROACH_ANIM_WALKEMPTY, 4);		// otherwise go get spear
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateRoach(theNode);
}




/********************** MOVE ROACH: DEATH ******************************/

static void  MoveRoach_Death(ObjNode *theNode)
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

	UpdateRoach(theNode);


}




/***************** UPDATE ROACH ************************/

static void UpdateRoach(ObjNode *theNode)
{
short			i;
float			r,aimX,aimZ;
static const OGLPoint3D	leftEye = {-10,1,-40};
static const OGLPoint3D	rightEye = {10,1,-40};
OGLMatrix4x4	m;


	UpdateEnemy(theNode);

	UpdateRoachSpear(theNode);


		/***********************/
		/* UPDATE EYE SPARKLES */
		/***********************/

	r = theNode->Rot.y;
	aimX = -sin(r);
	aimZ = -cos(r);

	FindJointFullMatrix(theNode,ROACH_JOINTNUM_HEAD,&m);						// get head matrix


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

/************************ PRIME ROACH ENEMY *************************/

Boolean PrimeEnemy_Roach(int splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&gSplineList[splineNum], placement, &x, &z);


				/* MAKE ROACH */

	newObj = MakeRoach(x,z, ROACH_ANIM_WALKSPEAR);

	newObj->Skeleton->AnimSpeed = WALK_ANIM_SPEED;


				/* SET BETTER INFO */

	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveRoachOnSpline;					// set move call

	newObj->Coord.y 		-= newObj->BottomOff;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE ROACH ON SPLINE ***************************/

static void MoveRoachOnSpline(ObjNode *theNode)
{
Boolean isInRange;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 180);
	GetObjectCoordOnSpline(theNode);


			/* UPDATE STUFF IF IN RANGE */

	if (isInRange)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// calc y coord
		UpdateObjectTransforms(theNode);															// update transforms
		UpdateShadow(theNode);

		UpdateRoachSpear(theNode);


				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < ROACH_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveRoach);

	}
}



#pragma mark -


/************************* ROACH GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

static void RoachGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	PlayEffect3D(EFFECT_FLYGOTKICKED, &kickedObj->Coord);
	PlayRumbleEffect(EFFECT_FLYGOTKICKED);

	kickedObj->Delta.x = -sin(r) * 800.0f;
	kickedObj->Delta.z = -cos(r) * 800.0f;
	kickedObj->Delta.y = 600.0f;

	HurtRoach(kickedObj, .3);
}




/*********************** HURT ROACH ***************************/

static Boolean HurtRoach(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveRoach);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillRoach(enemy);
		return(true);
	}
	else
	{
		MorphToSkeletonAnim(enemy->Skeleton, ROACH_ANIM_GOTHIT, 10);
		enemy->ButtTimer = 2.5;
	}

	return(false);
}


/****************** KILL ROACH ***********************/

static void KillRoach(ObjNode *enemy)
{
	enemy->CType 				= CTYPE_MISC;				// no longer an enemy
	enemy->HurtCallback 		= nil;
	enemy->GotKickedCallback 	= nil;

	MorphToSkeletonAnim(enemy->Skeleton, ROACH_ANIM_DEATH, 4);

	enemy->TerrainItemPtr = nil;			// dont ever come back

}


#pragma mark -


/***************************** GIVE ROACH A SPEAR *********************************/

static void GiveRoachASpear(ObjNode *enemy)
{
ObjNode	*spear;

			/* MAKE SPEAR OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_RoachSpear;
	gNewObjectDefinition.coord		= enemy->Coord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= enemy->Slot+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= enemy->Scale.x * .9f;
	spear = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			/* ATTACH SPEAR TO ENEMY */

	enemy->ChainNode = spear;
	spear->ChainHead = enemy;

	enemy->HasSpear = true;
}


/************************* UPDATE ROACH SPEAR ************************/
//
// Updated when is being held by roach.
//

static void UpdateRoachSpear(ObjNode *roach)
{
ObjNode					*spear;
OGLMatrix4x4			m,m2,mst,rm;
static const OGLPoint3D	zero = {0,0,0};
float					scale;

			/* VERIFY */

	if (!roach->HasSpear)
		return;
	if (!roach->ChainNode)
		return;

	spear = roach->ChainNode;


			/* CALC SCALE MATRIX */

	scale = spear->Scale.x / roach->Scale.x;					// to adjust from enemy's scale to spear's scale
	OGLMatrix4x4_SetScale(&mst, scale, scale, scale);

			/* CALC TRANSLATE MATRIX */

	mst.value[M03] = 1;												// set offset for hand
	mst.value[M13] = 8;
	mst.value[M23] = 0;


			/* CALC ROTATE MATRIX */

	OGLMatrix4x4_SetRotate_XYZ(&rm, 4.3, .4, 0); 					// set rotation to fit in hand
	OGLMatrix4x4_Multiply(&mst, &rm, &m2);

			/* GET ALIGNMENT MATRIX */

	FindJointFullMatrix(roach, ROACH_JOINTNUM_RIGHTHAND, &m);
	OGLMatrix4x4_Multiply(&m2, &m, &spear->BaseTransformMatrix);
	SetObjectTransformMatrix(spear);


			/* SET REAL POINT FOR CULLING */

	OGLPoint3D_Transform(&zero, &spear->BaseTransformMatrix, &spear->Coord);
}


/********************* THROW SPEAR ************************/

static void RoachThrowSpear(ObjNode *enemy)
{
ObjNode		 			*spear;
static const OGLPoint3D zero = {0,0,0};
float					rot, dist;
OGLMatrix4x4			m;


	enemy->HasSpear = false;							// dont have it anymore

	spear = enemy->ChainNode;						// get spear obj
	if (spear == nil)
		return;


		/* SETUP NEW LINKS TO REMEMBER SPEAR */

	enemy->ThrownSpear = spear;		// remember the ObjNode to the spear so I can go get it
	spear->SpearOwner = enemy;		// remember node of ant


		/* DETACH FROM CHAIN */

	enemy->ChainNode = nil;
	spear->ChainHead = nil;
	spear->MoveCall = MoveRoachSpear;


			/* CALC THROW START COORD */

	FindJointMatrixAtFlagEvent(enemy, ROACH_JOINTNUM_RIGHTHAND, 0, &m);
	OGLPoint3D_Transform(&zero, &m, &spear->Coord);


		/* CALC THROW VECTOR */

	dist = CalcQuickDistance(spear->Coord.x, spear->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	dist *= 2.0f;
	if (dist < 100.0f)								// keep @ minimum
		dist = 100.0f;

	rot = enemy->Rot.y;
	spear->Delta.x = -sin(rot) * dist;
	spear->Delta.z = -cos(rot) * dist;
	spear->Delta.y = 0;


			/* GIVE SPEAR COLLISION INFO */

	spear->CType  = CTYPE_HURTME;
	spear->CBits 	= CBITS_ALLSOLID;

	SetObjectCollisionBounds(spear, 40, -40, -40, 40, 40, -40);
	spear->Damage = SPEAR_DAMAGE;

	spear->Rot.x = .5f;
	spear->Rot.y = rot;
	spear->Rot.z = 0;


	enemy->Mode = ROACH_MODE_WATCHSPEAR;			// the ant is watching the spear go


			/* MAKE A SHADOW */

	AttachShadowToObject(spear, SHADOW_TYPE_CIRCULAR, .6, 3.5, false);


	PlayEffect_Parms3D(EFFECT_THROWBOTTLECAP, &spear->Coord, NORMAL_CHANNEL_RATE, 1.4);

}



/******************* MOVE ROACH SPEAR ****************/

static void MoveRoachSpear(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	y;

			/* SEE IF ITS STILL AROUND */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);


				/* MOVE IT */

	gDelta.y -= 600.0f * fps;						// gravity
	gCoord.x += gDelta.x * fps;						// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);
	if (theNode->Speed2D < 10.0f)
	{
		theNode->CType = CTYPE_MISC;			// don't hurt once stopped
	}


	theNode->Rot.x += fps * -1.5f;


				/* COLLISION */

	HandleCollisions(theNode, CTYPE_MISC | CTYPE_FENCE, 0);

	y = GetTerrainY(gCoord.x, gCoord.z);

	if (gCoord.y <= y)									// see if stuck in ground
	{
		gCoord.y = y;
		theNode->StatusBits |= STATUS_BIT_ONGROUND;
		theNode->CType = CTYPE_MISC;
		theNode->MoveCall = MoveStaticObject;
	}

	UpdateObject(theNode);
}


/************************ SEE IF ROACH ATTACK ****************************/

static Boolean SeeIfRoachAttack(ObjNode *theNode, float angleToTarget, float dist)
{
	if (!theNode->HasSpear)
		return(false);

	if ((angleToTarget < SPEAR_THROW_MIN_ANGLE) && (dist < SPEAR_ATTACK_DIST))
	{
		if (!SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))	// dont attack thru things
		{
			MorphToSkeletonAnim(theNode->Skeleton, ROACH_ANIM_THROW, 5);
			theNode->ThrowSpear = false;
			return(true);
		}
	}
	return(false);
}







