//
// items.h
//


#define	BUMBLEBEE_JOINTNUM_HAND		23

extern	void InitItemsManager(void);
void CreateCyclorama(void);

enum
{
	DOOR_MODE_CLOSED,
	DOOR_MODE_OPENING,
	DOOR_MODE_OPEN
};

#define	DoorOpenRot	SpecialF[0]

#define	SILICON_DOOR_SCALE	.5f

Boolean AddDaisy(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddTulip(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddRose(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddGrass(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddShrubRoot(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddDoor(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddBrick(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddPost(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddPebble(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddPoolCoping(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddPoolLeaf(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddDogHouse(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddTulipPot(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddBeachBall(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddChlorineFloat(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddPoolRingFloat(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddDrainPipe(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddGlassBottle(TerrainItemEntryType *itemPtr, float  x, float z);
void DrawCyclorama(ObjNode *theNode, const OGLSetupOutputType *setupInfo);

			/* ITEMS 2 */

Boolean AddLetterBlock(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean PrimeBumbleBee(long splineNum, SplineItemType *itemPtr);
Boolean AddLegoWall(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddFlashLight(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddCrayon(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddDCell(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddCardboardBox(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddShoeBox(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddClosetWall(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddSiliconDoor(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddBookStack(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddPictureFrame(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddLilyPad(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddCatTail(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddPlatformFlower(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddSilverware(TerrainItemEntryType *itemPtr, float  x, float z);


		/* ITEMS 3 */

Boolean AddSodaCan(TerrainItemEntryType *itemPtr, float  x, float z);
void PopSodaCanTab(ObjNode *can);
Boolean AddVeggie(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddJar(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddTinCan(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddDetergent(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddBoxWall(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddGliderPart(TerrainItemEntryType *itemPtr, float  x, float z);

		/* TRAPS */



enum
{
	SPRINKLER_MODE_OFF,
	SPRINKLER_MODE_UP,
	SPRINKLER_MODE_ON,
	SPRINKLER_MODE_DOWN
};

#define	VACUUME_SUCK_DIST	600.0f
#define	VACUUME_SUCK_OFF	200.0f


void InitSprinklerHeads(void);
void UpdateSprinklerHeads(void);
Boolean AddSprinklerHead(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddWindmill(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddFirecracker(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddMouseTrap(TerrainItemEntryType *itemPtr, float  x, float z);
void CountMice(void);
void SeeIfLiftMousetrapLever(ObjNode *player);
Boolean AddTrampoline(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean PrimeVacuume(long splineNum, SplineItemType *itemPtr);




		/* SNAILS */

enum
{
	SNAIL_KIND_INACTIVE = -1,

	SNAIL_KIND_FINDSHELL = 0,
	SNAIL_KIND_SCARECROWHEAD,
	SNAIL_KIND_BOWLING,
	SNAIL_KIND_POOL,
	SNAIL_KIND_SMASHBERRIES,
	SNAIL_KIND_SMASHBERRIESEND,
	SNAIL_KIND_DOGHOUSE,
	SNAIL_KIND_SLOTCAR,
	SNAIL_KIND_RESCUEMICE,
	SNAIL_KIND_PUZZLE,
	SNAIL_KIND_MOTHBALL,
	SNAIL_KIND_SILICONDOOR,
	SNAIL_KIND_REDCLOVERS,
	SNAIL_KIND_FISHING,
	SNAIL_KIND_PICNIC,
	SNAIL_KIND_BEEHIVE,
	SNAIL_KIND_MICEDROWN,
	SNAIL_KIND_GLIDER,
	SNAIL_KIND_SODACAN,
	SNAIL_KIND_BOTTLEKEY
};

#define	FOOD_TO_GET		6

#define	LureMaxWobbleDY	SpecialF[0]

ObjNode *MakeSnail(int slot, float x, float z, int snailKind, int keyColor, int rot, Boolean taskCompleted);
Boolean AddSnail(TerrainItemEntryType *itemPtr, float  x, float z);
ObjNode *FindClosestSnail(OGLPoint3D *from);
void AttachShellToSnail(ObjNode *shell, ObjNode *snail);
Boolean AddSnailShell(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddScarecrow(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddBowlingMarble(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddBowlingPins(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddSquishBerry(TerrainItemEntryType *itemPtr, float  x, float z);
void CountSquishBerries(void);
void AlignShellOnSnail(ObjNode *snail, ObjNode *shell);

Boolean AddPuzzle(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean PrimeHanger(long splineNum, SplineItemType *itemPtr);
Boolean AddFishingLure(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddPicnicBasket(TerrainItemEntryType *itemPtr, float  x, float z);



		/* POWERUPS */

enum
{
	POW_KIND_HEALTH,
	POW_KIND_FLIGHT,
	POW_KIND_MAP,
	POW_KIND_FREELIFE,
	POW_KIND_RAMGRAIN,
	POW_KIND_BUDDYBUG,
	POW_KIND_REDKEY,
	POW_KIND_GREENKEY,
	POW_KIND_BLUEKEY,
	POW_KIND_GREENCLOVER,
	POW_KIND_BLUECLOVER,
	POW_KIND_GOLDCLOVER,
	POW_KIND_SHIELD
};

Boolean AddButterfly(TerrainItemEntryType *itemPtr, float  x, float z);
void StartPowerupVanish(ObjNode *pow);
ObjNode *MakePOW(int powKind, OGLPoint3D *where);
void MovePowerup(ObjNode *theNode);
Boolean DoTrig_Powerup(ObjNode *pow, ObjNode *who, Byte sideBits);
Boolean AddPOW(TerrainItemEntryType *itemPtr, float  x, float z);


		/* PICKUPS */

enum
{
	PICKUP_KIND_SNAILSHELL = 0,
	PICKUP_KIND_ACORN,
	PICKUP_KIND_SCARECROWHEAD,
	PICKUP_KIND_BOWLINGMARBLE,
	PICKUP_KIND_SQUISHBERRY,
	PICKUP_KIND_POW,
	PICKUP_KIND_PUZZLEPIECE,
	PICKUP_KIND_MOTHBALL,
	PICKUP_KIND_CHIP1,
	PICKUP_KIND_CHIP2,
	PICKUP_KIND_BATTERY,
	PICKUP_KIND_FOOD,
	PICKUP_KIND_KINDLING,
	PICKUP_KIND_CANTAB,
	PICKUP_KIND_WHEEL,
	PICKUP_KIND_PROPELLER
};


void UpdateHeldObject(ObjNode *player);
Boolean AddAcorn(TerrainItemEntryType *itemPtr, float  x, float z);
void DefaultGotKickedCallback(ObjNode *player, ObjNode *kickedObj);
void MoveDefaultPickup(ObjNode *theNode);
void DefaultDropObject(ObjNode *player, ObjNode *held);

ObjNode *MakeCheckpoint(OGLPoint3D *where);
void SetCheckpoint(ObjNode *checkpoint, ObjNode *player);
Boolean CheckDropThruFence(ObjNode *player, ObjNode *held);

Boolean AddMothBall(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddSiliconPart(TerrainItemEntryType *itemPtr, float  x, float z);



		/* RIDE BALL */

Boolean AddRideBall(TerrainItemEntryType *itemPtr, float  x, float z);
void MovePlayer_WalkOnBall(ObjNode *theNode);


		/* CHIPMUNK */

Boolean AddChipmunk(TerrainItemEntryType *itemPtr, float  x, float z);


		/* SLOT CAR */

enum
{
	SLOTCAR_RACEMODE_IDLE,
	SLOTCAR_RACEMODE_START,
	SLOTCAR_RACEMODE_RACE,
	SLOTCAR_RACEMODE_OVER
};

Boolean PrimeSlotCar(long splineNum, SplineItemType *itemPtr);
void InitSlotCarRacing(void);
void UpdateSlotCarRacing(void);
Boolean AddFinishLine(TerrainItemEntryType *itemPtr, float  x, float z);


	/* BEE HIVE */


#define	KINDLING_NEEDED	5					// amount of kindling needed to light fire

#define	NUM_FEE_BEES	50

Boolean AddBeeHive(TerrainItemEntryType *itemPtr, float  x, float z);
Boolean AddKindling(TerrainItemEntryType *itemPtr, float  x, float z);
void IgniteKindling(ObjNode *theNode);









