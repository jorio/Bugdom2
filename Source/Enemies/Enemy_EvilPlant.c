/****************************/
/*   ENEMY: EVILPLANT.C	*/
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

static ObjNode *MakeEvilPlant(float x, float z);
static void MoveEvilPlant(ObjNode *theNode);
static void  MoveEvilPlant_Walk(ObjNode *theNode);
static void  MoveEvilPlant_Grow(ObjNode *theNode);
static void  MoveEvilPlant_Death(ObjNode *theNode);
static void  MoveEvilPlant_Attack(ObjNode *theNode);
static void UpdateEvilPlant(ObjNode *theNode);
static void  MoveEvilPlant_GotHit(ObjNode *theNode);
static Boolean HurtEvilPlant(ObjNode *enemy, float damage);
static void KillEvilPlant(ObjNode *enemy);
static void MoveEvilPlantSeed(ObjNode *theNode);
static void EvilPlantShoot(ObjNode *plant, int jointNum);
static void MovePollenSpore(ObjNode *spore);
static void EvilPlantGotKickedCallback(ObjNode *player, ObjNode *kickedObj);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_EVILPLANTS					2

#define	EVILPLANT_SCALE					2.0f

#define	EVILPLANT_CHASE_DIST_MAX		1100.0f
#define	EVILPLANT_ATTACK_DIST			500.0f

#define	EVILPLANT_TARGET_OFFSET			100.0f

#define EVILPLANT_TURN_SPEED			2.0f
#define EVILPLANT_WALK_SPEED			200.0f

#define	EVILPLANT_HEALTH				1.0f




		/* ANIMS */


enum
{
	EVILPLANT_ANIM_GROW,
	EVILPLANT_ANIM_WALK,
	EVILPLANT_ANIM_ATTACK,
	EVILPLANT_ANIM_GOTHIT,
	EVILPLANT_ANIM_DEATH
};

enum
{
	EVILPLANT_JOINT_HEAD = 2,
	EVILPLANT_JOINT_LEFTHAND = 5,
	EVILPLANT_JOINT_RIGHTHAND = 8
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	ButtTimer			SpecialF[0]

#define	RightShoot			Flag[1]
#define	LeftShoot			Flag[0]

#define	BounceCount			Special[0]


/************************ ADD EVILPLANT ENEMY *************************/
//
// This doesn't actually create the enemy, but rather just an even object that
// waits for the sprinkers to make the plant guy grow.
//

Boolean AddEnemy_EvilPlant(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+100;
	gNewObjectDefinition.moveCall 	= MoveEvilPlantSeed;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->BoundingSphereRadius = 200;

	newObj->Timer = .5f + RandomFloat() * 2.5f;						// delay to grow after sprinklers come on

	return(true);
}


/****************** MOVE EVIL PLANT SEED *****************************/

static void MoveEvilPlantSeed(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	if (gSprinklerMode == SPRINKLER_MODE_ON)				// if sprinkers are on then time to grow
	{
		theNode->Timer -= gFramesPerSecondFrac;
		if (theNode->Timer <= 0.0f)
		{
			MakeEvilPlant(theNode->Coord.x, theNode->Coord.z);	// make plant grow
			DeleteObject(theNode);								// delete this even until next time
		}
	}
}


/************************* MAKE EVILPLANT ****************************/

static ObjNode *MakeEvilPlant(float x, float z)
{
ObjNode	*newObj;
int	i,j;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(nil);

	if (gNumEnemyOfKind[ENEMY_KIND_EVILPLANT] >= MAX_EVILPLANTS)
		return(nil);

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_EVILPLANT, EVILPLANT_ANIM_GROW, x,z, EVILPLANT_SCALE, RandomFloat()*PI2, MoveEvilPlant);

				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= EVILPLANT_HEALTH;
	newObj->Damage 		= .3;
	newObj->Kind 		= ENEMY_KIND_EVILPLANT;

				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,EVILPLANT_TARGET_OFFSET);


	newObj->HurtCallback 		= HurtEvilPlant;						// set hurt callback function
	newObj->GotKickedCallback 	= EvilPlantGotKickedCallback;			// set callback for being kicked



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 3, 3,false);


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
			gSparkles[i].where = newObj->Coord;

			gSparkles[i].aim.x =
			gSparkles[i].aim.y =
			gSparkles[i].aim.z = 1;

			gSparkles[i].color.r = .5;
			gSparkles[i].color.g = .5;
			gSparkles[i].color.b = .5;
			gSparkles[i].color.a = 1;

			gSparkles[i].scale = 70.0f;
			gSparkles[i].separation = 10.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_YellowGlint;
		}
	}


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_EVILPLANT]++;

	return(newObj);
}


#pragma mark -



/********************* MOVE EVILPLANT **************************/

static void MoveEvilPlant(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
		{
			MoveEvilPlant_Grow,
			MoveEvilPlant_Walk,
			MoveEvilPlant_Attack,
			MoveEvilPlant_GotHit,
			MoveEvilPlant_Death,
		};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}




/********************** MOVE EVILPLANT: GROW ******************************/

static void  MoveEvilPlant_Grow(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, EVILPLANT_ANIM_WALK, 4);
	}

				/* UPDATE COORD & COLLISION BOX */

	gCoord.y = GetTerrainY(gCoord.x, gCoord.z) - theNode->BBox.min.y;


	UpdateEvilPlant(theNode);
}



/********************** MOVE EVILPLANT: WALKING ******************************/

static void  MoveEvilPlant_Walk(ObjNode *theNode)
{
float		r,fps,angle,dist;


	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, EVILPLANT_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * EVILPLANT_WALK_SPEED;
	gDelta.z = -cos(r) * EVILPLANT_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == EVILPLANT_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = EVILPLANT_WALK_SPEED * .006f;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


			/* SEE IF ATTACK */

	if (!gGamePrefs.kiddieMode)
	{
		theNode->Timer -= fps;											// dec delay timer
		if (theNode->Timer <= 0.0f)
		{
			dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
			if (dist <= EVILPLANT_ATTACK_DIST)
			{
				MorphToSkeletonAnim(theNode->Skeleton, EVILPLANT_ANIM_ATTACK, .7);
				theNode->Timer = 3.5f;
			}
		}
	}
	UpdateEvilPlant(theNode);
}

/********************** MOVE EVILPLANT: ATTACK ******************************/

static void  MoveEvilPlant_Attack(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;


			/* MOVE */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, EVILPLANT_TURN_SPEED, false);
	ApplyFrictionToDeltas(2000.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*fps;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

		/* SEE IF DONE */

	if (!theNode->Skeleton->IsMorphing)
	{
		theNode->Timer -= fps;
		if (theNode->Timer <= 0.0f)
		{
			theNode->Timer = 3.0f + RandomFloat() * 3.0f;								// set delay timer before can attack again
			MorphToSkeletonAnim(theNode->Skeleton, EVILPLANT_ANIM_WALK, 15);
		}
	}

		/* DO COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


		/* SEE IF SHOOT */

	if (theNode->RightShoot)
	{
		EvilPlantShoot(theNode, EVILPLANT_JOINT_RIGHTHAND);
		theNode->RightShoot = false;
	}
	if (theNode->LeftShoot)
	{
		EvilPlantShoot(theNode, EVILPLANT_JOINT_LEFTHAND);
		theNode->LeftShoot = false;
	}


	UpdateEvilPlant(theNode);
}


/********************** MOVE EVILPLANT: GOT HIT ******************************/

static void  MoveEvilPlant_GotHit(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (theNode->Speed3D > 300.0f)
		TurnObjectTowardTarget(theNode, &gCoord, gCoord.x - gDelta.x, gCoord.z - gDelta.z, 10, false);	// aim with motion

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(800.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*fps;			// add gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* SEE IF DONE */

	theNode->ButtTimer -= fps;
	if (theNode->ButtTimer <= 0.0)
	{
		MorphToSkeletonAnim(theNode->Skeleton, EVILPLANT_ANIM_WALK, 7);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateEvilPlant(theNode);
}




/********************** MOVE EVILPLANT: DEATH ******************************/

static void  MoveEvilPlant_Death(ObjNode *theNode)
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

				/* MOVE IT */

	if (theNode->Speed3D > 200.0f)
		TurnObjectTowardTarget(theNode, &gCoord, gCoord.x - gDelta.x, gCoord.z - gDelta.z, 10, false);	// aim with motion

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)		// if on ground, add friction
		ApplyFrictionToDeltas(600.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*fps;		// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES, true))
		return;


				/* UPDATE */

	UpdateEvilPlant(theNode);


}





/***************** UPDATE EVILPLANT ************************/

static void UpdateEvilPlant(ObjNode *theNode)
{
short			i;
float			r,aimX,aimZ;
const static OGLPoint3D	leftEye = {-9,0,-25};
const static OGLPoint3D	rightEye = {9,0,-25};
OGLMatrix4x4	m;

	CreateCollisionBoxFromBoundingBox(theNode, 1,1);				// call this continuously for evil plant since scale changes
	UpdateEnemy(theNode);


		/***********************/
		/* UPDATE EYE SPARKLES */
		/***********************/

	r = theNode->Rot.y;
	aimX = -sin(r);
	aimZ = -cos(r);

	FindJointFullMatrix(theNode,EVILPLANT_JOINT_HEAD,&m);						// get head matrix


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




/*********************** HURT EVILPLANT ***************************/

static Boolean HurtEvilPlant(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveEvilPlant);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillEvilPlant(enemy);
		return(true);
	}
	else
	{
					/* GO INTO HIT ANIM */

		if (enemy->Skeleton->AnimNum != EVILPLANT_ANIM_GOTHIT)
			MorphToSkeletonAnim(enemy->Skeleton, EVILPLANT_ANIM_GOTHIT, 9);

		enemy->ButtTimer = 3.0f;
	}

	return(false);
}


/****************** KILL EVILPLANT ***********************/

static void KillEvilPlant(ObjNode *enemy)
{
int		i;

	MorphToSkeletonAnim(enemy->Skeleton, EVILPLANT_ANIM_DEATH, 5);
	enemy->CType = 0;
	enemy->HurtCallback 		= nil;
	enemy->GotKickedCallback 	= nil;


			/* DELETE EYE SPARKLES */

	i = enemy->Sparkles[0];
	if (i != -1)
	{
		DeleteSparkle(i);
		enemy->Sparkles[0] = -1;
	}

	i = enemy->Sparkles[1];
	if (i != -1)
	{
		DeleteSparkle(i);
		enemy->Sparkles[1] = -1;
	}
}


/************************* EVIL PLANT GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

static void EvilPlantGotKickedCallback(ObjNode *player, ObjNode *enemy)
{
float	r = player->Rot.y;

	PlayEffect3D(EFFECT_FLYGOTKICKED, &enemy->Coord);

	enemy->Delta.x = -sin(r) * 200.0f;
	enemy->Delta.z = -cos(r) * 200.0f;
	enemy->Delta.y = 600.0f;

	HurtEvilPlant(enemy, .4);
}



#pragma mark -


/****************** EVIL PLANT SHOOT **********************/

static void EvilPlantShoot(ObjNode *plant, int jointNum)
{
OGLMatrix4x4		m;
OGLPoint3D			coord;
OGLVector3D			aim,aim2;
static OGLVector3D	rightAim = {.2,-1,0};
static OGLVector3D	leftAim = {-.2,-1,0};
ObjNode				*spore;
int					i,j;
float				speed;

		/* FIND MATRIX FOR THE HAND @ THE ANIMEVENT */

	if (jointNum == EVILPLANT_JOINT_LEFTHAND)
		FindJointMatrixAtFlagEvent(plant, jointNum, 0, &m);
	else
		FindJointMatrixAtFlagEvent(plant, jointNum, 1, &m);


			/* GET COORD AND AIM VECTOR */

	coord.x = m.value[M03];								// extract coord from matrix
	coord.y = m.value[M13];
	coord.z = m.value[M23];

	if (jointNum == EVILPLANT_JOINT_LEFTHAND)			// get aim for left or right hand
		OGLVector3D_Transform(&leftAim, &m, &aim);
	else
		OGLVector3D_Transform(&rightAim, &m, &aim);


			/********************/
			/* MAKE SOME SPARKS */
			/********************/

	MakeSparkExplosion(coord.x, coord.y, coord.z, 50.0f, 1.0, PARTICLE_SObjType_YellowGlint, 50, 1.5);
	MakePuff(&coord, 14.0, PARTICLE_SObjType_BlackSmoke, GL_SRC_ALPHA, GL_ONE, .5);

	PlayEffect3D(EFFECT_EVILPLANTSHOOT, &coord);


			/**********************/
			/* MAKE POLLEN SPORES */
			/**********************/

	for (j = 0; j < 6; j++)
	{
				/* RANDOM OFFSET TO VECTOR */

		OGLMatrix4x4_SetRotate_XYZ(&m, RandomFloat2() * .2f, RandomFloat2() * .2f, RandomFloat2() * .2f);
		OGLVector3D_Transform(&aim, &m, &aim2);

					/* MAKE POLLEN OBJ */

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;

		if (gLevelNum == LEVEL_NUM_GNOMEGARDEN)
			gNewObjectDefinition.type 	= GARDEN_ObjType_PollenSpore;
		else
			gNewObjectDefinition.type 	= SIDEWALK_ObjType_PollenSpore;

		gNewObjectDefinition.coord		= coord;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
		gNewObjectDefinition.moveCall 	= MovePollenSpore;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= .5f + RandomFloat() * .1f;
		spore = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		spore->BounceCount = 0;

		speed = 400.0f + RandomFloat() * 100.0f;
		spore->Delta.x = aim2.x * speed;
		spore->Delta.y = aim2.y * speed;
		spore->Delta.z = aim2.z * speed;

				/* SET COLLISION STUFF */

		spore->Damage 			= .15f;
		spore->CType 			= 0;
		spore->CBits			= 0;
		CreateCollisionBoxFromBoundingBox(spore,1,1);

				/* MAKE SHADOW */

		AttachShadowToObject(spore, 0, .6,.6, false);


				/* CREATE SPARKLE */

		i = spore->Sparkles[0] = GetFreeSparkle(spore);
		if (i != -1)
		{
			gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL | SPARKLE_FLAG_TRANSFORMWITHOWNER | SPARKLE_FLAG_RANDOMSPIN;
			gSparkles[i].where.x =
			gSparkles[i].where.y =
			gSparkles[i].where.z = 0;

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = .2;

			gSparkles[i].scale = 80.0f;
			gSparkles[i].separation = 20.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_YellowGlint;
		}

	}
}


/************************** MOVE POLLEN SPORE ***************************/

static void MovePollenSpore(ObjNode *spore)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(spore);


	gDelta.y -= 300.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

			/* SEE IF HIT PLAYER */

	if (DoSimpleBoxCollisionAgainstPlayer(gCoord.y - 10.0f, gCoord.y + 10.0f, gCoord.x - 10.0f,
										 gCoord.x + 10.0f, gCoord.z + 10.0f, gCoord.z - 10.0f))
	{
		PlayerGotHit(spore, 0, PLAYER_ANIM_GOTHIT_GENERIC);
		gPlayerInfo.objNode->Delta.x = gDelta.x;
		gPlayerInfo.objNode->Delta.z = gDelta.z;
		gPlayerInfo.objNode->Delta.y = 300.0f;
		goto boom;
	}

			/* SEE IF IMPACTED ANYTHING */

	if (HandleCollisions(spore, CTYPE_TERRAIN | CTYPE_MISC | CTYPE_FENCE, .5))
	{
		spore->BounceCount++;
		if (spore->BounceCount > 1)
		{
boom:
			ExplodeGeometry(spore, 100, SHARD_MODE_BOUNCE, 3, 2.0);
			DeleteObject(spore);
			return;
		}
		else
			spore->Scale.x = spore->Scale.y = spore->Scale.z *= .6f;
	}

	UpdateObject(spore);
}









