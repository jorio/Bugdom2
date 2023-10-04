/****************************/
/*   	RIDEBALL.C		    */
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

static void MoveRideBall(ObjNode *theNode);
static Boolean DoTrig_RideBall(ObjNode *door, ObjNode *who, Byte sideBits);
static void MoveRideBall(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_BALL_SPEED	1000.0f

#define	MIN_RIDE_SPEED	230.0f
#define	RIDE_FRICTION	400.0f
#define	RIDE_FRICTION_WATER	1500.0f
#define	COAST_FRICTION	500.0f
#define	RIDE_ACCEL		200.0f


/*********************/
/*    VARIABLES      */
/*********************/

Boolean	gResetRideBall = false;

#define	InitRotY		SpecialF[0]


/************************* ADD RIDE BALL *********************************/

Boolean AddRideBall(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*ball;
Boolean	playroom = (gLevelNum == LEVEL_NUM_PLAYROOM);

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	if (playroom)
		gNewObjectDefinition.type 		= PLAYROOM_ObjType_Baseball;
	else
		gNewObjectDefinition.type 		= SIDEWALK_ObjType_RideBall;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= PLAYER_SLOT-1;
	gNewObjectDefinition.moveCall 	= MoveRideBall;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8.0f);
	gNewObjectDefinition.scale 		= 40.0f;
	ball = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	ball->TerrainItemPtr = itemPtr;								// keep ptr to item list

	ball->Coord.y -= ball->BBox.min.y * ball->Scale.x;					// adjust so flush on ground
	UpdateObjectTransforms(ball);

	ball->InitRotY = ball->Rot.y;

			/* SET COLLISION STUFF */

	ball->CType 	= CTYPE_TRIGGER|CTYPE_BLOCKSHADOW|CTYPE_BLOCKCAMERA;	// note: dont set MISC since we don't want the berries pushing off this
	ball->CBits		= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(ball,1,1);

	ball->TriggerCallback = DoTrig_RideBall;

	ball->BoundingSphereRadius = ball->RightOff;				// set this to be accurate

	ball->What = WHAT_RIDEBALL;


			/* MAKE SHADOW */

	AttachShadowToObject(ball, 0, 6,6, false);

	return(true);
}


/******************** MOVE RIDE BALL ******************/

static void MoveRideBall(ObjNode *ball)
{
float	fps = gFramesPerSecondFrac;
OGLMatrix3x3		m;
static OGLPoint2D origin = {0,0};
float	force = 0;
float	rot = 0;
float	waterY;
ObjNode	*player = gPlayerInfo.objNode;

				/* SEE IF GONE */

	if (TrackTerrainItem(ball) || gResetRideBall)
	{
		ball->Coord = ball->InitCoord;					// don't delete the ball, just make sure it's @ it's init coord
		ball->Rot.y = ball->InitRotY;
		ball->AccelVector.x = 0;
		ball->AccelVector.y = 0;
		ball->Speed2D = 0;
		ball->StatusBits &= ~STATUS_BIT_UNDERWATER;
		gResetRideBall = false;
		return;
	}


	GetObjectInfo(ball);


				/*************************/
				/* HANDLE PLAYER CONTROL */
				/*************************/

	if (gPlayerInfo.ridingBall == ball)
	{
		if (gLevelNum != LEVEL_NUM_SIDEWALK)							// can only ride on sidewalk level
		{
			rot = RandomFloat2() * PI;									// pick some angle
			goto jump_off;
		}

				/* CALC ACCEL VECTOR */

		if ((gPlayerInfo.analogControlX == 0.0f) && (gPlayerInfo.analogControlZ == 0.0f))	// see if not acceling
		{
			ball->AccelVector.x = ball->AccelVector.y = 0;
			ball->Speed2D -= fps * RIDE_FRICTION;											// decelerate / friction
			if (ball->Speed2D < MIN_RIDE_SPEED)
				if (!(ball->StatusBits & STATUS_BIT_UNDERWATER))							// if not in water then keep going min speed
					ball->Speed2D = MIN_RIDE_SPEED;
		}
		else
		{
			OGLMatrix3x3_SetRotateAboutPoint(&m, &origin, gPlayerToCameraAngle);			// make a 2D rotation matrix camera-rel
			ball->AccelVector.x = gPlayerInfo.analogControlX;
			ball->AccelVector.y = gPlayerInfo.analogControlZ;
			OGLVector2D_Transform(&ball->AccelVector, &m, &ball->AccelVector);				// rotate the acceleration vector



			float maxBallSpeed = MAX_BALL_SPEED;

					/* GAMEPAD: CAP MAX SPEED TO THUMBSTICK MAGNITUDE FOR PRECISE CONTROL */

			if (!gPlayerInfo.analogIsMouse)
			{
				float analogMagnitude = sqrtf(SQUARED(gPlayerInfo.analogControlX) + SQUARED(gPlayerInfo.analogControlZ));

				if (analogMagnitude > EPS && analogMagnitude < 1.0f - EPS)
				{
					// Floor speed to 0.5x so we walk a reasonable pace when gently pushing the thumbstick.
					analogMagnitude = SDL_clamp(analogMagnitude, 0.5f, 1.0f);

					// Cap max speed to analog magnitude
					maxBallSpeed *= analogMagnitude;
				}
			}


					/* ACCELERATE */

			ball->Speed2D += fps * RIDE_ACCEL;

			if (ball->Speed2D > maxBallSpeed)
				ball->Speed2D = maxBallSpeed;
		}

			/* TURN  BALL TOWARD ACCEL VECTOR */

		TurnObjectTowardTarget(ball, &gCoord, gCoord.x+ball->AccelVector.x, gCoord.z+ball->AccelVector.y, .5f + ball->Speed2D * .003f, false);

		gPlayerInfo.invincibilityTimer -= fps;
	}

				/***********************************************/
				/* PLAYER IS NOT RIDING, SO DECELERATE TO STOP */
				/***********************************************/
	else
	{
		ball->Speed2D -= fps * COAST_FRICTION;													// decelerate / friction
		if (ball->Speed2D < 0.0f)
			ball->Speed2D = 0.0f;
	}


				/***********/
				/* MOVE IT */
				/***********/

	force = ball->Speed2D;
	rot = ball->Rot.y;
	gDelta.x = -sin(rot) * force;
	gDelta.z = -cos(rot) * force;

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y = GetTerrainY(gCoord.x, gCoord.z) - (ball->BBox.min.y * ball->Scale.x);					// adjust so flush on ground

				/* COLLISION */

	if (HandleCollisions(ball, CTYPE_MISC | CTYPE_FENCE, .4))
	{
		ball->Rot.y = CalcYAngleFromPointToPoint(rot, gCoord.x, gCoord.z, gCoord.x + gDelta.x, gCoord.z + gDelta.z);	// gotta do some manual work here to bounce the ball off anything it hit
	}


			/* SEE IF IN WATER */

	if (GetWaterY(gCoord.x, gCoord.z, &waterY))						// see if over water at all
	{
		if (ball->CollisionBoxes[0].bottom < waterY)				// see if in the water
		{
			ball->Speed2D -= fps * RIDE_FRICTION_WATER;				// decelerate / friction
			if (ball->Speed2D <= 0.0f)								// see if stopped
			{
				ball->Speed2D = 0.0f;

						/* MAKE PLAYER JUMP OFF AUTOMATICALLY */

				if (ball == gPlayerInfo.ridingBall)					// see if player is on this ball
				{
jump_off:
					player->Delta.y = 1600;
					player->Delta.x = sin(rot) * -700.0f;
					player->Delta.z = cos(rot) * -700.0f;

					gPlayerInfo.ridingBall = nil;
					SetPlayerJumpAnim(player, true);
					gDoGlidingAtApex = false;
				}
			}
			ball->StatusBits |= STATUS_BIT_UNDERWATER;
		}
	}




				/**********/
				/* UPDATE */
				/**********/

	ball->Rot.x -= ball->Speed2D * .01f * fps;
	UpdateObject(ball);


}



/************** DO TRIGGER - RIDE BALL ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_RideBall(ObjNode *ball, ObjNode *who, Byte sideBits)
{
			/* ONLY IF LANDED ON TOP OF BALL */

	if (sideBits & SIDE_BITS_BOTTOM)
	{

				/* NOT IF IN CERTAIN ANIMS */

		switch(who->Skeleton->AnimNum)
		{
			case	PLAYER_ANIM_GOTHIT_BACKFLIP:
			case	PLAYER_ANIM_GETUPFROMHIT_BACKFLIP:
			case	PLAYER_ANIM_GOTHIT_GENERIC:
					break;

			default:
					MorphToSkeletonAnim(who->Skeleton, PLAYER_ANIM_WALKONBALL, 8);
					gPlayerInfo.ridingBall = ball;
		}
	}

	return(true);
}


#pragma mark -

/******************** MOVE PLAYER: WALK ON BALL ***********************/

void MovePlayer_WalkOnBall(ObjNode *player)
{
ObjNode	*ball = gPlayerInfo.ridingBall;

			/********************************/
			/* UPDATE PLAYER WITH BALL INFO */
			/********************************/

	player->Skeleton->AnimSpeed = ball->Speed2D * .004f;

	gDelta = ball->Delta;

	gCoord.x = ball->Coord.x;
	gCoord.z = ball->Coord.z;
	gCoord.y = ball->Coord.y + (ball->BBox.max.y * ball->Scale.x) - player->BBox.min.y;

	gPlayerInfo.coord = gCoord;
	gPlayerInfo.coord.y -= 100.0f;		// move this down so camera will look lower

	player->Rot.y = ball->Rot.y;


			/***************************/
			/* SEE IF PLAYER WANTS OFF */
			/***************************/

	if (IsNeedDown(kNeed_Jump))										// see if user pressed the key
	{
		gDelta.y = 1600;
		gPlayerInfo.ridingBall = nil;
		SetPlayerJumpAnim(player, true);
		gDoGlidingAtApex = false;
	}


			/* UPDATE */

	player->StatusBits |= STATUS_BIT_ONGROUND;						// force this
	UpdateObject(player);
	HandlePlayerLineMarkerCrossing(player);
}











