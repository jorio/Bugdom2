/****************************/
/*    	ITEMS3.C		    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "3dmath.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gCurrentAspectRatio,gTerrainPolygonSize;
extern	OGLPoint3D			gCoord;
extern	OGLVector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLBoundingBox 		gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	OGLSetupOutputType	*gGameViewInfoPtr;
extern	u_long				gAutoFadeStatusBits,gGlobalMaterialFlags;
extern	Boolean				gG4,gSolidTriggerKeepDelta,gResetRideBall, gDoGlidingAtApex;
extern	int					gLevelNum,gNumBowlingPinsDown;
extern	SparkleType	gSparkles[MAX_SPARKLES];
extern	short				gNumEnemies;
extern	SpriteType	*gSpriteGroupList[];
extern	AGLContext		gAGLContext;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveSodaCan(ObjNode *can);
static void DoSodaSpray(ObjNode *theNode);
static void SeeIfPlayerInSodaSpray(ObjNode *can);
static void MoveGarbageGlider(ObjNode *theNode);
static void SeeIfAttachGliderProp(ObjNode *glider, ObjNode *prop);
static void SeeIfAttachGliderWheel(ObjNode *glider, ObjNode *wheel);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	CAN_SCALE	2.0f

/*********************/
/*    VARIABLES      */
/*********************/

#define	TabPopped	Flag[0]


#define	GliderHasLeftWheel 	Flag[0]
#define	GliderHasRightWheel Flag[1]
#define	GliderHasProp		Flag[2]

const OGLPoint3D gAxels[3] =
{
	-64, 2, -104,				// left wheel
	64, 2, -104,				// right wheel
	0,46,-142,					// prop
};

static	ObjNode					*gGliderPart[3] = {nil, nil, nil};

Boolean	gPoppedSodaCan = false;


/************************* ADD SODA CAN *********************************/

Boolean AddSodaCan(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj, *cap, *tab;

				/************/
				/* MAKE CAN */
				/************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= GARBAGE_ObjType_Can;
	gNewObjectDefinition.scale 		= CAN_SCALE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 330;
	gNewObjectDefinition.moveCall 	= MoveSodaCan;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_BLOCKCAMERA|CTYPE_MPLATFORM;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);


			/************/
			/* MAKE TAB */
			/************/

	gNewObjectDefinition.type 		= GARBAGE_ObjType_Tab;
	gNewObjectDefinition.coord.z	-= 9.0f * newObj->Scale.x;
	gNewObjectDefinition.coord.y 	+= 196.0f * newObj->Scale.x;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	tab = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			/* SET COLLISION STUFF */

	tab->Kind = PICKUP_KIND_CANTAB;								// remember what kind of pickup this is

	tab->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_MPLATFORM|CTYPE_PICKUP;
	tab->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(tab, 1, 1);

	newObj->ChainNode = tab;
	tab->ChainHead = newObj;

			/************/
			/* MAKE CAP */
			/************/

	gNewObjectDefinition.type 		= GARBAGE_ObjType_Cap;
	gNewObjectDefinition.coord	 	= newObj->Coord;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
//	gNewObjectDefinition.flags 		|= STATUS_BIT_DOUBLESIDED;		// note:  this was causing OS X to crash with 10.1.3
	cap = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	tab->ChainNode = cap;


	return(true);													// item was added
}


/********************* MOVE SODA CAN **********************/

static void MoveSodaCan(ObjNode *can)
{
const OGLPoint3D capOff = {0, 194 * CAN_SCALE, -6 * CAN_SCALE};

	if (TrackTerrainItem(can))							// just check to see if it's gone
	{
		DeleteObject(can);
		return;
	}


			/******************************/
			/* SEE IF CAN HAS BEEN POPPED */
			/******************************/

	if (can->TabPopped)
	{
		float	fps = gFramesPerSecondFrac;
		ObjNode	*tab, *cap;
		OGLMatrix4x4	m,sm,m2;

		tab = can->ChainNode;							// get tab & cap
		cap = tab->ChainNode;

			/* ROT TAB INTO POSITION */

		tab->Rot.x -= fps * 8.0f;
		if (tab->Rot.x < -.9f)
			tab->Rot.x = -.9f;

		UpdateObjectTransforms(tab);

			/* ROT CAP INTO POSITION */

		cap->Rot.x -= fps * 9.0f;
		if (cap->Rot.x < -1.1f)
			cap->Rot.x = -1.1f;


				/* SET CAP MATRIX */

		OGLMatrix4x4_SetRotateAboutPoint(&m, &capOff, cap->Rot.x, cap->Rot.y, 0);
		OGLMatrix4x4_SetTranslate(&m2, can->Coord.x, can->Coord.y, can->Coord.z);
		OGLMatrix4x4_Multiply(&m, &m2, &m);
		OGLMatrix4x4_SetScale(&sm, can->Scale.x, can->Scale.x, can->Scale.x);
		OGLMatrix4x4_Multiply(&sm, &m, &cap->BaseTransformMatrix);
		SetObjectTransformMatrix(cap);


			/*****************/
			/* DO SODA SPRAY */
			/*****************/

		DoSodaSpray(can);

		SeeIfPlayerInSodaSpray(can);
	}
}


/************ POP SODA CAN TAB ************************/

void PopSodaCanTab(ObjNode *tab)
{
	tab->CType = 0;										// make sure we can't pop it again
	tab->ChainHead->TabPopped = true;					// tell can we're popped

	PlayEffect3D(EFFECT_CANOPEN, &tab->Coord);

	gPoppedSodaCan = true;
}


/******************** DO SODA SPRAY **************************/

static void DoSodaSpray(ObjNode *theNode)
{
float				fps = gFramesPerSecondFrac;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
int					particleGroup,magicNum,i;
OGLVector3D			d;
OGLPoint3D			p;
float				x,y,z,dist;

				/*****************/
				/* UPDATE EFFECT */
				/*****************/

	if (theNode->EffectChannel == -1)
		theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_SODASPRAY, &theNode->Coord, NORMAL_CHANNEL_RATE, 1.0);
	else
		Update3DSoundChannel(EFFECT_SODASPRAY, &theNode->EffectChannel, &theNode->Coord);


				/**************/
				/* MAKE SPRAY */
				/**************/

	dist = CalcQuickDistance(theNode->Coord.x, theNode->Coord.z,
							gGameViewInfoPtr->cameraPlacement.cameraLocation.x,
							gGameViewInfoPtr->cameraPlacement.cameraLocation.z);

	if ((theNode->StatusBits & STATUS_BIT_ISCULLED) && (dist > 1000.0f))	// no spray if culled & far enough away
		return;


	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer <= 0.0f)										// see if time to make particles
	{
		theNode->ParticleTimer += .05f;

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND | PARTICLE_FLAGS_ALLAIM;
			groupDef.gravity				= 2800;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 25;
			groupDef.decayRate				=  -5.0;
			groupDef.fadeRate				= 1.0;
			groupDef.particleTextureNum		= PARTICLE_SObjType_CokeSpray;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			const OGLPoint3D openingOff = {0, 180, -24};

			OGLPoint3D_Transform(&openingOff, &theNode->BaseTransformMatrix, &p);	// calc coord of start
			x = p.x;
			y = p.y;
			z = p.z;

			for (i = 0; i < 4; i++)
			{
				p.x = x + RandomFloat2() * 20.0f;
				p.y = y + RandomFloat2() * 40.0f;
				p.z = z + RandomFloat2() * 20.0f;

				d.x = RandomFloat2() * 90.0f;
				d.z = RandomFloat2() * 90.0f;
				d.y = 2000.0f + RandomFloat2() * 600.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= 1.0;
				newParticleDef.rotZ			= RandomFloat()*PI2;
				newParticleDef.rotDZ		= RandomFloat2() * 8.0f;
				newParticleDef.alpha		= .8f;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}

/********************* SEE IF PLAYER IN SODA SPRAY ***************************/

static void SeeIfPlayerInSodaSpray(ObjNode *can)
{
const OGLPoint3D openingOff = {0, 190, -24};
OGLPoint3D	p;
float		d,playerY;

				/* CALC COORD OF SPRAY COLUMN */

	OGLPoint3D_Transform(&openingOff, &can->BaseTransformMatrix, &p);


				/* SEE IF PLAYER IS IN THE COLUMN */

	playerY = gPlayerInfo.coord.y;								// get player y coord
	if (playerY <= p.y)											// if player under it then bail
		return;

	d = CalcDistance(p.x, p.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);	// calc x/z dist from column
	if (d > 40.0f)
		return;

					/********************************/
					/* PLAYER IS IN THE SODA COLUMN */
					/********************************/

					/* SEE IF PLAYER IN ACCELERATION ZONE */

	d = playerY - p.y;
	d = 1400.0f - d;
	if (d > 800.0f)
	{
		ObjNode	*player = gPlayerInfo.objNode;

		d *= 2.2f;
		if (player->Delta.y < d)
			player->Delta.y = d;

		if (!IsPlayerDoingJumpAnim(player))
		{
			if (!IsPlayerDoingGlideAnim(player))
			{
				SetPlayerJumpAnim(player, false);			// player goes into jump
				gDoGlidingAtApex = true;					// and make auto-fly at top
			}
		}
	}

}



#pragma mark -




/************************* ADD VEGGIE *********************************/

Boolean AddVeggie(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];
float	offset;

	if (type > 3)			// make sure valid
		return(true);

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= GARBAGE_ObjType_Banana + type;
	gNewObjectDefinition.scale 		= 3.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 464 + type;;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SINK INTO GROUND */

	offset = RandomFloat() * 60.0f;
	newObj->Coord.y -= offset;
	newObj->Rot.x += offset * .013f;
	newObj->Rot.z += offset * .009f;
	UpdateObjectTransforms(newObj);


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);



	return(true);													// item was added
}



/************************* ADD JAR *********************************/

Boolean AddJar(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];
float	offset;

	if (type > 1)				// validate
		return(true);

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= GARBAGE_ObjType_Jar + type;
	gNewObjectDefinition.scale 		= 5.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 720;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SINK INTO GROUND */

	offset = RandomFloat() * 100.0f;
	newObj->Coord.y -= offset;
	newObj->Rot.x += offset * .002f;
	newObj->Rot.z += offset * .001f;
	UpdateObjectTransforms(newObj);


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);



	return(true);													// item was added
}



/************************* ADD TIN CAN *********************************/

Boolean AddTinCan(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];
float	offset;

	if (type > 1)		// make sure valid
		return(true);

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= GARBAGE_ObjType_TinCan;
	gNewObjectDefinition.scale 		= 3.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x,z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 119;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	if (type == 0)
		gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	else
		gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/4);

	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* LAY ON SIDE */

	if (type == 1)
	{
		newObj->Rot.x = PI/2;								// lay on side
		newObj->Coord.y += gObjectGroupBBoxList[MODEL_GROUP_LEVELSPECIFIC][GARBAGE_ObjType_TinCan].max.x * newObj->Scale.x;
	}

			/* SINK INTO GROUND */

	offset = RandomFloat() * 50.0f;
	newObj->Coord.y -= offset;
	newObj->Rot.x += offset * .004f;
	newObj->Rot.z += offset * .002f;
	UpdateObjectTransforms(newObj);


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);


	return(true);													// item was added
}



/************************* ADD DETERGENT *********************************/

Boolean AddDetergent(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
float	offset;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= GARBAGE_ObjType_Detergent;
	gNewObjectDefinition.scale 		= 3.2;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 89;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/4);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SINK INTO GROUND */

	offset = RandomFloat() * 200.0f;
	newObj->Coord.y -= offset;
//	newObj->Rot.x += offset * .001f;
//	newObj->Rot.z += offset * .0005f;
	UpdateObjectTransforms(newObj);


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1.0, .7);


	return(true);													// item was added
}




/************************* ADD BOX WALL *********************************/

Boolean AddBoxWall(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;
float	offset;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= GARBAGE_ObjType_BoxWall;
	gNewObjectDefinition.scale 		= 3.5;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 89;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/4);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SINK INTO GROUND */

	offset = RandomFloat() * 250.0f;
	newObj->Coord.y -= offset;
	newObj->Rot.x += offset * .001f;
	newObj->Rot.z += offset * .0005f;
	UpdateObjectTransforms(newObj);


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKSHADOW;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);


	return(true);													// item was added
}


#pragma mark -

/************************* ADD GLIDER PART *********************************/

Boolean AddGliderPart(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj, *rubber;
int		part = itemPtr->parm[0];

					/**************/
					/* MAKE MODEL */
					/**************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= GARBAGE_ObjType_Glider + part;
	gNewObjectDefinition.scale 		= 3.5;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y	= FindHighestCollisionAtXZ(x, z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN) - gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 199;
	gNewObjectDefinition.moveCall 	= MoveDefaultPickup;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


	switch(part)
	{
					/* FUSILAGE */

		case	0:
				newObj->Rot.x = 0.18f;
				newObj->Coord.y -= 60.0f;
				newObj->MoveCall = MoveGarbageGlider;

						/* MAKE RUBBER BAND */

				gNewObjectDefinition.type 		= GARBAGE_ObjType_Rubber;
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
				gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
				gNewObjectDefinition.moveCall 	= nil;
				rubber = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->ChainNode = rubber;
				break;

					/* WHEEL */

		case	1:

				newObj->GotKickedCallback = DefaultGotKickedCallback;			// set callback for being kicked
				newObj->Kind = PICKUP_KIND_WHEEL;								// remember what kind of pickup this is
				newObj->DropCallback = DefaultDropObject;						// set drop callback

				newObj->HoldOffset.x = -14;										// set holding offset for Skip
				newObj->HoldOffset.y = -20;
				newObj->HoldOffset.z = -5;

				newObj->HoldRot.x = .7f;

				newObj->CType 		= CTYPE_MISC|CTYPE_PICKUP|CTYPE_KICKABLE;
				newObj->CBits		= CBITS_ALLSOLID;
				CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);

				AttachShadowToObject(newObj, 0, 5,5, true);
				break;


					/* PROPELLER */

		case	2:

				newObj->Kind = PICKUP_KIND_PROPELLER;							// remember what kind of pickup this is
				newObj->DropCallback = DefaultDropObject;						// set drop callback

				newObj->HoldOffset.x = -14;										// set holding offset for Skip
				newObj->HoldOffset.y = -10;
				newObj->HoldOffset.z = -5;


				newObj->CType 		= CTYPE_MISC|CTYPE_PICKUP;
				newObj->CBits		= CBITS_ALLSOLID;
				CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);

				AttachShadowToObject(newObj, 0, 17,3, true);
				break;


	}

	UpdateObjectTransforms(newObj);

	return(true);													// item was added
}



/********************* MOVE GARBAGE GLIDER **********************/

static void MoveGarbageGlider(ObjNode *theNode)
{
ObjNode	*held, *rubber = theNode->ChainNode;
OGLPoint3D	axelPts[3];
int			i,numParts;
OGLMatrix4x4	rm;


			/**********************/
			/* SEE IF ATTACH PART */
			/**********************/

	held = gPlayerInfo.heldObject;							// get player's held objnode
	if (held)
	{
		if (held->Kind == PICKUP_KIND_WHEEL)				// see if try to attach a wheel
			SeeIfAttachGliderWheel(theNode, held);
		else
		if (held->Kind == PICKUP_KIND_PROPELLER)			// see if try to attach the prop
			SeeIfAttachGliderProp(theNode, held);
	}


			/**********************/
			/* UPDATE ORIENTATION */
			/**********************/

			/* HAS BOTH WHEELS */

	if ((theNode->GliderHasLeftWheel) && (theNode->GliderHasRightWheel))
	{
		theNode->Rot.z = 0;
		theNode->Rot.x = .22;
		theNode->Coord.y = theNode->InitCoord.y - 35.0f;
		UpdateObjectTransforms(theNode);
	}
			/* HAS JUST LEFT WHEEL */

	else
	if (theNode->GliderHasLeftWheel)
	{
		theNode->Rot.z = -.1;
		theNode->Rot.x = .2;
		theNode->Coord.y = theNode->InitCoord.y - 50.0f;
		UpdateObjectTransforms(theNode);
	}
			/* HAS JUST RIGHT WHEEL */

	else
	if (theNode->GliderHasRightWheel)
	{
		theNode->Rot.z = .1;
		theNode->Rot.x = .2;
		theNode->Coord.y = theNode->InitCoord.y - 50.0f;
		UpdateObjectTransforms(theNode);
	}

			/* ORIENT RUBBER BAND */

	OGLMatrix4x4_SetRotate_Z(&rm, rubber->Rot.z);							// spin band
	rm.value[M03] = 0;														// align
	rm.value[M13] = 45;
	rm.value[M23] = -28;
	OGLMatrix4x4_Multiply(&rm, &theNode->BaseTransformMatrix, &rubber->BaseTransformMatrix);
	SetObjectTransformMatrix(rubber);


			/****************/
			/* UPDATE PARTS */
			/****************/

		/* CALC COORDS OF AXEL POINTS */

	OGLPoint3D_TransformArray(gAxels, &theNode->BaseTransformMatrix, axelPts, 3);

	numParts = 0;
	for (i = 0; i < 3; i++)
	{
		if (theNode->Flag[i])							// see if has this part
		{
			ObjNode	*part = gGliderPart[i];				// get part objnode

			numParts++;									// inc part counter

			part->Coord = axelPts[i];
			switch(i)
			{
				case	0:
				case	1:
						part->Rot = theNode->Rot;
						part->Rot.z += PI/2;									// rot wheels 90 degrees into position
						UpdateObjectTransforms(part);
						break;

				case	2:
						part->Rot.x = theNode->Rot.x;
						part->Rot.y = theNode->Rot.y;

						if (numParts == 3)										// if we have all the parts then spin the prop
						{
							part->Speed2D += gFramesPerSecondFrac * 10.0f;			// accelerate prop spin
							if (part->Speed2D > 50.0f)
								part->Speed2D = 50.0f;

							part->Rot.z -= gFramesPerSecondFrac * part->Speed2D;	// spin prop
							rubber->Rot.z = part->Rot.z;

							StartLevelCompletion(5.0f);							// we just won!
							theNode->CType |= CTYPE_LOOKAT;


									/* UPDATE PROP SOUND */

							if (theNode->EffectChannel == -1)
								theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_PROP2, &part->Coord, NORMAL_CHANNEL_RATE, 1.0);
							else
								Update3DSoundChannel(EFFECT_PROP2, &theNode->EffectChannel, &part->Coord);
						}

						OGLMatrix4x4_SetRotate_Z(&rm, part->Rot.z);				// set z-rot
						rm.value[M03] = gAxels[2].x;
						rm.value[M13] = gAxels[2].y;
						rm.value[M23] = gAxels[2].z;
						OGLMatrix4x4_Multiply(&rm, &theNode->BaseTransformMatrix, &part->BaseTransformMatrix);
						SetObjectTransformMatrix(part);

						break;

			}
		}
	}

}


/********************** SEE IF ATTACH GLIDER WHEEL **************************/

static void SeeIfAttachGliderWheel(ObjNode *glider, ObjNode *wheel)
{
OGLPoint3D		wheelPts[2];
float			d;


			/* CALC COORDS OF WHEEL AXEL POINTS */

	OGLPoint3D_TransformArray(gAxels, &glider->BaseTransformMatrix, wheelPts, 2);

				/* SEE IF ATTACH LEFT WHEEL */

	if (!glider->GliderHasLeftWheel)
	{
		d = OGLPoint3D_Distance(&wheelPts[0], &wheel->Coord);
		if (d < 100.0f)
		{
			wheel->MoveCall = nil;
			wheel->CType = 0;

			gPlayerInfo.heldObject = nil;						// player isn't holding anything now
			glider->GliderHasLeftWheel = true;

			gGliderPart[0] = wheel;
		}
	}


				/* SEE IF ATTACH RIGHT WHEEL */

	if (!glider->GliderHasRightWheel)
	{
		d = OGLPoint3D_Distance(&wheelPts[1], &wheel->Coord);
		if (d < 100.0f)
		{
			wheel->MoveCall = nil;
			wheel->CType = 0;

			gPlayerInfo.heldObject = nil;						// player isn't holding anything now
			glider->GliderHasRightWheel = true;

			gGliderPart[1] = wheel;
		}

	}




}


/********************** SEE IF ATTACH GLIDER PROP **************************/

static void SeeIfAttachGliderProp(ObjNode *glider, ObjNode *prop)
{
OGLPoint3D		pt;
float			d;


			/* CALC COORDS OF PROP AXEL POINT */

	OGLPoint3D_Transform(&gAxels[2], &glider->BaseTransformMatrix, &pt);


				/* SEE IF ATTACH PROP */

	if (!glider->GliderHasProp)
	{
		d = OGLPoint3D_Distance(&pt, &prop->Coord);
		if (d < 90.0f)
		{
			prop->MoveCall = nil;
			prop->CType = 0;

			gPlayerInfo.heldObject = nil;						// player isn't holding anything now
			glider->GliderHasProp = true;

			gGliderPart[2] = prop;
		}
	}

}








