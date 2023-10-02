/****************************/
/*   METAOBJECTS.C		    */
/* (c)2000 Pangea Software  */
/*   By Brian Greenstone    */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

extern PFNGLACTIVETEXTUREPROC gGlActiveTextureProc;
extern PFNGLCLIENTACTIVETEXTUREARBPROC gGlClientActiveTextureProc;
#define glActiveTextureARB gGlActiveTextureProc
#define glClientActiveTextureARB gGlClientActiveTextureProc


/****************************/
/*    PROTOTYPES            */
/****************************/

static MetaObjectPtr AllocateEmptyMetaObject(uint32_t type);
static void SetMetaObjectToGroup(MOGroupObject *groupObj);
static void SetMetaObjectToMaterial(MOMaterialObject *matObj, MOMaterialData *inData);
static void SetMetaObjectToVertexArrayGeometry(MOVertexArrayObject *geoObj, MOVertexArrayData *data);
static void SetMetaObjectToMatrix(MOMatrixObject *matObj, OGLMatrix4x4 *inData);
static void MO_DetachFromLinkedList(MetaObjectPtr obj);
static void MO_DisposeObject_Group(MOGroupObject *group);
static void MO_DeleteObjectInfo_Material(MOMaterialObject *obj);
static void MO_CalcBoundingBox_Recurse(MetaObjectPtr object, OGLBoundingBox *bBox, const OGLMatrix4x4 *m);

static void SetMetaObjectToSprite(MOSpriteObject *spriteObj, MOSpriteSetupData *inData);
static void MO_DisposeObject_Sprite(MOSpriteObject *obj);


/****************************/
/*    CONSTANTS             */
/****************************/





/*********************/
/*    VARIABLES      */
/*********************/

MetaObjectHeader	*gFirstMetaObject = nil;
MetaObjectHeader 	*gLastMetaObject = nil;
int					gNumMetaObjects = 0;

float				gGlobalTransparency = 1;			// 0 == clear, 1 = opaque
OGLColorRGB			gGlobalColorFilter = {1,1,1};
uint32_t				gGlobalMaterialFlags = 0;

MOMaterialObject	*gMostRecentMaterial;


/***************** INIT META OBJECT HANDLER ******************/

void MO_InitHandler(void)
{
	gFirstMetaObject = nil;				// no meta object nodes yet
	gLastMetaObject = nil;
	gNumMetaObjects = 0;
}


#pragma mark -

/****************** MO: CREATE NEW OBJECT OF TYPE ****************/
//
// INPUT:	type = type of mo to create
//			data = pointer to any data needed to create the mo (optional)
//


MetaObjectPtr MO_CreateNewObjectOfType(uint32_t type, void *data)
{
MetaObjectPtr	mo;

			/* ALLOCATE EMPTY OBJECT */

	mo = AllocateEmptyMetaObject(type);
	if (mo == nil)
		return(nil);


			/* SET OBJECT INFO */

	switch(type)
	{
		case	MO_TYPE_GROUP:
				SetMetaObjectToGroup(mo);
				break;

		case	MO_TYPE_VERTEXARRAY:
				SetMetaObjectToVertexArrayGeometry(mo, data);
				break;

		case	MO_TYPE_MATERIAL:
				SetMetaObjectToMaterial(mo, data);
				break;

		case	MO_TYPE_MATRIX:
				SetMetaObjectToMatrix(mo, data);
				break;

		case	MO_TYPE_SPRITE:
				SetMetaObjectToSprite(mo, data);
				break;

		default:
				DoFatalAlert("MO_CreateNewObjectOfType: object type not recognized");
	}

	return(mo);
}


/****************** ALLOCATE EMPTY META OBJECT *****************/
//
// allocates an empty meta object and connects it to the linked list
//

static MetaObjectPtr AllocateEmptyMetaObject(uint32_t type)
{
MetaObjectHeader	*mo;
int					size = 0;

			/* DETERMINE SIZE OF DATA TO ALLOC */

	switch(type)
	{
		case	MO_TYPE_GROUP:
				size = sizeof(MOGroupObject);
				break;

		case	MO_TYPE_VERTEXARRAY:
				size = sizeof(MOVertexArrayObject);
				break;

		case	MO_TYPE_MATERIAL:
				size = sizeof(MOMaterialObject);
				break;

		case	MO_TYPE_MATRIX:
				size = sizeof(MOMatrixObject);
				break;

		case	MO_TYPE_SPRITE:
				size = sizeof(MOSpriteObject);
				break;

		default:
				DoFatalAlert("AllocateEmptyMetaObject: object type not recognized");
	}


			/* ALLOC MEMORY FOR META OBJECT */

	mo = AllocPtr(size);
	GAME_ASSERT(mo);


			/* INIT STRUCTURE */

	mo->cookie 		= MO_COOKIE;
	mo->type 		= type;
	mo->data 		= nil;
	mo->nextNode 	= nil;
	mo->refCount	= 1;							// initial reference count is always 1
	mo->parentGroup = nil;


		/***************************/
		/* ADD NODE TO LINKED LIST */
		/***************************/

		/* SEE IF IS ONLY NODE */

	if (gFirstMetaObject == nil)
	{
		GAME_ASSERT(!gLastMetaObject);	// gFirstMetaObject & gLastMetaObject should be nil

		mo->prevNode = nil;
		gFirstMetaObject = gLastMetaObject = mo;
		gNumMetaObjects = 1;
	}

		/* ADD TO END OF LINKED LIST */

	else
	{
		mo->prevNode = gLastMetaObject;		// point new prev to last
		gLastMetaObject->nextNode = mo;		// point old last to new
		gLastMetaObject = mo;				// set new last
		gNumMetaObjects++;
	}

	return(mo);
}

/******************* SET META OBJECT TO GROUP ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//

static void SetMetaObjectToGroup(MOGroupObject *groupObj)
{

			/* INIT THE DATA */

	groupObj->objectData.numObjectsInGroup = 0;
}


/*************** SET META OBJECT TO VERTEX ARRAY GEOMETRY ***************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.  It also boosts the ref count of
// any referenced items.
//

static void SetMetaObjectToVertexArrayGeometry(MOVertexArrayObject *geoObj, MOVertexArrayData *data)
{
			/* INIT THE DATA */

	geoObj->objectData = *data;									// copy from input data


		/* INCREASE MATERIAL REFERENCE COUNTS */

	for (int i = 0; i < data->numMaterials; i++)
	{
		if (data->materials[i] != nil)				// make sure this material ref is valid
			MO_GetNewReference(data->materials[i]);
	}
}


/******************* SET META OBJECT TO MATERIAL ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.
//

static void SetMetaObjectToMaterial(MOMaterialObject *matObj, MOMaterialData *inData)
{

		/* COPY INPUT DATA */

	matObj->objectData = *inData;

#if 0
		{
			MO_DrawMaterial(matObj);		// safety prime ----------
			glBegin(GL_TRIANGLES);
			glTexCoord2f(0,0); glVertex3f(0,0,0);
			glTexCoord2f(1,0); glVertex3f(0,100,0);
			glTexCoord2f(0,1); glVertex3f(0,0,1000);
			glEnd();
		}
#endif

}


/******************* SET META OBJECT TO MATRIX ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.
//

static void SetMetaObjectToMatrix(MOMatrixObject *matObj, OGLMatrix4x4 *inData)
{

		/* COPY INPUT DATA */

	matObj->matrix = *inData;
}


/******************* SET META OBJECT TO SPRITE ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.
//

static void SetMetaObjectToSprite(MOSpriteObject *spriteObj, MOSpriteSetupData *inData)
{
MOSpriteData	*spriteData = &spriteObj->objectData;

			/* GET MATERIAL FROM SPRITE LIST */

	short	group,type;

	group = inData->group;
	type = inData->type;

	GAME_ASSERT(inData->type < gNumSpritesInGroupList[group]);						// make sure type is valid

	spriteData->material = gSpriteGroupList[group][type].materialObject;
	MO_GetNewReference(spriteData->material);										// this is a new reference, so inc ref count

	spriteData->width 		= gSpriteGroupList[group][type].width;					// get width and height of texture
	spriteData->height 		= gSpriteGroupList[group][type].height;
	spriteData->aspectRatio = gSpriteGroupList[group][type].aspectRatio;			// get aspect ratio


			/*******************************/
			/* SET SOME SPRITE OBJECT DATA */
			/*******************************/

	spriteData->scaleBasis = spriteData->width / SPRITE_SCALE_BASIS_DENOMINATOR;		// calculate a scale basis to keep things scaled relative to texture size


	spriteData->coord.x		= -1.0;								// assume upper left corner
	spriteData->coord.y		= 1.0;
	spriteData->coord.z		= 0;								// assume in front
	spriteData->scaleX		= 1.0;								// scale is normal
	spriteData->scaleY		= 1.0;
	spriteData->rot			= 0;								// rot
}


#pragma mark -


/********************** MO: APPEND TO GROUP ****************/
//
// Attach new object to end of group
//

void MO_AppendToGroup(MOGroupObject *group, MetaObjectPtr newObject)
{
int	i;

			/* VERIFY COOKIE */

	GAME_ASSERT(group->objectHeader.cookie == MO_COOKIE);
	GAME_ASSERT(((MetaObjectHeader *)newObject)->cookie == MO_COOKIE);


	i = group->objectData.numObjectsInGroup++;		// get index into group list
	GAME_ASSERT(i < MO_MAX_ITEMS_IN_GROUP);

	MO_GetNewReference(newObject);					// get new reference to object
	group->objectData.groupContents[i] = newObject;	// save object reference in group
}

/**************** MO: ATTACH TO GROUP START **************/
//
// Attach new object to START of group
//

void MO_AttachToGroupStart(MOGroupObject *group, MetaObjectPtr newObject)
{
int	i,j;

			/* VERIFY COOKIE */

	GAME_ASSERT(group->objectHeader.cookie == MO_COOKIE);
	GAME_ASSERT(((MetaObjectHeader *)newObject)->cookie == MO_COOKIE);


	i = group->objectData.numObjectsInGroup++;		// get index into group list
	GAME_ASSERT(i < MO_MAX_ITEMS_IN_GROUP);

	MO_GetNewReference(newObject);					// get new reference to object


			/* SHIFT ALL EXISTING OBJECTS UP */

	for (j = i; j > 0; j--)
		group->objectData.groupContents[j] = group->objectData.groupContents[j-1];

	group->objectData.groupContents[0] = newObject;	// save object ref into group
}


#pragma mark -


/******************** MO: DRAW OBJECT ***********************/
//
// This recursive function will draw any objects submitted and parses groups.
//

void MO_DrawObject(const MetaObjectPtr object)
{
MetaObjectHeader	*objHead = object;

			/* VERIFY COOKIE */

	GAME_ASSERT(objHead->cookie == MO_COOKIE);


			/* HANDLE TYPE */

	switch(objHead->type)
	{
		case	MO_TYPE_VERTEXARRAY:
				MO_DrawGeometry_VertexArray(&((MOVertexArrayObject*)object)->objectData);
				break;

		case	MO_TYPE_MATERIAL:
				MO_DrawMaterial(object);
				break;

		case	MO_TYPE_GROUP:
				MO_DrawGroup(object);
				break;

		case	MO_TYPE_MATRIX:
				MO_DrawMatrix(object);
				break;

		case	MO_TYPE_SPRITE:
				MO_DrawSprite(object);
				break;

		default:
			DoFatalAlert("MO_DrawObject: unknown type!");
	}
}


/******************** MO_DRAW GROUP *************************/

void MO_DrawGroup(const MOGroupObject *object)
{
int	numChildren,i;

				/* VERIFY OBJECT TYPE */

	GAME_ASSERT(object->objectHeader.type == MO_TYPE_GROUP);


			/*************************************/
			/* PUSH CURRENT STATE ON STATE STACK */
			/*************************************/


	OGL_PushState();


				/***************/
				/* PARSE GROUP */
				/***************/

	numChildren = object->objectData.numObjectsInGroup;			// get # objects in group

	for (i = 0; i < numChildren; i++)
	{
		MO_DrawObject(object->objectData.groupContents[i]);
	}


			/******************************/
			/* POP OLD STATE OFF OF STACK */
			/******************************/

	OGL_PopState();
}


/******************** MO: DRAW GEOMETRY - VERTEX ARRAY *************************/

void MO_DrawGeometry_VertexArray(const MOVertexArrayData *data)
{
Boolean		useTexture = false, multiTexture = false, texGen = false;
uint32_t 	materialFlags;
Boolean		needNormals;

			/**********************/
			/* SETUP VERTEX ARRAY */
			/**********************/

	glEnableClientState(GL_VERTEX_ARRAY);				// enable vertex arrays
	glVertexPointer(3, GL_FLOAT, 0, data->points);		// point to points array



			/***********************/
			/* SETUP VERTEX COLORS */
			/***********************/

			/* IF LIGHTING, THEN USE COLOR FLOATS */

	if (gMyState_Lighting)
	{
		if (data->colorsFloat)									// do we have float colors?
		{
			glColorPointer(4, GL_FLOAT, 0, data->colorsFloat);
			glEnableClientState(GL_COLOR_ARRAY);				// enable color arrays
		}
		else
		if (data->colorsByte)									// no floats, so check bytes
		{
			glColorPointer(4, GL_UNSIGNED_BYTE, 0, data->colorsByte);
			glEnableClientState(GL_COLOR_ARRAY);				// enable color arrays
		}
		else
			glDisableClientState(GL_COLOR_ARRAY);				// no color data at all, so disable
	}

			/* IF NOT LIGHTING, THEN USE COLOR BYTES */

	else
	{
		if (data->colorsByte)									// do we have byte colors?
		{
			glColorPointer(4, GL_UNSIGNED_BYTE, 0, data->colorsByte);
			glEnableClientState(GL_COLOR_ARRAY);				// enable color arrays
		}
		else
		if (data->colorsFloat)									// no bytes, so check floats
		{
			glColorPointer(4, GL_FLOAT, 0, data->colorsFloat);
			glEnableClientState(GL_COLOR_ARRAY);				// enable color arrays
		}
		else
			glDisableClientState(GL_COLOR_ARRAY);				// no color data at all, so disable
	}

	OGL_CheckError();





			/****************************/
			/* SEE IF ACTIVATE MATERIAL */
			/****************************/
			//
			// For now, I'm just looking at material #0.
			//


	if (data->numMaterials < 0)							// if (-), then assume texture has been manually set
	{
		goto use_current;
	}

	if (data->numMaterials > 0)									// are there any materials?
	{
				/*************************************************/
				/* SEE IF DO PLAIN MULTI-TEXTURING FROM GEOMETRY */
				/*************************************************/

		if (data->numMaterials > 1)
		{
			useTexture = multiTexture = true;

			for (int i = 0; i < data->numMaterials; i++)
			{
				glActiveTextureARB(GL_TEXTURE0_ARB+i);								// activate texture layer #i
				glClientActiveTextureARB(GL_TEXTURE0_ARB+i);
				glEnable(GL_TEXTURE_2D);

				glTexCoordPointer(2, GL_FLOAT, 0,data->uvs[i]);						// enable uv arrays
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);

				MO_DrawMaterial(data->materials[i]);						// submit material #n

				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

				if (i == 0)
				{
					gMostRecentMaterial = nil;							// need duplicate submits to be okay
				}
			}

			goto go_here;
		}


				/* MAYBE ONLY 1 MATERIAL IN GEOMETRY */

		MO_DrawMaterial(data->materials[0]);			// submit material #0 (also applies for multitexture layer 0)


			/* IF TEXTURED, THEN ALSO ACTIVATE UV ARRAY */

use_current:

		materialFlags = gMostRecentMaterial->objectData.flags;	// get material flags
		if (materialFlags & BG3D_MATERIALFLAG_TEXTURED)
		{
			if (data->uvs[0])
			{
						/***************************/
						/* SEE IF DO MULTI-TEXTURE */
						/***************************/

				if (materialFlags & BG3D_MATERIALFLAG_MULTITEXTURE)
				{
					uint16_t	multiTextureMode 	= gMostRecentMaterial->objectData.multiTextureMode;
					uint16_t	multiTextureCombine = gMostRecentMaterial->objectData.multiTextureCombine;
					uint16_t	envMapNum 		= gMostRecentMaterial->objectData.envMapNum;

					GAME_ASSERT(envMapNum < gNumSpritesInGroupList[SPRITE_GROUP_SPHEREMAPS]);

					multiTexture = true;


					switch(multiTextureMode)
					{
								/* REFLECTION SPHERE */

						case	MULTI_TEXTURE_MODE_REFLECTIONSPHERE:
								for (int i = 0; i < 2; i++)
								{
									glActiveTextureARB(GL_TEXTURE0_ARB+i);								// activate texture layer #i
									glClientActiveTextureARB(GL_TEXTURE0_ARB+i);
									glEnable(GL_TEXTURE_2D);

									if (i == 0)
									{
										glTexCoordPointer(2, GL_FLOAT, 0,data->uvs[0]);					// enable uv arrays
										glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
										glEnableClientState(GL_TEXTURE_COORD_ARRAY);
									}
									else
									{
										MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_SPHEREMAPS][envMapNum].materialObject);		// activate reflection map texture

#if USE_GL_COLOR_MATERIAL
										glEnable(GL_COLOR_MATERIAL);
#endif

										switch(multiTextureCombine)														// set combining info
										{
											case	MULTI_TEXTURE_COMBINE_MODULATE:
													glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
													break;

											case	MULTI_TEXTURE_COMBINE_ADD:
													glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
													glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
													glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
													break;

											case	MULTI_TEXTURE_COMBINE_ADDALPHA:										// note, when we do this gGlobalTransparency will have no effect
													glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
													glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
													glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
													break;
										}

										glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);							// activate reflection mapping
										glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
										glEnable(GL_TEXTURE_GEN_S);
										glEnable(GL_TEXTURE_GEN_T);
										texGen = true;
									}
								}
								break;


					}
				}
							/* JUST 1 TEXTURE LAYER */
				else
				{
					glTexCoordPointer(2, GL_FLOAT, 0,data->uvs[0]);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);	// enable uv arrays
				}

				useTexture = true;

				OGL_CheckError();
			}
		}
	}
	else
		glDisable(GL_TEXTURE_2D);						// no materials, thus no texture, thus turn this off


		/* WE DONT HAVE ENOUGH INFO TO DO TEXTURES, SO DISABLE */

	if (!useTexture)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		OGL_CheckError();
	}

go_here:

			/************************/
			/* SETUP VERTEX NORMALS */
			/************************/
			//
			// We do this last because we need to know some things
			// before we can determine if normals are actually needed
			//

	if (data->normals == nil)							// see if we even have normals to pass
		needNormals = false;
	else
	{
		if (!gMyState_Lighting)							// if no lighting, then probably don't need to pass normals
			needNormals = texGen;						// pass normals if doing texGen for sphere maps, etc.
		else											// there's lighting, so pass the normals
			needNormals = true;
	}

	if (needNormals)
	{
		glNormalPointer(GL_FLOAT, 0, data->normals);
		glEnableClientState(GL_NORMAL_ARRAY);			// enable normal arrays

#if 0
			/* SHOW VERTEX NORMALS */
		{
			int	i;

			OGL_PushState();
			glDisable(GL_TEXTURE_2D);
			SetColor4f(1,1,0,1);
			for (i = 0; i < data->numPoints; i++)
			{
				glBegin(GL_LINES);

				glVertex3fv((GLfloat *)&data->points[i]);
				glVertex3f(data->points[i].x + data->normals[i].x * 20.0f,
							data->points[i].y + data->normals[i].y * 20.0f,
							data->points[i].z + data->normals[i].z * 20.0f);

				glEnd();
			}
			OGL_PopState();
		}
#endif

	}
	else
		glDisableClientState(GL_NORMAL_ARRAY);			// disable normal arrays

	OGL_CheckError();


			/***********/
			/* DRAW IT */
			/***********/

//	glLockArraysEXT(0, data->numPoints);
	glDrawElements(GL_TRIANGLES,data->numTriangles*3,GL_UNSIGNED_INT,&data->triangles[0]);
	OGL_CheckError();
//	glUnlockArraysEXT();

	gPolysThisFrame += data->numPoints;					// inc poly counter


			/* CLEANUP */

	if (multiTexture)
	{
		glActiveTextureARB(GL_TEXTURE1_ARB);			// turn off textureing for multi-texture layer 2 since it isnt needed anymore
		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);

		glActiveTextureARB(GL_TEXTURE0_ARB);			// make sure #0 is active when we leave
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
	}

}





/************************ DRAW MATERIAL **************************/

void MO_DrawMaterial(MOMaterialObject *matObj)
{
MOMaterialData		*matData;
OGLColorRGBA		*diffuseColor,diffColor2;
Boolean				textureHasAlpha = false;
Boolean				alreadySet;
uint32_t				matFlags;

			/* SEE IF THIS MATERIAL IS ALREADY SET AS CURRENT */

	matData = &matObj->objectData;									// point to material data

	GAME_ASSERT(matObj->objectHeader.cookie == MO_COOKIE);			// verify cookie


				/****************************/
				/* SEE IF TEXTURED MATERIAL */
				/****************************/

	matFlags = matData->flags | gGlobalMaterialFlags;				// check flags of material & global

	if (matFlags & BG3D_MATERIALFLAG_TEXTURED)
	{
		if (matData->pixelDstFormat == GL_RGBA)						// if 32bit with alpha, then turn blending on (see below)
			textureHasAlpha = true;


				/* ACTIVATE MATERIAL */

		alreadySet = (matObj == gMostRecentMaterial);
		if (alreadySet)												// see if even need to bother resetting this
		{
			glEnable(GL_TEXTURE_2D);								// just make sure textures are on
		}
		else
		{
			OGL_Texture_SetOpenGLTexture(matData->textureName[0]);	// set this texture active

			if (gDebugMode)
			{
				int	size;
				size = matData->width * matData->height;
				switch(matData->pixelDstFormat)
				{
					case	GL_RGBA:
					case	GL_RGB:
							gVRAMUsedThisFrame += size * 4;
							break;

					case	GL_RGB5_A1:
					case	GL_UNSIGNED_SHORT_1_5_5_5_REV:
							gVRAMUsedThisFrame += size * 2;
							break;
				}
			}
		}

				/* SET TEXTURE WRAPPING MODE */

		if (matFlags & BG3D_MATERIALFLAG_CLAMP_U)
		    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		else
		    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);

		if (matFlags & BG3D_MATERIALFLAG_CLAMP_V)
		    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		else
		    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


	}
	else
		glDisable(GL_TEXTURE_2D);									// not textured, so disable textures


			/********************/
			/* COLORED MATERIAL */
			/********************/

	diffuseColor = &matData->diffuseColor;			// point to diffuse color

	if (gGlobalTransparency != 1.0f)				// see if need to factor in global transparency
	{
		diffColor2.r = diffuseColor->r;
		diffColor2.g = diffuseColor->g;
		diffColor2.b = diffuseColor->b;
		diffColor2.a = diffuseColor->a * gGlobalTransparency;
	}
	else
		diffColor2 = *diffuseColor;					// copy to local so we can apply filter to it without munging original


			/* APPLY COLOR FILTER */

	diffColor2.r *= gGlobalColorFilter.r;
	diffColor2.g *= gGlobalColorFilter.g;
	diffColor2.b *= gGlobalColorFilter.b;


	SetColor4fv(&diffColor2.r);					// set current diffuse color
#if USE_GL_COLOR_MATERIAL
//	glEnable(GL_COLOR_MATERIAL);	//-------- continuously reenable this since OGL seems to have a bug where it will magically get disabled.
#endif


		/* SEE IF NEED TO ENABLE BLENDING */


	if (textureHasAlpha || (diffColor2.a != 1.0f) || (matFlags & BG3D_MATERIALFLAG_ALWAYSBLEND))		// if has alpha, then we need blending on
	    glEnable(GL_BLEND);
	else
	    glDisable(GL_BLEND);


			/* SAVE THIS STUFF */

	gMostRecentMaterial = matObj;
}


/************************ DRAW MATRIX **************************/

void MO_DrawMatrix(const MOMatrixObject *matObj)
{
const OGLMatrix4x4		*m;

	m = &matObj->matrix;							// point to matrix

				/* MULTIPLY CURRENT MATRIX BY THIS */

	glMultMatrixf((GLfloat *)m);
	OGL_CheckError();
}

/************************ MO: DRAW SPRITE **************************/
//
// Assume that the matrices are already set to identity
//
// Also, assume that the projection matrix is already the identity matrix.
//

void MO_DrawSprite(const MOSpriteObject *spriteObj)
{
const MOSpriteData	*spriteData = &spriteObj->objectData;
float			scaleX,scaleY,x,y;
MOMaterialObject	*mo;
float				aspect, xoff, yoff;
OGLMatrix3x3		m;
OGLPoint2D			p[4];

	x = spriteData->coord.x;
	y = spriteData->coord.y;

	scaleX = spriteData->scaleX;
	scaleY = spriteData->scaleY;

		/* ACTIVATE THE MATERIAL */

	MO_DrawMaterial(mo = spriteData->material);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

	xoff = scaleX*.5f;
	yoff = (scaleY*aspect)*.5f;


				/* SET COORDS */

	p[0].x = -xoff;		p[0].y = -yoff;
	p[1].x = xoff;		p[1].y = -yoff;
	p[2].x = xoff;		p[2].y = yoff;
	p[3].x = -xoff;		p[3].y = yoff;

	OGLMatrix3x3_SetRotate(&m, spriteData->rot);
	OGLPoint2D_TransformArray(p, &m, p, 4);


			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(p[0].x + x, p[0].y + y);
	glTexCoord2f(1,1);	glVertex2f(p[1].x + x, p[1].y + y);
	glTexCoord2f(1,0);	glVertex2f(p[2].x + x, p[2].y + y);
	glTexCoord2f(0,0);	glVertex2f(p[3].x + x, p[3].y + y);
	glEnd();



	gPolysThisFrame += 2;						// 2 more tris
}



#pragma mark -

/******************** MO GET NEW REFERENCE *********************/

MetaObjectPtr MO_GetNewReference(MetaObjectPtr mo)
{
MetaObjectHeader *h = mo;

	h->refCount++;

	return(mo);
}




/********************** MO DISPOSE OBJECT REFERENCE ******************************/
//
// NOTE:  	Groups and other objects are NOT sub-recursed.  When a group is de-referenced, only that group object is affected.
//

void MO_DisposeObjectReference(MetaObjectPtr obj)
{
MetaObjectHeader	*header = obj;

	GAME_ASSERT(obj);
	GAME_ASSERT(header->cookie == MO_COOKIE);


			/**************************************/
			/* DEC REFERENCE COUNT OF THIS OBJECT */
			/**************************************/

	header->refCount--;									// dec ref count

	GAME_ASSERT(header->refCount >= 0);					// see if over did it

	if (header->refCount == 0)							// see if we can DELETE this node
	{
			/* NO MORE REFERENCES, SO DELETE DATA */

		switch(header->type)
		{
			case	MO_TYPE_GROUP:
					MO_DisposeObject_Group(obj);
					break;

			case	MO_TYPE_VERTEXARRAY:
					MO_DeleteObjectInfo_Geometry_VertexArray(&((MOVertexArrayObject*)obj)->objectData);
					break;

			case	MO_TYPE_MATERIAL:
					MO_DeleteObjectInfo_Material(obj);
					break;

			case	MO_TYPE_SPRITE:
					MO_DisposeObject_Sprite(obj);
					break;

		}

			/* DELETE THE OBJECT NODE */

		MO_DetachFromLinkedList(obj);					// detach from linked list

		header->cookie = 0xdeadbeef;					// devalidate cookie
		SafeDisposePtr(obj);								// free memory used by object
		return;
	}
}



/***************** DETACH FROM LINKED LIST *********************/

static void MO_DetachFromLinkedList(MetaObjectPtr obj)
{
MetaObjectHeader	*header = obj;
MetaObjectHeader	*prev,*next;


			/* VERIFY COOKIE */

	GAME_ASSERT(header->cookie == MO_COOKIE);


	prev = header->prevNode;
	next = header->nextNode;

			/* SEE IF WAS 1ST NODE IN LIST */

	if (prev == nil)
	{
		gFirstMetaObject = next;
		if (gFirstMetaObject)
			gFirstMetaObject->prevNode = nil;
	}

			/* SEE IF WAS LAST NODE IN LIST */

	if (next == nil)
	{
		gLastMetaObject = prev;
		if (gLastMetaObject)
			gLastMetaObject->nextNode = nil;
	}

			/* SOMEWHERE IN THE MIDDLE */

	else
	if (prev != nil)
	{
		prev->nextNode = next;
		next->prevNode = prev;
	}

	gNumMetaObjects--;

	GAME_ASSERT(gNumMetaObjects >= 0);			// counter mismatch?

	if (gNumMetaObjects == 0)					// all gone
	{
		GAME_ASSERT(!prev);
		GAME_ASSERT(!next);
		GAME_ASSERT(!gFirstMetaObject);
		GAME_ASSERT(!gLastMetaObject);
	}
}


/**************** DISPOSE OBJECT:  GROUP ************************/
//
// Decrement the references of all objects in the group.
//

static void MO_DisposeObject_Group(MOGroupObject *group)
{
int	i,n;

	n = group->objectData.numObjectsInGroup;				// get # objects in group

	for (i = 0; i < n; i++)
	{
		MO_DisposeObjectReference(group->objectData.groupContents[i]);	// dispose of this object's ref
	}
}


/****************** DISPOSE OBJECT: SPRITE *******************/
//
// Decrement reference tothe material used in this sprite
//

static void MO_DisposeObject_Sprite(MOSpriteObject *obj)
{
MOSpriteData *data = &obj->objectData;

	MO_DisposeObjectReference(data->material);
}



/****************** DELETE OBJECT INFO: GEOMETRY : VERTEX ARRAY *******************/
//
// Assumes the contents (the materials) have already been dereferenced!
//

void MO_DeleteObjectInfo_Geometry_VertexArray(MOVertexArrayData *data)
{
int					i,n;

			/* DEREFERENCE ANY MATERIALS */

	n = data->numMaterials;
	for (i = 0; i < n; i++)
		MO_DisposeObjectReference(data->materials[i]);	// dispose of this object's ref


		/* DISPOSE OF VARIOUS ARRAYS */

	if (data->points)
	{
		SafeDisposePtr((Ptr)data->points);
		data->points = nil;
	}

	if (data->normals)
	{
		SafeDisposePtr((Ptr)data->normals);
		data->normals = nil;
	}

	if (data->uvs[0])
	{
		SafeDisposePtr((Ptr)data->uvs[0]);
		data->uvs[0] = nil;

		if (data->numMaterials == 2)					// see if also nuke secondary uv list
		{
			if (data->uvs[1])
			{
				SafeDisposePtr((Ptr)data->uvs[1]);
				data->uvs[1] = nil;
			}
		}
	}

	if (data->colorsByte)
	{
		SafeDisposePtr((Ptr)data->colorsByte);
		data->colorsByte = nil;
	}

	if (data->colorsFloat)
	{
		SafeDisposePtr((Ptr)data->colorsFloat);
		data->colorsFloat = nil;
	}

	if (data->triangles)
	{
		SafeDisposePtr((Ptr)data->triangles);
		data->triangles = nil;
	}
}



/****************** DELETE OBJECT INFO: MATERIAL *******************/

static void MO_DeleteObjectInfo_Material(MOMaterialObject *obj)
{
MOMaterialData		*data = &obj->objectData;

		/* DISPOSE OF TEXTURE NAMES */

	if (data->numMipmaps > 0)
		glDeleteTextures(data->numMipmaps, &data->textureName[0]);
}



#pragma mark -

/******************** MO_DUPLICATE VERTEX ARRAY DATA *********************/

void MO_DuplicateVertexArrayData(MOVertexArrayData *inData, MOVertexArrayData *outData)
{
int	i,n,s;
			/***********************************/
			/* GET NEW REFERENCES TO MATERIALS */
			/***********************************/

	outData->numMaterials = n = inData->numMaterials;

	for (i = 0; i < n; i++)
	{
		MO_GetNewReference(inData->materials[i]);
		outData->materials[i] = inData->materials[i];
	}

			/************************/
			/* DUPLICATE THE ARRAYS */
			/************************/

			/* POINTS */

	n = outData->numPoints = inData->numPoints;
	s = inData->numPoints * sizeof(OGLPoint3D);

	if (inData->points)
	{
		outData->points = AllocPtr(s);
		GAME_ASSERT(outData->points);
		BlockMove(inData->points, outData->points, s);
	}
	else
		outData->points = nil;


			/* NORMALS */

	s = n * sizeof(OGLVector3D);

	if (inData->normals)
	{
		outData->normals = AllocPtr(s);
		GAME_ASSERT(outData->normals);
		BlockMove(inData->normals, outData->normals, s);
	}
	else
		outData->normals = nil;


			/* UVS */

	s = n * sizeof(OGLTextureCoord);

	if (inData->uvs[0])
	{
		outData->uvs[0] = AllocPtr(s);
		GAME_ASSERT(outData->uvs[0]);
		BlockMove(inData->uvs[0], outData->uvs[0], s);
	}
	else
		outData->uvs[0] = nil;


			/* COLORS BYTE */

	s = n * sizeof(OGLColorRGBA_Byte);

	if (inData->colorsByte)
	{
		outData->colorsByte = AllocPtr(s);
		GAME_ASSERT(outData->colorsByte);
		BlockMove(inData->colorsByte, outData->colorsByte, s);
	}
	else
		outData->colorsByte = nil;


			/* COLORS FLOAT */

	s = n * sizeof(OGLColorRGBA);

	if (inData->colorsFloat)
	{
		outData->colorsFloat = AllocPtr(s);
		GAME_ASSERT(outData->colorsFloat);
		BlockMove(inData->colorsFloat, outData->colorsFloat, s);
	}
	else
		outData->colorsFloat = nil;


			/* TRIANGLES */

	n = outData->numTriangles = inData->numTriangles;
	s = n * sizeof(GLint) * 3;

	if (inData->triangles)
	{
		outData->triangles = AllocPtr(s);
		GAME_ASSERT(outData->triangles);
		BlockMove(inData->triangles, outData->triangles, s);
	}
	else
		outData->triangles = nil;
}

#pragma mark -


/******************** MO: CALC BOUNDING BOX ***********************/
//
// INPUT:
//			m = transform matrix to apply to verts or nil.
//

void MO_CalcBoundingBox(MetaObjectPtr object, OGLBoundingBox *bBox, OGLMatrix4x4 *m)
{
		/* INIT BBOX TO BOGUS VALUES */

	bBox->min.x =
	bBox->min.y =
	bBox->min.z = 100000000;

	bBox->max.x =
	bBox->max.y =
	bBox->max.z = -bBox->min.x;

	bBox->isEmpty = false;

			/* RECURSIVELY CALC BBOX */

	MO_CalcBoundingBox_Recurse(object, bBox, m);
}


/******************** MO: CALC BOUNDING BOX: RECURSE ***********************/

static void MO_CalcBoundingBox_Recurse(MetaObjectPtr object, OGLBoundingBox *bBox, const OGLMatrix4x4 *m)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayObject	*vObj;
MOGroupObject		*groupObject;
MOVertexArrayData	*geoData;
int					numChildren,i,numPoints;
float				x,y,z;

			/* VERIFY COOKIE */

	GAME_ASSERT(objHead->cookie == MO_COOKIE);


	switch(objHead->type)
	{
			/* CALC BBOX OF GEOMETRY */

		case	MO_TYPE_VERTEXARRAY:
				vObj = object;
				geoData = &vObj->objectData;
				numPoints = geoData->numPoints;

					/* TRANSFORM POINTS */

				if (m)
				{
					static OGLPoint3D tpoints[1000];

					GAME_ASSERT(numPoints <= 1000);					// make sure not overflowing buffer

					OGLPoint3D_TransformArray(geoData->points, m, tpoints, numPoints);
					for (i = 0; i < numPoints; i++)
					{
						x = tpoints[i].x;
						y = tpoints[i].y;
						z = tpoints[i].z;

						if (x < bBox->min.x)
							bBox->min.x = x;
						if (x > bBox->max.x)
							bBox->max.x = x;

						if (y < bBox->min.y)
							bBox->min.y = y;
						if (y > bBox->max.y)
							bBox->max.y = y;

						if (z < bBox->min.z)
							bBox->min.z = z;
						if (z > bBox->max.z)
							bBox->max.z = z;
					}

				}

					/* NO TRANSFORM, USE RAW DATA */

				else
				{
					for (i = 0; i < numPoints; i++)
					{
						x = geoData->points[i].x;
						y = geoData->points[i].y;
						z = geoData->points[i].z;

						if (x < bBox->min.x)
							bBox->min.x = x;
						if (x > bBox->max.x)
							bBox->max.x = x;

						if (y < bBox->min.y)
							bBox->min.y = y;
						if (y > bBox->max.y)
							bBox->max.y = y;

						if (z < bBox->min.z)
							bBox->min.z = z;
						if (z > bBox->max.z)
							bBox->max.z = z;
					}
				}
				break;


			/* RECURSE THRU GROUP */

		case	MO_TYPE_GROUP:
				groupObject = object;
				numChildren = groupObject->objectData.numObjectsInGroup;
				for (i = 0; i < numChildren; i++)
					MO_CalcBoundingBox_Recurse(groupObject->objectData.groupContents[i], bBox, m);
				break;


		case	MO_TYPE_MATRIX:
				DoFatalAlert("MO_CalcBoundingBox_Recurse: why is there a matrix here?");
				break;
	}
}


#pragma mark -


/*************** MO: GEOMETRY OFFSET UVS *********************/

void MO_Geometry_OffserUVs(short group, short type, short geometryNum, float du, float dv)
{
MOVertexArrayObject	*mo;

	mo = gBG3DGroupList[group][type];												// point to this object

				/****************/
				/* GROUP OBJECT */
				/****************/

	if (mo->objectHeader.type == MO_TYPE_GROUP)										// see if need to go into group
	{
		MOGroupObject	*groupObj = (MOGroupObject *)mo;

		GAME_ASSERT(geometryNum < groupObj->objectData.numObjectsInGroup);			// make sure # is valid

				/* POINT TO 1ST GEOMETRY IN THE GROUP */

		if (geometryNum == -1)														// if -1 then assign to all textures for this model
		{
			int	i;
			for (i = 0; i < groupObj->objectData.numObjectsInGroup; i++)
			{
				MO_VertexArray_OffsetUVs(groupObj->objectData.groupContents[i], du, dv);
			}
		}
		else
		{
			MO_VertexArray_OffsetUVs(groupObj->objectData.groupContents[geometryNum], du, dv);
		}
	}

			/* NOT A GROUNP, SO ASSUME GEOMETRY */
	else
	{
		MO_VertexArray_OffsetUVs(mo, du, dv);
	}
}


/******************* MO: OBJECT OFFSET UVS ************************/

void MO_Object_OffsetUVs(MetaObjectPtr object, float du, float dv)
{
MetaObjectHeader	*objHead = object;
MOGroupData			*data;
int					i;
MOGroupObject		*group;

			/* VERIFY COOKIE */

	GAME_ASSERT(objHead->cookie == MO_COOKIE);


			/* HANDLE IT */

	switch(objHead->type)
	{
		case	MO_TYPE_VERTEXARRAY:
				MO_VertexArray_OffsetUVs(object, du, dv);
				break;

		case	MO_TYPE_GROUP:
				group = object;
				data = &group->objectData;

							/* PARSE OBJECTS IN GROUP */

				for (i = 0; i < data->numObjectsInGroup; i++)
				{
					switch(data->groupContents[i]->type)
					{
						case	MO_TYPE_VERTEXARRAY:
								MO_VertexArray_OffsetUVs(data->groupContents[i], du, dv);
								break;

						case	MO_TYPE_GROUP:
								MO_Object_OffsetUVs(data->groupContents[i], du, dv);		// recurse this sub-group
								break;

					}
				}
				break;


		default:
			DoFatalAlert("MO_Group_OffsetUVs: object type is not supported.");
	}

}


/******************* MO: VERTEX ARRAY, OFFSET UVS ************************/

void MO_VertexArray_OffsetUVs(MetaObjectPtr object, float du, float dv)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayData	*data;
int					numPoints,i;
OGLTextureCoord		*uvPtr;
MOVertexArrayObject	*vObj;

		/* MAKE SURE IT IS A VERTEX ARRAY */

	GAME_ASSERT(objHead->cookie == MO_COOKIE);
	GAME_ASSERT(objHead->type == MO_TYPE_VERTEXARRAY);

	vObj = object;
	data = &vObj->objectData;						// point to data


	uvPtr = data->uvs[0];								// point to uv list
	if (uvPtr == nil)
		return;

	numPoints = data->numPoints;					// get # points

			/* OFFSET THE UV'S */

	for (i = 0; i < numPoints; i++)
	{
		uvPtr[i].u += du;
		uvPtr[i].v += dv;
	}
}


#pragma mark -


/****************** MO:  CREATE TEXTURE OBJECT FROM BUFFER **************************/

MOMaterialObject *MO_CreateTextureObjectFromBuffer(int width, int height, Ptr buffer)
{
MOMaterialData	data;
MOMaterialObject	*obj;


			/***************************/
			/* CREATE A TEXTURE OBJECT */
			/***************************/

		/* INIT NEW MATERIAL DATA */

	data.flags 					= BG3D_MATERIALFLAG_TEXTURED;
	data.width					= width;
	data.height					= height;
	data.multiTextureMode		= MULTI_TEXTURE_MODE_REFLECTIONSPHERE;
	data.multiTextureCombine	= MULTI_TEXTURE_COMBINE_MODULATE;
	data.envMapNum				= 0;
	data.diffuseColor.r			= 1;
	data.diffuseColor.g			= 1;
	data.diffuseColor.b			= 1;
	data.diffuseColor.a			= 1;
	data.numMipmaps				= 1;
	data.pixelSrcFormat			= GL_RGBA;
	data.pixelDstFormat			= GL_RGBA;
	data.texturePixels[0] 		= buffer;								// keep a ptr to the original buffer so we can write it to a file if needed
	data.textureName[0] 		= OGL_TextureMap_Load(buffer, width, height,
												 data.pixelSrcFormat, data.pixelDstFormat, GL_UNSIGNED_BYTE);

	/* CREATE NEW MATERIAL OBJECT */

	obj = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, &data);


	return(obj);

}
