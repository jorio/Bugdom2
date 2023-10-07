/****************************/
/*   	BG3D.C 				*/
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

static void ReadBG3DHeader(short refNum);
static void ParseBG3DFile(short refNum);
static void ReadMaterialFlags(short refNum);
static void ReadMaterialDiffuseColor(short refNum);
static void ReadMaterialTextureMap(short refNum);
static void ReadGroup(void);
static MetaObjectPtr ReadNewGeometry(short refNum);
static MetaObjectPtr ReadVertexElementsGeometry(BG3DGeometryHeader *header);
static void InitBG3DContainer(void);
static void EndGroup(void);
static void ReadVertexArray(short refNum);
static void ReadNormalArray(short refNum);
static void ReadUVArray(short refNum);
static void ReadVertexColorArray(short refNum);
static void ReadTriangleArray(short refNum);
static void PreLoadTextureMaterials(void);
static void ReadBoundingBox(short refNum);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	BG3D_GROUP_STACK_SIZE	50						// max nesting depth of groups



/*********************/
/*    VARIABLES      */
/*********************/

static int					gBG3D_GroupStackIndex;
static MOGroupObject		*gBG3D_GroupStack[BG3D_GROUP_STACK_SIZE];
static MOGroupObject		*gBG3D_CurrentGroup;

static MOMaterialObject		*gBG3D_CurrentMaterialObj;			// note: this variable contains an illegal ref to the object.  The real ref is in the file container material list.
static MOVertexArrayObject	*gBG3D_CurrentGeometryObj;

static BG3DFileContainer	*gBG3D_CurrentContainer;


BG3DFileContainer		*gBG3DContainerList[MAX_BG3D_GROUPS];
MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];		// ILLEGAL references!!!
int						gNumObjectsInBG3DGroupList[MAX_BG3D_GROUPS];
OGLBoundingBox			gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];


/****************** INIT BG3D MANAGER ***********************/

void InitBG3DManager(void)
{
int	i;

	for (i = 0; i < MAX_BG3D_GROUPS; i++)
	{
		gBG3DContainerList[i] = nil;
	}
}


/********************** IMPORT BG3D **************************/
//
// NOTE:  	All BG3D models must be imported AFTER the draw context has been created,
//			because all imported textures are named with OpenGL and loaded into OpenGL!
//

void ImportBG3D(FSSpec *spec, int groupNum)
{
short				refNum;
MetaObjectHeader	*header;
MOGroupObject		*group;
MOGroupData			*data;
OSErr				err;

			/* INIT SOME VARIABLES */

	gBG3D_CurrentMaterialObj 	= nil;
	gBG3D_CurrentGeometryObj	= nil;
	gBG3D_GroupStackIndex		= 0;			// init the group stack
	InitBG3DContainer();


		/************************/
		/* OPEN THE FILE & READ */
		/************************/

	err = FSpOpenDF(spec, fsRdPerm, &refNum);
	GAME_ASSERT(!err);

	ReadBG3DHeader(refNum);
	ParseBG3DFile(refNum);

			/* CLOSE FILE */

	FSClose(refNum);


		/*********************************************/
		/* PRELOAD ALL TEXTURE MATERIALS INTO OPENGL */
		/*********************************************/

	PreLoadTextureMaterials();


			/********************/
			/* SETUP GROUP INFO */
			/********************/
			//
			// A model "group" is a grouping of 3D models.
			//

	gBG3DContainerList[groupNum] = gBG3D_CurrentContainer;			// save container into list

	header = gBG3D_CurrentContainer->root;							// point to root object in container
	GAME_ASSERT(header);
	GAME_ASSERT(header->type == MO_TYPE_GROUP);						// root should be a group


			/* PARSE GROUP */
			//
			// Create a list of all of the models inside this file & Calc bounding box
			//

	group = gBG3D_CurrentContainer->root;
	data = &group->objectData;

	for (int i = 0; i < data->numObjectsInGroup; i++)
	{
		OGLBoundingBox	*bbox = &gObjectGroupBBoxList[groupNum][i];		// point to bbox destination

		gBG3DGroupList[groupNum][i] = data->groupContents[i];			// copy ILLEGAL ref to this object
		MO_CalcBoundingBox(data->groupContents[i], bbox, nil);				// calc bounding box of this model
	}

	gNumObjectsInBG3DGroupList[groupNum] = data->numObjectsInGroup;
}


/********************** READ BG3D HEADER **************************/

static void ReadBG3DHeader(short refNum)
{
BG3DHeaderType	headerData;
long			count;
OSErr			err;


	count = sizeof(BG3DHeaderType);

	err = FSRead(refNum, &count, (Ptr) &headerData);
	GAME_ASSERT(!err);

			/* VERIFY FILE */

	GAME_ASSERT_MESSAGE(0 == SDL_memcmp("BG3D", headerData.headerString, 4), "BG3D file has invalid header");
}


/****************** PARSE BG3D FILE ***********************/

static void ParseBG3DFile(short refNum)
{
	MetaObjectPtr newObj = NULL;

	while (1)
	{
			/* READ A TAG */

		uint32_t tag = FSReadBEULong(refNum);


			/* HANDLE THE TAG */

		switch(tag)
		{
			case	BG3D_TAGTYPE_MATERIALFLAGS:
					ReadMaterialFlags(refNum);
					break;

			case	BG3D_TAGTYPE_MATERIALDIFFUSECOLOR:
					ReadMaterialDiffuseColor(refNum);
					break;

			case	BG3D_TAGTYPE_TEXTUREMAP:
					ReadMaterialTextureMap(refNum);
					break;

			case	BG3D_TAGTYPE_GROUPSTART:
					ReadGroup();
					break;

			case	BG3D_TAGTYPE_GROUPEND:
					EndGroup();
					break;

			case	BG3D_TAGTYPE_GEOMETRY:
					newObj = ReadNewGeometry(refNum);
					if (gBG3D_CurrentGroup)								// add new geometry to current group
					{
						MO_AppendToGroup(gBG3D_CurrentGroup, newObj);
						MO_DisposeObjectReference(newObj);								// nuke the extra reference
					}
					break;

			case	BG3D_TAGTYPE_VERTEXARRAY:
					ReadVertexArray(refNum);
					break;

			case	BG3D_TAGTYPE_NORMALARRAY:
					ReadNormalArray(refNum);
					break;

			case	BG3D_TAGTYPE_UVARRAY:
					ReadUVArray(refNum);
					break;

			case	BG3D_TAGTYPE_COLORARRAY:
					ReadVertexColorArray(refNum);
					break;

			case	BG3D_TAGTYPE_TRIANGLEARRAY:
					ReadTriangleArray(refNum);
					break;

			case	BG3D_TAGTYPE_BOUNDINGBOX:
					ReadBoundingBox(refNum);
					break;

			case	BG3D_TAGTYPE_ENDFILE:
					return;

			default:
					DoFatalAlert("ParseBG3DFile: unrecognized tag 0x%08x", tag);
		}
	}
}


/******************* READ MATERIAL FLAGS ***********************/
//
// Reading new material flags indicatest the start of a new material.
//

static void ReadMaterialFlags(short refNum)
{
			/* READ FLAGS */

	uint32_t flags = FSReadBEULong(refNum);


		/* INIT NEW MATERIAL DATA */

	MOMaterialData data =
	{
		.flags 					= flags,
		.multiTextureMode		= MULTI_TEXTURE_MODE_REFLECTIONSPHERE,
		.multiTextureCombine	= MULTI_TEXTURE_COMBINE_MODULATE,
		.envMapNum				= 0,
		.diffuseColor			= {1, 1, 1, 1},
		.numMipmaps				= 0,				// there are currently 0 textures assigned to this material
	};

	/* CREATE NEW MATERIAL OBJECT */

	gBG3D_CurrentMaterialObj = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, &data);


	/* ADD THIS MATERIAL TO THE FILE CONTAINER */


	int i = gBG3D_CurrentContainer->numMaterials++;								// get index into file container's material list

	gBG3D_CurrentContainer->materials[i] = gBG3D_CurrentMaterialObj;			// stores the 1 reference here.
}


/*************** READ MATERIAL DIFFUSE COLOR **********************/

static void ReadMaterialDiffuseColor(short refNum)
{
GLfloat			color[4];
MOMaterialData	*data;

	GAME_ASSERT(gBG3D_CurrentMaterialObj);


			/* READ COLOR VALUE */

	color[0] = FSReadBEFloat(refNum);
	color[1] = FSReadBEFloat(refNum);
	color[2] = FSReadBEFloat(refNum);
	color[3] = FSReadBEFloat(refNum);


		/* ASSIGN COLOR TO CURRENT MATERIAL */

	data = &gBG3D_CurrentMaterialObj->objectData; 							// get ptr to material data

	data->diffuseColor.r = color[0];
	data->diffuseColor.g = color[1];
	data->diffuseColor.b = color[2];
	data->diffuseColor.a = color[3];
}


/******************* READ MATERIAL TEXTURE MAP ***********************/
//
// NOTE:  This may get called multiple times - once for each mipmap associated with the
//			material.
//

static void ReadMaterialTextureMap(short refNum)
{
BG3DTextureHeader	textureHeader;
void		*texturePixels;
MOMaterialData	*data;
OSErr			err;

			/* GET PTR TO CURRENT MATERIAL */

	GAME_ASSERT(gBG3D_CurrentMaterialObj);

	data = &gBG3D_CurrentMaterialObj->objectData; 	// get ptr to material data


			/***********************/
			/* READ TEXTURE HEADER */
			/***********************/

	textureHeader.width				= FSReadBEULong(refNum);
	textureHeader.height			= FSReadBEULong(refNum);
	textureHeader.srcPixelFormat	= FSReadBELong(refNum);
	textureHeader.dstPixelFormat	= FSReadBELong(refNum);
	textureHeader.bufferSize		= FSReadBEULong(refNum);

	SetFPos(refNum, fsFromMark, sizeof(textureHeader.reserved));		// skip padding


			/* COPY BASIC INFO */

	GAME_ASSERT(data->numMipmaps <= MO_MAX_MIPMAPS);		// see if overflow

	if (data->numMipmaps == 0)					// see if this is the first texture
	{
		data->width 			= textureHeader.width;
		data->height	 		= textureHeader.height;
		data->pixelSrcFormat 	= textureHeader.srcPixelFormat;		// internal format
		data->pixelDstFormat 	= textureHeader.dstPixelFormat;		// vram format
	}


		/***************************/
		/* READ THE TEXTURE PIXELS */
		/***************************/

	long count = textureHeader.bufferSize;		// get size of buffer to load

	texturePixels = AllocPtrClear(count);		// alloc memory for buffer
	GAME_ASSERT(texturePixels);

	err = FSRead(refNum, &count, texturePixels);	// read pixel data
	GAME_ASSERT(!err);


			/* SWIZZLE */

	if (textureHeader.srcPixelFormat == GL_UNSIGNED_SHORT_1_5_5_5_REV)
	{
		uint16_t *pix = texturePixels;
		for (int i = 0; i < (count/2); i++)
		{
			pix[i] = SwizzleUShort(&pix[i]);
		}
	}

		/* ASSIGN PIXELS TO CURRENT MATERIAL */

	int mipmapNum = data->numMipmaps++;					// increment the mipmap count
	GAME_ASSERT(mipmapNum < MO_MAX_MIPMAPS);
	data->texturePixels[mipmapNum] = texturePixels;		// set ptr to pixelmap


#if 0
	// Texture dumper
	static int num = 0;
	char path[256];
	SDL_snprintf(path, sizeof(path), "/tmp/bg3d_%d_%d.tga", refNum, num);
	num++;
	SDL_RWops* f = SDL_RWFromFile(path, "wb");
	SDL_WriteU8(f, 0);
	SDL_WriteU8(f, 0);
	SDL_WriteU8(f, 2);
	for (int i = 0; i < 9; i++) SDL_WriteU8(f, 0);
	SDL_WriteLE16(f, textureHeader.width);
	SDL_WriteLE16(f, textureHeader.height);
	SDL_WriteU8(f, textureHeader.srcPixelFormat == GL_UNSIGNED_SHORT_1_5_5_5_REV ? 16 : 32);
	SDL_WriteU8(f, textureHeader.srcPixelFormat == GL_UNSIGNED_SHORT_1_5_5_5_REV ? 1 : 8);
	if (textureHeader.srcPixelFormat == GL_RGBA)
	{
		for (int i = 0; i < textureHeader.bufferSize; i += 4)
		{
			SDL_WriteU8(f, ((uint8_t*)texturePixels)[i+2]);
			SDL_WriteU8(f, ((uint8_t*)texturePixels)[i+1]);
			SDL_WriteU8(f, ((uint8_t*)texturePixels)[i+0]);
			SDL_WriteU8(f, ((uint8_t*)texturePixels)[i+3]);
		}
	}
	else
	{
		SDL_RWwrite(f, texturePixels, textureHeader.bufferSize, 1);
	}
	const char* footer = "\0\0\0\0\0\0\0\0TRUEVISION-XFILE.\0";
	SDL_RWwrite(f, footer, sizeof(footer), 1);
	SDL_RWclose(f);
	printf("Wrote %s (pixel format 0x%x)\n", path, textureHeader.srcPixelFormat);
#endif
}


/******************* READ GROUP ***********************/
//
// Called when GROUPSTART tag is found.  There must be a matching GROUPEND tag later.
//

static void ReadGroup(void)
{
MOGroupObject	*newGroup;

			/* CREATE NEW GROUP OBJECT */

	newGroup = MO_CreateNewObjectOfType(MO_TYPE_GROUP, NULL);
	GAME_ASSERT(newGroup);


		/*************************/
		/* PUSH ONTO GROUP STACK */
		/*************************/

	GAME_ASSERT(gBG3D_GroupStackIndex < (BG3D_GROUP_STACK_SIZE-1));

		/* SEE IF THIS IS FIRST GROUP */

	if (gBG3D_CurrentGroup == nil)					// no parent
	{
		gBG3D_CurrentContainer->root = newGroup;	// set container's root to this group
	}

		/* ADD TO PARENT GROUP */

	else
	{
		gBG3D_GroupStack[gBG3D_GroupStackIndex++] = gBG3D_CurrentGroup;		// push the old group onto group stack
		MO_AppendToGroup(gBG3D_CurrentGroup, newGroup);						// add new group to existing group (which creates new ref)
		MO_DisposeObjectReference(newGroup);								// nuke the extra reference
	}

	gBG3D_CurrentGroup = newGroup;				// current group == this group
}


/******************* END GROUP ***********************/
//
// Signifies the end of a GROUPSTART tag group.
//

static void EndGroup(void)
{
	gBG3D_GroupStackIndex--;

	GAME_ASSERT(gBG3D_GroupStackIndex >= 0);					// stack mustn't be empty

	gBG3D_CurrentGroup = gBG3D_GroupStack[gBG3D_GroupStackIndex]; // get previous group off of stack
}

#pragma mark -

/******************* READ NEW GEOMETRY ***********************/

static MetaObjectPtr ReadNewGeometry(short refNum)
{
BG3DGeometryHeader	geoHeader;
MetaObjectPtr		newObj;

			/* READ GEOMETRY HEADER */

	geoHeader.type					= FSReadBEULong(refNum);
	geoHeader.numMaterials			= FSReadBEULong(refNum);
	for (int i = 0; i < MAX_MULTITEXTURE_LAYERS; i++)
		geoHeader.layerMaterialNum[i] = FSReadBEULong(refNum);
	geoHeader.flags					= FSReadBEULong(refNum);
	geoHeader.numPoints				= FSReadBEULong(refNum);
	geoHeader.numTriangles			= FSReadBEULong(refNum);
	SetFPos(refNum, fsFromMark, sizeof(geoHeader.reserved));


		/******************************/
		/* CREATE NEW GEOMETRY OBJECT */
		/******************************/

	GAME_ASSERT(geoHeader.type == BG3D_GEOMETRYTYPE_VERTEXELEMENTS);

	newObj = ReadVertexElementsGeometry(&geoHeader);

	return(newObj);
}


/*************** READ VERTEX ELEMENTS GEOMETRY *******************/
//
//
//

static MetaObjectPtr ReadVertexElementsGeometry(BG3DGeometryHeader *header)
{
MOVertexArrayData vertexArrayData;

			/* SETUP DATA */

	vertexArrayData.numMaterials 	= header->numMaterials;
	vertexArrayData.numPoints 		= header->numPoints;
	vertexArrayData.numTriangles 	= header->numTriangles;
	vertexArrayData.points 			= nil;							// these arrays havnt been read in yet
	vertexArrayData.normals 		= nil;

	for (int i = 0; i < MAX_MATERIAL_LAYERS; i++)
		vertexArrayData.uvs[i]	 		= nil;

	vertexArrayData.colorsByte 		= nil;
	vertexArrayData.colorsFloat		= nil;
	vertexArrayData.triangles		= nil;

	vertexArrayData.bBox.isEmpty 	= true;							// no bounding box assigned yet
	vertexArrayData.bBox.min.x =
	vertexArrayData.bBox.min.y =
	vertexArrayData.bBox.min.z =
	vertexArrayData.bBox.max.x =
	vertexArrayData.bBox.max.y =
	vertexArrayData.bBox.max.z = 0.0;


	/* SETUP MATERIAL LIST */
	//
	// These start as illegal references.  The ref count is incremented during the Object Creation function.
	//

	for (int i = 0; i < vertexArrayData.numMaterials; i++)
	{
		int	materialNum = header->layerMaterialNum[i];

		vertexArrayData.materials[i] = gBG3D_CurrentContainer->materials[materialNum];
	}


		/* CREATE THE NEW GEO OBJECT */

	gBG3D_CurrentGeometryObj = MO_CreateNewObjectOfType(MO_TYPE_VERTEXARRAY, &vertexArrayData);

	return(gBG3D_CurrentGeometryObj);
}


/******************* READ VERTEX ARRAY *************************/

static void ReadVertexArray(short refNum)
{
long				count;
int					numPoints;
MOVertexArrayData	*data;
OGLPoint3D			*pointList;

	data = &gBG3D_CurrentGeometryObj->objectData;					// point to geometry data
	GAME_ASSERT(!data->points);										// points mustn't be assigned yet

	numPoints = data->numPoints;									// get # points to expect to read

	count = sizeof(OGLPoint3D) * numPoints;							// calc size of data to read
	pointList = AllocPtrClear(count);								// alloc buffer to hold points
	GAME_ASSERT(pointList);

	for (int i = 0; i < numPoints; i++)
	{
		pointList[i].x = FSReadBEFloat(refNum);
		pointList[i].y = FSReadBEFloat(refNum);
		pointList[i].z = FSReadBEFloat(refNum);
	}

	data->points = pointList;										// assign point array to geometry header
}


/******************* READ NORMAL ARRAY *************************/

static void ReadNormalArray(short refNum)
{
long				count;
int					numPoints;
MOVertexArrayData	*data;
OGLVector3D			*normalList;

	data = &gBG3D_CurrentGeometryObj->objectData;					// point to geometry data
	numPoints = data->numPoints;									// get # normals to expect to read

	count = sizeof(OGLVector3D) * numPoints;						// calc size of data to read
	normalList = AllocPtrClear(count);								// alloc buffer to hold normals
	GAME_ASSERT(normalList);

	for (int i = 0; i < numPoints; i++)
	{
		normalList[i].x = FSReadBEFloat(refNum);
		normalList[i].y = FSReadBEFloat(refNum);
		normalList[i].z = FSReadBEFloat(refNum);
	}

	data->normals = normalList;										// assign normal array to geometry header
}


/******************* READ UV ARRAY *************************/

static void ReadUVArray(short refNum)
{
long				count;
int					numPoints;
MOVertexArrayData	*data;
OGLTextureCoord		*uvList;

	data = &gBG3D_CurrentGeometryObj->objectData;					// point to geometry data
	numPoints = data->numPoints;									// get # uv's to expect to read

	count = sizeof(OGLTextureCoord) * numPoints;					// calc size of data to read
	uvList = AllocPtrClear(count);									// alloc buffer to hold uv's
	GAME_ASSERT(uvList);

	for (int i = 0; i < numPoints; i++)
	{
		uvList[i].u = FSReadBEFloat(refNum);
		uvList[i].v = FSReadBEFloat(refNum);
	}

	data->uvs[0] = uvList;											// assign uv array to geometry header
}


/******************* READ VERTEX COLOR ARRAY *************************/

static void ReadVertexColorArray(short refNum)
{
long				count,i;
int					numPoints;
MOVertexArrayData	*data;
OGLColorRGBA_Byte	*colorList;
OGLColorRGBA		*colorsF;

	data = &gBG3D_CurrentGeometryObj->objectData;					// point to geometry data
	numPoints = data->numPoints;									// get # colors to expect to read

	count = sizeof(OGLColorRGBA_Byte) * numPoints;					// calc size of data to read
	colorList = AllocPtrClear(count);								// alloc buffer to hold data
	GAME_ASSERT(colorList);

	FSRead(refNum, &count, (Ptr) colorList);						// read the data

	data->colorsByte = colorList;									// assign color array to geometry header

		/* NOW ALSO CREATE COLOR ARRAY IN FLOAT FORMAT */
		//
		// Models have colors in both byte and float formats because
		// it is faster to render with Bytes if no lighting, but faster with floats if doing lights
		//

	colorsF = AllocPtrClear(sizeof(OGLColorRGBA) * numPoints);
	GAME_ASSERT(colorsF);

	data->colorsFloat = colorsF;									// assign color array to geometry header

	for (i = 0; i < numPoints; i++)									// copy & convert bytes to floats
	{
		colorsF[i].r = (float)(colorList[i].r) / 255.0f;
		colorsF[i].g = (float)(colorList[i].g) / 255.0f;
		colorsF[i].b = (float)(colorList[i].b) / 255.0f;
		colorsF[i].a = (float)(colorList[i].a) / 255.0f;
	}

}


/******************* READ TRIANGLE ARRAY *************************/

static void ReadTriangleArray(short refNum)
{
long				count;
int					numTriangles;
MOVertexArrayData	*data;
MOTriangleIndecies	*triList;

	data = &gBG3D_CurrentGeometryObj->objectData;					// point to geometry data
	numTriangles = data->numTriangles;								// get # triangles expect to read

	count = sizeof(MOTriangleIndecies) * numTriangles;				// calc size of data to read
	triList = AllocPtrClear(count);									// alloc buffer to hold data
	GAME_ASSERT(triList);

	for (int i = 0; i < numTriangles; i++)
	{
		triList[i].vertexIndices[0] = FSReadBEULong(refNum);
		triList[i].vertexIndices[1] = FSReadBEULong(refNum);
		triList[i].vertexIndices[2] = FSReadBEULong(refNum);
	}

	data->triangles = triList;										// assign triangle array to geometry header
}


/******************* READ BOUNDING BOX *************************/

static void ReadBoundingBox(short refNum)
{
MOVertexArrayData	*data;

	data = &gBG3D_CurrentGeometryObj->objectData;					// point to geometry data

	data->bBox.min.x = FSReadBEFloat(refNum);
	data->bBox.min.y = FSReadBEFloat(refNum);
	data->bBox.min.z = FSReadBEFloat(refNum);

	data->bBox.max.x = FSReadBEFloat(refNum);
	data->bBox.max.y = FSReadBEFloat(refNum);
	data->bBox.max.z = FSReadBEFloat(refNum);

	data->bBox.isEmpty = FSReadByte(refNum);

	SetFPos(refNum, fsFromMark, 3);		// padding
}




#pragma mark -


/***************** INIT BG3D CONTAINER *********************/
//
// The container is just a header that tracks all of the crap we
// read from a BG3D file.
//

static void InitBG3DContainer(void)
{
MOGroupObject	*rootGroup;

	gBG3D_CurrentContainer = AllocPtr(sizeof(BG3DFileContainer));
	GAME_ASSERT(gBG3D_CurrentContainer);

	gBG3D_CurrentContainer->numMaterials =	0;			// no materials yet


			/* CREATE NEW GROUP OBJECT */

	rootGroup = MO_CreateNewObjectOfType(MO_TYPE_GROUP, NULL);
	GAME_ASSERT(rootGroup);

	gBG3D_CurrentContainer->root 	= rootGroup;		// root is an empty group
	gBG3D_CurrentGroup 				= rootGroup;
}



/***************** PRELOAD TEXTURE MATERIALS ***********************/
//
// Called when we're done importing the file.  It scans all of the materials
// that were loaded and uploads all of the textures to OpenGL.
//

static void PreLoadTextureMaterials(void)
{
int					i, num,w,h;
MOMaterialObject	*mat;
MOMaterialData		*matData;
void				*pixels;

	num = gBG3D_CurrentContainer->numMaterials;


	for (i = 0; i < num; i++)
	{
		mat = gBG3D_CurrentContainer->materials[i];
		matData = &mat->objectData;

		if (matData->numMipmaps > 0)							// see if has textures
		{

				/* GET TEXTURE INFO */

			pixels 		= matData->texturePixels[0];			// get ptr to pixel buffer
			w 			= matData->width;						// get width
			h 			= matData->height;						// get height


				/********************/
				/* LOAD INTO OPENGL */
				/********************/

					/* DATA IS 16-BIT PACKED PIXEL FORMAT */

			if (matData->pixelSrcFormat == GL_UNSIGNED_SHORT_1_5_5_5_REV)
			{
				matData->textureName[0] = OGL_TextureMap_Load(pixels, w, h, GL_BGRA_EXT, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV); // load 16 as 16
			}

					/* CONVERT 24BIT TO 16-BIT */

			else
			if ((matData->pixelSrcFormat == GL_RGB) && (matData->pixelDstFormat == GL_RGB5_A1))	// see if convert 24 to 16-bit
			{
				uint16_t	*buff = (uint16_t *)AllocPtr(w*h*2);				// alloc buff for 16-bit texture

				ConvertTexture24To16(pixels, buff, w, h);
				matData->textureName[0] = OGL_TextureMap_Load( buff, w, h, GL_BGRA_EXT, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV); // load 16 as 16

				SafeDisposePtr((Ptr)buff);							// dispose buff

			}

					/* USE IT AS IT IS */
			else
				matData->textureName[0] = OGL_TextureMap_Load( pixels, w, h, matData->pixelSrcFormat, matData->pixelDstFormat, GL_UNSIGNED_BYTE);


			/* DISPOSE ORIGINAL PIXELS */

			SafeDisposePtr(pixels);
			matData->texturePixels[0] = nil;
		}

	}
}


/*********************** CONVERT TEXTURE; 24 TO 16 ***********************************/

void	ConvertTexture24To16(uint8_t *srcBuff24, uint16_t *destBuff16, int width, int height)
{
int		x,y,h;
uint32_t	pixel;
uint32_t	r,g,b;

	for (y = 0; y < height; y++)
	{
		h = 0;
		for (x = 0; x < width; x++)
		{
			r = srcBuff24[h++];				// get red
			g = srcBuff24[h++];			// get green
			b = srcBuff24[h++];			// get blue

			pixel = (r >> 3) << 10;
			pixel |= (g >> 3) << 5;
			pixel |= (b >> 3);

			destBuff16[x] = pixel | 0x8000;
		}
		srcBuff24 += width * 3;
		destBuff16 += width;
	}
}


#pragma mark -

/******************* DISPOSE ALL BG3D CONTAINERS ****************/

void DisposeAllBG3DContainers(void)
{
int	i;

	for (i = 0; i < MAX_BG3D_GROUPS; i++)
	{
		if (gBG3DContainerList[i])
			DisposeBG3DContainer(i);
	}
}


/************************** DISPOSE BG3D *****************************/

void DisposeBG3DContainer(int groupNum)
{
BG3DFileContainer	*file = gBG3DContainerList[groupNum];			// point to this file's container object
int					i;

	if (file == nil)												// see if already gone
		return;

			/* DISPOSE OF ALL MATERIALS */

	for (i = 0; i < file->numMaterials; i++)
	{
		MO_DisposeObjectReference(file->materials[i]);
	}


		/* DISPOSE OF ROOT OBJECT/GROUP */

	MO_DisposeObjectReference(file->root);


		/* FREE THE CONTAINER'S MEMORY */

	SafeDisposePtr((Ptr)gBG3DContainerList[groupNum]);
	gBG3DContainerList[groupNum] = nil;								// its gone
}

#pragma mark -



/*************** BG3D:  SET CONTAINER MATERIAL FLAGS *********************/
//
// Sets the material flags for this object's vertex array
//
// geometryNum, -1 == all
//

void BG3D_SetContainerMaterialFlags(short group, short type, short geometryNum, uint32_t flags)
{
MOVertexArrayObject	*mo;
MOVertexArrayData	*va;
MOMaterialObject	*mat;

	mo = gBG3DGroupList[group][type];			// point to this model


				/****************/
				/* GROUP OBJECT */
				/****************/

	if (mo->objectHeader.type == MO_TYPE_GROUP)											// see if need to go into group
	{
		MOGroupObject	*groupObj = (MOGroupObject *)mo;

		GAME_ASSERT(geometryNum < groupObj->objectData.numObjectsInGroup);				// make sure # is valid

				/* POINT TO 1ST GEOMETRY IN THE GROUP */

		if (geometryNum == -1)																// if -1 then assign to all textures for this model
		{
			for (int i = 0; i < groupObj->objectData.numObjectsInGroup; i++)
			{
				mo = (MOVertexArrayObject *)groupObj->objectData.groupContents[i];

				GAME_ASSERT(mo->objectHeader.type == MO_TYPE_VERTEXARRAY);

				va = &mo->objectData;								// point to vertex array data
				GAME_ASSERT(va->numMaterials > 0);					// make sure there are materials

				mat = va->materials[0];						// get pointer to material
				mat->objectData.flags |= flags;						// set flags
			}
		}
		else
		{
			mo = (MOVertexArrayObject *)(groupObj->objectData.groupContents[geometryNum]);	// point to the desired geometry #
			goto setit;
		}
	}

			/* NOT A GROUNP, SO ASSUME GEOMETRY */
	else
	{
setit:
		va = &mo->objectData;								// point to vertex array data
		GAME_ASSERT(va->numMaterials > 0);					// make sure there are materials

		mat = va->materials[0];								// get pointer to material
		mat->objectData.flags |= flags;						// set flags
	}
}



/*************** BG3D: SPHERE MAP GEOMETRY MATERIAL *********************/
//
// Set the appropriate flags on a geometry's matrial to be a sphere map
//

void BG3D_SphereMapGeomteryMaterial(short group, short type, short geometryNum, uint16_t combineMode, uint16_t envMapNum)
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
				mo = (MOVertexArrayObject *)groupObj->objectData.groupContents[i];
				SetSphereMapInfoOnVertexArrayObject(mo, combineMode, envMapNum);
			}
		}
		else
		{
			mo = (MOVertexArrayObject *)groupObj->objectData.groupContents[geometryNum];	// point to the desired geometry #
			SetSphereMapInfoOnVertexArrayObject(mo, combineMode, envMapNum);
		}
	}

			/* NOT A GROUNP, SO ASSUME GEOMETRY */
	else
	{
		SetSphereMapInfoOnVertexArrayObject(mo, combineMode, envMapNum);
	}
}


/*************** SET SPHERE MAP INFO ON VERTEX ARRAY OBJECT *********************/

void SetSphereMapInfoOnVertexArrayObject(MOVertexArrayObject *mo, uint16_t combineMode, uint16_t envMapNum)
{
MOVertexArrayData	*va;
MOMaterialObject	*mat;


	GAME_ASSERT(mo->objectHeader.type == MO_TYPE_VERTEXARRAY);

	va = &mo->objectData;														// point to vertex array data

	mat = va->materials[0];														// get pointer to material
	mat->objectData.flags |= BG3D_MATERIALFLAG_MULTITEXTURE;					// set flags for multi-texture

	mat->objectData.multiTextureMode	= MULTI_TEXTURE_MODE_REFLECTIONSPHERE;	// set type of multi-texturing
	mat->objectData.multiTextureCombine	= combineMode;							// set combining mode
	mat->objectData.envMapNum			= envMapNum;							// set sphere map texture # to use
}


/******************* SET SPHERE MAP INFO ON VERTEX ARRAY DATA ********************/

void SetSphereMapInfoOnVertexArrayData(MOVertexArrayData *va, uint16_t combineMode, uint16_t envMapNum)
{
MOMaterialObject	*mat;


	mat = va->materials[0];														// get pointer to material
	mat->objectData.flags |= BG3D_MATERIALFLAG_MULTITEXTURE;					// set flags for multi-texture

	mat->objectData.multiTextureMode	= MULTI_TEXTURE_MODE_REFLECTIONSPHERE;	// set type of multi-texturing
	mat->objectData.multiTextureCombine	= combineMode;							// set combining mode
	mat->objectData.envMapNum			= envMapNum;							// set sphere map texture # to use
}

/******************* SET SPHERE MAP INFO ON MAPTERIAL OBJECT ********************/

void SetSphereMapInfoOnMaterialObject(MOMaterialObject *mat, uint16_t combineMode, uint16_t envMapNum)
{
	GAME_ASSERT(mat->objectHeader.type == MO_TYPE_MATERIAL);

	mat->objectData.flags |= BG3D_MATERIALFLAG_MULTITEXTURE;					// set flags for multi-texture

	mat->objectData.multiTextureMode	= MULTI_TEXTURE_MODE_REFLECTIONSPHERE;	// set type of multi-texturing
	mat->objectData.multiTextureCombine	= combineMode;							// set combining mode
	mat->objectData.envMapNum			= envMapNum;							// set sphere map texture # to use
}
