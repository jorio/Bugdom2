/****************************/
/*   		ITEMS.C		    */
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


static void MoveWavingFlower(ObjNode *theNode);
static void MoveDoor(ObjNode *door);
static Boolean DoTrig_Door(ObjNode *door, ObjNode *who, Byte sideBits);
static void MovePoolLeaf(ObjNode *theNode);
static Boolean DoTrig_PoolLeaf(ObjNode *leaf, ObjNode *who, Byte sideBits);
static Boolean DoTrig_DogHouse(ObjNode *leaf, ObjNode *who, Byte sideBits);
static void MoveBeachBall(ObjNode *theNode);
static void MoveChlorineFloat(ObjNode *theNode);
static void MovePoolRingFloat(ObjNode *theNode);
static Boolean HurtGlassBottle(ObjNode *bottle, float damage);
static void MoveGlassBottle(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	LEAF_DEFAULT_WOBBLE_MAG		4.0f
#define	LEAF_DEFAULT_WOBBLE_SPEED	1.0f

/*********************/
/*    VARIABLES      */
/*********************/

#define	WaveXIndex	SpecialF[0]
#define	WaveZIndex	SpecialF[1]



#define	Wobble		SpecialF[0]
#define WobbleMag	SpecialF[1]
#define	WobbleSpeed	SpecialF[2]


/********************* INIT ITEMS MANAGER *************************/

void InitItemsManager(void)
{
	gHeadOnScarecrow = false;

	InitSprinklerHeads();

	CreateCyclorama();

	gNumBowlingPinsDown = 0;
	gNumPuzzlePiecesFit = 0;

	gResetRideBall = false;
	gPoppedSodaCan = false;
	gStartedSiliconDoor = false;

	CountAntHills();
	CountMice();
}


/************************* CREATE CYCLORAMA *********************************/

void CreateCyclorama(void)
{
ObjNode	*newObj;

	switch(gLevelNum)
	{
		case	LEVEL_NUM_FIDO:
		case	LEVEL_NUM_PLUMBING:
		case	LEVEL_NUM_CLOSET:
		case	LEVEL_NUM_BALSA:
				break;


		case	LEVEL_NUM_PLAYROOM:
				gNewObjectDefinition.group	= MODEL_GROUP_LEVELSPECIFIC;
				gNewObjectDefinition.type 	= 0;									// cyc is always 1st model in level bg3d files
				gNewObjectDefinition.coord.x = 0;
				gNewObjectDefinition.coord.y = 0;
				gNewObjectDefinition.coord.z = 0;
				gNewObjectDefinition.flags 	= STATUS_BIT_DONTCULL|STATUS_BIT_NOFOG;	// playroom cyc has lighting
				gNewObjectDefinition.slot 	= TERRAIN_SLOT+1;						// draw after terrain for better performance since terrain blocks much of the pixels
				gNewObjectDefinition.moveCall = nil;
				gNewObjectDefinition.rot 	= 0;
				gNewObjectDefinition.scale 	= gGameView->yon * .995f / 100.0f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->CustomDrawFunction = DrawCyclorama;
				break;


		case	LEVEL_NUM_GUTTER:
				gNewObjectDefinition.group	= MODEL_GROUP_LEVELSPECIFIC;
				gNewObjectDefinition.type 	= GUTTER_ObjType_Cyc;
				goto regular;
				break;


		case	LEVEL_NUM_PARK:
				gNewObjectDefinition.group	= MODEL_GROUP_LEVELSPECIFIC;
				gNewObjectDefinition.type 	= PARK_ObjType_Cyclorama;
				gNewObjectDefinition.coord.x = 0;
				gNewObjectDefinition.coord.y = 0;
				gNewObjectDefinition.coord.z = 0;
				gNewObjectDefinition.flags 	= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING;	// park cyc has fog
				gNewObjectDefinition.slot 	= TERRAIN_SLOT+1;						// draw after terrain for better performance since terrain blocks much of the pixels
				gNewObjectDefinition.moveCall = nil;
				gNewObjectDefinition.rot 	= 0;
				gNewObjectDefinition.scale 	= gGameView->yon * .995f / 100.0f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->CustomDrawFunction = DrawCyclorama;
				break;


		default:
				gNewObjectDefinition.group	= MODEL_GROUP_LEVELSPECIFIC;
				gNewObjectDefinition.type 	= 0;								// cyc is always 1st model in level bg3d files
regular:
				gNewObjectDefinition.coord.x = 0;
				gNewObjectDefinition.coord.y = 0;
				gNewObjectDefinition.coord.z = 0;
				gNewObjectDefinition.flags 	= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG;
				gNewObjectDefinition.slot 	= TERRAIN_SLOT+1;					// draw after terrain for better performance since terrain blocks much of the pixels
				gNewObjectDefinition.moveCall = nil;
				gNewObjectDefinition.rot 	= 0;
				gNewObjectDefinition.scale 	= gGameView->yon * .995f / 100.0f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->CustomDrawFunction = DrawCyclorama;
	}
}


/********************** DRAW CYCLORAMA *************************/

void DrawCyclorama(ObjNode *theNode)
{
OGLPoint3D cameraCoord = gGameView->cameraPlacement.cameraLocation;

		/* UPDATE CYCLORAMA COORD INFO */

	theNode->Coord.x = cameraCoord.x;
	theNode->Coord.y = cameraCoord.y + theNode->TargetOff.y;
	theNode->Coord.z = cameraCoord.z;
	UpdateObjectTransforms(theNode);



			/* DRAW THE OBJECT */

	MO_DrawObject(theNode->BaseGroup);
}


#pragma mark -

/************************* ADD DAISY *********************************/

Boolean AddDaisy(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_FOLIAGE;
	gNewObjectDefinition.type 		= FOLIAGE_ObjType_Daisy1 + itemPtr->parm[0];
	gNewObjectDefinition.scale 		= 1.0f + RandomFloat() * 2.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 190;
	gNewObjectDefinition.moveCall 	= MoveWavingFlower;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->WaveXIndex = x * .003f;
	newObj->WaveZIndex = z * .003f;

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_NOTTOP;
	SetObjectCollisionBounds(newObj, 500,0,-20,20,20,-20);

	return(true);													// item was added
}


/********************* MOVE WAVING FLOWER **********************/

static void MoveWavingFlower(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


	theNode->WaveXIndex += gFramesPerSecondFrac * 1.5f;
	theNode->WaveZIndex += gFramesPerSecondFrac * 1.5f;

	theNode->Rot.x = sin(theNode->WaveXIndex) * .04f;
	theNode->Rot.z = sin(theNode->WaveZIndex) * .04f;

	UpdateObjectTransforms(theNode);
}


/************************* ADD TULIP *********************************/

Boolean AddTulip(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_FOLIAGE;
	gNewObjectDefinition.type 		= FOLIAGE_ObjType_Tulip1 + itemPtr->parm[0];
	gNewObjectDefinition.scale 		= 2.5f + RandomFloat() * 1.5f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 2.0);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 45;
	gNewObjectDefinition.moveCall 	= MoveWavingFlower;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->WaveXIndex = x * .003f;
	newObj->WaveZIndex = z * .003f;

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_NOTTOP;
	SetObjectCollisionBounds(newObj, 500,0,-20,20,20,-20);

	return(true);													// item was added
}

/************************* ADD ROSE *********************************/

Boolean AddRose(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_FOLIAGE;
	gNewObjectDefinition.type 		= FOLIAGE_ObjType_Rose;
	gNewObjectDefinition.scale 		= 2.5f + RandomFloat() * 1.5f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 2.0);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 50;
	gNewObjectDefinition.moveCall 	= MoveWavingFlower;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->WaveXIndex = x * .003f;
	newObj->WaveZIndex = z * .003f;

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_NOTTOP;
	SetObjectCollisionBounds(newObj, 500,0,-20,20,20,-20);

	return(true);													// item was added
}


/************************* ADD GRASS *********************************/

Boolean AddGrass(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_FOLIAGE;
	gNewObjectDefinition.type 		= FOLIAGE_ObjType_Grass1 + itemPtr->parm[0];
	gNewObjectDefinition.scale 		= 1.7f + RandomFloat() * 1.8f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 80;
	gNewObjectDefinition.moveCall 	= MoveWavingFlower;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->WaveXIndex = x * .003f;
	newObj->WaveZIndex = z * .003f;

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_NOTTOP;
	SetObjectCollisionBounds(newObj, 200,0,-20,20,20,-20);

	return(true);													// item was added
}

/************************* ADD SHRUB ROOT *********************************/

Boolean AddShrubRoot(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_FOLIAGE;
	gNewObjectDefinition.type 		= FOLIAGE_ObjType_ShrubRoot;
	gNewObjectDefinition.scale 		= 1.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 45;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj, 500,0,-20,20,20,-20);

	return(true);													// item was added
}




#pragma mark -

/************************* ADD DOOR *********************************/

Boolean AddDoor(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*door;
int		isOpen = itemPtr->flags & ITEM_FLAGS_USER1;
int		doorColor = itemPtr->parm[1];


				/* SET MODEL TYPE */

	gNewObjectDefinition.scale 		= 1.8f;

	switch(gLevelNum)
	{
		case	LEVEL_NUM_GNOMEGARDEN:
				gNewObjectDefinition.type 		= GARDEN_ObjType_RedDoor + doorColor;
				break;

		case	LEVEL_NUM_SIDEWALK:
				gNewObjectDefinition.type 		= SIDEWALK_ObjType_RedDoor + doorColor;
				break;

		case	LEVEL_NUM_PLAYROOM:
				gNewObjectDefinition.type 		= PLAYROOM_ObjType_RedDoor + doorColor;
				break;

		case	LEVEL_NUM_CLOSET:
				if (doorColor == 0)										// special case for silicon door
					return (AddSiliconDoor(itemPtr, x, z));
				else
				{
					gNewObjectDefinition.type 		= CLOSET_ObjType_DiaryDoor;
					gNewObjectDefinition.scale 		= 6.0f;
					doorColor = 0;
				}
				break;

		case	LEVEL_NUM_PARK:
				gNewObjectDefinition.type 		= PARK_ObjType_RedDoor + doorColor;
				break;

		case	LEVEL_NUM_GARBAGE:
				gNewObjectDefinition.type 		= GARBAGE_ObjType_RedDoor + doorColor;
				break;


 		default:
				return(false);

	}

				/* MAKE MODEL */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + 10.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB - 30;		// put toward end so collision has final say
	gNewObjectDefinition.moveCall 	= MoveDoor;
	if (isOpen)
		gNewObjectDefinition.rot 	= itemPtr->parm[0] * PI/2 + PI/2;
	else
		gNewObjectDefinition.rot 	= itemPtr->parm[0] * PI/2;
	door = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	door->TerrainItemPtr = itemPtr;								// keep ptr to item list

	door->Kind = doorColor;

	if (isOpen)
	{
		door->Mode 	= DOOR_MODE_OPEN;
		door->CType = CTYPE_MISC|CTYPE_BLOCKCAMERA;
	}
	else
	{
		door->Mode 	= DOOR_MODE_CLOSED;
		door->CType = CTYPE_MISC|CTYPE_TRIGGER|CTYPE_BLOCKCAMERA;
	}

			/* SET COLLISION STUFF */

	door->CBits			= CBITS_ALLSOLID|CBITS_IMPENETRABLE;
	CreateCollisionBoxFromBoundingBox_Rotated(door,1,1);

	door->TriggerCallback = DoTrig_Door;

	return(true);
}


/******************** MOVE DOOR ******************/

static void MoveDoor(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	switch(theNode->Mode)
	{
		case	DOOR_MODE_OPENING:
				theNode->Rot.y += gFramesPerSecondFrac * PI;
				theNode->DoorOpenRot += gFramesPerSecondFrac * PI;

				if (theNode->DoorOpenRot >= (PI/2))			// see if opened all the way now
				{
					theNode->Mode = DOOR_MODE_OPEN;
					theNode->CType = CTYPE_MISC;
					CreateCollisionBoxFromBoundingBox_Rotated(theNode,1,1);
				}

				UpdateObjectTransforms(theNode);
				break;
	}
}


/************** DO TRIGGER - DOOR ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Door(ObjNode *door, ObjNode *who, Byte sideBits)
{
int	keyNum = door->Kind;							// get door color

#pragma unused (who, sideBits)

	if (gPlayerInfo.hasKey[keyNum])					// see if player has this key
	{
		door->Mode = DOOR_MODE_OPENING;
		door->CType = 0;							// not solid while opening
		door->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;	// set user flag so we'll always know its opened already

		gPlayerInfo.hasKey[keyNum] = false;			// used up this key

		PlayEffect3D(EFFECT_DOORCREAK, &door->Coord);
	}

	return(true);
}


#pragma mark -

/************************* ADD BRICK *********************************/

Boolean AddBrick(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	switch(gLevelNum)
	{
		case	LEVEL_NUM_GNOMEGARDEN:
				gNewObjectDefinition.type 		= GARDEN_ObjType_Brick;
				break;

		case	LEVEL_NUM_SIDEWALK:
				gNewObjectDefinition.type 		= SIDEWALK_ObjType_Brick;
				break;

		default:
				return(true);
	}

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.scale 		= 2.5f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-10;				// put toward end so collision has final say
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * PI/2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_BLOCKSHADOW;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);

	return(true);													// item was added
}




/************************* ADD POST *********************************/

Boolean AddPost(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int	type = itemPtr->parm[0];

	gNewObjectDefinition.scale 		= 1.5f;

	switch(gLevelNum)
	{
		case	LEVEL_NUM_GNOMEGARDEN:
				gNewObjectDefinition.type 		= GARDEN_ObjType_Post_Brick + type;
				break;

		case	LEVEL_NUM_SIDEWALK:
				gNewObjectDefinition.type 		= SIDEWALK_ObjType_Post_Brick + type;
				break;

		case	LEVEL_NUM_PLAYROOM:
				gNewObjectDefinition.type 		= PLAYROOM_ObjType_BlockPost + type;
				if (type == 0)
					gNewObjectDefinition.scale 		= 2.0f;
				else
					gNewObjectDefinition.scale 		= 1.35f;
				break;

		case	LEVEL_NUM_GARBAGE:
				gNewObjectDefinition.type 		= GARBAGE_ObjType_TPpost;
				gNewObjectDefinition.scale 		= 2.0f;
				break;

		case	LEVEL_NUM_PARK:
				gNewObjectDefinition.type 		= PARK_ObjType_GrassPost;
				break;

		default:
				return(true);
	}

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-10;				// put toward end so collision against this has final say
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW | CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID|CBITS_IMPENETRABLE;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);

	return(true);													// item was added
}



/************************* ADD PEBBLE *********************************/

Boolean AddPebble(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	switch(gLevelNum)
	{
		case	LEVEL_NUM_GNOMEGARDEN:
				gNewObjectDefinition.type 		= GARDEN_ObjType_LargeStone + itemPtr->parm[0];
				break;

		case	LEVEL_NUM_SIDEWALK:
				gNewObjectDefinition.type 		= SIDEWALK_ObjType_LargeStone + itemPtr->parm[0];
				break;

		default:
				return(true);
	}


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.scale 		= 1.0f + RandomFloat()*.1f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 419;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);

	return(true);													// item was added
}




/************************* ADD POOL COPING *********************************/

Boolean AddPoolCoping(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
Boolean	isCorner = itemPtr->parm[3] & 1;							// see if it's a corner piece

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	if (isCorner)
		gNewObjectDefinition.type 	= SIDEWALK_ObjType_CornerCoping;
	else
		gNewObjectDefinition.type 	= SIDEWALK_ObjType_Coping;
	gNewObjectDefinition.scale 		= gTerrainPolygonSize * 3.0f;						// make it fit exactly 3 tiles
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= 460.0f;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-10;				// put toward end so collision has final say
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/4.0f);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_BLOCKCAMERA;

	if (isCorner)
	{
		switch(itemPtr->parm[0])
		{
			case	0:
					newObj->CBits			= CBITS_TOP | CBITS_LEFT | CBITS_FRONT;
					break;

			case	1:
					newObj->CBits			= CBITS_TOP | CBITS_FRONT | CBITS_RIGHT;
					break;

			case	2:
					newObj->CBits			= CBITS_TOP | CBITS_BACK | CBITS_RIGHT;
					break;

			case	3:
					newObj->CBits			= CBITS_TOP | CBITS_BACK | CBITS_LEFT;
					break;
		}
	}
	else
	{
		switch(itemPtr->parm[0])
		{
			case	0:
			case	2:
					newObj->CBits			= CBITS_TOP | CBITS_FRONT | CBITS_BACK;
					break;

			case	1:
			case	3:
					newObj->CBits			= CBITS_TOP | CBITS_LEFT | CBITS_RIGHT;
					break;
		}
	}

	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);

	return(true);													// item was added
}

#pragma mark -

/************************* ADD POOL LEAF *********************************/

Boolean AddPoolLeaf(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

					/*************/
					/* MAKE LEAF */
					/*************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SIDEWALK_ObjType_PoolLeaf1 + (MyRandomLong() & 0x3);
	gNewObjectDefinition.scale 		= 2.0f + RandomFloat()*.4f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	GetWaterY(x,z, &gNewObjectDefinition.coord.y);
	gNewObjectDefinition.coord.y += 4.0f;
	gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= POW_SLOT-1;					// make sure before POW so that key will chain correctly
	gNewObjectDefinition.moveCall 	= MovePoolLeaf;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Wobble = RandomFloat()*PI2;
	newObj->WobbleMag = LEAF_DEFAULT_WOBBLE_MAG;
	newObj->WobbleSpeed = LEAF_DEFAULT_WOBBLE_SPEED;

	newObj->Timer = 0;

			/* SET COLLISION STUFF */

	newObj->TriggerCallback = DoTrig_PoolLeaf;
	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_MPLATFORM|CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);
	newObj->BottomOff		= -200;									// give leaves some depth
	CalcObjectBoxFromNode(newObj);
	KeepOldCollisionBoxes(newObj);


				/*******************/
				/* PUT KEY ON LEAF */
				/*******************/

	if (itemPtr->parm[3] & 1)								// see if has key
	{
		ObjNode		*key;
		OGLPoint3D	p;

		p.x = newObj->Coord.x;								// set coord of key
		p.y = newObj->Coord.y + 30.0f;
		p.z = newObj->Coord.z;

		key = MakePOW(POW_KIND_REDKEY, &p);					// make key
		key->MoveCall = nil;								// dont move on it's own
		key->CType |= CTYPE_TRIGGER;						// make the trigger active NOW

		newObj->ChainNode = key;							// chain them
		key->ChainHead = newObj;

		key->StatusBits |= STATUS_BIT_DONTPURGE;			// we need to keep these around
		newObj->StatusBits |= STATUS_BIT_DONTPURGE;
	}



	return(true);													// item was added
}


/******************* MOVE POOL LEAF ************************/

static void MovePoolLeaf(ObjNode *theNode)
{
float	fps;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);


			/* DO WOBBLE */

	theNode->Wobble += fps * theNode->WobbleSpeed;									// make it wobble

	gCoord.y = theNode->InitCoord.y + sin(theNode->Wobble) * theNode->WobbleMag;	// calc new y

	theNode->WobbleMag -= fps * 2.0f;												// decay to normal defaults
	if (theNode->WobbleMag < LEAF_DEFAULT_WOBBLE_MAG)
		theNode->WobbleMag = LEAF_DEFAULT_WOBBLE_MAG;
	theNode->WobbleSpeed -= fps * 2.0f;
	if (theNode->WobbleSpeed < LEAF_DEFAULT_WOBBLE_SPEED)
		theNode->WobbleSpeed = LEAF_DEFAULT_WOBBLE_SPEED;


	UpdateObject(theNode);

			/* MAKE RIPPLES */

	if (!(theNode->StatusBits & STATUS_BIT_ISCULLED))			// no ripples if culled
	{
		theNode->Timer -= fps;
		if (theNode->Timer <= 0.0f)
		{
			theNode->Timer = .2f + RandomFloat() * .3f;
			CreateNewRipple(gCoord.x + RandomFloat2() * 100.0f, gCoord.z + RandomFloat2() * 100.0f, RandomFloat() * 50.0f, 100.0f, .3);
		}
	}
}


/************** DO TRIGGER - POOL LEAF ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_PoolLeaf(ObjNode *leaf, ObjNode *who, Byte sideBits)
{
	if (sideBits & SIDE_BITS_BOTTOM)					// see if on top of leaf
	{
		who->Rot.y += leaf->DeltaRot.y * gFramesPerSecondFrac;

		if (who->Delta.y < -150.0f)					// see if landed hard
		{
			leaf->WobbleMag = 10.0f;				// make leaf do big wobble
			leaf->WobbleSpeed = 9.0f;
			leaf->Wobble = PI;
		}
	}

	return(true);
}


#pragma mark -



/************************* ADD DOG HOUSE *********************************/

Boolean AddDogHouse(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SIDEWALK_ObjType_DogHouse;
	gNewObjectDefinition.scale 		= 2.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 302;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= PI + ((float)itemPtr->parm[0] * (PI2/4));
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->TriggerCallback = DoTrig_DogHouse;
	newObj->CType 			= CTYPE_MISC|CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, .7, 1);

	return(true);													// item was added
}



/************** DO TRIGGER - DOG HOUSE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_DogHouse(ObjNode *house, ObjNode *who, Byte sideBits)
{
#pragma unused (house, who, sideBits)

	if (sideBits & SIDE_BITS_FRONT)
		StartLevelCompletion(0);

	return(true);
}




/************************* ADD TULIP POT *********************************/

Boolean AddTulipPot(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SIDEWALK_ObjType_TulipPot;
	gNewObjectDefinition.scale 		= 3.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 101;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/4);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, .9, 1);

	return(true);													// item was added
}



/************************* ADD BEACH BALL *********************************/

Boolean AddBeachBall(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	if (gLevelNum == LEVEL_NUM_SIDEWALK)
	{
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= SIDEWALK_ObjType_BeachBall;
		gNewObjectDefinition.scale 		= 2.5;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		GetWaterY(x,z, &gNewObjectDefinition.coord.y);
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot 		= 111;
		gNewObjectDefinition.moveCall 	= MoveBeachBall;
		gNewObjectDefinition.rot 		= RandomFloat()*PI2;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	}
	else
	{
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= PLAYROOM_ObjType_BeachBall;
		gNewObjectDefinition.scale 		= 2.5;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) - gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot 		= 111;
		gNewObjectDefinition.moveCall 	= MoveStaticObject;
		gNewObjectDefinition.rot 		= RandomFloat()*PI2;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	}

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, .8, 1);


				/* MAKE SHADOW */

	if (gLevelNum == LEVEL_NUM_PLAYROOM)
	{
		AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 25,25, false);
	}

	return(true);													// item was added
}

/********************* MOVE BEACH BALL **********************/

static void MoveBeachBall(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

			/* MOVE */

	theNode->Rot.y -= fps * .1f;							// slowly spin

	theNode->SpecialF[0] += fps * 10.0f;					// make wobble
	theNode->Coord.y = theNode->InitCoord.y + sin(theNode->SpecialF[0]) * 5.0f;



			/* MAKE RIPPLES */

	if (!(theNode->StatusBits & STATUS_BIT_ISCULLED))			// no ripples if culled
	{
		theNode->Timer -= fps;
		if (theNode->Timer <= 0.0f)
		{
			theNode->Timer = .3f + RandomFloat() * .3f;
			CreateNewRipple(theNode->Coord.x, theNode->Coord.z, 300.0f, 200.0f, .4);
		}
	}

	UpdateObjectTransforms(theNode);
}



/************************* ADD CHLORINE FLOAT *********************************/

Boolean AddChlorineFloat(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SIDEWALK_ObjType_ChlorineFloat;
	gNewObjectDefinition.scale 		= 3.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	GetWaterY(x,z, &gNewObjectDefinition.coord.y);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 111;
	gNewObjectDefinition.moveCall 	= MoveChlorineFloat;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, .9, 1);

	return(true);													// item was added
}


/********************* MOVE CHLORINE FLOAT **********************/

static void MoveChlorineFloat(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

			/* MOVE */

	theNode->Rot.y -= fps * .1f;							// slowly spin

	theNode->SpecialF[0] += fps * 8.0f;					// make wobble
	theNode->Coord.y = theNode->InitCoord.y + sin(theNode->SpecialF[0]) * 5.0f;


			/* MAKE RIPPLES */

	if (!(theNode->StatusBits & STATUS_BIT_ISCULLED))			// no ripples if culled
	{
		theNode->Timer -= fps;
		if (theNode->Timer <= 0.0f)
		{
			theNode->Timer = .2f + RandomFloat() * .3f;
			CreateNewRipple(theNode->Coord.x, theNode->Coord.z, 150.0f, 150.0f, .3);
		}
	}

	UpdateObjectTransforms(theNode);
}



/************************* ADD POOL RING FLOAT *********************************/

Boolean AddPoolRingFloat(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
float	s;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SIDEWALK_ObjType_PoolRingFloat;
	gNewObjectDefinition.scale 		= s = 3.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	GetWaterY(x,z, &gNewObjectDefinition.coord.y);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 111;
	gNewObjectDefinition.moveCall 	= MovePoolRingFloat;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;

	AddCollisionBoxToObject(newObj, 900, 0, -250.0f * s, 160.0f * s, -90.0f * s, -250.0f * s);		// far sponson
	AddCollisionBoxToObject(newObj, 900, 0, -250.0f * s, 160.0f * s, 250.0f * s, 90.0f * s);		// near sponson
	AddCollisionBoxToObject(newObj, 2000, 0, 30.0f * s, 180.0f * s, 90.0f * s, -90.0f * s);			// neck


	return(true);													// item was added
}


/********************* MOVE POOL RING FLOAT **********************/

static void MovePoolRingFloat(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

			/* MOVE */


	theNode->SpecialF[0] += fps * 5.0f;					// make wobble
	theNode->Coord.y = theNode->InitCoord.y + sin(theNode->SpecialF[0]) * 5.0f;



	UpdateObjectTransforms(theNode);
}


#pragma mark -

/************************* ADD DRAIN PIPE *********************************/

Boolean AddDrainPipe(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj, *grate;

				/************/
				/* ADD PIPE */
				/************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SIDEWALK_ObjType_DrainPipe;
	gNewObjectDefinition.scale 		= 2.5;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 484;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj, 1, 1);

				/**************/
				/* MAKE GRATE */
				/**************/

	gNewObjectDefinition.type 		= SIDEWALK_ObjType_Grate;
	gNewObjectDefinition.coord.x 	+= 79.0f * gNewObjectDefinition.scale;
	gNewObjectDefinition.coord.y 	+= 110.0f * gNewObjectDefinition.scale;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	grate = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			/* SET COLLISION STUFF */

	grate->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_BLOCKCAMERA;
	grate->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(grate, 1, 1);


	newObj->ChainNode = grate;

	return(true);													// item was added
}

#pragma mark -

/************************* ADD GLASS BOTTLE *********************************/

Boolean AddGlassBottle(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	if (gLevelNum == LEVEL_NUM_SIDEWALK)
		gNewObjectDefinition.type 		= SIDEWALK_ObjType_Bottle;
	else
		gNewObjectDefinition.type 		= PARK_ObjType_Bottle;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.scale 		= 2.1;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-1;
	gNewObjectDefinition.moveCall 	= MoveGlassBottle;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_BUDDYATTRACT;
	newObj->CBits			= CBITS_ALLSOLID|CBITS_IMPENETRABLE;
	CreateCollisionBoxFromBoundingBox(newObj, 1, 1);

	newObj->HurtCallback = HurtGlassBottle;

	newObj->Mode = 0;							// crack increment

	return(true);													// item was added
}


/********************* MOVE GLASS BOTTLE **********************/

static void MoveGlassBottle(ObjNode *theNode)
{
	if (theNode->Mode == 0)									// only let delete if not cracked
	{
		if (TrackTerrainItem(theNode))							// just check to see if it's gone
		{
			DeleteObject(theNode);
			return;
		}
	}
}


/*********************** HURT GLASS BOTTLE ***************************/
//
// Only called when buddy bug hits it
//

static Boolean HurtGlassBottle(ObjNode *bottle, float damage)
{
#pragma unused (damage)

			/* SEE IF BLOW IT UP */

	bottle->Mode++;
	if (bottle->Mode >= 5)
	{
		PlayEffect3D(EFFECT_BOTTLESHATTER, &bottle->Coord);
		ExplodeGeometry(bottle, 300, SHARD_MODE_FROMORIGIN | SHARD_MODE_BOUNCE, 1, .5);
		bottle->TerrainItemPtr = nil;							// make sure bottle doesn't come back
		DeleteObject(bottle);
		return(true);
	}

			/* UPDATE CRACK */

	bottle->Type++;
	ResetDisplayGroupObject(bottle);
	PlayEffect3D(EFFECT_BOTTLECRACK, &bottle->Coord);
	return(false);
}



























