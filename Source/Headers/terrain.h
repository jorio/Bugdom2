//
// Terrain.h
//

#pragma once

#include "main.h"


enum
{
	MAP_ITEM_MYSTARTCOORD		= 0,			// map item # for my start coords
	MAP_ITEM_SQUISHBERRY		= 26,
	MAP_ITEM_FLEA				= 38,
	MAP_ITEM_TICK				= 39,
	MAP_ITEM_MOUSETRAP			= 42,
	MAP_ITEM_ANTHILL			= 51,
	MAP_ITEM_MOTH				= 60
};

#define	ILLEGAL_TERRAIN_Y	-10000.0f			// returned from GetTerrainY if no terrain is loaded

		/* SUPER TILE MODES */

enum
{
	SUPERTILE_MODE_FREE,
	SUPERTILE_MODE_USED
};

#define	DEFAULT_TERRAIN_SCALE		150.0f											// size of a polygon
#define	SUPERTILE_TEXMAP_SIZE		128												// the width & height of a supertile's texture

#define	OREOMAP_TILE_SIZE			16 												// pixel w/h of texture tile


#define	SUPERTILE_SIZE				8  												// size of a super-tile / terrain object zone

#define	NUM_TRIS_IN_SUPERTILE		(SUPERTILE_SIZE * SUPERTILE_SIZE * 2)			// 2 triangles per tile
#define	NUM_VERTICES_IN_SUPERTILE	((SUPERTILE_SIZE+1)*(SUPERTILE_SIZE+1))			// # vertices in a supertile

#define	MAX_SUPERTILE_ACTIVE_RANGE	9

#define	SUPERTILE_DIST_WIDE			(gSuperTileActiveRange*2)
#define	SUPERTILE_DIST_DEEP			(gSuperTileActiveRange*2)

								// # visible supertiles * 2 players * 2 buffers
								// We need the x2 buffer because we dont free unused supertiles
								// until after we've allocated new supertiles, so we'll always
								// need more supertiles than are actually ever used.

#define	MAX_SUPERTILES			((MAX_SUPERTILE_ACTIVE_RANGE*2 * MAX_SUPERTILE_ACTIVE_RANGE*2)*2)	// the final *2 is because the old supertiles are not deleted until
																											// after new ones are created, thus we need some extas - worst case
																											// scenario is twice as many.

#define	MAX_TERRAIN_WIDTH		400
#define	MAX_TERRAIN_DEPTH		400

#define	MAX_SUPERTILES_WIDE		(MAX_TERRAIN_WIDTH/SUPERTILE_SIZE)
#define	MAX_SUPERTILES_DEEP		(MAX_TERRAIN_DEPTH/SUPERTILE_SIZE)


#define	MAX_SUPERTILE_TEXTURES	(MAX_SUPERTILES_WIDE*MAX_SUPERTILES_DEEP)


//=====================================================================


struct SuperTileMemoryType
{
	Byte				hiccupTimer;							// # frames to skip for use
	Byte				mode;									// free, used, etc.
	float				x,z,y;									// world coords
	int					left,back;								// integer coords of back/left corner
	int					tileRow,tileCol;						// tile row/col of the start of this supertile
	MOMaterialObject	*texture;								// refs to materials
	MOVertexArrayData	*meshData;								// mesh's data for the supertile
	OGLBoundingBox		bBox;									// bounding box
};
typedef struct SuperTileMemoryType SuperTileMemoryType;


typedef struct
{
	uint16_t		numItems;
	uint16_t		itemIndex;
}SuperTileItemIndexType;


#define	BOTTOMLESS_PIT_Y	-100000.0f				// to identify a blank area on Cloud Level


		/* TERRAIN ITEM FLAGS */

enum
{
	ITEM_FLAGS_INUSE	=	(1),
	ITEM_FLAGS_USER1	=	(1<<1),
	ITEM_FLAGS_USER2	=	(1<<2),
	ITEM_FLAGS_USER3	=	(1<<3)
};


enum
{
	SPLIT_BACKWARD = 0,
	SPLIT_FORWARD,
	SPLIT_ARBITRARY
};


typedef	struct
{
	uint16_t	supertileIndex;
	uint8_t		statusFlags;
	Boolean		playerHereFlag;
}SuperTileStatus;

enum									// statusFlags
{
	SUPERTILE_IS_DEFINED			=	1,
	SUPERTILE_IS_USED_THIS_FRAME	=	(1<<1)
};


typedef struct
{
	Boolean		isUsed;
	Byte		type;
	float		amplitude;						// max height of wave
	float		radius;							// radius of deformation wave
	float		speed;							// speed of wave
	float		oneOverWaveLength;				// 1.0 / wavelength
	float		radialWidth;					// width of radial wave
	float		decayRate;						// decay rate of wave
	OGLVector2D	origin;							// origin of wave
}DeformationType;


#define	MAX_DEFORMATIONS	8

enum
{
	DEFORMATION_TYPE_JELLO,
	DEFORMATION_TYPE_RADIALWAVE,
	DEFORMATION_TYPE_CONTINUOUSWAVE,
	DEFORMATION_TYPE_DAMPEN,
	DEFORMATION_TYPE_WELL
};


#define	MAX_LINEMARKERS	20

typedef struct
{
	int16_t	unused;
	int16_t	infoBits;

	float	x[2],z[2];			// the two endpoints
}LineMarkerDefType;


//=====================================================================


void SetTerrainScale(float polygonSize);

void CreateSuperTileMemoryList(void);
void DisposeSuperTileMemoryList(void);
void DisposeTerrain(void);
void GetSuperTileInfo(int x, int z, int *superCol, int *superRow, int *tileCol, int *tileRow);
void InitTerrainManager(void);
float	GetTerrainY(float x, float z);
float	GetMinTerrainY(float x, float z, short group, short type, float scale);
void InitCurrentScrollSettings(void);

void BuildTerrainItemList(void);
void AddTerrainItemsOnSuperTile(int row, int col);
Boolean TrackTerrainItem(ObjNode *theNode);
Boolean TrackTerrainItem_FromInit(ObjNode *theNode);
Boolean SeeIfCoordsOutOfRange(float x, float z);
void FindPlayerStartCoordItems(void);
void InitSuperTileGrid(void);
void RotateOnTerrain(ObjNode *theNode, float yOffset, OGLVector3D *surfaceNormal);
void DoPlayerTerrainUpdate(float x, float y);
void CalcTileNormals(int row, int col, OGLVector3D *n1, OGLVector3D *n2);
void CalcTileNormals_NotNormalized(int row, int col, OGLVector3D *n1, OGLVector3D *n2);
void CalculateSplitModeMatrix(void);
void CalculateSupertileVertexNormals(MOVertexArrayData *meshData, int startRow, int startCol);

short NewSuperTileDeformation(DeformationType *data);
void DeleteTerrainDeformation(short	i);
void UpdateDeformationCoords(short defNum, float x, float z);

void DoItemShadowCasting(void);
Boolean SeeIfCrossedLineMarker(ObjNode *theNode, int *whichLine);
