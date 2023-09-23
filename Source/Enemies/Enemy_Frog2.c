/****************************/
/*  	 ENEMY: FROG2.C		*/
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

static ObjNode *MakeFrog2(float x, float z, short animNum);
static void MoveFrog2(ObjNode *theNode);
static void  MoveFrog2_Stand(ObjNode *theNode);
static void  MoveFrog_Death(ObjNode *theNode);
static void UpdateFrog2(ObjNode *theNode);
static void  MoveFrog_GotHit(ObjNode *theNode);
static void  MoveFrog2_Fall(ObjNode *theNode);
static void  MoveFrog_SitTongue(ObjNode *theNode);
static void  MoveFrog_JumpForward(ObjNode *theNode);

static Boolean HurtFrog(ObjNode *enemy, float damage);

static void KillFrog(ObjNode *enemy);
static void FrogGotKickedCallback(ObjNode *player, ObjNode *kickedObj);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_FROG2S				8

#define	FROG2_SCALE				1.4f

#define	FROG2_ATTACK_DIST		400.0f

#define	FROG2_CHASE_DIST_MAX	1900.0f
#define	FROG2_CHASE_DIST_MIN	(FROG2_ATTACK_DIST + 150.0f)


#define FROG2_TURN_SPEED			1.5f

#define	FROG2_HEALTH				1.4f
#define	FROG2_DAMAGE				.22f

#define	FROG2_JUMP_DY			2500.0f

#define	FROG_COLLISION_CTYPE	(CTYPE_MISC|CTYPE_HURTENEMY|CTYPE_TRIGGER2|CTYPE_FENCE|CTYPE_WATER)

/*********************/
/*    VARIABLES      */
/*********************/

#define	 JumpNow			Flag[0]
#define	IsJumping			Flag[1]

#define	RippleTimer			SpecialF[0]

#define	JumpPause			SpecialF[2]				// delay before next jump
#define AttackPause			SpecialF[3]


/************************ ADD FROG2 ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Frog2(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_FROG] >= MAX_FROG2S)
			return(false);
	}

	newObj = MakeFrog2(x, z, FROG_ANIM_STAND);

	newObj->TerrainItemPtr = itemPtr;

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_FROG]++;


	return(true);
}


/************************* MAKE FROG2 ****************************/

static ObjNode *MakeFrog2(float x, float z, short animNum)
{
ObjNode	*newObj;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_FROG,animNum, x,z, FROG2_SCALE, 0, MoveFrog2);



				/* SET BETTER INFO */

	newObj->StatusBits |= STATUS_BIT_NOTEXTUREWRAP;

	newObj->Health 		= FROG2_HEALTH;
	newObj->Damage 		= FROG2_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_FROG;


				/* SET COLLISION INFO */

	newObj->ForceLookAtDist	= 800.0f;
	newObj->CType |= CTYPE_LOOKAT;

	CreateCollisionBoxFromBoundingBox(newObj, .7,.9);

	newObj->HurtCallback 		= HurtFrog;							// set hurt callback function
	newObj->GotKickedCallback 	= FrogGotKickedCallback;			// set callback for being kicked


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULARDARK, 9, 9, true);


	return(newObj);

}


/********************* MOVE FROG2 **************************/

static void MoveFrog2(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveFrog2_Stand,
					nil,
					nil,
					MoveFrog2_Fall,
					MoveFrog_JumpForward,
					MoveFrog2_Fall,
					MoveFrog_GotHit,
					MoveFrog_SitTongue,
					MoveFrog_Death,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE FROG2: STANDING ******************************/

static void  MoveFrog2_Stand(ObjNode *theNode)
{
float	aim;
float	fps = gFramesPerSecondFrac;
static float	dist;

			/* MOVE */

	ApplyFrictionToDeltas(4000.0,&gDelta);
	aim = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, FROG2_TURN_SPEED, false);

	gDelta.y -= ENEMY_GRAVITY*fps;									// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,FROG_COLLISION_CTYPE, false))
		return;


			/* SEE IF ATTACK OR JUMP */

	if (theNode->StatusBits & STATUS_BIT_UNDERWATER)				// if in water, immediately jump
		goto jumpnow;

	theNode->JumpPause -= fps;										// see if timer says ok to try
	theNode->AttackPause -= fps;

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);

	if (dist < FROG2_ATTACK_DIST)								// see if close enough for attack
	{
		if ((theNode->AttackPause < 0.0f) && (aim < (PI/7)))	// must be aiming at player for attack
		{
			MorphToSkeletonAnim(theNode->Skeleton, FROG_ANIM_SITATTACK, 8);
			theNode->AttackPause = 1.0f;
			PlayEffect3D(EFFECT_TONGUESWOOSH, &gCoord);
		}
	}
	else
	if ((dist < FROG2_CHASE_DIST_MAX) && (dist > FROG2_CHASE_DIST_MIN) && (theNode->JumpPause <= 0.0f))		// see if close enough to jump
	{
jumpnow:
		MorphToSkeletonAnim(theNode->Skeleton, FROG_ANIM_JUMPFORWARD, 5);
		theNode->JumpNow = false;
		theNode->IsJumping = false;
		theNode->JumpPause = 1.5;								// set delay until next jump can happen
	}



			/* MAKE WATER RIPPLES */

	if (!(theNode->StatusBits & STATUS_BIT_ISCULLED))				// no ripples if culled
	{
		if (IsXZOverWater(gCoord.x, gCoord.z))						// see if on water
		{
			theNode->RippleTimer -= fps;
			if (theNode->RippleTimer <= 0.0f)
			{
				theNode->RippleTimer = .2f + RandomFloat() * .3f;
				CreateNewRipple(gCoord.x + RandomFloat2() * 100.0f, gCoord.z + RandomFloat2() * 100.0f, RandomFloat() * 50.0f, 100.0f, .3);
			}
		}
	}


	UpdateFrog2(theNode);
}




/********************** MOVE FROG: JUMP FORWARD ******************************/

static void  MoveFrog_JumpForward(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	oldDY = gDelta.y;
float	r;


			/* SEE IF JUMP NOW */

	if (theNode->JumpNow)
	{
		theNode->JumpNow = false;
		gDelta.y = FROG2_JUMP_DY;

		r = theNode->Rot.y;
		gDelta.x = -sin(r) * 450.0f;
		gDelta.z = -cos(r) * 450.0f;

		PlayEffect3D(EFFECT_FROGJUMP2, &gCoord);

		theNode->IsJumping = true;
	}


			/* MOVE */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, FROG2_TURN_SPEED, false);

	gDelta.y -= 5000.0f*fps;									// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,FROG_COLLISION_CTYPE, false))
		return;


			/* IF @ APEX OF JUMP THEN FALL */

	if ((oldDY > 0.0f) && (gDelta.y <= 0.0f))
	{
		MorphToSkeletonAnim(theNode->Skeleton, FROG_ANIM_FALLFORWARD, 3);
		theNode->IsJumping = false;
	}
	else
	if (theNode->IsJumping && (theNode->StatusBits & STATUS_BIT_ONGROUND))
	{
		MorphToSkeletonAnim(theNode->Skeleton, FROG_ANIM_STAND, 4);
		theNode->IsJumping = false;
	}


	UpdateFrog2(theNode);
}


/********************** MOVE FROG: SIT TONGUE ******************************/

static void  MoveFrog_SitTongue(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLPoint3D	tongueCoord;
OGLMatrix4x4	m;

			/* MOVE */

	ApplyFrictionToDeltas(5000.0,&gDelta);

	gDelta.y -= 4000.0f*fps;									// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,FROG_COLLISION_CTYPE, false))
		return;

	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton, FROG_ANIM_STAND, 5);


			/****************************/
			/* SEE IF TONGUE HIT PLAYER */
			/****************************/

	FindCoordOfJoint(theNode, FROG_JOINT_TONGUE, &tongueCoord);						// get coord of tongue tip
	tongueCoord.y -= 20.0f;

				/* TRY 1 */

	if (SeeIfLineSegmentHitsObject(&tongueCoord, &gCoord, gPlayerInfo.objNode))		// see if line hits player
		goto hit_player;

				/* TRY 2 */

	OGLMatrix4x4_SetRotateAboutPoint(&m, &gCoord, 0, PI/10, 0);
	OGLPoint3D_Transform(&tongueCoord, &m, &tongueCoord);

	if (SeeIfLineSegmentHitsObject(&tongueCoord, &gCoord, gPlayerInfo.objNode))		// see if line hits player
		goto hit_player;

				/* TRY 3 */

	OGLMatrix4x4_SetRotateAboutPoint(&m, &gCoord, 0, -PI/5, 0);
	OGLPoint3D_Transform(&tongueCoord, &m, &tongueCoord);

	if (SeeIfLineSegmentHitsObject(&tongueCoord, &gCoord, gPlayerInfo.objNode))		// see if line hits player
	{
hit_player:
		if (gPlayerInfo.invincibilityTimer <= 0.0f)							// cant get hit if invincible
		{
			PlayerGotHit(theNode, 0, PLAYER_ANIM_GOTHIT_GENERIC);
			PlayEffect3D(EFFECT_TONGUEHIT, &gPlayerInfo.coord);
		}
	}




	UpdateFrog2(theNode);
}


/********************** MOVE FROG2: FALL ******************************/

static void  MoveFrog2_Fall(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* MOVE */

	gDelta.y -= 5000.0f*fps;									// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,FROG_COLLISION_CTYPE, false))
		return;

				/* SEE IF LANDED */

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		float	y;

		MorphToSkeletonAnim(theNode->Skeleton, FROG_ANIM_STAND, 7);
		if (GetWaterY(gCoord.x, gCoord.z, &y))
			MakeSplash(gCoord.x, y, gCoord.z, 2.0);

	}

	UpdateFrog2(theNode);
}


/********************** MOVE FROG2: GOT HIT ******************************/

static void  MoveFrog_GotHit(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	ApplyFrictionToDeltas(1000.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*fps;			// add gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;



				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,FROG_COLLISION_CTYPE, true))
		return;

				/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, FROG_ANIM_STAND, 3);
	}

	UpdateFrog2(theNode);
}


/********************** MOVE FROG2: DEATH ******************************/

static void  MoveFrog_Death(ObjNode *theNode)
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

	UpdateFrog2(theNode);


}



/***************** UPDATE FROG2 ************************/

static void UpdateFrog2(ObjNode *theNode)
{

	UpdateEnemy(theNode);

}



#pragma mark -




/************************* FROG2 GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

static void FrogGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	PlayEffect3D(EFFECT_FLYGOTKICKED, &kickedObj->Coord);

	kickedObj->Delta.x = -sin(r) * 800.0f;
	kickedObj->Delta.z = -cos(r) * 800.0f;
	kickedObj->Delta.y = 600.0f;

	HurtFrog(kickedObj, .5);
}




/*********************** HURT FROG2 ***************************/

static Boolean HurtFrog(ObjNode *enemy, float damage)
{

				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillFrog(enemy);
		return(true);
	}
	else
	{
		SetSkeletonAnim(enemy->Skeleton, FROG_ANIM_GOTHIT);
		enemy->AttackPause = 2.5;							// dont attack for a while after a hit
	}

	return(false);
}


/****************** KILL FROG2 ***********************/

static void KillFrog(ObjNode *enemy)
{
	enemy->CType 				= CTYPE_MISC;				// no longer an enemy
	enemy->HurtCallback 		= nil;
	enemy->GotKickedCallback 	= nil;

	SetSkeletonAnim(enemy->Skeleton, FROG_ANIM_DEATH);

	enemy->TerrainItemPtr = nil;			// dont ever come back

}






















