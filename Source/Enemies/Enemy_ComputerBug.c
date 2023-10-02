/****************************/
/*   ENEMY: COMPUTERBUG.C	*/
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

static ObjNode *MakeComputerBug(float x, float z, short animNum);
static void MoveComputerBug(ObjNode *theNode);
static void  MoveComputerBug_Stand(ObjNode *theNode);
static void  MoveComputerBug_Walk(ObjNode *theNode);
static void  MoveComputerBug_Drop(ObjNode *theNode);
static void MoveComputerBugOnSpline(ObjNode *theNode);
static void UpdateComputerBug(ObjNode *theNode);
static void UpdateComputerBugEyes(ObjNode *theNode);
static void SeeIfDropVirus(ObjNode *enemy);
static void DropVirus(ObjNode *enemy);
static void MoveVirus(ObjNode *theNode);
static Boolean DoTrig_Virus(ObjNode *virus, ObjNode *who, Byte sideBits);
static void MakeVirus(OGLPoint3D *where, OGLVector3D *delta, float scale);
static Boolean DoTrig_ComputerBug(ObjNode *enemy, ObjNode *who, Byte sideBits);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_COMPUTERBUGS				4

#define	COMPUTERBUG_SCALE				1.8f

#define	COMPUTERBUG_CHASE_DIST_MAX		1900.0f
#define	COMPUTERBUG_DETACH_DIST			400.0f

#define	COMPUTERBUG_TARGET_OFFSET		40.0f

#define COMPUTERBUG_TURN_SPEED			1.0f
#define COMPUTERBUG_WALK_SPEED			200.0f

#define	COMPUTERBUG_HEALTH				1.0f
#define	COMPUTERBUG_DAMAGE				.2f



		/* ANIMS */

enum
{
	COMPUTERBUG_ANIM_STAND,
	COMPUTERBUG_ANIM_WALK,
	COMPUTERBUG_ANIM_DROP
};


#define	COMPUTERBUG_JOINT_HEAD				8

#define	NORMAL_LED_BLINK_SPEED				1.0f
#define FAST_LED_BLINK_SPEED				3.0f
#define	LED_BLINK_DELAY						.3f
#define	VIRUS_BLINK_DELAY					(LED_BLINK_DELAY/2)

#define	MAX_VIRI						30
#define	VIRUS_DETONATE_DIST				300.0f
#define	VIRUS_SCALE						(COMPUTERBUG_SCALE)
#define	VIRUS_DAMAGE					.2f

/*********************/
/*    VARIABLES      */
/*********************/

#define	DropNow				Flag[0]
#define	LEDOn				Flag[1]
#define	LEDTimer			Timer
#define LEDBlinkSpeed		SpecialF[0]
#define	DropTimer			SpecialF[1]
#define	ButtTimer			SpecialF[2]

int		gNumViri = 0;


/************************ ADD COMPUTERBUG ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_ComputerBug(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_COMPUTERBUG] >= MAX_COMPUTERBUGS)
			return(false);
	}

	newObj = MakeComputerBug(x, z, COMPUTERBUG_ANIM_STAND);

	newObj->TerrainItemPtr = itemPtr;

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_COMPUTERBUG]++;


	return(true);
}


/************************* MAKE COMPUTERBUG ****************************/

static ObjNode *MakeComputerBug(float x, float z, short animNum)
{
ObjNode	*newObj;
int		j,i;

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_COMPUTERBUG,animNum, x,z, COMPUTERBUG_SCALE, 0, MoveComputerBug);



				/* SET BETTER INFO */

	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync

	newObj->StatusBits |= STATUS_BIT_NOTEXTUREWRAP;

	newObj->Health 		= COMPUTERBUG_HEALTH;
	newObj->Damage 		= COMPUTERBUG_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_COMPUTERBUG;


				/* SET COLLISION INFO */

	newObj->ForceLookAtDist	= 500.0f;
	newObj->CType |= CTYPE_LOOKAT;

	newObj->TriggerCallback = DoTrig_ComputerBug;
	newObj->CType 			|= CTYPE_TRIGGER;

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,COMPUTERBUG_TARGET_OFFSET);

	newObj->Timer = RandomFloat();

	newObj->Damage = COMPUTERBUG_DAMAGE;


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
			gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
			gSparkles[i].where = newObj->Coord;

			gSparkles[i].color.r = .8;
			gSparkles[i].color.g = .8;
			gSparkles[i].color.b = .8;
			gSparkles[i].color.a = 1;

			gSparkles[i].scale = 40.0f;
			gSparkles[i].separation = 10.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_RedGlint;
		}
	}

	newObj->LEDOn = true;
	newObj->LEDTimer = LED_BLINK_DELAY * RandomFloat();
	newObj->LEDBlinkSpeed = NORMAL_LED_BLINK_SPEED;

	return(newObj);

}





/********************* MOVE COMPUTERBUG **************************/

static void MoveComputerBug(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveComputerBug_Stand,
					MoveComputerBug_Walk,
					MoveComputerBug_Drop,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE COMPUTERBUG: STANDING ******************************/

static void  MoveComputerBug_Stand(ObjNode *theNode)
{
float	dist;
float	fps = gFramesPerSecondFrac;

	ApplyFrictionToDeltas(2000.0,&gDelta);

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, COMPUTERBUG_TURN_SPEED, false);

				/* SEE IF CHASE */

	if (!gGamePrefs.kiddieMode)
	{
		if (!theNode->Skeleton->IsMorphing)
		{
			if (!IsWaterInFrontOfEnemy(theNode->Rot.y))				// dont chase if we're in front of water
			{
				dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
				if (dist < COMPUTERBUG_CHASE_DIST_MAX)
				{
					MorphToSkeletonAnim(theNode->Skeleton, COMPUTERBUG_ANIM_WALK, 5);
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

	UpdateComputerBug(theNode);
}

/********************** MOVE COMPUTERBUG: WALK ******************************/

static void  MoveComputerBug_Walk(ObjNode *theNode)
{
float		r,fps,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, COMPUTERBUG_TURN_SPEED, false);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * COMPUTERBUG_WALK_SPEED;
	gDelta.z = -cos(r) * COMPUTERBUG_WALK_SPEED;

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


	dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	if ((dist > COMPUTERBUG_CHASE_DIST_MAX) || gGamePrefs.kiddieMode)
	{
		MorphToSkeletonAnim(theNode->Skeleton, COMPUTERBUG_ANIM_STAND, 6.0);
	}


			/* SEE IF DROP VIRUS */

	SeeIfDropVirus(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	UpdateComputerBug(theNode);
}



/********************** MOVE COMPUTERBUG: DROP ******************************/

static void  MoveComputerBug_Drop(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* MOVE */

	ApplyFrictionToDeltas(2000.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*fps;									// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

			/* SEE IF DROP NOW */

	if (theNode->DropNow)
	{
		theNode->DropNow = false;
		DropVirus(theNode);
	}

			/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton, COMPUTERBUG_ANIM_STAND, 6);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateComputerBug(theNode);
}






/***************** UPDATE COMPUTERBUG ************************/

static void UpdateComputerBug(ObjNode *theNode)
{
	UpdateEnemy(theNode);

	UpdateComputerBugEyes(theNode);


}

/***************** UPDATE COMPUTERBUG EYES ************************/

static void UpdateComputerBugEyes(ObjNode *theNode)
{
short			i, j;
float			r,aimX,aimZ, dist;
static const OGLPoint3D	eyeOff[2] = {-12.0, 9.0, -9.0,
									  12.0, 9.0, -9.0};
OGLMatrix4x4	m;

			/*****************/
			/* UPDATE BLINKY */
			/*****************/

				/* CALC BLINK SPEED */

	dist = CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	if (dist < 500.0f)
		theNode->LEDBlinkSpeed = FAST_LED_BLINK_SPEED;
	else
		theNode->LEDBlinkSpeed = NORMAL_LED_BLINK_SPEED;



				/* CHECK TIMER */

	theNode->LEDTimer -= gFramesPerSecondFrac * theNode->LEDBlinkSpeed;
	if (theNode->LEDTimer <= 0.0f)
	{
		if (theNode->LEDOn)								// if on, then turn off
		{
			theNode->LEDOn = false;
			theNode->LEDTimer = LED_BLINK_DELAY/2;		// off for half duration
		}
		else											// if off, then turn on
		{
			theNode->LEDOn = true;
			theNode->LEDTimer = LED_BLINK_DELAY;
		}

				/* SET ALPHA TO TURN ON/OFF */

		for (j = 0; j < 2; j++)
		{
			i = theNode->Sparkles[j];												// get sparkle index
			if (i != -1)
			{
				if (theNode->LEDOn)
					gSparkles[i].color.a = 1.0f;
				else
					gSparkles[i].color.a = 0.0f;
			}
		}

	}

			/******************/
			/* UPDATE SPARKLE */
			/******************/

	if (theNode->LEDOn)
	{
		r = theNode->Rot.y;
		aimX = -sin(r);
		aimZ = -cos(r);

		FindJointFullMatrix(theNode,COMPUTERBUG_JOINT_HEAD,&m);						// get head matrix


			/* UPDATE RIGHT EYE */

		for (j = 0; j < 2; j++)
		{
			i = theNode->Sparkles[j];												// get sparkle index
			if (i != -1)
			{
				OGLPoint3D_Transform(&eyeOff[j], &m, &gSparkles[i].where);			// calc coord of right eye
				gSparkles[i].aim.x = aimX;											// update aim vector
				gSparkles[i].aim.z = aimZ;
			}
		}
	}
}



//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME COMPUTERBUG ENEMY *************************/

Boolean PrimeEnemy_ComputerBug(int splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&gSplineList[splineNum], placement, &x, &z);


				/* MAKE COMPUTERBUG */

	newObj = MakeComputerBug(x,z, COMPUTERBUG_ANIM_WALK);


				/* SET BETTER INFO */

	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveComputerBugOnSpline;					// set move call

	newObj->Coord.y 		-= newObj->BottomOff;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE COMPUTERBUG ON SPLINE ***************************/

static void MoveComputerBugOnSpline(ObjNode *theNode)
{
Boolean isInRange;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 100);
	GetObjectCoordOnSpline(theNode);


			/* UPDATE STUFF IF IN RANGE */

	if (isInRange)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// calc y coord
		UpdateObjectTransforms(theNode);															// update transforms
		UpdateShadow(theNode);
		UpdateComputerBugEyes(theNode);

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < COMPUTERBUG_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveComputerBug);
	}
}



#pragma mark -




/****************** COMPUTERBUG TOUCHED PLAYER *********************/
//
// Player gets zapped when touches this.
//

void ComputerBugTouchedPlayer(ObjNode *enemy, ObjNode *player)
{
OGLVector2D	v;
OGLPoint3D	*pc, *ec;
OGLVector3D *pd;

	if (gPlayerInfo.invincibilityTimer > 0.0f)
		return;

			/********************/
			/* ENEMY HIT PLAYER */
			/********************/

	if (gCurrentNode == enemy)
	{
		pc = &player->Coord;
		ec = &gCoord;
		pd = &player->Delta;
	}

			/********************/
			/* PLAYER HIT ENEMY */
			/********************/
	else
	{
		pc = &gCoord;
		ec = &enemy->Coord;
		pd = &gDelta;
	}



			/* KNOCK PLAYER */

	v.x = pc->x - ec->x;											// calc vector to player
	v.y = pc->z - ec->z;
	FastNormalizeVector2D(v.x, v.y, &v, true);

	gSolidTriggerKeepDelta = true;
	pd->x = v.x * 1000.0f;
	pd->z = v.y * 1000.0f;
	pd->y = 800;

	PlayerGotHit(enemy, 0, PLAYER_ANIM_GOTHIT_BACKFLIP);

	gPlayerInfo.invincibilityTimer = 3.0;							// set this for short duration


			/* MAKE BOOM */

	MakeSparkExplosion(pc->x, pc->y, pc->z, 200, 1.0, PARTICLE_SObjType_BlueSpark, 100, 1.0);
	MakeSparkExplosion(pc->x, pc->y, pc->z, 150, 1.0, PARTICLE_SObjType_WhiteSpark4, 100, .8);


	PlayEffect_Parms3D(EFFECT_MINEBOOM, &gCoord, NORMAL_CHANNEL_RATE * 3/2, 1.0);
}


/************** DO TRIGGER - COMPUTER BUG ********************/

static Boolean DoTrig_ComputerBug(ObjNode *enemy, ObjNode *who, Byte sideBits)
{
	(void) sideBits;

	ComputerBugTouchedPlayer(enemy, who);

	return(true);
}



#pragma mark -

/******************* SEE IF DROP VIRUS *************************/

static void SeeIfDropVirus(ObjNode *enemy)
{
float	dist;

	if (gNumViri >= MAX_VIRI)
		return;

				/* SEE IF IN RANGE */

	dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	if (dist > 2500.0f)
		return;

				/* SEE IF TIME TO SQUAT */

	enemy->DropTimer -= gFramesPerSecondFrac;
	if (enemy->DropTimer > 0.0f)
		return;

	MorphToSkeletonAnim(enemy->Skeleton, COMPUTERBUG_ANIM_DROP, 3);
	enemy->DropNow = false;
	enemy->DropTimer = 1.5f + RandomFloat();

	PlayEffect3D(EFFECT_SERVO2, &gCoord);
}


/******************** DROP VIRUS *************************/

static void DropVirus(ObjNode *enemy)
{
OGLPoint3D	where;
OGLVector3D	delta;
float		scale = VIRUS_SCALE;

	(void) enemy;

	where.x = gCoord.x;
	where.z = gCoord.z;
	where.y = GetTerrainY(gCoord.x, gCoord.z) - gObjectGroupBBoxList[MODEL_GROUP_LEVELSPECIFIC][CLOSET_ObjType_Virus].min.y * scale;

	delta.x = delta.y = delta.z = 0;

	MakeVirus(&where, &delta, scale);
}


/******************* MAKE VIRUS ****************************/

static void MakeVirus(OGLPoint3D *where, OGLVector3D *delta, float scale)
{
ObjNode	*newObj;
int		i;

	if (gNumViri >= MAX_VIRI)									// make sure don't overload
		return;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOSET_ObjType_Virus;
	gNewObjectDefinition.coord		= *where;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveVirus;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= scale;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->Delta = *delta;

			/* SET COLLISION STUFF */

	newObj->Damage			= VIRUS_DAMAGE;
	newObj->TriggerCallback = DoTrig_Virus;
	newObj->CType 			= CTYPE_MISC|CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 1.5, 1.5, false);

	newObj->Health = 30.0f;											// how long it lives before being cullable

	gNumViri++;														// inc # of viruses

			/**************/
			/* ATTACH LED */
			/**************/

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL | SPARKLE_FLAG_TRANSFORMWITHOWNER;
		gSparkles[i].where.x = 0;
		gSparkles[i].where.z = 0;
		gSparkles[i].where.y = 0;

		gSparkles[i].color.r =
		gSparkles[i].color.g =
		gSparkles[i].color.b = .8;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 90.0f;
		gSparkles[i].separation = 15.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_GreenGlint;
	}

	newObj->LEDOn = true;
	newObj->LEDTimer = LED_BLINK_DELAY * RandomFloat();
	newObj->LEDBlinkSpeed = NORMAL_LED_BLINK_SPEED;



}


/****************** MOVE VIRUS *************************/

static void MoveVirus(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;

		/* SEE IF GONE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		if (theNode->StatusBits & STATUS_BIT_ISCULLED)
		{
			DeleteObject(theNode);
			gNumViri--;
			return;
		}
	}

			/********/
			/* MOVE */
			/********/

	GetObjectInfo(theNode);

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// do friction if on ground
		ApplyFrictionToDeltas(1000.0,&gDelta);

	gDelta.y -= 2000.0f * fps;						// gravity
	gCoord.x += gDelta.x * fps;						// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


				/* COLLISION */

	HandleCollisions(theNode, CTYPE_MISC | CTYPE_FENCE | CTYPE_TERRAIN, .5);




			/******************/
			/* UPDATE SPARKLE */
			/******************/

				/* CHECK TIMER */

	theNode->LEDTimer -= fps * theNode->LEDBlinkSpeed;
	if (theNode->LEDTimer <= 0.0f)
	{
		if (theNode->LEDOn)								// if on, then turn off
		{
			theNode->LEDOn = false;
			theNode->LEDTimer = VIRUS_BLINK_DELAY/2;		// off for half duration
		}
		else											// if off, then turn on
		{
			theNode->LEDOn = true;
			theNode->LEDTimer = VIRUS_BLINK_DELAY;
		}

				/* SET ALPHA TO TURN ON/OFF */

		i = theNode->Sparkles[0];						// get sparkle index
		if (i != -1)
		{
			if (theNode->LEDOn)
				gSparkles[i].color.a = 1.0f;
			else
				gSparkles[i].color.a = 0.0f;
		}
	}


	UpdateObject(theNode);

}


/************** DO TRIGGER - VIRUS ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Virus(ObjNode *virus, ObjNode *who, Byte sideBits)
{
OGLVector3D	delta;
OGLPoint3D	where = virus->Coord;

	(void) sideBits;
	(void) who;

	if (virus->StatusBits & STATUS_BIT_ONGROUND)				// only trigger when on ground
	{
		PlayerGotHit(virus, 0, PLAYER_ANIM_GOTHIT_GENERIC);

				/* BLOW UP THIS VIRUS */

		PlayEffect3D(EFFECT_MINEBOOM, &virus->Coord);
		MakeSparkExplosion(virus->Coord.x, virus->Coord.y, virus->Coord.z, 300.0f, .7, PARTICLE_SObjType_BlueGlint, 40, 1.0);
		ExplodeGeometry(virus, 300, SHARD_MODE_FROMORIGIN | SHARD_MODE_BOUNCE, 1, 1.0);
		DeleteObject(virus);
		gNumViri--;

					/* MAKE MORE */

		for (int i = 0; i < 3; i++)
		{
			delta.x = RandomFloat2() * 300.0f;
			delta.y = 500.0f + RandomFloat() * 400.0f;
			delta.z = RandomFloat2() * 300.0f;

			MakeVirus(&where, &delta, VIRUS_SCALE);
		}
	}

	return(true);
}





