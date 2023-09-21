//
// water.h
//

#ifndef WATER_H
#define WATER_H

#define	MAX_WATER_POINTS	100			// note:  cannot change this without breaking data files!!

enum
{
	WATER_FLAG_FIXEDHEIGHT	= (1)
};

enum
{
	WATER_TYPE_BLUEWATER,
	WATER_TYPE_POOLWATER,
	WATER_TYPE_GARBAGE,

	NUM_WATER_TYPES
};


typedef struct		// NOTE: MUST MATCH OREOTERRAIN DATA!!!
{
	uint16_t			type;							// type of water
	uint32_t			flags;							// flags
	long			height;							// height offset or hard-wired index
	short			numNubs;						// # nubs in water
	long			reserved;						// for future use
	OGLPoint2D		nubList[MAX_WATER_POINTS];		// nub list

	float			hotSpotX,hotSpotZ;				// hot spot coords
	Rect			bBox;							// bounding box of water area
}WaterDefType;




//============================================

void PrimeTerrainWater(void);
void DisposeWater(void);
Boolean DoWaterCollisionDetect(ObjNode *theNode, float x, float y, float z, int *patchNum);
Boolean IsXZOverWater(float x, float z);
Boolean GetWaterY(float x, float z, float *y);
void RaiseWater(void);
void ResetRisingWater(void);


	/* RAIN */

void InitRainEffect(void);
void StartRainEffect(void);
void StopRainEffect(void);

	/* RIPPLE */

void CreateNewRipple(float x, float z, float baseScale, float scaleSpeed, float fadeRate);

#endif


