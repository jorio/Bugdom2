/****************************/
/*   ENEMY: DRAGONFLY.C	*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLPoint3D				gCoord;
extern	int						gNumEnemies, gScratch;
extern	float					gFramesPerSecondFrac,gGlobalTransparency;
extern	OGLVector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];
extern	u_long		gAutoFadeStatusBits;
extern	SparkleType	gSparkles[];
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	int					gLevelNum,gMaxEnemies;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TerrainItemEntryType 	**gMasterItemList;
extern	short					gNumTerrainItems,gNumTicks;
extern	Boolean					gPlayerIsDead, gFreezeCameraFromY;
extern	ObjNode					*gFirstNodePtr, *gCurrentCarryingMoth;
extern	float					gCurrentMaxSpeed;



/****************************/
/*    PROTOTYPES            */
/****************************/

static ObjNode *MakeDragonfly(float x, float z, short animNum);
static void MoveDragonfly(ObjNode *theNode);
static void  MoveDragonfly_Flying(ObjNode *theNode);
static void  MoveDragonfly_Death(ObjNode *theNode);
static void MoveDragonflyOnSpline(ObjNode *theNode);
static void UpdateDragonfly(ObjNode *theNode);
static void  MoveDragonfly_GotHit(ObjNode *theNode);
static Boolean HurtDragonfly(ObjNode *enemy, float damage);
static void UpdateDragonflySound(ObjNode *theNode, OGLPoint3D *p);

static void KillDragonfly(ObjNode *enemy);
static void DragonflyGotKickedCallback(ObjNode *player, ObjNode *kickedObj);

static void MoveKillerDragonfly(ObjNode *theNode);


/*********************/
/*    CONSTANTS      */
/*********************/

#define	MAX_DRAGONFLYS				7

#define	DRAGONFLY_SCALE				.9f

#define	DRAGONFLY_TARGET_OFFSET		30.0f

#define	DRAGONFLY_CHASE_DIST		1200.0f
#define	DRAGONFLY_ATTACK_DIST		400.0f

#define DRAGONFLY_TURN_SPEED		20.0f
#define	DRAGONFLY_TURN_SPEED2		2.2f

#define DRAGONFLY_WALK_SPEED		350.0f
#define	DRAGONFLY_CHASE_SPEED		200.0f
#define	DRAGONFLY_ATTACK_SPEED		700.0f

#define	DRAGONFLY_HEALTH				1.0f
#define	DRAGONFLY_DAMAGE				.2f

#define	DRAGONFLY_HOVER_HEIGHT			350.0f



enum
{
	DRAGONFLY_ANIM_FLY = 0,
	DRAGONFLY_ANIM_GOTHIT,
	DRAGONFLY_ANIM_DEATH
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	DiveSpeed			SpecialF[0]
#define	ButtTimer			SpecialF[1]
#define	BuzzRate			SpecialF[2]


float	gDragonflyY;

ObjNode	*gKillerDragonFly;


/************************ ADD DRAGONFLY ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Dragonfly(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_DRAGONFLY] >= MAX_DRAGONFLYS)
			return(false);
	}

	newObj = MakeDragonfly(x, z, DRAGONFLY_ANIM_FLY);

	newObj->TerrainItemPtr = itemPtr;

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_DRAGONFLY]++;


	return(true);
}


/************************* MAKE DRAGONFLY ****************************/

static ObjNode *MakeDragonfly(float x, float z, short animNum)
{
ObjNode	*newObj;
float	s;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/

	if (gLevelNum == LEVEL_NUM_BALSA)
		s = DRAGONFLY_SCALE;
	else
		s = DRAGONFLY_SCALE/2;


	newObj = MakeEnemySkeleton(SKELETON_TYPE_DRAGONFLY, animNum, x,z, s, 0, MoveDragonfly);


	if (gLevelNum == LEVEL_NUM_BALSA)
		newObj->Coord.y = gDragonflyY;
	else
		newObj->Coord.y += DRAGONFLY_HOVER_HEIGHT;


				/* SET BETTER INFO */

	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync

	newObj->StatusBits |= STATUS_BIT_NOTEXTUREWRAP;

	newObj->Health 		= DRAGONFLY_HEALTH;
	newObj->Damage 		= DRAGONFLY_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_DRAGONFLY;


				/* SET COLLISION INFO */

	if (gLevelNum != LEVEL_NUM_BALSA)
	{
		newObj->CType |= CTYPE_HURTME;
		newObj->CBits = 0;
		CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	}
	else
		CreateCollisionBoxFromBoundingBox(newObj, 1,2);

	CalcNewTargetOffsets(newObj,DRAGONFLY_TARGET_OFFSET);


	newObj->HurtCallback 		= HurtDragonfly;							// set hurt callback function
	newObj->GotKickedCallback 	= DragonflyGotKickedCallback;			// set callback for being kicked

	newObj->Damage = DRAGONFLY_DAMAGE;


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULARDARK, 8.0f * s, 7.0f * s, false);



	return(newObj);

}





/********************* MOVE DRAGONFLY **************************/

static void MoveDragonfly(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveDragonfly_Flying,
					MoveDragonfly_GotHit,
					MoveDragonfly_Death,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE DRAGONFLY: FLYING ******************************/

static void  MoveDragonfly_Flying(ObjNode *theNode)
{
float		r,fps,y;

	fps = gFramesPerSecondFrac;


			/**************/
			/* BALSA MOVE */
			/**************/

	if (gLevelNum == LEVEL_NUM_BALSA)
	{
			/* AIM AT PLAYER */

		if (!gPlayerIsDead)								// don't aim at player if player is dead
			TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, DRAGONFLY_TURN_SPEED, false);

		r = theNode->Rot.y;
		gDelta.x = -sin(r) * DRAGONFLY_WALK_SPEED;
		gDelta.z = -cos(r) * DRAGONFLY_WALK_SPEED;

		gCoord.x += gDelta.x*fps;
		gCoord.z += gDelta.z*fps;
		gCoord.y = gDragonflyY;
	}


		/***************/
		/* NORMAL MOVE */
		/***************/

	else
	{
		float	dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);


				/* SLOW TO A HALT */

		if (dist > DRAGONFLY_CHASE_DIST)
		{
			theNode->Speed2D -= 400.0f * fps;
			if (theNode->Speed2D < 0.0f)
				theNode->Speed2D = 0;
		}
				/* CHASE */

		else
		if (dist > DRAGONFLY_ATTACK_DIST)
		{
			TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, DRAGONFLY_TURN_SPEED2, true);

			theNode->Speed2D += 200.0f * fps;						// accel
			if (theNode->Speed2D > DRAGONFLY_CHASE_SPEED)
				theNode->Speed2D = DRAGONFLY_CHASE_SPEED;

			r = theNode->Rot.y;										// set delta
			gDelta.x = -sin(r) * theNode->Speed2D;
			gDelta.z = -cos(r) * theNode->Speed2D;

			y = GetTerrainY(gCoord.x, gCoord.z) + DRAGONFLY_HOVER_HEIGHT;
			if (gCoord.y < y)
				gDelta.y += 400.0f * fps;
			else
			{
				gDelta.y -= 1000.0f * fps;
				if (gDelta.y < 0.0f)
					gDelta.y = 0.0f;
			}

		}

				/* ATTACK */
		else
		{
			theNode->Speed2D += 200.0f * fps;						// accel
			if (theNode->Speed2D > DRAGONFLY_ATTACK_SPEED)
				theNode->Speed2D = DRAGONFLY_ATTACK_SPEED;

			r = theNode->Rot.y;
			gDelta.x = -sin(r) * theNode->Speed2D;
			gDelta.z = -cos(r) * theNode->Speed2D;

			if (gCoord.y < gPlayerInfo.coord.y)
				gDelta.y += 600.0f * fps;
			else
				gDelta.y -= 600.0f * fps;
		}

		gCoord.x += gDelta.x*fps;
		gCoord.z += gDelta.z*fps;
		gCoord.y += gDelta.y*fps;
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,CTYPE_MISC|CTYPE_HURTENEMY|CTYPE_ENEMY|CTYPE_FENCE, false))
		return;


	UpdateDragonfly(theNode);
}


/********************** MOVE DRAGONFLY: GOT HIT ******************************/

static void  MoveDragonfly_GotHit(ObjNode *theNode)
{
float		r,fps,aim;

	fps = gFramesPerSecondFrac;


			/* MOVE TOWARD PLAYER */

	if (gPlayerIsDead)								// don't aim at player if player is dead
		aim = PI;
	else
		aim = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, DRAGONFLY_TURN_SPEED, false);


	r = theNode->Rot.y;
	gDelta.x = -sin(r) * DRAGONFLY_WALK_SPEED;
	gDelta.z = -cos(r) * DRAGONFLY_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity

	gCoord.x += gDelta.x*fps;
	gCoord.z += gDelta.z*fps;
	gCoord.y = gDragonflyY;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

				/* SEE IF DONE */

	theNode->ButtTimer -= gFramesPerSecondFrac;
	if (theNode->ButtTimer <= 0.0)
	{
		MorphToSkeletonAnim(theNode->Skeleton, DRAGONFLY_ANIM_FLY, 15);
	}


	UpdateDragonfly(theNode);
}




/********************** MOVE DRAGONFLY: DEATH ******************************/

static void  MoveDragonfly_Death(ObjNode *theNode)
{
const OGLVector3D	v = {0,0,-1};
OGLVector3D			v2;
float	fps = gFramesPerSecondFrac;

				/* INC BUZZ PITCH ON DIVE */

	theNode->BuzzRate += fps * (float)0x4fff;
	ChangeChannelRate(theNode->EffectChannel, (int)theNode->BuzzRate);


	GetObjectInfo(theNode);

			/* NOSE DIVE */

	theNode->Rot.x -= fps * 1.4f;
	if (theNode->Rot.x <= (-PI/3))
		theNode->Rot.x = -PI/3;


				/* MOVE IT */

	OGLVector3D_Transform(&v, &theNode->BaseTransformMatrix, &v2);

	theNode->DiveSpeed += fps * 700.0f;
	gDelta.x = v2.x * theNode->DiveSpeed;
	gDelta.y = v2.y * theNode->DiveSpeed;
	gDelta.z = v2.z * theNode->DiveSpeed;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* BURN */

	BurnPlane(theNode);


		/* SEE IF HIT GROUND */

	if (gCoord.y <= GetTerrainY(gCoord.x, gCoord.z))
	{
		ExplodeGeometry(theNode, 300, SHARD_MODE_BOUNCE, 1, .9);
		DeleteEnemy(theNode);
		MakeFireExplosion(&gCoord);
		PlayEffect_Parms3D(EFFECT_PLANECRASH, &gCoord, NORMAL_CHANNEL_RATE * 3/2, .5);
		return;
	}

				/* UPDATE */

	UpdateDragonfly(theNode);


}


/***************** UPDATE DRAGONFLY ************************/

static void UpdateDragonfly(ObjNode *theNode)
{
	UpdateEnemy(theNode);
	UpdateDragonflySound(theNode, &gCoord);
}


/****************** UPDATE DRAGONFLY SOUND *********************/

static void UpdateDragonflySound(ObjNode *theNode, OGLPoint3D *p)
{
float	v;

	if (gLevelNum == LEVEL_NUM_BALSA)
		v = .8f;
	else
		v = .3f;

	if (theNode->EffectChannel == -1)
	{
		theNode->BuzzRate = NORMAL_CHANNEL_RATE + (MyRandomLong() & 0x1fff);
		theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_DRAGONFLYBUZZ, p,theNode->BuzzRate, v);
	}
	else
		Update3DSoundChannel(EFFECT_DRAGONFLYBUZZ, &theNode->EffectChannel, p);

}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME DRAGONFLY ENEMY *************************/

Boolean PrimeEnemy_Dragonfly(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DRAGONFLY */

	newObj = MakeDragonfly(x,z, DRAGONFLY_ANIM_FLY);


				/* SET BETTER INFO */

	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveDragonflyOnSpline;					// set move call

	newObj->Coord.y 		= gDragonflyY;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE DRAGONFLY ON SPLINE ***************************/

static void MoveDragonflyOnSpline(ObjNode *theNode)
{
Boolean isInRange;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 130);
	GetObjectCoordOnSpline(theNode);


			/* UPDATE STUFF IF IN RANGE */

	if (isInRange)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		gCoord.y = gDragonflyY;
		UpdateObjectTransforms(theNode);															// update transforms
		UpdateShadow(theNode);


				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))
			return;

		UpdateDragonflySound(theNode, &theNode->Coord);

	}
}



#pragma mark -


/************************* DRAGONFLY GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

static void DragonflyGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	PlayEffect3D(EFFECT_FLYGOTKICKED, &kickedObj->Coord);

	kickedObj->Delta.x = -sin(r) * 800.0f;
	kickedObj->Delta.z = -cos(r) * 800.0f;
	kickedObj->Delta.y = 600.0f;

	HurtDragonfly(kickedObj, .5);
}




/*********************** HURT DRAGONFLY ***************************/

static Boolean HurtDragonfly(ObjNode *enemy, float damage)
{
	if (gLevelNum == LEVEL_NUM_BALSA)
		PlayEffect3D(EFFECT_DRAGONFLYHIT, &enemy->Coord);

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveDragonfly);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillDragonfly(enemy);
		return(true);
	}
	else
	{
		MorphToSkeletonAnim(enemy->Skeleton, DRAGONFLY_ANIM_GOTHIT, 10);
		enemy->ButtTimer = 2.5;
	}

	return(false);
}


/****************** KILL DRAGONFLY ***********************/

static void KillDragonfly(ObjNode *enemy)
{
	enemy->CType 				= 0;						// no longer an enemy
	enemy->HurtCallback 		= nil;
	enemy->GotKickedCallback 	= nil;

	MorphToSkeletonAnim(enemy->Skeleton, DRAGONFLY_ANIM_DEATH, 10);
	enemy->DiveSpeed = DRAGONFLY_WALK_SPEED;

//	enemy->TerrainItemPtr = nil;			// dont ever come back

}


#pragma mark -


/****************** SPAWN KILLER DRAGONFLY **********************/

void SpawnKillerDragonfly(void)
{
ObjNode	*newObj;

	if (gKillerDragonFly != nil)								 // see if already have this
		return;

			/****************************/
			/* MAKE NEW SKELETON OBJECT */
			/****************************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_DRAGONFLY;
	gNewObjectDefinition.animNum 	= DRAGONFLY_ANIM_FLY;							// assume default anim is #0
	gNewObjectDefinition.coord.x 	= gPlayerInfo.coord.x;
	gNewObjectDefinition.coord.y 	= gPlayerInfo.coord.y + 700.0f;
	gNewObjectDefinition.coord.z 	= gPlayerInfo.coord.z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 247;
	gNewObjectDefinition.moveCall 	= MoveKillerDragonfly;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	gKillerDragonFly = newObj;
}



/************************ MOVE KILLER DRAGONFLY ************************/

static void MoveKillerDragonfly(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

			/* TOWARD PLAYER */

	if (theNode->Mode == 0)
	{
		gCoord.x = gPlayerInfo.coord.x;
		gCoord.z = gPlayerInfo.coord.z;
		gCoord.y -= fps * 250.0f;

		TurnObjectTowardTarget(theNode, &theNode->OldCoord, gCoord.x, gCoord.z, DRAGONFLY_TURN_SPEED, false);

		if (gCoord.y <= (gPlayerInfo.coord.y + 100.0f))
		{
			theNode->Mode = 1;
			KillPlayer(PLAYER_DEATH_TYPE_KILLERDRAGONFLY);
			gCurrentCarryingMoth = theNode;
			gFreezeCameraFromY = true;
		}
	}


			/* UP, UP AND AWAY */

	else
	{
		float	r = theNode->Rot.y;

		gCoord.y += fps * 250.0f;
		gCoord.x -= sin(r) * (190.0f * fps);
		gCoord.z -= cos(r) * (190.0f * fps);

	}


	UpdateObject(theNode);
}










