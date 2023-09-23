/****************************/
/*    	BEE HIVE.C		    */
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

static void MoveBeeHive(ObjNode *theNode);
static void CountKindling(ObjNode *hive);
static void MoveKindling(ObjNode *theNode);
static void MoveKindling2(ObjNode *theNode);
static void MoveBeeOutOfHive(ObjNode *bee);
static void LetBeesOutOfHive(ObjNode *hive);



/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

int		gKindlingCount = 0;
Boolean	gBurnKindling = false;

#define	DoorOpenAngle	SpecialF[0]
#define	InitRotY		SpecialF[1]

static float gBeeFreeTimer= 0;
int	gNumFreedBees = 0;


/************************* ADD BEE HIVE *********************************/

Boolean AddBeeHive(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj, *door;
static OGLPoint3D doorOff = {-135, 350, -50};
const float	s = 2.0;

			/*************/
			/* MAKE HIVE */
			/*************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PARK_ObjType_Hive;
	gNewObjectDefinition.scale 		= s;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 511;
	gNewObjectDefinition.moveCall 	= MoveBeeHive;
	gNewObjectDefinition.rot 		= 0; //(float)itemPtr->parm[0] * (PI2/4);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC | CTYPE_BLOCKSHADOW | CTYPE_MPLATFORM;
	newObj->CBits			= CBITS_ALLSOLID;

	SetObjectCollisionBounds(newObj, 1940 * s, 0, -200 * s, 200 * s, 900 * s, 460 * s);		// tree trunk
	AddCollisionBoxToObject(newObj, 570 * s, 200 * s, -145 * s, 155 * s, 155 * s, -155 * s);	// hive
	AddCollisionBoxToObject(newObj, 304 * s, 278 * s, -188 * s, 0, 65 * s, -65 * s);			// porch

			/*************/
			/* MAKE DOOR */
			/*************/

	OGLPoint3D_Transform(&doorOff, &newObj->BaseTransformMatrix, &gNewObjectDefinition.coord);

	gNewObjectDefinition.type 		= PARK_ObjType_HiveDoor;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	door = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	door->DoorOpenAngle = 0;
	door->InitRotY= door->Rot.y;

	newObj->ChainNode = door;

	return(true);													// item was added
}


/********************* MOVE BEE HIVE **********************/

static void MoveBeeHive(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}




			/* SEE IF DOOR SHOULD OPEN */

	if (gBurnKindling)
	{
		float	oldTimer = theNode->Timer;

		theNode->Timer += fps;
		if (theNode->Timer > 2.0f)
		{
			ObjNode *door = theNode->ChainNode;					// get door

			if (oldTimer <= 2.0f)								// see if just started
				PlayEffect3D(EFFECT_DOORCREAK, &door->Coord);

			door->DoorOpenAngle += fps * 2.0f;					// open
			if (door->DoorOpenAngle > (PI/2))					// see if all open
			{
				door->DoorOpenAngle = PI/2;

				LetBeesOutOfHive(theNode);
			}

			door->Rot.y = door->InitRotY - door->DoorOpenAngle;

			UpdateObjectTransforms(door);
		}
	}

		/* NOT BURNING YET, SO SEE IF HAVE KINDLING THERE */

	else
		CountKindling(theNode);



			/* SEE IF LET PLAYER ENTER */

	if ((gNumFreedBees >= NUM_FEE_BEES) && (!gLevelCompleted))
	{
		static OGLPoint3D doorOff = {-145, 330, 0};
		OGLPoint3D	p;

		OGLPoint3D_Transform(&doorOff, &theNode->BaseTransformMatrix, &p);

		if (OGLPoint3D_Distance(&gPlayerInfo.coord, &p) < 50.0f)
		{
			StartLevelCompletion(0);
		}
	}
}



/********************* COUNT KINDLING *************************/

static void CountKindling(ObjNode *hive)
{
ObjNode		*thisNodePtr;
float		d;
float		hiveX = hive->Coord.x;
float		hiveZ = hive->Coord.z;
ObjNode	*heldObj = gPlayerInfo.heldObject;
Boolean		buddyAttract = false;

	if (gKindlingCount >= KINDLING_NEEDED)							// if already @ needed amount then remember to mark it for Buddy Bug
		buddyAttract = true;

	gKindlingCount = 0;												// assume none

	thisNodePtr = gFirstNodePtr;

	do
	{
		if (thisNodePtr->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if ((thisNodePtr != heldObj) && (thisNodePtr->StatusBits & STATUS_BIT_ONGROUND))	// skip anything player is holding
		{
			if (thisNodePtr->What == WHAT_KINDLING)					// look for kindling
			{
				d = CalcQuickDistance(hiveX, hiveZ, thisNodePtr->Coord.x, thisNodePtr->Coord.z);
				if (d < 300.0f)
				{
							/* THIS ONE IS IN RANGE */

					if (buddyAttract)
						thisNodePtr->CType = CTYPE_PLAYERONLY | CTYPE_BUDDYATTRACT;
					else
						thisNodePtr->CType = CTYPE_PLAYERONLY;

					thisNodePtr->GotKickedCallback = nil;
					thisNodePtr->MoveCall = MoveKindling2;


					gKindlingCount++;								// inc count of eligible kindling

					if (gKindlingCount > KINDLING_NEEDED)			// only do this many to avoid an overload on the fire effect
						break;
				}
			}
		}

		thisNodePtr = (ObjNode *)thisNodePtr->NextNode;				// next node
	}
	while (thisNodePtr != nil);

}



#pragma mark -


/************************* ADD KINDLING *********************************/

Boolean AddKindling(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
Boolean	part = itemPtr->parm[0];

			/* MAKE OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PARK_ObjType_Leaf + part;
	gNewObjectDefinition.scale 		= 2.2;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y	= FindHighestCollisionAtXZ(x, z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN) - gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 281+part;
	gNewObjectDefinition.moveCall 	= MoveKindling;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->What = WHAT_KINDLING;
	newObj->Kind = PICKUP_KIND_KINDLING;							// remember what kind of pickup this is
	newObj->DropCallback = DefaultDropObject;						// set drop callback

	switch(part)
	{
		case	0:													// leaf
				newObj->HoldOffset.x = -14;
				newObj->HoldOffset.y = -16;
				newObj->HoldOffset.z = -40;
				newObj->HoldRot.x = .3;
				break;

		case	1:													// twig
				newObj->HoldOffset.x = -14;
				newObj->HoldOffset.y = -10;
				newObj->HoldOffset.z = 0;
				newObj->HoldRot.y = -.4;
				newObj->HoldRot.x = -.5;
				break;
	}

			/* SET COLLISION STUFF */

	newObj->CType 		= CTYPE_MISC|CTYPE_PICKUP|CTYPE_KICKABLE;
	newObj->CBits		= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,.8,.8);

	newObj->GotKickedCallback = DefaultGotKickedCallback;			// set callback for being kicked


	newObj->SmokeParticleGroup = -1;


			/* MAKE SHADOW */

//	AttachShadowToObject(newObj, 0, 2,2, false);

	return(true);
}


/********************** MOVE KINDLING **************************/

static void MoveKindling(ObjNode *theNode)
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


/********************* IGNITE KINDLING ****************************/

void IgniteKindling(ObjNode *theNode)
{
#pragma unused (theNode)
	gBurnKindling = true;
	gNumFreedBees = 0;
}


/*********************** MOVE KINDLING 2 ***************************/

static void MoveKindling2(ObjNode *theNode)
{
int		i;
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;
float	x,y,z;


	if (!gBurnKindling)											// dont do anything until we can burn
		return;

	theNode->CType = CTYPE_PLAYERONLY;							// make sure no more buddy bugs can hit


	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;



		/**************/
		/* MAKE SMOKE */
		/**************/

	theNode->SmokeTimer -= fps;													// see if add smoke
	if (theNode->SmokeTimer <= 0.0f)
	{
		theNode->SmokeTimer += .2f;												// reset timer

		particleGroup 	= theNode->SmokeParticleGroup;
		magicNum 		= theNode->SmokeParticleMagic;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{

			theNode->SmokeParticleMagic = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND; //PARTICLE_FLAGS_ALLAIM;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 25.0f;
			groupDef.decayRate				=  -.5f;
			groupDef.fadeRate				= .12;
			groupDef.particleTextureNum		= PARTICLE_SObjType_BlackSmoke;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			theNode->SmokeParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * 40.0f;
				p.y = y + 80.0f + RandomFloat() * 50.0f;
				p.z = z + RandomFloat2() * 40.0f;

				d.x = RandomFloat2() * 80.0f;
				d.y = 100.0f + RandomFloat() * 80.0f;
				d.z = RandomFloat2() * 80.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2();
				newParticleDef.alpha		= .7;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->SmokeParticleGroup = -1;
					break;
				}
			}
		}
	}

		/*************/
		/* MAKE FIRE */
		/*************/

	theNode->FireTimer -= fps;													// see if add fire
	if (theNode->FireTimer <= 0.0f)
	{
		theNode->FireTimer += .1;										// reset timer

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= -200;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 35.0f;
			groupDef.decayRate				=  0;
			groupDef.fadeRate				= 2.0;
			groupDef.particleTextureNum		= PARTICLE_SObjType_Fire;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * 30.0f;
				p.y = y - RandomFloat() * 50.0f;
				p.z = z + RandomFloat2() * 30.0f;

				d.x = RandomFloat2() * 50.0f;
				d.y = 200.0f + RandomFloat() * 100.0f;
				d.z = RandomFloat2() * 50.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2();
				newParticleDef.alpha		= 1.0;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}

}


#pragma mark -


/****************** LET BEES OUT OF HIVE **************************/

static void LetBeesOutOfHive(ObjNode *hive)
{
ObjNode	*bee;
static OGLPoint3D doorOff = {-110, 330, 0};
float	r;

	if (gNumFreedBees > NUM_FEE_BEES)
		return;

			/* SEE IF MAKE NEW BEE */

	gBeeFreeTimer -= gFramesPerSecondFrac;
	if (gBeeFreeTimer <= 0.0f)
		gBeeFreeTimer = .17f;
	else
		return;


			/* CALC BEE ROT & COORD */

	r = hive->Rot.y + PI/2;
	r += RandomFloat2() * .3f;

	OGLPoint3D_Transform(&doorOff, &hive->BaseTransformMatrix, &gNewObjectDefinition.coord);



				/*******************/
				/* MAKE BEE OBJECT */
				/*******************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_BUMBLEBEE;
	gNewObjectDefinition.animNum 	= 0;
	gNewObjectDefinition.scale 		= 0.8;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 499;
	gNewObjectDefinition.moveCall 	= MoveBeeOutOfHive;
	gNewObjectDefinition.rot 		= r;

	bee = MakeNewSkeletonObject(&gNewObjectDefinition);

	bee->DeltaRot.y = RandomFloat2() * .7f;

	bee->Delta.y = -RandomFloat() * 400.0f;

	gNumFreedBees++;
}


/******************* MOVE BEE OUT OF HIVE *********************************/

static void MoveBeeOutOfHive(ObjNode *bee)
{
float fps = gFramesPerSecondFrac;
float	r,d;

	GetObjectInfo(bee);

			/* SEE IF BEE GONE */

	d = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	if (d > 2000.0f)
	{
		if (bee->StatusBits & STATUS_BIT_ISCULLED)							// delete when gone
		{
			DeleteObject(bee);
			return;
		}
	}


	r = bee->Rot.y += bee->DeltaRot.y * fps;


	gDelta.x = -sin(r) * 600.0f;
	gDelta.z = -cos(r) * 600.0f;
	gDelta.y += 200.0f * fps;
	if (gDelta.y > 100.0f)
		gDelta.y = 100.0f;

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y += gDelta.y * fps;

	UpdateObject(bee);


				/* UPDATE EFFECT */

	if (bee->EffectChannel == -1)
		bee->EffectChannel = PlayEffect_Parms3D(EFFECT_BUMBLERUMBLE, &bee->Coord, NORMAL_CHANNEL_RATE + (MyRandomLong() & 0x1fff), 1.0);
	else
		Update3DSoundChannel(EFFECT_BUMBLERUMBLE, &bee->EffectChannel, &bee->Coord);

}




















