//
// water.h
//

#pragma once

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
	uint16_t		type;							// type of water
	Byte			_padding0[2];
	uint32_t		flags;							// flags
	int32_t			height;							// height offset or hard-wired index
	int16_t			numNubs;						// # nubs in water
	Byte			_padding1[6];
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
