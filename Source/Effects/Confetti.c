/****************************/
/*   	CONFETTI.C		    */
/* By Brian Greenstone      */
/* (c)2002 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveConfettiGroups(ObjNode *theNode);
static void DrawConfettiGroups(ObjNode *theNode);

/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

static Pool					*gConfettiGroupPool = NULL;
static ConfettiGroupType	gConfettiGroups[MAX_CONFETTI_GROUPS];

NewConfettiGroupDefType	gNewConfettiGroupDef;



/************************ INIT CONFETTI MANAGER **************************/
//
// NOTE:  This uses the sprites in the particle.sprites file which is loaded
//		in particles.c
//

void InitConfettiManager(void)
{
	GAME_ASSERT(!gConfettiGroupPool);

	gConfettiGroupPool = Pool_New(MAX_CONFETTI_GROUPS);

			/* INIT GROUP ARRAY */

	for (int g = 0; g < MAX_CONFETTI_GROUPS; g++)
	{
		gConfettiGroups[g].pool = Pool_New(MAX_CONFETTIS);

			/*****************************/
			/* INIT THE GROUP'S GEOMETRY */
			/*****************************/

			/* SET THE DATA */

		int maxPoints = MAX_CONFETTIS * 4 * 2;
		int maxFaces = MAX_CONFETTIS * 2 * 2;

		MOVertexArrayData vertexArrayData =
		{
			.numMaterials 	= 0,
			.numPoints		= 0,
			.numTriangles	= 0,
			.points 		= (OGLPoint3D *) AllocPtrClear(sizeof(OGLPoint3D) * maxPoints),
			.normals 		= (OGLVector3D *) AllocPtrClear(sizeof(OGLVector3D) * maxPoints),
			.uvs[0]	 		= (OGLTextureCoord *) AllocPtrClear(sizeof(OGLTextureCoord) * maxPoints),
			.colorsByte 	= (OGLColorRGBA_Byte *) AllocPtrClear(sizeof(OGLColorRGBA_Byte) * maxPoints),
			.triangles		= (MOTriangleIndecies *) AllocPtrClear(sizeof(MOTriangleIndecies) * maxFaces),
		};

			/* INIT UV ARRAYS */

		for (int p = 0; p < maxPoints; )
		{
			// frontface
			vertexArrayData.uvs[0][p++] = (OGLTextureCoord) {0,1};			// upper left
			vertexArrayData.uvs[0][p++] = (OGLTextureCoord) {0,0};			// lower left
			vertexArrayData.uvs[0][p++] = (OGLTextureCoord) {1,0};			// lower right
			vertexArrayData.uvs[0][p++] = (OGLTextureCoord) {1,1};			// upper right

			// backface
			vertexArrayData.uvs[0][p++] = (OGLTextureCoord) {0,1};			// upper left
			vertexArrayData.uvs[0][p++] = (OGLTextureCoord) {0,0};			// lower left
			vertexArrayData.uvs[0][p++] = (OGLTextureCoord) {1,0};			// lower right
			vertexArrayData.uvs[0][p++] = (OGLTextureCoord) {1,1};			// upper right

			GAME_ASSERT(p <= maxPoints);
		}

			/* INIT TRIANGLE ARRAYS */

		for (int t = 0, p = 0; t < maxFaces; )
		{
			// frontface
			vertexArrayData.triangles[t].vertexIndices[0] = p+0;			// triangle A
			vertexArrayData.triangles[t].vertexIndices[1] = p+1;
			vertexArrayData.triangles[t].vertexIndices[2] = p+2;
			t++;

			vertexArrayData.triangles[t].vertexIndices[0] = p+0;			// triangle B
			vertexArrayData.triangles[t].vertexIndices[1] = p+2;
			vertexArrayData.triangles[t].vertexIndices[2] = p+3;
			t++;

			// backface
			vertexArrayData.triangles[t].vertexIndices[0] = p+4;			// backface triangle A
			vertexArrayData.triangles[t].vertexIndices[1] = p+5;
			vertexArrayData.triangles[t].vertexIndices[2] = p+6;
			t++;

			vertexArrayData.triangles[t].vertexIndices[0] = p+4;			// backface triangle B
			vertexArrayData.triangles[t].vertexIndices[1] = p+6;
			vertexArrayData.triangles[t].vertexIndices[2] = p+7;
			t++;

			p += 8;
			GAME_ASSERT(t <= maxFaces);
			GAME_ASSERT(p <= maxPoints);
		}

			/* CREATE NEW GEOMETRY OBJECT */

		GAME_ASSERT(gConfettiGroups[g].geometryObj == NULL);

		gConfettiGroups[g].geometryObj = MO_CreateNewObjectOfType(MO_TYPE_VERTEXARRAY, &vertexArrayData);
	}


		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE CONFETTI DRAWING AT THE DESIRED TIME */
		/*************************************************************************/
		//
		// The confettis need to be drawn after the fences object, but before any sprite or font objects.
		//

	MakeNewDriverObject(CONFETTI_SLOT, DrawConfettiGroups, MoveConfettiGroups);
}


/******************** DELETE ALL CONFETTI GROUPS *********************/

void DisposeConfettiManager(void)
{
	for (int g = 0; g < MAX_CONFETTI_GROUPS; g++)
	{
		ConfettiGroupType* confettiGroup = &gConfettiGroups[g];

		if (confettiGroup->geometryObj)
		{
			// We didn't ref count the materials, so prevent MetaObjects from trying to free a dangling pointer
			confettiGroup->geometryObj->objectData.numMaterials = 0;

			MO_DisposeObjectReference(confettiGroup->geometryObj);
			confettiGroup->geometryObj = NULL;
		}

		if (confettiGroup->pool)
		{
			Pool_Free(confettiGroup->pool);
			confettiGroup->pool = NULL;
		}
	}

	if (gConfettiGroupPool)
	{
		Pool_Free(gConfettiGroupPool);
		gConfettiGroupPool = NULL;
	}
}


#pragma mark -


/********************** NEW CONFETTI GROUP *************************/
//
// INPUT:	type 	=	group type to create
//
// OUTPUT:	group ID#
//

int NewConfettiGroup(const NewConfettiGroupDefType *def)
{
			/*************************/
			/* SCAN FOR A FREE GROUP */
			/*************************/

	int g = Pool_AllocateIndex(gConfettiGroupPool);
	if (g < 0)		// nothing free
		return g;

	GAME_ASSERT(g < MAX_CONFETTI_GROUPS);
	ConfettiGroupType* confettiGroup = &gConfettiGroups[g];


				/* INITIALIZE THE GROUP */

	Pool_Reset(confettiGroup->pool);			// mark all confettis unused

	confettiGroup->flags				= def->flags;
	confettiGroup->gravity				= def->gravity;
	confettiGroup->baseScale			= def->baseScale;
	confettiGroup->decayRate			= def->decayRate;
	confettiGroup->fadeRate				= def->fadeRate;
	confettiGroup->magicNum				= def->magicNum;
	confettiGroup->confettiTextureNum	= def->confettiTextureNum;



				/*****************************/
				/* INIT THE GROUP'S GEOMETRY */
				/*****************************/
				// Note: most everything was pre-initialized in InitConfettiGroups

	MOVertexArrayData* vertexArrayData = &confettiGroup->geometryObj->objectData;

	vertexArrayData->numPoints = 0;		// no quads until we call AddConfettiToGroup
	vertexArrayData->numTriangles = 0;

	vertexArrayData->numMaterials = 1;
	vertexArrayData->materials[0] = gSpriteGroupList[SPRITE_GROUP_PARTICLES][def->confettiTextureNum].materialObject;	// NOTE: not refcounted

	return g;
}


/******************** ADD CONFETTI TO GROUP **********************/
//
// Returns true if confetti group was invalid or is full.
//

Boolean AddConfettiToGroup(NewConfettiDefType *def)
{
	short group = def->groupNum;

	if (!Pool_IsUsed(gConfettiGroupPool, group))
	{
		return(true);
	}

	ConfettiGroupType* confettiGroup = &gConfettiGroups[group];

			/* SCAN FOR FREE SLOT */

	int p = Pool_AllocateIndex(confettiGroup->pool);

			/* NO FREE SLOTS */

	if (p < 0)
		return true;


			/* INIT PARAMETERS */

	confettiGroup->fadeDelay[p]	= def->fadeDelay;
	confettiGroup->alpha[p]		= def->alpha;
	confettiGroup->scale[p]		= def->scale;
	confettiGroup->coord[p]		= *def->where;
	confettiGroup->delta[p]		= *def->delta;
	confettiGroup->rot[p]		= def->rot;
	confettiGroup->deltaRot[p]	= def->deltaRot;

	return(false);
}


/****************** MOVE CONFETTI GROUPS *********************/

static void MoveConfettiGroups(ObjNode *theNode)
{
uint32_t	flags;
float		fps = gFramesPerSecondFrac;
float		y,gravity;
float		decayRate,fadeRate;
OGLPoint3D	*coord;
OGLVector3D	*delta;

	for (int g = Pool_First(gConfettiGroupPool), nextG = -1;
		 g >= 0;
		 g = nextG)
	{
		GAME_ASSERT(Pool_IsUsed(gConfettiGroupPool, g));

		nextG = Pool_Next(gConfettiGroupPool, g);				// get next index now so we can release the current one in this loop

		ConfettiGroupType* confettiGroup = &gConfettiGroups[g];
		Pool* confettiPool = confettiGroup->pool;

//		baseScale 	= confettiGroup->baseScale;					// get base scale
//		oneOverBaseScaleSquared = 1.0f/(baseScale*baseScale);
		gravity 	= confettiGroup->gravity;
		decayRate 	= confettiGroup->decayRate;
		fadeRate 	= confettiGroup->fadeRate;
		flags 		= confettiGroup->flags;

		for (int p = Pool_First(confettiPool), nextP = -1;
			 p >= 0;
			 p = nextP)
		{
			GAME_DEBUGASSERT(Pool_IsUsed(confettiPool, p));

			nextP = Pool_Next(confettiPool, p);							// get next index now so we can release the current one in this loop

			delta = &confettiGroup->delta[p];							// get ptr to deltas
			coord = &confettiGroup->coord[p];							// get ptr to coords

						/* ADD GRAVITY */

			delta->y -= gravity * fps;									// add gravity

					/* DO ROTATION & MOTION */

			confettiGroup->rot[p].x += confettiGroup->deltaRot[p].x * fps;
			confettiGroup->rot[p].y += confettiGroup->deltaRot[p].y * fps;
			confettiGroup->rot[p].z += confettiGroup->deltaRot[p].z * fps;

			coord->x += delta->x * fps;									// move it
			coord->y += delta->y * fps;
			coord->z += delta->z * fps;


			/*****************/
			/* SEE IF BOUNCE */
			/*****************/

			if (!(flags & PARTICLE_FLAGS_DONTCHECKGROUND))
			{
				y = GetTerrainY(coord->x, coord->z);						// get terrain coord at confetti x/z
				if (y == ILLEGAL_TERRAIN_Y)									// bounce for Win screen
					y = 0.0f;
				y += 10.0f;

				if (flags & PARTICLE_FLAGS_BOUNCE)
				{
					if (delta->y < 0.0f && coord->y < y)					// if moving down, see if hit floor
					{
						coord->y = y;
						delta->y *= -.4f;

						delta->x += gRecentTerrainNormal.x * 300.0f;	// reflect off of surface
						delta->z += gRecentTerrainNormal.z * 300.0f;

						if (flags & PARTICLE_FLAGS_DISPERSEIFBOUNCE)	// see if disperse on impact
						{
							delta->y *= .4f;
							delta->x *= 5.0f;
							delta->z *= 5.0f;
						}
					}
				}

				/***************/
				/* SEE IF GONE */
				/***************/

				else if (coord->y < y)									// if hit floor then nuke confetti
				{
					goto deleteConfetti;
				}
			}


				/* DO SCALE  */

			confettiGroup->scale[p] -= decayRate * fps;			// shrink it
			if (confettiGroup->scale[p] <= 0.0f)					// see if gone
				goto deleteConfetti;

				/* DO FADE */

			confettiGroup->fadeDelay[p] -= fps;
			if (confettiGroup->fadeDelay[p] <= 0.0f)
			{
				confettiGroup->alpha[p] -= fadeRate * fps;				// fade it
				if (confettiGroup->alpha[p] <= 0.0f)					// see if gone
					goto deleteConfetti;
			}


				/* IF GONE THEN RELEASE INDEX */

			continue;

deleteConfetti:
			Pool_ReleaseIndex(confettiPool, p);
		}

			/* SEE IF GROUP HAS BECOME EMPTY, THEN DELETE */

		if (Pool_Empty(confettiPool))
		{
			Pool_ReleaseIndex(gConfettiGroupPool, g);
		}
	}


		/* SKIP DRAW CALL IF NO CONFETTI GROUPS ACTIVE */

	if (Pool_Empty(gConfettiGroupPool))
	{
		theNode->StatusBits |= STATUS_BIT_HIDDEN;
	}
	else
	{
		theNode->StatusBits &= ~STATUS_BIT_HIDDEN;
	}
}


/**************** DRAW CONFETTI GROUPS *********************/

static void DrawConfettiGroups(ObjNode *theNode)
{
float				baseScale;
OGLColorRGBA_Byte	*vertexColors;
MOVertexArrayData	*geoData;
OGLBoundingBox	bbox;

	(void) theNode;

				/* EARLY OUT IF NO GROUPS ACTIVE */

	if (Pool_Empty(gConfettiGroupPool))
		return;


				/* SETUP ENVIRONTMENT */

	OGL_PushState();

	SetColor4f(1,1,1,1);										// full white & alpha to start with

	for (int g = Pool_First(gConfettiGroupPool); g >= 0; g = Pool_Next(gConfettiGroupPool, g))
	{
		GAME_ASSERT(Pool_IsUsed(gConfettiGroupPool, g));

		float	minX,minY,minZ,maxX,maxY,maxZ;

		ConfettiGroupType* confettiGroup = &gConfettiGroups[g];

		Pool* confettiPool = confettiGroup->pool;
		geoData 		= &confettiGroup->geometryObj->objectData;		// get pointer to geometry object data
		vertexColors 	= geoData->colorsByte;							// get pointer to vertex color array
		baseScale 		= confettiGroup->baseScale;						// get base scale

				/********************************/
				/* ADD ALL CONFETTIS TO TRIMESH */
				/********************************/

		minX = minY = minZ = 100000000;									// init bbox
		maxX = maxY = maxZ = -minX;

		GAME_DEBUGASSERT_MESSAGE(!Pool_Empty(confettiPool), "empty confetti pool should have been purged in MoveConfettiGroups");

		int n = 0;
		for (int p = Pool_First(confettiPool); p >= 0; p = Pool_Next(confettiPool, p))
		{
			GAME_DEBUGASSERT(Pool_IsUsed(confettiGroup->pool, p));

			OGLMatrix4x4	m;

						/* SET VERTEX COORDS */

			float s = confettiGroup->scale[p] * baseScale;

			OGLPoint3D vFront[4] =
			{
				{ s,  s, 0},
				{ s, -s, 0},
				{-s, -s, 0},
				{-s,  s, 0},
			};

			OGLPoint3D vBack[4] =
			{
				{-s,  s, 0},
				{-s, -s, 0},
				{ s, -s, 0},
				{ s,  s, 0},
			};

			int pointOffset = n*8;
			int triOffset = n*4;

				/* TRANSFORM THIS CONFETTI'S VERTICES & ADD TO TRIMESH */

			OGLMatrix4x4_SetRotate_XYZ(&m, confettiGroup->rot[p].x, confettiGroup->rot[p].y, confettiGroup->rot[p].z);
			m.value[M03] = confettiGroup->coord[p].x;								// set translate
			m.value[M13] = confettiGroup->coord[p].y;
			m.value[M23] = confettiGroup->coord[p].z;
			OGLPoint3D_TransformArray(vFront, &m, &geoData->points[pointOffset], 4);				// transform
			OGLPoint3D_TransformArray(vBack, &m, &geoData->points[pointOffset+4], 4);				// transform

			for (int t = triOffset+0; t < triOffset+4; t++)
			{
				int v0 = (int) geoData->triangles[t].vertexIndices[0];
				int v1 = (int) geoData->triangles[t].vertexIndices[1];
				int v2 = (int) geoData->triangles[t].vertexIndices[2];
				OGLVector3D normal;
				CalcFaceNormal(&geoData->points[v0], &geoData->points[v1], &geoData->points[v2], &normal);
				geoData->normals[v0] = normal;
				geoData->normals[v1] = normal;
				geoData->normals[v2] = normal;
				GAME_ASSERT(v0 >= pointOffset && v0 < pointOffset+8);
				GAME_ASSERT(v1 >= pointOffset && v1 < pointOffset+8);
				GAME_ASSERT(v2 >= pointOffset && v2 < pointOffset+8);
			}

						/* UPDATE BBOX */

			for (int i = 0; i < 4; i++)		// only look at 4 first vertices in confetti (i.e. front face)
			{
				int j = pointOffset+i;

				if (geoData->points[j].x < minX) minX = geoData->points[j].x;
				if (geoData->points[j].x > maxX) maxX = geoData->points[j].x;
				if (geoData->points[j].y < minY) minY = geoData->points[j].y;
				if (geoData->points[j].y > maxY) maxY = geoData->points[j].y;
				if (geoData->points[j].z < minZ) minZ = geoData->points[j].z;
				if (geoData->points[j].z > maxZ) maxZ = geoData->points[j].z;
			}

				/* UPDATE COLOR/TRANSPARENCY */

			for (int i = pointOffset; i < pointOffset+8; i++)
			{
				vertexColors[i].r =
				vertexColors[i].g =
				vertexColors[i].b = 0xff;
				vertexColors[i].a = confettiGroup->alpha[p] * 255.0f;		// set transparency alpha
			}

			n++;											// inc confetti count
		}

			/* UPDATE FINAL VALUES */

		geoData->numTriangles = n*4;
		geoData->numPoints = n*8;

		if (geoData->numPoints < 20)						// if small then just skip cull test
			goto drawme;

		bbox.min.x = minX;									// build bbox for culling test
		bbox.min.y = minY;
		bbox.min.z = minZ;
		bbox.max.x = maxX;
		bbox.max.y = maxY;
		bbox.max.z = maxZ;

		if (OGL_IsBBoxVisible(&bbox, nil))						// do cull test on it
		{
drawme:
				/* DRAW IT */

			MO_DrawObject(confettiGroup->geometryObj);			// draw geometry
		}
	}

			/* RESTORE MODES */

	OGL_PopState();
}


/**************** VERIFY CONFETTI GROUP MAGIC NUM ******************/

Boolean VerifyConfettiGroupMagicNum(int group, uint32_t magicNum)
{
	if (!Pool_IsUsed(gConfettiGroupPool, group))
		return(false);

	if (gConfettiGroups[group].magicNum != magicNum)
		return(false);

	return(true);
}


#pragma mark -

/********************* MAKE CONFETTI EXPLOSION ***********************/

void MakeConfettiExplosion(float x, float y, float z, float force, float scale, short texture, short quantity)
{
OGLVector3D				delta,v;
OGLPoint3D				pt;
NewConfettiDefType		newConfettiDef;
float					radius = 40.0f * scale;

	gNewConfettiGroupDef.magicNum				= 0;
	gNewConfettiGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewConfettiGroupDef.gravity				= 250;
	gNewConfettiGroupDef.baseScale				= 4.5f * scale;
	gNewConfettiGroupDef.decayRate				=  -.5;
	gNewConfettiGroupDef.fadeRate				= 1.5;
	gNewConfettiGroupDef.confettiTextureNum		= texture;

	int pg = NewConfettiGroup(&gNewConfettiGroupDef);
	if (pg < 0)			// out of memory
		return;

	for (int i = 0; i < quantity; i++)
	{
		pt.x = x + RandomFloat2() * radius;
		pt.y = y + RandomFloat2() * radius;
		pt.z = z + RandomFloat2() * radius;

		v.x = pt.x - x;
		v.y = pt.y - y;
		v.z = pt.z - z;
		FastNormalizeVector(v.x,v.y,v.z,&v);

		delta.x = v.x * (force * scale);
		delta.y = v.y * (force * scale);
		delta.z = v.z * (force * scale);

		newConfettiDef.groupNum		= pg;
		newConfettiDef.where		= &pt;
		newConfettiDef.delta		= &delta;
		newConfettiDef.scale		= 1.0f + RandomFloat()  * .5f;
		newConfettiDef.rot.x		= RandomFloat()*PI2;
		newConfettiDef.rot.y		= RandomFloat()*PI2;
		newConfettiDef.rot.z		= RandomFloat()*PI2;
		newConfettiDef.deltaRot.x	= RandomFloat2()*5.0f;
		newConfettiDef.deltaRot.y	= RandomFloat2()*5.0f;
		newConfettiDef.deltaRot.z	= RandomFloat2()*15.0f;
		newConfettiDef.alpha		= FULL_ALPHA;
		newConfettiDef.fadeDelay	= .5f + RandomFloat();
		if (AddConfettiToGroup(&newConfettiDef))
			break;
	}
}
