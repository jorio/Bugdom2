/****************************/
/*   	SPARKLE.C  			*/
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawSparkles(ObjNode* theNode);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

SparkleType	gSparkles[MAX_SPARKLES];
static Pool	*gSparklePool = NULL;


/*************************** INIT SPARKLES **********************************/

void InitSparkles(void)
{
	GAME_ASSERT(!gSparklePool);

	gSparklePool = Pool_New(MAX_SPARKLES);

	MakeNewDriverObject(PARTICLE_SLOT-1, DrawSparkles, NULL);
}

/*************************** DISPOSE SPARKLES *******************************/

void DisposeSparkles(void)
{
	if (gSparklePool)
	{
		Pool_Free(gSparklePool);
		gSparklePool = NULL;
	}
}


/*************************** GET FREE SPARKLE ******************************/
//
// OUTPUT:  -1 if none
//

int GetFreeSparkle(ObjNode *owner)
{
	int i = Pool_AllocateIndex(gSparklePool);

	if (i < 0)
		return i;

	gSparkles[i].owner = owner;
	return i;
}




/***************** DELETE SPARKLE *********************/

void DeleteSparkle(int i)
{
	if (i == -1)
		return;

	Pool_ReleaseIndex(gSparklePool, i);
}


/*************************** DRAW SPARKLES ******************************/

static void DrawSparkles(ObjNode* theNode)
{
uint32_t	flags;
float	dot,separation;
OGLMatrix4x4	m;
OGLVector3D	v;
OGLPoint3D	where;
OGLVector3D	aim;
static const OGLVector3D 	up = {0,1,0};
OGLPoint3D					*cameraLocation;



	(void) theNode;

			/* EARLY OUT IF NONE */

	if (Pool_Empty(gSparklePool))
		return;

	OGL_PushState();

	OGL_DisableLighting();									// deactivate lighting
	glDisable(GL_NORMALIZE);								// disable vector normalizing since scale == 1
	glDisable(GL_CULL_FACE);								// deactivate culling
	glDisable(GL_FOG);										// deactivate fog
	glDepthMask(GL_FALSE);									// no z-writes
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);		// set blending mode


			/*********************/
			/* DRAW EACH SPARKLE */
			/*********************/

	cameraLocation = &gGameView->cameraPlacement.cameraLocation;		// point to camera coord

	for (int i = Pool_First(gSparklePool); i >= 0; i = Pool_Next(gSparklePool, i))
	{
		GAME_ASSERT(Pool_IsUsed(gSparklePool, i));

		ObjNode	*owner;

		flags = gSparkles[i].flags;							// get sparkle flags

		owner = gSparkles[i].owner;
		if (owner != nil)									// if owner is culled then dont draw
		{
			if (owner->StatusBits & (STATUS_BIT_ISCULLED|STATUS_BIT_HIDDEN))
				continue;

					/* SEE IF TRANSFORM WITH OWNER */

			if (flags & SPARKLE_FLAG_TRANSFORMWITHOWNER)
			{
				OGLPoint3D_Transform(&gSparkles[i].where, &owner->BaseTransformMatrix, &where);
				OGLVector3D_Transform(&gSparkles[i].aim, &owner->BaseTransformMatrix, &aim);
			}
			else
			{
				where = gSparkles[i].where;
				aim = gSparkles[i].aim;
			}
		}
		else
		{
			where = gSparkles[i].where;
			aim = gSparkles[i].aim;
		}


			/* CALC ANGLE INFO */

		v.x = cameraLocation->x - where.x;			// calc vector from sparkle to camera
		v.y = cameraLocation->y - where.y;
		v.z = cameraLocation->z - where.z;
		FastNormalizeVector(v.x, v.y, v.z, &v);

		separation = gSparkles[i].separation;
		where.x += v.x * separation;									// offset the base point
		where.y += v.y * separation;
		where.z += v.z * separation;

		if (!(flags & SPARKLE_FLAG_OMNIDIRECTIONAL))		// if not omni then calc alpha based on angle
		{
			dot = OGLVector3D_Dot(&v, &aim);				// calc angle between
			if (dot <= 0.0f)
				continue;

			gSparkles[i].color.a = dot;									// make brighter as we look head-on
		}



			/* CALC TRANSFORM MATRIX */

		float s = gSparkles[i].scale;
		OGLPoint3D tc[4] =			// set size of quad
		{
			{-s,s,0},
			{s,s,0},
			{s,-s,0},
			{-s,-s,0},
		};

		SetLookAtMatrixAndTranslate(&m, &up, &where, cameraLocation);	// aim at camera & translate
		OGLPoint3D_TransformArray(tc, &m, tc, 4);



			/* DRAW IT */

		MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_PARTICLES][gSparkles[i].textureNum].materialObject);	// submit material

		if (flags & SPARKLE_FLAG_FLICKER)								// set transparency
		{
			OGLColorRGBA	c = gSparkles[i].color;

			c.a += RandomFloat2() * .5f;
			if (c.a < 0.0)
				continue;
			else
			if (c.a > 1.0f)
				c.a = 1.0;

			SetColor4fv((GLfloat *)&c);
		}
		else
			SetColor4fv((GLfloat *)&gSparkles[i].color);


		glBegin(GL_QUADS);
		glTexCoord2f(0,0);	glVertex3fv((GLfloat *)&tc[0]);
		glTexCoord2f(1,0);	glVertex3fv((GLfloat *)&tc[1]);
		glTexCoord2f(1,1);	glVertex3fv((GLfloat *)&tc[2]);
		glTexCoord2f(0,1);	glVertex3fv((GLfloat *)&tc[3]);
		glEnd();
	}


			/* RESTORE STATE */

	OGL_PopState();
}
