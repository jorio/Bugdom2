/****************************/
/*      FILE ROUTINES       */
/* By Brian Greenstone      */
/* (c)2002 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"
#include <time.h>


/****************************/
/*    PROTOTYPES            */
/****************************/

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType);
static void ReadDataFromPlayfieldFile(FSSpec *specPtr);
static void	ConvertTexture16To16(uint16_t *textureBuffer, int width, int height);

static void ReadDataFromTunnelFile(FSSpec *tunnelSpec, FSSpec *bg3dSpec, short fRefNum);


static Ptr TileImage(Ptr imageBank, int col, int row);

static inline void Blit16(
		const char*			src,
		int					srcWidth,
		int					srcHeight,
		int					srcRectX,
		int					srcRectY,
		int					srcRectWidth,
		int					srcRectHeight,
		char*				dst,
		int 				dstWidth,
		int 				dstHeight,
		int					dstRectX,
		int					dstRectY);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	SKELETON_FILE_VERS_NUM	0x0110			// v1.1


		/* PLAYFIELD HEADER */

typedef struct
{
	NumVersion	version;							// version of file
	int32_t		numItems;							// # items in map
	int32_t		mapWidth;							// width of map
	int32_t		mapHeight;							// height of map
	float		tileSize;							// 3D unit size of a tile
	float		minY,maxY;							// min/max height values
	int32_t		numSplines;							// # splines
	int32_t		numFences;							// # fences
	int32_t		numUniqueSuperTiles;				// # unique supertile
	int32_t		numWaterPatches;                    // # water patches
	int32_t		numCheckpoints;						// # checkpoints
	int32_t		unused[10];
}PlayfieldHeaderType;


		/* FENCE STRUCTURE IN FILE */
		//
		// note: we copy this data into our own fence list
		//		since the game uses a slightly different
		//		data structure.
		//

typedef	struct
{
	int32_t		x,z;
}FencePointType;


typedef struct
{
	uint16_t		type;				// type of fence
	int16_t			numNubs;			// # nubs in fence
	int32_t			junk;			// handle to nub list
	Rect			bBox;				// bounding box of fence area
}FileFenceDefType;


		/* TUNNEL HEADER */

typedef struct
{
	NumVersion	version;							// version of file
	Boolean		fullPipe;							// flag true if 360 degree pipe
	int32_t		numNubs;							// # spline nubs
	int32_t		numSplinePoints;					// # points in giant spline
	int32_t		numSections;						// # pieces of geometry to load
	int32_t		numItems;							// # items on spline
	int32_t		unused[16];
}TunnelFileHeaderType;


/**********************/
/*     VARIABLES      */
/**********************/


float	g3DTileSize, g3DMinY, g3DMaxY;



/******************* LOAD SKELETON *******************/
//
// Loads a skeleton file & creates storage for it.
//
// NOTE: Skeleton types 0..NUM_CHARACTERS-1 are reserved for player character skeletons.
//		Skeleton types NUM_CHARACTERS and over are for other skeleton entities.
//
// OUTPUT:	Ptr to skeleton data
//

SkeletonDefType *LoadSkeletonFile(short skeletonType)
{
QDErr		iErr;
short		fRefNum;
FSSpec		fsSpec;
SkeletonDefType	*skeleton;
const Str63	fileNames[MAX_SKELETON_TYPES] =
{
	":Skeletons:Skip_Explore.skeleton",
	":Skeletons:Skip_Tunnel.skeleton",
	":Skeletons:Skip_Title.skeleton",
	":Skeletons:Snail.skeleton",
	":Skeletons:Gnome.skeleton",
	":Skeletons:HouseFly.skeleton",
	":Skeletons:EvilPlant.skeleton",
	":Skeletons:Chipmunk.skeleton",
	":Skeletons:SnakeHead.skeleton",
	":Skeletons:BuddyBug.skeleton",
	":Skeletons:Checkpoint.skeleton",
	":Skeletons:Flea.skeleton",
	":Skeletons:Tick.skeleton",
	":Skeletons:MouseTrap.skeleton",
	":Skeletons:Mouse.skeleton",
	":Skeletons:Soldier.skeleton",
	":Skeletons:OttoToy.skeleton",
	":Skeletons:BumbleBee.skeleton",
	":Skeletons:HoboBag.skeleton",
	":Skeletons:DragonFly.skeleton",
	":Skeletons:Frog.skeleton",
	":Skeletons:Moth.skeleton",
	":Skeletons:ComputerBug.skeleton",
	":Skeletons:Roach.skeleton",
	":Skeletons:Ant.skeleton",
	":Skeletons:Fish.skeleton",
};


	if (skeletonType < MAX_SKELETON_TYPES)
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, fileNames[skeletonType], &fsSpec);
	else
		DoFatalAlert("LoadSkeleton: Unknown skeletonType!");


			/* OPEN THE FILE'S REZ FORK */

	fRefNum = FSpOpenResFile(&fsSpec,fsRdPerm);
	if (fRefNum == -1)
	{
		iErr = ResError();
		DoFatalAlert("Error %d opening Skel Rez file", iErr);
	}

	UseResFile(fRefNum);
	if (ResError())
		DoFatalAlert("Error using Rez file!");


			/* ALLOC MEMORY FOR SKELETON INFO STRUCTURE */

	skeleton = (SkeletonDefType *)AllocPtr(sizeof(SkeletonDefType));
	if (skeleton == nil)
		DoFatalAlert("Cannot alloc SkeletonInfoType");


			/* READ SKELETON RESOURCES */

	ReadDataFromSkeletonFile(skeleton, &fsSpec, skeletonType);
	PrimeBoneData(skeleton);

			/* CLOSE REZ FILE */

	CloseResFile(fRefNum);

	return(skeleton);
}


/************* READ DATA FROM SKELETON FILE *******************/
//
// Current rez file is set to the file.
//

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType)
{
Handle				hand;
int					i,k,j;
long				numJoints,numAnims,numKeyframes;
AnimEventType		*animEventPtr;
JointKeyframeType	*keyFramePtr;
SkeletonFile_Header_Type	*headerPtr;
short				version;
AliasHandle				alias;
OSErr					iErr;
FSSpec					target;
Boolean					wasChanged;
OGLPoint3D				*pointPtr;
SkeletonFile_AnimHeader_Type	*animHeaderPtr;


			/************************/
			/* READ HEADER RESOURCE */
			/************************/

	hand = GetResource('Hedr',1000);
	if (hand == nil)
		DoFatalAlert("ReadDataFromSkeletonFile: Error reading header resource!");
	headerPtr = (SkeletonFile_Header_Type *)*hand;
	version = SwizzleShort(&headerPtr->version);
	if (version != SKELETON_FILE_VERS_NUM)
		DoFatalAlert("Skeleton file has wrong version #");

	numAnims = skeleton->NumAnims = SwizzleShort(&headerPtr->numAnims);			// get # anims in skeleton
	numJoints = skeleton->NumBones = SwizzleShort(&headerPtr->numJoints);		// get # joints in skeleton
	ReleaseResource(hand);

	if (numJoints > MAX_JOINTS)										// check for overload
		DoFatalAlert("ReadDataFromSkeletonFile: numJoints > MAX_JOINTS");


				/*************************************/
				/* ALLOCATE MEMORY FOR SKELETON DATA */
				/*************************************/

	AllocSkeletonDefinitionMemory(skeleton);



		/********************************/
		/* 	LOAD THE REFERENCE GEOMETRY */
		/********************************/

	alias = (AliasHandle)GetResource(rAliasType,1000);				// alias to geometry BG3D file
	if (alias != nil)
	{
		iErr = ResolveAlias(fsSpec, alias, &target, &wasChanged);	// try to resolve alias
		if (!iErr)
			LoadBonesReferenceModel(&target,skeleton, skeletonType);
		else
			DoFatalAlert("ReadDataFromSkeletonFile: Cannot find Skeleton's BG3D file!");
		ReleaseResource((Handle)alias);
	}
	else
		DoFatalAlert("ReadDataFromSkeletonFile: file is missing the Alias resource");



		/***********************************/
		/*  READ BONE DEFINITION RESOURCES */
		/***********************************/

	for (i=0; i < numJoints; i++)
	{
		File_BoneDefinitionType	*bonePtr;
		uint16_t					*indexPtr;

			/* READ BONE DATA */

		hand = GetResource('Bone',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading Bone resource!");
		HLock(hand);
		bonePtr = (File_BoneDefinitionType *)*hand;

			/* COPY BONE DATA INTO ARRAY */

		skeleton->Bones[i].parentBone = SwizzleLong(&bonePtr->parentBone);					// index to previous bone
		skeleton->Bones[i].coord.x = SwizzleFloat(&bonePtr->coord.x);						// absolute coord (not relative to parent!)
		skeleton->Bones[i].coord.y = SwizzleFloat(&bonePtr->coord.y);
		skeleton->Bones[i].coord.z = SwizzleFloat(&bonePtr->coord.z);
		skeleton->Bones[i].numPointsAttachedToBone = SwizzleUShort(&bonePtr->numPointsAttachedToBone);		// # vertices/points that this bone has
		skeleton->Bones[i].numNormalsAttachedToBone = SwizzleUShort(&bonePtr->numNormalsAttachedToBone);	// # vertex normals this bone has
		ReleaseResource(hand);

			/* ALLOC THE POINT & NORMALS SUB-ARRAYS */

		skeleton->Bones[i].pointList = (uint16_t *)AllocPtr(sizeof(uint16_t) * (int)skeleton->Bones[i].numPointsAttachedToBone);
		if (skeleton->Bones[i].pointList == nil)
			DoFatalAlert("ReadDataFromSkeletonFile: AllocPtr/pointList failed!");

		skeleton->Bones[i].normalList = (uint16_t *)AllocPtr(sizeof(uint16_t) * (int)skeleton->Bones[i].numNormalsAttachedToBone);
		if (skeleton->Bones[i].normalList == nil)
			DoFatalAlert("ReadDataFromSkeletonFile: AllocPtr/normalList failed!");

			/* READ POINT INDEX ARRAY */

		hand = GetResource('BonP',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading BonP resource!");
		HLock(hand);
		indexPtr = (uint16_t *)(*hand);

			/* COPY POINT INDEX ARRAY INTO BONE STRUCT */

		for (j=0; j < skeleton->Bones[i].numPointsAttachedToBone; j++)
			skeleton->Bones[i].pointList[j] = SwizzleUShort(&indexPtr[j]);
		ReleaseResource(hand);


			/* READ NORMAL INDEX ARRAY */

		hand = GetResource('BonN',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading BonN resource!");
		HLock(hand);
		indexPtr = (uint16_t *)(*hand);

			/* COPY NORMAL INDEX ARRAY INTO BONE STRUCT */

		for (j=0; j < skeleton->Bones[i].numNormalsAttachedToBone; j++)
			skeleton->Bones[i].normalList[j] = SwizzleUShort(&indexPtr[j]);
		ReleaseResource(hand);

	}


		/*******************************/
		/* READ POINT RELATIVE OFFSETS */
		/*******************************/
		//
		// The "relative point offsets" are the only things
		// which do not get rebuilt in the ModelDecompose function.
		// We need to restore these manually.

	hand = GetResource('RelP', 1000);
	GAME_ASSERT(hand);
	HLock(hand);
	pointPtr = (OGLPoint3D *)*hand;

	i = (int) (GetHandleSize(hand) / (Size) sizeof(OGLPoint3D));
	if (i != skeleton->numDecomposedPoints)
		DoFatalAlert("# of points in Reference Model has changed!");
	else
		for (i = 0; i < skeleton->numDecomposedPoints; i++)
		{
			skeleton->decomposedPointList[i].boneRelPoint.x = SwizzleFloat(&pointPtr[i].x);
			skeleton->decomposedPointList[i].boneRelPoint.y = SwizzleFloat(&pointPtr[i].y);
			skeleton->decomposedPointList[i].boneRelPoint.z = SwizzleFloat(&pointPtr[i].z);
		}

	ReleaseResource(hand);


			/*********************/
			/* READ ANIM INFO   */
			/*********************/

	for (i=0; i < numAnims; i++)
	{
				/* READ ANIM HEADER */

		hand = GetResource('AnHd',1000+i);
		if (hand == nil)
			DoFatalAlert("Error getting anim header resource");
		HLock(hand);
		animHeaderPtr = (SkeletonFile_AnimHeader_Type *)*hand;

		skeleton->NumAnimEvents[i] = SwizzleShort(&animHeaderPtr->numAnimEvents);			// copy # anim events in anim
		ReleaseResource(hand);

			/* READ ANIM-EVENT DATA */

		hand = GetResource('Evnt',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading anim-event data resource!");
		animEventPtr = (AnimEventType *)*hand;
		for (j=0;  j < skeleton->NumAnimEvents[i]; j++)
		{
			skeleton->AnimEventsList[i][j] = *animEventPtr++;							// copy whole thing
			skeleton->AnimEventsList[i][j].time = SwizzleShort(&skeleton->AnimEventsList[i][j].time);	// then swizzle the 16-bit short value
		}
		ReleaseResource(hand);


			/* READ # KEYFRAMES PER JOINT IN EACH ANIM */

		hand = GetResource('NumK',1000+i);									// read array of #'s for this anim
		if (hand == nil)
			DoFatalAlert("Error reading # keyframes/joint resource!");
		for (j=0; j < numJoints; j++)
			skeleton->JointKeyframes[j].numKeyFrames[i] = (*hand)[j];
		ReleaseResource(hand);
	}


	for (j=0; j < numJoints; j++)
	{
				/* ALLOC 2D ARRAY FOR KEYFRAMES */

		Alloc_2d_array(JointKeyframeType,skeleton->JointKeyframes[j].keyFrames,	numAnims,MAX_KEYFRAMES);

		if ((skeleton->JointKeyframes[j].keyFrames == nil) || (skeleton->JointKeyframes[j].keyFrames[0] == nil))
			DoFatalAlert("ReadDataFromSkeletonFile: Error allocating Keyframe Array.");

					/* READ THIS JOINT'S KF'S FOR EACH ANIM */

		for (i=0; i < numAnims; i++)
		{
			numKeyframes = skeleton->JointKeyframes[j].numKeyFrames[i];					// get actual # of keyframes for this joint
			if (numKeyframes > MAX_KEYFRAMES)
				DoFatalAlert("Error: numKeyframes > MAX_KEYFRAMES");

					/* READ A JOINT KEYFRAME */

			hand = GetResource('KeyF',1000+(i*100)+j);
			if (hand == nil)
				DoFatalAlert("Error reading joint keyframes resource!");
			keyFramePtr = (JointKeyframeType *)*hand;
			for (k = 0; k < numKeyframes; k++)												// copy this joint's keyframes for this anim
			{
				skeleton->JointKeyframes[j].keyFrames[i][k].tick				= SwizzleLong(&keyFramePtr->tick);
				skeleton->JointKeyframes[j].keyFrames[i][k].accelerationMode	= SwizzleLong(&keyFramePtr->accelerationMode);
				skeleton->JointKeyframes[j].keyFrames[i][k].coord.x				= SwizzleFloat(&keyFramePtr->coord.x);
				skeleton->JointKeyframes[j].keyFrames[i][k].coord.y				= SwizzleFloat(&keyFramePtr->coord.y);
				skeleton->JointKeyframes[j].keyFrames[i][k].coord.z				= SwizzleFloat(&keyFramePtr->coord.z);
				skeleton->JointKeyframes[j].keyFrames[i][k].rotation.x			= SwizzleFloat(&keyFramePtr->rotation.x);
				skeleton->JointKeyframes[j].keyFrames[i][k].rotation.y			= SwizzleFloat(&keyFramePtr->rotation.y);
				skeleton->JointKeyframes[j].keyFrames[i][k].rotation.z			= SwizzleFloat(&keyFramePtr->rotation.z);
				skeleton->JointKeyframes[j].keyFrames[i][k].scale.x				= SwizzleFloat(&keyFramePtr->scale.x);
				skeleton->JointKeyframes[j].keyFrames[i][k].scale.y				= SwizzleFloat(&keyFramePtr->scale.y);
				skeleton->JointKeyframes[j].keyFrames[i][k].scale.z				= SwizzleFloat(&keyFramePtr->scale.z);

				keyFramePtr++;
			}
			ReleaseResource(hand);
		}
	}

}

#pragma mark -



/******************** LOAD PREFS **********************/
//
// Load in standard preferences
//

OSErr LoadPrefs(void)
{
OSErr		iErr = noErr;
short		refNum = 0;
FSSpec		file;
long		count;

	iErr = CheckPrefsFolder(false);
	if (iErr)
		goto err;

				/*************/
				/* READ FILE */
				/*************/

	iErr = FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, PREFS_FILE_PATH, &file);
	if (iErr)
		goto err;

	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);
	if (iErr)
		goto err;

	count = sizeof(PrefsType);
	iErr = FSRead(refNum, &count, (Ptr) &gGamePrefs);		// read data from file
	FSClose(refNum);
	if (iErr)
		goto err;

			/****************/
			/* VERIFY PREFS */
			/****************/

	if ((size_t) count < sizeof(PrefsType))
		goto err;

	if (gGamePrefs.version != CURRENT_PREFS_VERS)
		goto err;

			/* OVERWRITE NON-CONFIGURABLE BINDINGS */

	SDL_memcpy(&gGamePrefs.bindings[NUM_REMAPPABLE_NEEDS],
				&kDefaultInputBindings[NUM_REMAPPABLE_NEEDS],
				sizeof(gGamePrefs.bindings[0]) * (NUM_CONTROL_NEEDS - NUM_REMAPPABLE_NEEDS));

	return(noErr);

err:
	InitDefaultPrefs();
	return iErr;
}



/******************** SAVE PREFS **********************/

void SavePrefs(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;


	iErr = CheckPrefsFolder(true);
	if (iErr)
		return;

				/* CREATE BLANK FILE */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, PREFS_FILE_PATH, &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, kGameID, 'Pref', smSystemScript);					// create blank file
	if (iErr)
		return;

				/* OPEN FILE */

	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
		FSpDelete(&file);
		return;
	}

				/* WRITE DATA */

	count = sizeof(PrefsType);
	FSWrite(refNum, &count, (Ptr) &gGamePrefs);
	FSClose(refNum);
}


#pragma mark -

/******************* LOAD PLAYFIELD *******************/

void LoadPlayfield(FSSpec *specPtr)
{

	gDisableHiccupTimer = true;

			/* READ PLAYFIELD RESOURCES */

	ReadDataFromPlayfieldFile(specPtr);


				/* DO ADDITIONAL SETUP */

	CreateSuperTileMemoryList();				// allocate memory for the supertile geometry
	CalculateSplitModeMatrix();					// precalc the tile split mode matrix
	InitSuperTileGrid();						// init the supertile state grid

	BuildTerrainItemList();						// build list of items & find player start coords


			/* CAST ITEM SHADOWS */

	DoItemShadowCasting();
}


/********************** READ DATA FROM PLAYFIELD FILE ************************/

static void ReadDataFromPlayfieldFile(FSSpec *specPtr)
{
Handle					hand;
PlayfieldHeaderType		**header;
float					yScale;
short					fRefNum;
OSErr					iErr;

				/* MAKE SURE NO DANGLING POINTERS FROM PREVIOUS SCENE */

	GAME_ASSERT(!gSuperTileTextureGrid);
	GAME_ASSERT(!gMapYCoords);
	GAME_ASSERT(!gMapYCoordsOriginal);
	GAME_ASSERT(!gMasterItemList);
	GAME_ASSERT(!gSplineList);
	GAME_ASSERT(!gFenceList);
	GAME_ASSERT(!gWaterListHandle);
	GAME_ASSERT(!gWaterList);


				/* OPEN THE REZ-FORK */

	fRefNum = FSpOpenResFile(specPtr, fsRdPerm);
	GAME_ASSERT(fRefNum != -1);
	UseResFile(fRefNum);


			/************************/
			/* READ HEADER RESOURCE */
			/************************/

	hand = GetResource('Hedr',1000);
	GAME_ASSERT(hand);

	header = (PlayfieldHeaderType **)hand;
	gNumTerrainItems		= SwizzleLong(&(**header).numItems);
	gTerrainTileWidth		= SwizzleLong(&(**header).mapWidth);
	gTerrainTileDepth		= SwizzleLong(&(**header).mapHeight);
	g3DTileSize				= SwizzleFloat(&(**header).tileSize);
	g3DMinY					= SwizzleFloat(&(**header).minY);
	g3DMaxY					= SwizzleFloat(&(**header).maxY);
	gNumSplines				= SwizzleLong(&(**header).numSplines);
	gNumFences				= SwizzleLong(&(**header).numFences);
	gNumWaterPatches		= SwizzleLong(&(**header).numWaterPatches);
	gNumUniqueSuperTiles	= SwizzleLong(&(**header).numUniqueSuperTiles);
	gNumLineMarkers			= SwizzleLong(&(**header).numCheckpoints);

	ReleaseResource(hand);

	if ((gTerrainTileWidth % SUPERTILE_SIZE) != 0)		// terrain must be non-fractional number of supertiles in w/h
		DoFatalAlert("ReadDataFromPlayfieldFile: terrain width not a supertile multiple");
	if ((gTerrainTileDepth % SUPERTILE_SIZE) != 0)
		DoFatalAlert("ReadDataFromPlayfieldFile: terrain depth not a supertile multiple");


				/* CALC SOME GLOBALS HERE */

	gTerrainTileWidth = (gTerrainTileWidth/SUPERTILE_SIZE)*SUPERTILE_SIZE;		// round size down to nearest supertile multiple
	gTerrainTileDepth = (gTerrainTileDepth/SUPERTILE_SIZE)*SUPERTILE_SIZE;
	gTerrainUnitWidth = gTerrainTileWidth*gTerrainPolygonSize;					// calc world unit dimensions of terrain
	gTerrainUnitDepth = gTerrainTileDepth*gTerrainPolygonSize;
	gNumSuperTilesDeep = gTerrainTileDepth/SUPERTILE_SIZE;						// calc size in supertiles
	gNumSuperTilesWide = gTerrainTileWidth/SUPERTILE_SIZE;


			/*******************************/
			/* SUPERTILE RELATED RESOURCES */
			/*******************************/

			/* READ SUPERTILE GRID MATRIX */

//	if (gSuperTileTextureGrid)														// free old array
//		Free_2d_array(gSuperTileTextureGrid);
	Alloc_2d_array(short, gSuperTileTextureGrid, gNumSuperTilesDeep, gNumSuperTilesWide);

	hand = GetResource('STgd',1000);												// load grid from rez
	GAME_ASSERT(hand);

	// copy rez into 2D array
	{
		int16_t *srcShort = (int16_t *)*hand;

		for (int row = 0; row < gNumSuperTilesDeep; row++)
		for (int col = 0; col < gNumSuperTilesWide; col++)
		{
			gSuperTileTextureGrid[row][col] = SwizzleShort(srcShort++);
		}
	}
	ReleaseResource(hand);



			/* READ HEIGHT DATA MATRIX */

	yScale = gTerrainPolygonSize / g3DTileSize;											// need to scale original geometry units to game units

	if (gLevelNum == LEVEL_NUM_PARK)			// modify y scale for this level since we need more range
		yScale *= 2.0f;

	Alloc_2d_array(float, gMapYCoords, gTerrainTileDepth+1, gTerrainTileWidth+1);			// alloc 2D array for map
	Alloc_2d_array(float, gMapYCoordsOriginal, gTerrainTileDepth+1, gTerrainTileWidth+1);	// and the copy of it

	hand = GetResource('YCrd',1000);
	GAME_ASSERT(hand);
	{
		float* src = (float *)*hand;
		for (int row = 0; row <= gTerrainTileDepth; row++)
		for (int col = 0; col <= gTerrainTileWidth; col++)
		{
			gMapYCoordsOriginal[row][col] = gMapYCoords[row][col] = SwizzleFloat(src++) * yScale;
		}
	}
	ReleaseResource(hand);

				/**************************/
				/* ITEM RELATED RESOURCES */
				/**************************/

				/* READ ITEM LIST */

	hand = GetResource('Itms',1000);
	GAME_ASSERT(hand);

	{
		TerrainItemEntryType   *rezItems;

		DetachResource(hand);							// lets keep this data around
		HLockHi(hand);									// LOCK this one because we have the lookup table into this
		gMasterItemList = (TerrainItemEntryType **)hand;
		rezItems = (TerrainItemEntryType *)*hand;

				/* CONVERT COORDINATES */

		for (int i = 0; i < gNumTerrainItems; i++)
		{
			(*gMasterItemList)[i].x = SwizzleULong(&rezItems[i].x) * gMapToUnitValue;								// convert coordinates
			(*gMasterItemList)[i].y = SwizzleULong(&rezItems[i].y) * gMapToUnitValue;

			(*gMasterItemList)[i].type = SwizzleUShort(&rezItems[i].type);
			(*gMasterItemList)[i].parm[0] = rezItems[i].parm[0];
			(*gMasterItemList)[i].parm[1] = rezItems[i].parm[1];
			(*gMasterItemList)[i].parm[2] = rezItems[i].parm[2];
			(*gMasterItemList)[i].parm[3] = rezItems[i].parm[3];
			(*gMasterItemList)[i].flags = SwizzleUShort(&rezItems[i].flags);
		}
	}



			/****************************/
			/* SPLINE RELATED RESOURCES */
			/****************************/

			/* READ SPLINE LIST */

	hand = GetResource('Spln',1000);
	if (hand)
	{
		File_SplineDefType	*splinePtr = (File_SplineDefType *)*hand;

		gSplineList = (SplineDefType**) NewHandleClear(sizeof(SplineDefType) * gNumSplines);		// allocate memory for spline data

		for (int i = 0; i < gNumSplines; i++)
		{
			(*gSplineList)[i].numNubs = SwizzleShort(&splinePtr[i].numNubs);
			(*gSplineList)[i].numPoints = SwizzleLong(&splinePtr[i].numPoints);
			(*gSplineList)[i].numItems = SwizzleShort(&splinePtr[i].numItems);

			(*gSplineList)[i].bBox.top = SwizzleShort(&splinePtr[i].bBox.top);
			(*gSplineList)[i].bBox.bottom = SwizzleShort(&splinePtr[i].bBox.bottom);
			(*gSplineList)[i].bBox.left = SwizzleShort(&splinePtr[i].bBox.left);
			(*gSplineList)[i].bBox.right = SwizzleShort(&splinePtr[i].bBox.right);
		}

		ReleaseResource(hand);																// nuke the rez
	}
	else
	{
		gNumSplines = 0;
		gSplineList = nil;
	}


			/* READ SPLINE POINT LIST */

	for (int i = 0; i < gNumSplines; i++)
	{
		SplineDefType	*spline = &(*gSplineList)[i];									// point to Nth spline

		hand = GetResource('SpPt',1000+i);
		GAME_ASSERT(hand);

		SplinePointType	*ptList = (SplinePointType *)*hand;

		DetachResource(hand);
		HLockHi(hand);
		(*gSplineList)[i].pointList = (SplinePointType **)hand;

		for (int j = 0; j < spline->numPoints; j++)			// swizzle
		{
			(*spline->pointList)[j].x = SwizzleFloat(&ptList[j].x);
			(*spline->pointList)[j].z = SwizzleFloat(&ptList[j].z);
		}
	}


			/* READ SPLINE ITEM LIST */

	for (int i = 0; i < gNumSplines; i++)
	{
		SplineDefType	*spline = &(*gSplineList)[i];									// point to Nth spline

		hand = GetResource('SpIt',1000+i);
		GAME_ASSERT(hand);

		SplineItemType	*itemList = (SplineItemType *)*hand;

		DetachResource(hand);
		HLockHi(hand);
		(*gSplineList)[i].itemList = (SplineItemType **)hand;

		for (int j = 0; j < spline->numItems; j++)			// swizzle
		{
			(*spline->itemList)[j].placement = SwizzleFloat(&itemList[j].placement);
			(*spline->itemList)[j].type	= SwizzleUShort(&itemList[j].type);
			(*spline->itemList)[j].flags	= SwizzleUShort(&itemList[j].flags);
		}
	}

			/****************************/
			/* FENCE RELATED RESOURCES */
			/****************************/

			/* READ FENCE LIST */

	hand = GetResource('Fenc',1000);
	if (hand)
	{
		FileFenceDefType *inData;

		gFenceList = (FenceDefType *)AllocPtrClear(sizeof(FenceDefType) * gNumFences);	// alloc new ptr for fence data
		if (gFenceList == nil)
			DoFatalAlert("ReadDataFromPlayfieldFile: AllocPtr failed");

		inData = (FileFenceDefType *)*hand;								// get ptr to input fence list

		for (int i = 0; i < gNumFences; i++)							// copy data from rez to new list
		{
			gFenceList[i].type 		= SwizzleUShort(&inData[i].type);
			gFenceList[i].numNubs 	= SwizzleShort(&inData[i].numNubs);
			gFenceList[i].nubList 	= nil;
			gFenceList[i].sectionVectors = nil;
		}
		ReleaseResource(hand);
	}
	else
	{
		gNumFences = 0;
		gFenceList = NULL;
	}


			/* READ FENCE NUB LIST */

	for (int i = 0; i < gNumFences; i++)
	{
		hand = GetResource('FnNb',1000+i);					// get rez
		GAME_ASSERT(hand);
		HLock(hand);

		FencePointType *fileFencePoints = (FencePointType *)*hand;

		gFenceList[i].nubList = (OGLPoint3D *)AllocPtrClear(sizeof(FenceDefType) * gFenceList[i].numNubs);	// alloc new ptr for nub array
		GAME_ASSERT(gFenceList[i].nubList);

		for (int j = 0; j < gFenceList[i].numNubs; j++)		// convert x,z to x,y,z
		{
			gFenceList[i].nubList[j].x = SwizzleLong(&fileFencePoints[j].x);
			gFenceList[i].nubList[j].z = SwizzleLong(&fileFencePoints[j].z);
			gFenceList[i].nubList[j].y = 0;
		}
		ReleaseResource(hand);
	}

			/****************************/
			/* WATER RELATED RESOURCES */
			/****************************/

			/* READ WATER LIST */

	hand = GetResource('Liqd',1000);
	if (hand)
	{
		DetachResource(hand);
		HLockHi(hand);
		gWaterListHandle = (WaterDefType **)hand;
		gWaterList = *gWaterListHandle;

		for (int i = 0; i < gNumWaterPatches; i++)						// swizzle
		{
			gWaterList[i].type = SwizzleUShort(&gWaterList[i].type);
			gWaterList[i].flags = SwizzleULong(&gWaterList[i].flags);
			gWaterList[i].height = SwizzleLong(&gWaterList[i].height);
			gWaterList[i].numNubs = SwizzleShort(&gWaterList[i].numNubs);

			gWaterList[i].hotSpotX = SwizzleFloat(&gWaterList[i].hotSpotX);
			gWaterList[i].hotSpotZ = SwizzleFloat(&gWaterList[i].hotSpotZ);

			gWaterList[i].bBox.top = SwizzleShort(&gWaterList[i].bBox.top);
			gWaterList[i].bBox.bottom = SwizzleShort(&gWaterList[i].bBox.bottom);
			gWaterList[i].bBox.left = SwizzleShort(&gWaterList[i].bBox.left);
			gWaterList[i].bBox.right = SwizzleShort(&gWaterList[i].bBox.right);

			for (int j = 0; j < gWaterList[i].numNubs; j++)
			{
				gWaterList[i].nubList[j].x = SwizzleFloat(&gWaterList[i].nubList[j].x);
				gWaterList[i].nubList[j].y = SwizzleFloat(&gWaterList[i].nubList[j].y);
			}
		}
	}
	else
	{
		gNumWaterPatches = 0;
		gWaterListHandle = NULL;
		gWaterList = NULL;
	}



			/*************************/
			/* LINE MARKER RESOURCES */
			/*************************/

	if (gNumLineMarkers > 0)
	{
		GAME_ASSERT(gNumLineMarkers <= MAX_LINEMARKERS);

				/* READ CHECKPOINT LIST */

		hand = GetResource('CkPt',1000);
		if (hand)
		{
			HLock(hand);
			BlockMove(*hand, &gLineMarkerList[0], GetHandleSize(hand));
			ReleaseResource(hand);

						/* CONVERT COORDINATES */

			for (int i = 0; i < gNumLineMarkers; i++)
			{
				LineMarkerDefType	*lm = &gLineMarkerList[i];

				lm->infoBits = SwizzleShort(&gLineMarkerList[i].infoBits);			// swizzle data
				lm->x[0] = SwizzleFloat(&lm->x[0]);
				lm->x[1] = SwizzleFloat(&lm->x[1]);
				lm->z[0] = SwizzleFloat(&lm->z[0]);
				lm->z[1] = SwizzleFloat(&lm->z[1]);

				gLineMarkerList[i].x[0] *= gMapToUnitValue;
				gLineMarkerList[i].z[0] *= gMapToUnitValue;
				gLineMarkerList[i].x[1] *= gMapToUnitValue;
				gLineMarkerList[i].z[1] *= gMapToUnitValue;
			}
		}
		else
			gNumLineMarkers = 0;
	}



			/* CLOSE REZ FILE */

	CloseResFile(fRefNum);



		/********************************************/
		/* READ SUPERTILE IMAGE DATA FROM DATA FORK */
		/********************************************/


				/* ALLOC BUFFERS */

	int size = SQUARED(SUPERTILE_TEXMAP_SIZE) * sizeof(uint16_t);			// calc size of supertile 16-bit texture

	int seamlessCanvasSize = SQUARED(SUPERTILE_TEXMAP_SIZE + 2) * sizeof(uint16_t);		// seamless texture needs 1px border around supertile image

	Ptr imageBank = AllocPtrClear(gNumUniqueSuperTiles * size);							// all supertile images from .ter data fork
	Ptr canvas = AllocPtrClear(seamlessCanvasSize);										// we'll assemble the final supertile texture in there

	SDL_memset(gSuperTileTextureObjects, 0, sizeof(gSuperTileTextureObjects));			// clear all supertile texture pointers



				/* OPEN THE DATA FORK */

	iErr = FSpOpenDF(specPtr, fsRdPerm, &fRefNum);
	GAME_ASSERT(!iErr);



	for (int i = 0; i < gNumUniqueSuperTiles; i++)
	{
		static long	sizeoflong = 4;
		int32_t	compressedSize;
		int32_t	width,height;

		Ptr image = imageBank + i * size;

				/* READ THE SIZE OF THE NEXT COMPRESSED SUPERTILE TEXTURE */

		iErr = FSRead(fRefNum, &sizeoflong, (Ptr) &compressedSize);
		GAME_ASSERT(!iErr);

		compressedSize = SwizzleLong(&compressedSize);


				/* READ & DECOMPRESS IT */

		long decompressedSize = LZSS_Decode(fRefNum, image, compressedSize);
		GAME_ASSERT(decompressedSize == size);

		width = SUPERTILE_TEXMAP_SIZE;
		height = SUPERTILE_TEXMAP_SIZE;

				/* CONVERT PIXEL FORMAT */

		ConvertTexture16To16((uint16_t *) image, width, height);
	}


				/******************************/
				/* CREATE SUPERTILE MATERIALS */
				/******************************/

	for (int row = 0; row < gNumSuperTilesDeep; row++)
	for (int col = 0; col < gNumSuperTilesWide; col++)
	{
		int unique = gSuperTileTextureGrid[row][col];//.superTileID;
		if (unique == -1)											// if -1 then it's a blank
		{
			continue;
		}

		GAME_ASSERT_MESSAGE(gSuperTileTextureObjects[unique] == NULL, "supertile isn't unique!");

		int cw, ch;	// canvas width & height

		if (!gG4)	// No seamless texturing if we're in low-detail mode (requires NPOT textures)
		{
			cw = SUPERTILE_TEXMAP_SIZE;
			ch = SUPERTILE_TEXMAP_SIZE;
			SDL_memcpy(canvas, TileImage(imageBank, col, row), size);
		}
		else		// Do seamless texturing
		{
			int tw = SUPERTILE_TEXMAP_SIZE;		// supertile width & height
			int th = SUPERTILE_TEXMAP_SIZE;
			cw = tw + 2;
			ch = th + 2;

			// Clear canvas to black
			SDL_memset(canvas, 0, seamlessCanvasSize);

			// Blit supertile image to middle of canvas
			Blit16(TileImage(imageBank, col, row), tw, th, 0, 0, tw, th, canvas, cw, ch, 1, 1);

			// Scan for neighboring supertiles
			// (Any of these may come out to NULL, in which case Blit16 will skip them later)
			Ptr neighborN	= TileImage(imageBank, col  , row-1);
			Ptr neighborS	= TileImage(imageBank, col  , row+1);
			Ptr neighborW	= TileImage(imageBank, col-1, row  );
			Ptr neighborE	= TileImage(imageBank, col+1, row  );
			Ptr neighborNE	= TileImage(imageBank, col+1, row-1);
			Ptr neighborNW	= TileImage(imageBank, col-1, row-1);
			Ptr neighborSW	= TileImage(imageBank, col-1, row+1);
			Ptr neighborSE	= TileImage(imageBank, col+1, row+1);

			// Stitch edges from neighboring supertiles on each side and copy 1px corners from diagonal neighbors
			//     srcBuf      sW  sH    sX    sY  rW  rH  dstBuf  dW  dH    dX    dY
			Blit16(neighborN , tw, th,    0, th-1, tw,  1, canvas, cw, ch,    1,    0);		// north
			Blit16(neighborS , tw, th,    0,    0, tw,  1, canvas, cw, ch,    1, ch-1);		// south
			Blit16(neighborW , tw, th, tw-1,    0,  1, th, canvas, cw, ch,    0,    1);		// west
			Blit16(neighborE , tw, th,    0,    0,  1, th, canvas, cw, ch, cw-1,    1);		// east
			Blit16(neighborNE, tw, th,    0, th-1,  1,  1, canvas, cw, ch, cw-1,    0);		// northeast
			Blit16(neighborNW, tw, th, tw-1, th-1,  1,  1, canvas, cw, ch,    0,    0);		// northwest
			Blit16(neighborSW, tw, th, tw-1,    0,  1,  1, canvas, cw, ch,    0, ch-1);		// southwest
			Blit16(neighborSE, tw, th,    0,    0,  1,  1, canvas, cw, ch, cw-1, ch-1);		// southeast
		}

		int texture = OGL_TextureMap_Load(canvas, cw, ch, GL_BGRA_EXT, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		OGL_CheckError();

				/* INIT NEW MATERIAL DATA */

		MOMaterialData	matData =
		{
			.pixelSrcFormat			= GL_BGRA_EXT,
			.pixelDstFormat			= GL_RGBA,
			.textureName[0]			= texture,
			.flags					= BG3D_MATERIALFLAG_CLAMP_U | BG3D_MATERIALFLAG_CLAMP_V | BG3D_MATERIALFLAG_TEXTURED,
			.multiTextureMode		= MULTI_TEXTURE_MODE_REFLECTIONSPHERE,
			.multiTextureCombine	= MULTI_TEXTURE_COMBINE_ADD,
			.diffuseColor			= {1,1,1,1},
			.numMipmaps				= 1,										// 1 texture
			.width					= cw,
			.height					= ch,
			.texturePixels[0] 		= nil,										// the original pixels are gone (or will be soon)
		};


		gSuperTileTextureObjects[unique] = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);		// create the new object
	}

			/* CLOSE THE FILE AND CLEAN UP */

	FSClose(fRefNum);
	SafeDisposePtr(imageBank);
	SafeDisposePtr(canvas);
}


/*********************** TILE TEXTURE LOADER HELPER ***********************************/

static Ptr TileImage(Ptr imageBank, int col, int row)
{
	if (col < 0)
		return NULL;

	if (col >= gNumSuperTilesWide)
		return NULL;

	if (row < 0)
		return NULL;

	if (row >= gNumSuperTilesDeep)
		return NULL;

	int imageNum = gSuperTileTextureGrid[row][col];

	if (imageNum < 0)	// it's a blank
		return NULL;

	return imageBank + imageNum * SQUARED(SUPERTILE_TEXMAP_SIZE) * sizeof(uint16_t);
}


/*********************** CONVERT TEXTURE; 16 TO 16 ***********************************/
//
// Converts big-endian 1-5-5-5 to native endianness and cleans up the alpha bit.
//

static void	ConvertTexture16To16(uint16_t *textureBuffer, int width, int height)
{
//	bool blackOpaq = (gLevelNum != LEVEL_NUM_CLOUD);		// make black transparent on cloud
	bool blackOpaq = true;

	for (int p = 0; p < width*height; p++)
	{
		uint16_t pixel = SwizzleUShort(textureBuffer);

		if (blackOpaq || (pixel & 0x7fff))
			pixel |= 0x8000;
		else
			pixel &= 0x7fff;

		*textureBuffer = pixel;

		textureBuffer++;
	}
}

#pragma mark -


/***************************** SAVE GAME ********************************/
//
// Returns true if saving was successful
//

Boolean SaveGame(int slot)
{
SaveGameType	saveData;
short			fRefNum;
FSSpec			spec;
OSErr			err;
Str255			saveFilePath;

			/*************************/
			/* CREATE SAVE GAME DATA */
			/*************************/

	saveData.timestamp		= time(NULL);
	saveData.version		= SAVE_GAME_VERSION;				// save file version #
	saveData.score 			= gScore;
	saveData.health			= gPlayerInfo.health;
	saveData.realLevel		= gLevelNum + 1;					// save @ beginning of next level
	saveData.numLives		= gPlayerInfo.lives;
	saveData.numGoldClovers	= gPlayerInfo.numGoldClovers;		// save # gold clovers we have at this point

	saveData.timestamp		= SwizzleULong64(&saveData.timestamp);
	saveData.version		= SwizzleULong(&saveData.version);
	saveData.score			= SwizzleULong(&saveData.score);
	saveData.health			= SwizzleFloat(&saveData.health);

		/*******************/
		/* DO NAV SERVICES */
		/*******************/

	SDL_snprintf(saveFilePath, sizeof(saveFilePath), ":" PROJECT_NAME ":Save%c", 'A' + slot);

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, saveFilePath, &spec);

	err = FSpCreate(&spec, kGameID, 'B2Sv', 0);
	if (err != noErr)
	{
		DoAlert("Couldn't create save file.");
		return false;
	}

	err = FSpOpenDF(&spec, fsWrPerm, &fRefNum);

	if (err != noErr)
	{
		DoAlert("Couldn't open file for writing.");
		return false;
	}

	long count = sizeof(SaveGameType);
	err = FSWrite(fRefNum, &count, (Ptr)&saveData);
	FSClose(fRefNum);

	if (count != sizeof(SaveGameType) || err != noErr)
	{
		DoAlert("Couldn't write saved game file.");
		return false;
	}

	return true;
}


/***************************** LOAD SAVED GAME ********************************/

Boolean LoadSavedGameStruct(int slot, SaveGameType* saveData)
{
FSSpec			spec;
OSErr			err;
short			refNum;
Str255			saveFilePath;

	SDL_snprintf(saveFilePath, sizeof(saveFilePath), ":" PROJECT_NAME ":Save%c", 'A' + slot);

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, saveFilePath, &spec);
	err = FSpOpenDF(&spec, fsRdPerm, &refNum);

	if (err != noErr)
	{
		return false;
	}

	long count = sizeof(SaveGameType);
	err = FSRead(refNum, &count, (Ptr)saveData);
	FSClose(refNum);

	if (count != sizeof(SaveGameType) || err != noErr)
	{
		return false;
	}

	saveData->timestamp			= SwizzleULong64(&saveData->timestamp);
	saveData->version			= SwizzleULong(&saveData->version);
	saveData->score				= SwizzleULong(&saveData->score);
	saveData->health			= SwizzleFloat(&saveData->health);

	if (saveData->version != SAVE_GAME_VERSION)
		return false;

	if (saveData->realLevel >= NUM_LEVELS)
		return false;

	return true;
}

Boolean LoadSavedGame(int slot)
{
	SaveGameType saveData;

	if (!LoadSavedGameStruct(slot, &saveData))
		return false;

	gLoadedScore = gScore = saveData.score;

	gLevelNum = saveData.realLevel;

	gPlayerInfo.lives = saveData.numLives;
	gPlayerInfo.health = saveData.health;
	gPlayerInfo.numGoldClovers = saveData.numGoldClovers;

	return true;
}


#pragma mark -

/************************* LOAD TUNNEL ***************************/

void LoadTunnel(FSSpec *inSpec, FSSpec *bg3dSpec)
{
OSErr		iErr;
short		fRefNum;

				/* OPEN THE DATA FORK */

	iErr = FSpOpenDF(inSpec, fsRdPerm, &fRefNum);
	if (iErr)
		DoFatalAlert("LoadTunnel: FSpOpenDF failed!");



			/* READ DATA FROM FILE */

	ReadDataFromTunnelFile(inSpec, bg3dSpec, fRefNum);

	FSClose(fRefNum);


}


/************** READ DATA FROM TUNNEL FILE *****************/

static void ReadDataFromTunnelFile(FSSpec *tunnelSpec, FSSpec *bg3dSpec, short fRefNum)
{
TunnelFileHeaderType	header;
OSErr					iErr;
long					size;
int32_t					w,h;
Ptr						buffer;
int32_t					aliasSize;
int						i, j;
MOVertexArrayData		data;

	(void) tunnelSpec;

			/***************/
			/* READ HEADER */
			/***************/

	size = sizeof(header);
	iErr = FSRead(fRefNum, &size, (Ptr) &header);
	GAME_ASSERT(!iErr);
	GAME_ASSERT(size == sizeof(header));

	header.numNubs			= SwizzleLong(&header.numNubs);


			/* EXTRACT HEADER DATA */

//	gTunnelIsFullPipe 			= header.fullPipe;
	gNumTunnelItems				= SwizzleLong(&header.numItems);
	gNumTunnelSplinePoints		= SwizzleLong(&header.numSplinePoints);
	gNumTunnelSections			= SwizzleLong(&header.numSections);

	GAME_ASSERT(gNumTunnelSections <= MAX_TUNNEL_SECTIONS);


			/****************************/
			/* READ ALIAS TO BG3D FILE */
			/****************************/

			/* READ THE SIZE OF THE ALIAS DATA */

	size = sizeof(aliasSize);
	FSRead(fRefNum, &size, (Ptr) &aliasSize);
	aliasSize = SwizzleLong(&aliasSize);


				/* SKIP ALIAS */
				//
				// Swizzling an Alias would be too difficult, so for verison 3.0 I've removed this
				// and will now manually deal with the path to the BG3D file.
				//

	SetFPos(fRefNum, fsFromMark, aliasSize);

	ImportBG3D(bg3dSpec, MODEL_GROUP_LEVELSPECIFIC);



			/*************/
			/* SKIP NUBS */
			/*************/

	SetFPos(fRefNum, fsFromMark, sizeof(TunnelSplineNubType) * header.numNubs);


			/*****************/
			/* READ TEXTURES */
			/*****************/

		/* READ TUNNEL TEXTURE */

	size = sizeof(int32_t);												// read this texture's dimensions
	FSRead(fRefNum, &size, (Ptr) &w);
	w = SwizzleLong(&w);
	FSRead(fRefNum, &size, (Ptr) &h);
	h = SwizzleLong(&h);

	size = w * h * 4;													// read the pixel buffer
	buffer = AllocPtr(size);
	FSRead(fRefNum, &size, buffer);

	gTunnelTextureObj = MO_CreateTextureObjectFromBuffer(w, h, buffer);	// create material object from buffer


		/* READ WATER TEXTURE */

	size = sizeof(int32_t);												// read this texture's dimensions
	FSRead(fRefNum, &size, (Ptr) &w);
	w = SwizzleLong(&w);
	FSRead(fRefNum, &size, (Ptr) &h);
	h = SwizzleLong(&h);

	size = w * h * 4;													// read the pixel buffer

	SetFPos(fRefNum, fsFromMark, size);								// skip water texture

			/**************/
			/* READ ITEMS */
			/**************/

	if (gNumTunnelItems > 0)
	{
		if (gTunnelItemList)											// free any old item list
			SafeDisposePtr(gTunnelItemList);

		size = sizeof(TunnelItemDefType) * gNumTunnelItems;
		gTunnelItemList = AllocPtr(size);								// alloc a new list
		FSRead(fRefNum, &size, (Ptr) gTunnelItemList);					// read data into it

		for (i = 0; i < gNumTunnelItems; i++)
		{
			gTunnelItemList[i].type	=	SwizzleLong(&gTunnelItemList[i].type);
			gTunnelItemList[i].splineIndex	=	SwizzleLong(&gTunnelItemList[i].splineIndex);
			gTunnelItemList[i].sectionNum	=	SwizzleLong(&gTunnelItemList[i].sectionNum);
			gTunnelItemList[i].scale		=	SwizzleFloat(&gTunnelItemList[i].scale);
			SwizzleVector3D(&gTunnelItemList[i].rot);
			SwizzleVector3D(&gTunnelItemList[i].positionOffset);
			gTunnelItemList[i].flags	=	SwizzleULong(&gTunnelItemList[i].flags);
			gTunnelItemList[i].parms[0]	=	SwizzleULong(&gTunnelItemList[i].parms[0]);
			gTunnelItemList[i].parms[1]	=	SwizzleULong(&gTunnelItemList[i].parms[1]);
			gTunnelItemList[i].parms[2]	=	SwizzleULong(&gTunnelItemList[i].parms[2]);
		}
	}


		/**********************/
		/* READ SPLINE POINTS */
		/**********************/

	size = sizeof(TunnelSplinePointType) * gNumTunnelSplinePoints;
	gTunnelSplinePoints = AllocPtr(size);								// alloc a new list
	FSRead(fRefNum, &size, (Ptr) gTunnelSplinePoints);					// read data into it

	for (i = 0; i < gNumTunnelSplinePoints; i++)
	{
		SwizzlePoint3D(&gTunnelSplinePoints[i].point);
		SwizzleVector3D(&gTunnelSplinePoints[i].up);
	}

		/**************************/
		/* READ SECTION GEOMETRY */
		/**************************/

			/* PRIME SOME COMMON DATA */

	data.colorsByte 	= nil;
	data.colorsFloat 	= nil;

	for (j = 0; j < gNumTunnelSections; j++)
	{

				/* READ TUNNEL GEOMETRY FOR THIS SECTION */

		size = sizeof(OGLBoundingBox);							// read bbox
		iErr = FSRead(fRefNum, &size, (Ptr) &data.bBox);
		SwizzlePoint3D(&data.bBox.min);
		SwizzlePoint3D(&data.bBox.max);

		size = sizeof(int32_t);									// read # vertices
		iErr |= FSRead(fRefNum, &size, (Ptr) &data.numPoints);
		data.numPoints = SwizzleLong(&data.numPoints);
		size = sizeof(int32_t);									// read # triangles
		iErr |= FSRead(fRefNum, &size, (Ptr) &data.numTriangles);
		data.numTriangles = SwizzleLong(&data.numTriangles);

		size = sizeof(OGLPoint3D) * data.numPoints;
		data.points = AllocPtr(size);							// alloc coord list
		iErr |= FSRead(fRefNum, &size, (Ptr) data.points);		// read points coords
		for (i = 0; i < data.numPoints; i++)
			SwizzlePoint3D(&data.points[i]);

		size = sizeof(OGLVector3D) * data.numPoints;
		data.normals = AllocPtr(size);							// alloc normals list
		iErr |= FSRead(fRefNum, &size, (Ptr) data.normals);		// read normals
		for (i = 0; i < data.numPoints; i++)
			SwizzleVector3D(&data.normals[i]);

		size = sizeof(OGLTextureCoord) * data.numPoints;
		data.uvs[0] = AllocPtr(size);							// alloc uvs list
		iErr |= FSRead(fRefNum, &size, (Ptr) data.uvs[0]);		// read uvs
		for (i = 0; i < data.numPoints; i++)
			SwizzleUV(&data.uvs[0][i]);

		size = sizeof(MOTriangleIndecies) * data.numTriangles;
		data.triangles = AllocPtr(size);						// alloc triangle list
		iErr |= FSRead(fRefNum, &size, (Ptr) data.triangles);	// read triangles
		for (i = 0; i < data.numTriangles; i++)
		{
			data.triangles[i].vertexIndices[0] = SwizzleULong(&data.triangles[i].vertexIndices[0]);
			data.triangles[i].vertexIndices[1] = SwizzleULong(&data.triangles[i].vertexIndices[1]);
			data.triangles[i].vertexIndices[2] = SwizzleULong(&data.triangles[i].vertexIndices[2]);
		}

		if (iErr)
			DoFatalAlert("ReadDataFromTunnelFile: FSRead failed!");

		data.numMaterials 	= 1;
		data.materials[0] = gTunnelTextureObj;					// assign illegal ref (made legal below)

		gTunnelSectionObjects[j] = MO_CreateNewObjectOfType(MO_TYPE_GEOMETRY, MO_GEOMETRY_SUBTYPE_VERTEXARRAY, &data);	// make metaobject



				/* READ WATER GEOMETRY FOR THIS SECTION */

		size = sizeof(OGLBoundingBox);							// read bbox
		iErr = FSRead(fRefNum, &size, (Ptr) &data.bBox);
		SwizzlePoint3D(&data.bBox.min);
		SwizzlePoint3D(&data.bBox.max);

		size = sizeof(int32_t);									// read # vertices
		iErr |= FSRead(fRefNum, &size, (Ptr) &data.numPoints);
		data.numPoints = SwizzleLong(&data.numPoints);

		size = sizeof(int32_t);									// read # triangles
		iErr |= FSRead(fRefNum, &size, (Ptr) &data.numTriangles);
		data.numTriangles = SwizzleLong(&data.numTriangles);

		size = sizeof(OGLPoint3D) * data.numPoints;
		data.points = AllocPtr(size);							// alloc coord list
		iErr |= FSRead(fRefNum, &size, (Ptr) data.points);			// read points coords
		for (i = 0; i < data.numPoints; i++)
			SwizzlePoint3D(&data.points[i]);

		data.normals = nil;										// no normals on H2O

		size = sizeof(OGLTextureCoord) * data.numPoints;
		data.uvs[0] = AllocPtr(size);							// alloc uvs list
		iErr |= FSRead(fRefNum, &size, (Ptr) data.uvs[0]);		// read uvs
		for (i = 0; i < data.numPoints; i++)
			SwizzleUV(&data.uvs[0][i]);

		size = sizeof(MOTriangleIndecies) * data.numTriangles;
		data.triangles = AllocPtr(size);						// alloc triangle list
		iErr |= FSRead(fRefNum, &size, (Ptr) data.triangles);	// read triangles
		for (i = 0; i < data.numTriangles; i++)
		{
			data.triangles[i].vertexIndices[0] = SwizzleULong(&data.triangles[i].vertexIndices[0]);
			data.triangles[i].vertexIndices[1] = SwizzleULong(&data.triangles[i].vertexIndices[1]);
			data.triangles[i].vertexIndices[2] = SwizzleULong(&data.triangles[i].vertexIndices[2]);
		}

		GAME_ASSERT(!iErr);

		data.numMaterials = -1;
		data.materials[0] = nil;


		gTunnelSectionWaterObjects[j] = MO_CreateNewObjectOfType(MO_TYPE_GEOMETRY, MO_GEOMETRY_SUBTYPE_VERTEXARRAY, &data);	// make metaobject
	}
}


#pragma mark -


/*********************** LOAD DATA FILE INTO MEMORY ***********************************/
//
// Use SafeDisposePtr when done.
//

Ptr LoadDataFile(const char* path, long* outLength)
{
	FSSpec spec;
	OSErr err;
	short refNum;
	long fileLength = 0;
	long readBytes = 0;

	err = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &spec);
	if (err != noErr)
		return NULL;

	err = FSpOpenDF(&spec, fsRdPerm, &refNum);
	GAME_ASSERT_MESSAGE(!err, path);

	// Get number of bytes until EOF
	GetEOF(refNum, &fileLength);

	// Prep data buffer
	// Alloc 1 extra byte so LoadTextFile can return a null-terminated C string!
	Ptr data = AllocPtrClear(fileLength + 1);

	// Read file into data buffer
	readBytes = fileLength;
	err = FSRead(refNum, &readBytes, (Ptr) data);
	GAME_ASSERT_MESSAGE(err == noErr, path);
	FSClose(refNum);

	GAME_ASSERT_MESSAGE(fileLength == readBytes, path);

	if (outLength)
	{
		*outLength = fileLength;
	}

	return data;
}

/*********************** LOAD TEXT FILE INTO MEMORY ***********************************/
//
// Use SafeDisposePtr when done.
//

char* LoadTextFile(const char* spec, long* outLength)
{
	return LoadDataFile(spec, outLength);
}



/****************** COPY REGION BETWEEN 16-BIT PIXEL BUFFERS **********************/

static inline void Blit16(
		const char*			src,
		int					srcWidth,
		int					srcHeight,
		int					srcRectX,
		int					srcRectY,
		int					srcRectWidth,
		int					srcRectHeight,
		char*				dst,
		int 				dstWidth,
		int 				dstHeight,
		int					dstRectX,
		int					dstRectY
)
{
	if (!src)
		return;

	if (!dst)
		return;

	const int bytesPerPixel = 2;

	GAME_ASSERT(srcRectX + srcRectWidth <= srcWidth);
	GAME_ASSERT(srcRectY + srcRectHeight <= srcHeight);
	GAME_ASSERT(dstRectX + srcRectWidth <= dstWidth);
	GAME_ASSERT(dstRectY + srcRectHeight <= dstHeight);

	src += bytesPerPixel * (srcRectX + srcWidth * srcRectY);
	dst += bytesPerPixel * (dstRectX + dstWidth * dstRectY);

	for (int row = 0; row < srcRectHeight; row++)
	{
		SDL_memcpy(dst, src, bytesPerPixel * srcRectWidth);
		src += bytesPerPixel * srcWidth;
		dst += bytesPerPixel * dstWidth;
	}
}
