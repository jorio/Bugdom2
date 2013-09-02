/****************************/
/*   	BUDDY BUG.C    			*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	OGLPoint3D				gCoord;
extern	OGLVector3D				gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float					gFramesPerSecondFrac;
extern	short					gNumCollisions;
extern	CollisionRec			gCollisionList[];
extern	unsigned long 			gScore;
extern	float 					gCameraDistFromMe;
extern	u_long 					gAutoFadeStatusBits;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveMyBuddy(ObjNode *theNode);
static void BuddyFollowsPlayer(ObjNode *theNode);
static Boolean MoveBuddyTowardEnemy(ObjNode *theNode);
static void SplatterBuddy(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	BUDDY_DIST_FROM_ME	70.0f
#define	BUDDY_ACCEL			4.0f
#define	BUDDY_CLOSEST		30.0f
#define	BUDDY_HEIGHT_FACTOR	0.3f
#define	BUDDY_MINY			60.0f
#define	BUDDY_ATTACK_SPEED	1200.0f

enum
{
	BUDDY_MODE_LIKESME,
	BUDDY_MODE_ATTACK
};


/*********************/
/*    VARIABLES      */
/*********************/


/********************** CREATE MY BUDDY ****************************/
//
// Called when the buddy powerup nut is cracked open
//

void CreateMyBuddy(OGLPoint3D *where)
{
ObjNode *newObj;

	if (gPlayerInfo.numBuddyBugs >= MAX_BUDDY_BUGS)			// see if full
		return;

	gNewObjectDefinition.type 		= SKELETON_TYPE_BUDDYBUG;
	gNewObjectDefinition.animNum 	= 0;
	gNewObjectDefinition.coord		= *where;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= PLAYER_SLOT+1;
	gNewObjectDefinition.moveCall 	= MoveMyBuddy;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .6;
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	newObj->Mode = BUDDY_MODE_LIKESME;

	newObj->Skeleton->AnimSpeed = 3.0f;

	SetObjectCollisionBounds(newObj, 35,-35,-35,35,35,-35);

			/* SET OFFSET STUFF */

	newObj->Timer = RandomFloat();									// random time to re-scatter
	newObj->TargetOff.x = RandomFloat2() * 50.0f;					// random scatter
	newObj->TargetOff.y = RandomFloat2() * 50.0f;
	newObj->TargetOff.z = RandomFloat2() * 50.0f;

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, .5, .5, true);

	gPlayerInfo.buddyBugs[gPlayerInfo.numBuddyBugs++] = newObj;
}


/********************* MOVE MY BUDDY ************************/

static void MoveMyBuddy(ObjNode *theNode)
{

	GetObjectInfo(theNode);

	switch(theNode->Mode)
	{
		case	BUDDY_MODE_LIKESME:
				BuddyFollowsPlayer(theNode);
				break;

		case	BUDDY_MODE_ATTACK:
				if (MoveBuddyTowardEnemy(theNode))
					return;
				break;
	}

		/**********/
		/* UPDATE */
		/**********/

	if (theNode->EffectChannel == -1)
	{
		theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_BUDDYBUZZ, &gCoord, NORMAL_CHANNEL_RATE+0x8000+(MyRandomLong()&0x1fff), .08);
	}
	else
		Update3DSoundChannel(EFFECT_BUDDYBUZZ, &theNode->EffectChannel, &gCoord);


	UpdateObject(theNode);
}


/******************* LAUNCH BUDDY BUG *******************/

void SeeIfLaunchBuddyBug(void)
{
ObjNode	*buddy;
int		i;

	if (!gControlNeeds[kNeed_LaunchBuddy].newButtonPress)							// see if do it
		return;

	if (gPlayerInfo.numBuddyBugs == 0)								// see if have any buddies
		return;

	i = --gPlayerInfo.numBuddyBugs;									// dec count

	buddy = gPlayerInfo.buddyBugs[i];								// get buddy obj
	gPlayerInfo.buddyBugs[i] = nil;									// buddy isnt attached to me anymore

	if (buddy)
	{
		buddy->Mode = BUDDY_MODE_ATTACK;
		PlayEffect3D(EFFECT_BUDDYLAUNCH, &buddy->Coord);
	}
}


/****************** BUDDY FOLLOWS PLAYER ***********************/

static void BuddyFollowsPlayer(ObjNode *theNode)
{
OGLPoint3D	from,target;
float		distX,distZ,dist;
OGLVector2D	pToC;
float		myX,myY,myZ;
float		fps = gFramesPerSecondFrac;
ObjNode		*player = gPlayerInfo.objNode;

			/* UPDATE OFFSETS */

	theNode->Timer -= fps;
	if (theNode->Timer <= 0.0f)
	{
		theNode->Timer = RandomFloat();							// random time to re-scatter
		theNode->TargetOff.x = RandomFloat2() * 40.0f;					// random scatter
		theNode->TargetOff.y = RandomFloat2() * 40.0f;
		theNode->TargetOff.z = RandomFloat2() * 40.0f;
	}


			/*************************/
			/* UPDATE BUDDY POSITION */
			/*************************/

	myX = player->Coord.x;										// get player coords
	myY = player->Coord.y;
	myZ = player->Coord.z;

	myX += theNode->TargetOff.x;								// offset
	myY += theNode->TargetOff.y;
	myZ += theNode->TargetOff.z;

	pToC.x = gCoord.x - myX;									// calc player->buddy vector
	pToC.y = gCoord.z - myZ;
	FastNormalizeVector2D(pToC.x, pToC.y, &pToC, true);			// normalize it

	target.x = myX + (pToC.x * BUDDY_DIST_FROM_ME);				// target is appropriate dist based on buddy's current coord
	target.z = myZ + (pToC.y * BUDDY_DIST_FROM_ME);


			/* MOVE BUDDY TOWARDS POINT */

	distX = target.x - gCoord.x;
	distZ = target.z - gCoord.z;

	if (distX > 500.0f)											// pin max accel factor
		distX = 500.0f;
	else
	if (distX < -500.0f)
		distX = -500.0f;
	if (distZ > 500.0f)
		distZ = 500.0f;
	else
	if (distZ < -500.0f)
		distZ = -500.0f;

	from.x = gCoord.x+(distX * (fps * BUDDY_ACCEL));
	from.z = gCoord.z+(distZ * (fps * BUDDY_ACCEL));


		/* CALC FROM Y */

	dist = CalcQuickDistance(from.x, from.z, myX, myZ) - BUDDY_CLOSEST;
	if (dist < 0.0f)
		dist = 0.0f;

	target.y = myY + (dist*BUDDY_HEIGHT_FACTOR) + BUDDY_MINY;	// calc desired y based on dist and height factor


		/* MOVE ABOVE ANY SOLID OBJECT */

#if 0
	if (DoSimpleBoxCollision(target.y + 50.0f, target.y - 50.0f,
							target.x - 50.0f, target.x + 50.0f,
							target.z + 50.0f, target.z - 50.0f,
							CTYPE_MISC|CTYPE_ENEMY|CTYPE_TRIGGER))
	{
		ObjNode *obj = gCollisionList[0].objectPtr;					// get collided object
		if (obj)
		{
			CollisionBoxType *coll = obj->CollisionBoxes;			// get object's collision box
			if (coll)
			{
				if ((coll->top - gCoord.y) < 200.0f)				// if not too far up then move it up
					target.y = coll->top + 40.0f;					// set target on top of object
			}
		}
	}
#endif

	dist = (target.y - gCoord.y)*BUDDY_ACCEL;						// calc dist from current y to desired y
	from.y = gCoord.y+(dist*fps);


			/* MAKE SURE NOT UNDERGROUND */

	dist = GetTerrainY(from.x, from.z) + 50.0f;
	if (from.y < dist)
		from.y = dist;

	gCoord = from;


				/* AIM HIM AT ME */

	TurnObjectTowardTarget(theNode, &from, myX, myZ, 3.0, false);


	/* MATCH UNDERWATER BITS SO SHADOW WILL GO AWAY AUTOMATICALLY */

	if (player->StatusBits & STATUS_BIT_UNDERWATER)
		theNode->StatusBits |= STATUS_BIT_UNDERWATER;
	else
		theNode->StatusBits &= ~STATUS_BIT_UNDERWATER;

}


/******************* MOVE BUDDY TOWARD ENEMY *************************/
//
// Returns true if buddy was deleted
//

static Boolean MoveBuddyTowardEnemy(ObjNode *theNode)
{
ObjNode	*target;
float	fps = gFramesPerSecondFrac;

			/* FIND CLOSEST TARGET OBJ */

	target = FindClosestCType(&gCoord, CTYPE_BUDDYATTRACT, true);


			/* AIM AT TARGET */

	if (target)
	{
		TurnObjectTowardTarget(theNode, &gCoord, target->Coord.x, target->Coord.z, 4.0, false);

		if (gCoord.y < (target->Coord.y + target->BottomOff))
			gDelta.y += 400.0f * fps;
		else
		if (gCoord.y > (target->Coord.y + target->TopOff))
			gDelta.y -= 400.0f * fps;
		else
			gDelta.y *= .5f;
	}


			/* CALC DELTA & MOVE */

	gDelta.x = -sin(theNode->Rot.y) * BUDDY_ATTACK_SPEED;
	gDelta.z = -cos(theNode->Rot.y) * BUDDY_ATTACK_SPEED;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;



		/* SEE IF HIT FLOOR */

	if (gCoord.y < GetTerrainY(gCoord.x, gCoord.z))
	{
explode:
		SplatterBuddy(theNode);
		return(true);
	}


		/******************/
		/* SEE IF HIT OBJ */
		/******************/

	if (HandleCollisions(theNode, CTYPE_BUDDYATTRACT|CTYPE_FENCE, 0))
	{
		if (gNumCollisions)														// see if hit any objNodes (enemies)
		{
			target = gCollisionList[0].objectPtr;								// get objNode of the enemy we hit
			if (target)
			{
				if (target->HurtCallback)
					target->HurtCallback(target, 1.0f);

				if (target->What == WHAT_KINDLING)						// see if bee hit some kindling
				{
					IgniteKindling(target);
				}
			}
		}
		goto explode;
	}

		/***************/
		/* SEE IF GONE */
		/***************/

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return(true);
	}


	return(false);
}


/********************* SPLATTER BUDDY **************************/

static void SplatterBuddy(ObjNode *theNode)
{
	PlayEffect3D(EFFECT_BUDDYBOOM, &gCoord);

				/* MAKE PARTICLE EXPLOSION */

	MakeSparkExplosion(gCoord.x, gCoord.y, gCoord.z, 300.0f, 1.0, PARTICLE_SObjType_GreenSpark, 200, 1.0);
	MakeSparkExplosion(gCoord.x, gCoord.y, gCoord.z, 150.0f, 1.0, PARTICLE_SObjType_BlueSpark, 100, .6);

	DeleteObject(theNode);


}

