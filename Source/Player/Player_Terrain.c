/*******************************/
/*   	PLAYER_SKIP.C		   */
/* (c)2002 Pangea Software     */
/* By Brian Greenstone         */
/*******************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "3dmath.h"
#include "bones.h"
#include "infobar.h"

extern	OGLBoundingBox			gWaterBBox[];
extern	ObjNode					*gFirstNodePtr, *gCurrentCarryingMoth, *gSuckingVacuume;
extern	float					gFramesPerSecondFrac,gFramesPerSecond,gCameraLookAtYOff,gGlobalTransparency;
extern	OGLPoint3D				gCoord;
extern	OGLVector3D				gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	short					gNumCollisions;
extern	CollisionRec			gCollisionList[];
extern	PrefsType				gGamePrefs;
extern	OGLBoundingBox			gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	OGLVector3D				gRecentTerrainNormal;
extern	OGLMatrix4x4			gWorldToWindowMatrix;
extern	PlayerInfoType			gPlayerInfo;
extern	Boolean					gFreezeCameraFromXZ,gEnableSnakes;
extern	int						gLevelNum,gScratch;
extern	ParticleGroupType		*gParticleGroups[];
extern	SpriteType			*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	float				gPlayerToCameraAngle,gDeathTimer,gGravity,gMinHeightOffGround,gCameraDistFromMe;
extern	Boolean					gResetGliding,gDoGlidingAtApex;
extern	WaterDefType			*gWaterList;
extern	MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	OGLColorRGB			gGlobalColorFilter;

/****************************/
/*    PROTOTYPES            */
/****************************/


static void MovePlayer_Jump(ObjNode *theNode);

static Boolean DoSkipCollisionDetect(ObjNode *theNode, Boolean useBBoxForTerrain);
static void CheckPlayerActionControls(ObjNode *theNode);
static Boolean CheckPlayerJumpControl(ObjNode *theNode);
static void DoPlayerFrictionAndGravity_Terrain(ObjNode *theNode, float friction);
static Boolean DoPlayerMovementAndCollision_Ramming(ObjNode *theNode, Boolean useBBoxForTerrain);
static Boolean DoPlayerMovementAndCollision(ObjNode *theNode, Boolean useBBoxForTerrain);
static void MovePlayer_Stand(ObjNode *theNode);
static void MovePlayer_Walk(ObjNode *theNode);
static void MovePlayer_Gliding(ObjNode *theNode);
static void MovePlayer_Landing(ObjNode *theNode);
static void MovePlayer_Pickup(ObjNode *theNode);
static void MovePlayer_Fall(ObjNode *theNode);
static void MovePlayer_Kick(ObjNode *theNode);
static void MovePlayer_Swim(ObjNode *theNode);
static void MovePlayer_GotHit(ObjNode *theNode);
static void MovePlayer_GetUpFromHit(ObjNode *theNode);
static void MovePlayer_DropObject(ObjNode *theNode);
static void MovePlayer_Ramming(ObjNode *theNode);
static void MovePlayer_Push(ObjNode *theNode);
static void MovePlayer_Death(ObjNode *theNode);
static void MovePlayer_Drown(ObjNode *theNode);
static void MovePlayer_Carried(ObjNode *player);
static void MovePlayer_VacuumeSuck(ObjNode *theNode);

static float CalcWalkAnimSpeed(ObjNode *theNode);
static Boolean TryToGrabAPickup(ObjNode *player);

static void TurnPlayerTowardPickup(ObjNode *player);
static void TurnPlayerTowardKickable(ObjNode *player);
static void UpdatePlayerOnTerrain(ObjNode *theNode);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	PLAYER_WATER_FRICTION	200.0f
#define	PLAYER_AIR_FRICTION		800.0f	//400.0f
#define	PLAYER_LANDING_FRICTION	600.0f
#define	PLAYER_DEFAULT_FRICTION	4000.0f	//2500.0f
#define	PLAYER_HEAVY_FRICTION	5000.0f


#define	JUMP_DELTA				2000.0f
#define	HOP_DELTA				1300.0f

#define	DELTA_SUBDIV			8.0f				// smaller == more subdivisions per frame

#define	CONTROL_SENSITIVITY		3000.0f //2200.0f
#define	CONTROL_SENSITIVITY_AIR	5000.0f

#define	WALK_STAND_THRESHOLD	0.3f

#define	KEY_THRUST				4000.0f

/*********************/
/*    VARIABLES      */
/*********************/

#define	PickupNow			Flag[0]
#define	KickHitNow			Flag[0]
#define	BlinkNowFlag 		Flag[1]



float			gPlayerBottomOff = 0;

short			gPlayerMultiPassCount = 0;


//
// In order to let the player move faster than the max speed, we use a current and target value.
// the target is what we want the max to normally be, and the current is what it currently is.
// So, when the player needs to go faster, we modify Current and then slowly decay it back to Target.
//

float	gTargetMaxSpeed = PLAYER_NORMAL_MAX_SPEED;
float	gCurrentMaxSpeed = PLAYER_NORMAL_MAX_SPEED;


/*************************** CREATE PLAYER MODEL: TERRAIN ****************************/
//
// Creates an ObjNode for the player
//
// INPUT:
//			where = floor coord where to init the player.
//			rotY = rotation to assign to player if oldObj is nil.
//

void CreatePlayerModel_Terrain(OGLPoint3D *where, float rotY)
{
ObjNode	*newObj;

		/*****************************/
		/* MAKE  SKELETON BODY */
		/*****************************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_SKIP_EXPLORE;
	gNewObjectDefinition.animNum	= PLAYER_ANIM_STAND;
	gNewObjectDefinition.coord.x 	= where->x;
	gNewObjectDefinition.coord.z 	= where->z;
	gNewObjectDefinition.coord.y 	= FindHighestCollisionAtXZ(where->x,where->z, CTYPE_MISC|CTYPE_TERRAIN);
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_DONTCULL|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= PLAYER_SLOT;
	gNewObjectDefinition.moveCall	= MovePlayer_Terrain;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= PLAYER_DEFAULT_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);



	newObj->Rot.y = rotY;


				/* SET COLLISION INFO */

	newObj->CType = CTYPE_PLAYER;
	newObj->CBits = CBITS_ALLSOLID;

	SetObjectCollisionBounds(newObj, newObj->BBox.max.y, gPlayerBottomOff = newObj->BBox.min.y, -18, 18, 18, -18);


		/*******************/
		/* SET OTHER STUFF */
		/*******************/

	gTargetMaxSpeed = PLAYER_NORMAL_MAX_SPEED;
	gCurrentMaxSpeed = PLAYER_NORMAL_MAX_SPEED;

	gResetGliding = true;

	SetPlayerStandAnim(newObj, 10);						// just make sure we're standing appropriately

	newObj->Coord.y += newObj->BBox.min.y;				// offset y so foot is on ground
	UpdateObjectTransforms(newObj);

	gPlayerInfo.objNode 	= newObj;
	gPlayerInfo.coord		= newObj->Coord;


		/* SPECIAL CASE BALSA FLYING */

	if (gLevelNum == LEVEL_NUM_BALSA)
		PutPlayerInBalsaPlane(newObj);
	else
		AttachShadowToObject(newObj, 0, DEFAULT_PLAYER_SHADOW_SCALE,DEFAULT_PLAYER_SHADOW_SCALE * .8f, true);

}


/******************** MOVE PLAYER: SKIP ***********************/

void MovePlayer_Terrain(ObjNode *theNode)
{
	static void(*myMoveTable[])(ObjNode *) =
	{
		MovePlayer_Stand,							// personality
		MovePlayer_Walk,							// walking
		MovePlayer_Jump,							// jump
		MovePlayer_Landing,							// landing
		MovePlayer_Gliding,							// gliding
		MovePlayer_Pickup,							// pickup

		MovePlayer_Walk,							// walk & carry
		MovePlayer_Stand,							// stand & carry
		MovePlayer_Jump,							// jump & carry
		MovePlayer_Fall,							// fall & carry
		MovePlayer_Fall,							// fall

		MovePlayer_DropObject,						// drop object
		MovePlayer_Kick,							// kick
		MovePlayer_Landing,							// land & carry

		MovePlayer_Swim,							// swim
		MovePlayer_WalkOnBall,						// walk on ball
		MovePlayer_GotHit,							// got hit: backflip
		MovePlayer_Gliding,							// glide & carry

		MovePlayer_GetUpFromHit,					// get up from hit: backflip
		MovePlayer_GotHit,							// got hit: generic
		MovePlayer_EatenBySnake,					// eaten by snake

		MovePlayer_Ramming,							// ramming
		nil,										// drive slot car
		MovePlayer_Push,							// push

		MovePlayer_Stand,							// personality 1
		MovePlayer_Death,							// death
		MovePlayer_Drown,							// drown

		MovePlayer_Stand,							// stand
		MovePlayer_Stand,							// personality carry
		MovePlayer_Stand,							// personality carry
		MovePlayer_Stand,							// personality dance
		nil,										// clover toss
		nil,										// fly balsa plane
		nil,										// fall from plane

		MovePlayer_Carried,							// carried by moth
		MovePlayer_VacuumeSuck,						// sucked by vaccume
	};


	GetObjectInfo(theNode);

	SeeIfLaunchBuddyBug();							// always check buddy bug launches no matter what anim we're in


			/* JUMP TO HANDLER */

	if (myMoveTable[theNode->Skeleton->AnimNum] != nil)
		myMoveTable[theNode->Skeleton->AnimNum](theNode);


}



/******************** MOVE PLAYER: STAND ***********************/

static void MovePlayer_Stand(ObjNode *theNode)
{
	theNode->Rot.x = 0;								// make sure this is correct
	theNode->Rot.z = 0;								// make sure this is correct

			/* MOVE PLAYER */

	DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_HEAVY_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;



			/* SEE IF SHOULD WALK OR DO PERSONALITY */

	if (IsPlayerDoingStandAnim(theNode))
	{
		if (CalcWalkAnimSpeed(theNode) >= WALK_STAND_THRESHOLD)		// see if walk
			SetPlayerWalkAnim(theNode);
		else
			UpdatePersonality(theNode);

	}

			/* DO ACTION CONTROLS */
			//
			// do this last since we want any jump command to work smoothly
			//

	CheckPlayerActionControls(theNode);




			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}


/******************** MOVE PLAYER: WALK ***********************/

static void MovePlayer_Walk(ObjNode *theNode)
{
	theNode->Rot.x = 0;								// make sure this is correct
	theNode->Rot.z = 0;								// make sure this is correct


			/* MOVE PLAYER */

	DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_DEFAULT_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;


			/* SEE IF SHOULD STAND */

	if (IsPlayerDoingWalkAnim(theNode))
	{
		float	animSpeed =  CalcWalkAnimSpeed(theNode);

		if (animSpeed < WALK_STAND_THRESHOLD)
			SetPlayerStandAnim(theNode, 8);
		else
			theNode->Skeleton->AnimSpeed = animSpeed;
	}


			/* DO ACTION CONTROL */

	CheckPlayerActionControls(theNode);





			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}

/******************* CALC WALK ANIM SPEED **********************/

static float CalcWalkAnimSpeed(ObjNode *theNode)
{
float	speed = CalcQuickDistance(0,0,gDelta.x, gDelta.z) * .007f;
#pragma unused (theNode)

	return(speed);
}


/******************** MOVE PLAYER: JUMP ***********************/

static void MovePlayer_Jump(ObjNode *theNode)
{


			/* DO CONTROL */

	CheckPlayerActionControls(theNode);


			/* MOVE PLAYER */

	DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_AIR_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;


	if (IsPlayerDoingJumpAnim(theNode))							// only bother if still in Fall anim
	{
				/* SEE IF LANDED */

		if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		{
			SetPlayerLandAnim(theNode);
			gDelta.x *= .6f;
			gDelta.z *= .6f;
		}
				/* SEE IF FALLING */
		else
		if (gDelta.y < 0.0f)
		{
			if (gDoGlidingAtApex)							// see if we wanted to glide @ the apex
				StartPlayerGliding(theNode);
//			else
//				SetPlayerFallAnim(theNode);
		}

	}


			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}


/******************** MOVE PLAYER: FALL ***********************/

static void MovePlayer_Fall(ObjNode *theNode)
{
			/* MOVE PLAYER */

	DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_AIR_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;


	if (IsPlayerDoingFallAnim(theNode))								// only bother if still in Fall anim
	{
				/* SEE IF LANDED */

		if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		{
			SetPlayerLandAnim(theNode);
			gDelta.x *= .8f;
			gDelta.z *= .8f;
		}

	}


			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}


/******************** MOVE PLAYER: LANDING ***********************/

static void MovePlayer_Landing(ObjNode *theNode)
{

	theNode->Skeleton->AnimSpeed = 1.5f;							// set speed of anim

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no control during this anim

	DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_LANDING_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;



			/* DO ACTION CONTROL */

	CheckPlayerActionControls(theNode);



			/*****************/
			/* SEE IF LANDED */
			/*****************/

	if (IsPlayerDoingLandAnim(theNode))							// only bother if still in landing anim
	{
		if (theNode->Skeleton->AnimHasStopped)
		{
			SetPlayerStandAnim(theNode, 8);						// default to standing when landing
		}
	}


			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}


/******************** MOVE PLAYER: GLIDING ***********************/

static void MovePlayer_Gliding(ObjNode *theNode)
{

			/* MOVE PLAYER */

	DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_AIR_FRICTION);
	gDelta.y = -60.0f;														// always glide down slowly

			/* SEE IF END GLIDING */

	if (gControlNeeds[kNeed_Jump].newButtonPress || (theNode->StatusBits & STATUS_BIT_ONGROUND))
		EndGliding(theNode);

			/* DO ACTION CONTROL */
	else
		CheckPlayerActionControls(theNode);


	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;



			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}



/******************** MOVE PLAYER: PICKUP ***********************/

static void MovePlayer_Pickup(ObjNode *theNode)
{

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no control during this anim

	if (gPlayerInfo.heldObject == nil)							// turn toward target
		TurnPlayerTowardPickup(theNode);

	DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_DEFAULT_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;


		/* SEE IF DO PICKUP ACTION NOW */

	if (theNode->PickupNow)
	{
		theNode->PickupNow = false;
		if (!TryToGrabAPickup(theNode))							// see if pickup anything
		{
			SeeIfLiftMousetrapLever(theNode);					// didn't pickup anything, so see if lift mousetrap level
		}
	}


			/* SEE IF DONE */

	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_PICKUP)		// only bother if still in this anim
		if (theNode->Skeleton->AnimHasStopped)
			SetPlayerStandAnim(theNode, 8);						// default to standing when landing


			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}

/******************** MOVE PLAYER: DROP OBJECT ***********************/

static void MovePlayer_DropObject(ObjNode *theNode)
{

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no control during this anim

	DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_DEFAULT_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;


			/* SEE IF DONE */

	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_DROPOBJECT)				// only bother if still in this anim
		if (theNode->Skeleton->AnimHasStopped)
			SetPlayerStandAnim(theNode, 8);									// default to standing when landing


			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}

/******************** MOVE PLAYER: KICK ***********************/

static void MovePlayer_Kick(ObjNode *theNode)
{

	theNode->Skeleton->AnimSpeed = 1.3;

			/* SEE IF TRIGGER THE KICK */

	if (theNode->KickHitNow)
	{
		if (PlayerDoKick(theNode) > 0)
			theNode->KickHitNow = false;							// once we've hit something, clear the flag
	}


			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no control during this anim

	TurnPlayerTowardKickable(theNode);

	DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_HEAVY_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;


			/* SEE IF DONE */

	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_KICK)					// only bother if still in this anim
		if (theNode->Skeleton->AnimHasStopped)
			SetPlayerStandAnim(theNode, 8);								// default to standing when landing


			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}


/******************** MOVE PLAYER: SWIM ***********************/

static void MovePlayer_Swim(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* HURT IF IN POOL WATER */

	if (gWaterList[gPlayerInfo.waterPatch].type == WATER_TYPE_POOLWATER)
	{
		if (PlayerLoseHealth(fps * .1f, PLAYER_DEATH_TYPE_DROWN))
			goto update;
	}

	gPlayerInfo.analogControlX *= .5f;							// make controls sluggish in water
	gPlayerInfo.analogControlZ *= .5f;

			/* MOVE PLAYER */

	DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_WATER_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;

	if (IsPlayerDoingSwimAnim(theNode))
	{
			/* LEAVE RIPPLE */

		theNode->Timer -= fps;
		if (theNode->Timer < 0.0f)
		{
			theNode->Timer += .05f + RandomFloat() * .15f;
			CreateNewRipple(gCoord.x + RandomFloat2() * 30.0f, gCoord.z + RandomFloat2() * 30.0f, 10.0f, 80.0f, .6);
		}

			/* SEE IF EXIT WATER */

		if (!(theNode->StatusBits & STATUS_BIT_UNDERWATER))
		{
			SetPlayerStandAnim(theNode, 8);
		}

				/* DO ACTION CONTROL */
		else
		{
			CheckPlayerJumpControl(theNode);							// only allow jump actions when swimming
		}

	}


			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}


/******************** MOVE PLAYER: GOT HIT ***********************/

static void MovePlayer_GotHit(ObjNode *theNode)
{
			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no control during this anim

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_DEFAULT_FRICTION);
	else
		DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_AIR_FRICTION);

	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;


			/* TURN PLAYER */

	if ((gDelta.x != 0.0f) && (gDelta.z != 0.0f))
	{
		OGLVector2D	v;

		v.x = gDelta.x;
		v.y = gDelta.z;
		OGLVector2D_Normalize(&v, &v);
		TurnObjectTowardTarget(theNode, &gCoord, gCoord.x - v.x, gCoord.z - v.y, 30, false);
	}


			/* SEE IF SHOULD WALK */

	theNode->Timer -= gFramesPerSecondFrac;
	if (theNode->Timer <= 0.0f)
	{
		if (theNode->Skeleton->AnimNum == PLAYER_ANIM_GOTHIT_BACKFLIP)
			SetSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_GETUPFROMHIT_BACKFLIP);
		else
			SetPlayerStandAnim(theNode, 5);
	}

			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}


/******************** MOVE PLAYER: GET UP FROM HIT ***********************/

static void MovePlayer_GetUpFromHit(ObjNode *theNode)
{
	theNode->Skeleton->AnimSpeed = 1.5;	//------

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no control during this anim

	DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_DEFAULT_FRICTION);

	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;


			/* SEE IF SHOULD WALK */

	if (theNode->Skeleton->AnimHasStopped)
	{
		SetPlayerStandAnim(theNode, 20);
	}

			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}


/******************** MOVE PLAYER: DEATH ***********************/

static void MovePlayer_Death(ObjNode *theNode)
{
//float	oldTimer;

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no control during this anim

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_DEFAULT_FRICTION);
	else
		DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_AIR_FRICTION);

			/* TURN PLAYER */

	if ((gDelta.x != 0.0f) && (gDelta.z != 0.0f))
	{
		OGLVector2D	v;

		v.x = gDelta.x;
		v.y = gDelta.z;
		OGLVector2D_Normalize(&v, &v);
		TurnObjectTowardTarget(theNode, &gCoord, gCoord.x - v.x, gCoord.z - v.y, 30, false);
	}


	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;


			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);


#if 0
			/* SEE IF RESET PLAYER NOW */

	oldTimer = gDeathTimer;
	gDeathTimer -= gFramesPerSecondFrac;
	if (gDeathTimer <= 0.0f)
	{
		if (oldTimer > 0.0f)						// if just now crossed zero then start fade
			MakeFadeEvent(false);
		else
		if (gGammaFadePercent <= 0.0f)				// once fully faded out reset player @ checkpoint
			ResetPlayerAtBestCheckpoint();
	}
#endif
}

/******************** MOVE PLAYER: DROWN ***********************/

static void MovePlayer_Drown(ObjNode *theNode)
{
float	oldTimer;

#pragma unused (theNode)

			/* SEE IF RESET PLAYER NOW */

	oldTimer = gDeathTimer;
	gDeathTimer -= gFramesPerSecondFrac;
	if (gDeathTimer <= 0.0f)
	{
		if (oldTimer > 0.0f)						// if just now crossed zero then start fade
			MakeFadeEvent(false, 1);
		else
		if (gGammaFadePercent <= 0.0f)				// once fully faded out reset player @ checkpoint
			ResetPlayerAtBestCheckpoint();
	}

}


/******************** MOVE PLAYER: RAMMING ***********************/

static void MovePlayer_Ramming(ObjNode *theNode)
{
	theNode->Skeleton->AnimSpeed = theNode->Speed2D * .001f;

			/* SEE IF DONE RAMMING */

	gPlayerInfo.rammingTimer -= gFramesPerSecondFrac;
	if (gPlayerInfo.rammingTimer <= 0.0f)
	{
		SetPlayerWalkAnim(theNode);
	}


			/* MOVE PLAYER */

	DoPlayerFrictionAndGravity_Terrain(theNode, 0);

	if (DoPlayerMovementAndCollision_Ramming(theNode, false))
		goto update;


			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}


/******************** MOVE PLAYER: PUSH ***********************/

static void MovePlayer_Push(ObjNode *theNode)
{
			/* MOVE PLAYER */

	DoPlayerFrictionAndGravity_Terrain(theNode, PLAYER_DEFAULT_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, false))
		goto update;

			/* SEE IF STOP */

	if (IsPlayerDoingPushAnim(theNode))							// if still pushing
	{
		gPlayerInfo.pushTimer -= gFramesPerSecondFrac;
		if (gPlayerInfo.pushTimer < 0.0f)
			PlayerStopPushingObject(theNode);
	}
	else														// not pushing, so be sure push object is stopped
		PlayerStopPushingObject(theNode);

			/* UPDATE IT */

update:
	UpdatePlayerOnTerrain(theNode);
}



/******************** MOVE PLAYER: CARRIED ***********************/

static void MovePlayer_Carried(ObjNode *player)
{
			/* SEE IF PLAYER WAS DROPPED */

	if (gCurrentCarryingMoth == nil)
	{
		gDelta.x = gDelta.y = gDelta.z = 0;
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_FALL, 5);
		return;
	}

			/* UPDATE MY LOCATION */

	gCoord.x = gCurrentCarryingMoth->Coord.x;
	gCoord.y = gCurrentCarryingMoth->Coord.y - 80.0f;
	gCoord.z = gCurrentCarryingMoth->Coord.z;

	player->Rot.y = gCurrentCarryingMoth->Rot.y;

	UpdatePlayerOnTerrain(player);
}



/******************** MOVE PLAYER: VACUUME SUCK ***********************/

static void MovePlayer_VacuumeSuck(ObjNode *theNode)
{
OGLVector2D	v;
float		fps = gFramesPerSecondFrac;
float		vacX, vacZ, dist, r;

			/* CALC VECTOR TO VACUUME */

	r = gSuckingVacuume->Rot.y;
	vacX = gSuckingVacuume->Coord.x - sin(r) * VACUUME_SUCK_OFF;
	vacZ = gSuckingVacuume->Coord.z - cos(r) * VACUUME_SUCK_OFF;


	v.x = vacX - gCoord.x;
	v.y = vacZ - gCoord.z;
	FastNormalizeVector2D(v.x, v.y, &v, true);


			/* ACCEL TOWARD IT */

	gPlayerInfo.suckSpeed += 500.0f * fps;

	gDelta.y -= gGravity*fps;					// add gravity
	gDelta.x = v.x * gPlayerInfo.suckSpeed;
	gDelta.z = v.y * gPlayerInfo.suckSpeed;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* SET AIM */

	TurnObjectTowardTarget(theNode, &gCoord, vacX, vacZ, 7.0, false);


			/****************/
			/* DO COLLISION */
			/****************/

			/* DO GENERAL STUFF */

	theNode->BottomOff = theNode->BBox.min.y;
	HandleCollisions(theNode, CTYPE_TERRAIN | CTYPE_FENCE, 0);


			/* UPDATE IT */

	UpdatePlayerOnTerrain(theNode);


		/* SEE IF FAR ENOUGH INTO VACUUME */

	dist = CalcQuickDistance(vacX, vacZ, gCoord.x, gCoord.z);
	if (dist < 150.0f)
	{
		gPlayerInfo.invincibilityTimer = 0.0;						// gotta set to 0 to get hurt
		PlayerGotHit(gSuckingVacuume, 0, PLAYER_ANIM_GOTHIT_GENERIC);
		gPlayerInfo.objNode->Timer = 2.0;							// set player's hurt timer longer than normal
		PlayEffect3D(EFFECT_VACUUMECRUMCH, &gCoord);
	}
	else
	if (dist > (VACUUME_SUCK_DIST * 1.5f))							// see if escaped
	{
		SetPlayerStandAnim(theNode, 4);
	}
	else
		gPlayerInfo.invincibilityTimer = 3.0;						// keep this set while being vacuumed
}



#pragma mark -


/*********************** TURN PLAYER TOWARD KICKABLE ****************************/

static void TurnPlayerTowardKickable(ObjNode *player)
{
ObjNode *thisNode,*nearest;
float	ex,ey,ez,dist,bestDist;

	bestDist = 10000000;
	nearest = nil;


			/* SCAN FOR NEAREST PUNCHABLE IN FRONT */

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->CType & CTYPE_KICKABLE)
		{
			ex = thisNode->Coord.x;								// get obj coords
			ey = thisNode->Coord.y;
			ez = thisNode->Coord.z;

			dist = CalcDistance(gCoord.x, gCoord.z, ex, ez);
			if ((dist < bestDist) && (dist < 130.0f))			// see if best dist & close enough
			{
				bestDist = dist;
				nearest = thisNode;
			}
		}
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);


			/* THERE IS SOMETHING THERE */

	if (nearest)
	{
		TurnObjectTowardTarget(player, &gCoord, nearest->Coord.x,
								nearest->Coord.z, 6.0, false);
	}


}

/*********************** TURN PLAYER TOWARD PICKUP ****************************/

static void TurnPlayerTowardPickup(ObjNode *player)
{
ObjNode *thisNode,*nearest;
float	ex,ey,ez,dist,bestDist;

	bestDist = 10000000;
	nearest = nil;


			/* SCAN FOR NEAREST PICKUP IN FRONT */

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->CType & CTYPE_PICKUP)
		{
			ex = thisNode->Coord.x;								// get obj coords
			ey = thisNode->Coord.y;
			ez = thisNode->Coord.z;

			dist = CalcDistance(gCoord.x, gCoord.z, ex, ez);
			if ((dist < bestDist) && (dist < 130.0f))			// see if best dist & close enough
			{
				bestDist = dist;
				nearest = thisNode;
			}
		}
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);


			/* THERE IS SOMETHING THERE */

	if (nearest)
	{
		TurnObjectTowardTarget(player, &gCoord, nearest->Coord.x,
								nearest->Coord.z, 6.0, false);
	}


}


/********************* TRY TO GRAB A PICKUP **************************/

static Boolean TryToGrabAPickup(ObjNode *player)
{
ObjNode *thisNode,*nearest;
float	dist,bestDist;
OGLVector2D	aim;

	aim.x = -sin(player->Rot.y);								// calc player aim vector
	aim.y = -cos(player->Rot.y);

	bestDist = 10000000;
	nearest = nil;


			/************************************/
			/* SCAN FOR NEAREST PICKUP IN FRONT */
			/************************************/

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (!(thisNode->CType & CTYPE_PICKUP))					// only look for PICKUPS
			goto next;
		if (thisNode->StatusBits & STATUS_BIT_HIDDEN)			// ... that are visible
			goto next;

		dist = OGLPoint3D_Distance(&gCoord, &thisNode->Coord);
		if ((dist < bestDist) && (dist < 120.0f))				// see if best dist & close enough
		{
			bestDist = dist;
			nearest = thisNode;
		}

next:
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);


			/*****************************************/
			/* THERE IS SOMETHING THERE SO DO PICKUP */
			/*****************************************/

	if (nearest)
	{
		switch(nearest->Kind)
		{
			case	PICKUP_KIND_POW:							// powerups don't actually get picked up
					DoTrig_Powerup(nearest, player, 0);			// POW's get triggered when picked up
					break;

			case	PICKUP_KIND_CANTAB:							// pop the soda can tab
					PopSodaCanTab(nearest);
					break;

			default:
					gPlayerInfo.heldObject = nearest;
					nearest->StatusBits |= STATUS_BIT_NOMOVE|STATUS_BIT_NOCOLLISION;		// disable pickup's move & collide since player has control now
		}

		return(true);
	}

	return(false);
}


#pragma mark -

/************************ UPDATE PLAYER: SKIP ***************************/

static void UpdatePlayerOnTerrain(ObjNode *theNode)
{
const float fps = gFramesPerSecondFrac;


			/* UPDATE COLLISION BOX TOP */

	theNode->TopOff = theNode->BBox.max.y;


			/*****************************/
			/* UPDATE FINAL SPEED VALUES */
			/*****************************/

			/* SET APPROPRIATE MAX SPEED */

	switch(theNode->Skeleton->AnimNum)
	{
		case	PLAYER_ANIM_SWIM:							// max speed is pretty slow when swimming
				gTargetMaxSpeed = PLAYER_NORMAL_MAX_SPEED / 3;
				break;

		case	PLAYER_ANIM_VACUUMESUCK:
				gTargetMaxSpeed = PLAYER_NORMAL_MAX_SPEED * 4;
				break;

		default:
				gTargetMaxSpeed = PLAYER_NORMAL_MAX_SPEED;
	}

	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);


			/* UPDATE CURRENT MAX SPEED */

	if (theNode->Speed2D < gTargetMaxSpeed)					// if we're less than the target, then just reset current to target
		gCurrentMaxSpeed = gTargetMaxSpeed;
	else
	if (gCurrentMaxSpeed > gTargetMaxSpeed)					// see if in overdrive, so readjust currnet
	{
		if (theNode->Speed2D < gCurrentMaxSpeed)			// we're slower than Current, so adjust current down to us
		{
			gCurrentMaxSpeed = theNode->Speed2D;
		}
	}

	theNode->Speed3D = CalcVectorLength(&gDelta);


		/* CALC Y-ROTATION BASED ON DELTA VECTOR */
		//
		// We want the player to continue aiming in the direction of thrust when
		// pushed up against a solid object.  So, if going slow then turn woard accel vector
		// otherwise, turn toward actual delta.  Turning at the accel vector will cause
		// some minor moon-walking.
		//

	switch(theNode->Skeleton->AnimNum)
	{
		case	PLAYER_ANIM_VACUUMESUCK:
				break;

		default:
				if (theNode->Speed2D > (PLAYER_NORMAL_MAX_SPEED/4))
					TurnObjectTowardTarget(theNode, &gCoord, gCoord.x+gDelta.x, gCoord.z+gDelta.z, theNode->Speed2D*.02f, false);
				else														// if going really slow then aim in acceleration direction instead
					TurnObjectTowardTarget(theNode, &gCoord, gCoord.x+theNode->AccelVector.x, gCoord.z+theNode->AccelVector.y, theNode->Speed2D*.02f, false);
	}


		/* UPDATE OBJECT AS LONG AS NOT BEING MATRIX CONTROLLED */

	switch(theNode->Skeleton->AnimNum)
	{
//		case	PLAYER_ANIM_GRABBED:
//		case	PLAYER_ANIM_GRABBED2:
//		case	PLAYER_ANIM_GRABBEDBYSTRONGMAN:
//		case	PLAYER_ANIM_ROCKETSLED:
//				break;

		default:
				UpdateObject(theNode);
	}

	gPlayerInfo.coord = gCoord;				// update player coord



			/****************/
			/* UPDATE WINGS */
			/****************/

	UpdateGlidingWings(theNode);


		/* CHECK INV TIMER */

	gPlayerInfo.invincibilityTimer -= fps;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)		// if on ground then reset gliding
		gResetGliding = true;


			/****************/
			/* UPDATE BLINK */
			/****************/

	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_DEATH)		// put X's on eyes if dead
	{
		theNode->Skeleton->overrideTexture[1] = gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_SkipDead].materialObject;
		gPlayerInfo.blinkTimer = 0;							// set blink timer to 0 so that X's will go away automatically when out of death anim
	}
	else
	{
		if (theNode->BlinkNowFlag)								// see if blink now from anim
		{
			gPlayerInfo.blinkTimer = 0;
			theNode->BlinkNowFlag = false;
		}

		gPlayerInfo.blinkTimer -= fps;
		if (gPlayerInfo.blinkTimer <= 0.0f)						// see if in blink
		{
			if (gPlayerInfo.blinkTimer < -0.15f)				// see if stop blink now
			{
				theNode->Skeleton->overrideTexture[1] = nil;
				gPlayerInfo.blinkTimer = .5f + RandomFloat() * 3.0f;
			}
			else
				theNode->Skeleton->overrideTexture[1] = gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_SkipBlink].materialObject;
		}
	}

			/**********************/
			/* UPDATE HELD OBJECT */
			/**********************/

	UpdateHeldObject(theNode);



			/* SEE IF CROSSED ANY MARKERS */

	HandlePlayerLineMarkerCrossing(theNode);


		/* UPDATE SHIELD */

	UpdatePlayerShield();
}


#pragma mark -

/******************* DO PLAYER MOVEMENT AND COLLISION DETECT *********************/
//
// OUTPUT: true if disabled or killed
//

static Boolean DoPlayerMovementAndCollision(ObjNode *theNode, Boolean useBBoxForTerrain)
{
float				fps = gFramesPerSecondFrac,oldFPS,oldFPSFrac,terrainY;
OGLPoint3D			oldCoord;
OGLMatrix3x3		m;
static OGLPoint2D origin = {0,0};
int					numPasses,pass;
Boolean				killed = false;


				/*******************************/
				/* DO CAMERA-RELATIVE CONTROLS */
				/*******************************/

			/* ROTATE ANALOG ACCELERATION VECTOR BASED ON CAMERA POS & APPLY TO DELTA */

	if ((gPlayerInfo.analogControlX == 0.0f) && (gPlayerInfo.analogControlZ == 0.0f))	// see if not acceling
	{
		theNode->AccelVector.x = theNode->AccelVector.y = 0;
	}
	else
	{
		OGLMatrix3x3_SetRotateAboutPoint(&m, &origin, gPlayerToCameraAngle);			// make a 2D rotation matrix camera-rel
		theNode->AccelVector.x = gPlayerInfo.analogControlX;
		theNode->AccelVector.y = gPlayerInfo.analogControlZ;
		OGLVector2D_Transform(&theNode->AccelVector, &m, &theNode->AccelVector);		// rotate the acceleration vector


					/* APPLY ACCELERATION TO DELTAS */

		if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		{
			gDelta.x += theNode->AccelVector.x * (CONTROL_SENSITIVITY * fps);
			gDelta.z += theNode->AccelVector.y * (CONTROL_SENSITIVITY * fps);
		}
		else
		{
			gDelta.x += theNode->AccelVector.x * (CONTROL_SENSITIVITY_AIR * fps);
			gDelta.z += theNode->AccelVector.y * (CONTROL_SENSITIVITY_AIR * fps);
		}

	}

				/* SEE IF ALSO APPLY KEY THRUST */

	if (gControlNeeds[kNeed_AutoWalk].value)										// see if forward thrust
	{
		float	rot = theNode->Rot.y;
		gDelta.x += sin(rot) * (fps * -KEY_THRUST);
		gDelta.z += cos(rot) * (fps * -KEY_THRUST);
	}



				/*****************/
				/* CALC 2D SPEED */
				/*****************/

	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);					// calc 2D speed value
	if ((theNode->Speed2D >= 0.0f) && (theNode->Speed2D < 10000000.0f))		// check for weird NaN bug
	{
	}
	else
	{
		theNode->Speed2D = 0;
		gDelta.x = gDelta.z = 0;
	}

	if (theNode->Speed2D > gCurrentMaxSpeed)						// see if limit top speed
	{
		float	tweak;

		tweak = theNode->Speed2D / gCurrentMaxSpeed;

		gDelta.x /= tweak;
		gDelta.z /= tweak;

		theNode->Speed2D = gCurrentMaxSpeed;
	}


		/*****************************************/
		/* PART 1: MOVE AND COLLIDE IN MULTIPASS */
		/*****************************************/

		/* SUB-DIVIDE DELTA INTO MANAGABLE LENGTHS */

	oldFPS = gFramesPerSecond;											// remember what fps really is
	oldFPSFrac = gFramesPerSecondFrac;

	numPasses = (theNode->Speed2D*oldFPSFrac) * (1.0f / DELTA_SUBDIV);	// calc how many subdivisions to create
	numPasses++;


	gFramesPerSecondFrac *= 1.0f / (float)numPasses;					// adjust frame rate during motion and collision
	gFramesPerSecond *= 1.0f / (float)numPasses;

	fps = gFramesPerSecondFrac;

	for (pass = 0; pass < numPasses; pass++)
	{
		float	dx,dy,dz;

		oldCoord = gCoord;								// remember starting coord


				/* GET DELTA */

		dx = gDelta.x;
		dy = gDelta.y;
		dz = gDelta.z;

		if (theNode->MPlatform)						// see if factor in moving platform
		{
			ObjNode *plat = theNode->MPlatform;
			dx += plat->Delta.x;
			dy += plat->Delta.y;
			dz += plat->Delta.z;
		}

				/* MOVE IT */

		gCoord.x += dx*fps;
		gCoord.y += dy*fps;
		gCoord.z += dz*fps;


				/******************************/
				/* DO OBJECT COLLISION DETECT */
				/******************************/

		if (DoSkipCollisionDetect(theNode, useBBoxForTerrain))
			killed = true;



	}

	gFramesPerSecond = oldFPS;										// restore real FPS values
	gFramesPerSecondFrac = oldFPSFrac;


				/*************************/
				/* CHECK FENCE COLLISION */
				/*************************/

	DoFenceCollision(theNode);


				/* CHECK FLOOR */

	terrainY = GetTerrainY(gCoord.x, gCoord.z);
	gPlayerInfo.distToFloor = gCoord.y + theNode->BBox.min.y - terrainY;				// calc dist to floor

	return(killed);
}


/******************* DO PLAYER MOVEMENT AND COLLISION : RAMMING *********************/
//
// Special move function for ramming mode only
//

static Boolean DoPlayerMovementAndCollision_Ramming(ObjNode *theNode, Boolean useBBoxForTerrain)
{
float				fps = gFramesPerSecondFrac,oldFPS,oldFPSFrac,terrainY;
OGLPoint3D			oldCoord;
int					numPasses,pass;
Boolean				killed = false;
float				rot;

			/* ONLY CONTROL IS TURNING */

	rot = theNode->Rot.y -= gPlayerInfo.analogControlX * fps * 6.0f;


			/* ACCEL IN DIRECTION OF AIM */

	theNode->Speed2D += 1400.0f * fps;
	if (theNode->Speed2D > MAX_RAMMING_SPEED)
		theNode->Speed2D = MAX_RAMMING_SPEED;

	gCurrentMaxSpeed = theNode->Speed2D;

	gDelta.x = -sin(rot) * theNode->Speed2D;
	gDelta.z = -cos(rot) * theNode->Speed2D;


		/*********************************/
		/* MOVE AND COLLIDE IN MULTIPASS */
		/*********************************/

		/* SUB-DIVIDE DELTA INTO MANAGABLE LENGTHS */

	oldFPS = gFramesPerSecond;											// remember what fps really is
	oldFPSFrac = gFramesPerSecondFrac;

	numPasses = (theNode->Speed2D*oldFPSFrac) * (1.0f / DELTA_SUBDIV);	// calc how many subdivisions to create
	numPasses++;

	gFramesPerSecondFrac *= 1.0f / (float)numPasses;					// adjust frame rate during motion and collision
	gFramesPerSecond *= 1.0f / (float)numPasses;

	fps = gFramesPerSecondFrac;

	for (pass = 0; pass < numPasses; pass++)
	{
		float	dx,dy,dz;

		oldCoord = gCoord;								// remember starting coord


				/* GET DELTA */

		dx = gDelta.x;
		dy = gDelta.y;
		dz = gDelta.z;

				/* MOVE IT */

		gCoord.x += dx*fps;
		gCoord.y += dy*fps;
		gCoord.z += dz*fps;


				/******************************/
				/* DO OBJECT COLLISION DETECT */
				/******************************/

		if (DoSkipCollisionDetect(theNode, useBBoxForTerrain))
			killed = true;
	}

	gFramesPerSecond = oldFPS;										// restore real FPS values
	gFramesPerSecondFrac = oldFPSFrac;


				/*************************/
				/* CHECK FENCE COLLISION */
				/*************************/

	DoFenceCollision(theNode);


				/* CHECK FLOOR */

	terrainY = GetTerrainY(gCoord.x, gCoord.z);
	gPlayerInfo.distToFloor = gCoord.y + theNode->BBox.min.y - terrainY;				// calc dist to floor

	return(killed);
}


/************************ DO FRICTION & GRAVITY ****************************/
//
// Applies friction to the gDeltas
//

static void DoPlayerFrictionAndGravity_Terrain(ObjNode *theNode, float friction)
{
OGLVector2D	v;
float	x,z,fps;

	fps = gFramesPerSecondFrac;

			/**************/
			/* DO GRAVITY */
			/**************/

	gDelta.y -= gGravity*fps;					// add gravity

	if (gDelta.y < 0.0f)							// if falling, keep dy at least -1.0 to avoid collision jitter on platforms
		if (gDelta.y > (-20.0f * fps))
			gDelta.y = (-20.0f * fps);


			/***************/
			/* DO FRICTION */
			/***************/
			//
			// Dont do friction if player is pressing controls
			//

	if (gPlayerInfo.analogControlX || gPlayerInfo.analogControlZ)	// if there is any player control then no friction
		return;


	friction *= fps;							// adjust friction

	v.x = gDelta.x;
	v.y = gDelta.z;

	OGLVector2D_Normalize(&v, &v);				// get normalized motion vector
	x = -v.x * friction;						// make counter-motion vector
	z = -v.y * friction;

	if (gDelta.x < 0.0f)						// decelerate against vector
	{
		gDelta.x += x;
		if (gDelta.x > 0.0f)					// see if sign changed
			gDelta.x = 0;
	}
	else
	if (gDelta.x > 0.0f)
	{
		gDelta.x += x;
		if (gDelta.x < 0.0f)
			gDelta.x = 0;
	}

	if (gDelta.z < 0.0f)
	{
		gDelta.z += z;
		if (gDelta.z > 0.0f)
			gDelta.z = 0;
	}
	else
	if (gDelta.z > 0.0f)
	{
		gDelta.z += z;
		if (gDelta.z < 0.0f)
			gDelta.z = 0;
	}

	if ((gDelta.x == 0.0f) && (gDelta.z == 0.0f))
	{
		theNode->Speed2D = 0;
	}


}



/******************** DO SKIP COLLISION DETECT **************************/
//
// Standard collision handler for player
//
// OUTPUT: true = disabled/killed
//

static Boolean DoSkipCollisionDetect(ObjNode *theNode, Boolean useBBoxForTerrain)
{
short		i;
ObjNode		*hitObj;
u_char		sides;
float		distToFloor, terrainY;
float		bottomOff;
Boolean		killed = false;

			/***************************************/
			/* AUTOMATICALLY HANDLE THE GOOD STUFF */
			/***************************************/
			//
			// this also sets the ONGROUND status bit if on a solid object.
			//

	if (useBBoxForTerrain)
		theNode->BottomOff = theNode->BBox.min.y;
	else
		theNode->BottomOff = gPlayerBottomOff;

	sides = HandleCollisions(theNode, PLAYER_COLLISION_CTYPE, -.3);

			/* SCAN FOR INTERESTING STUFF */

	for (i=0; i < gNumCollisions; i++)
	{
		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
			hitObj = gCollisionList[i].objectPtr;				// get ObjNode of this collision

			if (hitObj->CType == INVALID_NODE_FLAG)				// see if has since become invalid
				continue;



			/* CHECK FOR TOTALLY IMPENETRABLE */

			if (hitObj->CBits & CBITS_IMPENETRABLE2)
			{
				if (!(gCollisionList[i].sides & SIDE_BITS_BOTTOM))	// dont do this if we landed on top of it
				{
					gCoord.x = theNode->OldCoord.x;					// dont take any chances, just move back to original safe place
					gCoord.z = theNode->OldCoord.z;
				}
			}

			/* CHECK FOR HURT ME */

			if (hitObj->CType & CTYPE_HURTME)
			{
				PlayerGotHit(hitObj, 0, PLAYER_ANIM_GOTHIT_GENERIC);
			}
		}
	}

		/*************************************/
		/* CHECK & HANDLE TERRAIN  COLLISION */
		/*************************************/

	if (useBBoxForTerrain)
		bottomOff = theNode->BBox.min.y;							// use bbox for bottom
	else
		bottomOff = theNode->BottomOff;								// use collision box for bottom

	terrainY =  GetTerrainY(gCoord.x, gCoord.z);					// get terrain Y

	distToFloor = (gCoord.y + bottomOff) - terrainY;				// calc amount I'm above or under

	if (distToFloor <= 0.0f)										// see if on or under floor
	{
		gCoord.y = terrainY - bottomOff;
		gDelta.y = -200.0f;											// keep some downward momentum
		theNode->StatusBits |= STATUS_BIT_ONGROUND;

	}

			/**************************/
			/* SEE IF IN WATER VOLUME */
			/**************************/

	if (!killed && (gDelta.y <= 0.0f))					// only check water if moving down and not killed yet
	{
		int		patchNum;
		Boolean	wasInWater;

					/* REMEMBER IF ALREADY IN WATER */

		if (theNode->StatusBits & STATUS_BIT_UNDERWATER)
			wasInWater = true;
		else
			wasInWater = false;

					/* CHECK IF IN WATER NOW */

		if (DoWaterCollisionDetect(theNode, gCoord.x, gCoord.y+theNode->BottomOff, gCoord.z, &patchNum))
		{
			gPlayerInfo.waterPatch = patchNum;

			gCoord.y = gWaterBBox[patchNum].max.y;

			if (!wasInWater || (!IsPlayerDoingSwimAnim(theNode)))
				PlayerEntersWater(theNode, patchNum);

			gDelta.y = 0;
		}
	}

	return(killed);
}




#pragma mark -


/**************** CHECK PLAYER ACTION CONTROLS ***************/
//
// Checks for special action controls
//
// INPUT:	theNode = the node of the player
//

static void CheckPlayerActionControls(ObjNode *theNode)
{


			/***************/
			/* SEE IF JUMP */
			/***************/

	if (CheckPlayerJumpControl(theNode))										// see if user pressed the key
		return;


			/***************************/
			/* SEE IF DO PICKUP / DROP */
			/***************************/

	if (gControlNeeds[kNeed_PickupDrop].newButtonPress)
	{
				/* TRY DROPPING */

		ObjNode	*heldObj = gPlayerInfo.heldObject;									// get the held object to drop
		if (heldObj)																// if holding something then drop it
		{
			PlayerDropObject(theNode, heldObj);
		}
				/* TRY PICKUP */
		else																		// nothing being held, so try to pickup
		{
			MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_PICKUP, 7);
			theNode->PickupNow = false;
		}
	}

			/******************/
			/* SEE IF DO KICK */
			/******************/
	else
	if (gControlNeeds[kNeed_Kick].newButtonPress)
	{
		if (!gPlayerInfo.heldObject)												// cannot kick if holding something
		{
			if (IsPlayerDoingStandAnim(theNode) || (IsPlayerDoingWalkAnim(theNode)))			// must be standing or walking to do a click
			{
				PlayEffect_Parms3D(EFFECT_SKIPKICK, &gCoord, NORMAL_CHANNEL_RATE, .5);
				MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_KICK, 12);
				theNode->KickHitNow = false;
			}
		}
	}

}


/********************* CHECK PLAYER JUMP CONTROL ************************/
//
// Returns true if player pressed jump key
//

static Boolean CheckPlayerJumpControl(ObjNode *theNode)
{
Boolean	isJumping, isFalling;

			/******************************/
			/* SEE IF PRESSED JUMP BUTTON */
			/******************************/

	if (gControlNeeds[kNeed_Jump].newButtonPress)										// see if user pressed the key
	{
		isJumping = IsPlayerDoingJumpAnim(theNode);
		isFalling = IsPlayerDoingFallAnim(theNode);

			/* CHECK GLIDE */

		if (isJumping || isFalling)
		{
			if (fabs(gDelta.y) < 400.0f)								// do it now if near apex of jump
				StartPlayerGliding(theNode);
			else
			if (isJumping)												// not @ apex, but if already jumping then tag to do GLIDING @ apex
				gDoGlidingAtApex = true;
			else
			if (gPlayerInfo.distToFloor > 100.0f)						// not @ apex, but since falling still allow GLIDING if high enough off ground
				StartPlayerGliding(theNode);
		}

			/* CHECK REGULAR JUMP */

		else
		if ((theNode->StatusBits & (STATUS_BIT_ONGROUND|STATUS_BIT_UNDERWATER))	||
			(gPlayerInfo.distToFloor < 15.0f))											// must be on something solid or liquid
		{
			if (IsPlayerDoingSwimAnim(theNode))							// don't jump high if was in water
				gDelta.y = JUMP_DELTA / 2;
			else
			if (IsXZOverWater(gCoord.x, gCoord.z))						// but if "over" water then assume on leaf or something so give a little boost
				gDelta.y = JUMP_DELTA * 1.1f;
			else
				gDelta.y = JUMP_DELTA;

			SetPlayerJumpAnim(theNode, true);

			gDoGlidingAtApex = false;


			if (theNode->MPlatform != nil)			// if jumping off of mplatform then also use platform's deltas
			{
				gDelta.x += theNode->MPlatform->Delta.x;
				gDelta.y += theNode->MPlatform->Delta.y;
				gDelta.z += theNode->MPlatform->Delta.z;
			}
		}
		return(true);
	}

	return(false);
}


/********************* PLAYER DROP OBJECT *******************************/

void PlayerDropObject(ObjNode *player, ObjNode *heldObj)
{
			/* RELEASE OBJ */

	ReleaseHeldObject(player, heldObj);


			/* DO DROP ANIM */

	switch(player->Skeleton->AnimNum)
	{
		case	PLAYER_ANIM_GLIDING:					// don't stop flying
		case	PLAYER_ANIM_GLIDECARRY:
				break;

		default:
				MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_DROPOBJECT, 10);
	}
}


/************************ RELEASE HELD OBJECT ***********************/

void ReleaseHeldObject(ObjNode *player, ObjNode *heldObj)
{
	heldObj->StatusBits &= ~(STATUS_BIT_NOMOVE|STATUS_BIT_NOCOLLISION);		// let move & collide again
	gPlayerInfo.heldObject = nil;											// not holding now
	if (heldObj->DropCallback)												// make sure there's a drop callback
	{
		heldObj->DropCallback(player, heldObj);								// call the callback
	}
}


#pragma mark -


/************************ HANDLE PLAYER LINE MARKER CROSSING ************************/

void	HandlePlayerLineMarkerCrossing(ObjNode *player)
{
int	markerNum;

			/* SEE IF CROSSED ANY LINE MARKERS */

	if (!SeeIfCrossedLineMarker(player, &markerNum))
		return;

	switch(gLevelNum)
	{
				/* HANDLE LINE MARKERS FOR GARDEN */

		case	LEVEL_NUM_GNOMEGARDEN:
				StartLevelCompletion(1.0);
				break;

				/* HANDLE LINE MARKERS FOR SIDEWALK */

		case	LEVEL_NUM_SIDEWALK:
				switch(markerNum)
				{
					case	0:				// activate snake zone
					case	1:
					case	3:
					case	4:
							gEnableSnakes = true;
							break;

					case	2:				// de-activate snake zone
					case	5:
					case	6:
							gEnableSnakes = false;
							break;


				}
				break;

				/* PLAYROOM */

		case	LEVEL_NUM_PLAYROOM:
				StartLevelCompletion(1.0);
				break;

				/* CLOSET */

		case	LEVEL_NUM_CLOSET:
				StartLevelCompletion(1.0);
				break;
	}
}







