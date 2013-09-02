/****************************/
/*   	SPARKLE.C  			*/
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"
#include <AGL/aglmacro.h>

extern	float			gFramesPerSecondFrac,gFramesPerSecond,gPlayerToCameraAngle;
extern	OGLPoint3D		gCoord;
extern	OGLVector3D		gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	OGLVector3D		gRecentTerrainNormal;
extern	PlayerInfoType	gPlayerInfo;
extern	u_long			gAutoFadeStatusBits;
extern	PrefsType			gGamePrefs;
extern	SpriteType	*gSpriteGroupList[];
extern	const OGLPoint3D gPlayerMuzzleTipOff;


/****************************/
/*    PROTOTYPES            */
/****************************/



/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

SparkleType	gSparkles[MAX_SPARKLES];


int	gNumSparkles;

/*************************** INIT SPARKLES **********************************/

void InitSparkles(void)
{
int		i;

	for (i = 0; i < MAX_SPARKLES; i++)
	{
		gSparkles[i].isActive = false;
	}


	gNumSparkles = 0;
}


/*************************** GET FREE SPARKLE ******************************/
//
// OUTPUT:  -1 if none
//

short GetFreeSparkle(ObjNode *theNode)
{
int		i;

			/* FIND A FREE SLOT */

	for (i = 0; i < MAX_SPARKLES; i++)
	{
		if (!gSparkles[i].isActive)
			goto got_it;

	}
	return(-1);

got_it:

	gSparkles[i].isActive = true;
	gSparkles[i].owner = theNode;
	gNumSparkles++;

	return(i);
}




/***************** DELETE SPARKLE *********************/

void DeleteSparkle(short i)
{
	if (i == -1)
		return;

	if (gSparkles[i].isActive)
	{
		gSparkles[i].isActive = false;
		gNumSparkles--;
	}
	else
		DoAlert("\pDeleteSparkle: double delete sparkle");
}


/*************************** DRAW SPARKLES ******************************/

void DrawSparkles(OGLSetupOutputType *setupInfo)
{
u_long	flags;
int		i;
float	dot,separation;
OGLMatrix4x4	m;
AGLContext agl_ctx = setupInfo->drawContext;
OGLVector3D	v;
OGLPoint3D	where;
OGLVector3D	aim;
static const OGLVector3D 	up = {0,1,0};
OGLPoint3D					tc[4], *cameraLocation;
static OGLPoint3D		frame[4] =
{
	-130,130,0,
	130,130,0,
	130,-130,0,
	-130,-130,0
};


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

	cameraLocation = &setupInfo->cameraPlacement.cameraLocation;		// point to camera coord

	for (i = 0; i < MAX_SPARKLES; i++)
	{
		ObjNode	*owner;

		if (!gSparkles[i].isActive)							// must be active
			continue;

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

		frame[0].x = -gSparkles[i].scale;								// set size of quad
		frame[0].y = gSparkles[i].scale;
		frame[1].x = gSparkles[i].scale;
		frame[1].y = gSparkles[i].scale;
		frame[2].x = gSparkles[i].scale;
		frame[2].y = -gSparkles[i].scale;
		frame[3].x = -gSparkles[i].scale;
		frame[3].y = -gSparkles[i].scale;

		SetLookAtMatrixAndTranslate(&m, &up, &where, cameraLocation);	// aim at camera & translate
		OGLPoint3D_TransformArray(&frame[0], &m, tc, 4);



			/* DRAW IT */

		MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_PARTICLES][gSparkles[i].textureNum].materialObject, setupInfo);	// submit material

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















