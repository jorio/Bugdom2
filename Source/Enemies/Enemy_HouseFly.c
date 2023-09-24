/****************************/
/*   ENEMY: HOUSEFLY.C	*/
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

static ObjNode *MakeHouseFly(float x, float z, short animNum);
static void MoveHouseFly(ObjNode *theNode);
static void MoveHouseFly_Standing(ObjNode *theNode);
static void  MoveHouseFly_Walk(ObjNode *theNode);
static void  MoveHouseFly_Flying(ObjNode *theNode);
static void  MoveHouseFly_Death(ObjNode *theNode);
static void MoveHouseFlyOnSpline(ObjNode *theNode);
static void UpdateHouseFly(ObjNode *theNode);
static Boolean HurtHouseFly(ObjNode *enemy, float damage);
static void  MoveHouseFly_Attack(ObjNode *theNode);
static void SeeIfHouseFlyAttack(ObjNode *theNode);

static void KillHouseFly(ObjNode *enemy);
static void HouseFlyGotKickedCallback(ObjNode *player, ObjNode *kickedObj);

static void StartChaseSpurt(ObjNode *enemy, float distToPlayer);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_HOUSEFLYS				8

#define	HOUSEFLY_SCALE				.8f

#define	HOUSEFLY_CHASE_DIST_MAX		2000.0f
#define	HOUSEFLY_CHASE_DIST_MIN		20.0f
#define	HOUSEFLY_DETACH_DIST		400.0f

#define	HOUSEFLY_ATTACK_DIST		100.0f

#define	HOUSEFLY_DAMAGE				.2f

#define	HOUSEFLY_TARGET_OFFSET		20.0f

#define HOUSEFLY_TURN_SPEED			20.0f
#define HOUSEFLY_WALK_SPEED			700.0f

#define	HOUSEFLY_HEALTH				1.0f
#define	HOUSEFLY_DAMAGE				.2f

		/* ANIMS */


enum
{
	HOUSEFLY_ANIM_STAND,
	HOUSEFLY_ANIM_WALK,
	HOUSEFLY_ANIM_FLY,
	HOUSEFLY_ANIM_DEATH,
	HOUSEFLY_ANIM_ATTACK
};




/*********************/
/*    VARIABLES      */
/*********************/

#define	SpurtTargetX		SpecialF[0]
#define	SpurtTargetZ		SpecialF[1]
#define	ButtTimer			SpecialF[2]
#define	BlinkTimer			SpecialF[3]

#define	AttackTimer			Timer
#define	AttackActive		Flag[0]


/************************ ADD HOUSEFLY ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_HouseFly(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_HOUSEFLY] >= MAX_HOUSEFLYS)
			return(false);
	}

	newObj = MakeHouseFly(x, z, HOUSEFLY_ANIM_STAND);

	newObj->TerrainItemPtr = itemPtr;

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_HOUSEFLY]++;


	return(true);
}


/************************* MAKE HOUSEFLY ****************************/

static ObjNode *MakeHouseFly(float x, float z, short animNum)
{
ObjNode	*newObj;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_HOUSEFLY,animNum, x,z, HOUSEFLY_SCALE, 0, MoveHouseFly);



				/* SET BETTER INFO */

	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync

	newObj->StatusBits |= STATUS_BIT_NOTEXTUREWRAP;

	newObj->Health 		= HOUSEFLY_HEALTH;
	newObj->Damage 		= HOUSEFLY_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_HOUSEFLY;


				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, .7,1);
	CalcNewTargetOffsets(newObj,HOUSEFLY_TARGET_OFFSET);


	newObj->HurtCallback 		= HurtHouseFly;							// set hurt callback function
	newObj->GotKickedCallback 	= HouseFlyGotKickedCallback;			// set callback for being kicked

	newObj->Timer = RandomFloat();

	newObj->Damage = HOUSEFLY_DAMAGE;


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 2, 2,false);


	return(newObj);

}





/********************* MOVE HOUSEFLY **************************/

static void MoveHouseFly(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveHouseFly_Standing,
					MoveHouseFly_Walk,
					MoveHouseFly_Flying,
					MoveHouseFly_Death,
					MoveHouseFly_Attack
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE HOUSEFLY: STANDING ******************************/

static void  MoveHouseFly_Standing(ObjNode *theNode)
{
float	dist;
float	fps = gFramesPerSecondFrac;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)									// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, HOUSEFLY_TURN_SPEED, false);

				/* SEE IF CHASE */

	if (!gGamePrefs.kiddieMode)
	{
		if (!IsWaterInFrontOfEnemy(theNode->Rot.y))				// dont chase if we're in front of water
		{
			theNode->Timer -= fps;															// see if time to spurt
			if (theNode->Timer <= 0.0f)
			{
				dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
				if ((dist < HOUSEFLY_CHASE_DIST_MAX) && (dist > HOUSEFLY_CHASE_DIST_MIN))
				{
					StartChaseSpurt(theNode, dist);
				}
			}
		}
	}
			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*fps;									// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	SeeIfHouseFlyAttack(theNode);

	UpdateHouseFly(theNode);
}




/********************** MOVE HOUSEFLY: WALKING ******************************/

static void  MoveHouseFly_Walk(ObjNode *theNode)
{
float		r,fps;

	theNode->Skeleton->AnimSpeed = 3.5f;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD SPURT TARGET */

	TurnObjectTowardTarget(theNode, &gCoord, theNode->SpurtTargetX, theNode->SpurtTargetZ, HOUSEFLY_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * HOUSEFLY_WALK_SPEED;
	gDelta.z = -cos(r) * HOUSEFLY_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	if (IsWaterInFrontOfEnemy(r))				// if about to enter water then stop
		goto do_stand;


				/* SEE IF DONE WITH SPURT */

	theNode->Timer -= fps;
	if (theNode->Timer <= 0.0f)
	{
do_stand:
		MorphToSkeletonAnim(theNode->Skeleton, HOUSEFLY_ANIM_STAND, 15);
		theNode->Timer = .5f + RandomFloat() * .5f;					// reset timer for waiting
		gDelta.x = gDelta.z = 0;
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	SeeIfHouseFlyAttack(theNode);

	UpdateHouseFly(theNode);
}





/********************** MOVE HOUSEFLY: FLYING ******************************/

static void  MoveHouseFly_Flying(ObjNode *theNode)
{


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateHouseFly(theNode);
}







/********************** MOVE HOUSEFLY: DEATH ******************************/

static void  MoveHouseFly_Death(ObjNode *theNode)
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

	UpdateHouseFly(theNode);


}


/********************** MOVE HOUSEFLY: ATTACK ******************************/

static void  MoveHouseFly_Attack(ObjNode *theNode)
{
float		fps,angle;

	fps = gFramesPerSecondFrac;


			/* MOVE TOWARD SPURT TARGET */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, HOUSEFLY_TURN_SPEED, false);


			/* MOVE */

	ApplyFrictionToDeltas(2000.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* SEE IF HIT PLAYER */

	if (theNode->AttackActive && (gPlayerInfo.invincibilityTimer <= 0.0f))
	{
		if (angle < (PI/3))																	// see if aimed @ player pretty much
		{
			if (OGLPoint3D_Distance(&gCoord, &gPlayerInfo.coord) < HOUSEFLY_ATTACK_DIST)	// and in range
			{
				ObjNode	*player = gPlayerInfo.objNode;
				float	r = theNode->Rot.y;													// get enemy rot

				PlayerGotHit(theNode, 0, PLAYER_ANIM_GOTHIT_BACKFLIP);


				if (theNode->Skeleton->AnimDirection == ANIM_DIRECTION_FORWARD)				// determine vector to smack player
					r -= PI/2;
				else
					r += PI/2;

				player->Delta.x = -sin(r) * 600.0f;
				player->Delta.z = -cos(r) * 600.0f;
				player->Delta.y = 600.0f;
//				gCurrentMaxSpeed = 600;

				PlayEffect3D(EFFECT_SMACK, &player->Coord);

			}
		}
	}


				/* SEE IF DONE */

	theNode->Timer -= fps;
	if (theNode->Timer <= 0.0f)
	{
		MorphToSkeletonAnim(theNode->Skeleton, HOUSEFLY_ANIM_STAND, 8);
		theNode->Timer = .5f + RandomFloat() * .5f;					// reset timer for waiting
		gDelta.x = gDelta.z = 0;
	}

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;




	UpdateHouseFly(theNode);
}





/***************** UPDATE HOUSEFLY ************************/

static void UpdateHouseFly(ObjNode *theNode)
{

	UpdateEnemy(theNode);


			/****************/
			/* UPDATE BLINK */
			/****************/

	if (theNode->Skeleton->AnimNum == HOUSEFLY_ANIM_DEATH)
		goto blinkon;

	theNode->BlinkTimer -= gFramesPerSecondFrac;
	if (theNode->BlinkTimer <= 0.0f)						// see if in blink
	{
		if (theNode->BlinkTimer < -0.1f)					// see if stop blink now
		{
			theNode->Skeleton->overrideTexture[0] = nil;
			theNode->BlinkTimer = .5f + RandomFloat() * 1.5f;
		}
		else
		{
blinkon:
			theNode->Skeleton->overrideTexture[0] = gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_HouseFlyBlink].materialObject;
		}
	}

}



#pragma mark -


/********************* START CHASE SPURT ************************/

static void StartChaseSpurt(ObjNode *enemy, float distToPlayer)
{

	MorphToSkeletonAnim(enemy->Skeleton, HOUSEFLY_ANIM_WALK, 15);

	enemy->Timer = .3f + RandomFloat() * .2f;

	if (distToPlayer > 2000.0f)
		distToPlayer = 2000.0f;
	else
	if (distToPlayer < 200.0f)							// when close enough, just home right in
		distToPlayer = 0.0f;


	CalcNewTargetOffsets(enemy,distToPlayer * .7f);		// gets more accurate as it gets closer to player

	enemy->SpurtTargetX = gPlayerInfo.coord.x;
	enemy->SpurtTargetZ = gPlayerInfo.coord.z;

	PlayEffect_Parms3D(EFFECT_FLYWALKBUZZ, &enemy->Coord, NORMAL_CHANNEL_RATE + (MyRandomLong()&0x3fff), .4f);
}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME HOUSEFLY ENEMY *************************/

Boolean PrimeEnemy_HouseFly(int splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE HOUSE FLY */

	newObj = MakeHouseFly(x,z, HOUSEFLY_ANIM_WALK);

	newObj->Skeleton->AnimSpeed = 2.5f;


				/* SET BETTER INFO */

	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveHouseFlyOnSpline;					// set move call

	newObj->Coord.y 		-= newObj->BottomOff;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE HOUSEFLY ON SPLINE ***************************/

static void MoveHouseFlyOnSpline(ObjNode *theNode)
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

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < HOUSEFLY_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveHouseFly);

	}
}



#pragma mark -


/************************* HOUSEFLY GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

static void HouseFlyGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	PlayEffect3D(EFFECT_FLYGOTKICKED, &kickedObj->Coord);

	kickedObj->Delta.x = -sin(r) * 800.0f;
	kickedObj->Delta.z = -cos(r) * 800.0f;
	kickedObj->Delta.y = 600.0f;

	HurtHouseFly(kickedObj, 1.5);
}




/*********************** HURT HOUSEFLY ***************************/

static Boolean HurtHouseFly(ObjNode *enemy, float damage)
{
	(void) damage;

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveHouseFly);


				/* HURT ENEMY & SEE IF KILL */

//	enemy->Health -= damage;
//	if (enemy->Health <= 0.0f)
	{
		KillHouseFly(enemy);
		return(true);
	}

	return(false);
}


/****************** KILL HOUSEFLY ***********************/

static void KillHouseFly(ObjNode *enemy)
{
	MorphToSkeletonAnim(enemy->Skeleton, HOUSEFLY_ANIM_DEATH, 4);
}




/**********************8 SEE IF HOUSE FLY ATTACK ******************************/

static void SeeIfHouseFlyAttack(ObjNode *theNode)
{
	if (gPlayerInfo.invincibilityTimer > 0.0f)				// dont attack if invincible
		return;

	if (OGLPoint3D_Distance(&gCoord, &gPlayerInfo.coord) < HOUSEFLY_ATTACK_DIST)
	{
		MorphToSkeletonAnim(theNode->Skeleton, HOUSEFLY_ANIM_ATTACK, 9);
		theNode->AttackTimer = 2.0f;
		theNode->AttackActive = false;

	}
}



