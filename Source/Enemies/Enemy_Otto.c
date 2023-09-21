/****************************/
/*   ENEMY: OTTO.C	*/
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
extern	uint32_t		gAutoFadeStatusBits;
extern	SparkleType	gSparkles[];
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	int					gLevelNum,gMaxEnemies;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	TerrainItemEntryType 	**gMasterItemList;
extern	short					gNumTerrainItems,gNumTicks,gNumCollisions;
extern	ObjNode					*gFirstNodePtr;
extern	float					gCurrentMaxSpeed;
extern	CollisionRec			gCollisionList[];
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	PrefsType			gGamePrefs;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void AlignKeyOnOtto(ObjNode *theNode);
static ObjNode *MakeOtto(float x, float z, short animNum);
static void MoveOtto(ObjNode *theNode);
static void MoveOtto_Standing(ObjNode *theNode);
static void  MoveOtto_Walk(ObjNode *theNode);
static void  MoveOtto_Death(ObjNode *theNode);
static void MoveOttoOnSpline(ObjNode *theNode);
static void UpdateOtto(ObjNode *theNode);
static void UpdateOttoSound(ObjNode *theNode, OGLPoint3D *p);
static void  MoveOtto_GotHit(ObjNode *theNode);
static Boolean HurtOtto(ObjNode *enemy, float damage);
static void  MoveOtto_Shoot(ObjNode *theNode);

static void KillOtto(ObjNode *enemy);
static void OttoGotKickedCallback(ObjNode *player, ObjNode *kickedObj);

static void ShootStunPulse(ObjNode *enemy);
static void MoveStunPulse(ObjNode *theNode);
static void ExplodeStunPulse(ObjNode *theNode);
static Boolean SeeIfOttoShoot(ObjNode *theNode, float angleToTarget, float dist);
static void UpdateOttoSparkles(ObjNode *theNode);


/*********************/
/*    CONSTANTS      */
/*********************/

#define	MAX_OTTOS				4

#define	OTTO_SCALE				3.5f

#define	OTTO_DETACH_DIST		800.0f

#define	OTTO_CHASE_DIST_MAX		1300.0f					// dist to start hop chase

#define	OTTO_TARGET_OFFSET		30.0f

#define OTTO_TURN_SPEED			5.0f
#define OTTO_WALK_SPEED			250.0f

#define	OTTO_HEALTH				2.0f
#define	OTTO_DAMAGE				.2f


		/* ANIMS */

enum
{
	OTTO_ANIM_STAND,
	OTTO_ANIM_WALK,
	OTTO_ANIM_SHOOT,
	OTTO_ANIM_GOTHIT,
	OTTO_ANIM_DEATH
};


#define	ATTACK_MIN_ANGLE		(PI/8)
#define	ATTACK_MAX_ANGLE		(PI/6)
#define	ATTACK_DIST				800.0f
#define	ATTACK_DIST_MAX			(ATTACK_DIST + 100.0f)


enum
{
	OTTO_SPARKLE_CHEST = 0,
	OTTO_SPARKLE_MUZZLEFLASH
};

#define	STUN_PULSE_SPEED		2000.0f

enum
{
	OTTO_JOINT_TORSO		=	2,
	OTTO_JOINT_LEFTHAND		=	7
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	SparkleColor 		SpecialF[0]
#define	ButtTimer			SpecialF[2]

#define	ShootNow			Flag[0]					// set by animation

const OGLPoint3D	gMuzzleTipOff = {-1,-35, -11};



/************************ ADD OTTO ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Otto(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_OTTO] >= MAX_OTTOS)
			return(false);
	}

	newObj = MakeOtto(x, z, OTTO_ANIM_STAND);

	newObj->TerrainItemPtr = itemPtr;

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_OTTO]++;


	return(true);
}


/************************* MAKE OTTO ****************************/

static ObjNode *MakeOtto(float x, float z, short animNum)
{
ObjNode	*newObj, *key;
int		i;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_OTTO, animNum, x,z, OTTO_SCALE, 0, MoveOtto);



				/* SET BETTER INFO */

	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync

	newObj->StatusBits |= STATUS_BIT_NOTEXTUREWRAP;

	newObj->Health 		= OTTO_HEALTH;
	newObj->Damage 		= OTTO_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_OTTO;

	newObj->What 		= WHAT_SOLDIER;

				/* SET COLLISION INFO */

	newObj->BoundingSphereRadius *= 2.0f;							// otto is big, so give him more room
	CreateCollisionBoxFromBoundingBox(newObj, .8,1);
	CalcNewTargetOffsets(newObj,OTTO_TARGET_OFFSET);


	newObj->HurtCallback 		= HurtOtto;							// set hurt callback function
	newObj->GotKickedCallback 	= OttoGotKickedCallback;			// set callback for being kicked

	newObj->Damage = OTTO_DAMAGE;


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 6, 6, false);


			/**************/
			/* ATTACH KEY */
			/**************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PLAYROOM_ObjType_OttoKey;
	gNewObjectDefinition.coord		= newObj->Coord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= OTTO_SCALE;
	key = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	key->Rot.y = PI;

	newObj->ChainNode = key;
	key->ChainHead = newObj;


			/***********************/
			/* CREATE CHEST LIGHTS */
			/***********************/

	i = newObj->Sparkles[OTTO_SPARKLE_CHEST] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where.x = newObj->Coord.x;
		gSparkles[i].where.y = newObj->Coord.y + 100.0f;
		gSparkles[i].where.z = newObj->Coord.z;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = .3;
		gSparkles[i].color.a = .7;

		gSparkles[i].scale = 130.0f;
		gSparkles[i].separation = 30.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark4;
	}

	return(newObj);
}





/********************* MOVE OTTO **************************/

static void MoveOtto(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveOtto_Standing,
					MoveOtto_Walk,
					MoveOtto_Shoot,
					MoveOtto_GotHit,
					MoveOtto_Death,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE OTTO: STANDING ******************************/

static void  MoveOtto_Standing(ObjNode *theNode)
{
float	angleToTarget;
float	dist;

				/* CALC ANGLE AND DIST TO PLAYER */

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, 0, false);
	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);


				/* SEE IF SHOOT OR CHASE */

	if (!gGamePrefs.kiddieMode)
	{
		if (!SeeIfOttoShoot(theNode, angleToTarget, dist))
		{
			if (dist < OTTO_CHASE_DIST_MAX)
			{
				gDelta.x = gDelta.z = 0;
				MorphToSkeletonAnim(theNode->Skeleton, OTTO_ANIM_WALK, 5);
			}
		}
	}
				/**********************/
				/* DO ENEMY COLLISION */
				/**********************/

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateOtto(theNode);
}




/********************** MOVE OTTO: WALKING ******************************/

static void  MoveOtto_Walk(ObjNode *theNode)
{
float		r,fps,aim,dist;

	fps = gFramesPerSecondFrac;


			/* MOVE TOWARD PLAYER */

	aim = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, OTTO_TURN_SPEED, false);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * OTTO_WALK_SPEED;
	gDelta.z = -cos(r) * OTTO_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;
	dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);


				/* SEE IF SHOOT OR STAND */

	if (!SeeIfOttoShoot(theNode, aim, dist))
	{
		if ((dist > OTTO_CHASE_DIST_MAX) || gGamePrefs.kiddieMode)
		{
			MorphToSkeletonAnim(theNode->Skeleton, OTTO_ANIM_STAND, 2.0);
			theNode->ShootNow = false;
		}
	}
				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	UpdateOtto(theNode);
}




/********************** MOVE OTTO: SHOOT ******************************/

static void  MoveOtto_Shoot(ObjNode *theNode)
{
float	angleToTarget, dist;

	gDelta.x = gDelta.z = gDelta.y = 0;

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, OTTO_TURN_SPEED, false);
	dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);

			/********************/
			/* SEE IF SHOOT NOW */
			/********************/

	if (theNode->ShootNow)
	{
		theNode->ShootNow = false;
		ShootStunPulse(theNode);
	}

			/* SEE IF PLAYER OUT OF RANGE */

	if ((angleToTarget > (ATTACK_MAX_ANGLE)) || (dist > ATTACK_DIST_MAX))
	{
		MorphToSkeletonAnim(theNode->Skeleton, OTTO_ANIM_STAND, 8);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateOtto(theNode);
}



/********************** MOVE OTTO: GOT HIT ******************************/

static void  MoveOtto_GotHit(ObjNode *theNode)
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
		MorphToSkeletonAnim(theNode->Skeleton, OTTO_ANIM_STAND, 6);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateOtto(theNode);
}




/********************** MOVE OTTO: DEATH ******************************/

static void  MoveOtto_Death(ObjNode *theNode)
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

	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES, false))
		return;


				/* UPDATE */

	UpdateOtto(theNode);


}



/***************** UPDATE OTTO ************************/

static void UpdateOtto(ObjNode *theNode)
{
	UpdateEnemy(theNode);
	AlignKeyOnOtto(theNode);
	UpdateOttoSound(theNode, &gCoord);
	UpdateOttoSparkles(theNode);
}

/****************** UPDATE OTTO SOUND *********************/

static void UpdateOttoSound(ObjNode *theNode, OGLPoint3D *p)
{
	if (theNode->Skeleton->AnimNum == OTTO_ANIM_DEATH)
	{
		StopAChannel(&theNode->EffectChannel);						// make sure no more mechanical sounds if dead
	}
	else
	{
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_OTTOMOTOR, p, NORMAL_CHANNEL_RATE, .3);
		else
			Update3DSoundChannel(EFFECT_OTTOMOTOR, &theNode->EffectChannel, p);
	}
}

/***************** ALIGN KEY ON OTTO *********************/

static void AlignKeyOnOtto(ObjNode *theNode)
{
ObjNode	*key = theNode->ChainNode;
OGLMatrix4x4	m,m2;


	key->Rot.z += gFramesPerSecondFrac * 6.0f;

	OGLMatrix4x4_SetRotate_XYZ(&m, 0,key->Rot.y, key->Rot.z);
	m.value[M03] = 0;
	m.value[M13] = 20;
	m.value[M23] = 4;

	FindJointFullMatrix(theNode, 3, &m2);							// get matrix of vertebrae

	OGLMatrix4x4_Multiply(&m, &m2, &key->BaseTransformMatrix);

	SetObjectTransformMatrix(key);

	key->Coord.x = key->BaseTransformMatrix.value[M03];			// extract coord from matrix
	key->Coord.y = key->BaseTransformMatrix.value[M13];
	key->Coord.z = key->BaseTransformMatrix.value[M23];

}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME OTTO ENEMY *************************/

Boolean PrimeEnemy_Otto(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE OTTO */

	newObj = MakeOtto(x,z, OTTO_ANIM_WALK);


				/* SET BETTER INFO */

	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveOttoOnSpline;					// set move call

	newObj->Coord.y 		-= newObj->BottomOff;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE OTTO ON SPLINE ***************************/

static void MoveOttoOnSpline(ObjNode *theNode)
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


				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < OTTO_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveOtto);


					/* UPDATE STUFF */

		AlignKeyOnOtto(theNode);
		UpdateOttoSparkles(theNode);
	}


	UpdateOttoSound(theNode, &theNode->Coord);						// update sfx
}



#pragma mark -


/************************* OTTO GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

static void OttoGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	PlayEffect3D(EFFECT_FLYGOTKICKED, &kickedObj->Coord);

	kickedObj->Delta.x = -sin(r) * 800.0f;
	kickedObj->Delta.z = -cos(r) * 800.0f;
	kickedObj->Delta.y = 600.0f;

	HurtOtto(kickedObj, .5);
}




/*********************** HURT OTTO ***************************/

static Boolean HurtOtto(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveOtto);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillOtto(enemy);
		return(true);
	}
	else
	{
		MorphToSkeletonAnim(enemy->Skeleton, OTTO_ANIM_GOTHIT, 10);
		enemy->ButtTimer = 2.5;
	}

	return(false);
}


/****************** KILL OTTO ***********************/

static void KillOtto(ObjNode *enemy)
{
	enemy->CType 				= CTYPE_MISC;				// no longer an enemy
	enemy->HurtCallback 		= nil;
	enemy->GotKickedCallback 	= nil;

	MorphToSkeletonAnim(enemy->Skeleton, OTTO_ANIM_DEATH, 4);

	enemy->TerrainItemPtr = nil;			// dont ever come back

}

#pragma mark -

/***************** SHOOT STUN PULSE ************************/

static void ShootStunPulse(ObjNode *enemy)
{
ObjNode	*newObj;
int				i;
OGLPoint3D		muzzleCoord;
OGLVector3D		aim;
OGLMatrix4x4	m;
const OGLVector3D	muzzleTipAim = {0, -1, -.3};


		/* CALC COORD & VECTOR OF MUZZLE */

	FindJointFullMatrix(enemy,OTTO_JOINT_LEFTHAND,&m);
	OGLPoint3D_Transform(&gMuzzleTipOff, &m, &muzzleCoord);
	OGLVector3D_Transform(&muzzleTipAim, &m, &aim);

	PlayEffect3D(EFFECT_OTTOSHOOT, &muzzleCoord);



		/*********************/
		/* MAKE MUZZLE FLASH */
		/*********************/

	if (enemy->Sparkles[OTTO_SPARKLE_MUZZLEFLASH] != -1)							// see if delete existing sparkle
	{
		DeleteSparkle(enemy->Sparkles[OTTO_SPARKLE_MUZZLEFLASH]);
		enemy->Sparkles[OTTO_SPARKLE_MUZZLEFLASH] = -1;
	}

	i = enemy->Sparkles[OTTO_SPARKLE_MUZZLEFLASH] = GetFreeSparkle(enemy);		// make new sparkle
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = muzzleCoord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 100.0f;
		gSparkles[i].separation = 10.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_BlueSpark;
	}



				/*********************/
				/* CREATE LASER BEAM */
				/*********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PLAYROOM_ObjType_OttoStunPulse;
	gNewObjectDefinition.coord		= muzzleCoord;
	gNewObjectDefinition.flags 		= STATUS_BIT_USEALIGNMENTMATRIX|STATUS_BIT_GLOW|STATUS_BIT_NOZWRITES|
									STATUS_BIT_NOFOG|STATUS_BIT_DOUBLESIDED|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveStunPulse;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .7;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ColorFilter.a = .99;			// do this just to turn on transparency so it'll glow

	newObj->Delta.x = aim.x * STUN_PULSE_SPEED;
	newObj->Delta.y = aim.y * STUN_PULSE_SPEED;
	newObj->Delta.z = aim.z * STUN_PULSE_SPEED;

	newObj->Health = 2.0f;
	newObj->Damage = .1f;


				/* SET COLLISION */

	newObj->CType 			= 0;
	newObj->CBits			= 0;
	CreateCollisionBoxFromBoundingBox_Maximized(newObj);


			/* SET THE ALIGNMENT MATRIX */

	SetAlignmentMatrix(&newObj->AlignmentMatrix, &aim);
}


/******************* MOVE STUN PULSE ***********************/

static void MoveStunPulse(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;

			/* SEE IF GONE */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}


	GetObjectInfo(theNode);

			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if (HandleCollisions(theNode, CTYPE_MISC | CTYPE_PLAYER | CTYPE_FENCE | CTYPE_TERRAIN, 0))
	{
			/* SEE IF HIT PLAYER */
		for (i = 0; i < gNumCollisions; i++)
		{
			if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
			{
				ObjNode	*player = gCollisionList[i].objectPtr;
				if (player == gPlayerInfo.objNode)					// see if this is the player
				{
					PlayerGotHit(theNode, 0, PLAYER_ANIM_GOTHIT_GENERIC);
				}
			}
		}

		ExplodeStunPulse(theNode);
		return;
	}


	UpdateObject(theNode);
}



/********************* EXPLODE STUN PULSE ***********************/

static void ExplodeStunPulse(ObjNode *theNode)
{
long					pg,i;
OGLVector3D				delta;
NewParticleDefType		newParticleDef;


			/**************/
			/*  PARTICLES */
			/**************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 400;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 12.0f;
	gNewParticleGroupDef.decayRate				= 2.0;
	gNewParticleGroupDef.fadeRate				= 0;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_BlueSpark;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 30; i++)
		{
			delta.x = RandomFloat2() * 250.0f;
			delta.y = RandomFloat2() * 250.0f;
			delta.z = RandomFloat2() * 250.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &gCoord;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= RandomFloat() + .5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA;
			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}

	PlayEffect3D(EFFECT_LASERBOOM, &gCoord);

	DeleteObject(theNode);									// delete the pulse

}


/************************ SEE IF OTTO SHOOT ****************************/

static Boolean SeeIfOttoShoot(ObjNode *theNode, float angleToTarget, float dist)
{
	if ((angleToTarget < ATTACK_MIN_ANGLE) && (dist < ATTACK_DIST))
	{
		if (!SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))	// dont attack thru things
		{
			MorphToSkeletonAnim(theNode->Skeleton, OTTO_ANIM_SHOOT, 5);
			theNode->ShootNow = false;
			return(true);
		}
	}
	return(false);
}

#pragma mark -


/*************** UPDATE OTTO SPARKLES ********************/

static void UpdateOttoSparkles(ObjNode *theNode)
{
short			i;
OGLMatrix4x4	m;
float			fps = gFramesPerSecondFrac;

		/*****************************/
		/* UPDATE THE CHEST SPARKLES */
		/*****************************/

	i = theNode->Sparkles[OTTO_SPARKLE_CHEST];								// get sparkle index
	if (i != -1)
	{
		const static OGLPoint3D	p = {0,0,0};

				/* GET MATRIX FOR JOINT */

		FindJointFullMatrix(theNode, OTTO_JOINT_TORSO, &m);


				/* CALC COORD */

		OGLPoint3D_Transform(&p, &m, &gSparkles[i].where);


		theNode->SparkleColor += fps * 10.0f;
		gSparkles[i].color.r = (1.0f + sin(theNode->SparkleColor)) * .5f;
	}


			/*************************/
			/* UPDATE MUZZLE SPARKLE */
			/*************************/

	i = theNode->Sparkles[OTTO_SPARKLE_MUZZLEFLASH];								// get sparkle index
	if (i != -1)
	{
		FindCoordOnJoint(theNode, OTTO_JOINT_LEFTHAND, &gMuzzleTipOff, &gSparkles[i].where);

		gSparkles[i].color.a -= fps * 4.0f;						// fade out
		if (gSparkles[i].color.a <= 0.0f)
		{
			DeleteSparkle(i);
			theNode->Sparkles[OTTO_SPARKLE_MUZZLEFLASH] = -1;
		}
	}


}



