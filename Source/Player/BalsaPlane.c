/****************************/
/*   	BALSA PLANE.C		*/
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

static void MoveBalsaPlane(ObjNode *plane);
static void MovePlayerOnBalsaPlane(ObjNode *player);
static void MoveBalsaPlaneCrashing(ObjNode *plane);
static void UpdateBalsaPlane(ObjNode *plane);
static void MovePlayer_FallingFromPlane(ObjNode *theNode);
static void PlaneShootBullet(ObjNode *plane);
static void PlaneDropBomb(ObjNode *plane);
static void MoveBomb(ObjNode *bomb);
static void ExplodeBomb(ObjNode *theNode);
static void MoveAntHill_Exploded(ObjNode *theNode);
static void MoveShockwave(ObjNode *theNode);
static void MoveBullet(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	BALSA_PLANE_SPEED	900.0f

#define	BULLET_SPEED		2000.0f

#define	MAX_BOMBS			4
#define	MAX_BULLETS			4

/*********************/
/*    VARIABLES      */
/*********************/

#define	 WobbleZ	SpecialF[0]
#define	 WobbleX	SpecialF[1]
#define	 WobbleFactor SpecialF[2]

static ObjNode *gPlane;

static	int		gNumBombs, gNumBullets;
int				gNumAntHills, gNumAntHillsDestroyed;



/************************* PUT PLAYER IN BALSA PLANE *********************************/

void PutPlayerInBalsaPlane(ObjNode *player)
{
ObjNode	*prop, *band;

				/************************/
				/* MAKE THE BALSA PLANE */
				/************************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BALSA_ObjType_Plane;
	gNewObjectDefinition.coord.x 	= player->Coord.x;
	gNewObjectDefinition.coord.z 	= player->Coord.z;
	gNewObjectDefinition.coord.y 	= gDragonflyY = player->Coord.y + BALSA_PLANE_HOVER_HEIGHT;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL | STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_ROTZXY;
	gNewObjectDefinition.slot 		= PLAYER_SLOT-1;
	gNewObjectDefinition.moveCall 	= MoveBalsaPlane;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .8f;
	gPlane = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* MAKE SHADOW */

	AttachShadowToObject(gPlane, SHADOW_TYPE_BALSAPLANE, 10,10, false);


					/********************/
					/* ATTACH PROPELLER */
					/********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BALSA_ObjType_Prop;
	gNewObjectDefinition.coord	 	= gPlane->Coord;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL | STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_DOUBLESIDED | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.scale 		= gPlane->Scale.x;
	prop = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gPlane->ChainNode = prop;

					/**********************/
					/* ATTACH RUBBER BAND */
					/**********************/

	gNewObjectDefinition.type 		= BALSA_ObjType_RubberBand;
	gNewObjectDefinition.coord	 	= gPlane->Coord;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot++;
	band = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	prop->ChainNode = band;


			/*********************/
			/* PUT SKIP ON PLANE */
			/*********************/

	SetSkeletonAnim(player->Skeleton, PLAYER_ANIM_FLYBALSAPLANE);
	player->MoveCall = MovePlayerOnBalsaPlane;


			/* START SOUND */

	gPlane->EffectChannel = PlayEffect_Parms(EFFECT_PROPELLER, FULL_CHANNEL_VOLUME/2, FULL_CHANNEL_VOLUME/2, NORMAL_CHANNEL_RATE);


	gNumBombs = gNumBullets = 0;
}


/*********************** RESET PLAYER ON BALSA PLANE **********************/

void ResetPlayerOnBalsaPlane(ObjNode *player)
{
	if (gPlane)					// make absolutely sure the old plane is gone
	{
		DeleteObject(gPlane);
		gPlane = nil;
	}

	PutPlayerInBalsaPlane(player);
}

/******************** MOVE BALSA PLANE ******************/

static void MoveBalsaPlane(ObjNode *plane)
{
const float	fps = gFramesPerSecondFrac;
float	r,w;

	GetObjectInfo(plane);


			/* DO USER CONTROL TURNING */

	r = plane->Rot.y -= gPlayerInfo.analogControlX * fps * 3.0f;


			/* DO ROT WOBBLE */

	plane->WobbleFactor -= fps * .5f;					// decay to desired amount of .2
	if (plane->WobbleFactor <= .2f)
		plane->WobbleFactor = .2f;
	w = plane->WobbleFactor;

	plane->WobbleZ += fps * 4.0f;
	plane->Rot.z = sin(plane->WobbleZ) * w;

	plane->WobbleX += fps * 3.5f;
	plane->Rot.x = sin(plane->WobbleX) * (w * .5f);


				/* MOVE IT */

	gDelta.x = -sin(r) * BALSA_PLANE_SPEED;
	gDelta.z = -cos(r) * BALSA_PLANE_SPEED;

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;


				/* WEAPONS */

	if (gControlNeeds[kNeed_Kick].newButtonPress)						// see if shoot
	{
		PlaneShootBullet(plane);
	}
	else
	if (gControlNeeds[kNeed_PickupDrop].newButtonPress)					// see if drop bomb
	{
		PlaneDropBomb(plane);
	}


				/* COLLISION */

	if (HandleCollisions(plane, CTYPE_MISC | CTYPE_FENCE | CTYPE_ENEMY | CTYPE_TRIGGER, .4))
	{
		PlayerGotHit(nil, .2, 0);
	}



				/**********/
				/* UPDATE */
				/**********/

	UpdateBalsaPlane(plane);

}


/************************* MOVE BALSA PLANE CRASHING *****************************/

static void MoveBalsaPlaneCrashing(ObjNode *plane)
{
const OGLVector3D	v = {0,0,-1};
OGLVector3D			v2;
const float	fps = gFramesPerSecondFrac;

	GetObjectInfo(plane);

			/* NOSE DIVE */

	plane->Rot.x -= fps;
	if (plane->Rot.x <= (-PI/3))
		plane->Rot.x = -PI/3;

	plane->Rot.z += fps * 6.0f;

				/* MOVE IT */

	OGLVector3D_Transform(&v, &plane->BaseTransformMatrix, &v2);

	gDelta.x = v2.x * BALSA_PLANE_SPEED;
	gDelta.y = v2.y * BALSA_PLANE_SPEED;
	gDelta.z = v2.z * BALSA_PLANE_SPEED;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* BURN */

	BurnPlane(plane);


		/* SEE IF HIT GROUND */

	if (gCoord.y <= GetTerrainY(gCoord.x, gCoord.z))
	{
		StopAChannel(&gPlane->EffectChannel);				// stop dive bomb sound
		PlayEffect3D(EFFECT_PLANECRASH, &gCoord);
		MakeFireExplosion(&gCoord);
		ExplodeGeometry(plane, 400, SHARD_MODE_FROMORIGIN | SHARD_MODE_BOUNCE | SHARD_MODE_UPTHRUST, 1, .5);
		DeleteObject(plane);
		gPlane = nil;
		return;
	}

	UpdateBalsaPlane(plane);
}


/************************* UPDATE BALSA PLANE ******************************/

static void UpdateBalsaPlane(ObjNode *plane)
{
ObjNode	*prop, *band;
OGLMatrix4x4	m;
const float	fps = gFramesPerSecondFrac;

	UpdateObject(plane);


				/* ALIGN PROP */

	prop = plane->ChainNode;															// get prop obj

	prop->Rot.z -= fps * 60.0f;															// spin prop

	OGLMatrix4x4_SetRotate_Z(&m, prop->Rot.z);											// set rot matrix
	m.value[M03] = 0;
	m.value[M13] = -16;
	m.value[M23] = -132;																// offset to nose of plane
	OGLMatrix4x4_Multiply(&m, &plane->BaseTransformMatrix, &prop->BaseTransformMatrix); // set matrix
	SetObjectTransformMatrix(prop);


				/* ALIGN RUBBER BAND */

	band = prop->ChainNode;

	band->Rot.z -= fps * 60.0f;															// spin prop

	OGLMatrix4x4_SetRotate_Z(&m, band->Rot.z);											// set rot matrix
	m.value[M03] = 0;
	m.value[M13] = -15;
	m.value[M23] = -30;																// offset to nose of plane
	OGLMatrix4x4_Multiply(&m, &plane->BaseTransformMatrix, &band->BaseTransformMatrix); // set matrix
	SetObjectTransformMatrix(band);


			/* UPDATE COLLISION BOX */

	CreateCollisionBoxFromBoundingBox_Rotated(plane, 1,1);
}


#pragma mark -


/********************** MOVE PLAYER ON BALSA PLANE **************************/
//
// Basically just keeps Skip aligned on the plane.
//

static void MovePlayerOnBalsaPlane(ObjNode *player)
{
OGLMatrix4x4	sm;


			/* JUST ALIGN PLAYER ON PLANE */

	OGLMatrix4x4_SetScale(&sm, 1.3, 1.3, 1.3);
	OGLMatrix4x4_Multiply(&sm, &gPlane->BaseTransformMatrix, &player->BaseTransformMatrix);

	SetObjectTransformMatrix(player);


	player->Coord.x = player->BaseTransformMatrix.value[M03];
	player->Coord.y = player->BaseTransformMatrix.value[M13];
	player->Coord.z = player->BaseTransformMatrix.value[M23];
	gPlayerInfo.coord = player->Coord;

	gPlayerInfo.invincibilityTimer -= gFramesPerSecondFrac;
}


/****************** HURT PLAYER ON BALSA PLANE **********************/
//
// Called from PlayerGotHit() which handles the health dec and other checks.
// This only needs to handle the plane's response.
//

void HurtPlayerOnBalsaPlane(void)
{
	gPlayerInfo.invincibilityTimer = 1.5;

	ExplodeGeometry(gPlane, 400, SHARD_MODE_FROMORIGIN, 2, 1.3);

	gPlane->WobbleFactor = 2.0f;					// make it really wobble hard

	PlayEffect3D(EFFECT_PLANEHIT, &gPlane->Coord);

}


/****************** KILL PLAYER ON BALSA PLANE ************************/

void KillPlayerOnBalsaPlane(ObjNode *skip)
{
	AttachShadowToObject(skip, SHADOW_TYPE_CIRCULARDARK, 7, 7, false);
	MorphToSkeletonAnim(skip->Skeleton, PLAYER_ANIM_FALLFROMPLANE, 5);
	skip->MoveCall = MovePlayer_FallingFromPlane;
	skip->Delta.x = gPlane->Delta.x * .8f;
	skip->Delta.z = gPlane->Delta.z * .8f;
	skip->Delta.y = 1000;
	gPlane->MoveCall = MoveBalsaPlaneCrashing;

	gFreezeCameraFromXZ = gFreezeCameraFromY = true;

	StopAChannel(&gPlane->EffectChannel);				// stop propeller sound
	gPlane->EffectChannel = PlayEffect(EFFECT_DIVEBOMB);
}


/***************** MOVE PLAYER: FALLING FROM PLANE *******************/

static void MovePlayer_FallingFromPlane(ObjNode *theNode)
{
const float	fps = gFramesPerSecondFrac;
float	y;

	GetObjectInfo(theNode);

	gDelta.y -= 3500.0f * fps;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.y += fps * 9.0f;
	theNode->Rot.x += fps * 6.0f;

			/* SEE IF HIT GROUND */

	y = GetTerrainY(gCoord.x, gCoord.z);
	if (gCoord.y <= y)
	{
		ExplodeGeometry(theNode, 200, SHARD_MODE_FROMORIGIN | SHARD_MODE_BOUNCE | SHARD_MODE_UPTHRUST, 3, 1.0);
		theNode->StatusBits |= STATUS_BIT_HIDDEN | STATUS_BIT_NOMOVE;
	}


	UpdateObject(theNode);
	gPlayerInfo.coord = theNode->Coord;
}



#pragma mark -


/*********************** PLANE SHOOT BULLET ******************************/

static void PlaneShootBullet(ObjNode *plane)
{
ObjNode	*bullet;
float	r;

	if (gNumBullets >= MAX_BULLETS)								// only allow n bullets at the same time
		return;

	r = plane->Rot.y;

	gNewObjectDefinition.coord.x = plane->Coord.x - sin(r) * 130.0f;
	gNewObjectDefinition.coord.z = plane->Coord.z - cos(r) * 130.0f;
	gNewObjectDefinition.coord.y = plane->Coord.y - 20.0f;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BALSA_ObjType_Bullet;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= plane->Slot - 2;			// don't move until next frame
	gNewObjectDefinition.moveCall 	= MoveBullet;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.0f;
	bullet = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	bullet->Delta.x = -sin(r) * BULLET_SPEED;
	bullet->Delta.z = -cos(r) * BULLET_SPEED;


			/* SET COLLISION STUFF */

	bullet->Damage 	= 1.0f;
	bullet->CType 	= CTYPE_HURTENEMY;
	bullet->CBits	= 0;
	CreateCollisionBoxFromBoundingBox(bullet, 2, 2);


			/* MAKE SHADOW */

	AttachShadowToObject(bullet, SHADOW_TYPE_CIRCULAR, 2,2, false);

	PlayEffect(EFFECT_BALSASHOOT);

	gNumBullets++;
}


/********************** MOVE BULLET ******************************/

static void MoveBullet(ObjNode *theNode)
{
const float	fps = gFramesPerSecondFrac;

			/* SEE IF GONE */

	if (theNode->StatusBits & STATUS_BIT_ISCULLED)
	{
		DeleteObject(theNode);
		gNumBullets--;
		return;
	}

	GetObjectInfo(theNode);

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;

	UpdateObject(theNode);
}



/*********************** PLANE DROP BOMB ******************************/

static void PlaneDropBomb(ObjNode *plane)
{
ObjNode	*bomb;

	if (gNumBombs >= MAX_BOMBS)								// only allow n bombs at the same time
		return;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BALSA_ObjType_Bomb;
	gNewObjectDefinition.coord 		= plane->Coord;
	gNewObjectDefinition.coord.y 	-= 40.0f;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL | STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_ROTZXY;
	gNewObjectDefinition.slot 		= plane->Slot - 1;		// don't move until next frame
	gNewObjectDefinition.moveCall 	= MoveBomb;
	gNewObjectDefinition.rot 		= plane->Rot.y;
	gNewObjectDefinition.scale 		= 5.0f;
	bomb = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	bomb->Delta.x = plane->Delta.x * .9f;
	bomb->Delta.z = plane->Delta.z * .9f;

			/* SET COLLISION STUFF */

	bomb->CType = CTYPE_HURTENEMY;
	bomb->CBits	= 0;
	CreateCollisionBoxFromBoundingBox(bomb, 1, 1);


			/* MAKE SHADOW */

	AttachShadowToObject(bomb, SHADOW_TYPE_CIRCULAR, 2,2, false);

	PlayEffect(EFFECT_BOMBFALL);

	gNumBombs++;
}


/*********************** MOVE BOMB *************************/

static void MoveBomb(ObjNode *bomb)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(bomb);

			/* MOVE IT */

	ApplyFrictionToDeltas(250.0,&gDelta);

	gDelta.y -= 4000.0f * fps;						// gravity
	gCoord.x += gDelta.x * fps;						// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	bomb->Rot.x += fps * 3.0f;						// spin
	bomb->Rot.y += fps * 6.0f;


				/* COLLISION */

	if (HandleCollisions(bomb, CTYPE_MISC | CTYPE_FENCE | CTYPE_TERRAIN, 1))
	{
		ExplodeBomb(bomb);
		return;
	}


	UpdateObject(bomb);
}


/********************** EXPLODE BOMB ********************************/

static void ExplodeBomb(ObjNode *theNode)
{
ObjNode	*thisNode;
float	y;

	gNumBombs--;


			/**********************/
			/* SEE IF HIT ANTHILL */
			/**********************/

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode->What == WHAT_ANTHILL)
		{
			if (CalcDistance(gCoord.x, gCoord.z, thisNode->Coord.x, thisNode->Coord.z) < 150.0f)
			{
				BlowUpAntHill(thisNode);
				break;
			}
		}

		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);


			/**************/
			/* BLOW IT UP */
			/**************/

	if (GetWaterY(gCoord.x, gCoord.z, &y))						// see if on water
	{
		CreateNewRipple(gCoord.x, gCoord.z, RandomFloat() * 50.0f, 100.0f, .3);
		MakeSplash(gCoord.x, y, gCoord.z, 1.0);

	}

	PlayEffect3D(EFFECT_BOMBBOOM, &gCoord);
	ExplodeGeometry(theNode, 300, SHARD_MODE_FROMORIGIN | SHARD_MODE_BOUNCE, 1, 2.0);
	MakeSparkExplosion(gCoord.x, gCoord.y, gCoord.z, 200, 3.0, PARTICLE_SObjType_YellowGlint, 100, 1.2);
	MakeFireExplosion(&gCoord);
	DeleteObject(theNode);
}


#pragma mark -

/**************************** COUNT ANT HILLS *********************************/

void CountAntHills(void)
{
int						i;
TerrainItemEntryType 	*itemPtr;

	gNumAntHillsDestroyed = 0;
	gNumAntHills = 0;

	itemPtr = *gMasterItemList; 											// get pointer to data inside the LOCKED handle

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_ANTHILL)
			gNumAntHills++;
	}

}


/************************* ADD ANT HILL *********************************/


Boolean AddAntHill(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

				/*************************/
				/* MAKE THE BLOWN UP TOP */
				/*************************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BALSA_ObjType_AntHill;
	gNewObjectDefinition.scale 		= 2.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + 90.0f;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 200;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 	= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->What = WHAT_ANTHILL;

			/* SEE IF BLOWN UP */

	if (itemPtr->flags & ITEM_FLAGS_USER1)							// see if blown up
	{
		newObj->MoveCall = MoveAntHill_Exploded;
	}

			/* NOT BLOWN UP YET */
	else
	{
		newObj->StatusBits |= STATUS_BIT_HIDDEN;					// hide it
	}

	return(true);													// item was added
}


/******************** BLOW UP ANT HILL ****************************/

void BlowUpAntHill(ObjNode *theNode)
{
float	x,z;
int		i;
ObjNode	*newObj;

	PlayEffect3D(EFFECT_HILLBOOM, &theNode->Coord);


	theNode->What = -1;
	theNode->StatusBits &= ~STATUS_BIT_HIDDEN;
	theNode->MoveCall = MoveAntHill_Exploded;
	theNode->ColorFilter.a = 0;

	ExplodeGeometry(theNode, 100, SHARD_MODE_FROMORIGIN | SHARD_MODE_BOUNCE, 1, 1.0);
	MakeFireExplosion(&gCoord);


			/* MAKE SMOKING AREAS */

	for (i = 0; i < 3; i++)
	{
		x = theNode->Coord.x + RandomFloat2() * 250.0f;
		z = theNode->Coord.z + RandomFloat2() * 250.0f;
		MakeSmoker(x,z, 0);
	}


			/* MAKE SHOCKWAVE */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BALSA_ObjType_Shockwave;
	gNewObjectDefinition.coord.x 	= theNode->Coord.x;
	gNewObjectDefinition.coord.z 	= theNode->Coord.z;
	gNewObjectDefinition.coord.y 	= theNode->Coord.y + 20.0f;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_GLOW | STATUS_BIT_DOUBLESIDED |
									STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= WATER_SLOT + 10;
	gNewObjectDefinition.moveCall 	= MoveShockwave;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* SEE IF THAT'S ALL */

	gNumAntHillsDestroyed++;

	if (gNumAntHillsDestroyed >= gNumAntHills)
	{
		StartLevelCompletion(5.0);
		DoDialogMessage(DIALOG_MESSAGE_HILLSDONE, 1, 5.0, nil);
	}
}


/****************** MOVE ANT HILL: EXPLODED ********************/

static void MoveAntHill_Exploded(ObjNode *theNode)
{
		/* FADE IN QUICKLY */

	theNode->ColorFilter.a += gFramesPerSecondFrac * 3.0f;
	if (theNode->ColorFilter.a > 1.0f)
		theNode->ColorFilter.a = 1.0f;


			/* BURN */

	if (!(theNode->StatusBits & STATUS_BIT_ISCULLED))							// only burn if visible
	{
		BurnFire(theNode, theNode->Coord.x, theNode->Coord.y, theNode->Coord.z, true,
				PARTICLE_SObjType_Fire, 2.0, PARTICLE_FLAGS_ALLAIM);
	}
}



/********************* MOVE SHOCKWAVE *************************/

static void MoveShockwave(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Scale.x =
	theNode->Scale.y =
	theNode->Scale.z = theNode->Scale.x += fps * 800.0f;

	theNode->ColorFilter.a -= fps * 1.5f;
	if (theNode->ColorFilter.a <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

	UpdateObjectTransforms(theNode);
}

#pragma mark -

/************************* ADD CLOUD *********************************/


Boolean AddCloud(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

				/*************************/
				/* MAKE THE BLOWN UP TOP */
				/*************************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BALSA_ObjType_Cloud;
	gNewObjectDefinition.scale 		= 6.0f + RandomFloat() * 6.0f;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + BALSA_PLANE_HOVER_HEIGHT + 150.0f;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_GLOW | STATUS_BIT_DOUBLESIDED |
								STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZBUFFER;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB + 100;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->ColorFilter.a = .9;

	return(true);													// item was added
}


#pragma mark -

/****************** BURN PLANE ************************/

#define	FIRE_TIMER	.04f
#define	SMOKE_TIMER	.05f

void BurnPlane(ObjNode *theNode)
{
int		i;
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;
float				x,y,z;

	x = gCoord.x;
	y = gCoord.y;
	z = gCoord.z;

		/**************/
		/* MAKE SMOKE */
		/**************/

	theNode->SmokeTimer -= fps;													// see if add smoke
	if (theNode->SmokeTimer <= 0.0f)
	{
		theNode->SmokeTimer += SMOKE_TIMER;										// reset timer

		particleGroup 	= theNode->SmokeParticleGroup;
		magicNum 		= theNode->SmokeParticleMagic;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{

			theNode->SmokeParticleMagic = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND|PARTICLE_FLAGS_ALLAIM;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 30.0f;
			groupDef.decayRate				=  -.2f;
			groupDef.fadeRate				= .2;
			groupDef.particleTextureNum		= PARTICLE_SObjType_BlackSmoke;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			theNode->SmokeParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (i = 0; i < 3; i++)
			{
				p.x = x + RandomFloat2() * 50.0f;
				p.y = y + RandomFloat() * 50.0f;
				p.z = z + RandomFloat2() * 50.0f;

				d.x = RandomFloat2() * 30.0f;
				d.y = RandomFloat() * 40.0f;
				d.z = RandomFloat2() * 30.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2();
				newParticleDef.alpha		= .9;
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
		theNode->FireTimer += FIRE_TIMER;										// reset timer

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND|PARTICLE_FLAGS_ALLAIM;
			groupDef.gravity				= -200;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 20.0f;
			groupDef.decayRate				=  -.5;
			groupDef.fadeRate				= 1.0;
			groupDef.particleTextureNum		= PARTICLE_SObjType_Fire;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (i = 0; i < 3; i++)
			{
				p.x = x + RandomFloat2() * 30.0f;
				p.y = y + RandomFloat() * 50.0f;
				p.z = z + RandomFloat2() * 30.0f;

				d.x = RandomFloat2() * 50.0f;
				d.y = 50.0f + RandomFloat() * 60.0f;
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









