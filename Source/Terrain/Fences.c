/**********************/
/*   	FENCES.C      */
/**********************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawFences(ObjNode *theNode);
static void SubmitFence(int f, float camX, float camZ);
static void MakeFenceGeometry(void);
static void DrawFenceNormals(short f);


/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_FENCES			90
#define	MAX_NUBS_IN_FENCE	80


#define	FENCE_SINK_FACTOR	30.0f

enum
{
	FENCE_TYPE_GRASS,
	FENCE_TYPE_LAWNEDGING,
	FENCE_TYPE_DOGHAIR,
	FENCE_TYPE_BRICKWALL,
	FENCE_TYPE_DOGCOLLAR,
	FENCE_TYPE_DOGHAIRDENSE,
	FENCE_TYPE_CARD,
	FENCE_TYPE_BLOCK,
	FENCE_TYPE_BALSA,
	FENCE_TYPE_CLOTH,
	FENCE_TYPE_BOOKS,
	FENCE_TYPE_COMPUTER,
	FENCE_TYPE_SHOEBOX,
	FENCE_TYPE_WATERGRASS,
	FENCE_TYPE_GARBAGECAN,
	FENCE_TYPE_BOXFENCE,

	NUM_FENCE_TYPES
};


/**********************/
/*     VARIABLES      */
/**********************/

int				gNumFences = 0;
int				gNumFencesDrawn = 0;
FenceDefType	*gFenceList = nil;


static const short gFenceTexture[NUM_FENCE_TYPES][2] =
{
	[FENCE_TYPE_GRASS]			= { SPRITE_GROUP_GLOBAL,		GLOBAL_SObjType_Fence_Grass },
	[FENCE_TYPE_LAWNEDGING]		= { SPRITE_GROUP_LEVELSPECIFIC,	GARDEN_SObjType_Fence_Edging },
	[FENCE_TYPE_DOGHAIR]		= { SPRITE_GROUP_LEVELSPECIFIC,	FIDO_SObjType_Fence_DogHair },
	[FENCE_TYPE_BRICKWALL]		= { SPRITE_GROUP_GLOBAL,		GLOBAL_SObjType_Fence_Brick },
	[FENCE_TYPE_DOGCOLLAR]		= { SPRITE_GROUP_LEVELSPECIFIC,	FIDO_SObjType_Fence_DogCollar },
	[FENCE_TYPE_DOGHAIRDENSE]	= { SPRITE_GROUP_LEVELSPECIFIC,	FIDO_SObjType_Fence_DogHairDense },
	[FENCE_TYPE_CARD]			= { SPRITE_GROUP_LEVELSPECIFIC,	PLAYROOM_SObjType_Fence_Cards },
	[FENCE_TYPE_BLOCK]			= { SPRITE_GROUP_LEVELSPECIFIC,	PLAYROOM_SObjType_Fence_Blocks },
	[FENCE_TYPE_BALSA]			= { SPRITE_GROUP_LEVELSPECIFIC,	BALSA_SObjType_Fence_Balsa },
	[FENCE_TYPE_CLOTH]			= { SPRITE_GROUP_LEVELSPECIFIC,	CLOSET_SObjType_Fence_Cloth },
	[FENCE_TYPE_BOOKS]			= { SPRITE_GROUP_LEVELSPECIFIC,	CLOSET_SObjType_Fence_Books },
	[FENCE_TYPE_COMPUTER]		= { SPRITE_GROUP_LEVELSPECIFIC,	CLOSET_SObjType_Fence_Computer },
	[FENCE_TYPE_SHOEBOX]		= { SPRITE_GROUP_LEVELSPECIFIC,	CLOSET_SObjType_Fence_ShoeBox },
	[FENCE_TYPE_WATERGRASS]		= { SPRITE_GROUP_LEVELSPECIFIC,	PARK_SObjType_WaterGrassFence },
	[FENCE_TYPE_GARBAGECAN]		= { SPRITE_GROUP_LEVELSPECIFIC,	GARBAGE_SObjType_Fence_GarbageCan },
	[FENCE_TYPE_BOXFENCE]		= { SPRITE_GROUP_LEVELSPECIFIC,	GARBAGE_SObjType_Fence_Box },
};


static const float gFenceHeight[NUM_FENCE_TYPES] =
{
	[FENCE_TYPE_GRASS]			= 1100,
	[FENCE_TYPE_LAWNEDGING]		= 300,
	[FENCE_TYPE_DOGHAIR]		= 600,
	[FENCE_TYPE_BRICKWALL]		= 1300,
	[FENCE_TYPE_DOGCOLLAR]		= 600,
	[FENCE_TYPE_DOGHAIRDENSE]	= 700,
	[FENCE_TYPE_CARD]			= 550,
	[FENCE_TYPE_BLOCK]			= 800,
	[FENCE_TYPE_BALSA]			= 3800,
	[FENCE_TYPE_CLOTH]			= 1800,
	[FENCE_TYPE_BOOKS]			= 1100,
	[FENCE_TYPE_COMPUTER]		= 800,
	[FENCE_TYPE_SHOEBOX]		= 1000,
	[FENCE_TYPE_WATERGRASS]		= 1700,
	[FENCE_TYPE_GARBAGECAN]		= 1700,
	[FENCE_TYPE_BOXFENCE]		= 800,
};

static const float gFenceSink[NUM_FENCE_TYPES] =
{
	[FENCE_TYPE_GRASS]			= FENCE_SINK_FACTOR,
	[FENCE_TYPE_LAWNEDGING]		= FENCE_SINK_FACTOR,
	[FENCE_TYPE_DOGHAIR]		= FENCE_SINK_FACTOR/2,
	[FENCE_TYPE_BRICKWALL]		= FENCE_SINK_FACTOR,
	[FENCE_TYPE_DOGCOLLAR]		= FENCE_SINK_FACTOR/2,
	[FENCE_TYPE_DOGHAIRDENSE]	= FENCE_SINK_FACTOR/2,
	[FENCE_TYPE_CARD]			= 0,
	[FENCE_TYPE_BLOCK]			= 0,
	[FENCE_TYPE_BALSA]			= FENCE_SINK_FACTOR*3,
	[FENCE_TYPE_CLOTH]			= -650,
	[FENCE_TYPE_BOOKS]			= FENCE_SINK_FACTOR/2,
	[FENCE_TYPE_COMPUTER]		= FENCE_SINK_FACTOR/2,
	[FENCE_TYPE_SHOEBOX]		= FENCE_SINK_FACTOR,
	[FENCE_TYPE_WATERGRASS]		= FENCE_SINK_FACTOR,
	[FENCE_TYPE_GARBAGECAN]		= FENCE_SINK_FACTOR,
	[FENCE_TYPE_BOXFENCE]		= FENCE_SINK_FACTOR,
};

static const Boolean gFenceIsLit[NUM_FENCE_TYPES] =
{
	[FENCE_TYPE_GRASS]			= false,
	[FENCE_TYPE_LAWNEDGING]		= false,
	[FENCE_TYPE_DOGHAIR]		= true,
	[FENCE_TYPE_BRICKWALL]		= false,
	[FENCE_TYPE_DOGCOLLAR]		= true,
	[FENCE_TYPE_DOGHAIRDENSE]	= true,
	[FENCE_TYPE_CARD]			= true,
	[FENCE_TYPE_BLOCK]			= true,
	[FENCE_TYPE_BALSA]			= false,
	[FENCE_TYPE_CLOTH]			= true,
	[FENCE_TYPE_BOOKS]			= true,
	[FENCE_TYPE_COMPUTER]		= true,
	[FENCE_TYPE_SHOEBOX]		= true,
	[FENCE_TYPE_WATERGRASS]		= false,
	[FENCE_TYPE_GARBAGECAN]		= false,
	[FENCE_TYPE_BOXFENCE]		= false,
};

static MOMaterialObject			*gFenceMaterials[MAX_FENCES];				// illegal refs to material for each fence in terrain

static MOVertexArrayData		gFenceMeshes[MAX_FENCES];

static MOTriangleIndecies		gFenceTriangles[MAX_FENCES][MAX_NUBS_IN_FENCE*2];
static OGLPoint3D				gFencePoints[MAX_FENCES][MAX_NUBS_IN_FENCE*2];
static OGLTextureCoord			gFenceUVs[MAX_FENCES][MAX_NUBS_IN_FENCE*2];
static OGLColorRGBA_Byte		gFenceColors[MAX_FENCES][MAX_NUBS_IN_FENCE*2];
static OGLVector3D				gFenceNormals[MAX_FENCES][MAX_NUBS_IN_FENCE*2];

static MOVertexArrayData		gBackFenceMeshes[MAX_FENCES];
static MOTriangleIndecies		gBackFenceTriangles[MAX_FENCES][MAX_NUBS_IN_FENCE * 2];
static OGLVector3D				gBackFenceNormals[MAX_FENCES][MAX_NUBS_IN_FENCE * 2];


/********************** DISPOSE FENCES *********************/

void DisposeFences(void)
{
	if (!gFenceList)
		return;

	for (int i = 0; i < gNumFences; i++)
	{
		if (gFenceList[i].sectionVectors)
			SafeDisposePtr((Ptr)gFenceList[i].sectionVectors);			// nuke section vectors
		gFenceList[i].sectionVectors = nil;

		if (gFenceList[i].sectionNormals)
			SafeDisposePtr((Ptr)gFenceList[i].sectionNormals);			// nuke normal vectors
		gFenceList[i].sectionNormals = nil;

		if (gFenceList[i].nubList)
			SafeDisposePtr((Ptr)gFenceList[i].nubList);
		gFenceList[i].nubList = nil;
	}

	SafeDisposePtr((Ptr)gFenceList);
	gFenceList = nil;
	gNumFences = 0;
}



/********************* PRIME FENCES ***********************/
//
// Called during terrain prime function to initialize
//

void PrimeFences(void)
{
long					numNubs,type, group, sprite;
FenceDefType			*fence;
OGLPoint3D				*nubs;
ObjNode					*obj;
float					sink;


	GAME_ASSERT(gNumFences <= MAX_FENCES);


			/******************************/
			/* ADJUST TO GAME COORDINATES */
			/******************************/

	for (int f = 0; f < gNumFences; f++)
	{
		fence 				= &gFenceList[f];					// point to this fence
		nubs 				= fence->nubList;					// point to nub list
		numNubs 			= fence->numNubs;					// get # nubs in fence
		type 				= fence->type;						// get fence type

		group = gFenceTexture[type][0];							// get sprite info
		sprite = gFenceTexture[type][1];						// get sprite info

		GAME_ASSERT(sprite <= gNumSpritesInGroupList[group]);
		GAME_ASSERT(numNubs != 1);
		GAME_ASSERT(numNubs <= MAX_NUBS_IN_FENCE);

		sink = gFenceSink[type];								// get fence sink factor

		for (int i = 0; i < numNubs; i++)						// adjust nubs
		{
			nubs[i].x *= gMapToUnitValue;
			nubs[i].z *= gMapToUnitValue;
			nubs[i].y = GetTerrainY(nubs[i].x,nubs[i].z) - sink;	// calc Y
		}

		/* CALCULATE VECTOR FOR EACH SECTION */

		fence->sectionVectors = (OGLVector2D *)AllocPtr(sizeof(OGLVector2D) * (numNubs-1));		// alloc array to hold vectors
		GAME_ASSERT(fence->sectionVectors);

		for (int i = 0; i < (numNubs-1); i++)
		{
			fence->sectionVectors[i].x = nubs[i+1].x - nubs[i].x;
			fence->sectionVectors[i].y = nubs[i+1].z - nubs[i].z;

			OGLVector2D_Normalize(&fence->sectionVectors[i], &fence->sectionVectors[i]);
		}


		/* CALCULATE NORMALS FOR EACH SECTION */

		fence->sectionNormals = (OGLVector2D *)AllocPtr(sizeof(OGLVector2D) * (numNubs-1));		// alloc array to hold vectors
		GAME_ASSERT(fence->sectionNormals);

		for (int i = 0; i < (numNubs-1); i++)
		{
			float x = fence->sectionVectors[i].x;				// get section vector (as calculated above)
			float z = fence->sectionVectors[i].y;

			fence->sectionNormals[i].x = -z;					//  reduced cross product to get perpendicular normal
			fence->sectionNormals[i].y = x;
			OGLVector2D_Normalize(&fence->sectionNormals[i], &fence->sectionNormals[i]);
		}

	}

			/***********************/
			/* MAKE FENCE GEOMETRY */
			/***********************/

	MakeFenceGeometry();

		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE FENCE DRAWING AT THE DESIRED TIME */
		/*************************************************************************/
		//
		// The fences need to be drawn after the Cyc object, but before any sprite or font objects.
		//

	obj = MakeNewDriverObject(FENCE_SLOT, DrawFences, NULL);
	obj->StatusBits |= STATUS_BIT_NOLIGHTING;
	// Do NOT set STATUS_BIT_DOUBLESIDED -- DrawFences will decide whether to do backface culling or not.
	// STATUS_BIT_DOUBLESIDED will cause the object task loop to enable GL_LIGHT_MODEL_TWO_SIDE,
	// which massively tanks performance with large meshes on modern GPUs!
	// As a workaround, we submit back and front meshes separately.
}


/*************** MAKE FENCE GEOMETRY *********************/

static void MakeFenceGeometry(void)
{
int						group, sprite;
int						numNubs;
uint16_t				type;
float					u,height,aspectRatio,textureUOff;
FenceDefType			*fence;
OGLPoint3D				*nubs;
float					minX,minY,minZ,maxX,maxY,maxZ;

	for (int f = 0; f < gNumFences; f++)
	{
				/******************/
				/* GET FENCE INFO */
				/******************/

		fence = &gFenceList[f];								// point to this fence
		nubs = fence->nubList;								// point to nub list
		numNubs = fence->numNubs;							// get # nubs in fence
		type = fence->type;									// get fence type
		height = gFenceHeight[type];						// get fence height

		group = gFenceTexture[type][0];						// get sprite info
		sprite = gFenceTexture[type][1];

		aspectRatio = gSpriteGroupList[group][sprite].aspectRatio;	// get aspect ratio

		textureUOff = 1.0f / height * aspectRatio;			// calc UV offset

		gFenceMaterials[f] = gSpriteGroupList[group][sprite].materialObject;	// keep illegal ref to the material


					/***************************/
					/* SET VERTEX ARRAY HEADER */
					/***************************/

		MOVertexArrayData* mesh = &gFenceMeshes[f];

		mesh->numMaterials			= -1;			// we submit these manually
		mesh->materials[0]			= nil;
		mesh->points 				= gFencePoints[f];
		mesh->triangles				= gFenceTriangles[f];
		mesh->uvs[0]				= gFenceUVs[f];
		mesh->normals				= nil;
		mesh->colorsByte			= gFenceColors[f];
		mesh->colorsFloat			= nil;
		mesh->numPoints				= numNubs * 2;				// 2 vertices per nub
		mesh->numTriangles			= (numNubs-1) * 2;			// 2 faces per nub (minus 1st)


				/* BUILD TRIANGLE INFO */

		for (int i = 0, j = 0; i < MAX_NUBS_IN_FENCE; i++, j+=2)
		{
			gFenceTriangles[f][j].vertexIndices[0] = 1 + j;
			gFenceTriangles[f][j].vertexIndices[1] = 0 + j;
			gFenceTriangles[f][j].vertexIndices[2] = 3 + j;

			gFenceTriangles[f][j+1].vertexIndices[0] = 3 + j;
			gFenceTriangles[f][j+1].vertexIndices[1] = 0 + j;
			gFenceTriangles[f][j+1].vertexIndices[2] = 2 + j;
		}

				/* INIT VERTEX COLORS */

		for (int i = 0; i < (MAX_NUBS_IN_FENCE*2); i++)
			gFenceColors[f][i].r = gFenceColors[f][i].g = gFenceColors[f][i].b = 0xff;


				/**********************/
				/* BUILD POINTS, UV's */
				/**********************/

		maxX = maxY = maxZ = -1000000;									// build new bboxes while we do this
		minX = minY = minZ = -maxX;

		u = 0;
		for (int i = 0, j = 0; i < numNubs; i++, j+=2)
		{
			float		x,y,z,y2;

					/* GET COORDS */

			x = nubs[i].x;
			z = nubs[i].z;
			y = nubs[i].y;
			y2 = y + height;

					/* CHECK BBOX */

			if (x < minX)	minX = x;									// find min/max bounds for bbox
			if (x > maxX)	maxX = x;
			if (z < minZ)	minZ = z;
			if (z > maxZ)	maxZ = z;
			if (y < minY)	minY = y;
			if (y2 > maxY)	maxY = y2;


				/* SET COORDS */

			gFencePoints[f][j].x = x;
			gFencePoints[f][j].y = y;
			gFencePoints[f][j].z = z;

			gFencePoints[f][j+1].x = x;
			gFencePoints[f][j+1].y = y2;
			gFencePoints[f][j+1].z = z;


				/* CALC UV COORDS */

			if (i > 0)
			{
				u += CalcDistance3D(gFencePoints[f][j].x, gFencePoints[f][j].y, gFencePoints[f][j].z,
									gFencePoints[f][j-2].x, gFencePoints[f][j-2].y, gFencePoints[f][j-2].z) * textureUOff;
			}

			gFenceUVs[f][j].v 		= 0;									// bottom
			gFenceUVs[f][j+1].v 	= 1.0;									// top
			gFenceUVs[f][j].u 		= gFenceUVs[f][j+1].u = u;
		}

				/* SET CALCULATED BBOX */

		fence->bBox.min.x = minX;
		fence->bBox.max.x = maxX;
		fence->bBox.min.y = minY;
		fence->bBox.max.y = maxY;
		fence->bBox.min.z = minZ;
		fence->bBox.max.z = maxZ;
		fence->bBox.isEmpty = false;



				/**************************************************************/
				/* BUILD VERTEX NORMALS + BACKFACES IF FENCE REQUIRES GOURAUD */
				/**************************************************************/

		if (gFenceIsLit[type])
		{
					/* COMPUTE VERTEX NORMALS FOR FRONTFACES */

			mesh->normals = gFenceNormals[f];	// grab buffer
			
			SDL_memset(mesh->normals, 0, mesh->numPoints * sizeof(mesh->normals[0]));		// zero out all normals

			for (int i = 0, j = 0; i < numNubs - 1; i++, j += 2)
			{
//				float faceNormalX = nubs[i+1].z - nubs[i].z;			// compute face normal
//				float faceNormalZ = nubs[i+1].x - nubs[i].x;
				float faceNormalX = fence->sectionNormals[i].x;			// grab face normal
				float faceNormalZ = fence->sectionNormals[i].y;

				for (int k = 0; k < 4; k++)								// add to normal of all 4 vertices making up the face
				{
					mesh->normals[j+k].x += faceNormalX;
					mesh->normals[j+k].z += faceNormalZ;
				}
			}

			for (int j = 0; j < mesh->numPoints; j++)					// normalize vertex normals to unit length
			{
				OGLVector3D_Normalize(&mesh->normals[j], &mesh->normals[j]);
			}

					/* MAKE BACKFACE MESH */

			MOVertexArrayData* backMesh = &gBackFenceMeshes[f];
			*backMesh = *mesh;								// copy most things from front mesh
			backMesh->triangles = gBackFenceTriangles[f];	// grab own triangle buffer
			backMesh->normals = gBackFenceNormals[f];		// grab own normal buffer

			// Wind all faces in opposite direction
			FlipFaceWinding(mesh->triangles, backMesh->triangles, mesh->numTriangles);

			// Flip all normals
			for (int j = 0; j < mesh->numPoints; j++)
			{
				OGLVector3D normal = gFenceNormals[f][j];
				normal.x = -normal.x;
				normal.y = -normal.y;
				normal.z = -normal.z;
				gBackFenceMeshes[f].normals[j] = normal;
			}
		}
	}
}


#pragma mark -

/********************* DRAW FENCES ***********************/

static void DrawFences(ObjNode *theNode)
{
long			type;
float			cameraX, cameraZ;

	(void) theNode;

	GLboolean hadCullFace = 0 == (theNode->StatusBits & STATUS_BIT_DOUBLESIDED);
	GLboolean hasCullFace = hadCullFace;
	GAME_DEBUGASSERT(hadCullFace == glIsEnabled(GL_CULL_FACE));		// this must match the driver's status bits

			/* GET CAMERA COORDS */

	cameraX = gGameView.cameraPlacement.cameraLocation.x;
	cameraZ = gGameView.cameraPlacement.cameraLocation.z;


			/* SET GLOBAL MATERIAL FLAGS */

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_ALWAYSBLEND;


			/*******************/
			/* DRAW EACH FENCE */
			/*******************/

	gNumFencesDrawn = 0;

	for (int f = 0; f < gNumFences; f++)
	{
		type = gFenceList[f].type;							// get type

					/* DO BBOX CULLING */

		if (OGL_IsBBoxVisible(&gFenceList[f].bBox, nil))
		{
					/* CHECK LIGHTING */

			if (gFenceIsLit[type])
			{
				OGL_EnableLighting();
				if (!hasCullFace)
				{
					glEnable(GL_CULL_FACE);
					hasCullFace = true;
				}
			}
			else
			{
				OGL_DisableLighting();
				if (hasCullFace)
				{
					glDisable(GL_CULL_FACE);
					hasCullFace = false;
				}
			}

				/* SUBMIT GEOMETRY */

			SubmitFence(f, cameraX, cameraZ);
			gNumFencesDrawn++;

			if (gDebugMode == 2)
			{
				DrawFenceNormals(f);
			}
		}
	}

	gGlobalMaterialFlags = 0;

	if (hasCullFace != hadCullFace)
	{
		if (hadCullFace)
		{
			glEnable(GL_CULL_FACE);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}
	}
}


/****************** DRAW FENCE NORMALS ***************************/

static void DrawFenceNormals(short f)
{
int				numNubs;
OGLPoint3D		*nubs;
OGLVector2D		*normals;
float			x,y,z,nx,nz;

	OGL_PushState();
	glDisable(GL_TEXTURE_2D);
	SetColor4f(1,0,0,1);
	glLineWidth(3);

	numNubs  	= gFenceList[f].numNubs - 1;					// get # nubs in fence minus 1
	nubs  		= gFenceList[f].nubList;						// get ptr to nub list
	normals 	= gFenceList[f].sectionNormals;					// get ptr to normals

	for (int i = 0; i < numNubs; i++)
	{
		glBegin(GL_LINES);

		x = nubs[i].x;
		y = nubs[i].y + 200.0f;			// show normal up a ways
		z = nubs[i].z;

		nx = normals[i].x * 150.0f;
		nz = normals[i].y * 150.0f;

		glVertex3f(x-nx,y,z-nz);
		glVertex3f(x + nx,y, z + nz);

		glEnd();
	}
	OGL_PopState();
	glLineWidth(1);

}


/******************** SUBMIT FENCE **************************/
//
// Visibility checks have already been done, so there's a good chance the fence is visible
//

static void SubmitFence(int f, float camX, float camZ)
{
int						doAutoFade = gAutoFadeStatusBits;
float					dist,alpha, autoFadeStart = gAutoFadeStartDist;
float					autoFadeEndDist = gAutoFadeEndDist;
float					autoFadeRangeFrac = gAutoFadeRange_Frac;
long					i,numNubs,j;
FenceDefType			*fence;
OGLPoint3D				*nubs;
Boolean					overrideAlphaFunc = false;

			/* GET FENCE INFO */

	fence = &gFenceList[f];								// point to this fence
	nubs = fence->nubList;								// point to nub list
	numNubs = fence->numNubs;							// get # nubs in fence


				/* CALC & SET TRANSPARENCY */

	for (i = j = 0; i < numNubs; i++, j+=2)
	{
				/* CALC & SET TRANSPARENCY */

		if (doAutoFade)														// see if this level has xparency
		{
			dist = CalcQuickDistance(camX, camZ, nubs[i].x, nubs[i].z);		// see if in fade zone
			if (dist < autoFadeStart)
				alpha = 1.0;
			else
			{
				overrideAlphaFunc = true;
				if (dist >= autoFadeEndDist)
					alpha = 0.0;
				else
				{
					dist -= autoFadeStart;										// calc xparency %
					dist *= autoFadeRangeFrac;
					if (dist < 0.0f)
						alpha = 0;
					else
						alpha = 1.0f - dist;
				}
			}
		}
		else
			alpha = 1.0f;

		gFenceColors[f][j].a = gFenceColors[f][j+1].a = 255.0f * alpha;
	}



		/*******************/
		/* SUBMIT GEOMETRY */
		/*******************/
		//
		// Fences often have 1-bit alpha transparency, and we want nice sharp edges.
		// The DrawMaterial() function will set the appropriate alpha func to keep the edges sharp,
		// however, this will also cause any AutoFade vertices to vanish instead of fade out,
		// so if any part of the fence is alpha faded then we need to set the alpha func back to normal.
		//

		/* ACTIVATE MATERIAL */

	MO_DrawMaterial(gFenceMaterials[f]);

	if (overrideAlphaFunc)				// override alpha func settings if any vertex alphas are not opaque
		glAlphaFunc(GL_NOTEQUAL, 0);


			/* SUBMIT GEO */

	MO_DrawGeometry_VertexArray(&gFenceMeshes[f]);


			/* SUBMIT BACKFACES SEPARATELY IF LIT */

	if (gFenceIsLit[fence->type])
	{
		MO_DrawGeometry_VertexArray(&gBackFenceMeshes[f]);
	}
}




#pragma mark -

/******************** DO FENCE COLLISION **************************/
//
// returns True if hit a fence
//

Boolean DoFenceCollision(ObjNode *theNode)
{
double			fromX,fromZ,toX,toZ;
long			f,numFenceSegments,i,numReScans;
double			segFromX,segFromZ,segToX,segToZ;
OGLPoint3D		*nubs;
Boolean			intersected;
float			intersectX,intersectZ;
OGLVector2D		lineNormal;
double			radius;
double			oldX,oldZ,newX,newZ;
Boolean			hit = false,letGoOver = false, letGoUnder = false;

			/* CALC MY MOTION LINE SEGMENT */

	oldX = theNode->OldCoord.x;						// from old coord
	oldZ = theNode->OldCoord.z;
	newX = gCoord.x;								// to new coord
	newZ = gCoord.z;
	radius = theNode->BoundingSphereRadius;


			/****************************************/
			/* SCAN THRU ALL FENCES FOR A COLLISION */
			/****************************************/

	for (f = 0; f < gNumFences; f++)
	{
		int		type;
		float	temp;
		float	r2 = radius + 20.0f;								// tweak a little to be safe

		if ((oldX == newX) && (oldZ == newZ))						// if no movement, then don't check anything
			break;


				/* SEE IF CAN GO OVER POSSIBLY */

		type = gFenceList[f].type;
		switch(type)
		{
			case	FENCE_TYPE_LAWNEDGING:					// things can go over this
					letGoOver = true;
					break;

			case	FENCE_TYPE_CLOTH:						// things can go under these
					letGoUnder = true;
					break;

			case	FENCE_TYPE_DOGHAIR:						// ignore these
					continue;


			default:
					letGoOver = false;
					letGoUnder = false;
		}


		/* QUICK CHECK TO SEE IF OLD & NEW COORDS (PLUS RADIUS) ARE OUTSIDE OF FENCE'S BBOX */

		temp = gFenceList[f].bBox.min.x - r2;
		if ((oldX < temp) && (newX < temp))
			continue;
		temp = gFenceList[f].bBox.max.x + r2;
		if ((oldX > temp) && (newX > temp))
			continue;

		temp = gFenceList[f].bBox.min.z - r2;
		if ((oldZ < temp) && (newZ < temp))
			continue;
		temp = gFenceList[f].bBox.max.z + r2;
		if ((oldZ > temp) && (newZ > temp))
			continue;

		nubs = gFenceList[f].nubList;				// point to nub list
		numFenceSegments = gFenceList[f].numNubs-1;	// get # line segments in fence



				/**********************************/
				/* SCAN EACH SECTION OF THE FENCE */
				/**********************************/

		numReScans = 0;
		for (i = 0; i < numFenceSegments; i++)
		{
			float	cross;

					/* GET LINE SEG ENDPOINTS */

			segFromX = nubs[i].x;
			segFromZ = nubs[i].z;
			segToX = nubs[i+1].x;
			segToZ = nubs[i+1].z;

					/* SEE IF ROUGHLY CAN GO OVER */

			if (letGoOver)
			{
				float y = GetTerrainY(segFromX,segFromZ);
				float y2 = GetTerrainY(segToX,segToZ);

				if (y2 < y)									// use lowest endpoint for this
					y = y2;

				y += gFenceHeight[type];					// calc top of fence
				if ((gCoord.y + theNode->BottomOff) >= y)	// see if bottom of the object is over it
					continue;
			}

					/* SEE IF ROUGHLY CAN GO UNDER */
			else
			if (letGoUnder)
			{
				float y = GetTerrainY(segFromX,segFromZ);

				y -= gFenceSink[type];						// calc bottom of fence
				if ((gCoord.y + theNode->TopOff) <= y)		// see if top of the object is under it
					continue;
			}


					/* CALC NORMAL TO THE LINE */
					//
					// We need to find the point on the bounding sphere which is closest to the line
					// in order to do good collision checks
					//

			lineNormal.x = oldX - segFromX;						// calc normalized vector from ref pt. to section endpoint 0
			lineNormal.y = oldZ - segFromZ;
			OGLVector2D_Normalize(&lineNormal, &lineNormal);
			cross = OGLVector2D_Cross(&gFenceList[f].sectionVectors[i], &lineNormal);	// calc cross product to determine which side we're on

			if (cross < 0.0f)
			{
				lineNormal.x = -gFenceList[f].sectionNormals[i].x;		// on the other side, so flip vector
				lineNormal.y = -gFenceList[f].sectionNormals[i].y;
			}
			else
			{
				lineNormal = gFenceList[f].sectionNormals[i];			// use pre-calculated vector
			}


					/* CALC FROM-TO POINTS OF MOTION */

			fromX = oldX; // - (lineNormal.x * radius);
			fromZ = oldZ; // - (lineNormal.y * radius);
			toX = newX - (lineNormal.x * radius);
			toZ = newZ - (lineNormal.y * radius);

					/* SEE IF THE LINES INTERSECT */

			intersected = IntersectLineSegments(fromX,  fromZ, toX, toZ,
						                     segFromX, segFromZ, segToX, segToZ,
				                             &intersectX, &intersectZ);

			if (intersected)
			{
				hit = true;

						/***************************/
						/* HANDLE THE INTERSECTION */
						/***************************/
						//
						// Move so edge of sphere would be tangent, but also a bit
						// farther so it isnt tangent.
						//

				gCoord.x = intersectX + (lineNormal.x * radius) + (lineNormal.x * 8.0f);
				gCoord.z = intersectZ + (lineNormal.y * radius) + (lineNormal.y * 8.0f);


						/* BOUNCE OFF WALL */

				{
					OGLVector2D deltaV;

					deltaV.x = gDelta.x;
					deltaV.y = gDelta.z;
					ReflectVector2D(&deltaV, &lineNormal, &deltaV);
					gDelta.x = deltaV.x * .6f;
					gDelta.z = deltaV.y * .6f;
				}

						/* UPDATE COORD & SCAN AGAIN */

				newX = gCoord.x;
				newZ = gCoord.z;
				if (++numReScans < 4)
					i = -1;							// reset segment index to scan all again (will ++ to 0 on next loop)
				else
				{
					if (!letGoOver)					// we don't want to get stuck inside the fence (from having landed on it)
					{
						gCoord.x = oldX;				// woah!  there were a lot of hits, so let's just reset the coords to be safe!
						gCoord.z = oldZ;
					}
					break;
				}
			}

			/**********************************************/
			/* NO INTERSECT, DO SAFETY CHECK FOR /\ CASES */
			/**********************************************/
			//
			// The above check may fail when the sphere is going thru
			// the tip of a tee pee /\ intersection, so this is a hack
			// to get around it.
			//

			else
			{
					/* SEE IF EITHER ENDPOINT IS IN SPHERE */

				if ((CalcQuickDistance(segFromX, segFromZ, newX, newZ) <= radius) ||
					(CalcQuickDistance(segToX, segToZ, newX, newZ) <= radius))
				{
					OGLVector2D deltaV;

					hit = true;

					gCoord.x = oldX;
					gCoord.z = oldZ;

						/* BOUNCE OFF WALL */

					deltaV.x = gDelta.x;
					deltaV.y = gDelta.z;
					ReflectVector2D(&deltaV, &lineNormal, &deltaV);
					gDelta.x = deltaV.x * .5f;
					gDelta.z = deltaV.y * .5f;
					return(hit);
				}
				else
					continue;
			}
		} // for i
	}

	return(hit);
}

/******************** SEE IF LINE SEGMENT HITS FENCE **************************/
//
// returns True if hit a fence
//

Boolean SeeIfLineSegmentHitsFence(const OGLPoint3D *endPoint1, const OGLPoint3D *endPoint2, OGLPoint3D *intersect, Boolean *overTop, float *fenceTopY)
{
float			fromX,fromZ,toX,toZ;
long			f,numFenceSegments,i;
float			segFromX,segFromZ,segToX,segToZ;
OGLPoint3D		*nubs;
Boolean			intersected;


	fromX = endPoint1->x;
	fromZ = endPoint1->z;
	toX = endPoint2->x;
	toZ = endPoint2->z;

			/****************************************/
			/* SCAN THRU ALL FENCES FOR A COLLISION */
			/****************************************/

	for (f = 0; f < gNumFences; f++)
	{
		short	type;

				/* SEE IF IGNORE */

		type = gFenceList[f].type;
		switch(type)
		{
			case	FENCE_TYPE_DOGHAIR:						// ignore these
			case	FENCE_TYPE_CLOTH:
					continue;
		}


		/* QUICK CHECK TO SEE IF OLD & NEW COORDS (PLUS RADIUS) ARE OUTSIDE OF FENCE'S BBOX */

		if ((fromX < gFenceList[f].bBox.min.x) && (toX < gFenceList[f].bBox.min.x))
			continue;
		if ((fromX > gFenceList[f].bBox.max.x) && (toX > gFenceList[f].bBox.max.x))
			continue;

		if ((fromZ < gFenceList[f].bBox.min.z) && (toZ < gFenceList[f].bBox.min.z))
			continue;
		if ((fromZ > gFenceList[f].bBox.max.z) && (toZ > gFenceList[f].bBox.max.z))
			continue;

		nubs = gFenceList[f].nubList;				// point to nub list
		numFenceSegments = gFenceList[f].numNubs-1;	// get # line segments in fence


				/**********************************/
				/* SCAN EACH SECTION OF THE FENCE */
				/**********************************/

		for (i = 0; i < numFenceSegments; i++)
		{
			float	ix,iz;

					/* GET LINE SEG ENDPOINTS */

			segFromX = nubs[i].x;
			segFromZ = nubs[i].z;
			segToX = nubs[i+1].x;
			segToZ = nubs[i+1].z;


					/* SEE IF THE LINES INTERSECT */

			intersected = IntersectLineSegments(fromX,  fromZ, toX, toZ,
						                     segFromX, segFromZ, segToX, segToZ,
				                             &ix, &iz);

			if (intersected)
			{
				float	fenceTop,dy,d1,d2,ratio,iy;


						/* SEE IF INTERSECT OCCURS OVER THE TOP OF THE FENCE */

				if (overTop || intersect || fenceTopY)
				{
					fenceTop = GetTerrainY(ix, iz) + gFenceHeight[gFenceList[f].type];		// calc y coord @ top of fence here

					dy = endPoint2->y - endPoint1->y;					// get dy of line segment

					d1 = CalcDistance(fromX, fromZ, toX, toZ);
					d2 = CalcDistance(fromX, fromZ, ix, iz);

					ratio = d2/d1;

					iy = endPoint1->y + (dy * ratio);					// calc intersect y coord

					if (overTop)
					{
						if (iy >= fenceTop)
							*overTop = true;
						else
							*overTop = false;
					}

					if (intersect)
					{
						intersect->x = ix;						// pass back intersect coords
						intersect->y = iy;
						intersect->z = iz;
					}

					if (fenceTopY)
						*fenceTopY = fenceTop;
				}

				return(true);
			}

		}
	}

	return(false);
}




