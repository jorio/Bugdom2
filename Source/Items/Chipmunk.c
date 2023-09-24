/****************************/
/*   	CHIPMUNK.C		    */
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

static void MoveChipmunk(ObjNode *theNode);
static Boolean DoTrig_Chipmunk(ObjNode *chipmunk, ObjNode *who, Byte sideBits);
static void ChipmunkGotAcorn(ObjNode *chipmunk, ObjNode *acorn);
static void MoveChipmunk_Dropping(ObjNode *theNode);
static void MoveChipmunk_Standing(ObjNode *theNode);
static void UpdateChipmunk(ObjNode *theNode);
static void	UpdateChipmunkHeldObj(ObjNode *chipmunk, ObjNode *pow);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	CHIPMUNK_DIALOG_ACTIVATE_DIST	450.0f

enum
{
	CHIPMUNK_KIND_CHECKPOINT,
	CHIPMUNK_KIND_MAP,
	CHIPMUNK_KIND_xxxxx,
	CHIPMUNK_KIND_MOUSETRAP,
	CHIPMUNK_KIND_RACE,

	CHIPMUNK_KIND_INACTIVE
};


enum
{
	CHIPMUNK_ANIM_STAND = 0,
	CHIPMUNK_ANIM_DROPPOW,
	CHIPMUNK_ANIM_STANDHOLDING,
	CHIPMUNK_ANIM_TURNLEFTH,
	CHIPMUNK_ANIM_TURNRIGHTH,
	CHIPMUNK_ANIM_TALKING,
	CHIPMUNK_ANIM_TALKINGHOLDING,
	CHIPMUNK_ANIM_STANDHOLDINGHAND
};

#define	CHIPMUNK_JOINT_LEFTHAND			13
#define	CHIPMUNK_JOINT_RIGHTHAND		15
#define	CHIPMUNK_JOINT_HOLD				17


/*********************/
/*    VARIABLES      */
/*********************/

#define	CheckPointNum	Special[0]

#define	DropPOWNow		Flag[0]

#define	OneHandHold		Flag[1]							// set if holding POW in one hand instead of both hands


/************************* ADD CHIPMUNK *********************************/

Boolean AddChipmunk(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj,*pow = nil;
int		taskCompleted = itemPtr->flags & ITEM_FLAGS_USER1;
int		kind = itemPtr->parm[1];
int		anim;

	if (taskCompleted)												// see if this guy has been done before
		kind = CHIPMUNK_KIND_INACTIVE;

	anim = CHIPMUNK_ANIM_STANDHOLDING;								// assume use this anim

	switch(kind)
	{
		/* DONT DO MOUSETRAP HINT IF ALREADY RESCUED A MOUSE */

		case	CHIPMUNK_KIND_MOUSETRAP:
				if (gPlayerInfo.numMiceRescued > 0)
					return(true);
				anim = CHIPMUNK_ANIM_TALKING;
				break;

			/* SEE IF HOLDING IN ONE HAND */

		case	CHIPMUNK_KIND_RACE:
				anim = CHIPMUNK_ANIM_STANDHOLDINGHAND;
				break;


			/* DONT MAKE MAP CHIPMUNK IF ALREADY HAVE MAP */

		case	CHIPMUNK_KIND_MAP:
				if (gPlayerInfo.hasMap)
					return(true);
				break;

	}


		/*****************/
		/* MAKE SKELETON */
		/*****************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_CHIPMUNK;
	gNewObjectDefinition.animNum	= anim;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= POW_SLOT-2;					// must be BEFORE powerup so that chains work correctly!
	gNewObjectDefinition.moveCall	= MoveChipmunk;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * PI2/8;
	gNewObjectDefinition.scale 		= 3.0;
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Kind = kind;											// which kind of chipmunk
	if (kind == CHIPMUNK_KIND_CHECKPOINT)							// which checkpoint #
		newObj->CheckPointNum = itemPtr->parm[2];

	newObj->DropPOWNow = false;

	newObj->Coord.y -= newObj->BBox.min.y;							// adjust so flush on ground
	UpdateObjectTransforms(newObj);


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_TRIGGER|CTYPE_LOOKAT;
	newObj->CBits			= CBITS_ALLSOLID | CBITS_ALWAYSTRIGGER;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,.5,1);

	newObj->TriggerCallback = DoTrig_Chipmunk;
	newObj->ForceLookAtDist	= CHIPMUNK_DIALOG_ACTIVATE_DIST;


		/***************************/
		/* SEE IF HOLDING ANYTHING */
		/***************************/

	switch(kind)
	{
		case	CHIPMUNK_KIND_CHECKPOINT:
				pow = MakeCheckpoint(&newObj->Coord);
				if (pow)
				{
					pow->StatusBits |= STATUS_BIT_NOMOVE;				// chipmunk has control of it for now
					newObj->ChainNode = pow;
				}
				break;

		case	CHIPMUNK_KIND_MAP:
				pow = MakePOW(POW_KIND_MAP, &newObj->Coord);
				if (pow)
				{
					pow->StatusBits |= STATUS_BIT_NOMOVE;				// chipmunk has control of it for now
					newObj->ChainNode = pow;
				}
				break;

		case	CHIPMUNK_KIND_RACE:
				newObj->OneHandHold = true;								// holding key in one hand
				pow = MakePOW(POW_KIND_REDKEY, &newObj->Coord);
				if (pow)
				{
					pow->StatusBits |= STATUS_BIT_NOMOVE;				// chipmunk has control of it for now
					pow->CType = 0;										// no collision yet
					newObj->ChainNode = pow;

					pow->HoldOffset.x = 2;								// align key in chip's hand
					pow->HoldOffset.y = -4;
					pow->HoldOffset.z = -2;

					pow->HoldRot.y = PI/2;
				}
				break;

	}

	if (pow)															// align the held obj
		UpdateChipmunkHeldObj(newObj, pow);

	return(true);													// item was added
}


/******************* MOVE CHIPMUNK *********************/

static void MoveChipmunk(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveChipmunk_Standing,
					MoveChipmunk_Dropping,
					MoveChipmunk_Standing,
					MoveChipmunk_Standing,			// turn left
					MoveChipmunk_Standing,			// turn right
					MoveChipmunk_Standing,			// talk
					MoveChipmunk_Standing,			// talk hold
					MoveChipmunk_Standing,			// stand hold hand
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}


/******************* MOVE CHIPMUNK: STANDING ****************/

static void MoveChipmunk_Standing(ObjNode *theNode)
{
float	dist, turnDirection, a;
ObjNode	*heldObj = theNode->ChainNode;

	dist = CalcDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);

			/**********************/
			/* TURN TOWARD PLAYER */
			/**********************/

	if (dist < (CHIPMUNK_DIALOG_ACTIVATE_DIST * 2))									// see if close enough to bother
	{
		a = TurnObjectTowardPlayer(theNode, nil, 1.5f, PI/8, &turnDirection);
		if (turnDirection != 0.0f)
		{
			if (turnDirection <= 0.0f)
			{
				if ((theNode->Skeleton->AnimNum != CHIPMUNK_ANIM_TURNLEFTH) && (a > (PI/6.0f)))
					MorphToSkeletonAnim(theNode->Skeleton, CHIPMUNK_ANIM_TURNLEFTH, 10);
			}
			else
			{
				if ((theNode->Skeleton->AnimNum != CHIPMUNK_ANIM_TURNRIGHTH) && (a > (PI/6.0f)))
					MorphToSkeletonAnim(theNode->Skeleton, CHIPMUNK_ANIM_TURNRIGHTH, 10);
			}
		}
				/* MAKE SURE WE'RE IN THE CORRECT STANDING ANIM */

		else
		{
			if (heldObj)
			{
				if (theNode->OneHandHold)															// see which holding standing anim to use
				{
					if (theNode->Skeleton->AnimNum != CHIPMUNK_ANIM_STANDHOLDINGHAND)				// one hand hold
						MorphToSkeletonAnim(theNode->Skeleton, CHIPMUNK_ANIM_STANDHOLDINGHAND, 10);
				}
				else
				{
					if (theNode->Skeleton->AnimNum != CHIPMUNK_ANIM_STANDHOLDING)					// two hands hold
						MorphToSkeletonAnim(theNode->Skeleton, CHIPMUNK_ANIM_STANDHOLDING, 10);
				}
			}
			else
			{
				switch(theNode->Kind)								// see which stand anim to use
				{
					case	CHIPMUNK_KIND_MOUSETRAP:
							if (theNode->Skeleton->AnimNum != CHIPMUNK_ANIM_TALKING)
								MorphToSkeletonAnim(theNode->Skeleton, CHIPMUNK_ANIM_TALKING, 10);
							break;

					default:
							if (theNode->Skeleton->AnimNum != CHIPMUNK_ANIM_STAND)
								MorphToSkeletonAnim(theNode->Skeleton, CHIPMUNK_ANIM_STAND, 10);
				}
			}
		}
	}

			/****************************/
			/* SEE IF DO DIALOG MESSAGE */
			/****************************/

	switch(theNode->Kind)
	{
		case	CHIPMUNK_KIND_CHECKPOINT:
				if (dist < CHIPMUNK_DIALOG_ACTIVATE_DIST)
					DoDialogMessage(DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT4ACORN, 1, .3, &theNode->Coord);
				break;

		case	CHIPMUNK_KIND_MAP:
				if (dist < CHIPMUNK_DIALOG_ACTIVATE_DIST)
					DoDialogMessage(DIALOG_MESSAGE_CHIPMUNK_MAP4ACORN, 1, .3, &theNode->Coord);
				break;

		case	CHIPMUNK_KIND_MOUSETRAP:
				if (gPlayerInfo.numMiceRescued == 0)
				{
					if (dist < (CHIPMUNK_DIALOG_ACTIVATE_DIST * 3/2))
						DoDialogMessage(DIALOG_MESSAGE_CHIPMUNK_MOUSETRAP, 1, .3, &theNode->Coord);
				}
				break;

		case	CHIPMUNK_KIND_RACE:
				switch(gSlotCarRacingMode)
				{
					case	SLOTCAR_RACEMODE_IDLE:
							if (dist < CHIPMUNK_DIALOG_ACTIVATE_DIST)
								DoDialogMessage(DIALOG_MESSAGE_SLOTCAR, 1, .3, &theNode->Coord);
							break;

					default:
							if (gNotifyOfSlotCarWin)
							{
								MorphToSkeletonAnim(theNode->Skeleton, CHIPMUNK_ANIM_DROPPOW, 2);		// make chipmunk drop the key
								DoDialogMessage(DIALOG_MESSAGE_SLOTCARPLAYERWON, 1, 4, &theNode->Coord);
								gNotifyOfSlotCarWin = false;
							}
							break;

				}
				break;
	}

	UpdateChipmunk(theNode);

}


/******************* MOVE CHIPMUNK: DROPPING ****************/

static void MoveChipmunk_Dropping(ObjNode *theNode)
{

	if (theNode->DropPOWNow)								// see if drop it now
	{
		ObjNode	*pow = theNode->ChainNode;
		if (pow)
		{
			pow->StatusBits &= ~STATUS_BIT_NOMOVE;			// let pow move on it's own again
			theNode->ChainNode = nil;						// detach from chipmunk

					/* IF IT'S A CHECKPOINT THEN SET IT */

			if (pow->What == WHAT_CHECKPOINT)
			{
				SetCheckpoint(pow, gPlayerInfo.objNode);
			}
		}
		theNode->DropPOWNow = false;

		theNode->TerrainItemPtr = nil;						// this chipmunk won't ever come back now
	}

	if (theNode->Skeleton->AnimHasStopped)					// see if done w/ this anim
	{
		MorphToSkeletonAnim(theNode->Skeleton, CHIPMUNK_ANIM_STAND, 5);
		theNode->Kind = CHIPMUNK_KIND_INACTIVE;			// turn into a inactive chipmunk
	}


	UpdateChipmunk(theNode);
}


/******************** UPDATE CHIPMUNK ************************/

static void UpdateChipmunk(ObjNode *theNode)
{
ObjNode	*pow = theNode->ChainNode;

			/* KEEP POW ALIGNED */

	if (pow)
		UpdateChipmunkHeldObj(theNode, pow);

	UpdateObjectTransforms(theNode);

	CreateCollisionBoxFromBoundingBox_Rotated(theNode,.6,1);
}

#pragma mark -



/************** DO TRIGGER - CHIPMUNK ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Chipmunk(ObjNode *chipmunk, ObjNode *who, Byte sideBits)
{
ObjNode	*acorn;

	(void) sideBits;

	if (chipmunk->Kind == CHIPMUNK_KIND_RACE)			// make sure we don't do anything with this chipmunk (fixed for 1.0.3 update)
		return(true);

	if (chipmunk->Skeleton->AnimNum == CHIPMUNK_ANIM_DROPPOW)		// only handle trigger if chipmunk doing nothing now
		return(true);

			/* SEE IF PLAYER IS CARRYING AN ACORN */

	acorn = gPlayerInfo.heldObject;									// get held obj
	if (acorn == nil)
		return(true);												// not holding anything so bail

	if (acorn->What != WHAT_ACORN)									// make sure we're holding an acorn
		return(true);


			/* GIVE THE ACORN TO THE CHIPMUNK */

	ChipmunkGotAcorn(chipmunk, acorn);


			/* PLAYER ISN'T HOLDING ANYTHING NOW */

	gPlayerInfo.heldObject = nil;
	SetPlayerStandAnim(who, 6);
	return(true);
}


/******************* CHIPMUNK GOT ACORN **********************/

static void ChipmunkGotAcorn(ObjNode *chipmunk, ObjNode *acorn)
{

	chipmunk->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;					// set user flag so we'll always know this guy's task is done

	DeleteObject(acorn);


			/* SEE WHAT TO DO */

	switch(chipmunk->Kind)
	{
		case	CHIPMUNK_KIND_CHECKPOINT:
				MorphToSkeletonAnim(chipmunk->Skeleton, CHIPMUNK_ANIM_DROPPOW, 2);		// make chipmunk drop the pow
				DoDialogMessage(DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT, 0, 3, &chipmunk->Coord);
				break;

		case	CHIPMUNK_KIND_MAP:
				MorphToSkeletonAnim(chipmunk->Skeleton, CHIPMUNK_ANIM_DROPPOW, 2);		// make chipmunk drop the pow
				DoDialogMessage(DIALOG_MESSAGE_CHIPMUNK_THANKS, 0, 5, &chipmunk->Coord);
				break;
	}

	chipmunk->CType = CTYPE_MISC|CTYPE_BLOCKCAMERA;										// dont trigger anymore
}



/******************** UPDATE CHIPMUNK HELD OBJECT *******************/

static void	UpdateChipmunkHeldObj(ObjNode *chipmunk, ObjNode *pow)
{
OGLMatrix4x4	mst,rm,m2,m;
float			scale;
const OGLPoint3D	zero = {0,0,0};

			/* CALC SCALE MATRIX */

	scale = pow->Scale.x / chipmunk->Scale.x;					// to adjust from chip's scale to pow's scale
	OGLMatrix4x4_SetScale(&mst, scale, scale, scale);

			/* CALC TRANSLATE MATRIX */

	mst.value[M03] = pow->HoldOffset.x;							// set offset
	mst.value[M13] = pow->HoldOffset.y;
	mst.value[M23] = pow->HoldOffset.z;


			/* CALC ROTATE MATRIX */

	OGLMatrix4x4_SetRotate_XYZ(&rm, pow->HoldRot.x, pow->HoldRot.y, pow->HoldRot.z);	// set rotation to fit in hand
	OGLMatrix4x4_Multiply(&mst, &rm, &m2);


			/* GET ALIGNMENT MATRIX */

	if (chipmunk->OneHandHold)
		FindJointFullMatrix(chipmunk, CHIPMUNK_JOINT_RIGHTHAND, &m);
	else
		FindJointFullMatrix(chipmunk, CHIPMUNK_JOINT_HOLD, &m);

	OGLMatrix4x4_Multiply(&m2, &m, &pow->BaseTransformMatrix);
	SetObjectTransformMatrix(pow);


			/* SET REAL POINT FOR CULLING */

	OGLPoint3D_Transform(&zero, &pow->BaseTransformMatrix, &pow->Coord);




}


















