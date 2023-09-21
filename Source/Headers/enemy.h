//
// enemy.h
//

#include "terrain.h"
#include "splineitems.h"


#define	DEFAULT_ENEMY_COLLISION_CTYPES	(CTYPE_MISC|CTYPE_HURTENEMY|CTYPE_ENEMY|CTYPE_TRIGGER2|CTYPE_FENCE|CTYPE_PLAYER)
#define	DEATH_ENEMY_COLLISION_CTYPES	(CTYPE_MISC|CTYPE_ENEMY|CTYPE_FENCE)

#define ENEMY_GRAVITY		3500.0f
#define	ENEMY_SLOPE_ACCEL		3000.0f

#define	EnemyRegenerate	Flag[3]


		/* ENEMY KIND */

enum
{
	ENEMY_KIND_GNOME = 0,
	ENEMY_KIND_HOUSEFLY,
	ENEMY_KIND_EVILPLANT,
	ENEMY_KIND_FLEA,
	ENEMY_KIND_TICK,
	ENEMY_KIND_TOYSOLDIER,
	ENEMY_KIND_OTTO,
	ENEMY_KIND_DRAGONFLY,
	ENEMY_KIND_FROG,
	ENEMY_KIND_MOTH,
	ENEMY_KIND_COMPUTERBUG,
	ENEMY_KIND_ROACH,
	ENEMY_KIND_ANT,
	ENEMY_KIND_PONDFISH,

	NUM_ENEMY_KINDS
};



//=====================================================================
//=====================================================================
//=====================================================================


			/* ENEMY */

ObjNode *MakeEnemySkeleton(Byte skeletonType, short animNum, float x, float z, float scale, float rot, void *moveCall);
extern	void DeleteEnemy(ObjNode *theEnemy);
Boolean DoEnemyCollisionDetect(ObjNode *theEnemy, uint32_t ctype, Boolean useBBoxBottom);
void EnemyTouchedPlayer(ObjNode *enemy, ObjNode *player);
extern	void UpdateEnemy(ObjNode *theNode);
extern	void InitEnemyManager(void);
void DetachEnemyFromSpline(ObjNode *theNode, void *moveCall);
ObjNode *FindClosestEnemy(OGLPoint3D *pt, float *dist);
Boolean	IsWaterInFrontOfEnemy(float r);
void MoveEnemySkipChunk(ObjNode *chunk);



		/* GNOME */

Boolean PrimeEnemy_Gnome(long splineNum, SplineItemType *itemPtr);
Boolean AddEnemy_Gnome(TerrainItemEntryType *itemPtr, float x, float z);


		/* HOUSEFLY */

Boolean PrimeEnemy_HouseFly(long splineNum, SplineItemType *itemPtr);
Boolean AddEnemy_HouseFly(TerrainItemEntryType *itemPtr, float x, float z);


		/* EVIL PLANT */

Boolean AddEnemy_EvilPlant(TerrainItemEntryType *itemPtr, float x, float z);


		/* SNAKE */

void InitSnakeStuff(void);
Boolean AddSnakeGenerator(TerrainItemEntryType *itemPtr, float  x, float z);
void MovePlayer_EatenBySnake(ObjNode *theNode);


		/* FLEA */

Boolean AddEnemy_Flea(TerrainItemEntryType *itemPtr, float x, float z);
Boolean PrimeEnemy_Flea(long splineNum, SplineItemType *itemPtr);
void CountFleas(void);

		/* TICK */

Boolean AddEnemy_Tick(TerrainItemEntryType *itemPtr, float x, float z);
Boolean PrimeEnemy_Tick(long splineNum, SplineItemType *itemPtr);
void CountTicks(void);


		/* TOY SOLIDER */

#define	TOYSOLDIER_JOINTNUM_RIGHTHAND			15

enum
{
	TOYSOLDIER_ANIM_STAND,
	TOYSOLDIER_ANIM_THROW,
	TOYSOLDIER_ANIM_DEATH,
	TOYSOLDIER_ANIM_WALK,
	TOYSOLDIER_ANIM_GOTHIT,
	TOYSOLDIER_ANIM_RELOAD,
	TOYSOLDIER_ANIM_CARRY
};



Boolean AddEnemy_ToySoldier(TerrainItemEntryType *itemPtr, float x, float z);
Boolean PrimeEnemy_ToySoldier(long splineNum, SplineItemType *itemPtr);


		/* OTTO */

Boolean AddEnemy_Otto(TerrainItemEntryType *itemPtr, float x, float z);
Boolean PrimeEnemy_Otto(long splineNum, SplineItemType *itemPtr);


		/* DRAGONFLY */

Boolean AddEnemy_Dragonfly(TerrainItemEntryType *itemPtr, float x, float z);
Boolean PrimeEnemy_Dragonfly(long splineNum, SplineItemType *itemPtr);
void SpawnKillerDragonfly(void);


		/* FROG */

enum
{
	FROG_ANIM_STAND,
	FROG_ANIM_JUMPUP,
	FROG_ANIM_JUMPTONGUE,
	FROG_ANIM_FALL,

	FROG_ANIM_JUMPFORWARD,
	FROG_ANIM_FALLFORWARD,

	FROG_ANIM_GOTHIT,
	FROG_ANIM_SITATTACK,
	FROG_ANIM_DEATH
};

#define FROG_JOINT_TONGUE	2


Boolean AddEnemy_Frog(TerrainItemEntryType *itemPtr, float x, float z);
Boolean AddEnemy_Frog2(TerrainItemEntryType *itemPtr, float x, float z);


		/* MOTH */

Boolean AddEnemy_Moth(TerrainItemEntryType *itemPtr, float x, float z);
Boolean PrimeMothPath(long splineNum, SplineItemType *itemPtr);


		/* COMPUTER BUG */

Boolean AddEnemy_ComputerBug(TerrainItemEntryType *itemPtr, float x, float z);
Boolean PrimeEnemy_ComputerBug(long splineNum, SplineItemType *itemPtr);
void ComputerBugTouchedPlayer(ObjNode *enemy, ObjNode *player);


		/* ROACH */

Boolean AddEnemy_Roach(TerrainItemEntryType *itemPtr, float x, float z);
Boolean PrimeEnemy_Roach(long splineNum, SplineItemType *itemPtr);


		/* ANT */

Boolean AddEnemy_Ant(TerrainItemEntryType *itemPtr, float x, float z);
Boolean PrimeEnemy_Ant(long splineNum, SplineItemType *itemPtr);


		/* POND FISH */

Boolean AddEnemy_PondFish(TerrainItemEntryType *itemPtr, float x, float z);
void MovePlayer_EatenByFish(ObjNode *player);



