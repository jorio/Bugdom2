/****************************/
/*   ENEMY: GNOME.C			*/
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

static ObjNode *MakeGnome(float x, float z);
static void MoveGnome(ObjNode *theNode);
static void MoveGnome_Standing(ObjNode *theNode);
static void  MoveGnome_Walk(ObjNode *theNode);
static void MoveGnomeOnSpline(ObjNode *theNode);
static void UpdateGnome(ObjNode *theNode);
static void  MoveGnome_KneeHurt(ObjNode *theNode);
static void  MoveGnome_Clap(ObjNode *theNode);

static void GnomeGotKickedCallback(ObjNode *player, ObjNode *enemy);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_GNOMES					7

#define	GNOME_SCALE					5.5f

#define	GNOME_CHASE_DIST_MAX		1400.0f
#define	GNOME_DETACH_DIST			(GNOME_CHASE_DIST_MAX / 2.0f)
#define	GNOME_ATTACK_DIST			200.0f

#define	GNOME_TARGET_OFFSET			50.0f

#define GNOME_TURN_SPEED			2.0f
#define GNOME_WALK_SPEED			130.0f

#define	GNOME_HEALTH				1.0f

		/* ANIMS */


enum
{
	GNOME_ANIM_STAND,
	GNOME_ANIM_WALK,
	GNOME_ANIM_CLAP,
	GNOME_ANIM_KNEEHURT
};


enum
{
	GNOME_JOINT_LEFTHAND	=	14,
	GNOME_JOINT_RIGHTHAND	=	17
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	WaveSpacer			SpecialF[0]
#define	AttackDuration		SpecialF[1]
#define	DelayUntilAttack	SpecialF[2]
#define	ButtTimer			SpecialF[4]

#define	SlapRight			Flag[0]						// set to 0 if swinging left, 1 = swinging right


/************************ ADD GNOME ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Gnome(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_GNOME] >= MAX_GNOMES)
			return(false);
	}

	newObj = MakeGnome(x, z);

	newObj->TerrainItemPtr = itemPtr;

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_GNOME]++;

	return(true);
}

/************************* MAKE GNOME ****************************/

static ObjNode *MakeGnome(float x, float z)
{
ObjNode	*newObj;

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_GNOME, GNOME_ANIM_STAND, x,z, GNOME_SCALE, 0, MoveGnome);


				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= GNOME_HEALTH;
	newObj->Damage 		= .1;
	newObj->Kind 		= ENEMY_KIND_GNOME;

				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, .7,1);
	CalcNewTargetOffsets(newObj,GNOME_TARGET_OFFSET);

	newObj->GotKickedCallback 	= GnomeGotKickedCallback;			// set callback for being kicked



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8,false);

	return(newObj);

}





/********************* MOVE GNOME **************************/

static void MoveGnome(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveGnome_Standing,
					MoveGnome_Walk,
					MoveGnome_Clap,
					MoveGnome_KneeHurt,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE Gnome: STANDING ******************************/

static void  MoveGnome_Standing(ObjNode *theNode)
{
float	dist;
float	fps = gFramesPerSecondFrac;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)									// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* SEE IF CHASE */

	if (!gGamePrefs.kiddieMode)
	{
		dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
		if (dist <= GNOME_CHASE_DIST_MAX)
		{
			MorphToSkeletonAnim(theNode->Skeleton, GNOME_ANIM_WALK, 4);
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



	UpdateGnome(theNode);
}




/********************** MOVE GNOME: WALKING ******************************/

static void  MoveGnome_Walk(ObjNode *theNode)
{
float		r,fps,angle,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, GNOME_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * GNOME_WALK_SPEED;
	gDelta.z = -cos(r) * GNOME_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* SEE IF STAND */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);

	if ((dist > GNOME_CHASE_DIST_MAX) || gGamePrefs.kiddieMode)
	{
		MorphToSkeletonAnim(theNode->Skeleton, GNOME_ANIM_STAND, 4);
	}
	else
	if (dist <= GNOME_ATTACK_DIST)
	{
		MorphToSkeletonAnim(theNode->Skeleton, GNOME_ANIM_CLAP, 1);
		theNode->Timer = 1.0f;
	}

				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == GNOME_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = GNOME_WALK_SPEED * .005f;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateGnome(theNode);
}

/********************** MOVE GNOME: CLAP ******************************/

static void  MoveGnome_Clap(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	dist;

			/* MOVE */

	ApplyFrictionToDeltas(2000.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*fps;									// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


			/*************************/
			/* SEE IF DONE ATTACKING */
			/*************************/

	if (gPlayerInfo.invincibilityTimer <= 0.0f)
	{
		dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
		if (dist > GNOME_ATTACK_DIST)									// see if player out of attack range
		{
			theNode->Timer -= fps;
			if (theNode->Timer <= 0.0f)
			{
				MorphToSkeletonAnim(theNode->Skeleton, GNOME_ANIM_STAND, 5);
			}
		}

				/**************************/
				/* SEE IF HAND HIT PLAYER */
				/**************************/

		else
		if (!theNode->Skeleton->IsMorphing)						// no hand collision if morphing
		{
			static OGLPoint3D	tipOff = {0,-10,0};
			OGLPoint3D			hc;
			Boolean				hitPlayer = false;

					/* SEE IF RIGHT HAND HIT PLAYER */

			FindCoordOnJoint(theNode, GNOME_JOINT_RIGHTHAND, &tipOff, &hc);
			if (DoSimpleBoxCollisionAgainstPlayer(hc.y + 20.0f, hc.y - 20.0f, hc.x - 30.0f, hc.x + 30.0f, hc.z + 30.0f, hc.z - 30.0f))
			{

				PlayerGotHit(theNode, 0, PLAYER_ANIM_GOTHIT_BACKFLIP);
				hitPlayer = true;
			}
			else
			{

					/* SEE IF LEFT HAND HIT PLAYER */

				FindCoordOnJoint(theNode, GNOME_JOINT_LEFTHAND, &tipOff, &hc);
				if (DoSimpleBoxCollisionAgainstPlayer(hc.y + 20.0f, hc.y - 20.0f, hc.x - 30.0f, hc.x + 30.0f, hc.z + 30.0f, hc.z - 30.0f))
				{
					PlayerGotHit(theNode, 0, PLAYER_ANIM_GOTHIT_BACKFLIP);
					hitPlayer = true;
				}
			}

					/* KNOCK PLAYER */

			if (hitPlayer)
			{
				ObjNode	*player = gPlayerInfo.objNode;
				float	r = theNode->Rot.y;										// get gnome rot

				if (theNode->SlapRight)											// determine vector to hit player
					r -= PI/2;
				else
					r += PI/2;

				player->Delta.x = -sin(r) * 900.0f;
				player->Delta.z = -cos(r) * 900.0f;
				player->Delta.y = 900.0f;
				gCurrentMaxSpeed = 900;

				PlayEffect3D(EFFECT_SMACK, &hc);
			}
		}
	}

	UpdateGnome(theNode);
}




/********************** MOVE GNOME: GOT HIT ******************************/

static void  MoveGnome_KneeHurt(ObjNode *theNode)
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
		MorphToSkeletonAnim(theNode->Skeleton, GNOME_ANIM_STAND, 4);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateGnome(theNode);
}





//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME GNOME ENEMY *************************/

Boolean PrimeEnemy_Gnome(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


			/* MAKE GNOME */

	newObj = MakeGnome(x, z);
	SetSkeletonAnim(newObj->Skeleton, GNOME_ANIM_WALK);				// walks on spline


			/* SET SPLINE INFO */

	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplinePlacement = placement;
	newObj->SplineNum 		= splineNum;
	newObj->SplineMoveCall 	= MoveGnomeOnSpline;					// set move call
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE Gnome ON SPLINE ***************************/

static void MoveGnomeOnSpline(ObjNode *theNode)
{
Boolean isInRange;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 70);
	GetObjectCoordOnSpline(theNode);


			/* UPDATE STUFF IF IN RANGE */

	if (isInRange)
	{

		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// calc y coord
		UpdateObjectTransforms(theNode);											// update transforms
		UpdateShadow(theNode);

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < GNOME_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveGnome);

	}
}


/***************** UPDATE GNOME ************************/

static void UpdateGnome(ObjNode *theNode)
{
	UpdateEnemy(theNode);
}


#pragma mark -




/************************* GNOME GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

static void GnomeGotKickedCallback(ObjNode *player, ObjNode *enemy)
{
#pragma unused (player)

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveGnome);

	MorphToSkeletonAnim(enemy->Skeleton, GNOME_ANIM_KNEEHURT, 6);

	enemy->ButtTimer = 4.0f;


	PlayEffect3D(EFFECT_GNOMEGOTKICKED, &enemy->Coord);

}





