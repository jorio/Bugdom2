/****************************/
/*   	PLAYER.C   			*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

extern	OGLPoint2D				gBestCheckpointCoord;
extern	OGLPoint3D				gCoord;
extern	OGLVector3D				gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float					gFramesPerSecondFrac,gBestCheckpointAim;
extern	short					gNumCollisions;
extern	CollisionRec			gCollisionList[];
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	u_long 					gAutoFadeStatusBits,gScore;
extern	int						gLevelNum,gScratch;
extern	Boolean				gFreezeCameraFromXZ,gFreezeCameraFromY,gPlayingFromSavedGame;
extern	Boolean				gEnableSnakes,gResetRideBall, gGameOver;
extern	float				gGlobalTransparency,gTargetMaxSpeed,gCurrentMaxSpeed;
extern	PrefsType			gGamePrefs;
extern	SparkleType			gSparkles[];
extern	SpriteType			*gSpriteGroupList[];
extern	float				gCameraDistFromMe;
extern	OGLBoundingBox		gWaterBBox[];
extern	MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	ObjNode				*gKillerDragonFly;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void ExplodePlayer(void);
static void DrawWingLayers(ObjNode *eventObj, const OGLSetupOutputType *setupInfo);
static void MoveShieldSphere(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	NUM_WING_BLUR_FRAMES	6


/*********************/
/*    VARIABLES      */
/*********************/


PlayerInfoType	gPlayerInfo;

float	gDeathTimer = 0;


Boolean	gPlayerIsDead = false;

Boolean			gResetGliding = true;				// set to true when player is on ground to reset gliding
Boolean			gDoGlidingAtApex = false;			// true if want to do Gliding when player reaches apex of jump


/******************** INIT PLAYER INFO ***************************/
//
// Called once at beginning of game
//

void InitPlayerInfo_Game(void)
{
int		i;

			/* INIT SOME THINGS IF NOT LOADING SAVED GAME */

	if (!gPlayingFromSavedGame)
	{
		gScore = 0;

		gPlayerInfo.lives			= 4;
		gPlayerInfo.health 			= 1.0;
		gPlayerInfo.numGoldClovers  = 0;
	}


	gDeathTimer = 0;

	gPlayerInfo.startX 			= 0;
	gPlayerInfo.startZ 			= 0;
	gPlayerInfo.coord.x 		= 0;
	gPlayerInfo.coord.y 		= 0;
	gPlayerInfo.coord.z 		= 0;

	gPlayerInfo.glidePower		= 1.0;

	for (i = 0; i < NUM_KEY_TYPES; i++)
		gPlayerInfo.hasKey[i]		= false;

	gPlayerInfo.blinkTimer 		= 2;


}


/******************* INIT PLAYER AT START OF LEVEL: TERRAIN ********************/
//
// Initializes player stuff at the beginning of each terrain exploration level.
//

void InitPlayerAtStartOfLevel_Terrain(void)
{
int	i;

	gPlayerInfo.invincibilityTimer = 0;

	gPlayerInfo.blurSprite = nil;
	gPlayerInfo.wingBlurFrame = 0;

	gPlayerInfo.burnTimer = 0;
	gPlayerInfo.heldObject = nil;
	gPlayerInfo.pushObj = nil;

	gPlayerInfo.ridingBall 	= nil;
	gPlayerInfo.slotCar 	= nil;
	gPlayerInfo.hasMap 		= false;

	gPlayerInfo.snake 	= -1;

	gPlayerInfo.rammingTimer = 0;

	gPlayerInfo.shieldObj[0] = nil;
	gPlayerInfo.shieldObj[1] = nil;
	gPlayerInfo.shieldChannel = -1;

	gPlayerInfo.numGreenClovers 	= 0;
	gPlayerInfo.numBlueClovers 		= 0;
	gPlayerInfo.numMiceRescued		= 0;

	gPlayerInfo.numBuddyBugs = 0;
	for (i = 0; i < MAX_BUDDY_BUGS; i++)
		gPlayerInfo.buddyBugs[i] = nil;


	gPlayerIsDead = false;

		/* FIRST PRIME THE TERRAIN TO CAUSE ALL OBJECTS TO BE GENERATED BEFORE WE PUT THE PLAYER DOWN */

	InitCurrentScrollSettings();
	DoPlayerTerrainUpdate(gPlayerInfo.coord.x, gPlayerInfo.coord.z);


			/**************************/
			/* THEN CREATE THE PLAYER */
			/**************************/

	gFreezeCameraFromY 		= false;					// assume no camera freeze
	gFreezeCameraFromXZ		= false;

	CreatePlayerModel_Terrain(&gPlayerInfo.coord, gPlayerInfo.startRotY);

	gBestCheckpointCoord.x = gPlayerInfo.coord.x;					// set first checkpoint @ starting location
	gBestCheckpointCoord.y = gPlayerInfo.coord.z;

	gBestCheckpointAim = gPlayerInfo.objNode->Rot.y;
}


/******************* INIT PLAYER AT START OF LEVEL: TUNNEL ********************/
//
// Initializes player stuff at the beginning of each Tunnel slide level.
//

void InitPlayerAtStartOfLevel_Tunnel(void)
{
int	i;

	gPlayerInfo.numGreenClovers = 0;
	gPlayerInfo.numBlueClovers = 0;
	gPlayerInfo.numMiceRescued		= 0;

	gPlayerInfo.invincibilityTimer = 0;

	gPlayerInfo.blurSprite = nil;
	gPlayerInfo.wingBlurFrame = 0;

	gPlayerInfo.burnTimer = 0;

	gPlayerInfo.tunnelAngle = 0;
	gPlayerInfo.tunnelDeltaRot = 0;
	gPlayerInfo.tunnelSpeed = 0;
	gPlayerInfo.tunnelBanking = 0;

	gPlayerInfo.heldObject = nil;

	gPlayerInfo.numBuddyBugs = 0;
	for (i = 0; i < MAX_BUDDY_BUGS; i++)
		gPlayerInfo.buddyBugs[i] = nil;

	gPlayerIsDead = false;

	gPlayerInfo.tunnelAngle = 0;


			/* THEN CREATE THE PLAYER */

	gFreezeCameraFromY 		= false;					// assume no camera freeze
	gFreezeCameraFromXZ		= false;

	CreatePlayerModel_Tunnel(200);

	gBestCheckpointCoord.x = gPlayerInfo.coord.x;		// set first checkpoint @ starting location
	gBestCheckpointCoord.y = gPlayerInfo.coord.z;
}


#pragma mark -



/********************* PLAYER GOT HIT ****************************/
//
// Normally damage comes from byWhat, but if byWhat = nil, then use altDamage
//

void PlayerGotHit(ObjNode *byWhat, float altDamage, int hitAnim)
{
ObjNode	*player = gPlayerInfo.objNode;
int	deathType = PLAYER_DEATH_TYPE_FALLOVER;

	if (gPlayerInfo.invincibilityTimer > 0.0f)							// cant get hit if invincible
		return;
	if (gPlayerInfo.shieldTimer > 0.0f)
		return;

	if (gPlayerIsDead)													// cant hurt if already dead
		return;

	if (player->Skeleton->AnimNum == PLAYER_ANIM_DRIVESLOTCAR)			// cant get hurt if driving
		return;

	if (gLevelNum == LEVEL_NUM_BALSA)									// special for balsa level
	{
		HurtPlayerOnBalsaPlane();
		deathType = PLAYER_DEATH_TYPE_BALSA;
		goto dodamage;
	}


			/* ASSUME SET GET-HIT ANIM */

	MorphToSkeletonAnim(player->Skeleton, hitAnim, 5);
	player->Timer = 1.5f;

	gPlayerInfo.invincibilityTimer = 4.0;


			/* SEE IF KNOCK OFF RIDING BALL (LEVEL 2) */

	if (gPlayerInfo.ridingBall)
		gPlayerInfo.ridingBall = nil;


			/* SEE IF DROP HELD OBJECT */

	if (gPlayerInfo.heldObject)
	{
		ObjNode *held = gPlayerInfo.heldObject;
		ReleaseHeldObject(player, held);
		held->Delta.x =
		held->Delta.y =
		held->Delta.z = 0;		// make sure it doesn't go zooming away
	}


			/* INFLICT DAMAGE */

dodamage:
	if (byWhat == nil)
		PlayerLoseHealth(altDamage, deathType);
	else
		PlayerLoseHealth(byWhat->Damage, deathType);


}



/***************** PLAYER LOSE HEALTH ************************/
//
// return true if player killed
//

Boolean PlayerLoseHealth(float damage, Byte deathType)
{
Boolean	killed = false;

	if (gPlayerInfo.health < 0.0f)				// see if already dead
		return(true);

	gPlayerInfo.health -= damage;

		/* SEE IF DEAD */

	if (gPlayerInfo.health <= 0.0f)
	{
		gPlayerInfo.health = 0;

		KillPlayer(deathType);
		killed = true;
	}


	return(killed);
}


/****************** KILL PLAYER *************************/

void KillPlayer(Byte deathType)
{
ObjNode	*player = gPlayerInfo.objNode;


		/* VERIFY ANIM IF ALREADY DEAD */
		//
		// This should assure us that we don't get kicked out of our death anim accidentally
		//

	if (gPlayerIsDead)						// see if already dead
	{
		switch(deathType)
		{
			case	PLAYER_DEATH_TYPE_EATENBYSNAKE:
					if (player->Skeleton->AnimNum != PLAYER_ANIM_EATENBYSNAKE)
						SetSkeletonAnim(player->Skeleton, PLAYER_ANIM_EATENBYSNAKE);
					break;


		}
		return;
	}


			/* KILL US NOW */

	EndGliding(player);						// make sure the wings are gone

	gPlayerIsDead = true;
	gPlayerInfo.health = 0;					// make sure this is set correctly

	switch(deathType)
	{
		case	PLAYER_DEATH_TYPE_EATENBYSNAKE:
				SetSkeletonAnim(player->Skeleton, PLAYER_ANIM_EATENBYSNAKE);
				gDeathTimer = gPlayerInfo.invincibilityTimer = 3.0f;
				break;

		case	PLAYER_DEATH_TYPE_EXPLODE:
				ExplodePlayer();
				gDeathTimer = gPlayerInfo.invincibilityTimer = 3.0f;
				break;

		case	PLAYER_DEATH_TYPE_FALLOVER:
				MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_DEATH, 5);
				gDeathTimer = gPlayerInfo.invincibilityTimer = 3.0f;
				break;

		case	PLAYER_DEATH_TYPE_DROWN:
				MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_DROWN, 5);
				gDeathTimer = gPlayerInfo.invincibilityTimer = 3.0f;
				break;

		case	PLAYER_DEATH_TYPE_BALSA:
				KillPlayerOnBalsaPlane(player);
				gDeathTimer = gPlayerInfo.invincibilityTimer = 4.0f;
				break;

		case	PLAYER_DEATH_TYPE_TUNNEL:
				KillPlayerInTunnel(player);
				gDeathTimer = gPlayerInfo.invincibilityTimer = 4.0f;
				break;

		case	PLAYER_DEATH_TYPE_KILLERDRAGONFLY:
				MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_CARRIED, 7);
				gDeathTimer = gPlayerInfo.invincibilityTimer = 3.0f;
				break;
	}




}


/*********************** EXPLODE PLAYER ***************************/

static void ExplodePlayer(void)
{
ObjNode	*player = gPlayerInfo.objNode;


		/* HIDE & DISABLE THE REAL PLAYER */

	HidePlayer(player);


			/***************/
			/* MAKE SPARKS */
			/***************/

	MakeSparkExplosion(player->Coord.x, player->Coord.y, player->Coord.z, 400, 1.0, PARTICLE_SObjType_BlueSpark, 0, 1.0);

	ExplodeGeometry(player, 150.0, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 2, .4);

}


/********************* HIDE PLAYER ************************/

void HidePlayer(ObjNode *player)
{

	player->StatusBits |= STATUS_BIT_HIDDEN | STATUS_BIT_NOMOVE;
	player->CType = 0;
}


/****************** RESET PLAYER @ BEST CHECKPOINT *********************/

void ResetPlayerAtBestCheckpoint(void)
{
ObjNode	*player = gPlayerInfo.objNode;


			/***************************************************/
			/* FIRST TAKE AWAY A LIFE AND SEE IF IT'S ALL OVER */
			/***************************************************/

	gPlayerInfo.lives--;				// dec # lives
	if (gPlayerInfo.lives <= 0)
	{
		gGameOver = true;
		return;
	}


				/****************************/
				/* RESET COORD @ CHECKPOINT */
				/****************************/

	gPlayerInfo.coord.x = player->Coord.x = gBestCheckpointCoord.x;
	gPlayerInfo.coord.z = player->Coord.z = gBestCheckpointCoord.y;
	DoPlayerTerrainUpdate(gPlayerInfo.coord.x, gPlayerInfo.coord.z);		// do this to prime any objecs/platforms there before we calc our new y Coord

	gPlayerInfo.coord.y = gPlayerInfo.objNode->Coord.y = FindHighestCollisionAtXZ(gPlayerInfo.coord.x, gPlayerInfo.coord.z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN) - player->BottomOff + 10.0f;

	player->OldCoord = player->Coord;
	player->Delta.x = player->Delta.y = player->Delta.z = 0;


				/* RESET COLLISION & STATUS INFO */

	player->CType = CTYPE_PLAYER;										// make sure collision is set
	player->StatusBits &= ~(STATUS_BIT_HIDDEN | STATUS_BIT_NOMOVE);		// make sure not hidden and movable

	gPlayerInfo.health = player->Health = 1.0;
	gPlayerInfo.burnTimer = 0;

	gPlayerIsDead		= false;

	gResetRideBall		= true;											// reset any ride balls back to start pos

	player->Rot.y = gBestCheckpointAim;										// set the aim

	gEnableSnakes = false;
	gPlayerInfo.snake = -1;


			/* SET PLAYER ANIM & CAMERA */

	if (gLevelNum == LEVEL_NUM_BALSA)								// special for balsa level
	{
		ResetPlayerOnBalsaPlane(player);
	}
	else
	{
		SetPlayerStandAnim(player, 100);
	}


			/* RESET RISING WATER ON GARBAGE LEVEL */

	if (gLevelNum == LEVEL_NUM_GARBAGE)
	{
		ResetRisingWater();

		if (gKillerDragonFly)									// see if also nuke killer dragonfly
		{
			DeleteObject(gKillerDragonFly);
			gKillerDragonFly = nil;
		}
	}

	InitCamera_Terrain();


	MakeFadeEvent(true, 1);

}





#pragma mark -


/*************** PLAYER ENTERS WATER **************************/
//
// called from player's collision handler if just now entered a water patch
//

void PlayerEntersWater(ObjNode *theNode, int patchNum)
{
float	topY;

	if (patchNum == -1)								// if no patch, then use terrain y
		topY = GetTerrainY(gCoord.x, gCoord.z);
	else
		topY = gWaterBBox[patchNum].max.y;

			/* SPLASH */

	if (gDelta.y < -100.0f)
		MakeSplash(gCoord.x, topY, gCoord.z, 1.0);


	MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_SWIM, 5.0f);

	theNode->Timer = 0;								// reset ripple timer

	gDelta.x *= .4f;								// slow on impact
	gDelta.z *= .4f;
}


/********************** PLAYER DO KICK *****************************/

short PlayerDoKick(ObjNode *player)
{
short		numHits,i;
OGLPoint3D	footPt;


			/* CALC COORD OF THE KICKING FOOT */

	FindCoordOfJoint(player, PLAYER_JOINT_LEFT_FOOT, &footPt);

	numHits = DoSimpleBoxCollision(footPt.y + 25.0f, footPt.y - 50.0f, footPt.x - 20.0f, footPt.x + 20.0f,
								footPt.z + 20.0f, footPt.z - 20.0f, CTYPE_KICKABLE);

			/* SEE WHAT WE HIT */

	for (i = 0; i < numHits; i++)
	{
		ObjNode	*kickedObj = gCollisionList[i].objectPtr;

		if (kickedObj->GotKickedCallback)								// call the got kicked callback
			kickedObj->GotKickedCallback(player, kickedObj);

	}

	return(numHits);
}




#pragma mark -

/******************* START PLAYER GLIDING *********************/

void StartPlayerGliding(ObjNode *player)
{

	if (!gResetGliding)								// cannot do this until gliding has been reset
		return;

	if (gPlayerInfo.glidePower <= 0.0f)				// can't do it if out of glide power
	{
		gDoGlidingAtApex = false;
		return;
	}

	gResetGliding = false;

			/* SET ANIM */

	SetPlayerGlideAnim(player);


			/* SET SOME VARIABLES */

	gDelta.y = 0;


			/***************************/
			/* CREATE WING BLUR SPRITE */
			/***************************/

	gNewObjectDefinition.group		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_WingBlur1;
	gNewObjectDefinition.coord		= gCoord;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|STATUS_BIT_DOUBLESIDED;
	gNewObjectDefinition.slot 		= WATER_SLOT+21;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= player->Scale.x * .9f;
	gPlayerInfo.blurSprite = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/********************/
			/* INIT WING LAYERS */
			/********************/

			/* CREATE EVENT OBJECT TO DRAW THEM */

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|
									STATUS_BIT_DOUBLESIDED|STATUS_BIT_ROTZXY|STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot 		= WATER_SLOT+20;
	gNewObjectDefinition.moveCall 	= nil;
	gPlayerInfo.wingLayerDrawObject = MakeNewObject(&gNewObjectDefinition);

	gPlayerInfo.wingLayerDrawObject->CustomDrawFunction = DrawWingLayers;


			/* START EFFECT */

	player->EffectChannel = PlayEffect_Parms(EFFECT_SKIPGLIDE, FULL_CHANNEL_VOLUME/2, FULL_CHANNEL_VOLUME/2, NORMAL_CHANNEL_RATE);

}


/**************** END GLIDING *****************/

void EndGliding(ObjNode *theNode)
{

	if (gPlayerInfo.blurSprite)
	{
		DeleteObject(gPlayerInfo.blurSprite);
		gPlayerInfo.blurSprite = nil;
	}

	if (gPlayerInfo.wingLayerDrawObject)
	{
		DeleteObject(gPlayerInfo.wingLayerDrawObject);
		gPlayerInfo.wingLayerDrawObject = nil;
	}

	if (IsPlayerDoingGlideAnim(theNode))						// if coming out of gliding then fall
		SetPlayerFallAnim(theNode);

	StopAChannel(&theNode->EffectChannel);
}


/************************* DRAW WING LAYERS **********************************/

static void DrawWingLayers(ObjNode *eventObj, const OGLSetupOutputType *setupInfo)
{
#pragma unused (eventObj)
int		i;

	for (i = NUM_WING_BLUR_LAYERS-1; i >= 0; i--)
	{

			/* DRAW RIGHT WING */

		glPushMatrix();
		glMultMatrixf((GLfloat *)&gPlayerInfo.wingLayerMatrix[i][0]);
		gGlobalTransparency = gPlayerInfo.wingLayerAlpha[i][0];
		MO_DrawObject(gBG3DGroupList[MODEL_GROUP_GLOBAL][GLOBAL_ObjType_RightWing], setupInfo);
		glPopMatrix();

			/* DRAW RIGHT WING */

		glPushMatrix();
		glMultMatrixf((GLfloat *)&gPlayerInfo.wingLayerMatrix[i][1]);
		gGlobalTransparency = gPlayerInfo.wingLayerAlpha[i][1];
		MO_DrawObject(gBG3DGroupList[MODEL_GROUP_GLOBAL][GLOBAL_ObjType_LeftWing], setupInfo);
		glPopMatrix();


	}

	gGlobalTransparency = 1.0f;
}


/******************** UPDATE GLIDING WINGS ************************/
//
// This function is called from UpdatePlayer() every time regardless of
// whether we are gliding or not, so the first thing we do here is verify
// some info about our state.
//

void UpdateGlidingWings(ObjNode *player)
{
ObjNode	*blurSprite;
int			i;
const OGLPoint3D off = {-2,12,5};
float	playerRotY,dot,r,frameCount,alpha;
OGLVector2D	playerAim,cameraAim;
float	scale = player->Scale.x;							// calc scale of wing
float	fps = gFramesPerSecondFrac;

			/* SEE IF WE'VE GOT WINGS TO UPDATE */

	blurSprite = gPlayerInfo.blurSprite;					// get blur object
	if (blurSprite == nil)									// no blur object?  Then no wings to update.
		return;

			/* LOSE SOME JUICE */

	gPlayerInfo.glidePower -= fps * .04f;
	if (gPlayerInfo.glidePower <= 0.0f)
	{
		gPlayerInfo.glidePower = 0.0f;
		if (IsPlayerDoingGlideAnim(player))					// knock out of glide anim since out of fuel
		{
			EndGliding(player);
			return;
		}
	}


			/* VERIFY THAT WE SHOULD BE DOING THIS */
			//
			// If something knocked the player out of the flying animation
			// then it's possible that the wings are still active, so
			// let's check for that and delete the wings if we find it.
			//


	if (!IsPlayerDoingGlideAnim(player))					// only do this if flying
	{
		EndGliding(player);									// something knocked us out of gliding, so get rid of the wings
		return;
	}


	/*******************************************************/
	/* FIRST DETERMINE CROSS-FADE BETWEEN WINGS AND SPRITE */
	/*******************************************************/

	playerRotY = player->Rot.y;

	playerAim.x = -sin(playerRotY);
	playerAim.y = -cos(playerRotY);

	cameraAim.x = gGameViewInfoPtr->cameraPlacement.pointOfInterest.x - gGameViewInfoPtr->cameraPlacement.cameraLocation.x;
	cameraAim.y = gGameViewInfoPtr->cameraPlacement.pointOfInterest.z - gGameViewInfoPtr->cameraPlacement.cameraLocation.z;
	FastNormalizeVector2D(cameraAim.x, cameraAim.y, &cameraAim, true);

	dot = fabs(OGLVector2D_Dot(&playerAim, &cameraAim));


		/*********************************/
		/* UPDATE THE BLUR SPRITE EFFECT */
		/*********************************/


			/* UPDATE BLUR FRAME */

	gPlayerInfo.wingBlurFrame += fps * 60.0f;
	if (gPlayerInfo.wingBlurFrame >= NUM_WING_BLUR_FRAMES)
		gPlayerInfo.wingBlurFrame = 0;
	blurSprite->Type = GLOBAL_ObjType_WingBlur1 + (int)gPlayerInfo.wingBlurFrame;
	ResetDisplayGroupObject(blurSprite);

	blurSprite->ColorFilter.a = dot;


			/* UPDATE BLUR MATRICES */

	FindCoordOnJoint(player, 0, &off, &blurSprite->Coord);
	blurSprite->Rot.y = playerRotY;
	UpdateObjectTransforms(blurSprite);


		/**************************/
		/* UPDATE THE WING LAYERS */
		/**************************/

	dot = 1.0f - dot;

			/* CALC WING ROTATION ANGLE */

	frameCount = gPlayerInfo.wingBlurFrame;
	alpha = 1.0f;

	for (i = 0; i < NUM_WING_BLUR_LAYERS; i++)
	{
		OGLPoint3D		coord;
		OGLMatrix4x4	m,my,mz,m2;
		const OGLPoint3D rightOff = {3,7,6};
		const OGLPoint3D leftOff = {-3,7,6};

		if (frameCount < (NUM_WING_BLUR_FRAMES/2))								// see if on down-swing
			r = -1.0f + (frameCount / (NUM_WING_BLUR_FRAMES/2)) * 2.0f;
		else																	// else, up-swing
			r = -1.0f + (1.0f - (frameCount - (NUM_WING_BLUR_FRAMES/2)) / (NUM_WING_BLUR_FRAMES/2)) * 2.0f;


				/* CALC RIGHT WING MATRIX */

		FindCoordOnJoint(player, 0, &rightOff, &coord);								// calc coord of wing
		OGLMatrix4x4_SetScale(&m, scale, scale, scale);								// set scale
		OGLMatrix4x4_SetRotate_Y(&my, playerRotY);									// set rotate
		OGLMatrix4x4_SetRotate_Z(&mz, -r);
		OGLMatrix4x4_Multiply(&mz,&my, &m2);
		m2.value[M03] = coord.x;													// set translate
		m2.value[M13] = coord.y;
		m2.value[M23] = coord.z;
		OGLMatrix4x4_Multiply(&m,&m2, &gPlayerInfo.wingLayerMatrix[i][0]);
		gPlayerInfo.wingLayerAlpha[i][0] = alpha * dot;								// set alpha



				/* CALC LEFT WING MATRIX */

		FindCoordOnJoint(player, 0, &leftOff, &coord);								// calc coord
		OGLMatrix4x4_SetRotate_Z(&mz, r);											// set rotate z
		OGLMatrix4x4_Multiply(&mz,&my, &m2);
		m2.value[M03] = coord.x;													// set translate
		m2.value[M13] = coord.y;
		m2.value[M23] = coord.z;
		OGLMatrix4x4_Multiply(&m,&m2, &gPlayerInfo.wingLayerMatrix[i][1]);
		gPlayerInfo.wingLayerAlpha[i][1] = alpha * dot;								// set alpha


				/* NEXT */

		if (alpha == 1.0f)
			alpha = .6f;
		else
			alpha *= .8f;
		frameCount -= .3f;
		if (frameCount < 0.0f)
			frameCount += NUM_WING_BLUR_FRAMES;
	}
}



#pragma mark -

/*********************** SET PLAYER WALK ANIM *******************************/

void SetPlayerWalkAnim(ObjNode *theNode)
{
	if (gPlayerInfo.heldObject)
		MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_WALKCARRY, 9);
	else
		MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_WALK, 9);
}


/********************* IS PLAYER DOING WALK ANIM *********************/

Boolean IsPlayerDoingWalkAnim(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_WALK)
		return(true);
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_WALKCARRY)
		return(true);

	return(false);
}

/*********************** SET PLAYER STAND ANIM *******************************/

void SetPlayerStandAnim(ObjNode *theNode, float speed)
{
	if (gPlayerInfo.heldObject)
		MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_STANDCARRY, speed);
	else
		MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_STAND, speed);

	theNode->Timer = 2.0f + RandomFloat() * 5.0f;			// set random personality delay timer (see below)
}


/********************* IS PLAYER DOING STAND ANIM *********************/

Boolean IsPlayerDoingStandAnim(ObjNode *theNode)
{
	switch(theNode->Skeleton->AnimNum)
	{
		case	PLAYER_ANIM_STAND:
		case	PLAYER_ANIM_STANDCARRY:
		case	PLAYER_ANIM_PERSONALITY2:
		case	PLAYER_ANIM_PERSONALITY1:
		case	PLAYER_ANIM_PERSONALITY2_CARRY:
		case	PLAYER_ANIM_PERSONALITY1_CARRY:
		case	PLAYER_ANIM_PERSONALITY3_DANCE:
				return(true);

		default:
				return(false);
	}
}


/********************** UPDATE PERSONALITY *****************************/

void UpdatePersonality(ObjNode *player)
{
float	fps = gFramesPerSecondFrac;
float	t;

	t = (player->Timer -= fps);									// dec personality timer

	switch(player->Skeleton->AnimNum)
	{
				/**************************************/
				/* SEE IF TIME TO DO SOME PERSONALITY */
				/**************************************/

						/* FROM BASIC STAND */

		case	PLAYER_ANIM_STAND:
				if (t <= 0.0f)
				{
					switch(RandomRange(0,2))
					{
						case	0:
								MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_PERSONALITY2, 6);
								player->Timer = 6.0f + RandomFloat() * 6.0f;						// personality 2 is 6 seconds long and loops
								break;

						case	1:
								MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_PERSONALITY1, 6);	// personality 1 doesn't look, so no timer reset is needed
								break;

						case	2:
								MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_PERSONALITY3_DANCE, 6);
								player->Timer = 10.0f + RandomFloat() * 8.0f;						// personality 2 is 10 seconds long and loops
								break;
					}
				}
				break;

						/* FROM CARRY STAND */

		case	PLAYER_ANIM_STANDCARRY:
				if (t <= 0.0f)
				{
					if (MyRandomLong() & 0x1)												// randomly choose between personality types
						MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_PERSONALITY1_CARRY, 6);	// personality 1 doesn't look, so no timer reset is needed
					else
					{
						MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_PERSONALITY2_CARRY, 6);
						player->Timer = 6.0f + RandomFloat() * 6.0f;						// personality 2 is 6 seconds long and loops
					}
				}
				break;


				/**************************************/
				/* UPDATE THE PERSONALITIES IN ACTION */
				/**************************************/

					/* FROM NON-LOOPING PERSONALITY 1 */

		case	PLAYER_ANIM_PERSONALITY1:
		case	PLAYER_ANIM_PERSONALITY1_CARRY:
				if (player->Skeleton->AnimHasStopped)
					SetPlayerStandAnim(player, 6);
				break;

					/* FROM LOOPING PERSONALITY 2 */

		case	PLAYER_ANIM_PERSONALITY2:
		case	PLAYER_ANIM_PERSONALITY2_CARRY:
		case	PLAYER_ANIM_PERSONALITY3_DANCE:
				if (t <= 0.0f)
					SetPlayerStandAnim(player, 6);
				break;


	}
}



/********************* IS PLAYER DOING FALL ANIM *********************/

Boolean IsPlayerDoingFallAnim(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_FALL)
		return(true);
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_FALLCARRY)
		return(true);

	return(false);
}


/************************ SET PLAYER FALL ANIM *************************/

void SetPlayerFallAnim(ObjNode *player)
{
	if (gPlayerInfo.heldObject)
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_FALLCARRY, 4.0);
	else
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_FALL, 4.0);
}

/********************* IS PLAYER DOING JUMP ANIM *********************/

Boolean IsPlayerDoingJumpAnim(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_JUMPCARRY)
		return(true);
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_JUMP)
		return(true);

	return(false);
}


/************************ SET PLAYER JUMP ANIM *************************/

void SetPlayerJumpAnim(ObjNode *player, Boolean playEffect)
{
	if (gPlayerInfo.heldObject)
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_JUMPCARRY, 14.0);
	else
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_JUMP, 14.0);

	player->StatusBits &= ~(STATUS_BIT_UNDERWATER | STATUS_BIT_ONGROUND);	// turn off onground an water bits

	if (playEffect)
		PlayEffect3D(EFFECT_JUMP, &player->Coord);

}


/********************* IS PLAYER DOING GLIDE ANIM *********************/

Boolean IsPlayerDoingGlideAnim(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_GLIDING)
		return(true);
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_GLIDECARRY)
		return(true);

	return(false);
}


/************************ SET PLAYER LAND ANIM *************************/

void SetPlayerLandAnim(ObjNode *player)
{
	if (gPlayerInfo.heldObject)
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_LANDCARRY, 5.0);
	else
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_LANDING, 5.0);

	PlayEffect_Parms(EFFECT_SKIPLAND, FULL_CHANNEL_VOLUME/2, FULL_CHANNEL_VOLUME/2, NORMAL_CHANNEL_RATE);
}


/********************* IS PLAYER DOING LAND ANIM *********************/

Boolean IsPlayerDoingLandAnim(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_LANDING)
		return(true);
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_LANDCARRY)
		return(true);

	return(false);
}


/********************* IS PLAYER DOING SWIM ANIM *********************/

Boolean IsPlayerDoingSwimAnim(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_SWIM)
		return(true);

	return(false);
}

/************************ SET PLAYER GLIDE ANIM *************************/

void SetPlayerGlideAnim(ObjNode *player)
{
	if (gPlayerInfo.heldObject)
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_GLIDECARRY, 6.0f);
	else
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_GLIDING, 6.0f);
}


/********************* IS PLAYER DOING RAMMING ANIM *********************/

Boolean IsPlayerDoingRammingAnim(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_RAMMING)
		return(true);

	return(false);
}


/*********************** SET PLAYER RAMMING ANIM *******************************/

void SetPlayerRammingAnim(ObjNode *theNode)
{
			/* IF HOLDING ANYTHING, DROP IT */

	if (gPlayerInfo.heldObject)
		ReleaseHeldObject(theNode, gPlayerInfo.heldObject);

	MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_RAMMING, 15);

	gPlayerInfo.rammingTimer = 10.0f;
}



/********************* IS PLAYER DOING PUSH ANIM *********************/

Boolean IsPlayerDoingPushAnim(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_PUSH)
		return(true);

	return(false);
}


#pragma mark -


/****************** PLAYER START PUSHING OBJECT **********************/

void PlayerStartPushingObject(ObjNode *player, ObjNode *pushObj)
{
	if (!IsPlayerDoingPushAnim(player))							// if not already pushing...
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_PUSH, 5.0);

	gPlayerInfo.pushObj = pushObj;
	gPlayerInfo.pushTimer = .4f;
	gTargetMaxSpeed = gCurrentMaxSpeed = PLAYER_PUSH_MAX_SPEED;
}


/***************** PLAYER STOP PUSHING OBJECT **********************/

void PlayerStopPushingObject(ObjNode *player)
{
	if (IsPlayerDoingPushAnim(player))
		SetPlayerStandAnim(player, 5.0);

	gPlayerInfo.pushObj = nil;

	gTargetMaxSpeed = PLAYER_NORMAL_MAX_SPEED;
}



#pragma mark -


/********************** UPDATE PLAYER SHIELD ***********************/
//
// Called from player update function to see if need to maintain shield.
//

void UpdatePlayerShield(void)
{
int		i;


			/* SEE IF SHIELD SHOULD BE GONE */

	if (gPlayerInfo.shieldTimer <= 0.0f)
	{
		if (gPlayerInfo.shieldObj[0])
		{
			DeleteObject(gPlayerInfo.shieldObj[0]);
			gPlayerInfo.shieldObj[0] = nil;
		}
		if (gPlayerInfo.shieldObj[1])
		{
			DeleteObject(gPlayerInfo.shieldObj[1]);
			gPlayerInfo.shieldObj[1] = nil;
		}
		return;
	}

			/* DEC TIMER */

	gPlayerInfo.shieldTimer -= gFramesPerSecondFrac;
	if (gPlayerInfo.shieldTimer < 0.0f)
	{
		gPlayerInfo.shieldTimer = 0.0f;
		StopAChannel(&gPlayerInfo.shieldChannel);
		return;
	}

		/* UPDATE SOUND */

	if (gPlayerInfo.shieldChannel == -1)
		gPlayerInfo.shieldChannel = PlayEffect(EFFECT_SHIELD);


		/*****************************/
		/* MAKE SURE WE HAVE SPHERES */
		/*****************************/

	for (i = 0; i < 2; i++)
	{
		if (gPlayerInfo.shieldObj[i] == nil)
		{
			ObjNode *newObj;

			gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
			gNewObjectDefinition.type 		= GLOBAL_ObjType_Shield;
			gNewObjectDefinition.scale 		= .7f;
			gNewObjectDefinition.coord	 	= gPlayerInfo.coord;
			gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL | STATUS_BIT_GLOW |
											 STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES;
			gNewObjectDefinition.slot 		= SLOT_OF_DUMB + 15;
			gNewObjectDefinition.moveCall 	= MoveShieldSphere;
			gNewObjectDefinition.rot 		= 0;
			newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			gPlayerInfo.shieldObj[i] = newObj;

			newObj->ColorFilter.a = .6f;

			if (i == 0)
			{
				newObj->ColorFilter.r = 1.0f;
				newObj->ColorFilter.g = .3f;
				newObj->ColorFilter.b = 1.0f;

				newObj->Scale.x =
				newObj->Scale.y =
				newObj->Scale.z = newObj->Scale.x * 1.06f;

				newObj->Rot.z = -1.0;
				newObj->Rot.y = PI;
				newObj->DeltaRot.y = -3.5;
			}
			else
			{
				newObj->ColorFilter.r = 1;
				newObj->ColorFilter.g = .8;
				newObj->ColorFilter.b = .6;

				newObj->Rot.y = 0;
				newObj->Rot.z = 1.0;
				newObj->DeltaRot.y = 3.5;

			}

		}
	}
}


/********************* MOVE SHIELD SPHERE ****************************/

static void MoveShieldSphere(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*player = gPlayerInfo.objNode;

	if (gPlayerInfo.shieldTimer < 2.0f)
		theNode->ColorFilter.a = .6f * (gPlayerInfo.shieldTimer * .5f);
	else
		theNode->ColorFilter.a = .6f;

	theNode->Rot.y += fps * theNode->DeltaRot.y;
	theNode->Rot.x += fps * .1f;

	theNode->Coord.x = player->Coord.x + (player->BBox.max.x + player->BBox.min.x) * .5f;
	theNode->Coord.y = player->Coord.y + (player->BBox.max.y + player->BBox.min.y) * .5f;
	theNode->Coord.z = player->Coord.z + (player->BBox.max.z + player->BBox.min.z) * .5f;

	UpdateObjectTransforms(theNode);
}






















