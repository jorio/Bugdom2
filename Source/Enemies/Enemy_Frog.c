/****************************/
/*  	 ENEMY: FROG.C		*/
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

static ObjNode *MakeFrog(float x, float z, short animNum);
static void MoveFrog(ObjNode *theNode);
static void  MoveFrog_Stand(ObjNode *theNode);
static void  MoveFrog_JumpTongue(ObjNode *theNode);
static void  MoveFrog_JumpUp(ObjNode *theNode);
static void UpdateFrog(ObjNode *theNode);
static void  MoveFrog_Fall(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_FROGS				8

#define	FROG_SCALE				2.0f

#define	FROG_CHASE_DIST_MAX		1100.0f

#define	FROG_ATTACK_DIST		400.0f

#define FROG_TURN_SPEED			6.0f

#define	FROG_HEALTH				1.1f
#define	FROG_DAMAGE				.2f

#define	FROG_JUMP_DY			4000.0f



/*********************/
/*    VARIABLES      */
/*********************/

#define	 JumpNow			Flag[0]

#define	ButtTimer			SpecialF[1]
#define	RippleTimer			SpecialF[0]


/************************ ADD FROG ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Frog(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;

	if (gLevelNum == LEVEL_NUM_PARK)							// see if for park level
		return(AddEnemy_Frog2(itemPtr,x,z));

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_FROG] >= MAX_FROGS)
			return(false);
	}

	newObj = MakeFrog(x, z, FROG_ANIM_STAND);

	newObj->TerrainItemPtr = itemPtr;

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_FROG]++;


	return(true);
}


/************************* MAKE FROG ****************************/

static ObjNode *MakeFrog(float x, float z, short animNum)
{
ObjNode	*newObj;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_FROG,animNum, x,z, FROG_SCALE, 0, MoveFrog);



				/* SET BETTER INFO */

	newObj->StatusBits |= STATUS_BIT_NOTEXTUREWRAP;

	newObj->Health 		= FROG_HEALTH;
	newObj->Damage 		= FROG_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_FROG;


				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULARDARK, 11, 11,false);


	return(newObj);

}


/********************* MOVE FROG **************************/

static void MoveFrog(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveFrog_Stand,
					MoveFrog_JumpUp,
					MoveFrog_JumpTongue,
					MoveFrog_Fall,
					nil,
					nil,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE FROG: STANDING ******************************/

static void  MoveFrog_Stand(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
static float	dist;

			/* MOVE */

	ApplyFrictionToDeltas(2000.0,&gDelta);
	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, FROG_TURN_SPEED, false);

	gDelta.y -= ENEMY_GRAVITY*fps;									// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


			/* SEE IF JUMP */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if (dist < FROG_CHASE_DIST_MAX)
	{
		MorphToSkeletonAnim(theNode->Skeleton, FROG_ANIM_JUMPUP, 5);
		theNode->JumpNow = false;
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


	UpdateFrog(theNode);
}


/********************** MOVE FROG: JUMP UP ******************************/

static void  MoveFrog_JumpUp(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	oldDY = gDelta.y;
float	y;

	if (theNode->JumpNow)
	{
		theNode->JumpNow = false;
		gDelta.y = FROG_JUMP_DY;

		if (gLevelNum == LEVEL_NUM_BALSA)
			PlayEffect3D(EFFECT_FROGJUMP, &gCoord);
		else
			PlayEffect3D(EFFECT_FROGJUMP2, &gCoord);

		if (GetWaterY(gCoord.x, gCoord.z, &y))					// make splash effect
			MakeSplash(gCoord.x, y, gCoord.z, 2.0);
	}


			/* MOVE */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, FROG_TURN_SPEED, false);

	gDelta.y -= 5000.0f*fps;									// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

			/* IF @ APEX OF JUMP THEN TONGUE */

	if ((oldDY > 600.0f) && (gDelta.y <= 600.0f))
	{
		MorphToSkeletonAnim(theNode->Skeleton, FROG_ANIM_JUMPTONGUE, 6);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateFrog(theNode);
}


/********************** MOVE FROG: JUMP TONGUE ******************************/

static void  MoveFrog_JumpTongue(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLPoint3D	tongueCoord;

			/* MOVE */

	gDelta.y -= 4000.0f*fps;									// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		MorphToSkeletonAnim(theNode->Skeleton, FROG_ANIM_STAND, 7);
	else
	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton, FROG_ANIM_FALL, 5);


			/* SEE IF TONGUE HIT PLAYER */

	FindCoordOfJoint(theNode, FROG_JOINT_TONGUE, &tongueCoord);						// get coord of tongue tip
	if (OGLPoint3D_Distance(&tongueCoord, &gPlayerInfo.coord) < 400.0f)				// see if hit player
	{
		if (gLevelNum == LEVEL_NUM_BALSA)
			PlayerGotHit(theNode, 0, 0);

	}

	UpdateFrog(theNode);
}


/********************** MOVE FROG: FALL ******************************/

static void  MoveFrog_Fall(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* MOVE */

	gDelta.y -= 5000.0f*fps;									// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

				/* SEE IF LANDED */

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		float	y;

		MorphToSkeletonAnim(theNode->Skeleton, FROG_ANIM_STAND, 7);
		if (GetWaterY(gCoord.x, gCoord.z, &y))
			MakeSplash(gCoord.x, y, gCoord.z, 2.0);

	}

	UpdateFrog(theNode);
}





/***************** UPDATE FROG ************************/

static void UpdateFrog(ObjNode *theNode)
{

	UpdateEnemy(theNode);

}























