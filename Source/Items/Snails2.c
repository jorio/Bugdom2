/****************************/
/*   	SNAILS2.C		    */
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

static void MovePuzzlePiece(ObjNode *theNode);
static void MoveHangerOnSpline(ObjNode *theNode);
static Boolean DoTrig_Hanger(ObjNode *theNode, ObjNode *who, Byte sideBits);
static void MoveFishingLure(ObjNode *theNode);
static Boolean DoTrig_FishingLure(ObjNode *theNode, ObjNode *who, Byte sideBits);
static Boolean DoTrig_PicnicBasket(ObjNode *basket, ObjNode *who, Byte sideBits);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	PUZZLE_SCALE	1.0f


/*********************/
/*    VARIABLES      */
/*********************/

int		gNumPuzzlePiecesFit;

int		gGatheredRedClovers, gTotalRedClovers;

int		gNumFoodOnBasket;

#define	PieceNum 	Special[0]


/************************* ADD PUZZLE *********************************/

Boolean AddPuzzle(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int		part = itemPtr->parm[0];

				/***************************/
				/* SEE IF MAKE MAIN PUZZLE */
				/***************************/

	if (part == 0)
	{
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= PLAYROOM_ObjType_PuzzleMain;
		gNewObjectDefinition.scale 		= PUZZLE_SCALE;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
		gNewObjectDefinition.slot 		= 454;
		gNewObjectDefinition.moveCall 	= MoveStaticObject;
		gNewObjectDefinition.rot 		= 0;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

		newObj->What = WHAT_PUZZLE;

				/* SET COLLISION STUFF */

		newObj->CType 		= CTYPE_MISC|CTYPE_BLOCKSHADOW;
		newObj->CBits		= CBITS_ALLSOLID;
		CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);
		newObj->BottomOff = -100;
		CalcObjectBoxFromNode(newObj);
	}

				/*********************/
				/* MAKE PUZZLE PIECE */
				/*********************/
	else
	{
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= PLAYROOM_ObjType_PuzzleMain + part;
		gNewObjectDefinition.scale 		= PUZZLE_SCALE;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
		gNewObjectDefinition.slot 		= 454;
		gNewObjectDefinition.moveCall 	= MovePuzzlePiece;
		gNewObjectDefinition.rot 		= RandomFloat() * PI2;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

		newObj->Kind = PICKUP_KIND_PUZZLEPIECE;							// remember what kind of pickup this is
		newObj->PieceNum = part-1;										// remember which puzzle piece it is too

		newObj->DropCallback = DefaultDropObject;							// set drop callback


				/* SET HOLDING OFFSETS */

		newObj->HoldOffset.y = -3;
		newObj->HoldRot.x = -PI/4;

		switch(part)
		{
			case	1:													// corner
					newObj->HoldOffset.x = -12;
					newObj->HoldOffset.z = -40;
					break;

			case	2:													// side
					newObj->HoldOffset.x = -10;
					newObj->HoldOffset.z = -45;
					break;

			case	3:													// center
					newObj->HoldOffset.x = -19;
					newObj->HoldOffset.z = -51;
					break;

		}

				/* SET COLLISION STUFF */

		newObj->CType 		= CTYPE_MISC|CTYPE_PICKUP;
		newObj->CBits		= CBITS_ALLSOLID;
		CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);


				/* MAKE SHADOW */

		AttachShadowToObject(newObj, 0, 3,3, false);
	}

	return(true);
}


/********************** MOVE PUZZLE PIECE **************************/

static void MovePuzzlePiece(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;
const 	OGLPoint2D	offsets[3] =
{
	112.0f * PUZZLE_SCALE, -146.0f * PUZZLE_SCALE,			// corner piece
	-101.0f * PUZZLE_SCALE, 15.0f * PUZZLE_SCALE,			// side piece
	40.0f * PUZZLE_SCALE, 101.0 * PUZZLE_SCALE,				// middle piece
};


	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

	gDelta.y -= 3000.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

			/****************/
			/* DO COLLISION */
			/****************/

	HandleCollisions(theNode, CTYPE_TERRAIN | CTYPE_MISC | CTYPE_FENCE, .4);

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		gDelta.x *= .8f;
		gDelta.z *= .8f;
	}


			/* SEE IF LANDED ON PUZZLE */

	for (i = 0; i < gNumCollisions; i++)
	{
		ObjNode *puz = gCollisionList[i].objectPtr;

		if (puz == nil)											// see if it's an object collision
			continue;

		if (puz->What == WHAT_PUZZLE)							// see if this is the puzzle body
		{
			float	tx = puz->Coord.x + offsets[theNode->PieceNum].x;
			float	tz = puz->Coord.z + offsets[theNode->PieceNum].y;
			float	dist = CalcQuickDistance(tx, tz, gCoord.x, gCoord.z);		// calc dist from piece to empty hole

					/* SEE IF CLOSE ENOUGH TO MAKE PIECE FIT */

			if (dist < 50.0f)
			{
				theNode->Rot.y = puz->Rot.y;
				gCoord.x = tx;
				gCoord.z = tz;
				gCoord.y = puz->Coord.y;

				theNode->CType = 0;								// no more collision, just make it a dummy object
				theNode->MoveCall = nil;						// keep it here

				gNumPuzzlePiecesFit++;

				PlayEffect_Parms3D(EFFECT_SKIPLAND, &gCoord, NORMAL_CHANNEL_RATE*3/2, 1.0);	// click when puzzle piece fits
			}
		}
	}


	UpdateObject(theNode);

}



#pragma mark -

/************************ PRIME HANGER *************************/

Boolean PrimeHanger(int splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj, *trig;
float			x,z,splineIndex,x2,z2,y;

	gGatheredRedClovers = 0;
	gTotalRedClovers = 0;

	splineIndex = 0;
	GetCoordOnSplineFromIndex(&gSplineList[splineNum], 0, &x, &z);	// get y coord @ start
	y = GetTerrainY(x,z) + 2200.0f;

			/* ADD MULTIPLE HANGARS ON THE SPLINE */

	while(true)						// add until we reach the end of the spline
	{

				/* GET SPLINE INFO */

		GetCoordOnSplineFromIndex(&gSplineList[splineNum], splineIndex, &x, &z);
		GetCoordOnSplineFromIndex(&gSplineList[splineNum], splineIndex+1, &x2, &z2);


					/***************/
					/* MAKE HANGER */
					/***************/

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.scale 		= 2.0;
		gNewObjectDefinition.type 		= CLOSET_ObjType_Hanger;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.coord.y 	= y;
		gNewObjectDefinition.flags 		= STATUS_BIT_ONSPLINE|STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot 		= 200;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


					/* SET BETTER INFO */

		newObj->Rot.y = CalcYAngleFromPointToPoint(newObj->Rot.y, x, z, x2, z2);			// calc y rot aim
		UpdateObjectTransforms(newObj);														// update transforms

		newObj->SplineItemPtr 	= itemPtr;
		newObj->SplineNum 		= splineNum;
		newObj->SplinePlacement = 0;
		newObj->SplineMoveCall 	= MoveHangerOnSpline;					// set move call

					/*******************/
					/* MAKE RED CLOVER */
					/*******************/

		gNewObjectDefinition.type 		= CLOSET_ObjType_RedClover;
		gNewObjectDefinition.coord.y 	= newObj->Coord.y - 130.0f;
		gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_DOUBLESIDED;
		gNewObjectDefinition.scale 		= .5;
		gNewObjectDefinition.slot++;
		gNewObjectDefinition.moveCall 	= nil;
		trig = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				/* SET COLLISION INFO */

		trig->CType 		= CTYPE_TRIGGER;
		trig->CBits			= 0;
		CreateCollisionBoxFromBoundingBox(trig,1.3,1.5);
		trig->TriggerCallback = DoTrig_Hanger;

		newObj->ChainNode = trig;
		trig->ChainHead = newObj;


				/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

		DetachObject(newObj, true);										// detach this object from the linked list
		AddToSplineObjectList(newObj, true);

		gTotalRedClovers++;


					/* PREPARE FOR NEXT HANGER */

		y -= 100.0f;
		splineIndex += 370.0f;
		if (splineIndex >= gSplineList[splineNum].numPoints)			// see if past end
			break;
	}

	return(true);
}


/******************** MOVE HANGER ON SPLINE ***************************/

static void MoveHangerOnSpline(ObjNode *theNode)
{
Boolean isInRange;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility

			/* UPDATE STUFF IF IN RANGE */

	if (isInRange && (!(theNode->StatusBits & STATUS_BIT_ISCULLED)))
	{
		ObjNode	*clover = theNode->ChainNode;
		if (clover)
		{
			clover->Rot.y += gFramesPerSecondFrac * PI2;
			UpdateObjectTransforms(clover);


		}
	}
}


/************** DO TRIGGER - HANGER ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Hanger(ObjNode *theNode, ObjNode *who, Byte sideBits)
{
	(void) who;
	(void) sideBits;

	gShowRedClovers = true;					// make sure we're showing this

	gGatheredRedClovers++;					// inc count

	ObjNode	* hanger = theNode->ChainHead;	// get parent
	hanger->ChainNode = nil;				// detach

	StartPowerupVanish(theNode);

	return(false);
}


#pragma mark -


/********************* ADD FISHING LURE *************************/

Boolean AddFishingLure(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PARK_ObjType_Lure;
	gNewObjectDefinition.scale 		= 1.7;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	GetWaterY(x,z, &gNewObjectDefinition.coord.y);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= LURE_SLOT;
	gNewObjectDefinition.moveCall 	= MoveFishingLure;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->TriggerCallback = DoTrig_FishingLure;

	newObj->CType 		= CTYPE_TRIGGER | CTYPE_BLOCKSHADOW | CTYPE_MPLATFORM;
	newObj->CBits		= CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj, 40,-40,-40,40,40,-40);

	return(true);
}


/******************** MOVE FISHING LURE ***********************/

static void MoveFishingLure(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	oldY;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


	GetObjectInfo(theNode);


				/* WOBBLE */

	if (gCoord.y < theNode->InitCoord.y)
	{
		gDelta.y += fps * 1200.0f;
	}
	else
	if (gCoord.y > theNode->InitCoord.y)
	{
		gDelta.y -= fps * 1200.0f;
	}


	theNode->LureMaxWobbleDY -= fps * 40.0f;
	if (theNode->LureMaxWobbleDY < 40.0f)
		theNode->LureMaxWobbleDY = 0.0f;

	if (gDelta.y > theNode->LureMaxWobbleDY)
		gDelta.y = theNode->LureMaxWobbleDY;

	oldY = gCoord.y;
	gCoord.y += gDelta.y * fps;


			/* RESET DY ON EACH BOB */

	if ((gCoord.y > theNode->InitCoord.y) && (oldY < theNode->InitCoord.y))
	{
		gDelta.y = theNode->LureMaxWobbleDY;
	}


			/****************/
			/* MAKE RIPPLES */
			/****************/

	if (fabs(gDelta.y) > 30.0f)
	{
		theNode->Timer -= fps;
		if (theNode->Timer <= 0.0f)
		{
			theNode->Timer = .2f + RandomFloat() * .3f;
			CreateNewRipple(gCoord.x + RandomFloat2() * 10.0f, gCoord.z + RandomFloat2() * 10.0f, 25.0f, 60.0f, .3);
		}
	}

	UpdateObject(theNode);
}


/************** DO TRIGGER - FISHING LURE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_FishingLure(ObjNode *lure, ObjNode *who, Byte sideBits)
{
	if (sideBits & SIDE_BITS_BOTTOM)			// only if player's bottom hit
	{
		if (who->CurrentTriggerObj != lure)		// only if we're not already on this
		{
			lure->Delta.y = -240.0f;
			lure->LureMaxWobbleDY = -lure->Delta.y;

			gDelta.x = gDelta.z = 0;
		}
	}

	return(true);
}


#pragma mark -


/********************* ADD PICNIC BASKET *************************/

Boolean AddPicnicBasket(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PARK_ObjType_PicnicBasket;
	gNewObjectDefinition.scale 		= 3.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/4);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->TriggerCallback = DoTrig_PicnicBasket;

	newObj->CType 		= CTYPE_MISC | CTYPE_TRIGGER | CTYPE_BLOCKSHADOW | CTYPE_BLOCKCAMERA;
	newObj->CBits		= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,.61);

	return(true);
}


/************** DO TRIGGER - PICNIC BASKET ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_PicnicBasket(ObjNode *basket, ObjNode *who, Byte sideBits)
{
	(void) basket;
	(void) sideBits;

	if (gNumFoodOnBasket >= FOOD_TO_GET)			// we only need n
		return(true);

	if (sideBits & SIDE_BITS_BOTTOM)				// only if object's bottom hit
	{
			/********************************/
			/* SEE IF FOOD LANDED ON BASKET */
			/********************************/

		if (who->Kind == PICKUP_KIND_FOOD)
		{
			gDelta.x = gDelta.z = 0;				// make sure doesn't slide off
			who->CType = CTYPE_PLAYERONLY;			// nothing happens to them now
			who->MoveCall = nil;					// cant get knocked off

			gFoodTypes[gNumFoodOnBasket] = who->Type - PARK_ObjType_CheeseBit;	// remember what we got
			gNumFoodOnBasket++;						// inc # of food items on the basket this loop
		}
	}

	return(true);
}







