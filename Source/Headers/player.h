//
// player.h
//

#pragma once

#define	MAX_BUDDY_BUGS			10

#define	PLAYER_DEFAULT_SCALE	1.2f
#define	DEFAULT_PLAYER_SHADOW_SCALE	2.0f


#define	PLAYER_COLLISION_CTYPE	(CTYPE_MISC|CTYPE_TRIGGER|CTYPE_FENCE|CTYPE_ENEMY|CTYPE_HURTME|CTYPE_PLAYERONLY)

#define	PLAYER_NORMAL_MAX_SPEED	700.0f
#define	MAX_RAMMING_SPEED		(PLAYER_NORMAL_MAX_SPEED * 2.0f)
#define PLAYER_PUSH_MAX_SPEED	200.0f


enum
{
	PLAYER_DEATH_TYPE_EATENBYSNAKE,
	PLAYER_DEATH_TYPE_EXPLODE,
	PLAYER_DEATH_TYPE_FALLOVER,
	PLAYER_DEATH_TYPE_DROWN,
	PLAYER_DEATH_TYPE_BALSA,
	PLAYER_DEATH_TYPE_TUNNEL,
	PLAYER_DEATH_TYPE_KILLERDRAGONFLY
};


		/* EXPLORE ANIMS */

enum
{
	PLAYER_ANIM_PERSONALITY2,
	PLAYER_ANIM_WALK,
	PLAYER_ANIM_JUMP,
	PLAYER_ANIM_LANDING,
	PLAYER_ANIM_GLIDING,
	PLAYER_ANIM_PICKUP,
	PLAYER_ANIM_WALKCARRY,
	PLAYER_ANIM_STANDCARRY,
	PLAYER_ANIM_JUMPCARRY,
	PLAYER_ANIM_FALLCARRY,
	PLAYER_ANIM_FALL,
	PLAYER_ANIM_DROPOBJECT,
	PLAYER_ANIM_KICK,
	PLAYER_ANIM_LANDCARRY,
	PLAYER_ANIM_SWIM,
	PLAYER_ANIM_WALKONBALL,
	PLAYER_ANIM_GOTHIT_BACKFLIP,
	PLAYER_ANIM_GLIDECARRY,
	PLAYER_ANIM_GETUPFROMHIT_BACKFLIP,
	PLAYER_ANIM_GOTHIT_GENERIC,
	PLAYER_ANIM_EATENBYSNAKE,
	PLAYER_ANIM_RAMMING,
	PLAYER_ANIM_DRIVESLOTCAR,
	PLAYER_ANIM_PUSH,
	PLAYER_ANIM_PERSONALITY1,
	PLAYER_ANIM_DEATH,
	PLAYER_ANIM_DROWN,
	PLAYER_ANIM_STAND,
	PLAYER_ANIM_PERSONALITY2_CARRY,
	PLAYER_ANIM_PERSONALITY1_CARRY,
	PLAYER_ANIM_PERSONALITY3_DANCE,
	PLAYER_ANIM_CLOVERTOSS,
	PLAYER_ANIM_FLYBALSAPLANE,
	PLAYER_ANIM_FALLFROMPLANE,
	PLAYER_ANIM_CARRIED,
	PLAYER_ANIM_VACUUMESUCK
};


		/* TUNNEL ANIMS */

enum
{
	PLAYER_TUNNEL_ANIM_SURF,
	PLAYER_TUNNEL_ANIM_BANKLEFT,
	PLAYER_TUNNEL_ANIM_BANKRIGHT,
	PLAYER_TUNNEL_ANIM_GOTHIT,
	PLAYER_TUNNEL_ANIM_FALLOFF
};


		/* TITLE ANIMS */

enum
{
	PLAYER_TITLE_ANIM_WALK,
	PLAYER_TITLE_ANIM_ROBBED,
	PLAYER_TITLE_ANIM_GETMAD,
	PLAYER_TITLE_ANIM_THINK,
	PLAYER_TITLE_ANIM_GETUP,
	PLAYER_TITLE_ANIM_JUMP,
	PLAYER_TITLE_ANIM_LOOK,
	PLAYER_TITLE_ANIM_PICKUP,
	PLAYER_TITLE_ANIM_WINWALK
};



enum
{
	PLAYER_JOINT_BASE = 0,
	PLAYER_JOINT_LEFT_FOOT = 11,
	PLAYER_JOINT_RIGHT_FOOT = 12,
	PLAYER_JOINT_UPPER_RIGHT_ELBOW = 14
};


#define	NUM_WING_BLUR_LAYERS	8


enum
{
	KEY_TYPE_RED	= 0,
	KEY_TYPE_GREEN,
	KEY_TYPE_BLUE,

	NUM_KEY_TYPES
};


		/***************/
		/* PLAYER INFO */
		/***************/

typedef struct
{
	int					startX,startZ;
	float				startRotY;

	OGLPoint3D			coord;
	ObjNode				*objNode;

	float				distToFloor;
	float				mostRecentFloorY;

	float				knockDownTimer;
	float				invincibilityTimer;

	float				shieldTimer;
	ObjNode				*shieldObj[2];

	short				shieldChannel;


	OGLRect				itemDeleteWindow;

	OGLCameraPlacement	camera;

	float				burnTimer;
	float				blinkTimer;


			/* WINGS */

	ObjNode				*blurSprite;
	float				wingBlurFrame;

	ObjNode				*wingLayerDrawObject;						// event object for drawing wing layers
	OGLMatrix4x4		wingLayerMatrix[NUM_WING_BLUR_LAYERS][2];	// matrices which define each of the wing layers on both sides
	float				wingLayerAlpha[NUM_WING_BLUR_LAYERS][2];	// alpha fades


		/* TILE ATTRIBUTE PHYSICS TWEAKS */


	float				groundTraction;
	float				groundFriction;
	float				groundAcceleration;

	int					waterPatch;
	ObjNode				*ridingBall;
	int					snake;								// index into snake list
	ObjNode				*slotCar;
	ObjNode				*pushObj;
	float				pushTimer;
	float				suckSpeed;							// for vacuume cleaner

			/* CONTROL INFO */

	float				analogControlX;
	float				analogControlZ;
	Boolean				analogIsMouse;

	float				tunnelAngle;
	float				tunnelDeltaRot;
	float				tunnelSpeed;
	float				tunnelBanking;
	OGLVector3D			tunnelAim;


			/* INVENTORY INFO */

	Byte				lives;
	float				health;

	float				glidePower;
	float				rammingTimer;

	Boolean				hasKey[NUM_KEY_TYPES];
	Boolean				hasMap;

	ObjNode				*heldObject;					// objnode of object being held

	short				numBuddyBugs;
	ObjNode				*buddyBugs[MAX_BUDDY_BUGS];

	short				numGreenClovers;
	short				numBlueClovers;
	short				numGoldClovers;

	short				numMiceRescued;

}PlayerInfoType;


//=======================================================

void InitPlayerInfo_Game(void);
void InitPlayerAtStartOfLevel_Terrain(void);
void InitPlayerAtStartOfLevel_Tunnel(void);
Boolean PlayerLoseHealth(float damage, Byte deathType);
void PlayerEntersWater(ObjNode *theNode, int patchNum);
void PlayerGotHit(ObjNode *byWhat, float altDamage, int hitAnim);
void KillPlayer(Byte deathType);
void ResetPlayerAtBestCheckpoint(void);
Boolean IsPlayerDoingGlideAnim(ObjNode *theNode);
Boolean IsPlayerDoingWalkAnim(ObjNode *theNode);
Boolean IsPlayerDoingStandAnim(ObjNode *theNode);
Boolean IsPlayerDoingFallAnim(ObjNode *theNode);
Boolean IsPlayerDoingJumpAnim(ObjNode *theNode);
Boolean IsPlayerDoingSwimAnim(ObjNode *theNode);
Boolean IsPlayerDoingRammingAnim(ObjNode *theNode);
Boolean IsPlayerDoingPushAnim(ObjNode *theNode);
void UpdatePersonality(ObjNode *player);
void SetPlayerRammingAnim(ObjNode *theNode);
void SetPlayerJumpAnim(ObjNode *player, Boolean playEffect);
void SetPlayerLandAnim(ObjNode *player);
Boolean IsPlayerDoingLandAnim(ObjNode *theNode);
void PlayerStartPushingObject(ObjNode *player, ObjNode *pushObj);
void PlayerStopPushingObject(ObjNode *player);
void UpdatePlayerShield(void);

void HidePlayer(ObjNode *player);
short PlayerDoKick(ObjNode *player);
void PlayerDropObject(ObjNode *player, ObjNode *heldObj);


void MovePlayer_Terrain(ObjNode *theNode);
void CreatePlayerModel_Terrain(OGLPoint3D *where, float rotY);
void SetPlayerStandAnim(ObjNode *theNode, float speed);
void SetPlayerWalkAnim(ObjNode *theNode);
void SetPlayerFallAnim(ObjNode *player);
void SetPlayerGlideAnim(ObjNode *player);

void StartPlayerGliding(ObjNode *player);
void EndGliding(ObjNode *theNode);
void UpdateGlidingWings(ObjNode *player);

void HandlePlayerLineMarkerCrossing(ObjNode *player);
void ReleaseHeldObject(ObjNode *player, ObjNode *heldObj);

		/* BUDDY BUG */

void CreateMyBuddy(OGLPoint3D *where);
void SeeIfLaunchBuddyBug(void);

		/* BALSA */

#define	BALSA_PLANE_HOVER_HEIGHT	1900.0f

void PutPlayerInBalsaPlane(ObjNode *player);
Boolean AddAntHill(TerrainItemEntryType *itemPtr, float  x, float z);
void BlowUpAntHill(ObjNode *theNode);
Boolean AddCloud(TerrainItemEntryType *itemPtr, float  x, float z);
void HurtPlayerOnBalsaPlane(void);
void KillPlayerOnBalsaPlane(ObjNode *skip);
void ResetPlayerOnBalsaPlane(ObjNode *player);
void BurnPlane(ObjNode *theNode);
void CountAntHills(void);
