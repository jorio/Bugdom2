/****************************/
/*   	SLOTCAR.C		    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gCurrentAspectRatio,gTerrainPolygonSize;
extern	OGLPoint3D			gCoord;
extern	OGLVector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLBoundingBox 		gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	OGLSetupOutputType	*gGameViewInfoPtr;
extern	uint32_t				gAutoFadeStatusBits,gGlobalMaterialFlags;
extern	Boolean				gG4,gHeadOnScarecrow,gResetRideBall;
extern	PlayerInfoType		gPlayerInfo;
extern	int					gLevelNum,gNumBowlingPinsDown, gDialogSoundEffect;
extern	SparkleType	gSparkles[MAX_SPARKLES];
extern	short				gNumEnemies, gDialogSoundChannel;
extern	AGLContext		gAGLContext;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveSlotCarOnSpline(ObjNode *theNode);
static Boolean DoTrig_SlotCar(ObjNode *car, ObjNode *who, Byte sideBits);
static void MoveSlotCarWhileRacing(ObjNode *car);
static void PutSnailOnCar(ObjNode *car);
static void MoveSnailOnSlotCar(ObjNode *snail);
static void PlayerExitsSlotCar(ObjNode *car);
static void ActivateCarLights(ObjNode *car);
static void NukeCarLights(ObjNode *car);
static void UpdateWheelAlignment(ObjNode *car);
static void AlignPlayerOnSlotCar(ObjNode *player);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	SLOT_CAR_SCALE	4.5f

			// NOTE: This butt-speed is mainly what determines how the car handles.
			//		 Raise this number and the car will spin more

#define	MAX_BUTT_SPEED	45.0f

#define	SLOTCAR_ACC				500.0f
#define	MAX_CAR_SPEED			1500.0f

#define	MAX_LAPS				3

#define	BUTT_DIST				400.0f

/*********************/
/*    VARIABLES      */
/*********************/

int		gSlotCarRacingMode = SLOTCAR_RACEMODE_IDLE;
float	gSlotCarStartTimer;
static	int		gSlotCarLap[2];

static	OGLVector2D	gButtVector[2];
static	OGLPoint2D	gButtPt[2];
static	float		gButtSpeed[2];

#define	CarNum		Special[0]
#define	CarMotorRPM	SpecialF[0]
#define	CarSpeed	SpecialF[1]
#define	HasSparkles	Flag[0]

Boolean	gPlayerWonSlotCarRace,gNotifyOfSlotCarWin,gSnailWonSlotCarRace;



/************************ INIT SLOT CAR RACING ************************/
//
// Global init called when level is initialized.
//

void InitSlotCarRacing(void)
{
	gSlotCarRacingMode = SLOTCAR_RACEMODE_IDLE;

}


/************************ PRIME SLOT CAR *************************/

Boolean PrimeSlotCar(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*car, *frontWheels, *rearWheels;
float			x,z,placement = 0;
int				carNum;

	carNum = itemPtr->parm[0];

			/* GET SPLINE INFO */

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);		// cars always start @ spline index == 0

				/************/
				/* MAKE CAR */
				/************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PLAYROOM_ObjType_SlotCarRed + carNum;
	gNewObjectDefinition.scale 		= SLOT_CAR_SCALE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= PLAYER_SLOT - 10;				// must move before player
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	car = MakeNewDisplayGroupObject(&gNewObjectDefinition);


				/* SET BETTER INFO */

	car->SplineItemPtr 		= itemPtr;
	car->SplineNum 			= splineNum;
	car->SplinePlacement	= placement;
	car->SplineMoveCall 	= MoveSlotCarOnSpline;					// set move call

	car->Coord.y 			-= car->BottomOff;

	car->CarNum 			= carNum;								// remember which car # this is


			/* SET COLLISION STUFF */

	if (carNum == 0)
	{
		car->TriggerCallback = DoTrig_SlotCar;
		car->CType 			= CTYPE_MISC|CTYPE_TRIGGER|CTYPE_BLOCKSHADOW;
	}
	else
		car->CType 			= CTYPE_MISC;

	car->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(car, 1, 1);


	gButtVector[carNum].x = 1;								// init butt vector
	gButtVector[carNum].y = 0;

	gButtPt[carNum].x = car->Coord.x - BUTT_DIST;
	gButtPt[carNum].y = car->Coord.z;
	gButtSpeed[carNum] = 0;


				/*********************/
				/* MAKE FRONT WHEELS */
				/*********************/

	gNewObjectDefinition.type 		= PLAYROOM_ObjType_FrontWheels;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot++;
	frontWheels = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	car->ChainNode = frontWheels;
	frontWheels->ChainHead = car;


				/*********************/
				/* MAKE REAR WHEELS */
				/*********************/

	gNewObjectDefinition.type 		= PLAYROOM_ObjType_RearWheels;
	gNewObjectDefinition.slot++;
	rearWheels = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	frontWheels->ChainNode = rearWheels;
	rearWheels->ChainHead = frontWheels;

	UpdateWheelAlignment(car);



			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(car, true);										// detach this object from the linked list
	AddToSplineObjectList(car, true);


			/******************************/
			/* SEE IF PUT SNAIL ON CAR #1 */
			/******************************/

	if (carNum == 1)
	{
		PutSnailOnCar(rearWheels);
	}

	return(true);
}


/********************* ACTIVATE CAR LIGHTS ***************************/

static void ActivateCarLights(ObjNode *car)
{
int	i,j;

	if (car->HasSparkles)
		return;

				/* CREATE TAIL LIGHTS */

	for (j = 0; j < 4; j++)
	{
		const	OGLPoint3D	off[4] = {
									 	-13, 11 ,75,
									 	-9, 11 ,75,
									 	13, 11 ,75,
									 	9, 11 ,75,
									 };

		i = car->Sparkles[j] = GetFreeSparkle(car);				// get free sparkle slot
		if (i != -1)
		{
			gSparkles[i].flags = SPARKLE_FLAG_TRANSFORMWITHOWNER | SPARKLE_FLAG_FLICKER;
			gSparkles[i].where = off[j];

			gSparkles[i].aim.x = 0;
			gSparkles[i].aim.y = 0;
			gSparkles[i].aim.z = 1;

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = .9;

			gSparkles[i].scale = 80.0f;
			gSparkles[i].separation = 15.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_RedGlint;
		}
	}


				/* CREATE HEADLIGHTS */

	for (j = 0; j < 2; j++)
	{
		const	OGLPoint3D	off[2] = {
									 	12, 8 ,-21,
									 	-12, 8 ,-21,
									 };

		i = car->Sparkles[4+j] = GetFreeSparkle(car);				// get free sparkle slot
		if (i != -1)
		{
			gSparkles[i].flags = SPARKLE_FLAG_TRANSFORMWITHOWNER | SPARKLE_FLAG_FLICKER;
			gSparkles[i].where = off[j];

			gSparkles[i].aim.x = 0;
			gSparkles[i].aim.y = 0;
			gSparkles[i].aim.z = -1;

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = 1;

			gSparkles[i].scale = 100.0f;
			gSparkles[i].separation = 15.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark4;
		}
	}

	car->HasSparkles = true;
}


/****************** NUKE CAR LIGHTS ******************************/

static void NukeCarLights(ObjNode *car)
{
int		i;

	if (car->HasSparkles)
	{
		for (i = 0; i < MAX_NODE_SPARKLES; i++)				// free sparkles
		{
			DeleteSparkle(car->Sparkles[i]);
			car->Sparkles[i] = -1;
		}

		car->HasSparkles = false;
	}
}

#pragma mark -


/******************** MOVE SLOTCAR ON SPLINE ***************************/

static void MoveSlotCarOnSpline(ObjNode *theNode)
{
Boolean isInRange;

	isInRange = IsSplineItemOnActiveTerrain(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	if (gSlotCarRacingMode == SLOTCAR_RACEMODE_RACE)
		MoveSlotCarWhileRacing(theNode);
	else
		NukeCarLights(theNode);									// make sure lights are off




			/* UPDATE STUFF IF IN RANGE */

	GetObjectCoordOnSpline(theNode);
	if (isInRange)
	{
		UpdateObjectTransforms(theNode);
		UpdateShadow(theNode);
		CreateCollisionBoxFromBoundingBox_Rotated(theNode, 1, 1);			// continuously update bbox for accuracy

		UpdateWheelAlignment(theNode);

				/* SEE IF ALIGN PLAYER IN CAR */

		if (theNode->CarNum == 0)
			if (gPlayerInfo.objNode->Skeleton->AnimNum == PLAYER_ANIM_DRIVESLOTCAR)
				AlignPlayerOnSlotCar(gPlayerInfo.objNode);
	}

}



/************** DO TRIGGER - SLOT CAR ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_SlotCar(ObjNode *car, ObjNode *who, Byte sideBits)
{
#pragma unused (who)

	if (sideBits & SIDE_BITS_BOTTOM)												// see if on top of car
	{
		car->CType &= ~CTYPE_BLOCKSHADOW;										// dont block shadows when sitting on car

		gPlayerWonSlotCarRace = false;
		gSnailWonSlotCarRace = false;
		gNotifyOfSlotCarWin = false;

		gPlayerInfo.slotCar = car;													// remember which car we're driving

		SetSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_DRIVESLOTCAR);	// set player anim

		gSlotCarRacingMode = SLOTCAR_RACEMODE_START;
		gSlotCarStartTimer = 3;
		return(false);
	}

	return(true);
}



/*********************** MOVE SLOTCAR WHILE RACING ************************/

static void MoveSlotCarWhileRacing(ObjNode *car)
{
float		fps = gFramesPerSecondFrac;
float		x1,z1,x2,z2,oldSplinePlacement;
OGLVector2D	trackAim, buttAim, curve;
int			splineNum = car->SplineNum;
float		angle;
int			carNum = car->CarNum;
float		speed,r,dot;
OGLVector2D	pinAim;

			/********************************/
			/* SEE IF THIS CAR HAS FINISHED */
			/********************************/

	if (gSlotCarLap[carNum] >= MAX_LAPS)
	{
		car->CarSpeed -= fps * (SLOTCAR_ACC * 10.0);			// brake fast

				/* CAR HAS STOPPED */

		if (car->CarSpeed < 0.0f)
		{
			car->CarSpeed = 0;

					/* PLAYER STOPPED */

			if (carNum == 0)
			{
				PlayerExitsSlotCar(car);					// get Skip out of there

				if (!gPlayerWonSlotCarRace)
					gSlotCarRacingMode = SLOTCAR_RACEMODE_IDLE;		// go idle so we can start another race
			}

					/* SNAIL STOPPED */

			else
			{
				if (gPlayerWonSlotCarRace)
					gSlotCarRacingMode = SLOTCAR_RACEMODE_OVER;		// player won, so the race is over
			}
		}
	}
	else
	{

			/*********************/
			/* DO PLAYER CONTROL */
			/*********************/

		if (carNum == 0)
		{
			float	acc = -gPlayerInfo.analogControlZ;

			if (acc)												// see if lights on/off
				ActivateCarLights(car);
			else
				NukeCarLights(car);


			if (acc == 0.0f)										// if no accel, then brake
				acc = -2.5f;
			else
			if (acc < 0.0f)											// if reverse then amplify braking
				acc *= 2.0f;

			car->CarSpeed += acc * fps * SLOTCAR_ACC;
			if (car->CarSpeed < 0.0f)								// cannot go backwards
				car->CarSpeed = 0.0f;
		}

			/********************/
			/* DO SNAIL CONTROL */
			/********************/

		else
		{
			float	oldSpeed = car->CarSpeed;

				/* CALC AIM VECTOR OF CURRENT LOCATION ON TRACK */

			GetCoordOnSpline(&(*gSplineList)[splineNum], car->SplinePlacement, &x1, &z1);			// get current coord
			GetNextCoordOnSpline(&(*gSplineList)[splineNum], car->SplinePlacement, &x2, &z2);		// get next coord
			trackAim.x = x2 - x1;																	// calc normalized aim vector
			trackAim.y = z2 - z1;
			FastNormalizeVector2D(trackAim.x, trackAim.y, &trackAim, true);

					/* CALC AIM VECTOR AHEAD */

			GetCoordOnSpline2(&(*gSplineList)[splineNum], car->SplinePlacement, 300, &x1, &z1);
			GetCoordOnSpline2(&(*gSplineList)[splineNum], car->SplinePlacement, 305, &x2, &z2);
			curve.x = x2 - x1;																	// calc normalized aim vector
			curve.y = z2 - z1;
			FastNormalizeVector2D(curve.x, curve.y, &curve, true);

			angle = acos(OGLVector2D_Dot(&curve, &trackAim));					// see if sharp turn ahead

			if (angle > (PI/5.0f))										// slow down if sharp turn
			{
				if (car->CarSpeed > 400.0f)								// if going fast then brake
				{
					car->CarSpeed -= fps * (SLOTCAR_ACC * 1.5);			// brake

					if (car->CarSpeed < 0.0f)							// never go backwards
						car->CarSpeed = 0;
				}
				else													// we're going slow enough that we can accelerate some on this turn
					car->CarSpeed += fps * SLOTCAR_ACC;

			}
			else
				car->CarSpeed += fps * SLOTCAR_ACC;

			if (car->CarSpeed > oldSpeed)
				ActivateCarLights(car);
			else
				NukeCarLights(car);
		}

				/* KEEP CARS FROM GOING TOO FAST */

		if (car->CarSpeed > MAX_CAR_SPEED)
			car->CarSpeed = MAX_CAR_SPEED;
	}
			/*********************/
			/* MOVE ALONG SPLINE */
			/*********************/

	oldSplinePlacement = car->SplinePlacement;						// rmemebre prev position
	IncreaseSplineIndex(car, car->CarSpeed);


			/* SEE IF LAPPED */

	if (car->SplinePlacement < oldSplinePlacement)				// we know it laps if the new placement is suddenly less than it was which means it wrapped
	{
		gSlotCarLap[carNum]++;									// inc lap counter

		if (gSlotCarLap[carNum] < MAX_LAPS)						// see if lapped but didn't win yet
		{

			if (carNum == 0)									// chipmunk talks each lap for player
			{
				if (gSlotCarLap[0] > gSlotCarLap[1])			// see if winning
					PlayEffect(EFFECT_CHIP_WINNING);
				else
					PlayEffect(EFFECT_CHIP_SAMWINNING);
			}
		}

				/**************************/
				/* THIS CAR JUST FINISHED */
				/**************************/
		else
		{
			NukeCarLights(car);									// make sure lights are off


					/***************/
					/* PLAYER DONE */
					/***************/

			if (carNum == 0)
			{
						/* PLAYER WON RACE */

				if (!gSnailWonSlotCarRace)							// see if player just won
				{
					car->CType = CTYPE_MISC | CTYPE_BLOCKCAMERA;	// don't trigger this car ever again
					gPlayerWonSlotCarRace = true;
					gNotifyOfSlotCarWin = true;
				}

						/* PLAYER LOST RACE */
				else
				{
					DoDialogMessage(DIALOG_MESSAGE_SLOTCARTRYAGAIN, 1, 4, &car->Coord);
				}
			}

					/**************/
					/* SNAIL DONE */
					/**************/
			else
			{
						/* SNAIL WON RACE */

				if (!gPlayerWonSlotCarRace)							// see if snail just won
					gSnailWonSlotCarRace = true;
			}
		}
	}

			/**********************/
			/* CALC Y-ROT BANKING */
			/**********************/


		/* CALC TRUE AIM VECTOR OF CURRENT LOCATION ON TRACK */

	GetCoordOnSpline(&(*gSplineList)[splineNum], car->SplinePlacement, &x1, &z1);			// get current coord
	GetCoordOnSpline2(&(*gSplineList)[splineNum], car->SplinePlacement, 5, &x2, &z2);		// get next coord
	trackAim.x = x2 - x1;																	// calc normalized aim vector
	trackAim.y = z2 - z1;
	FastNormalizeVector2D(trackAim.x, trackAim.y, &trackAim, true);

	{
		OGLPoint2D	oldButtPt = gButtPt[carNum];


				/* PULL BUTT TOWARD NEW DIRECTION */

		pinAim.x = x1 - gButtPt[carNum].x;									// calc vector to car's pin
		pinAim.y = z1 - gButtPt[carNum].y;
		FastNormalizeVector2D(pinAim.x, pinAim.y, &pinAim, true);

		gButtVector[carNum].x += pinAim.x * fps * 3.0f;						// shift aim toward car direction
		gButtVector[carNum].y += pinAim.y * fps * 3.0f;

		FastNormalizeVector2D(gButtVector[carNum].x, gButtVector[carNum].y, &gButtVector[carNum], true);


				/* CALC POINT WHERE BUTT SHOULD BE */

		gButtPt[carNum].x += gButtVector[carNum].x * gButtSpeed[carNum];	// move butt
		gButtPt[carNum].y += gButtVector[carNum].y * gButtSpeed[carNum];

		buttAim.x = gButtPt[carNum].x - x1;									// calc vector to butt
		buttAim.y = gButtPt[carNum].y - z1;
		FastNormalizeVector2D(buttAim.x, buttAim.y, &buttAim, true);

		gButtPt[carNum].x = x1 + (buttAim.x * BUTT_DIST);					// normalize butt coord
		gButtPt[carNum].y = z1 + (buttAim.y * BUTT_DIST);


				/* CALC BUTT-SLIDING SPEED */

		speed = CalcDistance(oldButtPt.x, oldButtPt.y, gButtPt[carNum].x, gButtPt[carNum].y) * .6f;
		if (speed > MAX_BUTT_SPEED)
			speed = MAX_BUTT_SPEED;

		if (speed > gButtSpeed[carNum])
			gButtSpeed[carNum] = speed;
		else
		{
			gButtSpeed[carNum] -= fps * 15.0f;		// butt friction
			if (gButtSpeed[carNum] < 0.0f)
				gButtSpeed[carNum] = 0.0f;
		}


					/* CALC Y-ROT */

		car->Rot.y = CalcYAngleFromPointToPoint(car->Rot.y, gButtPt[carNum].x, gButtPt[carNum].y, x1, z1);
		if (car->Rot.y > PI)
			car->Rot.y = -PI + (car->Rot.y - PI);

				/* SEE IF ADD FRICTION TO MOTION SPEED */

		if (car->CarSpeed > 200.0f)								// only do friction if going fast enough
		{
			r = car->Rot.y;
			curve.x = -sin(r);									// calc car aim vector
			curve.y = -cos(r);
			dot = OGLVector2D_Dot(&curve, &trackAim);			// calc dot between car aim and track aim (1.0 == going perfectly straight on track)
			angle = acos(dot);

			car->CarSpeed -= angle * 800.0f * fps;
		}
	}

				/* UPDATE EFFECT */

	if (carNum == 0)
	{
		if (car->CarSpeed < 20.0f)
			StopAChannel(&car->EffectChannel);
		else
		{
			if (car->EffectChannel == -1)
				car->EffectChannel = PlayEffect_Parms3D(EFFECT_SLOTCAR, &car->Coord, NORMAL_CHANNEL_RATE, 1.0);
			else
			{
				ChangeChannelRate(car->EffectChannel, (NORMAL_CHANNEL_RATE-0xa000) + (car->CarSpeed * 20.0f));
				Update3DSoundChannel(EFFECT_SLOTCAR, &car->EffectChannel, &car->Coord);
			}
		}
	}

}


/******************** ALIGN PLAYER ON SLOT CAR ***********************/

static void AlignPlayerOnSlotCar(ObjNode *player)
{
ObjNode	*car = gPlayerInfo.slotCar;
const OGLPoint3D off = {0, 21, 7};


			/***********************/
			/* KEEP ALIGNED ON CAR */
			/***********************/

	OGLPoint3D_Transform(&off, &car->BaseTransformMatrix, &gCoord);
	player->Rot.y = car->Rot.y;
	player->Rot.x = car->Rot.x;

		/* UPDATE */

	gPlayerInfo.coord = gCoord;
	UpdateObject(player);
}


#pragma mark -


/************************ UPDATE WHEEL ALIGNMENT **************************/

static void UpdateWheelAlignment(ObjNode *car)
{
ObjNode	*frontWheels, *rearWheels;
OGLPoint3D			off[2];
const OGLPoint3D	wheelOff[2] =
{
	0, 8, -6,						// front wheels
	0, 8, 54,						// rear wheels
};

	frontWheels = car->ChainNode;
	rearWheels = frontWheels->ChainNode;

	OGLPoint3D_TransformArray(wheelOff, &car->BaseTransformMatrix, off, 2);

	rearWheels->Rot.y = frontWheels->Rot.y = car->Rot.y;

	frontWheels->Rot.x -= car->CarSpeed * gFramesPerSecondFrac * .06f;
	rearWheels->Rot.x -= car->CarSpeed * gFramesPerSecondFrac * .06f;

	frontWheels->Coord = off[0];
	rearWheels->Coord = off[1];

	UpdateObjectTransforms(frontWheels);
	UpdateObjectTransforms(rearWheels);
}


/***************** PUT SNAIL ON CAR *********************/

static void PutSnailOnCar(ObjNode *rearWheels)
{
ObjNode *snail;

			/* MAKE SNAIL */

	snail = MakeSnail(rearWheels->Slot+1, rearWheels->Coord.x, rearWheels->Coord.z, SNAIL_KIND_SLOTCAR, 0, 0, false);

	snail->MoveCall = MoveSnailOnSlotCar;

	rearWheels->ChainNode = snail;							// rearWheels links to snail
	snail->ChainNode->ChainHead = rearWheels;				// snail's shell links back to rearWheels

}


/*********************** MOVE SNAIL ON SLOT CAR ****************************/

static void MoveSnailOnSlotCar(ObjNode *snail)
{
float	dist;
ObjNode	*car,*shell;
const OGLPoint3D snailOff = {0, 29, 26};

	shell = snail->ChainNode;
	car = shell->ChainHead->ChainHead->ChainHead;			// get rearWheels->frontWheels->car


			/***************************************/
			/* SEE IF WAITING FOR PLAYER TO GET ON */
			/***************************************/

	if (gSlotCarRacingMode == SLOTCAR_RACEMODE_IDLE)
	{
		dist = CalcQuickDistance(snail->Coord.x, snail->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
		if (dist < 300.0f)
		{
			DoDialogMessage(DIALOG_MESSAGE_SLOTCAR, 1, 4, &snail->Coord);

		}
	}


			/***********************/
			/* KEEP ALIGNED ON CAR */
			/***********************/

	OGLPoint3D_Transform(&snailOff, &car->BaseTransformMatrix, &snail->Coord);
	snail->Rot.y = car->Rot.y;
	snail->Rot.x = car->Rot.x;


			/* UPDATE SNAIL & SHELL */


	UpdateObjectTransforms(snail);
	AlignShellOnSnail(snail, shell);
}



#pragma mark -

/******************** UPDATE SLOTCAR RACING **************************/
//
// Called from MoveEverything()
//

void UpdateSlotCarRacing(void)
{
			/**************************/
			/* UPDATE START COUNTDOWN */
			/**************************/

	if (gSlotCarRacingMode == SLOTCAR_RACEMODE_START)
	{
		float	oldTimer = gSlotCarStartTimer;
		gSlotCarStartTimer -= gFramesPerSecondFrac;

				/* SEE IF DO "READY" */

		if ((oldTimer > 2.0f) && (gSlotCarStartTimer <= 2.0))
		{
			if (gDialogSoundChannel != -1)						// stop any existing chipmunk voice
				StopAChannelIfEffectNum(&gDialogSoundChannel, gDialogSoundEffect);

			PlayEffect(EFFECT_CHIP_READY);
		}


				/* SEE IF DO "SET" */

		else
		if ((oldTimer > 1.0f) && (gSlotCarStartTimer <= 1.0))
		{
			PlayEffect(EFFECT_CHIP_SET);
		}

				/* SEE IF DO "GO!" */

		else
		if ((oldTimer > 0.0f) && (gSlotCarStartTimer <= 0.0))
		{
			gSlotCarRacingMode = SLOTCAR_RACEMODE_RACE;								// start racing!
			gSlotCarLap[0] = gSlotCarLap[1] = 0;
			PlayEffect(EFFECT_CHIP_GO);
		}
	}

}


/********************* PLAYER EXITS CAR *****************************/

static void PlayerExitsSlotCar(ObjNode *car)
{
ObjNode	*player = gPlayerInfo.objNode;
float	r;

#pragma unused (car)

	if (player->Skeleton->AnimNum != PLAYER_ANIM_DRIVESLOTCAR)		// make sure Skip is still on car
		return;

	SetPlayerJumpAnim(player, true);

	r = player->Rot.y;
	player->Delta.x = -sin(r) * 400.0f;
	player->Delta.z = -cos(r) * 400.0f;

	player->Delta.y = 500.0f;

	StopAChannel(&car->EffectChannel);

}


#pragma mark -



/************************* ADD FINISH LINE *********************************/

Boolean AddFinishLine(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= PLAYROOM_ObjType_FinishLine;
	gNewObjectDefinition.scale 		= 1.9;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 431;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI/2);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	return(true);													// item was added
}


















