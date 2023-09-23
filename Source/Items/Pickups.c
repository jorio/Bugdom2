/****************************/
/*   	PICKUPS.C		    */
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

static void MoveAcorn(ObjNode *theNode);
static void AcornGotKickedCallback(ObjNode *player, ObjNode *kickedObj);
static void MothBallGotKickedCallback(ObjNode *player, ObjNode *kickedObj);
static void MoveMothBall(ObjNode *theNode);
static void DropMothBall(ObjNode *player, ObjNode *ball);
static void MakeMothBallFumes(ObjNode *theNode);
static void MoveSiliconPart(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

#define	Explosive		Flag[0]						// set after acorn has been kicked indicating it will blow up
#define	CloverType		Special[0]

#define		Activated		Flag[0]


/************************ UPDATE HELD OBJECT ********************************/

void UpdateHeldObject(ObjNode *player)
{
ObjNode *held = gPlayerInfo.heldObject;
OGLMatrix4x4	m,mst,rm,m2;
float			scale;

	if (held == nil)											// see if anything being held
		return;

			/* CALC SCALE MATRIX */

	scale = held->Scale.x / player->Scale.x;					// to adjust from player's scale to held's scale
	OGLMatrix4x4_SetScale(&mst, scale, scale, scale);

			/* CALC TRANSLATE MATRIX */

	mst.value[M03] = held->HoldOffset.x;						// insert translation into scale matrix
	mst.value[M13] = held->HoldOffset.y;
	mst.value[M23] = held->HoldOffset.z;


			/* CALC ROTATE MATRIX */

	OGLMatrix4x4_SetRotate_XYZ(&rm, held->HoldRot.x, held->HoldRot.y, held->HoldRot.z);
	OGLMatrix4x4_Multiply(&mst, &rm, &m2);


			/* GET ALIGNMENT MATRIX */

	FindJointFullMatrix(player, PLAYER_JOINT_UPPER_RIGHT_ELBOW, &m);			// get joint's matrix

	OGLMatrix4x4_Multiply(&m2, &m, &held->BaseTransformMatrix);
	SetObjectTransformMatrix(held);


			/* GET COORDS FOR OBJECT & KEEP COLLISION BOX */
			//
			// even tho the collision box is turned off while the player
			// is carrying the object, we maintain these values
			// so that when the player drops the object things
			// dont freak out.
			//

	held->Coord.x = held->OldCoord.x = held->BaseTransformMatrix.value[M03];
	held->Coord.y = held->OldCoord.y = held->BaseTransformMatrix.value[M13];
	held->Coord.z = held->OldCoord.z = held->BaseTransformMatrix.value[M23];

	held->Rot.y = player->Rot.y;
	CalcObjectBoxFromNode(held);
	UpdateShadow(held);


			/* ALSO UPDATE ANY CHAINS */

	if (held->ChainNode)
	{
		ObjNode	*child = held->ChainNode;

		child->BaseTransformMatrix = held->BaseTransformMatrix;
		child->Coord = held->Coord;
		SetObjectTransformMatrix(child);
	}



		/* DO ANYTHING CUSTOM */

	switch(held->Kind )
	{
		case	PICKUP_KIND_MOTHBALL:
				MakeMothBallFumes(held);
				break;




	}

}


/********************** MOVE DEFAULT PICKUP **************************/

void MoveDefaultPickup(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (theNode->TerrainItemPtr)							// only track if can come back
	{
		if (TrackTerrainItem(theNode))							// just check to see if it's gone
		{
			DeleteObject(theNode);
			return;
		}
	}

	GetObjectInfo(theNode);

	gDelta.y -= 3000.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	HandleCollisions(theNode, CTYPE_TERRAIN | CTYPE_MISC | CTYPE_FENCE | CTYPE_WATER, .4);

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		gDelta.x *= .8f;
		gDelta.z *= .8f;
	}
	else
	if (theNode->StatusBits & STATUS_BIT_UNDERWATER)
	{
		int		patchNum;
		if (DoWaterCollisionDetect(theNode, gCoord.x, gCoord.y+theNode->BottomOff, gCoord.z, &patchNum))
		{
			gCoord.y = gWaterBBox[patchNum].max.y;
			gDelta.y = 0;
		}
	}


	UpdateObject(theNode);

			/* ALSO UPDATE ANY CHAINS */

	if (theNode->ChainNode)
	{
		ObjNode	*child = theNode->ChainNode;

		child->Coord = gCoord;
		UpdateObjectTransforms(child);
	}
}


/************************ DEFAULT DROP OBJECT **************************/
//
// The default callback for pickups.  Called when player wants to drop this.
//

void DefaultDropObject(ObjNode *player, ObjNode *held)
{
float	r = player->Rot.y;

	held->Rot.y = r;

		/* MAKE SURE NOT BEING DROPPED BEHIND A FENCE */

	if (CheckDropThruFence(player, held))
		return;

		/* DROP FROM GLIDE */

	if (IsPlayerDoingGlideAnim(player))
	{
		held->Delta.x = gDelta.x;				// match w/ player
		held->Delta.z = gDelta.z;
		held->Delta.y = -300.0f;
	}

		/* DROP FROM OTHER ANIM */
	else
	{
		held->Delta.x = gDelta.x - sin(r) * 100.0f;				// toss the object forward
		held->Delta.z = gDelta.z - cos(r) * 100.0f;
		held->Delta.y = 150.0f;
	}

	UpdateObjectTransforms(held);
	CreateCollisionBoxFromBoundingBox_Rotated(held,1,1);
}


/******************** CHECK DROP THRU FENCE ******************************/
//
// Also checks that not dropped into a solid either.
//

Boolean CheckDropThruFence(ObjNode *player, ObjNode *held)
{
OGLVector2D	v;
OGLPoint3D	p;

			/* SEE IF INTERSECTS ANY MISC SOLIDS */

	CollisionDetect(held, CTYPE_MISC, 0);
	if (gNumCollisions)
		goto put_here;


			/* SEE IF IN FENCE */

	v.x = held->Coord.x - player->Coord.x;
	v.y = held->Coord.z - player->Coord.z;
	FastNormalizeVector2D(v.x, v.y, &v, true);

	p.x = held->Coord.x + v.x * 40.0f;
	p.y = held->Coord.y;
	p.z = held->Coord.z + v.y * 40.0f;

	if (SeeIfLineSegmentHitsFence(&player->Coord, &p, nil, nil, nil))
	{
put_here:
		held->Coord = player->Coord;				// just drop under player to avoid being stuck in fence
		held->Delta.x = held->Delta.z = 0;
		return(true);
	}

	return(false);
}



/************************* DEFAULT GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

void DefaultGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	kickedObj->Delta.x = -sin(r) * 1100.0f;
	kickedObj->Delta.z = -cos(r) * 1100.0f;
	kickedObj->Delta.y = 600.0f;

	PlayEffect3D(EFFECT_ACORNKICKED, &kickedObj->Coord);
}


#pragma mark -



/************************* ADD ACORN *********************************/

Boolean AddAcorn(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int	type = itemPtr->parm[0];

	if (type > 2)						// verify
		return(true);

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Acorn;
	gNewObjectDefinition.scale 		= 1.1;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y	= FindHighestCollisionAtXZ(x, z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN) - (gObjectGroupBBoxList[MODEL_GROUP_GLOBAL][GLOBAL_ObjType_Acorn].min.y * gNewObjectDefinition.scale);
 	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 113;
	gNewObjectDefinition.moveCall 	= MoveAcorn;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Kind = PICKUP_KIND_ACORN;								// remember what kind of pickup this is
	newObj->What = WHAT_ACORN;

	newObj->CloverType = type;										// green, blue, or gold clover?

	newObj->DropCallback = DefaultDropObject;						// set drop callback
	newObj->HoldOffset.x = -14;										// set holding offset for Skip
	newObj->HoldOffset.y = -10;
	newObj->HoldOffset.z = -5;

			/* SET COLLISION STUFF */

	newObj->CType 		= CTYPE_MISC|CTYPE_PICKUP|CTYPE_KICKABLE;
	newObj->CBits		= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);

	newObj->GotKickedCallback = AcornGotKickedCallback;			// set callback for being kicked

	newObj->Explosive = false;


			/* MAKE SHADOW */

	AttachShadowToObject(newObj, 0, 2,2, true);

	return(true);
}


/********************** MOVE ACORN **************************/

static void MoveAcorn(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

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


				/* SEE IF BOUNCE */

	HandleCollisions(theNode, CTYPE_TERRAIN | CTYPE_MISC | CTYPE_FENCE, .5);
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		gDelta.x *= .8f;
		gDelta.z *= .8f;

				/* WHEN STOPPED, BLOW OPEN */

		if (theNode->Explosive)
		{
			if ((fabs(gDelta.x) < 30.0f) && (fabs(gDelta.z) < 30.0f))
			{
				ObjNode *clover;

				PlayEffect3D(EFFECT_POPACORN, &gCoord);
				ExplodeGeometry(theNode, 250, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, 1.0);
				MakeConfettiExplosion(gCoord.x, gCoord.y, gCoord.z, 150, 1.0, PARTICLE_SObjType_YwllowDiasyConfetti, 40);

				theNode->TerrainItemPtr = nil;														// dont come back!
				DeleteObject(theNode);

				clover = MakePOW(POW_KIND_GREENCLOVER + theNode->CloverType, &gCoord);				// put clover here
				clover->StatusBits |= STATUS_BIT_DOUBLESIDED;
				return;
			}
		}

				/* SEE IF MAKE BOUNCE SOUND */

		if (gGammaFadePercent >= 1.0f)
		{
			if ((gDelta.y > 60.0f) && (theNode->OldCoord.y > gCoord.y))
				PlayEffect_Parms3D(EFFECT_ACORNKICKED, &gCoord, NORMAL_CHANNEL_RATE * 2/3, .4f);
		}
	}


			/**********/
			/* UPDATE */
			/**********/

	UpdateObject(theNode);

}



/************************* ACORN GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

static void AcornGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	kickedObj->Delta.x = -sin(r) * 1100.0f;
	kickedObj->Delta.z = -cos(r) * 1100.0f;
	kickedObj->Delta.y = 600.0f;

	PlayEffect3D(EFFECT_ACORNKICKED, &kickedObj->Coord);

	kickedObj->Explosive = true;								// it will blow up when it stops
}


#pragma mark -


/************************* ADD MOTHBALL *********************************/

Boolean AddMothBall(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOSET_ObjType_MothBall;
	gNewObjectDefinition.scale 		= .95;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y	= FindHighestCollisionAtXZ(x, z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN) - gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 110;
	gNewObjectDefinition.moveCall 	= MoveMothBall;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Kind = PICKUP_KIND_MOTHBALL;							// remember what kind of pickup this is
	newObj->What = WHAT_MOTHBALL;

	newObj->Activated = false;


	newObj->DropCallback = DropMothBall;							// set drop callback
	newObj->HoldOffset.x = -14;										// set holding offset for Skip
	newObj->HoldOffset.y = -10;
	newObj->HoldOffset.z = -5;

			/* SET COLLISION STUFF */

	newObj->CType 		= CTYPE_MISC|CTYPE_PICKUP|CTYPE_KICKABLE;
	newObj->CBits		= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);

	newObj->GotKickedCallback = MothBallGotKickedCallback;			// set callback for being kicked


			/* MAKE SHADOW */

	AttachShadowToObject(newObj, 0, 2,2, false);

	return(true);
}


/************************ DROP MOTHBALL **************************/

static void DropMothBall(ObjNode *player, ObjNode *ball)
{
float	r = player->Rot.y;

	ball->Activated = true;										// once it's been "touched" it can roll on its own

		/* MAKE SURE NOT BEING DROPPED BEHIND A FENCE */

	if (CheckDropThruFence(player, ball))
		return;


		/* DROP FROM GLIDE */

	if (IsPlayerDoingGlideAnim(player))
	{
		ball->Delta.x = gDelta.x;								// match w/ player
		ball->Delta.z = gDelta.z;
		ball->Delta.y = -300.0f;
	}

		/* DROP FROM OTHER ANIM */
	else
	{
		ball->Delta.x = gDelta.x - sin(r) * 100.0f;				// toss the object forward
		ball->Delta.z = gDelta.z - cos(r) * 100.0f;
		ball->Delta.y = 150.0f;
	}

}


/********************** MOVE MOTHBALL **************************/

static void MoveMothBall(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

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

	HandleCollisions(theNode, CTYPE_TERRAIN | CTYPE_MISC | CTYPE_FENCE, .1);


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
				DoObjectFriction(theNode, 400);

				gDelta.x += gRecentTerrainNormal.x * (3000.0f * fps);
				gDelta.z += gRecentTerrainNormal.z * (3000.0f * fps);
			}
		}
	}

			/* UPDATE */

	UpdateObject(theNode);

	MakeMothBallFumes(theNode);

}



/************************* MOTHBALL GOT KICKED CALLBACK *************************/
//
// The default callback for kickable objects
//

static void MothBallGotKickedCallback(ObjNode *player, ObjNode *kickedObj)
{
float	r = player->Rot.y;

	kickedObj->Delta.x = -sin(r) * 1100.0f;
	kickedObj->Delta.z = -cos(r) * 1100.0f;
	kickedObj->Delta.y = 600.0f;

	PlayEffect3D(EFFECT_ACORNKICKED, &kickedObj->Coord);

	kickedObj->Explosive = true;								// it will blow up when it stops
}




/****************** MAKE MOTHBALL FUMES ************************/

static void MakeMothBallFumes(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;

	if (gFramesPerSecond < 15.0f)
		return;

	if (theNode->StatusBits & STATUS_BIT_ISCULLED)
		return;

	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer <= 0.0f)
	{
		theNode->ParticleTimer += 0.07f;


		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND | PARTICLE_FLAGS_ALLAIM;
			groupDef.gravity				= 80;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 10;
			groupDef.decayRate				= -.7;
			groupDef.fadeRate				= .1;
			groupDef.particleTextureNum		= PARTICLE_SObjType_GreySmoke;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			p.x = theNode->Coord.x + RandomFloat2() * 10.0f;
			p.y = theNode->Coord.y + RandomFloat2() * 10.0f;
			p.z = theNode->Coord.z + RandomFloat2() * 10.0f;

			d.x = RandomFloat2() * 60.0f;
			d.y = 50.0f + RandomFloat() * 50.0f;
			d.z = RandomFloat2() * 60.0f;

			newParticleDef.groupNum		= particleGroup;
			newParticleDef.where		= &p;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= RandomFloat()*PI2;
			newParticleDef.rotDZ		= RandomFloat2();
			newParticleDef.alpha		= .15;
			if (AddParticleToGroup(&newParticleDef))
			{
				theNode->ParticleGroup = -1;
			}
		}
	}
}


#pragma mark -


/************************* ADD SILICON PART *********************************/

Boolean AddSiliconPart(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
Boolean	part = itemPtr->parm[0];

			/* MAKE OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOSET_ObjType_Chip1 + part;
	gNewObjectDefinition.scale 		= SILICON_DOOR_SCALE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y	= FindHighestCollisionAtXZ(x, z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN) - gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SILICONDOOR_SLOT+1;			// must be after door since this gets chained to door eventually
	gNewObjectDefinition.moveCall 	= MoveSiliconPart;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Kind = PICKUP_KIND_CHIP1 + part;						// remember what kind of pickup this is

	newObj->DropCallback = DefaultDropObject;						// set drop callback

	switch(part)
	{
		case	0:													// CHIP 1
				newObj->HoldOffset.x = -14;
				newObj->HoldOffset.y = -3;
				newObj->HoldOffset.z = -36;

				newObj->HoldRot.x = -.5;
				break;

		case	1:													// CHIP 2
				newObj->HoldOffset.x = -14;
				newObj->HoldOffset.y = -4;
				newObj->HoldOffset.z = -27;

				newObj->HoldRot.x = -.5;
				break;

		case	2:													// BATTERY
				newObj->HoldOffset.x = -14;
				newObj->HoldOffset.y = -15;
				newObj->HoldOffset.z = -10;

				newObj->HoldRot.x = .4f;
				break;
	}

			/* SET COLLISION STUFF */

	newObj->CType 		= CTYPE_MISC|CTYPE_PICKUP|CTYPE_KICKABLE;
	newObj->CBits		= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);

	newObj->GotKickedCallback = DefaultGotKickedCallback;			// set callback for being kicked


			/* MAKE SHADOW */

	AttachShadowToObject(newObj, 0, 2,2, false);

	return(true);
}


/********************** MOVE SILICON PART **************************/

static void MoveSiliconPart(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

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


				/* SEE IF BOUNCE */

	HandleCollisions(theNode, CTYPE_TERRAIN | CTYPE_MISC | CTYPE_FENCE, .5);
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		gDelta.x *= .8f;
		gDelta.z *= .8f;
	}


			/**********/
			/* UPDATE */
			/**********/

	UpdateObject(theNode);

}





















