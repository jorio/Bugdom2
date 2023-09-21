/****************************/
/*   ENEMY: TICK.C	*/
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
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	int					gLevelNum,gMaxEnemies;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TerrainItemEntryType 	**gMasterItemList;
extern	short					gNumTerrainItems, gNumKilledFleas, gTotalFleas;
extern	SparkleType	gSparkles[];
extern	PrefsType			gGamePrefs;


/****************************/
/*    PROTOTYPES            */
/****************************/

static ObjNode *MakeTick(float x, float z, short animNum);
static void MoveTick(ObjNode *theNode);
static void  MoveTick_Stand_Full(ObjNode *theNode);
static void  MoveTick_Stand_Empty(ObjNode *theNode);
static void  MoveTick_Walk_Full(ObjNode *theNode);
static void  MoveTick_Walk_Empty(ObjNode *theNode);
static void  MoveTick_Death(ObjNode *theNode);
static void MoveTickOnSpline(ObjNode *theNode);
static void UpdateTick(ObjNode *theNode);
static void  MoveTick_GotHit(ObjNode *theNode);
static Boolean HurtTick(ObjNode *enemy, float damage);
static void  MoveTick_Suck(ObjNode *theNode);
static void  MoveTick_Spit(ObjNode *theNode);
static void SeeIfTickAttack(ObjNode *theNode);
static void TickSpit(ObjNode *enemy);
static void MoveTickSpit(ObjNode *theNode);

static void KillTick(ObjNode *enemy);
static void TickGotKickedCallback(ObjNode *player, ObjNode *kickedObj);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_TICKS				8

#define	TICK_SCALE				1.8f

#define	TICK_CHASE_DIST_MAX		1500.0f
#define	TICK_CHASE_DIST_MIN		250.0f
#define	TICK_DETACH_DIST		400.0f

#define	TICK_ATTACK_DIST		250.0f

#define	TICK_TARGET_OFFSET		20.0f

#define TICK_TURN_SPEED			5.0f
#define TICK_WALK_SPEED_EMPTY	500.0f

#define TICK_LURCH_SPEED		800.0f

#define	TICK_HEALTH				1.1f
#define	TICK_DAMAGE				.2f


		/* ANIMS */

enum
{
	TICK_ANIM_STAND_FULL,
	TICK_ANIM_WALK_FULL,
	TICK_ANIM_SPIT,
	TICK_ANIM_GETHIT,
	TICK_ANIM_DEATH,
	TICK_ANIM_SUCK,
	TICK_ANIM_STAND_EMPTY,
	TICK_ANIM_WALK_EMPTY
};




#define	SPIT_SPEED				500.0f


#define	TICK_JOINT_HEAD				6


/*********************/
/*    VARIABLES      */
/*********************/

short	gTotalTicks = 0;
short	gNumKilledTicks = 0;

#define	SpitNow				Flag[0]
#define	LurchNow			Flag[0]

#define	ShotSpacing			SpecialF[1]
#define	ButtTimer			SpecialF[2]



/************************ ADD TICK ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Tick(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;

	newObj = MakeTick(x, z, TICK_ANIM_STAND_EMPTY);

	newObj->TerrainItemPtr = itemPtr;

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_TICK]++;


	return(true);
}


/************************* MAKE TICK ****************************/

static ObjNode *MakeTick(float x, float z, short animNum)
{
ObjNode	*newObj;
int		j,i;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_TICK,animNum, x,z, TICK_SCALE, 0, MoveTick);



				/* SET BETTER INFO */

	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync

	newObj->StatusBits |= STATUS_BIT_NOTEXTUREWRAP;

	newObj->Health 		= TICK_HEALTH;
	newObj->Damage 		= TICK_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_TICK;


				/* SET COLLISION INFO */

	newObj->ForceLookAtDist	= 800.0f;
	newObj->CType |= CTYPE_LOOKAT;

	CreateCollisionBoxFromBoundingBox(newObj, .7,1);
	CalcNewTargetOffsets(newObj,TICK_TARGET_OFFSET);


	newObj->HurtCallback 		= HurtTick;							// set hurt callback function
	newObj->GotKickedCallback 	= TickGotKickedCallback;			// set callback for being kicked

	newObj->Timer = RandomFloat();

	newObj->Damage = TICK_DAMAGE;


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 10,false);


			/******************/
			/* MAKE EYES GLOW */
			/******************/

	for (j = 0; j < 2; j++)
	{
		i = newObj->Sparkles[j] = GetFreeSparkle(newObj);				// get free sparkle slot
		if (i != -1)
		{
			gSparkles[i].flags = 0;
			gSparkles[i].where = newObj->Coord;

			gSparkles[i].aim.x = 0;
			gSparkles[i].aim.y = 0;
			gSparkles[i].aim.z = -1;

			gSparkles[i].color.r = .8;
			gSparkles[i].color.g = .8;
			gSparkles[i].color.b = .8;
			gSparkles[i].color.a = 1;

			gSparkles[i].scale = 60.0f;
			gSparkles[i].separation = 20.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_GreenGlint;
		}
	}


	return(newObj);

}





/********************* MOVE TICK **************************/

static void MoveTick(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveTick_Stand_Full,
					MoveTick_Walk_Full,
					MoveTick_Spit,
					MoveTick_GotHit,
					MoveTick_Death,
					MoveTick_Suck,
					MoveTick_Stand_Empty,
					MoveTick_Walk_Empty
				};

//	if (TrackTerrainItem(theNode))						// just check to see if it's gone
//	{
//		DeleteEnemy(theNode);
//		return;
//	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE TICK: STANDING FULL ******************************/

static void  MoveTick_Stand_Full(ObjNode *theNode)
{
float	dist;
float	fps = gFramesPerSecondFrac;

	ApplyFrictionToDeltas(2000.0,&gDelta);

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, TICK_TURN_SPEED, false);

				/* SEE IF CHASE */

	if (!gGamePrefs.kiddieMode)
	{
		if (!theNode->Skeleton->IsMorphing)
		{
			if (!IsWaterInFrontOfEnemy(theNode->Rot.y))				// dont chase if we're in front of water
			{
				dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
				if ((dist < TICK_CHASE_DIST_MAX) && (dist > TICK_CHASE_DIST_MIN))
				{
					MorphToSkeletonAnim(theNode->Skeleton, TICK_ANIM_WALK_FULL, 5);
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


	SeeIfTickAttack(theNode);

	UpdateTick(theNode);
}



/********************** MOVE TICK: STANDING EMPTY ******************************/

static void  MoveTick_Stand_Empty(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	dist;

	ApplyFrictionToDeltas(2000.0,&gDelta);

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*fps;									// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* SEE IF READY TO SUCK */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if (dist < TICK_CHASE_DIST_MAX)
	{
		theNode->Timer -= fps;
		if (theNode->Timer <= 0.0f)
		{
			MorphToSkeletonAnim(theNode->Skeleton, TICK_ANIM_SUCK, 4);
			theNode->EffectChannel = PlayEffect3D(EFFECT_TICKSUCK, &gCoord);
		}
	}



				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateTick(theNode);
}



/********************** MOVE TICK: WALK FULL ******************************/

static void  MoveTick_Walk_Full(ObjNode *theNode)
{
float		r,fps;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, TICK_TURN_SPEED, false);

	if (theNode->LurchNow)
	{
		r = theNode->Rot.y;
		gDelta.x = -sin(r) * TICK_LURCH_SPEED;
		gDelta.z = -cos(r) * TICK_LURCH_SPEED;
		theNode->LurchNow = false;
	}
	else
		ApplyFrictionToDeltas(900.0,&gDelta);



	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;



				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	SeeIfTickAttack(theNode);

	UpdateTick(theNode);
}


/********************** MOVE TICK: WALK EMPTY ******************************/

static void  MoveTick_Walk_Empty(ObjNode *theNode)
{
float		r,fps;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, TICK_TURN_SPEED, false);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * TICK_WALK_SPEED_EMPTY;
	gDelta.z = -cos(r) * TICK_WALK_SPEED_EMPTY;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;



				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	UpdateTick(theNode);
}







/********************** MOVE TICK: GOT HIT ******************************/

static void  MoveTick_GotHit(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (theNode->Speed3D > 500.0f)
		TurnObjectTowardTarget(theNode, &gCoord, gCoord.x - gDelta.x, gCoord.z - gDelta.z, 10, false);	// aim with motion

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(1200.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*fps;			// add gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* SEE IF DONE */

	theNode->ButtTimer -= fps;
	if (theNode->ButtTimer <= 0.0)
	{
		MorphToSkeletonAnim(theNode->Skeleton, TICK_ANIM_STAND_EMPTY, 3);
		theNode->Timer = 2.0f;
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateTick(theNode);
}




/********************** MOVE TICK: DEATH ******************************/

static void  MoveTick_Death(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;

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

	UpdateTick(theNode);


}


/********************** MOVE TICK: SPIT ******************************/

static void  MoveTick_Spit(ObjNode *theNode)
{
float		fps,angle;

	fps = gFramesPerSecondFrac;

	if (theNode->SpitNow)
		TickSpit(theNode);


			/* MOVE TOWARD SPURT TARGET */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, TICK_TURN_SPEED, false);


			/* MOVE */

	ApplyFrictionToDeltas(2000.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;



				/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, TICK_ANIM_STAND_EMPTY, 4);
		theNode->Timer = 2.0f;
		gDelta.x = gDelta.z = 0;
	}

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	UpdateTick(theNode);
}


/********************** MOVE TICK: SUCK ******************************/

static void  MoveTick_Suck(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton, TICK_ANIM_STAND_FULL, 3);


	ApplyFrictionToDeltas(2000.0,&gDelta);

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*fps;									// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


			/* UPDATE */

	Update3DSoundChannel(EFFECT_TICKSUCK, &theNode->EffectChannel, &gCoord);

	UpdateTick(theNode);
}





/***************** UPDATE TICK ************************/

static void UpdateTick(ObjNode *theNode)
{
short			i;
float			r,aimX,aimZ;
const static OGLPoint3D	leftEye = {-5,2.5,-35};
const static OGLPoint3D	rightEye = {5,2.5,-35};
OGLMatrix4x4	m;

	UpdateEnemy(theNode);


		/***********************/
		/* UPDATE EYE SPARKLES */
		/***********************/

	r = theNode->Rot.y;
	aimX = -sin(r);
	aimZ = -cos(r);

	FindJointFullMatrix(theNode,TICK_JOINT_HEAD,&m);						// get head matrix


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




//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME TICK ENEMY *************************/

Boolean PrimeEnemy_Tick(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE TICK */

	newObj = MakeTick(x,z, TICK_ANIM_WALK_EMPTY);

	newObj->Skeleton->AnimSpeed = 2.5f;


				/* SET BETTER INFO */

	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveTickOnSpline;					// set move call

	newObj->Coord.y 		-= newObj->BottomOff;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE TICK ON SPLINE ***************************/

static void MoveTickOnSpline(ObjNode *theNode)
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

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < TICK_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveTick);

	}
}



#pragma mark -


/************************* TICK GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

static void TickGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	PlayEffect3D(EFFECT_FLYGOTKICKED, &kickedObj->Coord);

	kickedObj->Delta.x = -sin(r) * 800.0f;
	kickedObj->Delta.z = -cos(r) * 800.0f;
	kickedObj->Delta.y = 600.0f;

	HurtTick(kickedObj, .5);
}




/*********************** HURT TICK ***************************/

static Boolean HurtTick(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveTick);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillTick(enemy);
		return(true);
	}

	return(false);
}


/****************** KILL TICK ***********************/

static void KillTick(ObjNode *enemy)
{
	enemy->CType 				= CTYPE_MISC;				// no longer an enemy
	enemy->HurtCallback 		= nil;
	enemy->GotKickedCallback 	= nil;

	MorphToSkeletonAnim(enemy->Skeleton, TICK_ANIM_DEATH, 4);

	enemy->TerrainItemPtr = nil;			// dont ever come back
	gNumKilledTicks++;

			/* SEE IF TALK ON FIDO LEVEL */

	if (gLevelNum == LEVEL_NUM_FIDO)
	{
		if (gNumKilledTicks >= gTotalTicks)
		{
			if (gNumKilledFleas < gTotalFleas)										// see if that was all the fleas but still have ticks
				DoDialogMessage(DIALOG_MESSAGE_GOTTICKS, 1, 8.0, nil);
			else																	// all ticks and fleas dead, so level done
			{
				StartLevelCompletion(4.0);
				DoDialogMessage(DIALOG_MESSAGE_HAPPYDOG, 1, 3.0, nil);
			}
		}

		PlayEffect3D(EFFECT_TICKDIE, &enemy->Coord);
	}
}


#pragma mark -



/**************************** COUNT TICKS *********************************/

void CountTicks(void)
{
int						i;
TerrainItemEntryType 	*itemPtr;

	gNumKilledTicks = 0;
	gTotalTicks = 0;

	itemPtr = *gMasterItemList; 											// get pointer to data inside the LOCKED handle

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_TICK)
			gTotalTicks++;
	}

}


#pragma mark -

/********************** SEE IF TICK ATTACK ******************************/

static void SeeIfTickAttack(ObjNode *theNode)
{
	if (OGLPoint3D_Distance(&gCoord, &gPlayerInfo.coord) < TICK_ATTACK_DIST)
	{
		MorphToSkeletonAnim(theNode->Skeleton, TICK_ANIM_SPIT, 5);
		theNode->SpitNow = false;
	}
}



/********************* TICK SPIT ****************************/

static void TickSpit(ObjNode *enemy)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*newObj;
static const OGLPoint3D	muzzleOff = {0,-10,-15};
static const OGLVector3D muzzleAim = {0,-.05,-1};
OGLVector3D	aim;
OGLMatrix4x4	m;

			/* SEE IF TIME TO FIRE OFF A SHOT */

	enemy->ShotSpacing -= fps;
	if (enemy->ShotSpacing <= 0.0f)
	{
		enemy->ShotSpacing += .25f;						// reset spacing timer

			/* CREATE BLOBULE */

		FindJointFullMatrix(enemy, TICK_JOINT_HEAD, &m);
		OGLPoint3D_Transform(&muzzleOff, &m, &gNewObjectDefinition.coord);	// calc start coord
		OGLVector3D_Transform(&muzzleAim, &m, &aim);						// calc delta/aim vector

		gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
		gNewObjectDefinition.type 		= GLOBAL_ObjType_TickSpit;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_USEALIGNMENTMATRIX;
		gNewObjectDefinition.slot 		= 502;
		gNewObjectDefinition.moveCall 	= MoveTickSpit;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= TICK_SCALE * .7f;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Damage 			= .2;

		newObj->CType 			= CTYPE_HURTME;
		newObj->CBits			= 0;

		CreateCollisionBoxFromBoundingBox(newObj, 1, 1);

		newObj->Delta.x = aim.x * SPIT_SPEED;
		newObj->Delta.y = aim.y * SPIT_SPEED;
		newObj->Delta.z = aim.z * SPIT_SPEED;

		SetAlignmentMatrix(&newObj->AlignmentMatrix, &aim);

		AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 2, 2, false);

		PlayEffect3D(EFFECT_TICKSPIT, &newObj->Coord);
	}
}


/******************* MOVE TICK SPIT ************************/

static void MoveTickSpit(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLVector3D	aim;


	GetObjectInfo(theNode);


	gDelta.y -= 1000.0f * fps;								// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/* HANDLE COLLISIONS */

	if (HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE, -.5))
	{
		MakeSplatter(&gCoord,GLOBAL_ObjType_TickSpit, theNode->Scale.x);
		DeleteObject(theNode);
		return;
	}

			/* UPDATE ALIGNMENT MATRIX */

	FastNormalizeVector(gDelta.x, gDelta.y, gDelta.z, &aim);
	SetAlignmentMatrix(&theNode->AlignmentMatrix, &aim);

	UpdateObject(theNode);
}
























