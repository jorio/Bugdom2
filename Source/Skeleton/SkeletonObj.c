/****************************/
/*   	SKELETON.C    	    */
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

static SkeletonObjDataType *MakeNewSkeletonBaseData(short sourceSkeletonNum);
static void DisposeSkeletonDefinitionMemory(SkeletonDefType *skeleton);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

static SkeletonDefType		*gLoadedSkeletonsList[MAX_SKELETON_TYPES];

static short	    gNumDecomposedTriMeshesInSkeleton[MAX_SKELETON_TYPES];
MOVertexArrayData	**gLocalTriMeshesOfSkelType = nil;


/**************** INIT SKELETON MANAGER *********************/

void InitSkeletonManager(void)
{
short	i;

	CalcAccelerationSplineCurve();									// calc accel curve

	for (i =0; i < MAX_SKELETON_TYPES; i++)
		gLoadedSkeletonsList[i] = nil;

		/* ALLOCATE LOCAL TRIMESHES FOR ALL SKELETON TYPES */

	Alloc_2d_array(MOVertexArrayData, gLocalTriMeshesOfSkelType, MAX_SKELETON_TYPES, MAX_DECOMPOSED_TRIMESHES);
}


/******************** LOAD A SKELETON ****************************/

void LoadASkeleton(Byte num)
{
short	i,numDecomp;

	if (num >= MAX_SKELETON_TYPES)
		DoFatalAlert("LoadASkeleton: MAX_SKELETON_TYPES exceeded!");

	if (gLoadedSkeletonsList[num] == nil)					// check if already loaded
		gLoadedSkeletonsList[num] = LoadSkeletonFile(num);


		/* MAKE LOCAL COPY OF DECOMPOSED TRIMESH */
		//
		// NOTE:  gLocalTriMeshesOfSkelType is the transformed version which is actually submitted and rendered.
		//

	numDecomp = gLoadedSkeletonsList[num]->numDecomposedTriMeshes;
	gNumDecomposedTriMeshesInSkeleton[num] = numDecomp;
	for (i=0; i < numDecomp; i++)
		MO_DuplicateVertexArrayData(&gLoadedSkeletonsList[num]->decomposedTriMeshes[i],&gLocalTriMeshesOfSkelType[num][i]);

}





/****************** FREE SKELETON FILE **************************/
//
// Disposes of all memory used by a skeleton file (from File.c)
//

void FreeSkeletonFile(Byte skeletonType)
{
short	i;

	if (gLoadedSkeletonsList[skeletonType])										// make sure this really exists
	{
		for (i=0; i < gNumDecomposedTriMeshesInSkeleton[skeletonType]; i++)		// dispose of the local copies of the decomposed trimeshes
		{
			MO_DeleteObjectInfo_Geometry_VertexArray(&gLocalTriMeshesOfSkelType[skeletonType][i]);	// delete the data
		}

		DisposeSkeletonDefinitionMemory(gLoadedSkeletonsList[skeletonType]);	// free skeleton data
		gLoadedSkeletonsList[skeletonType] = nil;
	}
}


/*************** FREE ALL SKELETON FILES ***************************/
//
// Free's all except for the input type (-1 == none to skip)
//

void FreeAllSkeletonFiles(short skipMe)
{
short	i;

	for (i = 0; i < MAX_SKELETON_TYPES; i++)
	{
		if (i != skipMe)
	 		FreeSkeletonFile(i);
	}
}

#pragma mark -

/***************** MAKE NEW SKELETON OBJECT *******************/
//
// This routine simply initializes the blank object.
// The function CopySkeletonInfoToNewSkeleton actually attaches the specific skeleton
// file to this ObjNode.
//

ObjNode	*MakeNewSkeletonObject(NewObjectDefinitionType *newObjDef)
{
ObjNode	*newNode;


			/* CREATE NEW OBJECT NODE */

	newObjDef->genre = SKELETON_GENRE;
	newNode = MakeNewObject(newObjDef);
	if (newNode == nil)
		return(nil);


			/* LOAD SKELETON FILE INTO OBJECT */

	newNode->Skeleton = MakeNewSkeletonBaseData(newObjDef->type); 			// alloc & set skeleton data
	if (newNode->Skeleton == nil)
		DoFatalAlert("MakeNewSkeletonObject: MakeNewSkeletonBaseData == nil");

	UpdateObjectTransforms(newNode);


			/*  SET INITIAL DEFAULT POSITION */

	SetSkeletonAnim(newNode->Skeleton, newObjDef->animNum);
	UpdateSkeletonAnimation(newNode);
	UpdateSkinnedGeometry(newNode);								// prime the trimesh

	newNode->BoundingSphereRadius =  fabs(newNode->BBox.min.z);	// set correct bounding sphere for fence collision

	return(newNode);
}




/***************** ALLOC SKELETON DEFINITION MEMORY **********************/
//
// Allocates all of the sub-arrays for a skeleton file's definition data.
// ONLY called by ReadDataFromSkeletonFile in file.c.
//
// NOTE: skeleton has already been allocated by LoadSkeleton!!!
//

void AllocSkeletonDefinitionMemory(SkeletonDefType *skeleton)
{
long	numAnims,numJoints;

	numJoints = skeleton->NumBones;											// get # joints in skeleton
	numAnims = skeleton->NumAnims;											// get # anims in skeleton

				/***************************/
				/* ALLOC ANIM EVENTS LISTS */
				/***************************/

	skeleton->NumAnimEvents = (Byte *)AllocPtr(sizeof(Byte)*numAnims);		// array which holds # events for each anim
	if (skeleton->NumAnimEvents == nil)
		DoFatalAlert("Not enough memory to alloc NumAnimEvents");

	Alloc_2d_array(AnimEventType, skeleton->AnimEventsList, numAnims, MAX_ANIM_EVENTS);

			/* ALLOC BONE INFO */

	skeleton->Bones = (BoneDefinitionType *)AllocPtr(sizeof(BoneDefinitionType)*numJoints);
	if (skeleton->Bones == nil)
		DoFatalAlert("Not enough memory to alloc Bones");


		/* ALLOC DECOMPOSED DATA */

	skeleton->decomposedPointList = (DecomposedPointType *)AllocPtr(sizeof(DecomposedPointType)*MAX_DECOMPOSED_POINTS);
	if (skeleton->decomposedPointList == nil)
		DoFatalAlert("Not enough memory to alloc decomposedPointList");

	skeleton->decomposedNormalsList = (OGLVector3D *)AllocPtr(sizeof(OGLVector3D)*MAX_DECOMPOSED_NORMALS);
	if (skeleton->decomposedNormalsList == nil)
		DoFatalAlert("Not enough memory to alloc decomposedNormalsList");


}


/*************** DISPOSE SKELETON DEFINITION MEMORY ***************************/
//
// Disposes of all alloced memory (from above) used by a skeleton file definition.
//

static void DisposeSkeletonDefinitionMemory(SkeletonDefType *skeleton)
{
short	j,numJoints;

	if (skeleton == nil)
		return;

	numJoints = skeleton->NumBones;

			/* NUKE THE SKELETON BONE POINT & NORMAL INDEX ARRAYS */

	for (j=0; j < numJoints; j++)
	{
		if (skeleton->Bones[j].pointList)
			SafeDisposePtr((Ptr)skeleton->Bones[j].pointList);
		if (skeleton->Bones[j].normalList)
			SafeDisposePtr((Ptr)skeleton->Bones[j].normalList);
	}
	SafeDisposePtr((Ptr)skeleton->Bones);									// free bones array
	skeleton->Bones = nil;

				/* DISPOSE ANIM EVENTS LISTS */

	SafeDisposePtr((Ptr)skeleton->NumAnimEvents);

	Free_2d_array(skeleton->AnimEventsList);


			/* DISPOSE JOINT INFO */

	for (j=0; j < numJoints; j++)
	{
		Free_2d_array(skeleton->JointKeyframes[j].keyFrames);		// dispose 2D array of keyframe data

		skeleton->JointKeyframes[j].keyFrames = nil;
	}

			/* DISPOSE DECOMPOSED DATA ARRAYS */

//	for (j = 0; j < skeleton->numDecomposedTriMeshes; j++)			// first dispose of the trimesh data in there
//	{
//		MO_DisposeObject_Geometry_VertexArray(&skeleton->decomposedTriMeshes[j]);		// dispose of material refs
//		MO_DeleteObjectInfo_Geometry_VertexArray(&skeleton->decomposedTriMeshes[j]);	// free the arrays
//	}

	if (skeleton->decomposedPointList)
	{
		SafeDisposePtr((Ptr)skeleton->decomposedPointList);
		skeleton->decomposedPointList = nil;
	}

	if (skeleton->decomposedNormalsList)
	{
		SafeDisposePtr((Ptr)skeleton->decomposedNormalsList);
		skeleton->decomposedNormalsList = nil;
	}

			/* DISPOSE OF MASTER DEFINITION BLOCK */

	SafeDisposePtr((Ptr)skeleton);
}

#pragma mark -


/****************** MAKE NEW SKELETON OBJECT DATA *********************/
//
// Allocates & inits the Skeleton data for an ObjNode.
//

static SkeletonObjDataType *MakeNewSkeletonBaseData(short sourceSkeletonNum)
{
SkeletonDefType		*skeletonDefPtr;
SkeletonObjDataType	*skeletonData;
int					i;


	skeletonDefPtr = gLoadedSkeletonsList[sourceSkeletonNum];				// get ptr to source skeleton definition info
	if (skeletonDefPtr == nil)
	{
		DoFatalAlert("MakeNewSkeletonBaseData: Skeleton data %d isn't loaded!", sourceSkeletonNum);
	}


			/* ALLOC MEMORY FOR NEW SKELETON OBJECT DATA STRUCTURE */

	skeletonData = (SkeletonObjDataType *)AllocPtrClear(sizeof(SkeletonObjDataType));
	if (skeletonData == nil)
		DoFatalAlert("MakeNewSkeletonBaseData: Cannot alloc new SkeletonObjDataType");


			/* INIT NEW SKELETON */

	skeletonData->skeletonDefinition = skeletonDefPtr;						// point to source animation data
	skeletonData->AnimSpeed = 1.0;
	skeletonData->JointsAreGlobal = false;

	for (i = 0; i < MAX_DECOMPOSED_TRIMESHES; i++)
		skeletonData->overrideTexture[i] = nil;									// assume no override texture.. yet.


			/****************************************/
			/* MAKE COPY OF TRIMESHES FOR LOCAL USE */
			/****************************************/


	return(skeletonData);
}


/************************ FREE SKELETON BASE DATA ***************************/

void FreeSkeletonBaseData(SkeletonObjDataType *data)
{

			/* FREE THE SKELETON DATA */

	SafeDisposePtr((Ptr)data);
}



#pragma mark -

/*************************** DRAW SKELETON ******************************/

void DrawSkeleton(ObjNode *theNode)
{
short		i,numTriMeshes;
short			skelType;
MOMaterialObject	*overrideTexture, *oldTexture = nil;

	UpdateSkinnedGeometry(theNode);													// update skeleton geometry
	numTriMeshes = theNode->Skeleton->skeletonDefinition->numDecomposedTriMeshes;
	skelType = theNode->Type;


	for (i = 0; i < numTriMeshes; i++)												// submit each trimesh of it
	{
		overrideTexture = theNode->Skeleton->overrideTexture[i];					// get any override texture ref (illegal ref)
		if (overrideTexture)														// set override texture
		{
			if (gLocalTriMeshesOfSkelType[skelType][i].numMaterials > 0)
			{
				oldTexture = gLocalTriMeshesOfSkelType[skelType][i].materials[0];		// get the real texture for this mesh
				gLocalTriMeshesOfSkelType[skelType][i].materials[0] = overrideTexture;	// set the override one temporarily
			}
		}

		MO_DrawGeometry_VertexArray(&gLocalTriMeshesOfSkelType[skelType][i]);

		if (overrideTexture && oldTexture)											// see if need to set texture back to normal
			gLocalTriMeshesOfSkelType[skelType][i].materials[0] = oldTexture;
	}
}














