/****************************/
/*   	SNAILS.C		    */
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

static void MoveSnail(ObjNode *theNode);
static void SnailDropKey(ObjNode *snail);
static void MoveScarecrowHead(ObjNode *theNode);
static void ScarecrowHeadLandedOnBody(ObjNode *head, ObjNode *body);
static Boolean IsPlayerCarryingScarecrowHead(void);
static void DropBowlingMarble(ObjNode *player, ObjNode *marble);
static void MoveBowlingMarble(ObjNode *theNode);
static void SeeIfMarbleHitPin(ObjNode *marble);
static void MarbleGotKickedCallback(ObjNode *player, ObjNode *kickedObj);
static void SeeIfPinHitPin(ObjNode *inPin);
static void MoveBowlingPin_Hit(ObjNode *pin);
static Boolean IsPlayerCarryingMarble(void);
static void DropScarecrowHead(ObjNode *player, ObjNode *held);
static ObjNode *FindScarecrow(OGLPoint3D *from);
static void MoveSquishBerry(ObjNode *theNode);
static void SquishTheBerry(ObjNode *berry);
static Boolean DoTrig_Snail(ObjNode *snail, ObjNode *who, Byte sideBits);
static void MoveSquishedBerry(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/



#define	SNAIL_SCALE	1.2f

#define	SCARECROW_SLOT	60
#define	SCARECROW_SCALE	1.1f

enum
{
	PIN_MODE_STANDING,
	PIN_MODE_HIT

};


/*********************/
/*    VARIABLES      */
/*********************/

// #define	TargetSnail		SpecialObjPtr[0]							// which snail this snail shell is thrown to

#define	HasDroppedKey	Flag[0]
#define	KeyColor		Special[1]

Boolean		gHeadOnScarecrow;

static	OGLVector3D		gMarbleDelta;

int			gNumBowlingPinsDown;

#define		Activated		Flag[0]

int			gNumSquishBerries;
int			gNumBerriesSquished;

Boolean		gIgnoreBottleKeySnail = false;


/************************* ADD SNAIL *********************************/

Boolean AddSnail(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*snail;
int		taskCompleted = itemPtr->flags & ITEM_FLAGS_USER1;
int		snailKind = itemPtr->parm[0];

	if (snailKind > 19)			// verify
		return(true);

	if (snailKind == SNAIL_KIND_SODACAN)					// dont add soda snails if we've already popped one
		if (gPoppedSodaCan)
			return(true);

	snail = MakeSnail(SNAIL_SLOT, x, z, snailKind, itemPtr->parm[2], itemPtr->parm[1], taskCompleted);

	snail->TerrainItemPtr = itemPtr;								// keep ptr to item list

	return(true);
}


/********************** MAKE SNAIL ********************************/

ObjNode *MakeSnail(int slot, float x, float z, int snailKind, int keyColor, int rot, Boolean taskCompleted)
{
ObjNode *snail;

		/***********************/
		/* MAKE SNAIL SKELETON */
		/***********************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_SNAIL;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= FindHighestCollisionAtXZ(x, z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= slot;
	gNewObjectDefinition.moveCall	= MoveSnail;
	gNewObjectDefinition.rot 		= (float)rot * (PI2/8);
	gNewObjectDefinition.scale 		= SNAIL_SCALE;

	snail = MakeNewSkeletonObject(&gNewObjectDefinition);

	snail->Kind 		= snailKind;								// remember the snail kind
	snail->KeyColor 	= keyColor;									// remember key color
	snail->What 		= WHAT_SNAIL;
	snail->HasDroppedKey = false;


			/* SET COLLISION STUFF */

	snail->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	snail->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(snail,.8,1);

	if (!taskCompleted)												// make camera auto lookat snails
	{
//		snail->CType |= CTYPE_LOOKAT;
		snail->ForceLookAtDist	= DEFAULT_DIALOG_ACTIVATE_DIST;
	}


		/****************/
		/* ATTACH SHELL */
		/****************/

	if ((snail->Kind != SNAIL_KIND_FINDSHELL) || taskCompleted)		// see if attach shell
	{
		ObjNode	*shell;

		gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
		gNewObjectDefinition.type 		= GLOBAL_ObjType_SnailShell;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.scale 		= SNAIL_SCALE;
		shell = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		snail->ChainNode = shell;

		AlignShellOnSnail(snail, shell);
	}

			/* THIS IS A FINDSHELL SNAIL, SO MAKE HIM A TRIGGER */
	else
	{
		snail->CType |= CTYPE_TRIGGER;
		snail->TriggerCallback = DoTrig_Snail;
	}



	return(snail);													// item was added
}




/******************* MOVE SNAIL *********************/

static void MoveSnail(ObjNode *snail)
{
Boolean	playerInRange;
float	dist;
ObjNode	*shell = snail->ChainNode;

			/* MOVE FORWARD */

	if (snail->Timer > 0.0f)
	{
		float	r = snail->Rot.y;
		float	fps = gFramesPerSecondFrac;

		snail->Timer -= fps;
		if (snail->Timer <= 0.0f)							//  if all done then make snail inactive
			snail->Kind = SNAIL_KIND_INACTIVE;

		snail->Coord.x += -sin(r) * (90.0f * fps);
		snail->Coord.z += -cos(r) * (90.0f * fps);

				/* UPDATE SNAIL */

		if (fabs(snail->Coord.y - GetTerrainY(snail->Coord.x, snail->Coord.z)) < 50.0f)		// keep flush on terrain if on terrain
		{
			RotateOnTerrain(snail, 0, nil);						// set transform matrix
			SetObjectTransformMatrix(snail);
		}
		else
			UpdateObjectTransforms(snail);

		CalcObjectBoxFromNode(snail);
	}


		/*********************/
		/* DO KIND SPECIFICS */
		/*********************/

	dist = CalcQuickDistance(snail->Coord.x, snail->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	playerInRange = (dist < 400.0f);
	if ((snail->Coord.y - gPlayerInfo.coord.y) > 300.0f)		// see if snail is really too high above
		playerInRange = false;

	switch(snail->Kind)
	{
			/* FIND SHELL */

		case	SNAIL_KIND_FINDSHELL:
				if (!shell)
				{
					if (gPlayerInfo.heldObject)
					{
						if (gPlayerInfo.heldObject->Kind == PICKUP_KIND_SNAILSHELL)	// if player is hold shell then don't do anything
							break;
					}

					if (playerInRange)
						DoDialogMessage(DIALOG_MESSAGE_NEEDSNAILSHELL, 1, 4, &snail->Coord);
				}
				break;


			/* SCARECROW HEAD */

		case	SNAIL_KIND_SCARECROWHEAD:
				if (gHeadOnScarecrow)													// see if the head is on the scarecrow now
				{
					DoDialogMessage(DIALOG_MESSAGE_ATTACHEDSCARECROWHEAD, 0, 3.0, &snail->Coord);		// say thanks for doing it
					SnailDropKey(snail);												// drop the key
					snail->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;					// set user flag so we'll always know this snail's task is done
				}
				else																	// still waiting for head to get on there
				{
					if (playerInRange)
					{
						if (IsPlayerCarryingScarecrowHead())
							DoDialogMessage(DIALOG_MESSAGE_PUTSCARECROWHEAD, 1, .3, &snail->Coord);
						else
							DoDialogMessage(DIALOG_MESSAGE_FINDSCARECROWHEAD, 1, .3, &snail->Coord);

					}
				}
				break;


				/* BOWING */

		case	SNAIL_KIND_BOWLING:
				if (gNumBowlingPinsDown >= 10)											// see if all the pins are down
				{
					DoDialogMessage(DIALOG_MESSAGE_DONEBOWLING, 0, 3.0, &snail->Coord);	// say thanks for doing it
					SnailDropKey(snail);												// drop the key
					snail->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;					// set user flag so we'll always know this snail's task is done
				}
				else
				{
					if (playerInRange)
					{
						if (gNumBowlingPinsDown == 0)									// dont talk if player has already knocked some pins down
						{
							if (IsPlayerCarryingMarble())
								DoDialogMessage(DIALOG_MESSAGE_BOWLMARBLE, 1, .3, &snail->Coord);
							else
								DoDialogMessage(DIALOG_MESSAGE_FINDMARBLE, 1, .3, &snail->Coord);
						}
					}
				}
				break;


				/* POOL */

		case	SNAIL_KIND_POOL:
				if (playerInRange)
					DoDialogMessage(DIALOG_MESSAGE_POOLWATER, 3, 4, &snail->Coord);
				break;


				/* SMASH BERRIES */

		case	SNAIL_KIND_SMASHBERRIES:
				if (playerInRange)
					DoDialogMessage(DIALOG_MESSAGE_SMASHBERRIES, 1, 4, &snail->Coord);
				break;

		case	SNAIL_KIND_SMASHBERRIESEND:
				if (playerInRange)
				{
					if (gNumBerriesSquished >= gNumSquishBerries)
					{
						DoDialogMessage(DIALOG_MESSAGE_SQUISHDONE, 1, 4, &snail->Coord);
						SnailDropKey(snail);												// drop the key
						snail->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;					// set user flag so we'll always know this snail's task is done
					}
					else
						DoDialogMessage(DIALOG_MESSAGE_SQUISHMORE, 1, 4, &snail->Coord);

				}
				break;


				/* DOG HOUSE */

		case	SNAIL_KIND_DOGHOUSE:
				if (playerInRange)
				{
					DoDialogMessage(DIALOG_MESSAGE_DOGHOUSE, 1, 4, &snail->Coord);
				}
				break;


				/* SLOT CAR */

		case	SNAIL_KIND_SLOTCAR:
				break;


				/* RESCUE MICE */

		case	SNAIL_KIND_RESCUEMICE:
				if (playerInRange)
				{
					if (gPlayerInfo.numMiceRescued < 4)
					{
						if (gPlayerInfo.numMiceRescued >= 2)								// see if close to done
							DoDialogMessage(DIALOG_MESSAGE_RESCUEMICE2, 1, 4, &snail->Coord);
						else
							DoDialogMessage(DIALOG_MESSAGE_RESCUEMICE, 1, 4, &snail->Coord);
					}
					else
					{
						DoDialogMessage(DIALOG_MESSAGE_MICESAVED, 1, 4, &snail->Coord);
						SnailDropKey(snail);												// drop the key
						snail->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;					// set user flag so we'll always know this snail's task is done
					}
				}
				break;


				/* PUZZLE */

		case	SNAIL_KIND_PUZZLE:
				if (playerInRange)
				{
					if (gNumPuzzlePiecesFit >= 3)											// see if finished
					{
						DoDialogMessage(DIALOG_MESSAGE_DONEPUZZLE, 1, 4, &snail->Coord);
						SnailDropKey(snail);												// drop the key
						snail->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;					// set user flag so we'll always know this snail's task is done
					}
					else
//					if (gNumPuzzlePiecesFit == 0)											// see if haven't even started puzzle yet
						DoDialogMessage(DIALOG_MESSAGE_DOPUZZLE, 1, 4, &snail->Coord);		// not finished yet
				}
				break;

				/* MOTHBALL */

		case	SNAIL_KIND_MOTHBALL:
				if (playerInRange)
				{
					DoDialogMessage(DIALOG_MESSAGE_MOTHBALL, 1, 4, &snail->Coord);
				}
				break;


				/* SILICON DOOR */

		case	SNAIL_KIND_SILICONDOOR:
				if (playerInRange && (!gStartedSiliconDoor))
				{
					DoDialogMessage(DIALOG_MESSAGE_SILICONDOOR, 1, 4, &snail->Coord);
				}
				break;


				/* RED CLOVERS */

		case	SNAIL_KIND_REDCLOVERS:
				if (playerInRange)
				{
					if (gGatheredRedClovers < gTotalRedClovers)
					{
						DoDialogMessage(DIALOG_MESSAGE_GETREDCLOVERS, 1, 4, &snail->Coord);
						gShowRedClovers = true;					// make sure we're showing this
					}
					else
					{
						DoDialogMessage(DIALOG_MESSAGE_GOTREDCLOVERS, 1, 4, &snail->Coord);
						SnailDropKey(snail);												// drop the key
						snail->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;					// set user flag so we'll always know this snail's task is done
						gShowRedClovers = false;				// we can hide this now
					}
				}
				break;


				/* FISHING */

		case	SNAIL_KIND_FISHING:
				if (playerInRange)
				{
					if (gNumCaughtFish < 4)
					{
						if (gNumCaughtFish == 0)
							DoDialogMessage(DIALOG_MESSAGE_GOFISHING, 2, 4, &snail->Coord);
						else
							DoDialogMessage(DIALOG_MESSAGE_MOREFISH, 1, 4, &snail->Coord);
					}
					else
					{
						DoDialogMessage(DIALOG_MESSAGE_THANKSFISH, 0, 4, &snail->Coord);
						SnailDropKey(snail);												// drop the key
						snail->TerrainItemPtr = nil;										// this snail can now vanish forever
						gShowFish = false;													// fish icon can fade out
					}
				}
				break;

				/* PICNIC */

		case	SNAIL_KIND_PICNIC:
				if (playerInRange)
				{
					if (gNumFoodOnBasket < FOOD_TO_GET)
					{
						if (gNumFoodOnBasket == 0)
							DoDialogMessage(DIALOG_MESSAGE_GETFOOD, 1, 8, &snail->Coord);
						else
							DoDialogMessage(DIALOG_MESSAGE_MOREFOOD, 1, 2, &snail->Coord);
					}
					else
					{
						DoDialogMessage(DIALOG_MESSAGE_THANKSFOOD, 0, 3, &snail->Coord);
						SnailDropKey(snail);												// drop the key
						snail->TerrainItemPtr = nil;										// this snail can now vanish forever
						gShowFood = false;
					}
				}
				break;


				/* BEE HIVE */

		case	SNAIL_KIND_BEEHIVE:
				if (dist < 700.0f)
				{
					if (gKindlingCount == 0)
						DoDialogMessage(DIALOG_MESSAGE_GETKINDLING, 3, 1, &snail->Coord);
					else
					if (gKindlingCount < KINDLING_NEEDED)
						DoDialogMessage(DIALOG_MESSAGE_MOREKINDLING, 2, 1, &snail->Coord);
					else
					if (!gBurnKindling)
					{
						DoDialogMessage(DIALOG_MESSAGE_LIGHTFIRE, 1, 1, &snail->Coord);
					}
					else
					if (gNumFreedBees >= NUM_FEE_BEES)
					{
						DoDialogMessage(DIALOG_MESSAGE_ENTERHIVE, 1, 1, &snail->Coord);
					}
				}
				break;

				/* MICE DROWN */

		case	SNAIL_KIND_MICEDROWN:
				if (playerInRange)
				{
					if (gNumDrowningMiceRescued < gNumDrowingMiceToRescue)
						DoDialogMessage(DIALOG_MESSAGE_MICEDROWN, 1, 4, &snail->Coord);
					else
					{
						DoDialogMessage(DIALOG_MESSAGE_THANKSNODROWN, 0, 4, &snail->Coord);
						SnailDropKey(snail);												// drop the key
						snail->TerrainItemPtr = nil;										// this snail can now vanish forever
					}
				}
				break;


				/* GLIDER */

		case	SNAIL_KIND_GLIDER:
				if (playerInRange)
				{
					DoDialogMessage(DIALOG_MESSAGE_GLIDER, 1, 4, &snail->Coord);
				}
				break;


				/* SODA CAN */

		case	SNAIL_KIND_SODACAN:
				if (playerInRange && (!gPoppedSodaCan))
				{
					DoDialogMessage(DIALOG_MESSAGE_SODACAN, 1, 4, &snail->Coord);
				}
				break;


				/* BOTTLE KEY */

		case	SNAIL_KIND_BOTTLEKEY:
				if (playerInRange && (!gIgnoreBottleKeySnail))
				{
					DoDialogMessage(DIALOG_MESSAGE_BOTTLEKEY, 1, 4, &snail->Coord);
				}
				break;

	}


			/****************/
			/* UPDATE SHELL */
			/****************/

	if (shell)
		AlignShellOnSnail(snail, shell);

}


/************** DO TRIGGER - SNAIL ********************/

static Boolean DoTrig_Snail(ObjNode *snail, ObjNode *who, Byte sideBits)
{
ObjNode	*shell = gPlayerInfo.heldObject;

#pragma unused (sideBits)

			/* SEE IF PLAYER HAS THE SHELL */

	if (shell == nil)
		return(true);
	if (shell->Kind != PICKUP_KIND_SNAILSHELL)
		return(true);

		/* ATTACH IT TO THE SNAIL */

	shell->DropCallback = nil;						// don't use shell's drop callback
	PlayerDropObject(who, shell);					// have player do drop
	AttachShellToSnail(shell, snail);				// force it to attach to snail
	return(true);
}


/******************** FIND CLOSEST SNAIL ************************/

ObjNode *FindClosestSnail(OGLPoint3D *from)
{
ObjNode		*thisNodePtr,*best = nil;
float	d,minDist = 10000000;


	thisNodePtr = gFirstNodePtr;

	do
	{
		if (thisNodePtr->Slot > SNAIL_SLOT)					// see if past snail slot
			break;

		if (thisNodePtr->What == WHAT_SNAIL)				// see if this is a snail
		{
			d = CalcQuickDistance(from->x,from->z,thisNodePtr->Coord.x, thisNodePtr->Coord.z);
			if ((d < minDist) && (d < 300.0f))				// see if closest & in range
			{
				minDist = d;
				best = thisNodePtr;
			}
		}
		thisNodePtr = thisNodePtr->NextNode;		// next node
	}
	while (thisNodePtr != nil);

	return(best);
}


/********************** ALIGN SHELL ON SNAIL *********************/

void AlignShellOnSnail(ObjNode *snail, ObjNode *shell)
{
OGLMatrix4x4	m,m2,m3;
float			scale;

			/* CALC SCALE MATRIX */

	scale = shell->Scale.x / snail->Scale.x;					// to adjust from snails' scale to shell's scale
	OGLMatrix4x4_SetScale(&m, scale, scale, scale);

			/* CALC TRANSLATE MATRIX */

	OGLMatrix4x4_SetTranslate(&m2, 0, 45, 5);
	OGLMatrix4x4_Multiply(&m, &m2, &m3);


			/* GET ALIGNMENT MATRIX */

	FindJointFullMatrix(snail, 0, &m);							// get joint's matrix

	OGLMatrix4x4_Multiply(&m3, &m, &shell->BaseTransformMatrix);
	SetObjectTransformMatrix(shell);


			/* GET COORDS FOR OBJECT */

	shell->Coord.x = shell->BaseTransformMatrix.value[M03];
	shell->Coord.y = shell->BaseTransformMatrix.value[M13];
	shell->Coord.z = shell->BaseTransformMatrix.value[M23];

}

#pragma mark -

/********************** SNAIL DROP KEY ***************************/

static void SnailDropKey(ObjNode *snail)
{
ObjNode	*key;
OGLPoint3D	where;

	if (snail->HasDroppedKey)				// make sure we dont do this twice
		return;

	snail->Timer = 3.0f;					// set timer to move snail
	snail->HasDroppedKey = true;

			/* MAKE THE KEY */

	where.x = snail->Coord.x;
	where.y = snail->Coord.y + 20.0f;
	where.z = snail->Coord.z;

	key = MakePOW(POW_KIND_REDKEY + snail->KeyColor, &where);
	key->Rot.y = snail->Rot.y;
	UpdateObjectTransforms(key);
}



#pragma mark -

/************************* ADD SNAIL SHELL *********************************/

Boolean AddSnailShell(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_SnailShell;
	gNewObjectDefinition.scale 		= SNAIL_SCALE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) - gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SNAIL_SLOT + 1;				// set so will chain correctly
	gNewObjectDefinition.moveCall 	= MoveDefaultPickup;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Kind = PICKUP_KIND_SNAILSHELL;							// remember what kind of pickup this is


	newObj->DropCallback = DefaultDropObject;							// set drop callback
	newObj->HoldOffset.x = -15;										// set holding offset for Skip
	newObj->HoldOffset.y = -25;
	newObj->HoldOffset.z = -34;

			/* SET COLLISION STUFF */

	newObj->CType 		= CTYPE_MISC|CTYPE_PICKUP|CTYPE_KICKABLE|CTYPE_LOOKAT;
	newObj->CBits		= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);

	newObj->GotKickedCallback = DefaultGotKickedCallback;			// set callback for being kicked

	newObj->ForceLookAtDist	= 500;


			/* MAKE SHADOW */

	AttachShadowToObject(newObj, 0, 2,3, false);

	return(true);
}

#if 0
/************************ DROP SNAIL SHELL **************************/

static void DropSnailShell(ObjNode *player, ObjNode *held)
{
float	r,dist;
ObjNode	*snail;
OGLVector2D	myAim,toSnail;


			/* FIND SNAIL IN RANGE */

	snail = FindClosestSnail(&gCoord);
	if (snail == nil)									// if no snail in range then do default drop
	{
do_default:
		DefaultDropObject(player, held);
		return;
	}

	if (snail->Kind != SNAIL_KIND_FINDSHELL)			// only allow this kind of snail
		goto do_default;


			/* SEE IF WE'RE AIMING AT IT */

	r = player->Rot.y;									// calc player aim vector
	myAim.x = -sin(r);
	myAim.y = -cos(r);

	toSnail.x = snail->Coord.x - gCoord.x;				// calc vector to snail
	toSnail.y = snail->Coord.z - gCoord.z;
	OGLVector2D_Normalize(&toSnail, &toSnail);

	if (acos(OGLVector2D_Dot(&myAim, &toSnail)) > (PI/3))		// see if aimed at it
		goto do_default;


		/* THROW AT SNAIL */

	dist = OGLPoint3D_Distance(&gCoord, &snail->Coord) * 4.0f;	// calc dist to snail

	held->Delta.x = toSnail.x * dist;
	held->Delta.z = toSnail.y * dist;
	held->Delta.y = 300.0f;

	held->Rot.y = r;

	held->TargetSnail = snail;							// remember which snail it's going to
	held->MoveCall = MoveSnailShellToSnail;				// put into special move mode
}
#endif


/******************* ATTACH SHELL TO SNAIL *********************/

void AttachShellToSnail(ObjNode *shell, ObjNode *snail)
{
	snail->ChainNode = shell;					// chain the shell to the snail
	snail->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;	// set user flag so we'll always know this snail's task is done
	snail->CType = CTYPE_MISC;

	shell->MoveCall = nil;						// shell doesnt move on its own anymore
	shell->CType = 0;							// and no more collision for the shell
	shell->TerrainItemPtr = nil;				// dont ever come back once gone

	if (shell->ShadowNode)
	{
		DeleteObject(shell->ShadowNode);			// nuke shell's shadow
		shell->ShadowNode = nil;
	}

	AlignShellOnSnail(snail, shell);

			/* SAY THANKS! */

	DoDialogMessage(DIALOG_MESSAGE_GOTSNAILSHELL, 0, 6.0, &snail->Coord);


			/* LEAVE THE KEY */

	SnailDropKey(snail);

}


#pragma mark -

/************************* ADD SCARECROW *********************************/

Boolean AddScarecrow(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int		taskCompleted = itemPtr->flags & ITEM_FLAGS_USER1;

			/************/
			/* ADD BODY */
			/************/

	if (itemPtr->parm[0] == 0)
	{
				/* MAIN BODY */

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= GARDEN_ObjType_ScarecrowBody;
		gNewObjectDefinition.scale 		= SCARECROW_SCALE;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
		gNewObjectDefinition.slot 		= SCARECROW_SLOT;
		gNewObjectDefinition.moveCall 	= MoveStaticObject;
		gNewObjectDefinition.rot 		= 0;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

		newObj->What = WHAT_SCARECROW;

				/* SET COLLISION STUFF */

		newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW;
		newObj->CBits			= CBITS_ALLSOLID;
		CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);


				/* SHIRT */

		gNewObjectDefinition.type 		= GARDEN_ObjType_ScarecrowShirt;
		gNewObjectDefinition.flags 		|= STATUS_BIT_DOUBLESIDED;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
		newObj->ChainNode = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* SEE IF ATTACH HEAD TOO */

		if (taskCompleted)
		{
			gNewObjectDefinition.type 		= GARDEN_ObjType_ScarecrowHead;
			gNewObjectDefinition.coord.y	= newObj->CollisionBoxes[0].top;
			gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
			gNewObjectDefinition.slot++;
			newObj->ChainNode->ChainNode = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		}
		else
		{
//			newObj->CType |= CTYPE_LOOKAT;										// since no head yet, make it do a lookat
//			newObj->ForceLookAtDist	= 300;
		}
	}

			/************/
			/* ADD HEAD */
			/************/
	else
	{
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= GARDEN_ObjType_ScarecrowHead;
		gNewObjectDefinition.scale 		= SCARECROW_SCALE;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.coord.y	= FindHighestCollisionAtXZ(x, z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN) - gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
		gNewObjectDefinition.slot 		= SCARECROW_SLOT+1;
		gNewObjectDefinition.moveCall 	= MoveScarecrowHead;
		gNewObjectDefinition.rot 		= 0;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

		newObj->Kind = PICKUP_KIND_SCARECROWHEAD;						// remember what kind of pickup this is
		newObj->What = WHAT_SCARECROWHEAD;

		newObj->DropCallback = DropScarecrowHead;						// set drop callback
		newObj->HoldOffset.x = -14;										// set holding offset for Skip
		newObj->HoldOffset.y = -14;
		newObj->HoldOffset.z = -6;

				/* SET COLLISION STUFF */

		newObj->CType 		= CTYPE_MISC|CTYPE_PICKUP|CTYPE_KICKABLE;
		newObj->CBits		= CBITS_ALLSOLID;
		CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);

		newObj->GotKickedCallback = DefaultGotKickedCallback;			// set callback for being kicked

//		newObj->ForceLookAtDist	= 500;

				/* MAKE SHADOW */

		AttachShadowToObject(newObj, 0, 2,3, true);
	}

	return(true);
}

/************************ DROP SCARECROW HEAD **************************/

static void DropScarecrowHead(ObjNode *player, ObjNode *held)
{
float	r = player->Rot.y;

		/* MAKE SURE NOT BEING DROPPED BEHIND A FENCE */

	if (CheckDropThruFence(player, held))
		return;


		/*******************/
		/* DROP FROM GLIDE */
		/*******************/

	if (IsPlayerDoingGlideAnim(player))
	{
		ObjNode	*scarecrow = FindScarecrow(&player->Coord);

				/* THROW @ SCARECROW */

		if (scarecrow)
		{
			held->Delta.x = (scarecrow->Coord.x - held->Coord.x) * 2.0f;
			held->Delta.z = (scarecrow->Coord.z - held->Coord.z) * 2.0f;
			held->Delta.y = -200.0f;
		}

				/* NORMAL */
		else
		{
			held->Delta.x = gDelta.x;				// match w/ player
			held->Delta.z = gDelta.z;
			held->Delta.y = -300.0f;
		}
	}

		/* DROP FROM OTHER ANIM */
	else
	{
		held->Delta.x = gDelta.x - sin(r) * 100.0f;				// toss the object forward
		held->Delta.z = gDelta.z - cos(r) * 100.0f;
		held->Delta.y = 150.0f;
	}

	held->Rot.y = r;
}


/************************* FIND SCARECROW ****************************/

static ObjNode *FindScarecrow(OGLPoint3D *from)
{
ObjNode	*thisNodePtr = gFirstNodePtr;

	do
	{
		if (thisNodePtr->What == WHAT_SCARECROW)		// see if this is a scarecrow
		{
			float d = CalcQuickDistance(from->x,from->z,thisNodePtr->Coord.x, thisNodePtr->Coord.z);
			if (d < 300.0f)								// see if in range
			{
				return(thisNodePtr);
			}
		}
		thisNodePtr = thisNodePtr->NextNode;		// next node
	}
	while (thisNodePtr != nil);

	return(nil);
}



/********************** MOVE SCARECROW HEAD **************************/

static void MoveScarecrowHead(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;


	GetObjectInfo(theNode);

	gDelta.y -= 3000.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

			/* HANDLE COLLISION AND SEE IF LANDED ON SCARECROW */

	if (HandleCollisions(theNode, CTYPE_WATER | CTYPE_TERRAIN | CTYPE_MISC | CTYPE_FENCE, .4) & SIDE_BITS_BOTTOM)
	{
		for (i = 0; i < gNumCollisions; i++)							// see if we hit the scarecrow
		{
			ObjNode *targetObj 	= gCollisionList[i].objectPtr;			// get hit object
			if (targetObj)
			{
				if ((targetObj->Genre == DISPLAY_GROUP_GENRE) &&
					(targetObj->Group == MODEL_GROUP_LEVELSPECIFIC) &&
					(targetObj->Type == GARDEN_ObjType_ScarecrowBody))
				{
					ScarecrowHeadLandedOnBody(theNode, targetObj);
					goto update;
				}
			}
		}
	}

			/* SEE IF HIT GROUND */

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		if (theNode->StatusBits & STATUS_BIT_UNDERWATER)				// if under water then reset
		{
			gCoord = theNode->InitCoord;
			gDelta.x = gDelta.z = gDelta.y = 0;
		}
		else
		{
			gDelta.x *= .8f;
			gDelta.z *= .8f;
		}
	}


update:
	UpdateObject(theNode);
}


/******************** SCARECROW HEAD LANDED ON BODY ********************/

static void ScarecrowHeadLandedOnBody(ObjNode *head, ObjNode *body)
{
	body->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;		// remember that head is attached

	body->ChainNode->ChainNode = head;

	head->CType 			= 0;
	head->MoveCall 			= nil;			// no more motion
	head->TerrainItemPtr	= nil;			// never coming back

	if (head->ShadowNode)					// get rid of shadow
	{
		DeleteObject(head->ShadowNode);
		head->ShadowNode = nil;
	}

	gCoord.x = body->Coord.x;				// align with body
	gCoord.z = body->Coord.z;
	gCoord.y = body->CollisionBoxes[0].top;

	PlayEffect3D(EFFECT_SQUISHBERRY, &gCoord);

	gHeadOnScarecrow = true;
}


/*************** IS PLAYER CARRYING SCARECROW HEAD **********************/

static Boolean IsPlayerCarryingScarecrowHead(void)
{
ObjNode	*heldObj = gPlayerInfo.heldObject;

	if (!heldObj)
		return(false);

	if (heldObj->What == WHAT_SCARECROWHEAD)
		return(true);

	return(false);

}



#pragma mark -




/************************* ADD BOWLING MARBLE *********************************/

Boolean AddBowlingMarble(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*guts,*sphere;

			/********************/
			/* MAKE MARBLE GUTS */
			/********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PLAYROOM_ObjType_MarbleGuts;
	gNewObjectDefinition.scale 		= .8;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) - gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_DOUBLESIDED | STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-10;
	gNewObjectDefinition.moveCall 	= MoveBowlingMarble;
	gNewObjectDefinition.rot 		= 0;
	guts = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	guts->TerrainItemPtr = itemPtr;								// keep ptr to item list

	guts->DropCallback = DropBowlingMarble;						// set drop callback
	guts->HoldOffset.x = -16;									// set holding offset for Skip
	guts->HoldOffset.y = -12;
	guts->HoldOffset.z = -20;

	guts->Kind = PICKUP_KIND_BOWLINGMARBLE;								// remember what kind of pickup this is
	guts->What = WHAT_MARBLE;

	guts->Activated = false;

			/* SET COLLISION STUFF */

	guts->CType 	= CTYPE_MISC|CTYPE_PICKUP|CTYPE_KICKABLE|CTYPE_LOOKAT;
	guts->CBits		= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(guts,1,1);
	guts->BottomOff	-= 5.0f;

	guts->GotKickedCallback = MarbleGotKickedCallback;			// set callback for being kicked

	guts->ForceLookAtDist	= 500;


			/**********************/
			/* MAKE MARBLE SPHERE */
			/**********************/

	gNewObjectDefinition.type 		= PLAYROOM_ObjType_MarbleShell;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+20;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	sphere = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	sphere->ColorFilter.a = .6;

	guts->ChainNode = sphere;




			/* MAKE SHADOW */

	AttachShadowToObject(guts, 0, 1.5,1.5, false);

	return(true);
}


/************************ DROP BOWLING MARBLE **************************/

static void DropBowlingMarble(ObjNode *player, ObjNode *marble)
{
float	r = player->Rot.y;

	marble->Activated = true;					// once it's been "touched" it can roll on its own

		/* MAKE SURE NOT BEING DROPPED BEHIND A FENCE */

	if (CheckDropThruFence(player, marble))
		return;

		/* DROP FROM GLIDE */

	if (IsPlayerDoingGlideAnim(player))
	{
		marble->Delta.x = gDelta.x;				// match w/ player
		marble->Delta.z = gDelta.z;
		marble->Delta.y = -300.0f;
	}

		/* DROP FROM OTHER ANIM */
	else
	{
		marble->Delta.x = gDelta.x - sin(r) * 100.0f;				// toss the object forward
		marble->Delta.z = gDelta.z - cos(r) * 100.0f;
		marble->Delta.y = 150.0f;
	}

}

/************************* MARBLE GOT KICKED CALLBACK *************************/

static void MarbleGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	kickedObj->Delta.x = -sin(r) * 1100.0f;
	kickedObj->Delta.z = -cos(r) * 1100.0f;
	kickedObj->Delta.y = 600.0f;

	kickedObj->Activated = true;				// once it's been "touched" it can roll on its own

	PlayEffect3D(EFFECT_KICKMARBLE, &kickedObj->Coord);
}


/********************** MOVE BOWLING MARBLE **************************/

static void MoveBowlingMarble(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*sphere = theNode->ChainNode;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


	GetObjectInfo(theNode);

			/* MOVE IT */


	gDelta.y -= 2500.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* DO COLLISION */

	gMarbleDelta = gDelta;												// remember delta
	if (HandleCollisions(theNode, CTYPE_TERRAIN | CTYPE_MISC | CTYPE_FENCE, .1))
	{
		SeeIfMarbleHitPin(theNode);
	}

			/* SPIN */

	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);
	if (theNode->Speed2D > 60.0f)
		TurnObjectTowardTarget(theNode,  &gCoord, gCoord.x + gDelta.x, gCoord.z + gDelta.z, 20, false);

	theNode->Rot.x -= theNode->Speed2D * fps * .02f;


			/* APPLY SLOPE TO DELTAS */

	if (theNode->Activated)
	{
		if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		{
			if (theNode->StatusBits & STATUS_BIT_UNDERWATER)				// if under water then reset
			{
				gCoord = theNode->InitCoord;
				gDelta.x = gDelta.z = gDelta.y = 0;
			}
			else
			{
				DoObjectFriction(theNode, 200);

				gDelta.x += gRecentTerrainNormal.x * (3700.0f * fps);
				gDelta.z += gRecentTerrainNormal.z * (3700.0f * fps);
			}
		}
	}

			/* UPDATE */

	UpdateObject(theNode);

	if (gNumBowlingPinsDown >= 10)											// see if all the pins are down
		theNode->CType &= ~CTYPE_LOOKAT;									// make sure it doesn't lookat once all the pins are down



			/* UPDATE SPHERE */

	sphere->Coord = gCoord;
	sphere->Rot = theNode->Rot;
	UpdateObjectTransforms(sphere);


}

/*************** IS PLAYER CARRYING MARBLE **********************/

static Boolean IsPlayerCarryingMarble(void)
{
ObjNode	*heldObj = gPlayerInfo.heldObject;

	if (!heldObj)
		return(false);

	if (heldObj->What == WHAT_MARBLE)
		return(true);

	return(false);

}



/************************* ADD BOWLING PINS *********************************/

#define	PIN_SEP	80.0f

Boolean AddBowlingPins(TerrainItemEntryType *itemPtr, float  x, float z)
{
int			i;
ObjNode	*pin,*prev = nil;
static const OGLPoint2D pinOffTable[10] =
{
										0,0,
						-PIN_SEP/2,PIN_SEP,	PIN_SEP/2,PIN_SEP,
				-PIN_SEP,PIN_SEP*2,	0,PIN_SEP*2, PIN_SEP,PIN_SEP*2,
	-PIN_SEP*3/2,PIN_SEP*3, -PIN_SEP/2,PIN_SEP*3,	PIN_SEP/2,PIN_SEP*3, PIN_SEP*3/2,PIN_SEP*3
};

OGLPoint2D pinOff[10];
OGLMatrix3x3	m;

			/* TRANSFORM PIN TABLE */

	OGLMatrix3x3_SetRotate(&m, (float)itemPtr->parm[0] * (PI2/8));
	OGLPoint2D_TransformArray(pinOffTable, &m, pinOff, 10);


				/* MAKE PINS */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PLAYROOM_ObjType_Battery;
	gNewObjectDefinition.scale 		= .7;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 64;
	gNewObjectDefinition.moveCall 	= nil;								// no movement until hit - once added, keep these!

	for (i = 0; i < 10; i++)
	{
		gNewObjectDefinition.coord.x 	= x + pinOff[i].x;
		gNewObjectDefinition.coord.z 	= z + pinOff[i].y;
		gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) - gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale;
		gNewObjectDefinition.rot 		= RandomFloat()*PI2;
		pin = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		pin->Mode			= PIN_MODE_STANDING;

		pin->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
		pin->CBits			= CBITS_ALLSOLID;
		CreateCollisionBoxFromBoundingBox_Rotated(pin,1,1);


		gNewObjectDefinition.moveCall = nil;							// only 1st pin moves

		if (prev)
			prev->ChainNode = pin;

		prev = pin;
	}

	return(true);
}


/**************************** MOVE BOWLING PIN: HIT *****************************/

static void MoveBowlingPin_Hit(ObjNode *pin)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(pin);

	gDelta.y -= 2000.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* HANDLE COLLISION*/

	if (HandleCollisions(pin, CTYPE_TERRAIN | CTYPE_MISC | CTYPE_FENCE, .9))
		SeeIfPinHitPin(pin);


			/* DO SPIN */

	pin->Rot.x += pin->DeltaRot.x * fps;
	pin->Rot.y += pin->DeltaRot.y * fps;

	if (pin->Rot.x > PI2)											// keep from wrapping
		pin->Rot.x -= PI2;
	else
	if (pin->Rot.x < -PI2)
		pin->Rot.x += PI2;

	if (pin->Rot.y > PI2)
		pin->Rot.y -= PI2;
	else
	if (pin->Rot.y < -PI2)
		pin->Rot.y += PI2;

			/* HIT GROUND */

	if (pin->StatusBits & STATUS_BIT_ONGROUND)
	{
		gDelta.x *= .95f;
		gDelta.z *= .95f;

		pin->DeltaRot.x *= -.8f;
		pin->DeltaRot.y *= -.8f;

		if (pin->Rot.x >= (PI/2))
		{
			pin->Rot.x *= .8f;
			if (pin->Rot.x < PI/2)
				pin->Rot.x = PI/2;
		}
		else
		if (pin->Rot.x <= (-PI/2))
		{
			pin->Rot.x *= .8f;
			if (pin->Rot.x > (-PI/2))
				pin->Rot.x = -PI/2;
		}
		else
		if (pin->Rot.x >= 0.0f)
			pin->Rot.x = PI/2;
		else
			pin->Rot.x = -PI/2;

	}


		/* SEE IF HIDE */

	if (pin->StatusBits & STATUS_BIT_ISCULLED)			// if culled & out of range, make hidden.  Cannot delete since these are chained
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) > 1000.0f)
		{
			pin->StatusBits |= STATUS_BIT_HIDDEN;
		}
	}


	UpdateObject(pin);
}


/************************ SEE IF MARBLE HIT PIN ****************************/

static void SeeIfMarbleHitPin(ObjNode *marble)
{
int				i;
ObjNode			*hitObj;
OGLVector2D		hitVec,marbleTravel;
float			speed;

	VectorLength2D(marble->Speed2D, gMarbleDelta.x, gMarbleDelta.z);				// calc 2D speed of marble
	speed = marble->Speed2D;


			/* SEE IF ANY COLLISIONS WERE BOWLING PINS */

	for (i = 0; i < gNumCollisions; i++)
	{
		if (gCollisionList[i].type != COLLISION_TYPE_OBJ)				// see if obj
			continue;

		hitObj = gCollisionList[i].objectPtr;							// get the hit obj
		if (hitObj->Genre != DISPLAY_GROUP_GENRE)						// see if it's a pin
			continue;
		if (hitObj->Group != MODEL_GROUP_LEVELSPECIFIC)
			continue;
		if (hitObj->Type != PLAYROOM_ObjType_Battery)
			continue;

				/****************/
				/* WE HIT A PIN */
				/****************/

		if (hitObj->Mode != PIN_MODE_STANDING)							// only hit standing pins
			continue;

		gNumBowlingPinsDown++;

		hitObj->CType		= 0;
		hitObj->Mode 		= PIN_MODE_HIT;
		hitObj->MoveCall 	= MoveBowlingPin_Hit;

		hitObj->BottomOff = hitObj->LeftOff;							// adjust collision box for side landing
		hitObj->TopOff = hitObj->RightOff;

		hitVec.x = hitObj->Coord.x - gCoord.x;							// calc vector of impact
		hitVec.y = hitObj->Coord.z - gCoord.z;
		FastNormalizeVector2D(hitVec.x, hitVec.y, &hitVec, true);

		hitObj->Delta.x = hitVec.x * (speed * .6f);						// give momentum to the pin
		hitObj->Delta.z = hitVec.y * (speed * .6f);
		hitObj->Delta.y = 300.0f + RandomFloat() * (speed * .5f);

		hitObj->DeltaRot.x = RandomFloat2() * (speed * .018f);
		hitObj->DeltaRot.y = RandomFloat2() * (speed * .01f);


					/* DEFLECT MARBLE */

		marbleTravel.x = gMarbleDelta.x;
		marbleTravel.y = gMarbleDelta.z;
		hitVec.x = -hitVec.x;
		hitVec.y = -hitVec.y;
		ReflectVector2D(&marbleTravel, &hitVec, &marbleTravel);
		gDelta.x = gMarbleDelta.x += (marbleTravel.x * .3);				// apply some deflection to the real delta
		gDelta.z = gMarbleDelta.z += (marbleTravel.y * .3);

		PlayEffect3D(EFFECT_BOWLINGHIT, &hitObj->Coord);

	}

}


/************************ SEE IF PIN HIT PIN ****************************/

static void SeeIfPinHitPin(ObjNode *inPin)
{
int				i;
ObjNode			*hitObj;
OGLVector2D		hitVec;
float			speed;

	VectorLength2D(inPin->Speed2D, gMarbleDelta.x, gMarbleDelta.z);				// calc 2D speed of inpin
	speed = inPin->Speed2D;


			/* SEE IF ANY COLLISIONS WERE BOWLING PINS */

	for (i = 0; i < gNumCollisions; i++)
	{
		if (gCollisionList[i].type != COLLISION_TYPE_OBJ)				// see if obj
			continue;

		hitObj = gCollisionList[i].objectPtr;							// get the hit obj
		if (hitObj->Genre != DISPLAY_GROUP_GENRE)						// see if it's a pin
			continue;
		if (hitObj->Group != MODEL_GROUP_LEVELSPECIFIC)
			continue;
		if (hitObj->Type != PLAYROOM_ObjType_Battery)
			continue;

				/****************/
				/* WE HIT A PIN */
				/****************/

		if (hitObj->Mode != PIN_MODE_STANDING)							// only hit standing pins
			continue;

		gNumBowlingPinsDown++;

		hitObj->CType		= 0;
		hitObj->Mode 		= PIN_MODE_HIT;
		hitObj->MoveCall 	= MoveBowlingPin_Hit;

		hitObj->BottomOff = hitObj->LeftOff;							// adjust collision box for side landing
		hitObj->TopOff = hitObj->RightOff;

		hitVec.x = hitObj->Coord.x - gCoord.x;							// calc vector of impact
		hitVec.y = hitObj->Coord.z - gCoord.z;
		FastNormalizeVector2D(hitVec.x, hitVec.y, &hitVec, true);

		hitObj->Delta.x = hitVec.x * (speed * .6f);						// give momentum to the pin
		hitObj->Delta.z = hitVec.y * (speed * .6f);
		hitObj->Delta.y = 300.0f + RandomFloat() * (speed * .5f);

		hitObj->DeltaRot.x = RandomFloat2() * (speed * .015f);
		hitObj->DeltaRot.y = RandomFloat2() * (speed * .01f);

		PlayEffect3D(EFFECT_BOWLINGHIT, &hitObj->Coord);

	}

}


#pragma mark -

/********************** COUNT SQUISH BERRIES ************************/

void CountSquishBerries(void)
{
int						i;
TerrainItemEntryType 	*itemPtr;

	gNumSquishBerries = 0;
	gNumBerriesSquished = 0;


	itemPtr = *gMasterItemList; 											// get pointer to data inside the LOCKED handle

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_SQUISHBERRY)						// see if it's a Squish Berry item
			gNumSquishBerries++;
	}

}


/************************* ADD SQUISH BERRY *********************************/

Boolean AddSquishBerry(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

			/* MAKE SQUISHED BERRY */

	if (itemPtr->flags & ITEM_FLAGS_USER1)
	{
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= SIDEWALK_ObjType_SquishSplat;
		gNewObjectDefinition.scale 		= 1.0;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) - gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale + 1.0f;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
		gNewObjectDefinition.moveCall 	= MoveSquishedBerry;
		gNewObjectDefinition.rot 		= RandomFloat() * PI2;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

		newObj->Kind = PICKUP_KIND_SQUISHBERRY;							// remember what kind of pickup this is

	}
			/**************/
			/* MAKE BERRY */
			/**************/
	else
	{
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= SIDEWALK_ObjType_SquishBerry;
		gNewObjectDefinition.scale 		= 1.0;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) - gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot 		= 258;
		gNewObjectDefinition.moveCall 	= MoveSquishBerry;
		gNewObjectDefinition.rot 		= RandomFloat() * PI2;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

		newObj->Kind = PICKUP_KIND_SQUISHBERRY;							// remember what kind of pickup this is


		newObj->GotKickedCallback = DefaultGotKickedCallback;			// set callback for being kicked
		newObj->DropCallback = DefaultDropObject;						// set drop callback
		newObj->HoldOffset.x = -15;										// set holding offset for Skip
		newObj->HoldOffset.y = -35;
		newObj->HoldOffset.z = -34;

				/* SET COLLISION STUFF */

		newObj->CType 		= CTYPE_PICKUP|CTYPE_KICKABLE;				// note: dont set MISC since the ball needs to roll thru these
		newObj->CBits		= CBITS_ALLSOLID;
		CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);


				/* MAKE SHADOW */

		AttachShadowToObject(newObj, 0, 1.5,1.5, false);
	}
	return(true);
}


/********************** MOVE SQUISH BERRY **************************/

static void MoveSquishBerry(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*ball;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

			/* DO GENERIC PICKUP STUFF */

	GetObjectInfo(theNode);

	gDelta.y -= 3000.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	HandleCollisions(theNode, CTYPE_TERRAIN | CTYPE_MISC | CTYPE_FENCE, .4);

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		if (theNode->StatusBits & STATUS_BIT_UNDERWATER)				// if under water then reset
		{
			gCoord = theNode->InitCoord;
			gDelta.x = gDelta.z = gDelta.y = 0;
		}
		else
		{
			gDelta.x *= .8f;
			gDelta.z *= .8f;
		}
	}

	UpdateObject(theNode);


			/* SEE IF GOT SQUISHED BY BALL */

	ball = gPlayerInfo.ridingBall;									// see if player is riding the ball
	if (ball)
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, ball->Coord.x, ball->Coord.z) < 120.0f)		// see if ball is on this berry
		{
			SquishTheBerry(theNode);
			return;
		}
	}
}


/****************** SQUISH THE BERRY ***********************/

static void SquishTheBerry(ObjNode *berry)
{
	ExplodeGeometry(berry, 500, SHARD_MODE_FROMORIGIN, 1, 1.0f);

		/* CHANGE BERRY INTO SQUISH */

	berry->Type = SIDEWALK_ObjType_SquishSplat;
	ResetDisplayGroupObject(berry);

		/* ADJUST COORD & KILL COLLISION */

	berry->Rot.y = RandomFloat()*PI2;
	berry->Coord.y = GetTerrainY(berry->Coord.x, berry->Coord.z) + 1.0f;
	UpdateObjectTransforms(berry);
	berry->CType = 0;

	berry->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;					// set user flag so we'll always know this guy is squished

	berry->MoveCall = MoveSquishedBerry;									// make call default move call
	berry->GotKickedCallback = nil;
	berry->DropCallback = nil;


			/* DELETE SHADOW */

	if (berry->ShadowNode)
	{
		DeleteObject(berry->ShadowNode);
		berry->ShadowNode = nil;
	}

	PlayEffect3D(EFFECT_SQUISHBERRY, &berry->Coord);
	gNumBerriesSquished++;

}


/********************* MOVE SQUISHED BERRY ******************/

static void MoveSquishedBerry(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	RotateOnTerrain(theNode, 1, nil);							// set transform matrix
	SetObjectTransformMatrix(theNode);

}





