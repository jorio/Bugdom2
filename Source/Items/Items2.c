/****************************/
/*    	ITEMS2.C		    */
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

static Boolean DoTrig_LetterBlock(ObjNode *leaf, ObjNode *who, Byte sideBits);
static void MoveLetterBlock(ObjNode *block);
static void MoveBumbleBeeOnSpline(ObjNode *theNode);
static void MoveFlashlight(ObjNode *theNode);
static void MoveSiliconDoor(ObjNode *theNode);
static Boolean DoTrig_SiliconDoor(ObjNode *door, ObjNode *who, Byte sideBits);
static void AttachPartToDoor(ObjNode *part, ObjNode *door);
static void MoveLilyPad(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	BUMBLEBEE_SCALE	1.3f

#define	BOOK_SCALE		6.0f

/*********************/
/*    VARIABLES      */
/*********************/

#define	BumbleBeeFlying	Flag[0]

#define	Wobble	SpecialF[0]

Boolean		gStartedSiliconDoor = false;


/************************* ADD LETTER BLOCK *********************************/

Boolean AddLetterBlock(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PLAYROOM_ObjType_LetterBlock1 + itemPtr->parm[0];
	gNewObjectDefinition.scale 		= 1.3;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= PLAYER_SLOT - 4;
	gNewObjectDefinition.moveCall 	= MoveLetterBlock;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->TriggerCallback = DoTrig_LetterBlock;
	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_BLOCKCAMERA|CTYPE_TRIGGER|CTYPE_MPLATFORM;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);

			/* RANDOM ROT */

	newObj->Rot.x = (float)(MyRandomLong() & 0x3) * PI/2;
	newObj->Rot.y = (float)(MyRandomLong() & 0x3) * PI/2;
	newObj->Rot.z = (float)(MyRandomLong() & 0x3) * PI/2;
	UpdateObjectTransforms(newObj);

	return(true);													// item was added
}


/********************* MOVE LETTER BLOCK **********************/

static void MoveLetterBlock(ObjNode *block)
{
float	fps = gFramesPerSecondFrac;

	if (TrackTerrainItem(block))							// just check to see if it's gone
	{
		DeleteObject(block);
		return;
	}

	GetObjectInfo(block);

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;

//	ApplyFrictionToDeltas(3000, &gDelta);

	gDelta.x = gDelta.z = 0;

	HandleCollisions(block, CTYPE_TERRAIN | CTYPE_FENCE | CTYPE_MISC | CTYPE_PLAYER, 0);

	UpdateObject(block);
}


/************** DO TRIGGER - LETTER BLOCK ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_LetterBlock(ObjNode *block, ObjNode *who, Byte sideBits)
{
float	r;
OGLVector2D	v1,v2;

	if (sideBits & SIDE_BITS_BOTTOM)					// see if on top of leaf - just bail since can't push from here
		return(true);

	if (gPlayerInfo.heldObject)							// if player holding something then can't push
		return(true);


				/* SEE IF PLAYER IS AIMED AT THE BLOCK */

	r = who->Rot.y;										// calc player's aim vector
	v1.x = -sin(r);
	v1.y = -cos(r);

	v2.x = block->Coord.x - gCoord.x;					// calc player->block vector
	v2.y = block->Coord.z - gCoord.z;
	FastNormalizeVector2D(v2.x, v2.y, &v2, false);

	r = acos(OGLVector2D_Dot(&v1, &v2));				// calc angle between vectors
	if (r > (PI/2))										// if not aiming enough @ block then bail
		return(true);


				/* ATTACH TO A SIDE */

	switch(sideBits)
	{
		case	SIDE_BITS_LEFT:							// hit right side of block
				who->Rot.y = PI/2;
				break;

		case	SIDE_BITS_RIGHT:						// hit left side of block
				who->Rot.y = -PI/2;
				break;

		case	SIDE_BITS_FRONT:						// hit back side of block
				who->Rot.y = PI;
				break;

		case	SIDE_BITS_BACK:							// hit front side of block
				who->Rot.y = 0;
				break;
	}


				/******************/
				/* PUSH THE BLOCK */
				/******************/

	r = who->Rot.y;
	gDelta.x = -sin(r) * who->Speed2D;
	gDelta.z = -cos(r) * who->Speed2D;

	block->Delta.x = gDelta.x * .4f;
	block->Delta.z = gDelta.z * .4f;

	PlayerStartPushingObject(who, block);

	gSolidTriggerKeepDelta = true;						// set this so the player won't be stopped, but it'll treat it as a solid hit
	return(true);
}


#pragma mark -

/************************ PRIME BUMBLE BEE *************************/

Boolean PrimeBumbleBee(int splineNum, SplineItemType *itemPtr)
{
ObjNode		*bee,*bag;
float		x,z,placement;


			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&gSplineList[splineNum], placement, &x, &z);

				/*******************/
				/* MAKE BEE OBJECT */
				/*******************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_BUMBLEBEE;
	gNewObjectDefinition.animNum 	= 1;
	gNewObjectDefinition.scale 		= BUMBLEBEE_SCALE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + 50.0f;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP|STATUS_BIT_ONSPLINE;
	gNewObjectDefinition.slot 		= PLAYER_SLOT-1;			// must move before player for title screen hobo bag chain
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;

	bee = MakeNewSkeletonObject(&gNewObjectDefinition);


				/* SET BETTER INFO */

	bee->SplineItemPtr 		= itemPtr;
	bee->SplineNum 			= splineNum;
	bee->SplinePlacement 	= placement;

	bee->BumbleBeeFlying 	= false;

	AttachShadowToObject(bee, SHADOW_TYPE_CIRCULAR, 5,6, true);

	SetSplineAim(bee);												// set initial aim
	UpdateObjectTransforms(bee);


				/*****************/
				/* MAKE HOBO BAG */
				/*****************/

	if (gLevelNum != -1)												// not if on title screen
	{
		bee->SplineMoveCall 	= MoveBumbleBeeOnSpline;

		gNewObjectDefinition.type 		= SKELETON_TYPE_HOBOBAG;
		gNewObjectDefinition.animNum 	= 1;
		gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot		= SLOT_OF_DUMB;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= BUMBLEBEE_SCALE;
		bag = MakeNewSkeletonObject(&gNewObjectDefinition);

		bee->ChainNode = bag;
	}
	else
		bee->SplineMoveCall 	= MoveBumbleBeeOnSpline_Title;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(bee, true);										// detach this object from the linked list
	AddToSplineObjectList(bee, true);

	return(true);
}


/******************** MOVE BUMBLEBEE ON SPLINE ***************************/

static void MoveBumbleBeeOnSpline(ObjNode *theNode)
{
Boolean isInRange;
ObjNode	*bag = theNode->ChainNode;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility

				/***************/
				/* FLYING AWAY */
				/***************/

	if (theNode->BumbleBeeFlying)
	{
		float	fps = gFramesPerSecondFrac;

				/* ACCELERATE */

		theNode->Speed2D += fps * 300.0f;							// accelerate
		if (theNode->Speed2D > 400.0f)
		{
			theNode->Speed2D = 400.0f;

		}

			/* MOVE ALONG THE SPLINE */

		if (IncreaseSplineIndex(theNode, theNode->Speed2D))			// delete when @ end of spline
		{
			DeleteObject(theNode);
			return;
		}

		GetObjectCoordOnSpline(theNode);

		SetSplineAim(theNode);										// update aim

		theNode->Coord.y += fps * theNode->Speed2D * .4f;			// rise

					/* UPDATE EFFECT */

		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_BUMBLERUMBLE, &theNode->Coord, NORMAL_CHANNEL_RATE, 1.0);
		else
			Update3DSoundChannel(EFFECT_BUMBLERUMBLE, &theNode->EffectChannel, &theNode->Coord);

		goto update;
	}


			/**********************************************/
			/* IF PLAYER IS CLOSE THEN START THE FLY-AWAY */
			/**********************************************/

	else
	if (isInRange)
	{
		if (!theNode->BumbleBeeFlying)
		{
			if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 500.0f)
			{
				theNode->BumbleBeeFlying = true;
				MorphToSkeletonAnim(theNode->Skeleton, 0, 7);
				MorphToSkeletonAnim(bag->Skeleton, 2, 4);
			}
		}

				/* UPDATE */

update:
		UpdateObjectTransforms(theNode);							// update transforms
		UpdateShadow(theNode);

				/* UPDATE HOBO BAG */

		if (bag)
		{
			OGLMatrix4x4	m,rm;
			FindJointFullMatrix(theNode, BUMBLEBEE_JOINTNUM_HAND, &m);
			OGLMatrix4x4_SetRotate_X(&rm, 0.5f);							// rotate to position
			rm.value[M03] = 0;												// also offset to align to hand
			rm.value[M13] = -5;
			rm.value[M23] = 0;
			OGLMatrix4x4_Multiply(&rm, &m, &bag->BaseTransformMatrix);		// calc bag's matrix
			SetObjectTransformMatrix(bag);

			bag->Coord.x = bag->BaseTransformMatrix.value[M03];					// extract coords of bag
			bag->Coord.y = bag->BaseTransformMatrix.value[M13];
			bag->Coord.z = bag->BaseTransformMatrix.value[M23];
		}
	}
}





#pragma mark -


/************************* ADD LEGO WALL *********************************/

Boolean AddLegoWall(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int		r = itemPtr->parm[1];
int		type = itemPtr->parm[0];

	if (type > 5)			// validate
		return(true);

				/*************/
				/* LEGO WALL */
				/*************/

	if (type == 0)
	{
		gNewObjectDefinition.type 		= PLAYROOM_ObjType_LegoWall;
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.scale 		= .85;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot 		= 100;
		gNewObjectDefinition.moveCall 	= MoveStaticObject;
		gNewObjectDefinition.rot 		= (float)r * (PI/2);
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

				/* SET COLLISION STUFF */

		newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_BLOCKCAMERA|CTYPE_BLOCKRAYS;
		newObj->CBits			= CBITS_ALLSOLID | CBITS_IMPENETRABLE;
		CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);
	}

				/**************/
				/* LEGO BRICK */
				/**************/

	else
	{
		if (itemPtr->parm[3] & 1)							// see if choose random color brick
			gNewObjectDefinition.type 		= PLAYROOM_ObjType_LegoBrick_Red + RandomRange(0, 4);
		else
			gNewObjectDefinition.type 		= PLAYROOM_ObjType_LegoWall + type;

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.scale 		= .7;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
		gNewObjectDefinition.slot 		= 101 + type;
		gNewObjectDefinition.moveCall 	= MoveStaticObject2;
		if (r == 4)
			gNewObjectDefinition.rot 	= RandomFloat() * PI2;
		else
			gNewObjectDefinition.rot 	= (float)r * (PI/2);
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


				/* SET COLLISION STUFF */

		newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_BLOCKCAMERA;
		newObj->CBits			= CBITS_ALLSOLID;
		CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);
	}

	return(true);													// item was added
}


#pragma mark -

/************************* ADD FLASHLIGHT ******************************/

Boolean AddFlashLight(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj, *beam;
int		i;

				/*******************/
				/* MAKE FLASHLIGHT */
				/*******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOSET_ObjType_FlashLight;
	gNewObjectDefinition.scale 		= 1.85;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 222;
	gNewObjectDefinition.moveCall 	= MoveFlashlight;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_BLOCKSHADOW | CTYPE_MPLATFORM;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj, 1, 1);


			/* CREATE BULB SPARKLE */

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = 0;
		gSparkles[i].where.x = newObj->Coord.x;
		gSparkles[i].where.y = newObj->Coord.y + newObj->TopOff;
		gSparkles[i].where.z = newObj->Coord.z;

		gSparkles[i].aim.x = 0;
		gSparkles[i].aim.y = 1.0;
		gSparkles[i].aim.z = 0;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 270.0f;
		gSparkles[i].separation = 130.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark2;
	}


				/*******************/
				/* MAKE LIGHT CONE */
				/*******************/

	gNewObjectDefinition.type 		= CLOSET_ObjType_LightBeam;
	gNewObjectDefinition.flags 		= STATUS_BIT_GLOW |	STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB + 40;
	gNewObjectDefinition.moveCall 	= nil;
	beam = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	beam->ColorFilter.b = .8f;

	newObj->ChainNode = beam;

	return(true);													// item was added
}


/******************** MOVE FLASHLIGHT *******************************/

static void MoveFlashlight(ObjNode *theNode)
{

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

//	beam->TextureTransformU += gFramesPerSecondFrac * .2f;

}


#pragma mark -

/************************* ADD CRAYON *********************************/

Boolean AddCrayon(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
OGLColorRGBA	crayonColors[8] =
{
	{1,.2,.2,1},
	{.2,1,.2,1},
	{.2,.2,1,1},
	{.9,.9,0,1},
	{0,1,1,1},
	{.3,.3,.3,1},
	{1,.5,0,1},
	{.8,.1,.8,1},
};
static const OGLPoint3D	 baseOff = {0, 0, 134};
static const OGLVector3D toTip = {0,0,-1};
OGLVector3D				v;
OGLPoint3D				base;
OGLMatrix4x4			m;
float					w,w2,y,s;
int						numBoxes,i;
CollisionBoxType		*box;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PLAYROOM_ObjType_Crayon;
	gNewObjectDefinition.scale 		= 1.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 484;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->ColorFilter = crayonColors[MyRandomLong()&0x7];			// set color

	RotateOnTerrain(newObj, -newObj->BBox.min.y, nil);								// set transform matrix

	OGLMatrix4x4_SetRotate_Z(&m, RandomFloat() * PI2);			// random z spin
	OGLMatrix4x4_Multiply(&m, &newObj->BaseTransformMatrix, &newObj->BaseTransformMatrix);
	SetObjectTransformMatrix(newObj);


			/***********************/
			/* SET COLLISION STUFF */
			/***********************/

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW;
	newObj->CBits			= CBITS_ALLSOLID;


			/* CALC VECTOR OF AIM & BASE POINT */

	OGLVector3D_Transform(&toTip, &newObj->BaseTransformMatrix, &v);
	OGLPoint3D_Transform(&baseOff, &newObj->BaseTransformMatrix, &base);


			/* CALC SIZE INFO */

	s = newObj->Scale.x;
	w = newObj->BBox.max.x * s * .9f;								// get the cubic size of each collision box
	w2 = w * 2.0f;

	numBoxes = 8;													// calc # collision boxes needed
	if (numBoxes > MAX_COLLISION_BOXES)
		numBoxes = MAX_COLLISION_BOXES;

	newObj->NumCollisionBoxes = numBoxes;						// set # boxes
	box = &newObj->CollisionBoxes[0];							// point to boxes


			/* SET ALL BOXES */

	x = base.x;// + v.x * w;									// move up to center 1st box
	y = base.y;// + v.y * w;
	z = base.z;// + v.z * w;


	for (i = 0; i < numBoxes; i++)
	{
		box[i].top 		= y + w*2;
		box[i].bottom 	= 0;
		box[i].left 	= x - w;
		box[i].right 	= x + w;
		box[i].front 	= z + w;
		box[i].back 	= z - w;

		x += v.x * w2;
		y += v.y * w2;
		z += v.z * w2;

	}


		/* SHAODW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 2,12, false);


	return(true);													// item was added
}

#pragma mark -

/********************* ADD DCELL *************************/

Boolean AddDCell(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PLAYROOM_ObjType_DCell;
	gNewObjectDefinition.scale 		= 1.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x, z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 358;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 		= CTYPE_MISC | CTYPE_BLOCKCAMERA | CTYPE_BLOCKSHADOW;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,1,1);

	return(true);
}





#pragma mark -


/************************* ADD CARDBOARD BOX *********************************/

Boolean AddCardboardBox(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
float	y;
int		stackLevel = itemPtr->parm[1];
int		type = itemPtr->parm[0];

	if (type > 3)		// validate
		return(true);

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOSET_ObjType_CardboardBox1 + type;
	gNewObjectDefinition.scale 		= gTerrainPolygonSize * 6.0f;		// must match OreoTerrain's special rect size of N

	y = GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale);		// see if stacked box
	if (stackLevel > 0)
		y += (float)stackLevel * (gNewObjectDefinition.scale * .75f);

	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y	= y;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB - 1;				// put @ end of list so that collision against box has final say
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW;
	if (stackLevel == 0)
		newObj->CType |= CTYPE_BLOCKCAMERA;

	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);

	return(true);													// item was added
}


/************************* ADD SHOE BOX *********************************/

Boolean AddShoeBox(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
float	y;
int		stackLevel = itemPtr->parm[1];


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOSET_ObjType_ShoeBox;
	gNewObjectDefinition.scale 		= gTerrainPolygonSize * 8.0f;		// must match OreoTerrain's special rect size of N

	y = GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale);		// see if stacked box
	if (stackLevel > 0)
		y += (float)stackLevel * (gNewObjectDefinition.scale * .44f);

	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y	= y;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB - 1;				// put @ end of list so that collision against box has final say
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI/2);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 		= CTYPE_MISC|CTYPE_BLOCKSHADOW;
	if (stackLevel == 0)
		newObj->CType |= CTYPE_BLOCKCAMERA;

	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);

	return(true);													// item was added
}


/************************* ADD CLOSET WALL CARD *********************************/

Boolean AddClosetWall(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
const float scales[3] = {4, BOOK_SCALE, BOOK_SCALE};
int	type = itemPtr->parm[1];

	if (type > 2)			// validate
		return(true);

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOSET_ObjType_PCICard + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB - 10;			// put toward end of list so that collision against box has final say
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.scale 		= scales[type];
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/4);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC | CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);

	return(true);													// item was added
}


/************************* ADD BOOK STACK *********************************/

Boolean AddBookStack(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int	type = itemPtr->parm[0];

	if (type > 2)			// validate
		return(true);

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOSET_ObjType_FlatBook + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB - 10;			// put toward end of list so that collision against box has final say
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	if (type == 2)													// keep tall stack short
		gNewObjectDefinition.scale 		= BOOK_SCALE/3;
	else
		gNewObjectDefinition.scale 		= BOOK_SCALE;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/4);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC | CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);

	return(true);													// item was added
}



#pragma mark -

/************************* ADD SILICON DOOR *********************************/

Boolean AddSiliconDoor(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*door;
int		isOpen = itemPtr->flags & ITEM_FLAGS_USER1;

				/**************/
				/* MAKE MODEL */
				/**************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOSET_ObjType_SiliconDoor;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + 10.0f;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SILICONDOOR_SLOT;		// put toward end so collision has final say
	gNewObjectDefinition.moveCall 	= MoveSiliconDoor;
	if (isOpen)
		gNewObjectDefinition.rot 	= itemPtr->parm[0] * PI/2 + PI/2;
	else
		gNewObjectDefinition.rot 	= itemPtr->parm[0] * PI/2;
	gNewObjectDefinition.scale 		= SILICON_DOOR_SCALE;
	door = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	door->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

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

	door->CBits			= CBITS_ALLSOLID|CBITS_IMPENETRABLE;
	CreateCollisionBoxFromBoundingBox_Rotated(door,1,1);

	door->TriggerCallback = DoTrig_SiliconDoor;

	return(true);
}


/******************** MOVE SILICON DOOR ******************/
//
// NOTE: don't ever delete the door - pain to fix all the parts
//

static void MoveSiliconDoor(ObjNode *theNode)
{
ObjNode 	*part;

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


			/***************/
			/* ALIGN PARTS */
			/***************/

	part = theNode->ChainNode;
	while(part)												// traverse all chained parts
	{
		part->Rot.y = theNode->Rot.y + part->HoldRot.y;		// match y rotation

		OGLPoint3D holdOffset = {part->HoldOffset.x, part->HoldOffset.y, part->HoldOffset.z};
		OGLPoint3D_Transform(&holdOffset, &theNode->BaseTransformMatrix, &part->Coord);
		UpdateObjectTransforms(part);

		part = part->ChainNode;
	}


}


/************** DO TRIGGER - SILICON DOOR ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_SiliconDoor(ObjNode *door, ObjNode *who, Byte sideBits)
{
ObjNode	*held;

	(void) who;
	(void) sideBits;

			/* SEE WHICH SILICON PART WE HAVE & SET FLAGS */

	held = gPlayerInfo.heldObject;
	if (held == nil)
		return(true);

	switch(held->Kind)
	{
		case	PICKUP_KIND_CHIP1:
				door->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;		// got chip #1
				break;

		case	PICKUP_KIND_CHIP2:
				door->TerrainItemPtr->flags |= ITEM_FLAGS_USER2;		// got chip #2
				break;

		case	PICKUP_KIND_BATTERY:								// we have the battery
				door->TerrainItemPtr->flags |= ITEM_FLAGS_USER3;
				break;

		default:
				return(true);
	}


				/* ATTACH THE PART TO THE DOOR */

	gPlayerInfo.heldObject = nil;								// player doesn't have it anymore

	AttachPartToDoor(held, door);

	PlayEffect3D(EFFECT_CHIPCLICK, &door->Coord);

	gStartedSiliconDoor = true;


			/* SEE IF THAT WAS EVERYTHING */

	if ((door->TerrainItemPtr->flags & (ITEM_FLAGS_USER1 | ITEM_FLAGS_USER2 | ITEM_FLAGS_USER3)) ==
										(ITEM_FLAGS_USER1 | ITEM_FLAGS_USER2 | ITEM_FLAGS_USER3))
	{
		door->Mode = DOOR_MODE_OPENING;
		door->CType = 0;										// not solid while opening
		PlayEffect3D(EFFECT_SILICONDOOROPEN, &door->Coord);
	}

	return(true);
}


/********************** ATTACH PART TO DOOR ***************************/

static void AttachPartToDoor(ObjNode *part, ObjNode *door)
{
ObjNode	*link;
int		i, n = part->Kind - PICKUP_KIND_CHIP1;

	part->MoveCall = nil;							// part is on door, so don't move it anymore
	part->CType = 0;


			/* FIND LAST OBJECT IN CHAIN */

	link = door;
	while(link->ChainNode)
		link = link->ChainNode;


			/* ATTACH IT */

	link->ChainNode = part;


			/* SET COORD OFFSET */

	switch(n)
	{
		case	0:							// chip 1
				part->Rot.x = PI/2;
				part->Rot.z = 0;
				part->HoldRot.y = 0;

				part->HoldOffset.x = 105.0f;
				part->HoldOffset.z = 15.0f;

				part->HoldOffset.y = 325.0f;
				break;

		case	1:							// chip 2
				part->HoldRot.y = 0;
				part->Rot.x = PI/2;
				part->Rot.z = 0;

				part->HoldOffset.x = 105.0f;
				part->HoldOffset.z = 15.0f;

				part->HoldOffset.y = 80.0f;
				break;

		case	2:							// we have a battery
				part->Rot.x = 0;
				part->HoldRot.y = PI/2;
				part->Rot.z = 0;

				part->HoldOffset.x = 400.0f;
				part->HoldOffset.y = 75.0f;
				part->HoldOffset.z = 35.0f;
				break;

	}


			/* CREATE LED SPARKLE */

	i = door->Sparkles[n] = GetFreeSparkle(door);				// get free sparkle slot
	if (i != -1)
	{
		const OGLPoint3D ledOffs[3] =
		{
			{215, 85, 30},
			{260, 70, 30},
			{300, 88, 30},
		};


		gSparkles[i].flags = SPARKLE_FLAG_TRANSFORMWITHOWNER | SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = ledOffs[n];

		gSparkles[i].color.r = .8;
		gSparkles[i].color.g = .8;
		gSparkles[i].color.b = .8;
		gSparkles[i].color.a = 0;								// start off since we check battery below

		gSparkles[i].scale 		= 80.0f;
		gSparkles[i].separation = 20.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_GreenGlint;
	}


			/* IF BATTERY IN, MAKE SURE ALL LED'S ARE ON */

	if (door->TerrainItemPtr->flags & ITEM_FLAGS_USER3)
	{
		for (n = 0; n < 3; n++)
		{
			if (door->Sparkles[n] != -1)
				gSparkles[door->Sparkles[n]].color.a = 1.0f;
		}
	}


}

#pragma mark -

/********************* ADD PICTURE FRAME *************************/

Boolean AddPictureFrame(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOSET_ObjType_PictureFrame_Brian + itemPtr->parm[0];
	gNewObjectDefinition.scale 		= 7.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x, z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 262;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI/2);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 		= CTYPE_MISC | CTYPE_BLOCKCAMERA;
	newObj->CBits		= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,1,1);

	return(true);
}



/********************* ADD LILY PAD *************************/

Boolean AddLilyPad(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PARK_ObjType_LilyPad;
	gNewObjectDefinition.scale 		= 3.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	GetWaterY(x,z, &gNewObjectDefinition.coord.y);
	gNewObjectDefinition.coord.y += 4.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 177;
	gNewObjectDefinition.moveCall 	= MoveLilyPad;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 		= CTYPE_MISC | CTYPE_BLOCKCAMERA | CTYPE_BLOCKSHADOW;
	newObj->CBits		= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,.9,1);

	newObj->BottomOff = -2000;										// give infinite bottom so fish dont swim under
	CalcObjectBoxFromNode(newObj);

	newObj->Wobble = RandomFloat() * PI2;

	return(true);
}


/**************** MOVE LILY PAD *********************/

static void MoveLilyPad(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

	theNode->Wobble += fps * 4.0f;
	gCoord.y = theNode->InitCoord.y + sin(theNode->Wobble) * 4.0f;

	UpdateObject(theNode);

}


/********************* ADD CAT TAIL *************************/

Boolean AddCatTail(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PARK_ObjType_CatTail;
	gNewObjectDefinition.scale 		= 2.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 177;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 		= CTYPE_MISC;
	newObj->CBits		= CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj,1600,0,-60,60,60,-60);

	return(true);
}



/********************* ADD PLATFORM FLOWER *************************/

Boolean AddPlatformFlower(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int	type = itemPtr->parm[0];
float	s;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PARK_ObjType_ShortFlower + type;
	s = gNewObjectDefinition.scale 		= 1.2;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 177;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 		= CTYPE_MISC | CTYPE_BLOCKSHADOW | CTYPE_MPLATFORM;
	newObj->CBits		= CBITS_ALLSOLID;


	switch(type)
	{
				/* SHORT */

		case	0:
				SetObjectCollisionBounds(newObj, 180.0f * s, 0, -20.0f * s, 20.0f * s, 20.0f * s, -20.0f * s);				// stem
				AddCollisionBoxToObject(newObj, 218.0f * s, 170.0f * s, -165.0f * s, 165.0f * s, 165.0f * s, -165.0f * s); // flower
				break;

				/* MEDIUM */

		case	1:
				SetObjectCollisionBounds(newObj, 395.0f * s, 0, -20.0f * s, 20.0f * s, 20.0f * s, -20.0f * s);				// stem
				AddCollisionBoxToObject(newObj, 448.0f * s, 400.0f * s, -160.0f * s, 160.0f * s, 160.0f * s, -160.0f * s);	// flower
				break;

				/* TALL */

		case	2:
				SetObjectCollisionBounds(newObj, 395.0f * s, 0, -20.0f * s, 20.0f * s, 20.0f * s, -20.0f * s);				// stem
				AddCollisionBoxToObject(newObj, 685.0f * s, 645.0f * s, -160.0f * s, 160.0f * s, 160.0f * s, -160.0f * s);	// flower
				break;

	}

	return(true);
}


/********************* ADD SILVERWARE *************************/

Boolean AddSilverware(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PARK_ObjType_Fork + itemPtr->parm[0];
	gNewObjectDefinition.scale 		= 3.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 600;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/4);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 		= CTYPE_MISC|CTYPE_BLOCKSHADOW;
	newObj->CBits		= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);


	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULARDARK, 3.5,15, false);

	return(true);
}


