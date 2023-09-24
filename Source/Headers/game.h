#pragma once

#include "Pomme.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#include <stdint.h>

#if !defined(__LITTLE_ENDIAN__) && !(__BIG_ENDIAN__)
#define __LITTLE_ENDIAN__ 1
#endif

#define APPSTORE	0

#include "version.h"
#include "globals.h"
#include "structs.h"

#include "metaobjects.h"
#include "ogl_support.h"
#include "main.h"
#include "player.h"
#include "mobjtypes.h"
#include "objects.h"
#include "misc.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "skeletonjoints.h"
#include "sound2.h"
#include "sobjtypes.h"
#include "terrain.h"
#include "sprites.h"
#include "shards.h"
#include "sparkle.h"
#include "bg3d.h"
#include "effects.h"
#include "camera.h"
#include "collision.h"
#include "input.h"
#include "inputsdl.h"
#include "file.h"
#include "fences.h"
#include "splineitems.h"
#include "items.h"
#include "window.h"
#include "enemy.h"
#include "water.h"
#include "miscscreens.h"
#include "3dmath.h"
#include "infobar.h"
#include "tunnel.h"
#include "lzss.h"
#include "bones.h"
#include "vaportrails.h"
#include "localization.h"
#include "dialog.h"

extern	BG3DFileContainer		*gBG3DContainerList[];
extern	Boolean					gAllowAudioKeys;
extern	Boolean					gAltivec;
extern	Boolean					gBurnKindling;
extern	Boolean					gDisableAnimSounds;
extern	Boolean					gDisableHiccupTimer;
extern	Boolean					gDoGlidingAtApex;
extern	Boolean					gDrawLensFlare;
extern	Boolean					gEnableSnakes;
extern	Boolean					gFreezeCameraFromXZ;
extern	Boolean					gFreezeCameraFromY;
extern	Boolean					gG4;
extern	Boolean					gGameOver;
extern	Boolean					gHeadOnScarecrow;
extern	Boolean					gIgnoreBottleKeySnail;
extern	Boolean					gLevelCompleted;
extern	Boolean					gMuteMusicFlag;
extern	Boolean					gMyState_Lighting;
extern	Boolean					gNotifyOfSlotCarWin;
extern	Boolean					gPlayerIsDead;
extern	Boolean					gPlayingFromSavedGame;
extern	Boolean					gPoppedSodaCan;
extern	Boolean					gResetGliding;
extern	Boolean					gResetRideBall;
extern	Boolean					gShowFish;
extern	Boolean					gShowFood;
extern	Boolean					gShowRedClovers;
extern	Boolean					gSlowCPU;
extern	Boolean					gSolidTriggerKeepDelta;
extern	Boolean					gSongPlayingFlag;
extern	Boolean					gStartedSiliconDoor;
extern	Byte					**gMapSplitMode;
extern	Byte					gAnaglyphPass;
extern	Byte					gDebugMode;
extern	Byte					gFoodTypes[];
extern	Byte					gSprinklerMode;
extern	ChannelInfoType			gChannelInfo[];
extern	CollisionRec			gCollisionList[];
extern	FSSpec					gDataSpec;
extern	FenceDefType			*gFenceList;
extern	HighScoreType			gHighScores[NUM_SCORES];
extern	LineMarkerDefType		gLineMarkerList[];
extern	MOMaterialObject		*gMostRecentMaterial;
extern	MOMaterialObject		*gSuperTileTextureObjects[MAX_SUPERTILE_TEXTURES];
extern	MOMaterialObject		*gTunnelTextureObj;
extern	MOPictureObject			*gBackgoundPicture;
extern	MOVertexArrayData		**gLocalTriMeshesOfSkelType;
extern	MOVertexArrayObject		*gTunnelSectionObjects[];
extern	MOVertexArrayObject		*gTunnelSectionWaterObjects[];
extern	MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	NewConfettiGroupDefType	gNewConfettiGroupDef;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	OGLBoundingBox			gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	OGLBoundingBox			gWaterBBox[];
extern	OGLColorRGB				gGlobalColorFilter;
extern	OGLMatrix4x4			*gCurrentObjMatrix;
extern	OGLMatrix4x4			gViewToFrustumMatrix;
extern	OGLMatrix4x4			gWorldToFrustumMatrix;
extern	OGLMatrix4x4			gWorldToViewMatrix;
extern	OGLMatrix4x4			gWorldToWindowMatrix;
extern	OGLPoint2D				gBestCheckpointCoord;
extern	OGLPoint3D				gCoord;
extern	OGLSetupOutputType		*gGameView;
extern	OGLVector3D				gDelta;
extern	OGLVector3D				gRecentTerrainNormal;
extern	OGLVector3D				gWorldSunDirection;
extern	ObjNode					*gCurrentCarryingMoth;
extern	ObjNode					*gCurrentNode;
extern	ObjNode					*gFirstNodePtr;
extern	ObjNode					*gKillerDragonFly;
extern	ObjNode					*gSuckingVacuume;
extern	ParticleGroupType		*gParticleGroups[];
extern	PlayerInfoType			gPlayerInfo;
extern	PrefsType				gGamePrefs;
extern	SDL_GLContext			gAGLContext;
extern	SDL_Window				*gSDLWindow;
extern	SparkleType				gSparkles[MAX_SPARKLES];
extern	SplineDefType			**gSplineList;
extern	SpriteType				*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	SuperTileItemIndexType	**gSuperTileItemIndexGrid;
extern	SuperTileMemoryType		gSuperTileMemoryList[];
extern	SuperTileStatus			**gSuperTileStatusGrid;
extern	TerrainItemEntryType	**gMasterItemList;
extern	TunnelItemDefType		*gTunnelItemList;
extern	TunnelSplinePointType	*gTunnelSplinePoints;
extern	WaterDefType			**gWaterListHandle;
extern	WaterDefType			*gWaterList;
extern	char					gTextInput[SDL_TEXTINPUTEVENT_TEXT_SIZE];
extern	const InputBinding		kDefaultInputBindings[NUM_CONTROL_NEEDS];
extern	float					**gMapYCoords;
extern	float					**gMapYCoordsOriginal;
extern	float					**gVertexShading;
extern	float					gAnaglyphEyeSeparation;
extern	float					gAnaglyphFocallength;
extern	float					gAnaglyphScaleFactor;
extern	float					gAutoFadeEndDist;
extern	float					gAutoFadeRange_Frac;
extern	float					gAutoFadeStartDist;
extern	float					gBestCheckpointAim;
extern	float					gCameraDistFromMe;
extern	float					gCameraLookAtYOff;
extern	float					gCurrentAspectRatio;
extern	float					gCurrentMaxSpeed;
extern	float					gDeathTimer;
extern	float					gDragonflyY;
extern	float					gFramesPerSecond;
extern	float					gFramesPerSecondFrac;
extern	float					gGammaFadePercent;
extern	float					gGlobalTransparency;
extern	float					gGravity;
extern	float					gLevelCompletedCoolDownTimer;
extern	float					gMapToUnitValue;
extern	float					gMinHeightOffGround;
extern	float					gPlayerBottomOff;
extern	float					gPlayerToCameraAngle;
extern	float					gPlayerTunnelIndex;
extern	float					gScratchF;
extern	float					gSlotCarStartTimer;
extern	float					gSprinklerPopUpOffset;
extern	float					gSprinklerTimer;
extern	float					gTargetMaxSpeed;
extern	float					gTerrainPolygonSize;
extern	float					gTerrainSuperTileUnitSize;
extern	float					gTerrainSuperTileUnitSizeFrac;
extern	int						gDialogSoundEffect;
extern	int						gGameWindowHeight;
extern	int						gGameWindowWidth;
extern	int						gGatheredRedClovers;
extern	int						gKindlingCount;
extern	int						gLevelNum;
extern	int						gMaxEnemies;
extern	int						gNumActiveParticleGroups;
extern	int						gNumAntHills;
extern	int						gNumAntHillsDestroyed;
extern	int						gNumBowlingPinsDown;
extern	int						gNumCaughtFish;
extern	int						gNumCollisions;
extern	int						gNumDrowingMiceToRescue;
extern	int						gNumDrowningMiceRescued;
extern	int						gNumEnemies;
extern	int						gNumEnemyOfKind[NUM_ENEMY_KINDS];
extern	int						gNumFences;
extern	int						gNumFencesDrawn;
extern	int						gNumFoodOnBasket;
extern	int						gNumFreedBees;
extern	int						gNumKilledFleas;
extern	int						gNumKilledTicks;
extern	int						gNumLineMarkers;
extern	int						gNumLoopingEffects;
extern	int						gNumMice;
extern	int						gNumObjectNodes;
extern	int						gNumObjectsInBG3DGroupList[MAX_BG3D_GROUPS];
extern	int						gNumPointers;
extern	int						gNumPuzzlePiecesFit;
extern	int						gNumSparkles;
extern	int						gNumSplines;
extern	int						gNumSuperTilesDeep;
extern	int						gNumSuperTilesDrawn;
extern	int						gNumSuperTilesWide;
extern	int						gNumTerrainItems;
extern	int						gNumTunnelItems;
extern	int						gNumTunnelSections;
extern	int						gNumTunnelSplinePoints;
extern	int						gNumUniqueSuperTiles;
extern	int						gNumViri;
extern	int						gNumWaterDrawn;
extern	int						gNumWaterPatches;
extern	int						gPolysThisFrame;
extern	int						gScratch;
extern	int						gSlotCarRacingMode;
extern	int						gSuperTileActiveRange;
extern	int						gTerrainTileDepth;
extern	int						gTerrainTileWidth;
extern	int						gTerrainUnitDepth;
extern	int						gTerrainUnitWidth;
extern	int						gTotalFleas;
extern	int						gTotalRedClovers;
extern	int						gTotalTicks;
extern	int						gVRAMUsedThisFrame;
extern	int16_t					**gSuperTileTextureGrid;
extern	int32_t					gNumSpritesInGroupList[MAX_SPRITE_GROUPS];
extern	long					gPrefsFolderDirID;
extern	short					gCurrentSong;
extern	short					gDialogSoundChannel;
extern	short					gPrefsFolderVRefNum;
extern	uint32_t				gAutoFadeStatusBits;
extern	uint32_t				gGameFrameNum;
extern	uint32_t				gGlobalMaterialFlags;
extern	uint32_t				gLoadedScore;
extern	uint32_t				gScore;
