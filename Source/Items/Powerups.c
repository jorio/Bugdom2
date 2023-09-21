/****************************/
/*   	POWERUPS.C		    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gGravity,gPlayerBottomOff,gGlobalTransparency;
extern	OGLPoint3D			gCoord;
extern	OGLVector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLBoundingBox 		gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	OGLSetupOutputType	*gGameViewInfoPtr;
extern	uint32_t				gAutoFadeStatusBits;
extern	Boolean				gG4, gIgnoreBottleKeySnail;
extern	PlayerInfoType		gPlayerInfo;
extern	int					gLevelNum;
extern	SparkleType	gSparkles[MAX_SPARKLES];
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	float				gCameraDistFromMe,gCameraLookAtYOff;
extern	short				gBestCheckpointNum;
extern	OGLPoint2D			gBestCheckpointCoord;
extern	float				gBestCheckpointAim, gDragonflyY;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveButterfly(ObjNode *body);
static Boolean DoTrig_Butterfly(ObjNode *trigger, ObjNode *who, Byte sideBits);
static void MovePowerupVanish(ObjNode *pow);
static void MoveCheckpoint(ObjNode *theNode);



/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

#define	FlapIndex	SpecialF[0]
#define	HoverWobble	SpecialF[1]			// small local wobble
#define	Metabolism	SpecialF[2]
#define	FlyWobble	SpecialF[3]			// larger up/down wobble in butterfly flight

#define	SpinRadius	SpecialF[0]
#define	SpinRot		SpecialF[1]

#define	POWKind		Special[0]
#define	Regenerate	Flag[0]


/************************* ADD BUTTERFLY *********************************/

Boolean AddButterfly(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*body,*left,*right;
float	yOff;
Boolean	upHigh = itemPtr->parm[3] & (1<<1);
int		powKind = itemPtr->parm[0];


	if (powKind > 12)		// verify
		return(true);

	if (upHigh)								// see if put up high (for garbage level)
		yOff = 1100.0f;
	else
		yOff = 150.0f;

			/*************/
			/* MAKE BODY */
			/*************/

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_ButterflyBody;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + yOff;
	if (upHigh)
		gNewObjectDefinition.flags 	= STATUS_BIT_NOTEXTUREWRAP;			// don't do depth fade since they're up so high they're hard to see that way
	else
		gNewObjectDefinition.flags 	= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 500;
	gNewObjectDefinition.moveCall 	= MoveButterfly;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	gNewObjectDefinition.scale 		= .6f;
	body = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	body->TerrainItemPtr = itemPtr;								// keep ptr to item list

	body->POWKind = powKind;									// save POW kind
	body->Regenerate = itemPtr->parm[3] & 1;					// see if regenerating kind

	if (body->POWKind == POW_KIND_BUDDYBUG)						// buddy bugs always regnerate
		body->Regenerate = true;

	body->FlapIndex 	= 0;
	body->HoverWobble 	= RandomFloat()*PI2;
	body->FlyWobble 	= RandomFloat()*PI2;
	body->Metabolism 	= .9f + RandomFloat() * .3f;				// random speed of animation for this guy

			/* SET COLLISION STUFF */

	body->CType 		= CTYPE_MISC|CTYPE_TRIGGER|CTYPE_BLOCKSHADOW;
	body->CBits			= CBITS_ALWAYSTRIGGER;
	SetObjectCollisionBounds(body,30,-30,-80,80,80,-80);

	body->TriggerCallback = DoTrig_Butterfly;


			/**************/
			/* MAKE WINGS */
			/**************/

	gNewObjectDefinition.type 		= GLOBAL_ObjType_ButterflyLeftWing;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_DOUBLESIDED | STATUS_BIT_ROTZXY |
									STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_NOLIGHTING | STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	left = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	left->ColorFilter.a = .9;
	body->ChainNode = left;


	gNewObjectDefinition.type 		= GLOBAL_ObjType_ButterflyRightWing;
	gNewObjectDefinition.slot++;
	right = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	right->ColorFilter.a = .9;
	left->ChainNode = right;


			/* MAKE SHADOW */

	AttachShadowToObject(body, 0, 6,6, false);

	return(true);
}



/********************** MOVE BUTTERFLY ****************************/

static void MoveButterfly(ObjNode *body)
{
float	meta,sinFlap,y;
float	fps = gFramesPerSecondFrac;
ObjNode	*left = body->ChainNode;
ObjNode	*right = left->ChainNode;
ObjNode	*shadowObj = body->ShadowNode;
OGLPoint3D offOut[2];
static const OGLPoint3D off[2] =
{
	-5,0,0,
	5,0,0
};

	if (TrackTerrainItem(body))							// just check to see if it's gone
	{
		DeleteObject(body);
		return;
	}

	GetObjectInfo(body);

	meta = body->Metabolism * fps;

			/* DO WOBBLES */

	if (gLevelNum == LEVEL_NUM_BALSA)									// special for Balsa level
	{
		gCoord.y = gDragonflyY;
	}
	else
	{
		body->FlyWobble += meta * 0.9f;										// large up/down wobble
		y = body->InitCoord.y + (1.0f + sin(body->FlyWobble)) * 170.0f;

		body->HoverWobble += meta * 10.0f;									// small local jitter wobble
		y += sin(body->HoverWobble) * 15.0f;

		gCoord.y = y;
	}

	body->Rot.y += fps * 1.1f;
	left->Rot.y = right->Rot.y = body->Rot.y;


			/* UPDATE FLAP ROTATION */

	body->FlapIndex += meta * 20.0f;
	if (body->FlapIndex > PI2)
		body->FlapIndex -= PI2;

	sinFlap = sin(body->FlapIndex);
	left->Rot.z = sinFlap * .85f;
	right->Rot.z = -left->Rot.z;

			/* POSITION THE WINGS */

	OGLPoint3D_TransformArray(off, &body->BaseTransformMatrix, offOut, 2);
	left->Coord = offOut[0];
	right->Coord = offOut[1];


			/* MAKE SHADOW FLUTTER */

	if (shadowObj)
	{
		shadowObj->ShadowScaleX = shadowObj->ShadowScaleZ + (sinFlap * 1.5f);
	}


			/* UPDATE OBJECTS */

	UpdateObjectTransforms(left);
	UpdateObjectTransforms(right);

	UpdateObject(body);
}



/************** DO TRIGGER - BUTTERFLY ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Butterfly(ObjNode *trigger, ObjNode *who, Byte sideBits)
{
ObjNode	*pow;

#pragma unused (who, sideBits)

	MakeConfettiExplosion(trigger->Coord.x, trigger->Coord.y, trigger->Coord.z,
							100, 1.0, PARTICLE_SObjType_YwllowDiasyConfetti, 100);

	MakeConfettiExplosion(trigger->Coord.x, trigger->Coord.y, trigger->Coord.z,
							50, 1.0, PARTICLE_SObjType_PurpleDiasyConfetti, 100);


			/* MAKE POWERUP */

	pow = MakePOW(trigger->POWKind, &trigger->Coord);
	if (pow)
	{
		pow->Delta.x = RandomFloat2() * 200.0f;
		pow->Delta.z = RandomFloat2() * 200.0f;
		pow->Delta.y = 300.0f;
	}


	if (gLevelNum == LEVEL_NUM_BALSA)									// instantly get powerup on Balsa level
	{
		PlayEffect(EFFECT_BUTTERFLYBOOM);								// need to play loud on balsa level
		DoTrig_Powerup(pow, nil, 0);
	}
	else
		PlayEffect3D(EFFECT_BUTTERFLYBOOM, &trigger->Coord);




			/* CLEANUP */

	if (!trigger->Regenerate)
		trigger->TerrainItemPtr = nil;			// dont come back

	DeleteObject(trigger);

	return(true);
}

#pragma mark -


/************************* MAKE POWERUP ***************************/

ObjNode *MakePOW(int powKind, OGLPoint3D *where)
{
ObjNode	*pow;
static	short	powToModel[] =
{
	GLOBAL_ObjType_HealthPOW,
	GLOBAL_ObjType_FlightPOW,
	GLOBAL_ObjType_MapPOW,
	GLOBAL_ObjType_FreeLife,					// free life
	GLOBAL_ObjType_RamGrainPOW,
	0,											// buddy bug
	GLOBAL_ObjType_RedKeyPOW,
	GLOBAL_ObjType_GreenKeyPOW,
	GLOBAL_ObjType_BlueKeyPOW,

	GLOBAL_ObjType_GreenCloverPOW,				// green clover
	GLOBAL_ObjType_BlueCloverPOW,				// blue clover
	GLOBAL_ObjType_GoldCloverPOW,				// gold clover

	GLOBAL_ObjType_ShieldPOW,
};

			/* SEE IF MAKE BUDDY BUG */

	if (powKind == POW_KIND_BUDDYBUG)
	{
		CreateMyBuddy(where);
		return(nil);
	}

				/********************/
				/* MAKE GENERAL POW */
				/********************/
	if ((gLevelNum == LEVEL_NUM_CLOSET) && (powKind == POW_KIND_MAP))		// use special paper map on Closet level
	{
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= CLOSET_ObjType_PaperMap;
	}
	else
	{
		gNewObjectDefinition.type 		= powToModel[powKind];
		gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	}

	gNewObjectDefinition.coord		= *where;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= POW_SLOT;
	gNewObjectDefinition.moveCall 	= MovePowerup;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0f;

	switch(powKind)								// set scale of POW item
	{
		case	POW_KIND_HEALTH:
				gNewObjectDefinition.scale 		= 1.6f;
				break;

		case	POW_KIND_GREENCLOVER:
		case	POW_KIND_BLUECLOVER:
		case	POW_KIND_GOLDCLOVER:
				gNewObjectDefinition.flags 		|= STATUS_BIT_DOUBLESIDED;
				gNewObjectDefinition.scale 		= .4f;
				break;

		case	POW_KIND_MAP:
				gNewObjectDefinition.flags 		|= STATUS_BIT_DOUBLESIDED;
				gNewObjectDefinition.scale 		= 3.0f;
				break;

		case	POW_KIND_SHIELD:
				break;

	}

	pow = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	pow->Kind = PICKUP_KIND_POW;			// pickup kind is a POW
	pow->POWKind = powKind;					// also set POW kind


			/* SET COLLISION STUFF */

	pow->CType 		= CTYPE_MISC|CTYPE_PICKUP|CTYPE_KICKABLE;
	pow->CBits		= CBITS_ALWAYSTRIGGER;
	CreateCollisionBoxFromBoundingBox_Maximized(pow);

	pow->TriggerCallback = DoTrig_Powerup;

	switch(powKind)								// set timer before trigger becomes active
	{
		case	POW_KIND_GREENCLOVER:
		case	POW_KIND_BLUECLOVER:
		case	POW_KIND_GOLDCLOVER:
				pow->Timer = .05;					// clovers are instant
				break;

		default:
				pow->Timer = .4f;
	}

	pow->GotKickedCallback = DefaultGotKickedCallback;	// set callback for being kicked


			/* MAKE SHADOW */

	AttachShadowToObject(pow, 0, 1.5,1.5, true);

	return(pow);
}


/********************** MOVE POWERUP *****************************/

void MovePowerup(ObjNode *theNode)
{
			/* MAKE TRIGGER ACTIVE */

	if (theNode->Timer > 0.0f)
	{
		theNode->Timer -= gFramesPerSecondFrac;
		if (theNode->Timer <= 0.0f)
			theNode->CType |= CTYPE_TRIGGER;
	}

			/* DO CUSTOM STUFF */

	switch(theNode->POWKind)
	{
		case	POW_KIND_SHIELD:
				theNode->Rot.y += gFramesPerSecondFrac * 7.0f;
				break;


	}


			/* DO DEFAULT MOVE CALL */

	MoveDefaultPickup(theNode);
}


/************** DO TRIGGER - POWERUP ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_Powerup(ObjNode *pow, ObjNode *who, Byte sideBits)
{
#pragma unused (who, sideBits)

	switch(pow->POWKind)
	{
				/* GET HEALTH */

		case	POW_KIND_HEALTH:
				gPlayerInfo.health += .15f;
				if (gPlayerInfo.health > 1.0f)
					gPlayerInfo.health = 1.0f;
				break;


			/* GET FLIGHT FRUIT */

		case	POW_KIND_FLIGHT:
				gPlayerInfo.glidePower += .2f;
				if (gPlayerInfo.glidePower > 1.0f)
					gPlayerInfo.glidePower = 1.0f;
				break;


				/* GET MAP */

		case	POW_KIND_MAP:
				gPlayerInfo.hasMap = true;
				break;


				/* FREE LIFE */

		case	POW_KIND_FREELIFE:
				gPlayerInfo.lives++;
				break;


				/* GET RAM GRAIN */

		case	POW_KIND_RAMGRAIN:
				SetPlayerRammingAnim(who);
				break;


				/* GET KEY */

		case	POW_KIND_REDKEY:
		case	POW_KIND_GREENKEY:
		case	POW_KIND_BLUEKEY:
				gPlayerInfo.hasKey[pow->POWKind - POW_KIND_REDKEY] = true;

				if (pow->POWKind == POW_KIND_GREENKEY)					// on park level make the snail shut up if we already got the green key
					gIgnoreBottleKeySnail = true;
				break;



				/* CLOVER */

		case	POW_KIND_GREENCLOVER:
				gPlayerInfo.numGreenClovers++;
				break;
		case	POW_KIND_BLUECLOVER:
				gPlayerInfo.numBlueClovers++;
				break;
		case	POW_KIND_GOLDCLOVER:
				gPlayerInfo.numGoldClovers++;
				break;


				/* SHIELD */

		case	POW_KIND_SHIELD:
				gPlayerInfo.shieldTimer = 15.0f;
				break;

	}


	StartPowerupVanish(pow);
	return(true);
}


#pragma mark -

/********************** START POWERUP VANISH **********************/

void StartPowerupVanish(ObjNode *pow)
{
	pow->InitCoord = pow->Coord;							// set central axis

	pow->SpinRadius = 10.0f;
	pow->SpinRot = 0;
	pow->DeltaRot.y = 20.0f;

	pow->MoveCall = MovePowerupVanish;
	pow->StatusBits &= ~STATUS_BIT_NOMOVE;			// make sure it can move
	pow->CType = 0;

	if (pow->ChainHead)								// see if was attached to something
	{
		pow->ChainHead->ChainNode = nil;			// detach from chain
		pow->ChainHead = nil;
	}

	if (gLevelNum == LEVEL_NUM_BALSA)
		PlayEffect(EFFECT_GETPOW);
	else
		PlayEffect3D(EFFECT_GETPOW, &pow->Coord);
}


/******************** MOVE POWERUP VANISH ********************/

static void MovePowerupVanish(ObjNode *pow)
{
float	fps = gFramesPerSecondFrac;
float	r,r2;

	GetObjectInfo(pow);

	r2 = pow->SpinRadius += fps * 30.0f;

	pow->DeltaRot.y += fps * 15.0f;
	r = pow->SpinRot += pow->DeltaRot.y * fps;

	pow->Rot.y -= (pow->DeltaRot.y * fps) * .1f;
	pow->Rot.x -= (pow->DeltaRot.y * fps) * .15f;

	gCoord.x = pow->InitCoord.x + sin(r) * r2;
	gCoord.z = pow->InitCoord.z + cos(r) * r2;
	gCoord.y += 3.0f * (pow->DeltaRot.y * fps);

	pow->ColorFilter.a -= fps * .6f;
	if (pow->ColorFilter.a <= 0.0f)
	{
		DeleteObject(pow);
		return;
	}

	UpdateObject(pow);
}


#pragma mark -


/******************* ADD POW **************************/

Boolean AddPOW(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*pow;
OGLPoint3D	where;

	where.x	= x;
	where.y = FindHighestCollisionAtXZ(x, z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN|CTYPE_WATER) + 30.0f;
	where.z	= z;

	pow = MakePOW(itemPtr->parm[0], &where);
	if (pow)
		pow->TerrainItemPtr = itemPtr;			// keep ptr to item list

	return(true);
}




#pragma mark -

/************************* MAKE CHECKPOINT ***************************/

ObjNode *MakeCheckpoint(OGLPoint3D *where)
{
ObjNode	*newObj;

	gNewObjectDefinition.type 		= SKELETON_TYPE_CHECKPOINT;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord		= *where;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= POW_SLOT;
	gNewObjectDefinition.moveCall 	= MoveCheckpoint;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.5f;
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	newObj->What = WHAT_CHECKPOINT;

			/* SET COLLISION STUFF */

	newObj->CType 		= CTYPE_MISC;
	newObj->CBits		= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj, .9, 1);

	newObj->HoldOffset.x = 0;
	newObj->HoldOffset.y = -12;
	newObj->HoldOffset.z = -10;

			/* MAKE SHADOW */

	AttachShadowToObject(newObj, 0, 2,3, false);

	return(newObj);
}



/********************** SET CHECKPOINT ************************/
//
// Called to set the input checkpoint as the current checkpoint
//

void SetCheckpoint(ObjNode *checkpoint, ObjNode *player)
{
	gBestCheckpointAim = player->Rot.y;
	gBestCheckpointCoord.x = checkpoint->Coord.x;
	gBestCheckpointCoord.y = checkpoint->Coord.z;

}

/********************* MOVE CHECKPOINT **********************/

static void MoveCheckpoint(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	y;

//	if (TrackTerrainItem(theNode))							// just check to see if it's gone
//	{
//		DeleteObject(theNode);
//		return;
//	}

	if (theNode->Skeleton->AnimNum == 0)				// make grow
		SetSkeletonAnim(theNode->Skeleton, 1);


			/* DO SIMPLE GRAVITY BOUNCE */

	GetObjectInfo(theNode);

	gDelta.y -= DEFAULT_GRAVITY * fps;
	gCoord.y += gDelta.y * fps;

	y = GetTerrainY(gCoord.x, gCoord.z);
	if ((gCoord.y + theNode->BottomOff) <= y)
	{
		gCoord.y = y - theNode->BottomOff;
		gDelta.y *= -.4f;
	}
	HandleCollisions(theNode, CTYPE_TERRAIN, .5f);


	UpdateObject(theNode);
}











