/****************************/
/*   ENEMY: ANT.C	*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/



#include "dialog.h"
#include "3dmath.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLPoint3D				gCoord;
extern	int						gNumEnemies;
extern	float					gFramesPerSecondFrac,gGlobalTransparency;
extern	OGLVector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];
extern	u_long		gAutoFadeStatusBits;
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	int					gLevelNum,gMaxEnemies;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TerrainItemEntryType 	**gMasterItemList;
extern	short					gNumTerrainItems,gNumTicks;
extern	PrefsType			gGamePrefs;


/****************************/
/*    PROTOTYPES            */
/****************************/

static ObjNode *MakeAnt(float x, float z, short animNum, int foodType);
static void MoveAnt(ObjNode *theNode);
static void  MoveAnt_Stand_Food(ObjNode *theNode);
static void  MoveAnt_Stand(ObjNode *theNode);
static void  MoveAnt_WalkFood(ObjNode *theNode);
static void  MoveAnt_WalkEmpty(ObjNode *theNode);
static void  MoveAnt_Death(ObjNode *theNode);
static void  MoveAnt_GetUp(ObjNode *theNode);
static void  MoveAnt_Attack(ObjNode *theNode);

static void MoveAntOnSpline(ObjNode *theNode);
static void UpdateAnt(ObjNode *theNode);
static void  MoveAnt_GotHit(ObjNode *theNode);
static Boolean HurtAnt(ObjNode *enemy, float damage);

static void KillAnt(ObjNode *enemy);
static void AntGotKickedCallback(ObjNode *player, ObjNode *kickedObj);

static void GiveAntFood(ObjNode *enemy, int foodType);
static void UpdateAntFood(ObjNode *ant);
static void AntDropFood(ObjNode *enemy);
static void MoveAntFood(ObjNode *theNode);

/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_ANTS				7

#define	ANT_SCALE				1.1f

#define	ANT_DETACH_DIST		500.0f

#define	ANT_CHASE_DIST_MAX	900.0f

#define	ANT_ATTACK_DIST		100.0f

#define	ANT_TARGET_OFFSET		20.0f

#define ANT_TURN_SPEED			4.0f
#define ANT_WALK_SPEED			300.0f

#define	ANT_HEALTH				1.1f
#define	ANT_DAMAGE				.2f


		/* ANIMS */

enum
{
	ANT_ANIM_STAND_WITH_FOOD,
	ANT_ANIM_WALKFOOD,
	ANT_ANIM_STAND_NO_FOOD,
	ANT_ANIM_GOTHIT,
	ANT_ANIM_DEATH,
	ANT_ANIM_WALKEMPTY,
	ANT_ANIM_ATTACK,
	ANT_ANIM_GETUP
};



#define	ANT_JOINTNUM_HEAD				2
#define	ANT_JOINTNUM_RIGHTHAND			13



		/* FOOD */

#define	FOOD_ATTACK_DIST				500.0f
#define	FOOD_THROW_MIN_ANGLE			0.03f
#define	DIST_TO_RETRIVE_FOOD			140.0f
#define	FOOD_DAMAGE					.2f



/*********************/
/*    VARIABLES      */
/*********************/

#define	ButtTimer			SpecialF[2]



/************************ ADD ANT ENEMY *************************/

Boolean AddEnemy_Ant(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;
int		foodType = itemPtr->parm[0];


	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_ANT] >= MAX_ANTS)
			return(false);
	}

	newObj = MakeAnt(x, z, ANT_ANIM_STAND_WITH_FOOD, foodType);

	newObj->TerrainItemPtr = itemPtr;

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_ANT]++;


	return(true);
}


/************************* MAKE ANT ****************************/

static ObjNode *MakeAnt(float x, float z, short animNum, int foodType)
{
ObjNode	*newObj;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_ANT,animNum, x,z, ANT_SCALE, 0, MoveAnt);



				/* SET BETTER INFO */

	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync

	newObj->StatusBits |= STATUS_BIT_NOTEXTUREWRAP;

	newObj->Health 		= ANT_HEALTH;
	newObj->Damage 		= ANT_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_ANT;


				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, .7,1);
	CalcNewTargetOffsets(newObj,ANT_TARGET_OFFSET);


	newObj->HurtCallback 		= HurtAnt;							// set hurt callback function
	newObj->GotKickedCallback 	= AntGotKickedCallback;			// set callback for being kicked

	newObj->Damage = ANT_DAMAGE;


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 4, 6,false);


		/* GIVE THE ANT FOOD */

	GiveAntFood(newObj, foodType);

	return(newObj);

}



/********************* MOVE ANT **************************/

static void MoveAnt(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveAnt_Stand_Food,
					MoveAnt_WalkFood,
					MoveAnt_Stand,
					MoveAnt_GotHit,
					MoveAnt_Death,
					MoveAnt_WalkEmpty,
					MoveAnt_Attack,
					MoveAnt_GetUp,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE ANT: STANDING w/ FOOD ******************************/

static void  MoveAnt_Stand_Food(ObjNode *theNode)
{
float	angleToTarget;
ObjNode	*food;
float	dist;

			/* VERIFY FOOD */

	food = (ObjNode *)theNode->ChainNode;
	if (food == nil)
		goto bad_food;
	if (food->CType == INVALID_NODE_FLAG)			// see if isnt valid anymore
	{
bad_food:
		theNode->ChainNode = nil;
		MorphToSkeletonAnim(theNode->Skeleton, ANT_ANIM_STAND_NO_FOOD, 6);
	}

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, ANT_TURN_SPEED, false);
	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);		// calc dist to player


				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateAnt(theNode);
}



/********************** MOVE ANT: STANDING NO FOOD ******************************/

static void  MoveAnt_Stand(ObjNode *theNode)
{
float	angleToTarget;
float	dist;

				/* TURN TOWARDS ME */

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, ANT_TURN_SPEED, false);
	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);		// calc dist to player




	if (!gGamePrefs.kiddieMode)
	{
		if (!IsWaterInFrontOfEnemy(theNode->Rot.y))				// dont chase if we're in front of water
		{
			if (dist < ANT_CHASE_DIST_MAX)
			{
				MorphToSkeletonAnim(theNode->Skeleton, ANT_ANIM_WALKEMPTY, 6);
			}
		}
	}



				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateAnt(theNode);
}


/********************** MOVE ANT: GET UP ******************************/

static void  MoveAnt_GetUp(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton, ANT_ANIM_WALKEMPTY, 6);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateAnt(theNode);
}




/********************** MOVE ANT: WALKING WITH FOOD ******************************/

static void  MoveAnt_WalkFood(ObjNode *theNode)
{
float		r,fps,aim,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	aim = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, ANT_TURN_SPEED, false);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * ANT_WALK_SPEED;
	gDelta.z = -cos(r) * ANT_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* SEE IF STAND */

	dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	if ((dist > ANT_CHASE_DIST_MAX) || gGamePrefs.kiddieMode)
	{
		MorphToSkeletonAnim(theNode->Skeleton, ANT_ANIM_STAND_WITH_FOOD, 2.0);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	UpdateAnt(theNode);
}




/********************** MOVE ANT: WALKING EMPTY ******************************/

static void  MoveAnt_WalkEmpty(ObjNode *theNode)
{
float		r,fps,aim,dist;

	fps = gFramesPerSecondFrac;


			/* MOVE TOWARD PLAYER */

	aim = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, ANT_TURN_SPEED, false);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * ANT_WALK_SPEED;
	gDelta.z = -cos(r) * ANT_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	if (IsWaterInFrontOfEnemy(r))				// if about to enter water then stop
	{
		MorphToSkeletonAnim(theNode->Skeleton, ANT_ANIM_STAND_NO_FOOD, 8);
	}

			/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


			/* SEE IF ATTACK */

	dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	if (dist < ANT_ATTACK_DIST)
	{
		MorphToSkeletonAnim(theNode->Skeleton, ANT_ANIM_ATTACK, 2);
	}


	UpdateAnt(theNode);
}


/********************** MOVE ANT: ATTACK ******************************/

static void  MoveAnt_Attack(ObjNode *theNode)
{
float		r,fps,aim;
const OGLPoint3D	off = {0,20, -30};
OGLPoint3D	pt;

	fps = gFramesPerSecondFrac;


			/* MOVE TOWARD PLAYER */

	aim = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, ANT_TURN_SPEED, false);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * (ANT_WALK_SPEED/2);
	gDelta.z = -cos(r) * (ANT_WALK_SPEED/2);
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

			/* SEE IF HIT PLAYER */

	FindCoordOnJoint(theNode, ANT_JOINTNUM_HEAD, &off, &pt);
	if (DoSimplePointCollisionAgainstPlayer(&pt))
	{
		PlayerGotHit(theNode, 0, PLAYER_ANIM_GOTHIT_GENERIC);
	}



			/* SEE IF DONE */

	if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) > ANT_ATTACK_DIST)
	{
		MorphToSkeletonAnim(theNode->Skeleton, ANT_ANIM_WALKEMPTY, 8);
	}

	UpdateAnt(theNode);
}


/********************** MOVE ANT: GOT HIT ******************************/

static void  MoveAnt_GotHit(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	ApplyFrictionToDeltas(1200.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*fps;			// add gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* SEE IF DONE */

	theNode->ButtTimer -= fps;
	if (theNode->ButtTimer <= 0.0)
	{
		SetSkeletonAnim(theNode->Skeleton, ANT_ANIM_GETUP);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateAnt(theNode);
}




/********************** MOVE ANT: DEATH ******************************/

static void  MoveAnt_Death(ObjNode *theNode)
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
		ApplyFrictionToDeltas(2000.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*fps;		// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES, true))
		return;


				/* UPDATE */

	UpdateAnt(theNode);


}




/***************** UPDATE ANT ************************/

static void UpdateAnt(ObjNode *theNode)
{

	UpdateEnemy(theNode);

	UpdateAntFood(theNode);



}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME ANT ENEMY *************************/

Boolean PrimeEnemy_Ant(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;
int				foodType = itemPtr->parm[0];

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE ANT */

	newObj = MakeAnt(x,z, ANT_ANIM_WALKFOOD, foodType);


				/* SET BETTER INFO */

	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveAntOnSpline;					// set move call

	newObj->Coord.y 		-= newObj->BottomOff;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE ANT ON SPLINE ***************************/

static void MoveAntOnSpline(ObjNode *theNode)
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

		UpdateAntFood(theNode);


				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))
			return;
	}
}



#pragma mark -


/************************* ANT GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

static void AntGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	PlayEffect3D(EFFECT_FLYGOTKICKED, &kickedObj->Coord);

	kickedObj->Delta.x = -sin(r) * 800.0f;
	kickedObj->Delta.z = -cos(r) * 800.0f;
	kickedObj->Delta.y = 600.0f;

	kickedObj->Rot.y = player->Rot.y + PI;

	HurtAnt(kickedObj, .3);
}




/*********************** HURT ANT ***************************/

static Boolean HurtAnt(ObjNode *enemy, float damage)
{
	AntDropFood(enemy);

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveAnt);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillAnt(enemy);
		return(true);
	}
	else
	{
		MorphToSkeletonAnim(enemy->Skeleton, ANT_ANIM_GOTHIT, 10);
		enemy->ButtTimer = 1.5;
	}

	return(false);
}


/****************** KILL ANT ***********************/

static void KillAnt(ObjNode *enemy)
{
	enemy->CType 				= CTYPE_MISC;				// no longer an enemy
	enemy->HurtCallback 		= nil;
	enemy->GotKickedCallback 	= nil;

	SetSkeletonAnim(enemy->Skeleton, ANT_ANIM_DEATH);

	enemy->TerrainItemPtr = nil;			// dont ever come back

}


#pragma mark -


/***************************** GIVE ANT A FOOD *********************************/

static void GiveAntFood(ObjNode *enemy, int foodType)
{
ObjNode	*food;

			/* MAKE FOOD OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PARK_ObjType_CheeseBit + foodType;
	gNewObjectDefinition.coord		= enemy->Coord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= enemy->Slot+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.0f;
	food = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			/* ATTACH FOOD TO ENEMY */

	enemy->ChainNode = food;

	food->Kind = foodType;
}


/************************* UPDATE ANT FOOD ************************/
//
// Updated when is being held by ant.
//

static void UpdateAntFood(ObjNode *ant)
{
ObjNode					*food;
OGLMatrix4x4			m,m2,mst,rm;
static const OGLPoint3D	zero = {0,0,0};
float					scale;
int						foodType;
const OGLPoint3D foodOff[3] =
{
	-20, 45, 10,					// cheese
	-20, 42, -4,					// cherry
	-20, 55, 0,					// olive
};
const OGLVector3D foodRot[3] =
{
	4.7, 0.6, 0.0,					// cheese
	4.7, 0.6, 0.0,					// cherry
	4.3, 0.6, 0.0,					// olive
};
			/* VERIFY */

	if (!ant->ChainNode)
		return;

	food = ant->ChainNode;
	foodType = food->Kind;

			/* CALC SCALE MATRIX */

	scale = food->Scale.x / ant->Scale.x;							// to adjust from enemy's scale to food's scale
	OGLMatrix4x4_SetScale(&mst, scale, scale, scale);

			/* CALC TRANSLATE MATRIX */

	mst.value[M03] = foodOff[foodType].x;							// set offset for hand
	mst.value[M13] = foodOff[foodType].y;
	mst.value[M23] = foodOff[foodType].z;


			/* CALC ROTATE MATRIX */

	OGLMatrix4x4_SetRotate_XYZ(&rm, foodRot[foodType].x, foodRot[foodType].y, foodRot[foodType].z); 	// set rotation to fit in hand
	OGLMatrix4x4_Multiply(&mst, &rm, &m2);

			/* GET ALIGNMENT MATRIX */

	FindJointFullMatrix(ant, ANT_JOINTNUM_RIGHTHAND, &m);
	OGLMatrix4x4_Multiply(&m2, &m, &food->BaseTransformMatrix);
	SetObjectTransformMatrix(food);


			/* SET REAL POINT FOR CULLING */

	OGLPoint3D_Transform(&zero, &food->BaseTransformMatrix, &food->Coord);
}


/********************* ANT DROP FOOD ************************/

static void AntDropFood(ObjNode *enemy)
{
ObjNode		*food;
int			foodType;
const OGLPoint3D foodOff[3] =
{
	-14, -30, -10,					// cheese
	-14, -15, -30,					// cherry
	-14, 45, 0,					// olive
};
const OGLVector3D foodRot[3] =
{
	.9, 0, 0,					// cheese
	-.5, 0, 0,					// cherry
	-1.8, 0, 0,					// olive
};


	food = enemy->ChainNode;						// get food obj
	if (food == nil)
		return;

	foodType = food->Kind;


		/* DETACH FROM CHAIN */

	enemy->ChainNode = nil;


			/* SET PICKUP INFO */

	food->MoveCall 		= MoveAntFood;
	food->Kind 			= PICKUP_KIND_FOOD;						// remember what kind of pickup this is
	food->DropCallback 	= DefaultDropObject;					// set drop callback
	food->GotKickedCallback = DefaultGotKickedCallback;			// set callback for being kicked

	food->HoldOffset.x = foodOff[foodType].x;					// set holding offset for Skip
	food->HoldOffset.y = foodOff[foodType].y;
	food->HoldOffset.z = foodOff[foodType].z;

	food->HoldRot.x = foodRot[foodType].x;
	food->HoldRot.y = foodRot[foodType].y;
	food->HoldRot.z = foodRot[foodType].z;


			/* SET COLLISION STUFF */

	food->CType 	= CTYPE_MISC|CTYPE_PICKUP|CTYPE_KICKABLE;
	food->CBits		= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(food,1,1);


			/* MAKE A SHADOW */

	AttachShadowToObject(food, SHADOW_TYPE_CIRCULAR, 4, 4, true);

}



/******************* MOVE ANT FOOD ****************/

static void MoveAntFood(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);


				/* MOVE IT */

	gDelta.y -= 3000.0f * fps;
	gCoord.x += gDelta.x * fps;						// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* COLLISION */

	HandleCollisions(theNode, CTYPE_MISC | CTYPE_FENCE | CTYPE_TRIGGER | CTYPE_TERRAIN, .4);

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		gDelta.x *= .8f;
		gDelta.z *= .8f;
	}

	UpdateObject(theNode);
}






