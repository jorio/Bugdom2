/****************************/
/*   	TRAPS.C			    */
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

static void MoveSprinklerHead(ObjNode *base);
static void MoveWindmill(ObjNode *base);
static void WindmillBladeHitPlayer(ObjNode *blades, ObjNode *player, ObjNode *rideBall);
static void MoveFirecracker(ObjNode *theNode);
static void MoveMouseTrap(ObjNode *trap);
static void MouseTrapGotKickedCallback(ObjNode *player, ObjNode *trap);
static void SetOffMouseTrap(ObjNode *trap);
static void MoveMouse_Freed(ObjNode *mouse);
static Boolean DoTrig_MouseTrap(ObjNode *trap, ObjNode *who, Byte sideBits);
static Boolean DoTrig_Trampoline(ObjNode *theNode, ObjNode *who, Byte sideBits);
static void MoveTrampoline(ObjNode *theNode);
static void MoveVacuumeOnSpline(ObjNode *theNode);
static void SeeIfVacuumeSuckPlayer(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	WINDMILL_SCALE	2.0f
#define	MOUSETRAP_SCALE	3.5f

enum
{
	MOUSETRAP_ANIM_SNAPPED = 0,
	MOUSETRAP_ANIM_PRIMED,
	MOUSETRAP_ANIM_THWACK,
	MOUSETRAP_ANIM_LIFT
};

#define	MOUSE_WALK_SPEED	600.0f


enum
{
	MOUSE_ANIM_STUCK = 0,
	MOUSE_ANIM_RUNAWAY,
	MOUSE_ANIM_STUCK2
};

/*********************/
/*    VARIABLES      */
/*********************/

int		gNumDrowningMiceRescued, gNumDrowingMiceToRescue, gNumMice;

ObjNode	*gSuckingVacuume = nil;

Byte	gSprinklerMode;
float	gSprinklerPopUpOffset,gSprinklerTimer;

#define	TrampAmplitude		SpecialF[0]
#define	TrampWobbleIndex	SpecialF[1]
#define TrampDecaySpeed		SpecialF[2]

#define	Drowning			Flag[0]


/********************** INIT SPRINKLER HEADS ***********************/

void InitSprinklerHeads(void)
{
	gSprinklerMode = SPRINKLER_MODE_OFF;
	gSprinklerPopUpOffset = gSprinklerTimer = 0;

}


/************************* ADD SPRINKLER HEAD *********************************/

Boolean AddSprinklerHead(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*base, *head;
int	headType;
			/*************/
			/* MAKE BASE */
			/*************/

	switch(gLevelNum)
	{
		case	LEVEL_NUM_GNOMEGARDEN:
				gNewObjectDefinition.type 	= GARDEN_ObjType_SprinklerBase;
				headType 					= GARDEN_ObjType_SprinklerPost;
				break;

		case	LEVEL_NUM_SIDEWALK:
				gNewObjectDefinition.type 	= SIDEWALK_ObjType_SprinklerBase;
				headType 					= SIDEWALK_ObjType_SprinklerPost;
				break;

		default:
				return(true);
	}

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.scale 		= 2.5;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x, z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveSprinklerHead;
	gNewObjectDefinition.rot 		= itemPtr->parm[0] * (PI/2);
	base = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	base->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	base->CType 		= CTYPE_MISC;
	base->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(base,.8,1);


			/*************/
			/* MAKE HEAD */
			/*************/

	gNewObjectDefinition.type 		= headType;
	gNewObjectDefinition.moveCall 	= nil;
	head = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	base->ChainNode = head;

	return(true);													// item was added
}


/************************ MOVE SPRINKLER HEAD **************************/

static void MoveSprinklerHead(ObjNode *base)
{
ObjNode	*head = base->ChainNode;
ObjNode	*spray = head->ChainNode;

			/* SEE IF THIS ONE IS GONE */

	if (TrackTerrainItem(base))
	{
		DeleteObject(base);
		return;
	}


			/* SET HEAD'S POSITION */

	head->Coord.y = head->InitCoord.y + gSprinklerPopUpOffset;
	UpdateObjectTransforms(head);


			/* SEE IF NEED TO ACTIVATE */

	if (gSprinklerMode == SPRINKLER_MODE_OFF)
	{
		if (CalcQuickDistance(head->Coord.x, head->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 1500.0f)
		{
			gSprinklerMode = SPRINKLER_MODE_UP;
			gSprinklerTimer = 6.0f;
		}
	}

			/***********************/
			/* SEE IF UPDATE SPRAY */
			/***********************/

	if (gSprinklerMode == SPRINKLER_MODE_ON)
	{

				/* MAKE SPRAY */

		if (spray == nil)
		{
			switch(gLevelNum)
			{
				case	LEVEL_NUM_GNOMEGARDEN:
						gNewObjectDefinition.type 	= GARDEN_ObjType_SprinklerSpray;
						break;

				case	LEVEL_NUM_SIDEWALK:
						gNewObjectDefinition.type 	= SIDEWALK_ObjType_SprinklerSpray;
						break;
			}

			gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
			gNewObjectDefinition.coord	 	= head->Coord;
			gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_DOUBLESIDED | STATUS_BIT_NOZWRITES |
											STATUS_BIT_NOLIGHTING;
			gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
			gNewObjectDefinition.moveCall 	= nil;
			gNewObjectDefinition.rot 		= 0;
			gNewObjectDefinition.scale 		= head->Scale.x;
			spray = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			head->ChainNode = spray;
		}

		spray->Rot.y = RandomFloat()*PI2;

		UpdateObjectTransforms(spray);

				/* SOUND */

		if (base->EffectChannel == -1)
			base->EffectChannel = PlayEffect_Parms3D(EFFECT_SPRINKLER, &base->Coord, NORMAL_CHANNEL_RATE + (MyRandomLong()&0x3fff), 1.0);
		else
			Update3DSoundChannel(EFFECT_SPRINKLER, &base->EffectChannel, &base->Coord);
	}

				/* IF NOT ON, THEN MAKE SURE SPRAY IS GONE */

	else
	{
		if (spray)
		{
			DeleteObject(spray);
			spray = nil;
			head->ChainNode = nil;
		}

		StopAChannel(&base->EffectChannel);
	}

}


/********************** UPDATE SPRINKLER HEADS ************************/

void UpdateSprinklerHeads(void)
{

	switch(gSprinklerMode)
	{
				/* MOVE UP */

		case	SPRINKLER_MODE_UP:
				gSprinklerPopUpOffset += gFramesPerSecondFrac * 700.0f;
				if (gSprinklerPopUpOffset >= 240.0f)						// see if all the way up
				{
					gSprinklerPopUpOffset = 240.0f;
					gSprinklerMode = SPRINKLER_MODE_ON;
					StartRainEffect();
				}
				break;


				/* IT'S ON */

		case	SPRINKLER_MODE_ON:
				gSprinklerTimer -= gFramesPerSecondFrac;
				if (gSprinklerTimer <= 0.0f)
				{
					gSprinklerMode = SPRINKLER_MODE_DOWN;
					StopRainEffect();
				}
				break;


				/* MOVE DOWN */

		case	SPRINKLER_MODE_DOWN:
				gSprinklerPopUpOffset -= gFramesPerSecondFrac * 400.0f;
				if (gSprinklerPopUpOffset <= 0.0f)
				{
					gSprinklerPopUpOffset = 0.0f;
					gSprinklerMode = SPRINKLER_MODE_OFF;
				}
				break;
	}

}


#pragma mark -



/************************* ADD WINDMILL *********************************/

Boolean AddWindmill(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*base, *blades;
float	r = itemPtr->parm[0] * (PI2/4.0f);
static const OGLPoint3D bladeOff = {0,1050.0f * WINDMILL_SCALE,-560.0f * WINDMILL_SCALE};
OGLMatrix4x4	m;

			/*************/
			/* MAKE BASE */
			/*************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SIDEWALK_ObjType_WindmillBase;
	gNewObjectDefinition.scale 		= WINDMILL_SCALE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x, z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale);
	gNewObjectDefinition.flags 		= 0; //gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 199;
	gNewObjectDefinition.moveCall 	= MoveWindmill;
	gNewObjectDefinition.rot 		= r;
	base = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	base->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	base->CType 		= CTYPE_MISC;
	base->CBits			= CBITS_ALLSOLID;

	if (itemPtr->parm[0] & 1)									// build 2 collision boxes
	{
		AddCollisionBoxToObject(base, 2000, 0, -460.0f * WINDMILL_SCALE, 460.0f * WINDMILL_SCALE,		// near
								530.0f * WINDMILL_SCALE, 230.0f * WINDMILL_SCALE);

		AddCollisionBoxToObject(base, 2000, 0, -460.0f * WINDMILL_SCALE, 460.0f * WINDMILL_SCALE,		// far
								-230.0f * WINDMILL_SCALE, -530.0f * WINDMILL_SCALE);
	}
	else
	{
		AddCollisionBoxToObject(base, 2000, 0, -530.0f * WINDMILL_SCALE, -230.0f * WINDMILL_SCALE,		// left
								460.0f * WINDMILL_SCALE, -460.0f * WINDMILL_SCALE);

		AddCollisionBoxToObject(base, 2000, 0, 230.0f * WINDMILL_SCALE, 530.0f * WINDMILL_SCALE,		// right
								460.0f * WINDMILL_SCALE, -460.0f * WINDMILL_SCALE);

	}



			/**************/
			/* MAKE BLADE */
			/**************/

	OGLMatrix4x4_SetRotate_Y(&m, r);
	OGLPoint3D_Transform(&bladeOff, &m, &gNewObjectDefinition.coord);
	gNewObjectDefinition.coord.x	+= base->Coord.x;
	gNewObjectDefinition.coord.y	+= base->Coord.y;
	gNewObjectDefinition.coord.z	+= base->Coord.z;

	gNewObjectDefinition.flags 		= STATUS_BIT_ROTZXY;
	gNewObjectDefinition.type 		= SIDEWALK_ObjType_WindmillBlades;
	gNewObjectDefinition.moveCall 	= nil;
	blades = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	base->ChainNode = blades;


		/*********************/
		/* MAKE BLADE SHADOW */
		/*********************/

	AttachStaticShadowToObject(blades, 0, 25,2.0);



	return(true);													// item was added
}


/********************** MOVE WINDMILL *************************/

static void MoveWindmill(ObjNode *base)
{
ObjNode			*player;
OGLMatrix4x4	tm,rym,rzm,rm,m;
ObjNode	*blades = base->ChainNode;
ObjNode	*shadow = blades->ShadowNode;
float	r;
int		i;
const OGLPoint3D	bladeOffTable[4][2] =						// offsets to each blades endpoints
{
	-180.0f * WINDMILL_SCALE, 960.0f * WINDMILL_SCALE, 0,				// top blade
	170.0f * WINDMILL_SCALE, 1040.0f * WINDMILL_SCALE, 0,

	960.0f * WINDMILL_SCALE, 180.0f * WINDMILL_SCALE, 0,				// right blade
	1040.0f * WINDMILL_SCALE, -170.0f * WINDMILL_SCALE, 0,

	-180.0f * WINDMILL_SCALE, -1040.0f * WINDMILL_SCALE, 0,				// bottom blade
	180.0f * WINDMILL_SCALE, -960.0f * WINDMILL_SCALE, 0,

	-1040.0f * WINDMILL_SCALE, 180.0f * WINDMILL_SCALE, 0,				// left blade
	-970.0f * WINDMILL_SCALE, -180.0f * WINDMILL_SCALE, 0
};

OGLPoint3D	bladePts[4][2];



			/* SEE IF THIS ONE IS GONE */

	if (TrackTerrainItem(base))
	{
		DeleteObject(base);
		return;
	}


			/* SPIN BLADES */

	r = blades->Rot.z += gFramesPerSecondFrac * 1.5f;
	UpdateObjectTransforms(blades);


			/* UPDATE SHADOW */

	shadow->ColorFilter.a = cos(r * 4.0f);


			/***************************/
			/* SEE IF BLADE HIT PLAYER */
			/***************************/

			/* CALC ENDPOINTS OF TIP OF BLADES */

	OGLMatrix4x4_SetTranslate(&tm, blades->Coord.x, blades->Coord.y, blades->Coord.z);
	OGLMatrix4x4_SetRotate_Y(&rym, blades->Rot.y);
	OGLMatrix4x4_SetRotate_Z(&rzm, blades->Rot.z);
	OGLMatrix4x4_Multiply(&rzm, &rym, &rm);
	OGLMatrix4x4_Multiply(&rm, &tm, &m);

	OGLPoint3D_TransformArray(&bladeOffTable[0][0], &m, &bladePts[0][0], 8);

	player = gPlayerInfo.objNode;

	for (i = 0; i < 4; i++)								// check all 4 blades
	{
			/* SEE IF ANY BLADES HIT PLAYER */

		if (SeeIfLineSegmentHitsObject(&bladePts[i][0], &bladePts[i][1], player))
		{
			WindmillBladeHitPlayer(blades, player, gPlayerInfo.ridingBall);
			break;
		}

			/* SEE IF ANY BLADES HIT BALL */

		else
		{
			ObjNode *ball = SeeIfLineSegmentHitsWhat(&bladePts[i][0], &bladePts[i][1], WHAT_RIDEBALL);	// see if hit any ball at all, regardless if we're riding or not

			if (ball)
			{
				if (ball == gPlayerInfo.ridingBall)							// see if hit ball we're riding
					WindmillBladeHitPlayer(blades, player, ball);
				else														// hit a lone ball w/ no rider
					WindmillBladeHitPlayer(blades, nil, ball);
			}
		}
	}

}


/************************* WINDMILL BLADES HIT PLAYER ***************************/

static void WindmillBladeHitPlayer(ObjNode *blades, ObjNode *player, ObjNode *rideBall)
{
const OGLPoint3D	whackVector = {1000, 800, -200};
OGLMatrix4x4		m;
OGLPoint3D			v;

			/* TRANSFORM THE WHACK VECTOR */

	OGLMatrix4x4_SetRotate_Y(&m, blades->Rot.y);
	OGLPoint3D_Transform(&whackVector, &m, &v);


				/* WHACK PLAYER */

	if (player)
	{
		player->Delta.x = v.x;
		player->Delta.y = v.y;
		player->Delta.z = v.z;

		PlayerGotHit(blades, .4, PLAYER_ANIM_GOTHIT_BACKFLIP);
		PlayEffect3D(EFFECT_SMACK, &player->Coord);
		gCurrentMaxSpeed = 1000;
	}


			/* ALSO WHACK RIDING BALL */

	if (rideBall)
	{
		rideBall->Speed2D = 700.0f;
		rideBall->Rot.y = CalcYAngleFromPointToPoint(rideBall->Rot.y, rideBall->Coord.x, rideBall->Coord.z,
													rideBall->Coord.x + v.x, rideBall->Coord.z + v.z);

	}

}


#pragma mark -


/********************* ADD FIRECRACKER *************************/

Boolean AddFirecracker(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Firecracker;
	gNewObjectDefinition.scale 		= 1.1;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x, z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 358;
	gNewObjectDefinition.moveCall 	= MoveFirecracker;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	return(true);
}


/************************ MOVE FIRECRACKER ************************/

static void MoveFirecracker(ObjNode *theNode)
{
int		i;
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;
float				dist;

			/* SEE IF THIS ONE IS GONE */

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}


			/*********************/
			/* MAKE FUSE SPARKLE */
			/*********************/

	if (gFramesPerSecond > 15.0f)										// only do if running at good frame rate
	{
		theNode->Timer -= fps;											// see if add spark
		if (theNode->Timer <= 0.0f)
		{
			theNode->Timer += .1f;										// reset timer

			particleGroup 	= theNode->ParticleGroup;
			magicNum 		= theNode->ParticleMagicNum;

			if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
			{

				theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

				groupDef.magicNum				= magicNum;
				groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
				groupDef.flags					= PARTICLE_FLAGS_BOUNCE | PARTICLE_FLAGS_ALLAIM;
				groupDef.gravity				= 300;
				groupDef.magnetism				= 0;
				groupDef.baseScale				= 20.0f;
				groupDef.decayRate				= 0;
				groupDef.fadeRate				= .6f;
				groupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark4;
				groupDef.srcBlend				= GL_SRC_ALPHA;
				groupDef.dstBlend				= GL_ONE;
				theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
			}

			if (particleGroup != -1)
			{
				static const OGLPoint3D tipOff = {16, 52, -164};

				OGLPoint3D_Transform(&tipOff, &theNode->BaseTransformMatrix, &p);

				for (i = 0; i < 2; i++)
				{
					d.x = RandomFloat2() * 60.0f;
					d.y = RandomFloat2() * 60.0f;
					d.z = RandomFloat2() * 60.0f;

					newParticleDef.groupNum		= particleGroup;
					newParticleDef.where		= &p;
					newParticleDef.delta		= &d;
					newParticleDef.scale		= 1.0f + RandomFloat() * .6f;
					newParticleDef.rotZ			= 0;
					newParticleDef.rotDZ		= 0;
					newParticleDef.alpha		= .6;
					if (AddParticleToGroup(&newParticleDef))
					{
						theNode->ParticleGroup = -1;
						break;
					}
				}
			}
		}
	}


			/******************/
			/* SEE IF EXPLODE */
			/******************/

	dist = OGLPoint3D_Distance(&theNode->Coord, &gPlayerInfo.coord);		// calc dist from firecracker to player

	if (gLevelNum == LEVEL_NUM_SIDEWALK)									// range on this level is wide
		dist -= 300.0f;
	else
		dist -= 200.0f;

	if (dist < 0.0f)
	{
		ObjNode	*player = gPlayerInfo.objNode;
		float	x = theNode->Coord.x;
		float	y = theNode->Coord.y + 15.0f;
		float	z = theNode->Coord.z;
		OGLVector3D	v;

					/* MAKE GO BOOM */

		MakeSparkExplosion(x, y, z, 600, 1.0f, PARTICLE_SObjType_WhiteSpark4, 150, 1.0);
		MakeSparkExplosion(x, y, z, 400, 1.0f, PARTICLE_SObjType_RedSpark, 150, .5);

		ExplodeGeometry(theNode, 600, SHARD_MODE_BOUNCE | SHARD_MODE_FROMORIGIN, 1, 2.0);

		PlayEffect3D(EFFECT_FIRECRACKER, &theNode->Coord);
		DeleteObject(theNode);


					/* WHACK RIDE BALL */

		v.x = player->Coord.x - x;
		v.y = player->Coord.y - y;
		v.z = player->Coord.z - z;
		FastNormalizeVector(v.x, v.y, v.z, &v);

		if (gPlayerInfo.ridingBall)
		{
			ObjNode	*ball = gPlayerInfo.ridingBall;

			ball->Rot.y = CalcYAngleFromPointToPoint(ball->Rot.y, ball->Coord.x, ball->Coord.z,
													ball->Coord.x + v.x, ball->Coord.z + v.z);

			ball->Speed2D = 300.0f;
		}


					/* WHACK PLAYER */

		player->Delta.x = v.x * 1200.0f;
		player->Delta.y = v.y * 1200.0f;
		player->Delta.z = v.z * 1200.0f;

		gCurrentMaxSpeed = 1000;

		PlayerGotHit(nil, .3, PLAYER_ANIM_GOTHIT_BACKFLIP);


		return;

	}

}


#pragma mark -



/**************************** COUNT MICE *********************************/

void CountMice(void)
{
int						i;
TerrainItemEntryType 	*itemPtr;

	gNumDrowningMiceRescued = gNumDrowingMiceToRescue = gNumMice = 0;


	itemPtr = *gMasterItemList; 											// get pointer to data inside the LOCKED handle

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_MOUSETRAP)							// is mouse trap?
		{
			if (!(itemPtr[i].parm[3] & 1))									// is not primed?
			{
				gNumMice++;
				if (itemPtr[i].parm[3] & (1<<1))							// is drowning?
					gNumDrowingMiceToRescue++;
			}
		}
	}
}



/********************* ADD MOUSETRAP *************************/

Boolean AddMouseTrap(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*trap, *mouse;
int		anim;
Boolean	primed = itemPtr->parm[3] & 0x1;
Boolean	drowning = itemPtr->parm[3] & (1<<1);
float	r,y;
				/****************************/
				/* MAKE MOUSETRAP MECHANISM */
				/****************************/

	if (primed)
		anim = MOUSETRAP_ANIM_PRIMED;
	else
		anim = MOUSETRAP_ANIM_SNAPPED;

	gNewObjectDefinition.type 		= SKELETON_TYPE_MOUSETRAP;
	gNewObjectDefinition.animNum	= anim;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= y = GetTerrainY(x, z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= POW_SLOT-1;					// make sure before POW so that bait will chain correctly
	gNewObjectDefinition.moveCall	= MoveMouseTrap;
	gNewObjectDefinition.rot 		= r = (float)itemPtr->parm[0] * (PI / 2);
	gNewObjectDefinition.scale 		= MOUSETRAP_SCALE;

	trap = MakeNewSkeletonObject(&gNewObjectDefinition);
	trap->TerrainItemPtr = itemPtr;								// keep ptr to item list

	trap->What = WHAT_MOUSETRAP;

	trap->Drowning = drowning;

			/* SET COLLISION */

	trap->Damage 		= .25;
	trap->CType 		= CTYPE_MISC | CTYPE_BLOCKSHADOW;
	trap->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(trap,1,.2);
	trap->BottomOff = -300;
	CalcObjectBoxFromNode(trap);

	if (primed)													// primed traps are triggers
	{
		trap->CType |= CTYPE_TRIGGER | CTYPE_KICKABLE;
		trap->TriggerCallback = DoTrig_MouseTrap;
		trap->GotKickedCallback = MouseTrapGotKickedCallback;			// set callback for being kicked
	}


				/********************/
				/* PUT BAIT ON TRAP */
				/********************/

	if (primed)
	{
		ObjNode	 	*pow;
		OGLPoint3D	p;

		p.x = x + sin(r) * 150.0f;
		p.z = z + cos(r) * 150.0f;
		p.y = y + trap->TopOff;

		pow = MakePOW(POW_KIND_HEALTH, &p);							// make POW
		pow->StatusBits |= STATUS_BIT_NOMOVE;						// dont move on it's own yet
		pow->CType |= CTYPE_TRIGGER;								// make the trigger active NOW
		pow->CType &= ~CTYPE_PICKUP;								// this isn't pickupable

		pow->Coord.y -= pow->BottomOff;								// move to be on top of trap
		UpdateObjectTransforms(pow);
		UpdateShadow(pow);
		CalcObjectBoxFromNode(pow);

		trap->ChainNode = pow;										// chain it all
		pow->ChainHead = trap;
	}

				/**********************/
				/* MAKE MOUSE IN TRAP */
				/**********************/

	else
	{
		if (itemPtr->flags & ITEM_FLAGS_USER1)						// see if this mouse has already been freed
			return(true);

		gNewObjectDefinition.type 		= SKELETON_TYPE_MOUSE;
		gNewObjectDefinition.animNum	= MOUSE_ANIM_STUCK;
		gNewObjectDefinition.coord.x 	= x + sin(r) * 310.0f;
		gNewObjectDefinition.coord.z 	= z + cos(r) * 310.0f;
		gNewObjectDefinition.coord.y 	= y;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
		gNewObjectDefinition.slot++;
		gNewObjectDefinition.moveCall	= nil;
		gNewObjectDefinition.rot 		= r + PI;
		gNewObjectDefinition.scale 		= 2.0;

		mouse = MakeNewSkeletonObject(&gNewObjectDefinition);

		mouse->Coord.y -= mouse->BBox.min.y;						// offset so bottom touches ground
		UpdateObjectTransforms(mouse);

		trap->ChainNode = mouse;

		mouse->Timer = 2.0f + RandomFloat() * 2.0f;					// set random time to swich stuck anims

				/* SET COLLISION */

		mouse->CType 		= CTYPE_MISC | CTYPE_BLOCKCAMERA;
		mouse->CBits		= CBITS_ALLSOLID;
		SetObjectCollisionBounds(mouse, 80, -90, -60, 60, 60, -60);

				/* MAKE SHADOW */

		AttachShadowToObject(mouse, SHADOW_TYPE_CIRCULAR, 5, 5,false);
		UpdateShadow(mouse);
	}


	return(true);
}


/************** DO TRIGGER - MOUSE TRAP ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_MouseTrap(ObjNode *trap, ObjNode *who, Byte sideBits)
{
#pragma unused (who)

	if (!(sideBits & SIDE_BITS_BOTTOM))					// only trigger if land on top
		return(true);

				/* SET OFF THE TRAP */

	SetOffMouseTrap(trap);


				/* ASSUME PLAYER WILL BE HIT */

	PlayerGotHit(trap, 0, PLAYER_ANIM_GOTHIT_BACKFLIP);	// hurt player

	return(true);
}


/************************* MOUSETRAP GOT KICKED CALLBACK *************************/

static void MouseTrapGotKickedCallback(ObjNode *player, ObjNode *trap)
{
#pragma unused (player)

	SetOffMouseTrap(trap);
}


/******************** SET OFF MOUSE TRAP **********************/

static void SetOffMouseTrap(ObjNode *trap)
{
	SetSkeletonAnim(trap->Skeleton, MOUSETRAP_ANIM_THWACK);
	PlayEffect_Parms3D(EFFECT_MOUSETRAP, &trap->Coord, NORMAL_CHANNEL_RATE, 2.0f);

	trap->CType &= ~(CTYPE_TRIGGER|CTYPE_KICKABLE);		// dont ever trigger again
	trap->TriggerCallback = nil;
	trap->GotKickedCallback = nil;
}



/************************* MOVE MOUSE TRAP ************************/

static void MoveMouseTrap(ObjNode *trap)
{
ObjNode		*mouse;
int			a;

	if (TrackTerrainItem(trap))							// just check to see if it's gone
	{
		DeleteObject(trap);
		return;
	}

	switch(trap->Skeleton->AnimNum)
	{
		case	MOUSETRAP_ANIM_SNAPPED:
				mouse = trap->ChainNode;
				if (mouse)
				{
					mouse->Timer -= gFramesPerSecondFrac;				// see if change anims
					if (mouse->Timer <= 0.0f)
					{
						if (mouse->Skeleton->AnimNum == MOUSE_ANIM_STUCK2)
							a = MOUSE_ANIM_STUCK;
						else
							a = MOUSE_ANIM_STUCK2;

						MorphToSkeletonAnim(mouse->Skeleton, a, 4);

						mouse->Timer = 2.0f + RandomFloat() * 4.0f;		// set random time to swich stuck anims
					}
				}
				break;


	}
}


/************************* SEE IF LIFT MOUSETRAP LEVER *************************/
//
// Called from MovePlayer_Pickup() to see if player should lift a mousetrap level during
// the player's pickup anim.
//

void SeeIfLiftMousetrapLever(ObjNode *player)
{
int		i, j;
ObjNode	*thisNode, *mouse;
float	r, ix, iz;
OGLPoint3D	edge[4];
const OGLPoint3D corners[4] = 									// offsets to corners on snapping edge of mousetrap
{
	-33, 7, 0,
	-33, 7, 60,
	33, 7, 60,
	33, 7, 0,
};
OGLPoint2D	pl[4];

	if (!(player->StatusBits & STATUS_BIT_ONGROUND))			// only bother if player is on ground
		return;


				/***************************/
				/* SCAN FOR ALL MOUSETRAPS */
				/***************************/

	thisNode = gFirstNodePtr;										// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)							// see if reach end of usable list
			break;

		if (thisNode->What != WHAT_MOUSETRAP)						// only look for mousetraps
			goto next;

		if (thisNode->Skeleton->AnimNum	!= MOUSETRAP_ANIM_SNAPPED)	// only if mouse in it
			goto next;

		mouse = thisNode->ChainNode;								// only if possible mouse here
		if (mouse == nil)
			goto next;


					/* CALC THE SNAPPING-EDGE LINE SEGMENT */

		OGLPoint3D_TransformArray(corners, &thisNode->BaseTransformMatrix, edge, 4);	// transform corners


					/* CALC A LINE SEG IN FRONT OF PLAYER */

		pl[0].x = gCoord.x;											// player coord
		pl[0].y = gCoord.z;

		r = player->Rot.y;
		pl[1].x = gCoord.x - sin(r) * 60.0f;						// coord in front of player
		pl[1].y = gCoord.z - cos(r) * 60.0f;


					/* CALC PLAYER'S CROSS SEG (PERP TO ABOVE) */

		r += PI/2;
		pl[2].x = pl[1].x - sin(r) * 40.0f;						// coord on left
		pl[2].y = pl[1].y - cos(r) * 40.0f;

		pl[3].x = pl[1].x + sin(r) * 40.0f;						// coord on right
		pl[3].y = pl[1].y + cos(r) * 40.0f;



					/* SEE IF LINES INTERSECT */

		for (i = 0; i < 3; i++)															// check all 3 trap line segments
		{
			for (j = 0; j < 4; j += 2)												// check both player lines
			{
				if (IntersectLineSegments(edge[i].x, edge[i].z, edge[i+1].x, edge[i+1].z,	// if intersect, then lift this
					                     pl[j].x, pl[j].y, pl[j+1].x, pl[j+1].y, &ix, &iz))
			  	{
					thisNode->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;			// set user flag so we'll always know this mouse was freed
			  		thisNode->ChainNode = nil;										// detach mouse from trap
			  		mouse->MoveCall = MoveMouse_Freed;								// set move call
			  		mouse->Speed2D = 200.0f;										// give some initial speed
					SetSkeletonAnim(thisNode->Skeleton, MOUSETRAP_ANIM_LIFT);
					MorphToSkeletonAnim(mouse->Skeleton, 1, 10);

			  		gPlayerInfo.numMiceRescued++;
			  		if (thisNode->Drowning)											// was that a special garbage can drowning mouse?
			  			gNumDrowningMiceRescued++;
					return;
			  	}
			  }
		}

				/* TRY NEXT TRAP */

next:
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);
}


/*************************** MOVE MOUSE: FREED ***************************/

static void MoveMouse_Freed(ObjNode *mouse)
{
float	fps = gFramesPerSecondFrac;
float	r,s;

	if (TrackTerrainItem(mouse) || (mouse->StatusBits & STATUS_BIT_ISCULLED))	// just check to see if it's gone
	{
		DeleteObject(mouse);
		return;
	}

	GetObjectInfo(mouse);

			/* MOVE IT */

	r = mouse->Rot.y += (fps * .7f);			// make curve

	mouse->Speed2D += fps * 1000.0f;			// accel
	if (mouse->Speed2D > MOUSE_WALK_SPEED)
		mouse->Speed2D = MOUSE_WALK_SPEED;
	s = mouse->Speed2D;

	gDelta.x = -sin(r) * s;
	gDelta.z = -cos(r) * s;
	gDelta.y -= 1000.0f * fps;					// add gravity

	gCoord.y += gDelta.y*fps;					// move
	gCoord.x += gDelta.x*fps;
	gCoord.z += gDelta.z*fps;

	HandleCollisions(mouse, CTYPE_TERRAIN | CTYPE_FENCE | CTYPE_MISC, .5);


	UpdateObject(mouse);
}


#pragma mark -

/********************* ADD TRAMPOLINE *************************/

Boolean AddTrampoline(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*web, *base;
float	y;

	y = FindHighestCollisionAtXZ(x, z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN);

						/*************/
						/* MAKE BASE */
						/*************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.scale 		= .8;
	gNewObjectDefinition.type 		= CLOSET_ObjType_TrampolineBase;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB - 2;
	gNewObjectDefinition.moveCall 	= MoveTrampoline;
	gNewObjectDefinition.rot 		= 0;
	base = MakeNewDisplayGroupObject(&gNewObjectDefinition);


						/************/
						/* MAKE WEB */
						/************/

	gNewObjectDefinition.type 		= CLOSET_ObjType_TrampolineWebDown;
	gNewObjectDefinition.coord.y 	= y + 100.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_DOUBLESIDED;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	web = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	web->TerrainItemPtr = itemPtr;								// keep ptr to item list

	web->ColorFilter.r =
	web->ColorFilter.g =
	web->ColorFilter.b = 1.3;									// brighten so is visible

			/* SET COLLISION INFO */

	web->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_TRIGGER|CTYPE_BLOCKSHADOW;
	web->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(web,1,1);

	web->TriggerCallback = DoTrig_Trampoline;

	web->TrampAmplitude = 0;


	base->ChainNode = web;

	return(true);
}


/******************* MOVE TRAMPOLINE ***********************/

static void MoveTrampoline(ObjNode *base)
{
float	fps = gFramesPerSecondFrac;
float	s;
ObjNode	*web = base->ChainNode;

			/* SEE IF THIS ONE IS GONE */

	if (TrackTerrainItem(base))
	{
		DeleteObject(base);
		return;
	}

			/* WOBBLE */


	web->TrampAmplitude -= fps * .35f;						// dec wobble amplitude
	if (web->TrampAmplitude < 0.0f)
		web->TrampAmplitude = 0;
	else
	{
		web->TrampDecaySpeed += fps * 5.0f;							// accel wobble as it shrinks
		web->TrampWobbleIndex += fps * web->TrampDecaySpeed;	// inc wobble index
	}


			/* SCALE ON Y */
			//
			// since -y scales will invert the lighting we swap between the up and down
			// versions of the model so that the scale is always (+)
			//

	s = sin(web->TrampWobbleIndex) * web->TrampAmplitude;
	if (s > 0.0f)
	{
downscale:
		web->Scale.y = s;
		if (web->Type != CLOSET_ObjType_TrampolineWebDown)
		{
			web->Type = CLOSET_ObjType_TrampolineWebDown;
			ResetDisplayGroupObject(web);
		}
	}
	else
	if (s < 0.0f)
	{
		web->Scale.y = -s;
		if (web->Type != CLOSET_ObjType_TrampolineWebUp)
		{
			web->Type = CLOSET_ObjType_TrampolineWebUp;
			ResetDisplayGroupObject(web);
		}
	}
	else				// WARNING!!!  Avoid scales of 0.0!!
	{
		s = .0001f;
		goto downscale;
	}

	UpdateObjectTransforms(web);
}


/************** DO TRIGGER - TRAMPOLINE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Trampoline(ObjNode *theNode, ObjNode *who, Byte sideBits)
{
	if ((gDelta.y < 0.0f) && (sideBits & SIDE_BITS_BOTTOM))		// only trigger if player's bottom hit and was going down
	{
		gDelta.y = 2500.0f;
		gCoord.y = theNode->Coord.y + theNode->TopOff;

		theNode->TrampAmplitude = 1.5f;
		theNode->TrampWobbleIndex = 0;
		theNode->TrampDecaySpeed = 8.0;

		SetPlayerJumpAnim(who, true);
		gResetGliding = true;
		gDoGlidingAtApex = false;

		return(false);									// report non-solid hit so won't affect deltas
	}

	return(true);
}


#pragma mark -

/************************ PRIME VACUUME *************************/

Boolean PrimeVacuume(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj, *light;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/****************/
				/* MAKE VACUUME */
				/****************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.scale 		= 2.0;
	gNewObjectDefinition.type 		= CLOSET_ObjType_Vacuume;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= STATUS_BIT_ONSPLINE|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-60;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


				/* SET BETTER INFO */

	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveVacuumeOnSpline;					// set move call

	newObj->Coord.y 		-= newObj->BottomOff;


			/* SET COLLISION STUFF */

	newObj->Damage			= .33f;

	newObj->CType 			= CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID | CBITS_IMPENETRABLE;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);


				/**************/
				/* MAKE LIGHT */
				/**************/

	gNewObjectDefinition.type 		= CLOSET_ObjType_Light;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	light = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ChainNode = light;


	AttachShadowToObject(newObj, SHADOW_TYPE_SQUARE, 18, 18, false);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);



	return(true);
}


/******************** MOVE VACUUME ON SPLINE ***************************/

static void MoveVacuumeOnSpline(ObjNode *theNode)
{
Boolean isInRange;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 200);
	GetObjectCoordOnSpline(theNode);


			/****************************/
			/* UPDATE STUFF IF IN RANGE */
			/****************************/

	if (isInRange)
	{
		ObjNode *light = theNode->ChainNode;

		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff + 20.0f;	// calc y coord
		UpdateObjectTransforms(theNode);															// update transforms
		UpdateShadow(theNode);

				/* SOUND */

		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_VACUUME, &theNode->Coord, NORMAL_CHANNEL_RATE, 1.6);
		else
			Update3DSoundChannel(EFFECT_VACUUME, &theNode->EffectChannel, &theNode->Coord);


					/* ALSO UPDATE LIGHT */

		light->BaseTransformMatrix = theNode->BaseTransformMatrix;

		SetObjectTransformMatrix(light);
		light->Coord.x = light->BaseTransformMatrix.value[M03];					// extract x,y,z coords
		light->Coord.y = light->BaseTransformMatrix.value[M13];
		light->Coord.z = light->BaseTransformMatrix.value[M23];

				/* UPDATE COLLISION BOX */

		CreateCollisionBoxFromBoundingBox_Rotated(theNode,.9,.6);


					/* SEE IF SUCK PLAYER */

		SeeIfVacuumeSuckPlayer(theNode);
	}

			/* HIDDEN, SO BE SURE SOUND IS OFF */
	else
	{
		StopAChannel(&theNode->EffectChannel);
	}
}


/******************** SEE IF VACUUME SUCK PLAYER ************************/

static void SeeIfVacuumeSuckPlayer(ObjNode *theNode)
{
float	dist;
ObjNode	*player;
float	x,z,r;

	if (gPlayerInfo.invincibilityTimer > 0.0f)								// must be eligible
		return;

	player = gPlayerInfo.objNode;
	if (!(player->StatusBits & STATUS_BIT_ONGROUND))						// must be on ground
		return;

			/* CALC CENTRAL SUCK COORD */

	r = theNode->Rot.y;
	x = theNode->Coord.x - sin(r) * VACUUME_SUCK_OFF;
	z = theNode->Coord.z - cos(r) * VACUUME_SUCK_OFF;

	dist = CalcQuickDistance(x, z, player->Coord.x, player->Coord.z);
	if (dist > VACUUME_SUCK_DIST)														// must be in range
		return;

	MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_VACUUMESUCK, 5);
	gPlayerInfo.invincibilityTimer = 3.0;
	gSuckingVacuume = theNode;
	gPlayerInfo.suckSpeed = 50;

}












