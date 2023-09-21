/****************************/
/*   ENEMY: PONDFISH.C		*/
/* (c)1999 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	int						gNumEnemies, gMaxEnemies;
extern	signed char				gNumEnemyOfKind[NUM_ENEMY_KINDS];
extern	float					gFramesPerSecondFrac, gFramesPerSecond, gDeathTimer;
extern	OGLPoint3D				gCoord;
extern	OGLVector3D				gDelta;
extern	ObjNode					*gFirstNodePtr;
extern	uint32_t				gAutoFadeStatusBits;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MovePondFish(ObjNode *theNode);
static void MovePondFish_Swimming(ObjNode *theNode);
static void  MovePondFish_JumpAttack(ObjNode *theNode);
static void  MovePondFish_Caught(ObjNode *fish);
static void UpdatePondFish(ObjNode *theNode);
static void SeeIfFishEatsPlayer(ObjNode *fish);
static void PondFish_ContinueEatingPlayer(ObjNode *fish);
static Boolean VerifyTargetLure(ObjNode *fish);
static void FindTargetLure(ObjNode *fish);
static void CheckFishBiteLure(ObjNode *fish, ObjNode *lure);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_PONDFISH			9

#define	PONDFISH_CHASE_DIST		2000.0f
#define	PONDFISH_CHASESPEED		330.0f

#define PONDFISH_ATTACK_DIST	300.0f
#define PONDFISH_JUMPSPEED		1700.0f

#define PONDFISH_TURN_SPEED		2.0f

#define	PONDFISH_DAMAGE			0.4f

#define	PONDFISH_SCALE			4.0f

#define	FISH_SWIM_DEPTH			150.0f

enum
{
	PONDFISH_MODE_WAIT,
	PONDFISH_MODE_CHASEPLAYER,
	PONDFISH_MODE_GETLURE
};


enum
{
	PONDFISH_JOINT_HEAD = 2

};

enum
{
	PONDFISH_ANIM_SWIM,
	PONDFISH_ANIM_JUMPATTACK,
	PONDFISH_ANIM_MOUTHFULL,
	PONDFISH_ANIM_CAUGHT
};


/*********************/
/*    VARIABLES      */
/*********************/

int		gNumCaughtFish;

#define	TargetLure		Special[0]

#define RippleTimer		SpecialF[0]
#define	AttackTimer		SpecialF[1]
#define	RandomJumpTimer	SpecialF[2]
#define	EatenTimer		SpecialF[3]
#define	JumpNow			Flag[0]
#define	IsJumping		Flag[1]
#define	EatPlayer		Flag[2]
#define	TweakJumps		Flag[3]

static const OGLPoint3D gPondFishMouthOff = {0,-10,-20};

ObjNode	*gCurrentEatingFish = nil;


/************************ ADD PONDFISH ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_PondFish(TerrainItemEntryType *itemPtr, float x, float z)
{
ObjNode	*newObj;

	if (gNumEnemyOfKind[ENEMY_KIND_PONDFISH] >= MAX_PONDFISH)
		return(false);


				/* MAKE DEFAULT SKELETON ENEMY */

	gNewObjectDefinition.type 		= SKELETON_TYPE_FISH;
	gNewObjectDefinition.animNum 	= 0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= LURE_SLOT-1;
	gNewObjectDefinition.moveCall 	= MovePondFish;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= PONDFISH_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;


				/* SET BETTER INFO */


	newObj->Damage 		= PONDFISH_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_PONDFISH;
	newObj->Mode		= PONDFISH_MODE_WAIT;

	newObj->Coord.y -= newObj->BBox.min.y;						// offset so bottom touches ground
	UpdateObjectTransforms(newObj);


				/* SET DEFAULT COLLISION INFO */

	newObj->ForceLookAtDist	= 800.0f;

	newObj->CType = CTYPE_ENEMY|CTYPE_BLOCKCAMERA|CTYPE_AUTOTARGETWEAPON|CTYPE_KICKABLE|CTYPE_BUDDYATTRACT;
	newObj->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj, 50,-50,-130,130,130,-130);


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_PONDFISH]++;
	return(true);
}



/********************* MOVE PONDFISH **************************/

static void MovePondFish(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
		{
			MovePondFish_Swimming,
			MovePondFish_JumpAttack,
			MovePondFish_Swimming,						// mouthfull
			MovePondFish_Caught,
		};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
	}
	else
	{
		GetObjectInfo(theNode);

		theNode->AttackTimer -= gFramesPerSecondFrac;		// dec this timer

		myMoveTable[theNode->Skeleton->AnimNum](theNode);
	}
}


/********************** MOVE PONDFISH: SWIMMING ******************************/

static void  MovePondFish_Swimming(ObjNode *theNode)
{
float	dist,r,aim,swimY;
float	fps = gFramesPerSecondFrac;
float	waterY;

			/* SET WATER Y */

	GetWaterY(gCoord.x, gCoord.z, &swimY);
	swimY -= FISH_SWIM_DEPTH;

	if (gCoord.y > swimY)							// move down to target Y
		gCoord.y -= 200.0f * fps;
	else
		gCoord.y = swimY;



	switch(theNode->Mode)
	{
				/*****************************/
				/* JUST HOVERING AND WAITING */
				/*****************************/

		case	PONDFISH_MODE_WAIT:
				TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, PONDFISH_TURN_SPEED, false);

						/* SEE IF DO RANDOM JUMP */

				if ((theNode->RandomJumpTimer -= gFramesPerSecondFrac) < 0.0f)
				{
					theNode->RandomJumpTimer = 4.0f + RandomFloat()*5.0f;
					theNode->TweakJumps = false;
					goto attack;
				}


					/* SEE IF PLAYER IS IN THE WATER AND IS CLOSE */

				if (IsPlayerDoingSwimAnim(gPlayerInfo.objNode))
				{
					dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
					if (dist < PONDFISH_CHASE_DIST)
					{
						theNode->Mode = PONDFISH_MODE_CHASEPLAYER;
					}
				}

						/* SEE IF GO FOR A LURE */

				FindTargetLure(theNode);
				break;


				/***********/
				/* CHASING */
				/***********/

		case	PONDFISH_MODE_CHASEPLAYER:
				aim = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, PONDFISH_TURN_SPEED, false);


						/* MOVE FORWARD */

				r = theNode->Rot.y;
				gDelta.x = -sin(r) * PONDFISH_CHASESPEED;
				gDelta.z = -cos(r) * PONDFISH_CHASESPEED;
				gCoord.x += gDelta.x * fps;
				gCoord.y += gDelta.y * fps;
				gCoord.z += gDelta.z * fps;

				if (!GetWaterY(gCoord.x, gCoord.z, &waterY))					// if no longer in water, then reset coord
				{
resetc:
					gCoord.x = theNode->OldCoord.x;
					gCoord.z = theNode->OldCoord.z;
					gDelta.x = gDelta.z = 0;
				}
				else
				if (waterY < GetTerrainY(gCoord.x, gCoord.z))					// or if in water, but water is under terrain, then reset
					goto resetc;


					/* SEE IF PLAYER IS STILL IN THE WATER AND IS CLOSE */

				if (IsPlayerDoingSwimAnim(gPlayerInfo.objNode))
				{
					dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
					if (dist > PONDFISH_CHASE_DIST)
					{
						theNode->Mode = PONDFISH_MODE_WAIT;
					}
							/* ALSO SEE IF CLOSE ENOUGH TO ATTACK */
					else
					if (dist < PONDFISH_ATTACK_DIST)
					{
						if (aim < .8f)							// see if aiming at player
						{
							if (theNode->AttackTimer < 0.0f)
							{
								theNode->TweakJumps = true;
attack:
								MorphToSkeletonAnim(theNode->Skeleton, PONDFISH_ANIM_JUMPATTACK, 6.0);
								theNode->JumpNow = false;
								theNode->IsJumping = false;
							}
						}
					}
				}

						/* SEE IF GO FOR A LURE */

				FindTargetLure(theNode);
				break;


				/************/
				/* GET LURE */
				/************/

		case	PONDFISH_MODE_GETLURE:
				if (VerifyTargetLure(theNode))								// if lure is still kosher then go for it
				{
					ObjNode	*lure = (ObjNode *)theNode->TargetLure;
					float	r;

					TurnObjectTowardTarget(theNode, &gCoord, lure->Coord.x, lure->Coord.z, PONDFISH_TURN_SPEED*5, false);

					r = theNode->Rot.y;
					gDelta.x = -sin(r) * PONDFISH_CHASESPEED;
					gDelta.z = -cos(r) * PONDFISH_CHASESPEED;
					gCoord.x += gDelta.x * fps;
					gCoord.y += gDelta.y * fps;
					gCoord.z += gDelta.z * fps;

					CheckFishBiteLure(theNode, lure);
				}
				break;

	}



		/****************/
		/* DO COLLISION */
		/****************/

	HandleCollisions(theNode, CTYPE_MISC | CTYPE_FENCE, 0);



			/* UPDATE POND FISH */

	UpdatePondFish(theNode);
}


/********************** MOVE PONDFISH: JUMP ATTACK ******************************/

static void  MovePondFish_JumpAttack(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, PONDFISH_TURN_SPEED, false);

			/*************************/
			/* SEE IF START LEAP NOW */
			/*************************/

	if (theNode->JumpNow)
	{
		float	y;

		theNode->JumpNow = false;
		theNode->IsJumping = true;

			/* CALC JUMP AIM DELTA */

		gDelta.x = 0;
		gDelta.y = PONDFISH_JUMPSPEED;
		gDelta.z = 0;
		gCoord.x += gDelta.x * fps;
		gCoord.y += gDelta.y * fps;
		gCoord.z += gDelta.z * fps;

		GetWaterY(gCoord.x, gCoord.z, &y);

		MakeSplash(gCoord.x, y, gCoord.z, 2.0);
		CreateNewRipple(gCoord.x + RandomFloat2() * 200.0f, gCoord.z + RandomFloat2() * 200.0f, 40.0f, 140.0f, .3);
		CreateNewRipple(gCoord.x + RandomFloat2() * 200.0f, gCoord.z + RandomFloat2() * 200.0f, 30.0f, 100.0f, .3);
	}

			/****************/
			/* PROCESS JUMP */
			/****************/

	else
	if (theNode->IsJumping)
	{
		float waterY;

			/* DO GRAVITY & SEE IF SPLASHDOWN */

		gDelta.y -= 3400.0f * fps;					// gravity
		gCoord.x += gDelta.x * fps;
		gCoord.y += gDelta.y * fps;
		gCoord.z += gDelta.z * fps;

		GetWaterY(gCoord.x, gCoord.z, &waterY);

		if ((gCoord.y < waterY) && (gDelta.y < 0.0f))			// see if hit water
		{
			gDelta.x =
			gDelta.y =
			gDelta.z = 0;
			if (theNode->EatPlayer)
				MorphToSkeletonAnim(theNode->Skeleton, PONDFISH_ANIM_MOUTHFULL, 4);
			else
				MorphToSkeletonAnim(theNode->Skeleton, PONDFISH_ANIM_SWIM, 4);
			theNode->IsJumping = false;

			MakeSplash(gCoord.x, waterY, gCoord.z, 2.0);
			CreateNewRipple(gCoord.x + RandomFloat2() * 200.0f, gCoord.z + RandomFloat2() * 200.0f, 40.0f, 140.0f, .3);
			CreateNewRipple(gCoord.x + RandomFloat2() * 200.0f, gCoord.z + RandomFloat2() * 200.0f, 30.0f, 100.0f, .3);
			theNode->AttackTimer = 2.0f + RandomFloat()*2.0f;	// dont attack for n more seconds at least
		}
	}

		/*****************************/
		/* WAITING FOR JUMP TO START */
		/*****************************/

	else
	{
		ApplyFrictionToDeltas(400.0,&gDelta);
		gCoord.x += gDelta.x * fps;
		gCoord.y += gDelta.y * fps;
		gCoord.z += gDelta.z * fps;
	}


		/*********************/
		/* SEE IF EAT PLAYER */
		/*********************/

	if (!theNode->EatPlayer)
		SeeIfFishEatsPlayer(theNode);


			/* UPDATE */

	UpdatePondFish(theNode);
}


/********************** MOVE PONDFISH: CAUGHT ******************************/

static void  MovePondFish_Caught(ObjNode *fish)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*lure = fish->ChainNode;
OGLPoint3D	pt;
float	diffX, diffY, diffZ;

		/* ONCE FISH & LURE ARE OUT OF RANGE THEN DELETE */

	if ((fish->StatusBits & STATUS_BIT_ISCULLED) && (lure->StatusBits & STATUS_BIT_ISCULLED))
	{
		DeleteEnemy(fish);
		return;
	}

			/* MOVE LURE UP */

	lure->Delta.y += 1000.0f * fps;					// accel up
	lure->Coord.y += lure->Delta.y * fps;			// move up
	UpdateObjectTransforms(lure);


			/* ALIGN FISH ON HOOK */

	FindCoordOnJoint(fish, PONDFISH_JOINT_HEAD, &gPondFishMouthOff, &pt);		// get coord of fish mouth

	diffX = pt.x - fish->Coord.x;					// calc offset from fish's origin
	diffY = pt.y - fish->Coord.y;
	diffZ = pt.z - fish->Coord.z;

	fish->Coord.x = lure->Coord.x - diffX;
	fish->Coord.y = (lure->Coord.y - 170.0f) - diffY;
	fish->Coord.z = lure->Coord.z - diffZ;

	UpdateObjectTransforms(fish);
}


/****************** UPDATE POND FISH ********************/

static void UpdatePondFish(ObjNode *theNode)
{
	if (theNode->EatPlayer)
		PondFish_ContinueEatingPlayer(theNode);

		/* ADD WATER RIPPLE */

	if ((theNode->Mode != PONDFISH_MODE_WAIT) && (!theNode->IsJumping))
	{
		theNode->RippleTimer += gFramesPerSecondFrac;
		if (theNode->RippleTimer > .20f)
		{
			theNode->RippleTimer = 0;
//			MakeRipple(gCoord.x, WATER_Y, gCoord.z, 2.0);
		}
	}

	UpdateEnemy(theNode);
}

#pragma mark -



/********************** SEE IF FISH EATS PLAYER *************************/
//
// Does simply collision check to see if fish should eat the player.
//

static void SeeIfFishEatsPlayer(ObjNode *fish)
{
OGLPoint3D	pt;
float		dist;
ObjNode		*player = 	gPlayerInfo.objNode;

	if (player->StatusBits & STATUS_BIT_ONGROUND)				// can be eaten if player is on ground
		return;


			/* GET COORD OF MOUTH */

	FindCoordOnJoint(fish, PONDFISH_JOINT_HEAD, &gPondFishMouthOff, &pt);


		/* SEE IF POINT IS NEAR PLAYER */

	dist = OGLPoint3D_Distance(&pt, &gPlayerInfo.coord);
	if (dist < 100.0f)
	{
		gCurrentEatingFish = fish;
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_EATENBYSNAKE, 7);
		player->CType = 0;							// no more collision
		fish->EatPlayer = true;
		fish->EatenTimer = 3;									// start timer
		PondFish_ContinueEatingPlayer(fish);
	}
}


/****************** PONDFISH: CONTINUE EATING PLAYER *********************/
//
// Called after fish has eaten player.
//

static void PondFish_ContinueEatingPlayer(ObjNode *fish)
{
		/* SEE IF EATEN TIMER HAS TIMED OUT */

	fish->EatenTimer -= gFramesPerSecondFrac;
	if (fish->EatenTimer < 0.0f)
	{
		fish->EatPlayer = false;
		KillPlayer(false);
	}
}


/******************** MOVE PLAYER: EATEN BY FISH ***********************/

void MovePlayer_EatenByFish(ObjNode *player)
{
float	oldTimer;

	FindCoordOnJoint(gCurrentEatingFish, PONDFISH_JOINT_HEAD, &gPondFishMouthOff, &gCoord);			// calc coord to put player
	player->Rot.y = gCurrentEatingFish->Rot.y;


		/* UPDATE */

	UpdateObject(player);

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


#pragma mark -


/******************* FIND TARGET LURE ****************************/

static void FindTargetLure(ObjNode *fish)
{
ObjNode		*lure;

			/********************/
			/* SCAN ALL OBJECTS */
			/********************/

	lure = gFirstNodePtr;

	do
	{
		if (lure->Slot > LURE_SLOT)									// see if past any lures
			break;

		if ((lure->Group == MODEL_GROUP_LEVELSPECIFIC) &&			// is this ObjNode a lure?
			(lure->Type == PARK_ObjType_Lure))
		{
			if (lure->LureMaxWobbleDY > 0.0f)						// is this lure wobbling?
			{
				if (CalcQuickDistance(gCoord.x, gCoord.z, lure->Coord.x, lure->Coord.z) < 2000.0f)	// is this lure in range?
				{
					/* GOT IT */

					fish->TargetLure = (int)lure;
					fish->Mode = PONDFISH_MODE_GETLURE;
					return;
				}
			}
		}
		lure = lure->NextNode;					// next node
	}
	while (lure != nil);
}



/***************** VERIFY TARGET LURE *************************/
//
// Verifies that the fish's target lure is still a valid lure
//

static Boolean VerifyTargetLure(ObjNode *fish)
{
ObjNode	*lure;

	lure = (ObjNode *)fish->TargetLure;									// get objnode of target lure
	if (lure == nil)
		goto bad_lure;

	if (lure->CType == INVALID_NODE_FLAG)								// see if deleted
		goto bad_lure;

	if ((lure->Group != MODEL_GROUP_LEVELSPECIFIC) || (lure->Type != PARK_ObjType_Lure))	// make sure objnode is still the lure
		goto bad_lure;

	if (lure->LureMaxWobbleDY <= 0.0f)										// see if lure is not wobbling, thus inactive now
		goto bad_lure;

		/* LURE IS GOOD */

	return(true);



		/* LURE IS INVALID */

bad_lure:
	fish->TargetLure = nil;
	fish->Mode = PONDFISH_MODE_WAIT;
	return(false);
}


/********************* CHECK FISH BITE LURE ***************************/

static void CheckFishBiteLure(ObjNode *fish, ObjNode *lure)
{
OGLPoint3D	pt;
float		dist;

	if (lure->CType == 0)								// see if lure already got fish
		return;

			/* GET COORD OF MOUTH */

	FindCoordOnJoint(fish, PONDFISH_JOINT_HEAD, &gPondFishMouthOff, &pt);


		/* SEE IF POINT IS LURE */

	dist = CalcDistance(pt.x, pt.z, lure->Coord.x, lure->Coord.z);
	if (dist < 100.0f)
	{
		MorphToSkeletonAnim(fish->Skeleton, PONDFISH_ANIM_CAUGHT, 7);
		PlayEffect3D(EFFECT_FISHFLOP, &fish->Coord);
		PlayEffect3D(EFFECT_SPLASH, &fish->Coord);

				/* SET LURE TO GO UP */

		lure->CType = 0;						// no more collision on the lure
		lure->MoveCall = nil;					// lure won't move on its own anymore
		lure->Delta.y = 100;
		fish->ChainNode = lure;					// chain the lure to the fish

		gNumCaughtFish++;
	}
}














