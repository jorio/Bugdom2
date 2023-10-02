/****************************/
/*   ENEMY: TOYSOLDIER.C	*/
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

static ObjNode *MakeToySoldier(float x, float z, short animNum);
static void MoveToySoldier(ObjNode *theNode);
static void MoveToySoldier_Standing(ObjNode *theNode);
static void  MoveToySoldier_Walk(ObjNode *theNode);
static void  MoveToySoldier_Death(ObjNode *theNode);
static void MoveToySoldierOnSpline(ObjNode *theNode);
static void UpdateToySoldier(ObjNode *theNode);
static void  MoveToySoldier_GotHit(ObjNode *theNode);
static Boolean HurtToySoldier(ObjNode *enemy, float damage);
static void  MoveToySoldier_Throw(ObjNode *theNode);
static void  MoveToySoldier_ReLoad(ObjNode *theNode);

static void KillToySoldier(ObjNode *enemy);
static void ToySoldierGotKickedCallback(ObjNode *player, ObjNode *kickedObj);

static void GiveToySoldierAGrenade(ObjNode *enemy);
static void UpdateToySoldierGrenade(ObjNode *enemy);
static void ThrowGrenade(ObjNode *theEnemy);
static void MoveToySoldierGrenade(ObjNode *theNode);
static void ExplodeGrenade(ObjNode *theNode);
static Boolean SeeIfToySoldierAttack(ObjNode *theNode, float angleToTarget, float dist);


/*********************/
/*    CONSTANTS      */
/*********************/

#define	MAX_TOYSOLDIERS				8

#define	TOYSOLDIER_SCALE				1.5f

#define	TOYSOLDIER_DETACH_DIST			400.0f

#define	TOYSOLDIER_CHASE_DIST_MIN		500.0f
#define	TOYSOLDIER_CHASE_DIST_MAX		1400.0f					// dist to start hop chase

#define	TOYSOLDIER_TARGET_OFFSET		20.0f

#define TOYSOLDIER_TURN_SPEED			20.0f
#define TOYSOLDIER_WALK_SPEED			300.0f

#define	TOYSOLDIER_HEALTH				1.0f
#define	TOYSOLDIER_DAMAGE				.2f



		/* MODES */

enum
{
	TOYSOLDIER_MODE_NONE,
	TOYSOLDIER_MODE_WATCHGRENADE
};


		/* GRENADE */

#define	GRENADE_ATTACK_DIST				600.0f
#define	GRENADE_THROW_MIN_ANGLE			0.04f
#define	GRENADE_DAMAGE					.3f

#define	GRENADE_SPEED					400.0f


/*********************/
/*    VARIABLES      */
/*********************/

#define	ButtTimer			SpecialF[2]


#define	ThrowGrenadeNow		Flag[0]					// set by animation when spear should be thrown
#define	ReLoadNow			Flag[0]
#define	HasGrenade			Flag[1]					// true if this guy has a bottle grenade


		/* GRENADE */

#define	PrevSides				Special[1]


/************************ ADD TOYSOLDIER ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_ToySoldier(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_TOYSOLDIER] >= MAX_TOYSOLDIERS)
			return(false);
	}

	newObj = MakeToySoldier(x, z, TOYSOLDIER_ANIM_STAND);

	newObj->TerrainItemPtr = itemPtr;

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_TOYSOLDIER]++;


	return(true);
}


/************************* MAKE TOYSOLDIER ****************************/

static ObjNode *MakeToySoldier(float x, float z, short animNum)
{
ObjNode	*newObj;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_TOYSOLDIER, animNum, x,z, TOYSOLDIER_SCALE, 0, MoveToySoldier);



				/* SET BETTER INFO */

	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync

	newObj->StatusBits |= STATUS_BIT_NOTEXTUREWRAP;

	newObj->Health 		= TOYSOLDIER_HEALTH;
	newObj->Damage 		= TOYSOLDIER_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_TOYSOLDIER;
	newObj->Mode		= TOYSOLDIER_MODE_NONE;

	newObj->What 		= WHAT_SOLDIER;

				/* SET COLLISION INFO */

//	newObj->ForceLookAtDist	= 800.0f;
//	newObj->CType |= CTYPE_LOOKAT;

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,TOYSOLDIER_TARGET_OFFSET);


	newObj->HurtCallback 		= HurtToySoldier;							// set hurt callback function
	newObj->GotKickedCallback 	= ToySoldierGotKickedCallback;			// set callback for being kicked

	newObj->Damage = TOYSOLDIER_DAMAGE;


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 2, 2, false);


		/* GIVE THE TOYSOLDIER A GRENADE */

	GiveToySoldierAGrenade(newObj);


	return(newObj);

}





/********************* MOVE TOYSOLDIER **************************/

static void MoveToySoldier(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveToySoldier_Standing,
					MoveToySoldier_Throw,
					MoveToySoldier_Death,
					MoveToySoldier_Walk,
					MoveToySoldier_GotHit,
					MoveToySoldier_ReLoad,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE TOYSOLDIER: STANDING ******************************/

static void  MoveToySoldier_Standing(ObjNode *theNode)
{
float	angleToTarget;
float	dist;

	switch(theNode->Mode)
	{
				/****************************************/
				/* HANDLE WAITING FOR BOTTLE GRENADE TO HIT */
				/****************************************/

		case	TOYSOLDIER_MODE_WATCHGRENADE:
				theNode->Timer -= gFramesPerSecondFrac;						// see if can throw again
				if (theNode->Timer <= 0.0f)
				{
					theNode->Mode = TOYSOLDIER_MODE_NONE;
				}
				break;


				/**************************/
				/* HANDLE ALL OTHER MODES */
				/**************************/

		default:

							/* TURN TOWARDS ME */

				angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, TOYSOLDIER_TURN_SPEED, false);

				dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);		// calc dist to player

				if (!gGamePrefs.kiddieMode)
				{
							/* SEE IF THROW GRENADE */

					if (theNode->HasGrenade)
					{
						if (SeeIfToySoldierAttack(theNode, angleToTarget, dist))
							goto doco;
					}

						/* SEE IF START CHASING PLAYER */

					if ((dist < TOYSOLDIER_CHASE_DIST_MAX) && (dist > TOYSOLDIER_CHASE_DIST_MIN))
					{
						gDelta.x = gDelta.z = 0;
						MorphToSkeletonAnim(theNode->Skeleton, TOYSOLDIER_ANIM_WALK, 5);
					}
				}
	}

				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/

doco:
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateToySoldier(theNode);
}




/********************** MOVE TOYSOLDIER: WALKING ******************************/

static void  MoveToySoldier_Walk(ObjNode *theNode)
{
float		r,fps,dist;

	fps = gFramesPerSecondFrac;


			/* MOVE TOWARD PLAYER */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, TOYSOLDIER_TURN_SPEED, false);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * TOYSOLDIER_WALK_SPEED;
	gDelta.z = -cos(r) * TOYSOLDIER_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* SEE IF STAND */

	dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	if ((dist < TOYSOLDIER_CHASE_DIST_MIN) || (dist > TOYSOLDIER_CHASE_DIST_MAX) || gGamePrefs.kiddieMode)
	{
		MorphToSkeletonAnim(theNode->Skeleton, TOYSOLDIER_ANIM_STAND, 2.0);
	}

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	UpdateToySoldier(theNode);
}




/********************** MOVE TOYSOLDIER: THROW ******************************/

static void  MoveToySoldier_Throw(ObjNode *theNode)
{
float	angleToTarget;

	gDelta.x = gDelta.z = gDelta.y = 0;

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, TOYSOLDIER_TURN_SPEED, false);

			/*****************************/
			/* SEE IF LAUNCH THE GRENADE */
			/*****************************/

	if (theNode->ThrowGrenadeNow)
	{
		theNode->ThrowGrenadeNow = false;

				/* MAKE SURE STILL AIMED AT ME */

		if (angleToTarget < GRENADE_THROW_MIN_ANGLE)
			ThrowGrenade(theNode);
	}


			/* SEE IF DONE WITH ANIM */

	if (theNode->Skeleton->AnimHasStopped)
	{
		theNode->ReLoadNow = false;
		MorphToSkeletonAnim(theNode->Skeleton, TOYSOLDIER_ANIM_RELOAD, 10);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateToySoldier(theNode);
}



/********************** MOVE TOYSOLDIER: GOT HIT ******************************/

static void  MoveToySoldier_GotHit(ObjNode *theNode)
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
		MorphToSkeletonAnim(theNode->Skeleton, TOYSOLDIER_ANIM_STAND, 6);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateToySoldier(theNode);
}




/********************** MOVE TOYSOLDIER: DEATH ******************************/

static void  MoveToySoldier_Death(ObjNode *theNode)
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
	gDelta.y -= ENEMY_GRAVITY*fps				;		// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES, true))
		return;


				/* UPDATE */

	UpdateToySoldier(theNode);


}


/********************** MOVE TOYSOLDIER: RELOAD ******************************/

static void  MoveToySoldier_ReLoad(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;

	ApplyFrictionToDeltas(1200.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*fps;			// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

			/* SEE IF HAS GRENADE NOW */

	if (theNode->ReLoadNow)
	{
		GiveToySoldierAGrenade(theNode);
		theNode->ReLoadNow = false;
	}


				/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, TOYSOLDIER_ANIM_STAND, 10);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	UpdateToySoldier(theNode);
}




/***************** UPDATE TOYSOLDIER ************************/

static void UpdateToySoldier(ObjNode *theNode)
{
	UpdateEnemy(theNode);

	UpdateToySoldierGrenade(theNode);
}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME TOYSOLDIER ENEMY *************************/

Boolean PrimeEnemy_ToySoldier(int splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&gSplineList[splineNum], placement, &x, &z);


				/* MAKE TOYSOLDIER */

	newObj = MakeToySoldier(x,z, TOYSOLDIER_ANIM_WALK);


				/* SET BETTER INFO */

	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveToySoldierOnSpline;					// set move call

	newObj->Coord.y 		-= newObj->BottomOff;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE TOYSOLDIER ON SPLINE ***************************/

static void MoveToySoldierOnSpline(ObjNode *theNode)
{
Boolean isInRange;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 80);
	GetObjectCoordOnSpline(theNode);


			/* UPDATE STUFF IF IN RANGE */

	if (isInRange)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// calc y coord
		UpdateObjectTransforms(theNode);															// update transforms
		UpdateShadow(theNode);

		UpdateToySoldierGrenade(theNode);


				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < TOYSOLDIER_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveToySoldier);

	}
}



#pragma mark -


/************************* TOYSOLDIER GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

static void ToySoldierGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	PlayEffect3D(EFFECT_FLYGOTKICKED, &kickedObj->Coord);

	kickedObj->Delta.x = -sin(r) * 800.0f;
	kickedObj->Delta.z = -cos(r) * 800.0f;
	kickedObj->Delta.y = 600.0f;

	HurtToySoldier(kickedObj, .5);
}




/*********************** HURT TOYSOLDIER ***************************/

static Boolean HurtToySoldier(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveToySoldier);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillToySoldier(enemy);
		return(true);
	}
	else
	{
		MorphToSkeletonAnim(enemy->Skeleton, TOYSOLDIER_ANIM_GOTHIT, 10);
		enemy->ButtTimer = 2.5;
	}

	return(false);
}


/****************** KILL TOYSOLDIER ***********************/

static void KillToySoldier(ObjNode *enemy)
{
	enemy->CType 				= CTYPE_MISC;				// no longer an enemy
	enemy->HurtCallback 		= nil;
	enemy->GotKickedCallback 	= nil;

	MorphToSkeletonAnim(enemy->Skeleton, TOYSOLDIER_ANIM_DEATH, 4);

	enemy->TerrainItemPtr = nil;			// dont ever come back

}


#pragma mark -


/***************************** GIVE TOYSOLDIER A BOTTLE GRENADE *********************************/

static void GiveToySoldierAGrenade(ObjNode *enemy)
{
ObjNode	*grenade;

	if (enemy->HasGrenade)								// make sure doesn't already have one
		return;

			/* MAKE GRENADE OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Grenade;
	gNewObjectDefinition.coord		= enemy->Coord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;				// grenades don't get collided against, so can put @ end
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= TOYSOLDIER_SCALE;
	grenade = MakeNewDisplayGroupObject(&gNewObjectDefinition);


	CreateCollisionBoxFromBoundingBox(grenade,.5,.5);

	grenade->Damage = GRENADE_DAMAGE;

			/* ATTACH GRENADE TO ENEMY */

	enemy->ChainNode = grenade;
	grenade->ChainHead = enemy;

	enemy->HasGrenade = true;
}


/************************* UPDATE TOYSOLDIER GRENADE ************************/
//
// Updated when is being held by enemy.
//

static void UpdateToySoldierGrenade(ObjNode *enemy)
{
ObjNode					*grenade;
OGLMatrix4x4			m,m2;
static const OGLPoint3D	zero = {0,0,0};

			/* VERIFY */

	if (!enemy->HasGrenade)
		return;
	if (!enemy->ChainNode)
		return;

	grenade = enemy->ChainNode;


			/* SET GRENADE MATRIX */

	FindJointFullMatrix(enemy, TOYSOLDIER_JOINTNUM_RIGHTHAND, &m);
	OGLMatrix4x4_SetTranslate(&m2, -1,-5,1);
	OGLMatrix4x4_Multiply(&m2, &m, &grenade->BaseTransformMatrix);

	SetObjectTransformMatrix(grenade);


			/* SET REAL POINT FOR CULLING */

	OGLPoint3D_Transform(&zero, &grenade->BaseTransformMatrix, &grenade->Coord);
}


/********************* THROW GRENADE ************************/

static void ThrowGrenade(ObjNode *enemy)
{
ObjNode		 			*grenade;
float					rot;

	enemy->HasGrenade = false;							// dont have it anymore

	grenade = enemy->ChainNode;							// get spear obj
	if (grenade == nil)
		return;


		/* DETACH FROM CHAIN */

	enemy->ChainNode 	= nil;
	grenade->ChainHead 	= nil;
	grenade->MoveCall 	= MoveToySoldierGrenade;


		/* CALC THROW VECTOR */

	rot = enemy->Rot.y;
	grenade->Delta.x = -sin(rot) * GRENADE_SPEED;
	grenade->Delta.z = -cos(rot) * GRENADE_SPEED;
	grenade->Delta.y = GRENADE_SPEED*2/3;

	grenade->Timer = 1.5f + RandomFloat();				// time to detonate


	enemy->Mode = TOYSOLDIER_MODE_WATCHGRENADE;			// the ant is watching the spear go
	enemy->Timer = 1.0f + (RandomFloat() * 3.0f);		// time before solider throws again

			/* MAKE A SHADOW */

	AttachShadowToObject(grenade, SHADOW_TYPE_CIRCULAR, 1, 1, false);

	PlayEffect3D(EFFECT_GRENADETHROW, &grenade->Coord);

}



/******************* MOVE TOYSOLDIER GRENADE ****************/

static void MoveToySoldierGrenade(ObjNode *theNode)
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

			/* SEE IF EXPLODE */

	theNode->Timer -= fps;
	if (theNode->Timer <= 0.0f)
	{
		ExplodeGrenade(theNode);
		return;
	}



				/* MOVE IT */

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// do heavy friction if on ground
		ApplyFrictionToDeltas(800.0,&gDelta);
	else
	{
		ApplyFrictionToDeltas(100.0,&gDelta);
		theNode->Rot.z -= fps * 8.0f;						// spin when in air
	}


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

	sides = HandleCollisions(theNode, CTYPE_MISC | CTYPE_FENCE | CTYPE_TERRAIN | CTYPE_PLAYER, .4);
	if (sides && (sides != theNode->PrevSides))							// if hit something new then make a sound
	{
//		PlayEffect_Parms3D(EFFECT_BOTTLEGRENADEBOUNCE, &theNode->Coord, NORMAL_CHANNEL_RATE, .3);
	}
	theNode->PrevSides = sides;


	UpdateObject(theNode);
}


/********************** EXPLODE GRENADE ********************************/

static void ExplodeGrenade(ObjNode *theNode)
{
ObjNode	*thisNode;


			/******************/
			/* SEE WHO WE HIT */
			/******************/

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

				/* SEE IF HIT PLAYER */

		if (thisNode == gPlayerInfo.objNode)					// look for stuff we care about
		{
			if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 300.0f)
			{
				OGLVector2D	v;

				v.x = thisNode->Coord.x - theNode->Coord.x;		// calc blast vector to player
				v.y = thisNode->Coord.z - theNode->Coord.z;
				FastNormalizeVector2D(v.x, v.y, &v, true);

				thisNode->Delta.y = 700.0f;						// throw player
				thisNode->Delta.x = v.x * 700.0f;
				thisNode->Delta.z = v.y * 700.0f;

				gCurrentMaxSpeed = 700.0f;

				PlayerGotHit(theNode, 0, PLAYER_ANIM_GOTHIT_GENERIC);	// hurt player

			}
		}

				/* SEE IF HIT SOLDIER */

		else
		if (thisNode->What == WHAT_SOLDIER)
		{
			if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 200.0f)
			{
				//---- todo
			}
		}

		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);




			/**************/
			/* BLOW IT UP */
			/**************/

	PlayEffect3D(EFFECT_GRENADEBOOM, &gCoord);
	ExplodeGeometry(theNode, 300, SHARD_MODE_FROMORIGIN | SHARD_MODE_BOUNCE, 1, 2.0);
	MakeSparkExplosion(gCoord.x, gCoord.y, gCoord.z, 350, 1.0, PARTICLE_SObjType_YellowGlint, 80, 1.2);
	MakeFireExplosion(&gCoord);
	DeleteObject(theNode);
}


/************************ SEE IF TOYSOLDIER ATTACK ****************************/

static Boolean SeeIfToySoldierAttack(ObjNode *theNode, float angleToTarget, float dist)
{
	if ((angleToTarget < GRENADE_THROW_MIN_ANGLE) && (dist < GRENADE_ATTACK_DIST))
	{
		if (!SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))	// dont attack thru things
		{
			MorphToSkeletonAnim(theNode->Skeleton, TOYSOLDIER_ANIM_THROW, 5);
			theNode->ThrowGrenadeNow = false;
			return(true);
		}
	}
	return(false);
}


