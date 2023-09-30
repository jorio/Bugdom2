/****************************/
/*   	PARTICLES.C		    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveParticleGroups(ObjNode *theNode);

static void DrawParticleGroup(ObjNode *theNode);
static void MoveBlobDroplet(ObjNode *theNode);



static void MoveSmoker(ObjNode *theNode);
static void MoveBubbler(ObjNode *theNode);

/****************************/
/*    CONSTANTS             */
/****************************/

#define	FIRE_BLAST_RADIUS			(gTerrainPolygonSize * 1.5f)

#define	FIRE_TIMER	.05f
#define	SMOKE_TIMER	.07f


/*********************/
/*    VARIABLES      */
/*********************/

static Pool					*gParticleGroupPool = NULL;
static ParticleGroupType	gParticleGroups[MAX_PARTICLE_GROUPS];

static float	gGravitoidDistBuffer[MAX_PARTICLES][MAX_PARTICLES];

NewParticleGroupDefType	gNewParticleGroupDef;


#define	RippleTimer	SpecialF[0]


/************************* INIT EFFECTS ***************************/

void InitEffects(void)
{
	InitParticleSystem();
	InitSparkles();
	InitConfettiManager();
	InitShardSystem();
	InitRainEffect();
}

/************************* DISPOSE EFFECTS ***************************/

void DisposeEffects(void)
{
	DisposeParticleSystem();
	DisposeConfettiManager();
	DisposeShardSystem();
	DisposeSparkles();
}


#pragma mark -

/************************ INIT PARTICLE SYSTEM **************************/

void InitParticleSystem(void)
{
	GAME_ASSERT(!gParticleGroupPool);

	gParticleGroupPool = Pool_New(MAX_PARTICLE_GROUPS);


			/* INIT GROUP ARRAY */

	for (int g = 0; g < MAX_PARTICLE_GROUPS; g++)
	{
			/* ALLOCATE NEW GROUP */

		gParticleGroups[g].pool = Pool_New(MAX_PARTICLES);

			/*****************************/
			/* INIT THE GROUP'S GEOMETRY */
			/*****************************/

				/* SET THE DATA */

		MOVertexArrayData vertexArrayData =
		{
			.numMaterials 	= 0,
			.numPoints 		= 0,
			.numTriangles 	= 0,
			.points 		= (OGLPoint3D *)AllocPtr(sizeof(OGLPoint3D) * MAX_PARTICLES * 4),
			.normals 		= nil,
			.uvs[0]	 		= (OGLTextureCoord *)AllocPtr(sizeof(OGLTextureCoord) * MAX_PARTICLES * 4),
			.colorsByte 	= (OGLColorRGBA_Byte *)AllocPtr(sizeof(OGLColorRGBA_Byte) * MAX_PARTICLES * 4),
			.colorsFloat	= nil,
			.triangles		= (MOTriangleIndecies *)AllocPtr(sizeof(MOTriangleIndecies) * MAX_PARTICLES * 2),
		};


				/* INIT UV ARRAYS */

		for (int j = 0; j < (MAX_PARTICLES*4); j+=4)
		{
			vertexArrayData.uvs[0][j+0] = (OGLTextureCoord) {0,1};			// upper left
			vertexArrayData.uvs[0][j+1] = (OGLTextureCoord) {0,0};			// lower left
			vertexArrayData.uvs[0][j+2] = (OGLTextureCoord) {1,0};			// lower right
			vertexArrayData.uvs[0][j+3] = (OGLTextureCoord) {1,1};			// upper right
		}

				/* INIT TRIANGLE ARRAYS */

		for (int j = 0, k = 0; j < (MAX_PARTICLES*2); j+=2, k+=4)
		{
			vertexArrayData.triangles[j].vertexIndices[0] = k;				// triangle A
			vertexArrayData.triangles[j].vertexIndices[1] = k+1;
			vertexArrayData.triangles[j].vertexIndices[2] = k+2;

			vertexArrayData.triangles[j+1].vertexIndices[0] = k;			// triangle B
			vertexArrayData.triangles[j+1].vertexIndices[1] = k+2;
			vertexArrayData.triangles[j+1].vertexIndices[2] = k+3;
		}


			/* CREATE NEW GEOMETRY OBJECT */

		GAME_ASSERT(gParticleGroups[g].geometryObj == NULL);

		gParticleGroups[g].geometryObj = MO_CreateNewObjectOfType(MO_TYPE_GEOMETRY, MO_GEOMETRY_SUBTYPE_VERTEXARRAY, &vertexArrayData);
	}


		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE PARTICLE DRAWING AT THE DESIRED TIME */
		/*************************************************************************/
		//
		// The particles need to be drawn after the fences object, but before any sprite or font objects.
		//

	ObjNode* driver = MakeNewDriverObject(PARTICLE_SLOT, DrawParticleGroup, MoveParticleGroups);
	driver->StatusBits |= STATUS_BIT_DOUBLESIDED|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES|STATUS_BIT_NOFOG;
}


/******************** DISPOSE PARTICLE SYSTEM **********************/

void DisposeParticleSystem(void)
{
	for (int g = 0; g < MAX_PARTICLE_GROUPS; g++)
	{
		ParticleGroupType* particleGroup = &gParticleGroups[g];

		// We didn't ref count the materials, so prevent MetaObjects from trying to free a dangling pointer
		particleGroup->geometryObj->objectData.numMaterials = 0;

		MO_DisposeObjectReference(particleGroup->geometryObj);
		particleGroup->geometryObj = NULL;

		Pool_Free(particleGroup->pool);
		particleGroup->pool = NULL;
	}

	Pool_Free(gParticleGroupPool);
	gParticleGroupPool = NULL;
}


#pragma mark -


/********************** NEW PARTICLE GROUP *************************/
//
// INPUT:	type 	=	group type to create
//
// OUTPUT:	group ID#
//

short NewParticleGroup(NewParticleGroupDefType *def)
{
			/*************************/
			/* SCAN FOR A FREE GROUP */
			/*************************/


	int g = Pool_AllocateIndex(gParticleGroupPool);
	if (g < 0)		// nothing free
		return g;

	GAME_ASSERT(g < MAX_PARTICLE_GROUPS);
	ParticleGroupType* particleGroup = &gParticleGroups[g];


		/* INITIALIZE THE GROUP */


	Pool_Reset(particleGroup->pool);						// mark all unused

	particleGroup->type					= def->type;
	particleGroup->flags 				= def->flags;
	particleGroup->gravity 				= def->gravity;
	particleGroup->magnetism 			= def->magnetism;
	particleGroup->baseScale 			= def->baseScale;
	particleGroup->decayRate 			= def->decayRate;
	particleGroup->fadeRate 			= def->fadeRate;
	particleGroup->magicNum 			= def->magicNum;
	particleGroup->particleTextureNum	= def->particleTextureNum;
	particleGroup->srcBlend 			= def->srcBlend;
	particleGroup->dstBlend 			= def->dstBlend;

		/*****************************/
		/* INIT THE GROUP'S GEOMETRY */
		/*****************************/
		// Note: most everything was pre-initialized in InitParticleGroups

	MOVertexArrayData* vertexArrayData = &particleGroup->geometryObj->objectData;

			/* SET THE DATA */

	vertexArrayData->numPoints = 0;		// no quads until we call AddParticleToGroup
	vertexArrayData->numTriangles = 0;

	vertexArrayData->numMaterials = 1;
	vertexArrayData->materials[0] = gSpriteGroupList[SPRITE_GROUP_PARTICLES][def->particleTextureNum].materialObject;	// NOTE: not refcounted

	return g;
}


/******************** ADD PARTICLE TO GROUP **********************/
//
// Returns true if particle group was invalid or is full.
//

Boolean AddParticleToGroup(NewParticleDefType *def)
{
	short group = def->groupNum;

	if (!Pool_IsUsed(gParticleGroupPool, group))
	{
		return(true);
	}

	ParticleGroupType* particleGroup = &gParticleGroups[group];

			/* SCAN FOR FREE SLOT */

	int p = Pool_AllocateIndex(particleGroup->pool);

			/* NO FREE SLOTS */

	if (p < 0)
		return true;

			/* INIT PARAMETERS */

	particleGroup->alpha[p] = def->alpha;
	particleGroup->scale[p] = def->scale;
	particleGroup->coord[p] = *def->where;
	particleGroup->delta[p] = *def->delta;
	particleGroup->rotZ[p] = def->rotZ;
	particleGroup->rotDZ[p] = def->rotDZ;

	return(false);
}


/****************** MOVE PARTICLE GROUPS *********************/

static void MoveParticleGroups(ObjNode *theNode)
{
uint32_t	flags;
float		fps = gFramesPerSecondFrac;
float		y,baseScale,oneOverBaseScaleSquared,gravity;
float		decayRate,magnetism,fadeRate;
OGLPoint3D	*coord;
OGLVector3D	*delta;

	(void) theNode;

	for (int g = Pool_First(gParticleGroupPool), nextG = -1;
		 g >= 0;
		 g = nextG)
	{
		GAME_ASSERT(Pool_IsUsed(gParticleGroupPool, g));

		nextG = Pool_Next(gParticleGroupPool, g);				// get next index now so we can release the current one in this loop

		ParticleGroupType* particleGroup = &gParticleGroups[g];
		Pool* particlePool = particleGroup->pool;

		baseScale 	= particleGroup->baseScale;					// get base scale
		oneOverBaseScaleSquared = 1.0f/(baseScale*baseScale);
		gravity 	= particleGroup->gravity;					// get gravity
		decayRate 	= particleGroup->decayRate;					// get decay rate
		fadeRate 	= particleGroup->fadeRate;					// get fade rate
		magnetism 	= particleGroup->magnetism;					// get magnetism
		flags 		= particleGroup->flags;

		for (int p = Pool_First(particlePool), nextP = -1;
			 p >= 0;
			 p = nextP)
		{
			GAME_DEBUGASSERT(Pool_IsUsed(particlePool, p));

			nextP = Pool_Next(particlePool, p);							// get next index now so we can release the current one in this loop

			delta = &particleGroup->delta[p];							// get ptr to deltas
			coord = &particleGroup->coord[p];							// get ptr to coords

						/* ADD GRAVITY */

			delta->y -= gravity * fps;									// add gravity


					/* DO ROTATION */

			particleGroup->rotZ[p] += particleGroup->rotDZ[p] * fps;



			switch (particleGroup->type)
			{
						/* FALLING SPARKS */

				case	PARTICLE_TYPE_FALLINGSPARKS:
						coord->x += delta->x * fps;						// move it
						coord->y += delta->y * fps;
						coord->z += delta->z * fps;
						break;


						/* GRAVITOIDS */
						//
						// Every particle has gravity pull on other particle
						//

				case	PARTICLE_TYPE_GRAVITOIDS:
						for (int q = Pool_Last(particlePool); q >= 0; q = Pool_Prev(particlePool, q))
						{
							GAME_DEBUGASSERT(Pool_IsUsed(particlePool, q));

							float		dist,x,z;
							OGLVector3D	v;

							if (p == q)									// dont check against self
								continue;

							x = particleGroup->coord[q].x;
							y = particleGroup->coord[q].y;
							z = particleGroup->coord[q].z;

									/* calc 1/(dist2) */

							if (p < q)									// see if calc or get from buffer
							{
								float dx = coord->x - x;
								float dy = coord->y - y;
								float dz = coord->z - z;

								dist = sqrtf(dx*dx + dy*dy + dz*dz);
								if (dist != 0.0f)
									dist = 1.0f / (dist*dist);

								if (dist > oneOverBaseScaleSquared)		// adjust if closer than radius
									dist = oneOverBaseScaleSquared;

								gGravitoidDistBuffer[p][q] = dist;		// remember it
							}
							else
							{
								dist = gGravitoidDistBuffer[q][p];		// use from buffer
							}

										/* calc vector to particle */

							if (dist != 0.0f)
							{
								x = x - coord->x;
								y = y - coord->y;
								z = z - coord->z;
								FastNormalizeVector(x, y, z, &v);
							}
							else
							{
								v.x = v.y = v.z = 0;
							}

							delta->x += v.x * (dist * magnetism * fps);		// apply gravity to particle
							delta->y += v.y * (dist * magnetism * fps);
							delta->z += v.z * (dist * magnetism * fps);
						}

						coord->x += delta->x * fps;						// move it
						coord->y += delta->y * fps;
						coord->z += delta->z * fps;
						break;
			}

				/********************/
				/* SEE IF HAS MAX Y */
				/********************/

			if (flags & PARTICLE_FLAGS_HASMAXY)
			{
				if (coord->y > particleGroup->maxY)
				{
					goto deleteParticle;
				}
			}


			/*****************/
			/* SEE IF BOUNCE */
			/*****************/

			if (!(flags & PARTICLE_FLAGS_DONTCHECKGROUND))
			{
				y = GetTerrainY(coord->x, coord->z)+10.0f;					// get terrain coord at particle x/z

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

				else
				{
					if (coord->y < y)									// if hit floor then nuke particle
					{
						goto deleteParticle;
					}
				}
			}


				/* DO SCALE */

			particleGroup->scale[p] -= decayRate * fps;					// shrink it
			if (particleGroup->scale[p] <= 0.0f)						// see if gone
				goto deleteParticle;

				/* DO FADE */

			particleGroup->alpha[p] -= fadeRate * fps;					// fade it
			if (particleGroup->alpha[p] <= 0.0f)						// see if gone
				goto deleteParticle;


			continue;
deleteParticle:
			Pool_ReleaseIndex(particlePool, p);
		}

			/* SEE IF GROUP HAS BECOME EMPTY, THEN DELETE */

		if (Pool_Empty(particlePool))
		{
			Pool_ReleaseIndex(gParticleGroupPool, g);
		}
	}
}


/**************** DRAW PARTICLE GROUPS *********************/

static void DrawParticleGroup(ObjNode *theNode)
{
float				scale,baseScale;
OGLColorRGBA_Byte	*vertexColors;
MOVertexArrayData	*geoData;
OGLPoint3D		v[4],*camCoords,*coord;
static const OGLVector3D up = {0,1,0};
OGLBoundingBox	bbox;

	(void) theNode;


				/* EARLY OUT IF NO GROUPS ACTIVE */

	if (Pool_Empty(gParticleGroupPool))
		return;


	v[0].z = 												// init z's to 0
	v[1].z =
	v[2].z =
	v[3].z = 0;

				/* SETUP ENVIRONTMENT */

	OGL_PushState();

	glEnable(GL_BLEND);
	SetColor4f(1,1,1,1);													// full white & alpha to start with

	camCoords = &gGameView->cameraPlacement.cameraLocation;

	for (int g = Pool_First(gParticleGroupPool); g >= 0; g = Pool_Next(gParticleGroupPool, g))
	{
		GAME_ASSERT(Pool_IsUsed(gParticleGroupPool, g));

		ParticleGroupType* particleGroup = &gParticleGroups[g];
		Pool* particlePool = particleGroup->pool;

		uint32_t	allAim = particleGroup->flags & PARTICLE_FLAGS_ALLAIM;

		geoData = &particleGroup->geometryObj->objectData;			// get pointer to geometry object data
		vertexColors = geoData->colorsByte;							// get pointer to vertex color array
		baseScale = particleGroup->baseScale;						// get base scale

				/********************************/
				/* ADD ALL PARTICLES TO TRIMESH */
				/********************************/

		float	minX,minY,minZ,maxX,maxY,maxZ;
		minX = minY = minZ = 100000000;									// init bbox
		maxX = maxY = maxZ = -minX;

		GAME_DEBUGASSERT_MESSAGE(!Pool_Empty(particlePool), "empty particle pool should have been purged in MoveParticleGroups");

		int n = 0;
		for (int p = Pool_First(particlePool); p >= 0; p = Pool_Next(particlePool, p))
		{
			GAME_DEBUGASSERT(Pool_IsUsed(particlePool, p));

			float			rot;
			OGLMatrix4x4	m;


						/* CREATE VERTEX DATA */

			scale = particleGroup->scale[p] * baseScale;

			v[0].x = -scale;
			v[0].y = scale;

			v[1].x = -scale;
			v[1].y = -scale;

			v[2].x = scale;
			v[2].y = -scale;

			v[3].x = scale;
			v[3].y = scale;


				/* TRANSFORM THIS PARTICLE'S VERTICES & ADD TO TRIMESH */

			coord = &particleGroup->coord[p];
			if ((n == 0) || allAim)										// only set the look-at matrix for the 1st particle unless we want to force it for all (optimization technique)
				SetLookAtMatrixAndTranslate(&m, &up, coord, camCoords);	// aim at camera & translate
			else
			{
				m.value[M03] = coord->x;								// update just the translate
				m.value[M13] = coord->y;
				m.value[M23] = coord->z;
			}

			rot = particleGroup->rotZ[p];								// get z rotation
			if (rot != 0.0f)											// see if need to apply rotation matrix
			{
				OGLMatrix4x4	rm;

				OGLMatrix4x4_SetRotate_Z(&rm, rot);
				OGLMatrix4x4_Multiply(&rm, &m, &rm);
				OGLPoint3D_TransformArray(&v[0], &rm, &geoData->points[n*4], 4);	// transform w/ rot
			}
			else
				OGLPoint3D_TransformArray(&v[0], &m, &geoData->points[n*4], 4);		// transform no-rot


						/* UPDATE BBOX */

			for (int i = 0; i < 4; i++)
			{
				int j = n*4+i;

				if (geoData->points[j].x < minX)	minX = geoData->points[j].x;
				if (geoData->points[j].x > maxX)	maxX = geoData->points[j].x;
				if (geoData->points[j].y < minY)	minY = geoData->points[j].y;
				if (geoData->points[j].y > maxY)	maxY = geoData->points[j].y;
				if (geoData->points[j].z < minZ)	minZ = geoData->points[j].z;
				if (geoData->points[j].z > maxZ)	maxZ = geoData->points[j].z;
			}

				/* UPDATE COLOR/TRANSPARENCY */

			int temp = n*4;
			for (int i = temp; i < (temp+4); i++)
			{
				vertexColors[i].r =
				vertexColors[i].g =
				vertexColors[i].b = 0xff;
				vertexColors[i].a = particleGroup->alpha[p] * 255.0f;		// set transparency alpha
			}

			n++;											// inc particle count
		}

		if (n == 0)											// if no particles, then skip
			continue;

			/* UPDATE FINAL VALUES */

		geoData->numTriangles = n*2;
		geoData->numPoints = n*4;

		bbox.min.x = minX;									// build bbox for culling test
		bbox.min.y = minY;
		bbox.min.z = minZ;
		bbox.max.x = maxX;
		bbox.max.y = maxY;
		bbox.max.z = maxZ;

		if (OGL_IsBBoxVisible(&bbox, nil))									// do cull test on it
		{
			GLint	src,dst;

			src = particleGroup->srcBlend;
			dst = particleGroup->dstBlend;

				/* DRAW IT */

			glBlendFunc(src, dst);							// set blending mode
			MO_DrawObject(particleGroup->geometryObj);		// draw geometry
		}
	}

			/* RESTORE MODES */

	OGL_PopState();
	SetColor4f(1,1,1,1);										// reset this

}


/**************** VERIFY PARTICLE GROUP MAGIC NUM ******************/

Boolean VerifyParticleGroupMagicNum(short group, long magicNum)
{
	if (!Pool_IsUsed(gParticleGroupPool, group))
		return(false);

	if (gParticleGroups[group].magicNum != magicNum)
		return(false);

	return(true);
}


/************* PARTICLE HIT OBJECT *******************/
//
// INPUT:	inFlags = flags to check particle types against
//

Boolean ParticleHitObject(ObjNode *theNode, uint16_t inFlags)
{
	for (int g = Pool_First(gParticleGroupPool); g >= 0; g = Pool_Next(gParticleGroupPool, g))
	{
		GAME_DEBUGASSERT(Pool_IsUsed(gParticleGroupPool, g));

		ParticleGroupType* particleGroup = &gParticleGroups[g];
		Pool* particlePool = particleGroup->pool;

		if (inFlags)												// see if check flags
		{
			if (!(inFlags & particleGroup->flags))
				continue;
		}

		for (int p = Pool_First(particlePool); p >= 0; p = Pool_Next(particlePool, p))
		{
			GAME_DEBUGASSERT(Pool_IsUsed(particlePool, p));

			if (particleGroup->alpha[p] < .4f)				// if particle is too decayed, then skip
				continue;

			const OGLPoint3D* coord = &particleGroup->coord[p];	// get ptr to coords
			if (DoSimpleBoxCollisionAgainstObject(coord->y+40.0f,coord->y-40.0f,
												coord->x-40.0f, coord->x+40.0f,
												coord->z+40.0f, coord->z-40.0f,
												theNode))
			{
				return(true);
			}
		}
	}
	return(false);
}

#pragma mark -

/********************* MAKE PUFF ***********************/

void MakePuff(OGLPoint3D *where, float scale, short texNum, GLint src, GLint dst, float decayRate)
{
long					pg,i;
OGLVector3D				delta;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;

			/* white sparks */

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_ALLAIM;
	gNewParticleGroupDef.gravity				= -80;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= scale;
	gNewParticleGroupDef.decayRate				=  -1.5;
	gNewParticleGroupDef.fadeRate				= decayRate;
	gNewParticleGroupDef.particleTextureNum		= texNum;
	gNewParticleGroupDef.srcBlend				= src;
	gNewParticleGroupDef.dstBlend				= dst;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		x = where->x;
		y = where->y;
		z = where->z;

		for (i = 0; i < 10; i++)
		{
			pt.x = x + RandomFloat2() * (2.0f * scale);
			pt.y = y + RandomFloat() * 2.0f * scale;
			pt.z = z + RandomFloat2() * (2.0f * scale);

			delta.x = RandomFloat2() * (3.0f * scale);
			delta.y = RandomFloat() * (2.0f  * scale);
			delta.z = RandomFloat2() * (3.0f * scale);


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0f + RandomFloat2() * .2f;
			newParticleDef.rotZ			= RandomFloat() * PI2;
			newParticleDef.rotDZ		= RandomFloat2() * 4.0f;
			newParticleDef.alpha		= FULL_ALPHA;
			AddParticleToGroup(&newParticleDef);
		}
	}
}


/********************* MAKE SPARK EXPLOSION ***********************/

void MakeSparkExplosion(float x, float y, float z, float force, float scale, short sparkTexture, short quantityLimit, float fadeRate)
{
long					pg,i,n;
OGLVector3D				delta,v;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;

	n = force * .3f;

	if (quantityLimit)
		if (n > quantityLimit)
			n = quantityLimit;


			/* white sparks */

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_ALLAIM;
	gNewParticleGroupDef.gravity				= 200;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 15.0f * scale;
	gNewParticleGroupDef.decayRate				=  0;
	gNewParticleGroupDef.fadeRate				= fadeRate;
	gNewParticleGroupDef.particleTextureNum		= sparkTexture;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < n; i++)
		{
			pt.x = x + RandomFloat2() * (30.0f * scale);
			pt.y = y + RandomFloat2() * (30.0f * scale);
			pt.z = z + RandomFloat2() * (30.0f * scale);

			v.x = pt.x - x;
			v.y = pt.y - y;
			v.z = pt.z - z;
			FastNormalizeVector(v.x,v.y,v.z,&v);

			delta.x = v.x * (force * scale);
			delta.y = v.y * (force * scale);
			delta.z = v.z * (force * scale);


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0f + RandomFloat()  * .5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA;
			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}
}

/****************** MAKE STEAM ************************/

void MakeSteam(ObjNode *blob, float x, float y, float z)
{
int		i;
float	fps = gFramesPerSecondFrac;
long	particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;

	if (gFramesPerSecond < 15.0f)
		return;

	blob->ParticleTimer -= fps;
	if (blob->ParticleTimer <= 0.0f)
	{
		blob->ParticleTimer += 0.02f;


		particleGroup 	= blob->ParticleGroup;
		magicNum 		= blob->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			blob->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_GRAVITOIDS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 90000;
			groupDef.baseScale				= 15;
			groupDef.decayRate				= -.5;
			groupDef.fadeRate				= .4;
			groupDef.particleTextureNum		= PARTICLE_SObjType_GreySmoke;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			blob->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * (20.0f);
				p.y = y + RandomFloat2() * (20.0f);
				p.z = z + RandomFloat2() * (20.0f);

				d.x = RandomFloat2() * 190.0f;
				d.y = 150.0f + RandomFloat() * 190.0f;
				d.z = RandomFloat2() * 190.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat()*PI2;
				newParticleDef.rotDZ		= 0;
				newParticleDef.alpha		= .5;
				if (AddParticleToGroup(&newParticleDef))
				{
					blob->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}


#pragma mark -

/********************* MAKE SPLASH ***********************/

void MakeSplash(float x, float y, float z, float scale)
{
long	pg,i;
OGLVector3D	delta;
OGLPoint3D	pt;
NewParticleDefType		newParticleDef;
float	volume;

	pt.y = y;

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_ALLAIM;
	gNewParticleGroupDef.gravity				= 400;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 15.0f * scale;
	gNewParticleGroupDef.decayRate				=  -.6;
	gNewParticleGroupDef.fadeRate				= .8;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Splash;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;


	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 30; i++)
		{
			pt.x = x + RandomFloat2() * (30.0f * scale);
			pt.z = z + RandomFloat2() * (30.0f * scale);

			delta.x = RandomFloat2() * (200.0f * scale);
			delta.y = 100.0f + RandomFloat() * (150.0f * scale);
			delta.z = RandomFloat2() * (200.0f * scale);

			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= RandomFloat2()*PI2;
			newParticleDef.alpha		= FULL_ALPHA;
			AddParticleToGroup(&newParticleDef);
		}
	}

			/* PLAY SPLASH SOUND */

	pt.x = x;
	pt.z = z;

	if (gLevelNum == LEVEL_NUM_BALSA)
		volume = 7.0;
	else
		volume = 1.5;

	PlayEffect_Parms3D(EFFECT_SPLASH, &pt, NORMAL_CHANNEL_RATE, volume);
}


/***************** SPRAY WATER *************************/

void SprayWater(ObjNode *theNode, float x, float y, float z)
{
float	fps = gFramesPerSecondFrac;
long	particleGroup, magicNum;

	theNode->ParticleTimer += fps;				// see if time to spew water
	if (theNode->ParticleTimer > 0.02f)
	{
		theNode->ParticleTimer += .02f;

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->SmokeParticleMagic = magicNum = MyRandomLong();			// generate a random magic num

			gNewParticleGroupDef.magicNum				= magicNum;
			gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			gNewParticleGroupDef.flags					= 0;
			gNewParticleGroupDef.gravity				= 1400;
			gNewParticleGroupDef.magnetism				= 0;
			gNewParticleGroupDef.baseScale				= 35;
			gNewParticleGroupDef.decayRate				=  -1.7f;
			gNewParticleGroupDef.fadeRate				= 1.0;
			gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Splash;
			gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
			gNewParticleGroupDef.dstBlend				= GL_ONE;
			theNode->SmokeParticleGroup = particleGroup = NewParticleGroup(&gNewParticleGroupDef);
		}


					/* ADD PARTICLE TO GROUP */

		if (particleGroup != -1)
		{
			OGLPoint3D	pt;
			OGLVector3D delta;
			NewParticleDefType		newParticleDef;

			FastNormalizeVector(gDelta.x, 0, gDelta.z, &delta);			// calc spray delta shooting out of butt
			delta.x *= -300.0f;
			delta.z *= -300.0f;

			delta.x += (RandomFloat()-.5f) * 50.0f;						// spray delta
			delta.z += (RandomFloat()-.5f) * 50.0f;
			delta.y = 500.0f + RandomFloat() * 100.0f;

			pt.x = x + (RandomFloat()-.5f) * 80.0f;			// random noise to coord
			pt.y = y;
			pt.z = z + (RandomFloat()-.5f) * 80.0f;


			newParticleDef.groupNum		= particleGroup;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA;
			AddParticleToGroup(&newParticleDef);
		}
	}
}





#pragma mark -



/****************** BURN FIRE ************************/

void BurnFire(ObjNode *theNode, float x, float y, float z, Boolean doSmoke,
			short particleType, float scale, uint32_t moreFlags)
{
float	fps = gFramesPerSecondFrac;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;


		/**************/
		/* MAKE SMOKE */
		/**************/

	if (doSmoke && (gFramesPerSecond > 20.0f))										// only do smoke if running at good frame rate
	{
		theNode->SmokeTimer -= fps;													// see if add smoke
		if (theNode->SmokeTimer <= 0.0f)
		{
			theNode->SmokeTimer += SMOKE_TIMER;										// reset timer

			long particleGroup 	= theNode->SmokeParticleGroup;
			long magicNum 		= theNode->SmokeParticleMagic;

			if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
			{

				theNode->SmokeParticleMagic = magicNum = MyRandomLong();			// generate a random magic num

				groupDef.magicNum				= magicNum;
				groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
				groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND|moreFlags;
				groupDef.gravity				= 0;
				groupDef.magnetism				= 0;
				groupDef.baseScale				= 20.0f * scale;
				groupDef.decayRate				=  -.2f;
				groupDef.fadeRate				= .2;
				groupDef.particleTextureNum		= PARTICLE_SObjType_BlackSmoke;
				groupDef.srcBlend				= GL_SRC_ALPHA;
				groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
				theNode->SmokeParticleGroup = particleGroup = NewParticleGroup(&groupDef);
			}

			if (particleGroup != -1)
			{
				for (int i = 0; i < 3; i++)
				{
					p.x = x + RandomFloat2() * (40.0f * scale);
					p.y = y + 200.0f + RandomFloat() * (50.0f * scale);
					p.z = z + RandomFloat2() * (40.0f * scale);

					d.x = RandomFloat2() * (20.0f * scale);
					d.y = 150.0f + RandomFloat() * (40.0f * scale);
					d.z = RandomFloat2() * (20.0f * scale);

					newParticleDef.groupNum		= particleGroup;
					newParticleDef.where		= &p;
					newParticleDef.delta		= &d;
					newParticleDef.scale		= RandomFloat() + 1.0f;
					newParticleDef.rotZ			= RandomFloat() * PI2;
					newParticleDef.rotDZ		= RandomFloat2();
					newParticleDef.alpha		= .7;
					if (AddParticleToGroup(&newParticleDef))
					{
						theNode->SmokeParticleGroup = -1;
						break;
					}
				}
			}
		}
	}

		/*************/
		/* MAKE FIRE */
		/*************/

	theNode->FireTimer -= fps;													// see if add fire
	if (theNode->FireTimer <= 0.0f)
	{
		theNode->FireTimer += FIRE_TIMER;										// reset timer

		long particleGroup 	= theNode->ParticleGroup;
		long magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND|moreFlags;
			groupDef.gravity				= -200;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 30.0f * scale;
			groupDef.decayRate				=  0;
			groupDef.fadeRate				= .8;
			groupDef.particleTextureNum		= particleType;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (int i = 0; i < 3; i++)
			{
				p.x = x + RandomFloat2() * (30.0f * scale);
				p.y = y + RandomFloat() * (50.0f * scale);
				p.z = z + RandomFloat2() * (30.0f * scale);

				d.x = RandomFloat2() * (50.0f * scale);
				d.y = 50.0f + RandomFloat() * (60.0f * scale);
				d.z = RandomFloat2() * (50.0f * scale);

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2();
				newParticleDef.alpha		= 1.0;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}






#pragma mark -

/************** MAKE FIRE EXPLOSION *********************/

void MakeFireExplosion(OGLPoint3D *where)
{
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;


		/*********************/
		/* FIRST MAKE FLAMES */
		/*********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
	gNewParticleGroupDef.gravity				= -120;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 18;
	gNewParticleGroupDef.decayRate				=  -.7;
	gNewParticleGroupDef.fadeRate				= 1.3;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Fire;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		x = where->x;
		y = where->y;
		z = where->z;


		for (i = 0; i < 50; i++)
		{
			pt.x = x + RandomFloat2() * 20.0f;
			pt.y = y + RandomFloat2() * 20.0f;
			pt.z = z + RandomFloat2() * 20.0f;

			d.y = RandomFloat2() * 100.0f;
			d.x = RandomFloat2() * 150.0f;
			d.z = RandomFloat2() * 150.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= RandomFloat2() * 10.0f;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}


}





#pragma mark -


/********************** MAKE SPLATTER ************************/

void MakeSplatter(OGLPoint3D *where, short modelObjType, float scale)
{
OGLVector3D	aim;
int		i;
ObjNode	*newObj;

	scale *= .6f;							// calc scale and see if too small
	if (scale < .1f)
		return;


	for (i = 0; i < 14; i++)
	{
				/* RANDOM AIM TO START WITH */

		aim.x = RandomFloat2();
		aim.y = RandomFloat() * 3.0f;
		aim.z = RandomFloat2();
		FastNormalizeVector(aim.x, aim.y, aim.z, &aim);

		gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
		gNewObjectDefinition.type 		= modelObjType;
		gNewObjectDefinition.coord.x	= where->x + aim.x * 25.0f;
		gNewObjectDefinition.coord.y	= where->y + aim.y * 25.0f;
		gNewObjectDefinition.coord.z	= where->z + aim.z * 25.0f;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_USEALIGNMENTMATRIX;
		gNewObjectDefinition.slot 		= 479;
		gNewObjectDefinition.moveCall 	= MoveBlobDroplet;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= scale;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Special[0] = 0;						// init counter

			/* SET DELTAS */

		newObj->Delta.x = aim.x * 400.0f;
		newObj->Delta.y = aim.y * 400.0f;
		newObj->Delta.z = aim.z * 400.0f;

				/* SET THE ALIGNMENT MATRIX */

		SetAlignmentMatrix(&newObj->AlignmentMatrix, &aim);


			/* SET COLLISION */

		newObj->Damage 			= .1;
		newObj->CType 			= CTYPE_HURTME;

		CreateCollisionBoxFromBoundingBox_Maximized(newObj);
	}

}


/******************* MOVE BLOB DROPLET ********************/

static void MoveBlobDroplet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLVector3D	aim;


	GetObjectInfo(theNode);

			/* MOVE IT */

	gDelta.y -= 800.0f * fps;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

			/* SHRINK */

	theNode->Scale.x =
	theNode->Scale.y =
	theNode->Scale.z -= fps * .7f;
	if (theNode->Scale.x <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if (HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE, -.5f))
	{
//		theNode->Special[0]++;
//		if (theNode->Special[0] > 3)
//		{
//			ExplodeGeometry(theNode, 200, SHARD_MODE_FROMORIGIN, 3, .5);
//			goto gone;
//		}
	}


		/* SET NEW ALIGNMENT & UPDATE */


	FastNormalizeVector(gDelta.x, gDelta.y, gDelta.z, &aim);
	SetAlignmentMatrix(&theNode->AlignmentMatrix, &aim);
	UpdateObject(theNode);
}


#pragma mark -

/******************** ADD SMOKER ****************************/

Boolean AddSmoker(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	newObj = MakeSmoker(x,z, itemPtr->parm[0]);
	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	return(true);
}


/****************** MAKE SMOKER **********************/

ObjNode *MakeSmoker(float  x, float z, int kind)
{
ObjNode	*newObj;

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= FindHighestCollisionAtXZ(x,z, CTYPE_TERRAIN | CTYPE_WATER);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+10;
	gNewObjectDefinition.moveCall 	= MoveSmoker;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->Kind = kind;								// save smoke kind

	return(newObj);
}


/******************** MOVE SMOKER ************************/

static void MoveSmoker(ObjNode *theNode)
{
float				fps = gFramesPerSecondFrac;
int					i,t;
int					particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;
short				smokeType;

static const short	textures[] =
{
	PARTICLE_SObjType_GreySmoke,
	PARTICLE_SObjType_BlackSmoke,
	PARTICLE_SObjType_RedFumes,
	PARTICLE_SObjType_GreenFumes
};

static const Boolean	glow[] =
{
	false,
	false,
	true,
	true
};

		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


	theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z);		// make sure on ground (for when volcanos grow over it)


		/**************/
		/* MAKE SMOKE */
		/**************/

	theNode->Timer -= fps;													// see if add smoke
	if (theNode->Timer <= 0.0f)
	{
		theNode->Timer += .1f;												// reset timer

		t = textures[smokeType = theNode->Kind];										// get texture #

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{

			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 170;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 20;
			groupDef.decayRate				= -.7;
			groupDef.fadeRate				= .2;
			groupDef.particleTextureNum		= t;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			if (glow[smokeType])
				groupDef.dstBlend				= GL_ONE;
			else
				groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			float				x,y,z;

			x = theNode->Coord.x;
			y = theNode->Coord.y;
			z = theNode->Coord.z;

			for (i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * 30.0f;
				p.y = y;
				p.z = z + RandomFloat2() * 30.0f;

				d.x = RandomFloat2() * 80.0f;
				d.y = 150.0f + RandomFloat() * 75.0f;
				d.z = RandomFloat2() * 80.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat()*PI2;
				newParticleDef.rotDZ		= RandomFloat2() * .1f;
				newParticleDef.alpha		= .6;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}


#pragma mark -

/******************** ADD BUBBLER ****************************/

Boolean AddBubbler(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= WATER_SLOT-1;
	gNewObjectDefinition.moveCall 	= MoveBubbler;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	return(true);
}


/******************** MOVE BUBBLER ************************/

static void MoveBubbler(ObjNode *theNode)
{
float				fps = gFramesPerSecondFrac;
int					particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
static OGLVector3D	d = {0,80,0};
OGLPoint3D			p;
float	x,y,z;

		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


		/************************/
		/* MAKE SURFACE RIPPLES */
		/************************/

	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;

	theNode->RippleTimer -= fps;
	if (theNode->RippleTimer <= 0.0f)
	{
		CreateNewRipple(x + RandomFloat2() * 20.0f, z + RandomFloat2() * 20.0f, 1.0f, 40.0f, .5);
		theNode->RippleTimer = .25;
	}


		/****************/
		/* MAKE BUBBLES */
		/****************/

	theNode->Timer -= fps;													// see if add bubbles
	if (theNode->Timer <= 0.0f)
	{
		theNode->Timer += .12f;												// reset timer

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{

			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND  | PARTICLE_FLAGS_HASMAXY;
			groupDef.gravity				= -15;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 2;
			groupDef.decayRate				= 0;
			groupDef.fadeRate				= 0;
			groupDef.particleTextureNum		= PARTICLE_SObjType_Bubble;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);

			GetWaterY(x, z, &gParticleGroups[particleGroup].maxY);			// set to pop on waterline
		}

		if (particleGroup != -1)
		{
			x = theNode->Coord.x;
			z = theNode->Coord.z;

			for (int i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * 10.0f;
				p.y = y;
				p.z = z + RandomFloat2() * 10.0f;

				d.y = 30.0f + RandomFloat() * 90.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= 1.0f + d.y * (1.0f / 120.0f);	// faster bubbles are bigger
				newParticleDef.rotZ			= 0;
				newParticleDef.rotDZ		= 0;
				newParticleDef.alpha		= .7f + RandomFloat() * .3f;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}









