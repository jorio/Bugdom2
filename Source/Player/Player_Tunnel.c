/****************************/
/*     TUNNEL.C			    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/**********************/
/*     PROTOTYPES     */
/**********************/

static void MovePlayer_TunnelFallOff(ObjNode *player);
static void MovePlayer_Tunnel(ObjNode *player);
static void DoTunnelSpin(ObjNode *player);
static void UpdatePlayer_Tunnel(ObjNode *player);
static void MakeTunnelSpray(ObjNode *player);
static void MoveTunnelSpray(ObjNode *spray);
static void DoTunnelObjectCollision_Sewer(ObjNode *player);
static void DoTunnelObjectCollision_Gutter(ObjNode *player);
static void PlayerGotHitInTunnel(ObjNode *player);
static void ResetPlayerInTunnel(void);
static void PlayerFallsOffEdgeOfTunnel(ObjNode *player, Boolean isLeft);


/**********************/
/*     CONSTANTS      */
/**********************/

#define	PLAYER_TUNNEL_SCALE	(PLAYER_DEFAULT_SCALE * .11f)


#define	SPAR_DIAMATER				30.0f		// needs to match that in Tunnelizer tool!

#define	TUNNEL_ROT_GRAVITY			10.0f
#define	TUNNEL_ROT_FRICTION			.6f

#define	MIN_TUNNEL_SPEED			70.0f
#define	MAX_TUNNEL_SPEED			120.0f

#define	TUNNEL_ROT_SENSITIVITY		12.0f			// bigger == more sensitive / faster spin turn


#define	TUNNEL_PLAYER_JOINT_LEFTFOOT		22

/**********************/
/*     VARIABLES      */
/**********************/

int		gNumTunnelSplinePoints = 0;
int		gNumTunnelSections = 0;

TunnelSplinePointType	*gTunnelSplinePoints = nil;

MOVertexArrayObject	*gTunnelSectionMeshes[MAX_TUNNEL_SECTIONS];
MOVertexArrayObject	*gTunnelSectionWaterObjects[MAX_TUNNEL_SECTIONS];

MOMaterialObject	*gTunnelTextureObj = nil;
static	float		gTunnelWaterAnimIndex;

float	gTunnelWaterTextureScroll = 0;


int					gNumTunnelItems = 0;
TunnelItemDefType	*gTunnelItemList = nil;

float				gPlayerTunnelIndex;

static OGLPoint3D	gPreviousSprayCoord;


/******************** ARE WE PLAYING A TUNNEL LEVEL? *********************/

Boolean IsTunnelLevel(void)
{
	return gLevelNum == LEVEL_NUM_PLUMBING || gLevelNum == LEVEL_NUM_GUTTER;
}

/******************** INIT AREA:  TUNNELS ***************************/

void InitArea_Tunnels(void)
{
OGLSetupInputType	viewDef;

	gTunnelWaterAnimIndex = 0;

			/*************/
			/* MAKE VIEW */
			/*************/


	gDrawLensFlare = false;
	gAutoFadeStatusBits = STATUS_BIT_AUTOFADE;


			/* SET ANAGLYPH INFO */

	if (gGamePrefs.anaglyph)
	{
		gAnaglyphScaleFactor 	= 1.0f;
		gAnaglyphFocallength	= 35.0f * gAnaglyphScaleFactor;	// set camera info
		gAnaglyphEyeSeparation 	= 2.5f * gAnaglyphScaleFactor;
	}



			/* PLUMBING */

	if (gLevelNum == LEVEL_NUM_PLUMBING)
	{
		const OGLColorRGBA	fogColor = {.6,.7,.2,1};

		OGL_NewViewDef(&viewDef);
		viewDef.view.clearBackBuffer 	= true;
		viewDef.camera.hither 			= 8.0f;
		viewDef.camera.yon 				= 700.0f;
		viewDef.camera.fov 				= 1.3;

		viewDef.styles.useFog			= true;
		viewDef.styles.fogStart			= viewDef.camera.yon * .2f;
		viewDef.styles.fogEnd			= viewDef.camera.yon * .95;
		viewDef.view.clearColor 		= fogColor;

		gWorldSunDirection.x = 0;
		gWorldSunDirection.y = -1;
		gWorldSunDirection.z = .2;
		OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

		viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
		viewDef.lights.ambientColor.r 		= .4;
		viewDef.lights.ambientColor.g 		= .4;
		viewDef.lights.ambientColor.b 		= .4;
	}

		/* GUTTER */

	else
	{
		const OGLColorRGBA	fogColor = {1,1,1,1};

		OGL_NewViewDef(&viewDef);
		viewDef.view.clearBackBuffer 	= true;
		viewDef.camera.hither 			= 8.0f;
		if (gG4)
			viewDef.camera.yon 				= 1400.0f;
		else
			viewDef.camera.yon 				= 1000.0f;
		viewDef.camera.fov 				= 1.3;

		viewDef.styles.useFog			= true;
		viewDef.styles.fogStart			= viewDef.camera.yon * .6f;
		viewDef.styles.fogEnd			= viewDef.camera.yon * 1.9f;
		viewDef.view.clearColor 		= fogColor;

		gWorldSunDirection.x = .3;
		gWorldSunDirection.y = -.6;
		gWorldSunDirection.z = -.2;
		OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

		viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
		viewDef.lights.ambientColor.r 		= .5;
		viewDef.lights.ambientColor.g 		= .5;
		viewDef.lights.ambientColor.b 		= .4;
	}

	OGL_SetupWindow(&viewDef, &gGameView);


			/*************/
			/* LOAD DATA */
			/*************/

	LoadLevelArt_Tunnel();
	InitInfobar();

			/* INIT OTHER MANAGERS */

	InitEnemyManager();
	InitEffects();
	InitDialogManager();						// init all help messages


	/* INIT THE TUNNEL ITEMS, PLAYER & RELATED STUFF */

	InitTunnelItems();

	InitPlayerAtStartOfLevel_Tunnel();

	InitCamera_Tunnel();


			/****************/
			/* INIT SPECIAL */
			/****************/

	switch(gLevelNum)
	{

		case	LEVEL_NUM_PLUMBING:
				DoDialogMessage(DIALOG_MESSAGE_PLUMBINGINTRO, 1, 6.0, nil);
				break;


	}
 }



/********************** DISPOSE TUNNEL DATA ************************/

void DisposeTunnelData(void)
{
int		i;

		/* DISPOSE OF ALL GEOMETRY DATA */

	for (i = 0; i < gNumTunnelSections; i++)
	{
		MO_DisposeObjectReference(gTunnelSectionMeshes[i]);
		gTunnelSectionMeshes[i] = nil;

		MO_DisposeObjectReference(gTunnelSectionWaterObjects[i]);
		gTunnelSectionWaterObjects[i] = nil;
	}
	gNumTunnelSections = 0;

		/* DISPOSE OF EXTRA TEXTURE REFS */

	if (gTunnelTextureObj)
	{
		MO_DisposeObjectReference(gTunnelTextureObj);
		gTunnelTextureObj = nil;
	}


		/* FREE THE BG3D DATA */

	DisposeBG3DContainer(MODEL_GROUP_LEVELSPECIFIC);


		/* FREE SPLINE POINTS */

	if (gTunnelSplinePoints)
	{
		SafeDisposePtr(gTunnelSplinePoints);
		gTunnelSplinePoints = nil;
	}


		/* FREE ITEM LIST */

	if (gTunnelItemList)
	{
		SafeDisposePtr(gTunnelItemList);
		gTunnelItemList = nil;
		gNumTunnelItems = 0;
	}
}


/********************* INIT TUNNEL ITEMS ***************************/
//
// Scan list of tunnel items and make objects
//

void InitTunnelItems(void)
{
int		i;
ObjNode	*newObj;
OGLPoint3D	splinePt;

	CreateCyclorama();

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;				// they all come from this group!
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;


	for (i = 0; i < gNumTunnelItems; i++)
	{
				/* CALC COORD OF THIS ITEM */

		CalcTunnelCoordFromIndex(gTunnelItemList[i].splineIndex, &splinePt);				// get spline coord

		gNewObjectDefinition.coord.x = splinePt.x + gTunnelItemList[i].positionOffset.x;	// offset to get item's coord
		gNewObjectDefinition.coord.y = splinePt.y + gTunnelItemList[i].positionOffset.y;
		gNewObjectDefinition.coord.z = splinePt.z + gTunnelItemList[i].positionOffset.z;

		gNewObjectDefinition.type 		= gTunnelItemList[i].type;
		gNewObjectDefinition.slot 		= 200 + gNewObjectDefinition.type;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= gTunnelItemList[i].scale;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Rot = gTunnelItemList[i].rot;												// set item's rot
		UpdateObjectTransforms(newObj);
	}
}


#pragma mark -


/**************** PLAY AREA: TUNNEL  ************************/
//
// The main loop for a tunnel slide level
//

void PlayArea_Tunnel(void)
{

		/******************/
		/* MAIN GAME LOOP */
		/******************/

	while(true)
	{
				/* GET CONTROL INFORMATION FOR THIS FRAME */

		UpdateInput();


				/* MOVE OBJECTS */

		MoveEverything();


			/* DRAW IT ALL */

		OGL_DrawScene(DrawTunnel);


			/**************/
			/* MISC STUFF */
			/**************/

			/* SEE IF PAUSED */

		if (IsNeedDown(kNeed_UIPause))
			DoPaused();

		CalcFramesPerSecond();

		gGameFrameNum++;

				/* CHEATS */

#if !_DEBUG
		if (IsKeyActive(SDL_SCANCODE_LGUI) || IsKeyActive(SDL_SCANCODE_RGUI))
#endif
		{
			if (IsKeyDown(SDL_SCANCODE_F10))			// win level cheat
				break;
		}

		if (IsCheatKeyComboDown())						// get health & stuff
		{
			gPlayerInfo.health = 1.0;
			gPlayerInfo.glidePower = 1.0;
			if (gPlayerInfo.lives < 3)
				gPlayerInfo.lives = 3;
		}


				/* SEE IF TRACK IS COMPLETED */

		if (gGameOver)													// if we need immediate abort, then bail now
			break;

		if (gLevelCompleted)
		{
			gLevelCompletedCoolDownTimer -= gFramesPerSecondFrac;		// game is done, but wait for cool-down timer before bailing
			if (gLevelCompletedCoolDownTimer <= 0.0f)
				break;
		}

				/* SEE IF RESET PLAYER NOW */

		if (gPlayerIsDead)
		{
			float	oldTimer = gDeathTimer;
			gDeathTimer -= gFramesPerSecondFrac;
			if (gDeathTimer <= 0.0f)
			{
				if (oldTimer > 0.0f)						// if just now crossed zero then start fade
					MakeFadeEvent(false, 1);
				else
				if (gGammaFadeFrac <= 0.0f)				// once fully faded out reset player @ checkpoint
					ResetPlayerInTunnel();
			}
		}
	}
}

/*********************** DRAW TUNNEL ************************/

void DrawTunnel(void)
{
int			i,t;

	OGL_PushState();


			/**************************/
			/* DRAW GEOMETRY SECTIONS */
			/**************************/

	if (gLevelNum == LEVEL_NUM_GUTTER)											// use double-sided shading	if gutter
	{
		glDisable(GL_CULL_FACE);
#if ALLOW_GL_LIGHT_MODEL_TWO_SIDE
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
#endif
	}

	for (i = 0; i < gNumTunnelSections; i++)
	{
		if (OGL_IsBBoxVisible(&gTunnelSectionMeshes[i]->objectData.bBox, nil))
		{
			MO_DrawObject(gTunnelSectionMeshes[i]);
		}
	}
#if ALLOW_GL_LIGHT_MODEL_TWO_SIDE
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
#endif



			/***********************/
			/* DRAW WATER SECTIONS */
			/***********************/
			//
			// We draw the water last since it is semi-transparent
			//

	gTunnelWaterTextureScroll -= gFramesPerSecondFrac * 1.1f;
	glMatrixMode(GL_TEXTURE);											// set texture matrix
	glTranslatef(0, gTunnelWaterTextureScroll, 0);
	glMatrixMode(GL_MODELVIEW);

	OGL_DisableLighting();		//---------
	gGlobalColorFilter.r =
	gGlobalColorFilter.g =
	gGlobalColorFilter.b = .7f;	//-------

			/* INC WATER TEXTURE INDEX */


	if (gLevelNum == LEVEL_NUM_PLUMBING)
	{
		gTunnelWaterAnimIndex += gFramesPerSecondFrac * 10.0f;
		t = gTunnelWaterAnimIndex;
		if (t > 5)
		{
			gTunnelWaterAnimIndex = 0;
			t = 0;
		}
				/* SUBMIT TEXTURE */

		MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][PLUMBING_SObjType_Water0 + t].materialObject);
	}
	else
	{
		MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][GUTTER_SObjType_Water0].materialObject);

	}

			/* DRAW SECTIONS */

	for (i = gNumTunnelSections-1; i >= 0 ; i--)						// draw these in reverse order to help overlapping transparencies be in the correct order
	{
		if (OGL_IsBBoxVisible(&gTunnelSectionWaterObjects[i]->objectData.bBox, nil))
		{
			MO_DrawObject(gTunnelSectionWaterObjects[i]);
		}
	}

				/* CLEANUP */

	glMatrixMode(GL_TEXTURE);											// reset texture matrix
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	OGL_PopState();
	gGlobalColorFilter.r =
	gGlobalColorFilter.g =
	gGlobalColorFilter.b = 1.0f;


			/*************************************************/
			/* DRAW THE REGULAR CACHE OF OBJECTS AND EFFECTS */
			/*************************************************/

	DrawObjects();
}

#pragma mark -


/*************************** CREATE PLAYER MODEL: TUNNEL ****************************/

void CreatePlayerModel_Tunnel(int	tunnelIndex)
{
ObjNode	*newObj;

	gPlayerTunnelIndex = tunnelIndex;


		/*************************/
		/* MAKE  SKELETON OBJECT */
		/*************************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_SKIP_TUNNEL;
	gNewObjectDefinition.animNum	= PLAYER_TUNNEL_ANIM_SURF;
	gNewObjectDefinition.coord 		= gTunnelSplinePoints[tunnelIndex].point;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_DONTCULL|STATUS_BIT_ROTZXY;
	gNewObjectDefinition.slot 		= PLAYER_SLOT;
	gNewObjectDefinition.moveCall	= MovePlayer_Tunnel;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= PLAYER_TUNNEL_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	gPlayerInfo.objNode 	= newObj;


				/* SET COLLISION INFO */

	newObj->CType = CTYPE_PLAYER;
	newObj->CBits = CBITS_ALLSOLID;

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);


	gPreviousSprayCoord = newObj->Coord;							// set spray to start coord

}


/******************* RESET PLAYER IN TUNNEL ***********************/

static void ResetPlayerInTunnel(void)
{
ObjNode	*player = gPlayerInfo.objNode;

			/***************************************************/
			/* FIRST TAKE AWAY A LIFE AND SEE IF IT'S ALL OVER */
			/***************************************************/

	gPlayerInfo.lives--;				// dec # lives
	if (gPlayerInfo.lives <= 0)
	{
		gGameOver = true;
		return;
	}


	player->MoveCall = MovePlayer_Tunnel;
	player->Rot.z = 0;
	player->Rot.x = 0;

	gPlayerInfo.health = player->Health = 1.0;

	gPlayerIsDead		= false;

	SetSkeletonAnim(player->Skeleton, PLAYER_TUNNEL_ANIM_SURF);

	gPlayerTunnelIndex = 200;
	gPlayerInfo.tunnelSpeed = 0;
	gPlayerInfo.tunnelAngle = 0;
	gPlayerInfo.tunnelDeltaRot = 0;

	MakeFadeEvent(true, 1);

}



/********************* MOVE PLAYER: TUNNEL **************************/

static void MovePlayer_Tunnel(ObjNode *player)
{
float	oldSpeed,oldIndex;
float	dy;
OGLPoint3D	p1,p2;
float	fps = gFramesPerSecondFrac;
int		anim;

	GetObjectInfo(player);

			/****************************/
			/* SET CORRECT BANKING ANIM */
			/****************************/

	anim = player->Skeleton->AnimNum;									// get anim #
	if (anim != PLAYER_TUNNEL_ANIM_GOTHIT)								// if got hit then leave it there
	{
				/* SEE IF BANK LEFT */

		if (gPlayerInfo.analogControlX < 0.0f)
		{
			if (anim != PLAYER_TUNNEL_ANIM_BANKLEFT)
				MorphToSkeletonAnim(player->Skeleton, PLAYER_TUNNEL_ANIM_BANKLEFT, 6);
		}
		else
				/* SEE IF BANK RIGHT */

		if (gPlayerInfo.analogControlX > 0.0f)
		{
			if (anim != PLAYER_TUNNEL_ANIM_BANKRIGHT)
				MorphToSkeletonAnim(player->Skeleton, PLAYER_TUNNEL_ANIM_BANKRIGHT, 6);
		}

				/* NO BANKING */

		else
		{
			if (anim != PLAYER_TUNNEL_ANIM_SURF)
				MorphToSkeletonAnim(player->Skeleton, PLAYER_TUNNEL_ANIM_SURF, 6);
		}
	}

				/*****************************/
				/* MOVE FORWARD ALONG TUNNEL */
				/*****************************/

				/* ACCEL BASED ON SLOPE */

	CalcTunnelCoordFromIndex(gPlayerTunnelIndex, &p1);
	CalcTunnelCoordFromIndex(gPlayerTunnelIndex+5, &p2);
	dy = p1.y - p2.y;
	dy *= 8.0f;

	oldSpeed = gPlayerInfo.tunnelSpeed;									// keep old speed
	if (oldSpeed < MIN_TUNNEL_SPEED)									// if going really slow, then give a boost
		dy = MIN_TUNNEL_SPEED;

	gPlayerInfo.tunnelSpeed +=  dy * fps;								// inc speed

	if (gPlayerInfo.tunnelSpeed <= 0.0f)								// keep from going backwards
		gPlayerInfo.tunnelSpeed = MIN_TUNNEL_SPEED;
	else
	if (gPlayerInfo.tunnelSpeed < MIN_TUNNEL_SPEED)
	{
		if (oldSpeed >= MIN_TUNNEL_SPEED)								// see if used to be faster than min, if so, keep at min
			gPlayerInfo.tunnelSpeed = MIN_TUNNEL_SPEED;
	}
	else
	if (gPlayerInfo.tunnelSpeed > MAX_TUNNEL_SPEED)
		gPlayerInfo.tunnelSpeed = MAX_TUNNEL_SPEED;


					/* MOVE IT */

	oldIndex = gPlayerTunnelIndex;
	gPlayerTunnelIndex += gPlayerInfo.tunnelSpeed * fps;

	if ((gPlayerTunnelIndex > (gNumTunnelSplinePoints * .95f)) &&		// see if crossed ending point
		(oldIndex <=  (gNumTunnelSplinePoints * .95f)))
	{
		StartLevelCompletion(0);
	}

	if (gPlayerTunnelIndex >= gNumTunnelSplinePoints)			// make sure we don't go over
		gPlayerTunnelIndex = gNumTunnelSplinePoints-1;

				/***************/
				/* DO SPINNING */
				/***************/

	DoTunnelSpin(player);


				/***********************/
				/* DO OBJECT COLLISION */
				/***********************/

	if (gLevelNum == LEVEL_NUM_PLUMBING)
		DoTunnelObjectCollision_Sewer(player);
	else
		DoTunnelObjectCollision_Gutter(player);


				/* UPDATE */

	UpdatePlayer_Tunnel(player);
}


/*********************** DO TUNNEL SPIN **************************/
//
// Handles calculations for making the player spin around the circumference of the tunnel tube.
//

static void DoTunnelSpin(ObjNode *player)
{
float	tunnelAngle, deltaRot;
float	g,friction;
float	fps = gFramesPerSecondFrac;
static const OGLPoint3D tunnelBottomPt = {0,-SPAR_DIAMATER + 7.0f,0};
int		i;
OGLPoint3D	splinePt;
OGLMatrix4x4	m;
OGLPoint3D	wallOff;

	tunnelAngle = gPlayerInfo.tunnelAngle;							// get current angle
	deltaRot	= gPlayerInfo.tunnelDeltaRot;						// get spin delta


				/*************************/
				/* ACCEL FROM USER INPUT */
				/*************************/

	if (player->Skeleton->AnimNum != PLAYER_TUNNEL_ANIM_GOTHIT)					// if got hit then no control
		deltaRot += gPlayerInfo.analogControlX * fps * -TUNNEL_ROT_SENSITIVITY;	// accelerate the spin


				/***********************/
				/* ENVIRONMENTAL ACCEL */
				/***********************/

			/* APPLY GRAVITY TO THE SPIN */

	g = fabs(tunnelAngle) / (PI/2);
	if (g > 1.0f)
		g = 1.0f;
	g = sin(g) * fps * TUNNEL_ROT_GRAVITY;
	if (tunnelAngle < 0.0f)
		deltaRot += g;
	else
		deltaRot -= g;


			/* APPLY FRICTION TO THE SPIN */

	if (fabs(tunnelAngle) < (PI/6))				// heavy friction if on water
		friction = TUNNEL_ROT_FRICTION * 1.5f;
	else
		friction = TUNNEL_ROT_FRICTION;

	if (deltaRot > 0.0f)
	{
		deltaRot -= fps * friction;
		if (deltaRot < 0.0f)
			deltaRot = 0.0f;
	}
	else
	if (deltaRot < 0.0f)
	{
		deltaRot += fps * friction;
		if (deltaRot > 0.0f)
			deltaRot = 0.0f;
	}


			/* CHECK MAX SPIN SPEED */

	if (deltaRot > PI)
		deltaRot = PI;
	else
	if (deltaRot < -PI)
		deltaRot = -PI;


			/* DO SPIN & UPDATE */

	tunnelAngle += deltaRot * fps;									// spin it
	if (tunnelAngle > PI)											// wrap angle around
		tunnelAngle = tunnelAngle - PI2;
	else
	if (tunnelAngle < -PI)
		tunnelAngle = tunnelAngle + PI2;


			/***************************/
			/* CALC TUNNEL AIM & COORD */
			/***************************/

	gPlayerInfo.tunnelAngle = tunnelAngle;
	gPlayerInfo.tunnelDeltaRot = deltaRot;

	i = gPlayerTunnelIndex;
	gPlayerInfo.tunnelAim.x = gTunnelSplinePoints[i+1].point.x - gTunnelSplinePoints[i].point.x;
	gPlayerInfo.tunnelAim.y = gTunnelSplinePoints[i+1].point.y - gTunnelSplinePoints[i].point.y;
	gPlayerInfo.tunnelAim.z = gTunnelSplinePoints[i+1].point.z - gTunnelSplinePoints[i].point.z;
	OGLVector3D_Normalize(&gPlayerInfo.tunnelAim, &gPlayerInfo.tunnelAim);


			/* CALC COORD */

	CalcTunnelCoordFromIndex(gPlayerTunnelIndex, &splinePt);						// get spline's coord

	OGLMatrix4x4_SetRotateAboutAxis(&m, &gPlayerInfo.tunnelAim, tunnelAngle + gPlayerInfo.tunnelBanking);		// calc offset to tunnel wall
	OGLPoint3D_Transform(&tunnelBottomPt, &m, &wallOff);
	gCoord.x = splinePt.x + wallOff.x;												// calc final coord
	gCoord.y = splinePt.y + wallOff.y;
	gCoord.z = splinePt.z + wallOff.z;



		/*********************************/
		/* SEE IF FLY OFF EDGE OF GUTTER */
		/*********************************/

	if (gLevelNum == LEVEL_NUM_GUTTER)
	{
		if (tunnelAngle > 1.65f)			// see if off left edge
		{
			PlayerFallsOffEdgeOfTunnel(player, true);
		}
		else
		if (tunnelAngle < -1.65f)		// see if off right edge
		{
			PlayerFallsOffEdgeOfTunnel(player, false);
		}
	}

}


/*************************** UPDATE PLAYER:  TUNNEL *********************************/

static void UpdatePlayer_Tunnel(ObjNode *player)
{
float	fps = gFramesPerSecondFrac;
float	targetZrot,realAngle;
OGLVector2D	tvec;
static const OGLVector2D negz = {0,1};
Boolean		onFlat;

			/* SEE IF FALLEN OFF */

	if (player->Skeleton->AnimNum == PLAYER_TUNNEL_ANIM_FALLOFF)
	{
		StopAChannel(&player->EffectChannel);		// make sure not doing metal scrape or anything
		UpdateObject(player);
		return;
	}

			/******************/
			/* CALC X & Y ROT */
			/******************/
			//
			// Keep Skip aimed down the tunnel
			//

	FastNormalizeVector2D(gPlayerInfo.tunnelAim.x, gPlayerInfo.tunnelAim.z, &tvec, true);
	if (OGLVector2D_Cross(&tvec, &negz) < 0.0f)
		player->Rot.y = PI - acos(OGLVector2D_Dot(&tvec, &negz));
	else
		player->Rot.y = PI + acos(OGLVector2D_Dot(&tvec, &negz));

	player->Rot.x = -acos(gPlayerInfo.tunnelAim.y) + PI/2;						// assume the tunnelAim.y is a usable dot product angle


			/**************/
			/* CALC Z ROT */
			/**************/
			//
			// We hack this a bit since we want Skip to appear to remain
			// flat on the water area.  He'll tilt when he hits the pipe wall
			//

	realAngle = gPlayerInfo.tunnelBanking + gPlayerInfo.tunnelAngle;
	if (fabs(realAngle) > (PI/6))					// keep flat when on water area
	{
		targetZrot = -realAngle;
		onFlat = false;
	}
	else
	{
		targetZrot = 0;
		onFlat = true;
	}

	if (fabs(targetZrot) > (PI/3))								// if at steep angle above sides, then just assign rot
		player->Rot.z = targetZrot;
	else														// otherwise, accel into the rot to avoid popping when leave water zone
	{
		if (player->Rot.z < targetZrot)
		{
			player->Rot.z += fps * PI2;
			if (player->Rot.z > targetZrot)
				player->Rot.z = targetZrot;
		}
		else
		if (player->Rot.z > targetZrot)
		{
			player->Rot.z -= fps * PI2;
			if (player->Rot.z < targetZrot)
				player->Rot.z = targetZrot;
		}
	}
			/* UPDATE OBJECT */

	gDelta.x = gCoord.x - player->OldCoord.x;
	gDelta.y = gCoord.y - player->OldCoord.y;
	gDelta.z = gCoord.z - player->OldCoord.z;

			/* UPDATE COLLISION BOX */

	CreateCollisionBoxFromBoundingBox(player, 1,1);


				/* UPDATE GOT-HIT */

	if (!gPlayerIsDead)
	{
		gPlayerInfo.invincibilityTimer -= fps;
		if (player->Skeleton->AnimNum == PLAYER_TUNNEL_ANIM_GOTHIT)					// see if is in get-hit anim
		{
			if (player->Skeleton->AnimHasStopped)									// see if anim is done
			{
				MorphToSkeletonAnim(player->Skeleton, PLAYER_TUNNEL_ANIM_SURF, 10);
			}
		}
	}

	UpdateObject(player);


		/********************/
		/* MAKE WATER SPRAY */
		/********************/

	if (onFlat)
	{
		MakeTunnelSpray(player);

		StopAChannelIfEffectNum(&player->EffectChannel, EFFECT_METALSCRAPE);		// make sure not doing metal scrape

		if (player->EffectChannel == -1)											// see if need to start water sound
			player->EffectChannel = PlayEffect_Parms3D(EFFECT_TUNNELWATER, &gCoord, NORMAL_CHANNEL_RATE, .1);
		else																		// update sound pitch
		{
			float	v = FULL_CHANNEL_VOLUME * (gPlayerInfo.tunnelSpeed * .004f);

			ChangeChannelRate(player->EffectChannel, (NORMAL_CHANNEL_RATE-0x0000) + (gPlayerInfo.tunnelSpeed * 900.0f));
			ChangeChannelVolume(player->EffectChannel, v, v);
		}
	}

				/* ON METAL, NOT WATER */

	else
	{
		StopAChannelIfEffectNum(&player->EffectChannel, EFFECT_TUNNELWATER);		// make sure not doing water

		if (player->EffectChannel == -1)											// see if need to start metal sound
			player->EffectChannel = PlayEffect_Parms3D(EFFECT_METALSCRAPE, &gCoord, NORMAL_CHANNEL_RATE, .1);
		else																		// update sound pitch
		{
			float	v = FULL_CHANNEL_VOLUME * (gPlayerInfo.tunnelSpeed * .005f);

			ChangeChannelRate(player->EffectChannel, (NORMAL_CHANNEL_RATE-0x3000) + (gPlayerInfo.tunnelSpeed * 100.0f));
			ChangeChannelVolume(player->EffectChannel, v, v);
		}
	}

}

#define	DIST_BETWEEN_SPRAY	2.2f

/*********************** MAKE TUNNEL SPRAY ***************************/

static void MakeTunnelSpray(ObjNode *player)
{
float	dist = OGLPoint3D_Distance(&player->Coord, &gPreviousSprayCoord);					// see if gone far enough for next spray segment
ObjNode	*spray;
int		numSegments;
int		i;
float	dx,dy,dz;
OGLPoint3D	sprayCoord;
OGLVector3D	v;
static const OGLPoint3D footOff = {0,0,0};

	numSegments = dist * (1.0f/DIST_BETWEEN_SPRAY);					// determine how many spray segments to make;
	if (numSegments < 1)
		return;

			/* CALC SPRAY POINT UNDER PLAYER'S FEET */

	FindCoordOnJoint(player, TUNNEL_PLAYER_JOINT_LEFTFOOT, &footOff, &sprayCoord);

	if (numSegments > 10)											// if huge then its been a while since we sprayed so we should just reset it
	{
		gPreviousSprayCoord = sprayCoord;
		return;
	}



			/* CALC INTERPOLATION DELTAS */

	v.x = sprayCoord.x - gPreviousSprayCoord.x;						// vector from previous to player
	v.y = sprayCoord.y - gPreviousSprayCoord.y;
	v.z = sprayCoord.z - gPreviousSprayCoord.z;
	OGLVector3D_Normalize(&v, &v);

	dx = v.x * DIST_BETWEEN_SPRAY;
	dy = v.y * DIST_BETWEEN_SPRAY;
	dz = v.z * DIST_BETWEEN_SPRAY;

			/* CALC COORD OF FIRST */

	gNewObjectDefinition.coord.x = gPreviousSprayCoord.x + dx;
	gNewObjectDefinition.coord.y = gPreviousSprayCoord.y + dy;
	gNewObjectDefinition.coord.z = gPreviousSprayCoord.z + dz;


		/* CREATE SPRAY SEGMENTS ALONG THE INTERPOLATED LINE */

	for (i = 0; i < numSegments; i++)
	{
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		if (gLevelNum == LEVEL_NUM_PLUMBING)
			gNewObjectDefinition.type 		= PLUMBING_ObjType_Spray;
		else
			gNewObjectDefinition.type 		= GUTTER_ObjType_Spray;
		gNewObjectDefinition.scale 		= 5.0;
		gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_DOUBLESIDED | STATUS_BIT_DONTCULL |
										 STATUS_BIT_ROTZXY | STATUS_BIT_NOFOG | STATUS_BIT_NOLIGHTING;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB + 10;
		gNewObjectDefinition.moveCall 	= MoveTunnelSpray;
		gNewObjectDefinition.rot 		= player->Rot.y;
		spray = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		spray->Rot.x = player->Rot.x;
		spray->Rot.z = player->Rot.z;

		spray->ColorFilter.a = .9f;

		gPreviousSprayCoord = gNewObjectDefinition.coord;			// remember where it was

		gNewObjectDefinition.coord.x += dx;
		gNewObjectDefinition.coord.y += dy;
		gNewObjectDefinition.coord.z += dz;
	}
}


/****************** MOVE TUNNEL SPRAY *************************/

static void MoveTunnelSpray(ObjNode *spray)
{
float	fps = gFramesPerSecondFrac;

			/* SCALE UP */

	spray->Scale.x += fps * 160.0f;
	spray->Scale.y += fps * 90.0f;


			/* FADE OUT */

	spray->ColorFilter.a -= fps * 5.2f;
	if (spray->ColorFilter.a <= 0.0f)
	{
		DeleteObject(spray);
		return;
	}

	UpdateObjectTransforms(spray);
}


#pragma mark -

/********************* CALC TUNNEL COORD FROM INDEX ***************************/

void CalcTunnelCoordFromIndex(float index, OGLPoint3D *coord)
{
int		i,i2;
OGLPoint3D	*p1,*p2;
float	ratio,oneMinusRatio;

			/* CALC INDEX OF THIS PT AND NEXT */

	i = index;
	i2 = i+1;
	if (i2 >= gNumTunnelSplinePoints)									// make sure not go too far
		i2 = gNumTunnelSplinePoints-1;


			/* GET THIS POINT AND NEXT */

	p1 = &gTunnelSplinePoints[i].point;
	p2 = &gTunnelSplinePoints[i2].point;



			/* INTERPOLATE */

	ratio = index - (float)i;											// calc 0.0 - 1.0 ratio for weighing the points
	oneMinusRatio = 1.0f - ratio;

	coord->x = p1->x * oneMinusRatio + p2->x * ratio;
	coord->y = p1->y * oneMinusRatio + p2->y * ratio;
	coord->z = p1->z * oneMinusRatio + p2->z * ratio;

}


#pragma mark -


/************************ DO TUNNEL OBJECT COLLISION: SEWER ***************************/

static void DoTunnelObjectCollision_Sewer(ObjNode *player)
{
ObjNode			*thisNode;
OGLMatrix4x4	*m;
int				i,j;
static const OGLPoint3D	nailLines[4][2] =						// line segs for nail collision
{
	0, -19, 0,		-3.4, 20.2, 0,
	0, -19, 0,		3.4, 20.2, 0,
	0, -19, 0,		0, 20.2, -3.4,
	0, -19, 0,		0, 20.2, 3.4
};

static const OGLPoint3D	blobLines[7][2] =						// line segs for blob collision
{
	-8.2, 0, 0,			8.4, 0, 0,			// x
	0, 0, -8.6,			0, 0, 8.4,			// z
	0, 7, 0,			0, -6, 0,			// y

	-6.0, 5.2, -6.6,	6.4, -5.2, 6.4,
	-6.0, -5.2, -6.6,	6.4, 5.2, 6.4,

	-6.0, -5.0, 6.4,	6.6, -5, -5,
	-6.0, 5.0, 6.4,		6.6, 5, -5
};

static const OGLPoint3D	healthLines[2][2] =						// line segs for blob collision
{
	-18, 17.5, -19,		10, -14, 19,
	-18, 17.5, 19,		10, -14, -19,

};

static const OGLPoint3D	ringLines[16][2] =						// line segs for blob collision
{
	-20, -40, 24,		20, -20, 35,
	-20, -20, 35,		20, 0, 45,

	-20, 0, 45,			20, 25, 30,
	-20, 25, 30,		20, 40, 0,

	-20, 40, 0,			20, 25, -30,
	-20, 25, -30,		20, 0, -40,

	-20, 0, -40, 		20, -20, -35,
	-20, -20, -35,		20, -40, -24,

	20, -40, 24,		-20, -20, 35,
	20, -20, 35,		-20, 0, 45,

	20, 0, 45,			-20, 25, 30,
	20, 25, 30,			-20, 40, 0,

	20, 40, 0,			-20, 25, -30,
	20, 25, -30,		-20, 0, -40,

	20, 0, -40, 		-20, -20, -35,
	20, -20, -35,		-20, -40, -24,


};


OGLPoint3D	p[32];

	if (gPlayerInfo.invincibilityTimer > 0.0f)					// no collision if invincible
		return;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode == player)									// don't collide against self
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_ISCULLED)			// only collide with things we can see
			goto next;

		if (thisNode->Group != MODEL_GROUP_LEVELSPECIFIC)		// only collide with tunnel object models
			goto next;

				/************************************************/
				/* HANDLE CUSTOM COLLISIONS BASED ON MODEL TYPE */
				/************************************************/


		m = &thisNode->BaseTransformMatrix;						// get ptr to the object's matrix


		switch(thisNode->Type)
		{
					/********/
					/* NAIL */
					/********/

			case	PLUMBING_ObjType_Nail:
					OGLPoint3D_TransformArray(&nailLines[0][0], m, p, 8);			// transform to world coords
					for (j = i = 0; i < 4; i++, j+=2)								// check 4 line segments for collision
					{
						if (SeeIfLineSegmentHitsObject(&p[j], &p[j+1], player))
						{
							PlayEffect3D(EFFECT_NAILHIT, &gCoord);
							PlayerGotHitInTunnel(player);
							return;
						}
					}
					break;


					/********/
					/* BLOB */
					/********/

			case	PLUMBING_ObjType_Blob:
					OGLPoint3D_TransformArray(&blobLines[0][0], m, p, 14);			// transform to world coords
					for (j = i = 0; i < 7; i++, j+=2)								// check 4 line segments for collision
					{
						if (SeeIfLineSegmentHitsObject(&p[j], &p[j+1], player))
						{
							PlayEffect3D(EFFECT_SLUDGEHIT, &gCoord);
							PlayerGotHitInTunnel(player);
							ExplodeGeometry(thisNode, 40, SHARD_MODE_FROMORIGIN, 1, 2.0);
							DeleteObject(thisNode);
							return;
						}
					}
					break;


					/**************/
					/* HEALTH POW */
					/**************/

			case	PLUMBING_ObjType_HealthPOW:
					if (thisNode->MoveCall == nil)										// make sure not already tagged
					{
						OGLPoint3D_TransformArray(&healthLines[0][0], m, p, 4);			// transform to world coords
						for (j = i = 0; i < 2; i++, j+=2)								// check 4 line segments for collision
						{
							if (SeeIfLineSegmentHitsObject(&p[j], &p[j+1], player))
							{
								gPlayerInfo.health += .2f;
								if (gPlayerInfo.health > 1.0f)
									gPlayerInfo.health = 1.0f;

								StartPowerupVanish(thisNode);
							}
						}
					}
					break;


					/********/
					/* RING */
					/********/

			case	PLUMBING_ObjType_Ring:
					OGLPoint3D_TransformArray(&ringLines[0][0], m, p, 32);			// transform to world coords
					for (j = i = 0; i < 16; i++, j+=2)								// check 4 line segments for collision
					{
						if (SeeIfLineSegmentHitsObject(&p[j], &p[j+1], player))
						{
							PlayEffect3D(EFFECT_NAILHIT, &gCoord);
							PlayerGotHitInTunnel(player);
							return;
						}
					}
					break;


		}

next:
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);
}





/************************ DO TUNNEL OBJECT COLLISION: GUTTER ***************************/

static void DoTunnelObjectCollision_Gutter(ObjNode *player)
{
ObjNode			*thisNode;
OGLMatrix4x4	*m;
int				i,j;

static const OGLPoint3D	pineconeLines[5][2] =						// line segs for pinecone collision
{
	-20, 0, 0,			20, 0, 0,
	0, 0, -20,			0, 0, 20,
	0, 23, 0,			0, -23, 0,
	-13, 13, -13,		13, -13, 13,
	-13, 13, 13,		13, 13, -13
};

static const OGLPoint3D	leafLines[5][2] =							// line segs for leaf collision
{
	-30, 5, 0,			30, 5, 0,
	-30, -5, 0,			30, -5, 0,
	0, 0, 23,			0, 0, -23,
	-20, 0, 20,			20, 0, -20,
	-20, 0, -20,		20, 0, 20
};


OGLPoint3D	p[24];

	if (gPlayerInfo.invincibilityTimer > 0.0f)					// no collision if invincible
		return;

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode == player)									// don't collide against self
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_ISCULLED)			// only collide with things we can see
			goto next;

		if (thisNode->Group != MODEL_GROUP_LEVELSPECIFIC)		// only collide with tunnel object models
			goto next;

				/************************************************/
				/* HANDLE CUSTOM COLLISIONS BASED ON MODEL TYPE */
				/************************************************/


		m = &thisNode->BaseTransformMatrix;						// get ptr to the object's matrix


		switch(thisNode->Type)
		{
					/*************/
					/* PINE CONE */
					/*************/

			case	GUTTER_ObjType_PineCone:
					OGLPoint3D_TransformArray(&pineconeLines[0][0], m, p, 10);			// transform to world coords
					for (j = i = 0; i < 5; i++, j+=2)									// check n line segments for collision
					{
						if (SeeIfLineSegmentHitsObject(&p[j], &p[j+1], player))
						{
							PlayEffect3D(EFFECT_HITPINECONE, &gCoord);
							PlayerGotHitInTunnel(player);
							ExplodeGeometry(thisNode, 40, SHARD_MODE_FROMORIGIN, 1, 2.0);
							DeleteObject(thisNode);
							return;
						}
					}
					break;

					/********/
					/* LEAF */
					/********/

			case	GUTTER_ObjType_Leaf:
					OGLPoint3D_TransformArray(&leafLines[0][0], m, p, 10);			// transform to world coords
					for (j = i = 0; i < 5; i++, j+=2)									// check n line segments for collision
					{
						if (SeeIfLineSegmentHitsObject(&p[j], &p[j+1], player))
						{
							PlayEffect3D(EFFECT_HITLEAF, &gCoord);
							PlayerGotHitInTunnel(player);
							return;
						}
					}
					break;

		}

next:
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);
}



/********************** PLAYER GOT HIT IN TUNNEL ****************************/

static void PlayerGotHitInTunnel(ObjNode *player)
{
	gPlayerInfo.tunnelSpeed *= .4f;

	gPlayerInfo.invincibilityTimer = 2.5;

	MorphToSkeletonAnim(player->Skeleton, PLAYER_TUNNEL_ANIM_GOTHIT, 10);

	PlayerLoseHealth(.2, PLAYER_DEATH_TYPE_TUNNEL);
}


/****************** KILL PLAYER IN TUNNEL ************************/

void KillPlayerInTunnel(ObjNode *skip)
{
	(void) skip;
}


#pragma mark -

/******************** PLAYER FALLS OFF EDGE OF TUNNEL *********************/

static void PlayerFallsOffEdgeOfTunnel(ObjNode *player, Boolean isLeft)
{
OGLVector3D	v;
OGLPoint3D	splinePt;

	MorphToSkeletonAnim(player->Skeleton, PLAYER_TUNNEL_ANIM_FALLOFF, 5);
	player->MoveCall = MovePlayer_TunnelFallOff;


			/* CALC VECTOR OF CENTRIFICAL FORCE */

	CalcTunnelCoordFromIndex(gPlayerTunnelIndex, &splinePt);
	v.x = gCoord.x - splinePt.x;
	v.y = gCoord.y - splinePt.y;
	v.z = gCoord.z - splinePt.z;
	FastNormalizeVector(v.x, v.y, v.z, &v);

	v.x *= 70.0f;
	v.y *= 70.0f;
	v.z *= 70.0f;


				/* CALC DELTAS */

	gDelta.x = v.x + (gCoord.x - player->OldCoord.x) * gFramesPerSecond;
	gDelta.y = v.y + (gCoord.y - player->OldCoord.y) * gFramesPerSecond;
	gDelta.z = v.z + (gCoord.z - player->OldCoord.z) * gFramesPerSecond;


	gPlayerIsDead = true;
	gDeathTimer = 2.0f;


	if (isLeft)
		player->DeltaRot.z = 12;
	else
		player->DeltaRot.z = -12;
}


/************************ MOVE PLAYER: TUNNEL FALL OFF ************************/

static void MovePlayer_TunnelFallOff(ObjNode *player)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(player);


			/* MOVE PLAYER */

	gDelta.y -= 500.0f * fps;						// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	player->Rot.z += player->DeltaRot.z * fps;

	UpdateObject(player);


			/* MOVE CAMERA ALONG SPLINE */

	gPlayerTunnelIndex += gPlayerInfo.tunnelSpeed * fps;

}





