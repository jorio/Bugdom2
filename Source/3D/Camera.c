/****************************/
/*   	CAMERA.C    	    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <AGL/aglmacro.h>

#include "3DMath.h"
#include "tunnel.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	float					gFramesPerSecond,gFramesPerSecondFrac,gCurrentAspectRatio;
extern	OGLVector3D				gWorldSunDirection;
extern	CollisionRec			gCollisionList[];
extern	FSSpec					gDataSpec;
extern	OGLMatrix4x4			gWorldToWindowMatrix;
extern	float					gGlobalTransparency,gTerrainTileDepth,gTerrainPolygonSize, gAnaglyphEyeSeparation;
extern	SpriteType				*gSpriteGroupList[];
extern	OGLColorRGB			gGlobalColorFilter;
extern	int						gLevelNum;
extern	float					gScratchF,gPlayerTunnelIndex,gTerrainSuperTileUnitSize;
extern	PrefsType			gGamePrefs;
extern	TunnelSplinePointType	*gTunnelSplinePoints;
extern	int				gGameWindowWidth, gGameWindowHeight,gScratch;
extern	Boolean					gSlowCPU;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void ResetCameraSettings(void);
static void UpdateCamera_Terrain(void);
static void UpdateCamera_Tunnel(void);
static void UpdateCamera_TopView(void);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	CAM_MINY			60.0f

#define	CAMERA_CLOSEST		75.0f
#define	CAMERA_FARTHEST		400.0f

#define	NUM_FLARE_TYPES		4
#define	NUM_FLARES			6

#define	CAMERA_DEFAULT_DIST_FROM_ME	350.0f
#define	DEFAULT_CAMERA_YOFF			100.0f

enum
{
	CAMERA_MODE_NORMAL,
	CAMERA_MODE_2,
	CAMERA_MODE_3,
	CAMERA_MODE_4,

	MAX_CAMERA_MODES
};

/*********************/
/*    VARIABLES      */
/*********************/

static OGLCameraPlacement	gAnaglyphCameraBackup;		// backup of original camera info before offsets applied

Boolean				gDrawLensFlare = true, gFreezeCameraFromXZ = false, gFreezeCameraFromY = false;

float				gCameraStartupTimer;

float				gPlayerToCameraAngle = 0.0f;
static float		gCameraLookAtAccel,gCameraFromAccelY,gCameraFromAccel;
float				gCameraDistFromMe, gCameraHeightFactor,gCameraLookAtYOff;
float				gMinHeightOffGround, gTopCamDist, gMaxCameraHeightOff;
Byte				gCameraMode;

static OGLPoint3D	gSunCoord;

static const float	gFlareOffsetTable[]=
{
	1.0,
	.6,
	.3,
	1.0/8.0,
	-.25,
	-.5
};


static const float	gFlareScaleTable[]=
{
	.3,
	.06,
	.1,
	.2,
	.03,
	.1
};

static const Byte	gFlareImageTable[]=
{
	PARTICLE_SObjType_LensFlare0,
	PARTICLE_SObjType_LensFlare1,
	PARTICLE_SObjType_LensFlare2,
	PARTICLE_SObjType_LensFlare3,
	PARTICLE_SObjType_LensFlare2,
	PARTICLE_SObjType_LensFlare1
};


/*********************** DRAW LENS FLARE ***************************/

void DrawLensFlare(OGLSetupOutputType *setupInfo)
{
short			i;
float			x,y,dot;
OGLPoint3D		sunScreenCoord,from;
float			cx,cy;
float			dx,dy,length;
OGLVector3D		axis,lookAtVector,sunVector;
static OGLColorRGBA	transColor = {1,1,1,1};
int				px,py,pw,ph;
AGLContext 		agl_ctx = gGameViewInfoPtr->drawContext;

	if (!gDrawLensFlare)
		return;

	if (gSlowCPU)					// no lens flares if slow
		return;


			/************/
			/* SET TAGS */
			/************/

	OGL_PushState();

	OGL_DisableLighting();												// Turn OFF lighting
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	SetColor4f(1,1,1,1);										// full white & alpha to start with


			/* CALC SUN COORD */

	from = setupInfo->cameraPlacement.cameraLocation;
	gSunCoord.x = from.x - (gWorldSunDirection.x * setupInfo->yon);
	gSunCoord.y = from.y - (gWorldSunDirection.y * setupInfo->yon);
	gSunCoord.z = from.z - (gWorldSunDirection.z * setupInfo->yon);



	/* CALC DOT PRODUCT BETWEEN VIEW AND LIGHT VECTORS TO SEE IF OUT OF RANGE */

	FastNormalizeVector(from.x - gSunCoord.x,
						from.y - gSunCoord.y,
						from.z - gSunCoord.z,
						&sunVector);

	FastNormalizeVector(setupInfo->cameraPlacement.pointOfInterest.x - from.x,
						setupInfo->cameraPlacement.pointOfInterest.y - from.y,
						setupInfo->cameraPlacement.pointOfInterest.z - from.z,
						&lookAtVector);

	dot = OGLVector3D_Dot(&lookAtVector, &sunVector);
	if (dot >= 0.0f)
		goto bye;

	dot = acos(dot) * -2.0f;				// get angle & modify it
	transColor.a = cos(dot);				// get cos of modified angle


			/* CALC SCREEN COORDINATE OF LIGHT */

	OGLPoint3D_Transform(&gSunCoord, &gWorldToWindowMatrix, &sunScreenCoord);


				/*************************/
				/* SEE IF SUN IS BLOCKED */
				/*************************/

#if 0
	x2 = sunScreenCoord.x;
	y2 = sunScreenCoord.y;

	if ((x2 >= 0.0f) && (x2 < gGameWindowWidth) &&			// see if center is in window
		(y2 >= 0.0f) && (y2 < gGameWindowHeight))
	{
		GLfloat	zbuffer;

					/* SEE IF CENTER IS BLOCKED */

		y2 = gGameWindowHeight - y2;											// flip y since 0,0 is bottom left

		glReadPixels(x2, y2, 1,1, GL_DEPTH_COMPONENT, GL_FLOAT, &zbuffer);		// read z-buffer to see if flare is blocked

//		gScratchF = zbuffer;	//---------
		if (zbuffer < .998f)
			goto bye;
	}
#endif

			/* CALC CENTER OF VIEWPORT */

	OGL_GetCurrentViewport(setupInfo, &px, &py, &pw, &ph);
	cx = pw/2 + px;
	cy = ph/2 + py;


			/* CALC VECTOR FROM CENTER TO LIGHT */

	dx = sunScreenCoord.x - cx;
	dy = sunScreenCoord.y - cy;
	length = sqrt(dx*dx + dy*dy);
	FastNormalizeVector(dx, dy, 0, &axis);


			/***************/
			/* DRAW FLARES */
			/***************/

			/* INIT MATRICES */

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	for (i = 0; i < NUM_FLARES; i++)
	{
		float	sx,sy,o,fx,fy;

		if (i == 0)
			gGlobalTransparency = .99;		// sun is always full brightness (leave @ < 1 to ensure GL_BLEND)
		else
			gGlobalTransparency = transColor.a;

		MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_PARTICLES][gFlareImageTable[i]].materialObject, setupInfo);		// activate material



		if (i == 1)												// always draw sun, but fade flares based on dot
		{
			if (transColor.a <= 0.0f)							// see if faded all out
				break;
			SetColor4fv((float *)&transColor);
		}


		o = gFlareOffsetTable[i];
		sx = gFlareScaleTable[i];
		sy = sx * gCurrentAspectRatio;

		x = cx + axis.x * length * o;
		y = cy + axis.y * length * o;

		fx = x / (pw/2) - 1.0f;
		fy = (ph-y) / (ph/2) - 1.0f;

		glBegin(GL_QUADS);
		glTexCoord2f(0,0);	glVertex2f(fx - sx, fy - sy);
		glTexCoord2f(1,0);	glVertex2f(fx + sx, fy - sy);
		glTexCoord2f(1,1);	glVertex2f(fx + sx, fy + sy);
		glTexCoord2f(0,1);	glVertex2f(fx - sx, fy + sy);
		glEnd();
	}

			/* RESTORE MODES */

bye:
	gGlobalTransparency = 1.0f;
	OGL_PopState();

}


//===============================================================================================================================================================

#pragma mark -

/*************** INIT CAMERA: TERRAIN ***********************/
//
// This MUST be called after the players have been created so that we know
// where to put the camera.
//

void InitCamera_Terrain(void)
{
ObjNode	*playerObj = gPlayerInfo.objNode;

	ResetCameraSettings();


			/******************************/
			/* SET CAMERA STARTING COORDS */
			/******************************/

	if (playerObj)
	{
		float	dx,dz,x,z,r;

		r = playerObj->Rot.y;

		dx = sin(r) * gCameraDistFromMe;
		dz = cos(r) * gCameraDistFromMe;

		x = gGameViewInfoPtr->cameraPlacement.cameraLocation.x = playerObj->Coord.x + dx;
		z = gGameViewInfoPtr->cameraPlacement.cameraLocation.z = playerObj->Coord.z + dz;
		gGameViewInfoPtr->cameraPlacement.cameraLocation.y = GetTerrainY(x, z) + 200.0f;

		gGameViewInfoPtr->cameraPlacement.pointOfInterest.x = playerObj->Coord.x;
		gGameViewInfoPtr->cameraPlacement.pointOfInterest.y = playerObj->Coord.y + gCameraLookAtYOff;
		gGameViewInfoPtr->cameraPlacement.pointOfInterest.z = playerObj->Coord.z;
	}
}


/*************** INIT CAMERA: TUNNEL ***********************/

void InitCamera_Tunnel(void)
{
ObjNode	*playerObj = gPlayerInfo.objNode;

	ResetCameraSettings();


			/******************************/
			/* SET CAMERA STARTING COORDS */
			/******************************/


	if (playerObj)
	{
		int		i = gPlayerTunnelIndex - 100.0f;

		if (i < 0.0f)
			i = 0;

		gGameViewInfoPtr->cameraPlacement.cameraLocation = gTunnelSplinePoints[i].point;
		gGameViewInfoPtr->cameraPlacement.pointOfInterest = playerObj->Coord;

	}



}


/******************** RESET CAMERA SETTINGS **************************/

static void ResetCameraSettings(void)
{
	gCameraMode = CAMERA_MODE_NORMAL;

	gTopCamDist = 300.0f;

	gCameraFromAccel 	= 4.0;
	gCameraFromAccelY	= 2.9;


	gMinHeightOffGround = 60;

			/* SPECIAL SETTINGS FOR SAUCER LEVEL */

	gCameraLookAtAccel 	= 10.0;

	gCameraHeightFactor = 0.2;
	gMaxCameraHeightOff = 300.0f;
	gCameraLookAtYOff 	= DEFAULT_CAMERA_YOFF;

	gCameraDistFromMe 	= CAMERA_DEFAULT_DIST_FROM_ME;

	gFreezeCameraFromXZ = gFreezeCameraFromY = false;
}



/*************** UPDATE CAMERA ***************/

void UpdateCamera(void)
{
	/*************************************************/
	/* UPDATE BASED ON THE TYPE OF LEVEL WE'RE DOING */
	/*************************************************/

	switch(gLevelNum)
	{
			/* TUNNEL LEVELS */

		case	LEVEL_NUM_PLUMBING:
		case	LEVEL_NUM_GUTTER:
				UpdateCamera_Tunnel();
				break;


			/* BALSA FLYING */

		case	LEVEL_NUM_BALSA:
				UpdateCamera_TopView();
				break;

			/* EXPLORATION LEVELS */

		default:
				UpdateCamera_Terrain();
	}
}


/******************** UPDATE CAMERA: TERRAIN **************************/

static void UpdateCamera_Terrain(void)
{
float	fps = gFramesPerSecondFrac;
OGLPoint3D	from,to,target;
float		distX,distZ,distY,dist,fromAcc;
OGLVector2D	pToC;
float		myX,myY,myZ,delta;
ObjNode		*playerObj = gPlayerInfo.objNode;
SkeletonObjDataType	*skeleton;
float			oldCamX,oldCamZ,oldCamY,oldPointOfInterestX,oldPointOfInterestZ,oldPointOfInterestY;
ObjNode		*lookAtObj;
OGLMatrix3x3	m;
Boolean			snapTo = false;

	if (!playerObj)
		return;

	skeleton = playerObj->Skeleton;
	if (!skeleton)
		DoFatalAlert("\pMoveCamera: player has no skeleton!");


				/******************/
				/* GET COORD DATA */
				/******************/

	oldCamX = gGameViewInfoPtr->cameraPlacement.cameraLocation.x;				// get current/old cam coords
	oldCamY = gGameViewInfoPtr->cameraPlacement.cameraLocation.y;
	oldCamZ = gGameViewInfoPtr->cameraPlacement.cameraLocation.z;

	oldPointOfInterestX = gGameViewInfoPtr->cameraPlacement.pointOfInterest.x;
	oldPointOfInterestY = gGameViewInfoPtr->cameraPlacement.pointOfInterest.y;
	oldPointOfInterestZ = gGameViewInfoPtr->cameraPlacement.pointOfInterest.z;

	myX = gPlayerInfo.coord.x;
	myY = gPlayerInfo.coord.y + playerObj->BottomOff;
	myZ = gPlayerInfo.coord.z;

	gPlayerToCameraAngle = PI - CalcYAngleFromPointToPoint(PI-gPlayerToCameraAngle, myX, myZ, oldCamX, oldCamZ);	// calc angle of camera around player

			/* CALC PLAYER->CAMERA VECTOR */

	pToC.x = oldCamX - myX;												// calc player->camera vector
	pToC.y = oldCamZ - myZ;
	OGLVector2D_Normalize(&pToC,&pToC);									// normalize it


			/*****************************/
			/* SEE IF TOGGLE CAMERA MODE */
			/*****************************/

	if (gControlNeeds[kNeed_CameraMode].newButtonPress)
	{
		gCameraMode++;										// inc to next mode
		if (gCameraMode >= MAX_CAMERA_MODES)
			gCameraMode = CAMERA_MODE_NORMAL;

		switch(gCameraMode)
		{
			case	CAMERA_MODE_NORMAL:
					ResetCameraSettings();
					break;

			case	CAMERA_MODE_2:
					gCameraDistFromMe 	= CAMERA_DEFAULT_DIST_FROM_ME * 1.5f;
					gCameraHeightFactor = .7f;
					gMaxCameraHeightOff *= 2.0f;
					break;

			case	CAMERA_MODE_3:
					gCameraDistFromMe 	= CAMERA_DEFAULT_DIST_FROM_ME * 2.0f;
					gCameraHeightFactor = 1.0f;
					gMaxCameraHeightOff *= 2.0f;
					break;

			case	CAMERA_MODE_4:
					break;
		}
	}

	if (gCameraMode == CAMERA_MODE_4)
	{
		float	r = playerObj->Rot.y;
		float	s = sin(r);
		float	c = cos(r);

		from.x = playerObj->Coord.x + (s * 90.0f);
		from.z = playerObj->Coord.z + (c * 90.0f);
		from.y = playerObj->Coord.y + 110.0f;

		to.x = from.x - (s * 100.0f);
		to.z = from.z - (c * 100.0f);
		to.y = from.y - 30.0f;
		goto update;
	}



		/*******************************************/
		/* SEE IF THERE'S A LOOKAT OBJECT IN RANGE */
		/*******************************************/

	if ((gPlayerInfo.objNode->Delta.x == 0.0f) && (gPlayerInfo.objNode->Delta.z == 0.0f))		// only do auto-lookat if player isn't being controlled
	{
		lookAtObj = FindClosestCType(&gPlayerInfo.coord, CTYPE_LOOKAT, false);			// find closest "lookat" object if any
		if (lookAtObj)
		{
			dist = OGLPoint3D_Distance(&lookAtObj->Coord, &gPlayerInfo.coord);			// calc accurate distance
			if (dist < lookAtObj->ForceLookAtDist)										// make sure in range to do this
			{
				OGLVector2D	camToLook, camToPlayer;
				float		angle;

						/* CALC LOOK VECTORS */

				camToLook.x = lookAtObj->Coord.x - oldCamX;								// calc vector from camera->lookat
				camToLook.y = lookAtObj->Coord.z - oldCamZ;
				FastNormalizeVector2D(camToLook.x, camToLook.y, &camToLook, false);

				camToPlayer.x = lookAtObj->Coord.x - myX;								// calc vector from camera->player
				camToPlayer.y = lookAtObj->Coord.z - myZ;
				FastNormalizeVector2D(camToPlayer.x, camToPlayer.y, &camToPlayer, false);

						/* SEE IF MOVE CAMERA */

				angle = acos(OGLVector2D_Dot(&camToLook, &camToPlayer));					// calc angle between
				if (angle > .1f)
				{
					if (OGLVector2D_Cross(&camToLook, &camToPlayer) < 0.0f)					// calc cross prod to determine which way to swing the camera
						angle = -angle;

					OGLMatrix3x3_SetRotate(&m, angle);										// rot it
					OGLVector2D_Transform(&pToC, &m, &pToC);
				}
			}
			else
				lookAtObj = nil;
		}
	}
	else
		lookAtObj = nil;


			/**********************/
			/* CALC LOOK AT POINT */
			/**********************/

	target.x = myX;								// accelerate "to" toward target "to"
	target.y = myY + gCameraLookAtYOff;
	target.z = myZ;

	distX = target.x - oldPointOfInterestX;
	distY = target.y - oldPointOfInterestY;
	distZ = target.z - oldPointOfInterestZ;


				/* MOVE "TO" */

	delta = distX * (fps * gCameraLookAtAccel);										// calc delta to move
	if (fabs(delta) > fabs(distX))													// see if will overshoot
		delta = distX;
	to.x = oldPointOfInterestX + delta;												// move x

	delta = distY * (fps * gCameraLookAtAccel) * .7f;								// calc delta to move
	if (fabs(delta) > fabs(distY))													// see if will overshoot
		delta = distY;
	to.y = oldPointOfInterestY + delta;												// move y

	delta = distZ * (fps * gCameraLookAtAccel);										// calc delta to move
	if (fabs(delta) > fabs(distZ))													// see if will overshoot
		delta = distZ;
	to.z = oldPointOfInterestZ + delta;												// move z


			/*******************/
			/* CALC FROM POINT */
			/*******************/

		/* IF RAMMING THEN FORCE CAMERA BEHIND PLAYER */

	if (playerObj->Skeleton->AnimNum == PLAYER_ANIM_RAMMING)			// keep camera behind when ramming
	{
		float	rot = playerObj->Rot.y;
		dist = gCameraDistFromMe;
		target.x = myX + sin(rot) * dist;
		target.z = myZ + cos(rot) * dist;
		goto got_target;
	}

			/* IF CAMERA XZ FROZEN */

	if (gFreezeCameraFromXZ)
	{
		from.x = target.x = oldCamX;
		from.z = target.x = oldCamZ;
	}

			/* REGULAR CAMERA MOTION */

	else
	{


				/* ROTATE BY USER ROT */

		if (lookAtObj == nil)
		{
			if (gControlNeeds[kNeed_CameraLeft].value)
			{
				OGLMatrix3x3_SetRotate(&m, fps * -6.0f);
				OGLVector2D_Transform(&pToC, &m, &pToC);
				snapTo = true;
			}
			else
			if (gControlNeeds[kNeed_CameraRight].value)
			{
				OGLMatrix3x3_SetRotate(&m, fps * 6.0f);
				OGLVector2D_Transform(&pToC, &m, &pToC);
				snapTo = true;
			}
		}

		dist = gCameraDistFromMe;
		fromAcc = gCameraFromAccel;

		if (playerObj->Skeleton->AnimNum == PLAYER_ANIM_DRIVESLOTCAR)
		{
			dist *= .6f;
			fromAcc *= 2.0f;
		}


		if (playerObj->Skeleton->AnimNum == PLAYER_ANIM_WALKONBALL)			// move camera back when riding ball
			dist *= 1.5f;

		target.x = myX + (pToC.x * dist);									// target is appropriate dist based on cam's current coord
		target.z = myZ + (pToC.y * dist);

got_target:
				/* MOVE CAMERA TOWARDS POINT */

		distX = target.x - oldCamX;
		distZ = target.z - oldCamZ;

		if (snapTo)
		{
			from.x = target.x;
			from.z = target.z;
		}
		else
		{
			delta = distX * (fps * fromAcc);							// calc delta to move
			if (fabs(delta) > fabs(distX))								// see if will overshoot
				delta = distX;
			from.x = oldCamX + delta;									// move x

			delta = distZ * (fps * fromAcc);							// calc delta to move
			if (fabs(delta) > fabs(distZ))								// see if will overshoot
				delta = distX;
			from.z = oldCamZ + delta;									// move z
		}

	}



		/***************/
		/* CALC FROM Y */
		/***************/

	if (gFreezeCameraFromY)
	{
		from.y = oldCamY;
	}
	else
	{
		float	camBottom,fenceTopY,waterY,off;
		Boolean	overTop;
		OGLPoint3D	intersect;

		dist = CalcQuickDistance(from.x, from.z, to.x, to.z) - CAMERA_CLOSEST;
		if (dist < 0.0f)
			dist = 0.0f;

		off = CAM_MINY + (dist*gCameraHeightFactor);		// calc desired y based on dist and height factor
		if (off > gMaxCameraHeightOff)
			off = gMaxCameraHeightOff;

		target.y = myY + off;

		target.y += gPlayerInfo.distToFloor * .3f;			// raise camera if player is off ground

			/* CHECK CLEAR LINE OF SIGHT WITH FENCE */

		if (SeeIfLineSegmentHitsFence(&target, &to, &intersect,&overTop,&fenceTopY))			// see if intersection with fence
		{
			if (!overTop)
			{
				target.y = fenceTopY += 200.0f;
			}
		}


			/*******************************/
			/* MOVE ABOVE ANY SOLID OBJECT */
			/*******************************/
			//
			// well... not any solid object, just reasonable ones
			//

		camBottom = target.y - 100.0f;
		if (DoSimpleBoxCollision(target.y + 100.0f, camBottom,
								from.x - 150.0f, from.x + 150.0f,
								from.z + 150.0f, from.z - 150.0f,
								CTYPE_BLOCKCAMERA))
		{
			ObjNode *obj = gCollisionList[0].objectPtr;					// get collided object
			if (obj)
			{
				target.y = obj->CollisionBoxes[0].top + 100.0f;				// set target on top of object
			}
		}

		dist = (target.y - gGameViewInfoPtr->cameraPlacement.cameraLocation.y)*gCameraFromAccelY;	// calc dist from current y to desired y
		from.y = gGameViewInfoPtr->cameraPlacement.cameraLocation.y+(dist*fps);


				/* MAKE SURE NOT UNDERGROUND OR WATER */

		dist = GetTerrainY(from.x, from.z) + gMinHeightOffGround;
		if (from.y < dist)
			from.y = dist;

		if (GetWaterY(from.x, from.z, &waterY))
		{
			waterY += 50.0f;
			if (from.y < waterY)
				from.y = waterY;
		}
	}


				/**********************/
				/* UPDATE CAMERA INFO */
				/**********************/

update:
	OGL_UpdateCameraFromTo(gGameViewInfoPtr,&from,&to);


				/* UPDATE PLAYER'S CAMERA INFO */

	gPlayerInfo.camera.cameraLocation = from;
	gPlayerInfo.camera.pointOfInterest = to;
}


/**************************** UPDATE CAMERA:  TUNNEL ********************************/

static void UpdateCamera_Tunnel(void)
{
ObjNode	*playerObj = gPlayerInfo.objNode;

	if (playerObj)
	{
		float	i = gPlayerTunnelIndex - 25.0f;

		if (i < 0.0f)
			i = 0.0;

		CalcTunnelCoordFromIndex(i, &gGameViewInfoPtr->cameraPlacement.cameraLocation);
		if (gLevelNum == LEVEL_NUM_GUTTER)
			gGameViewInfoPtr->cameraPlacement.cameraLocation.y += 5.0f;
		else
			gGameViewInfoPtr->cameraPlacement.cameraLocation.y -= 12.0f;

		CalcTunnelCoordFromIndex(gPlayerTunnelIndex, &gGameViewInfoPtr->cameraPlacement.pointOfInterest);
		gGameViewInfoPtr->cameraPlacement.pointOfInterest.y -= 11.0f;

	}


}



/******************** UPDATE CAMERA: TOP VIEW **************************/

static void UpdateCamera_TopView(void)
{
float	fps = gFramesPerSecondFrac;
OGLPoint3D	from,to;
const OGLVector3D	up = {0,1,0};

			/* MOVE CAMERA TO FINAL HEIGHT */

	gTopCamDist += fps * 650.0f;
	if (gTopCamDist > 2200.0f)
		gTopCamDist = 2200.0f;


			/* SET CAM COORD */

	if (!gFreezeCameraFromXZ)
	{
		from.x = gPlayerInfo.coord.x;
		from.z = gPlayerInfo.coord.z + (.35f * gTopCamDist);
	}
	else
	{
		from.x = gPlayerInfo.camera.cameraLocation.x;
		from.z = gPlayerInfo.camera.cameraLocation.z;
	}

	if (!gFreezeCameraFromY)
		from.y = gPlayerInfo.coord.y + (.8f * gTopCamDist);
	else
		from.y = gPlayerInfo.camera.cameraLocation.y;


	to.x = gPlayerInfo.coord.x;
	to.y = gPlayerInfo.coord.y;
	to.z = gPlayerInfo.coord.z;


		/* PIN SO DOESN'T GET TOO CLOSE TO FENCE */

//	if (from.z < 2000.0f)
//		from.z = 2000.0f;
//	else
	if (from.z > 30500.0f)
		from.z = 30500.0f;

//	gScratch = from.z;	//-------


				/* UPDATE */

	OGL_UpdateCameraFromToUp(gGameViewInfoPtr,&from,&to, &up);

	gPlayerInfo.camera.cameraLocation = from;
	gPlayerInfo.camera.pointOfInterest = to;
}



#pragma mark -

/********************** PREP ANAGLYPH CAMERAS ***************************/
//
// Make a copy of the camera's real coord info before we do anaglyph offsetting.
//

void PrepAnaglyphCameras(void)
{
	gAnaglyphCameraBackup = gGameViewInfoPtr->cameraPlacement;

}

/********************** RESTORE CAMERAS FROM ANAGLYPH ***************************/

void RestoreCamerasFromAnaglyph(void)
{
	gGameViewInfoPtr->cameraPlacement = gAnaglyphCameraBackup;
}


/******************** CALC ANAGLYPH CAMERA OFFSET ***********************/

void CalcAnaglyphCameraOffset(short pass)
{
OGLVector3D	aim;
OGLVector3D	xaxis;
float		sep = gAnaglyphEyeSeparation;
const	OGLVector3D up = {0,1,0};

	if (pass > 0)
		sep = -sep;

			/* CALC CAMERA'S X-AXIS */

	aim.x = gAnaglyphCameraBackup.pointOfInterest.x - gAnaglyphCameraBackup.cameraLocation.x;
	aim.y = gAnaglyphCameraBackup.pointOfInterest.y - gAnaglyphCameraBackup.cameraLocation.y;
	aim.z = gAnaglyphCameraBackup.pointOfInterest.z - gAnaglyphCameraBackup.cameraLocation.z;
	OGLVector3D_Normalize(&aim, &aim);

	OGLVector3D_Cross(&up, &aim, &xaxis);


				/* OFFSET CAMERA FROM */

	gGameViewInfoPtr->cameraPlacement.cameraLocation.x = gAnaglyphCameraBackup.cameraLocation.x + (xaxis.x * sep);
	gGameViewInfoPtr->cameraPlacement.cameraLocation.z = gAnaglyphCameraBackup.cameraLocation.z + (xaxis.z * sep);


				/* OFFSET CAMERA TO */

	gGameViewInfoPtr->cameraPlacement.pointOfInterest.x = gAnaglyphCameraBackup.pointOfInterest.x + (xaxis.x * sep);
	gGameViewInfoPtr->cameraPlacement.pointOfInterest.z = gAnaglyphCameraBackup.pointOfInterest.z + (xaxis.z * sep);


}












