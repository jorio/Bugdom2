/****************************/
/*      FILE ROUTINES       */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include 	"bones.h"
#include 	"lzss.h"
#include 	"tunnel.h"

extern	short			gCurrentSong;
extern	short			gNumTerrainItems;
extern	short			gPrefsFolderVRefNum;
extern	long			gPrefsFolderDirID;
extern	long			gTerrainTileWidth,gTerrainTileDepth,gTerrainUnitWidth,gTerrainUnitDepth,gNumUniqueSuperTiles;
extern	long			gNumSuperTilesDeep,gNumSuperTilesWide;
extern	FSSpec			gDataSpec;
extern	u_long			gScore,gLoadedScore;
extern	float			gDemoVersionTimer,gTerrainPolygonSize,gMapToUnitValue;
extern	float			**gMapYCoords,**gMapYCoordsOriginal;
extern	Byte			**gMapSplitMode;
extern	TerrainItemEntryType 	**gMasterItemList;
extern	short			**gSuperTileTextureGrid;
extern	FenceDefType	*gFenceList;
extern	long			gNumFences,gNumSplines,gNumWaterPatches;
extern	int				gLevelNum,gNumTunnelItems,gNumTunnelSplinePoints,gNumTunnelSections,gNumLineMarkers;
extern	u_short			**gAttributeGrid;
extern	MOMaterialObject	*gSuperTileTextureObjects[MAX_SUPERTILE_TEXTURES];
extern	PrefsType			gGamePrefs;
extern	AGLContext		gAGLContext;
extern	AGLDrawable		gAGLWin;
extern	Boolean			gLowMemMode,gMuteMusicFlag,gMuteMusicFlag,gLoadedDrawSprocket;
extern	WaterDefType	**gWaterListHandle, *gWaterList;
extern	Boolean			gPlayingFromSavedGame,gG4,gTunnelIsFullPipe;
extern	MOMaterialObject	*gTunnelTextureObj;
extern	TunnelItemDefType	*gTunnelItemList;
extern	MOVertexArrayObject	*gTunnelSectionObjects[];
extern	MOVertexArrayObject	*gTunnelSectionWaterObjects[];
extern	TunnelSplinePointType	*gTunnelSplinePoints;
extern	LineMarkerDefType		gLineMarkerList[];
extern	Boolean					gDisableHiccupTimer;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType, OGLSetupOutputType *setupInfo);
static void ReadDataFromPlayfieldFile(FSSpec *specPtr, OGLSetupOutputType *setupInfo);
static void	ConvertTexture16To16(u_short *textureBuffer, int width, int height);

static OSErr GetFileWithNavServices(FSSpec *documentFSSpec);
pascal void myEventProc(NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms,
						NavCallBackUserData callBackUD);
pascal Boolean myFilterProc(AEDesc*theItem,void*info, NavCallBackUserData callBackUD, NavFilterModes filterMode);
static OSErr PutFileWithNavServices(NavReplyRecord *reply, FSSpec *outSpec);

static void ReadDataFromTunnelFile(FSSpec *tunnelSpec, FSSpec *bg3dSpec, short fRefNum, OGLSetupOutputType *setupInfo);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	BASE_PATH_TILE		900					// tile # of 1st path tile

#define	PICT_HEADER_SIZE	512

#define	SKELETON_FILE_VERS_NUM	0x0110			// v1.1

#define	SAVE_GAME_VERSION	0x0100		// 1.0

		/* SAVE GAME */

typedef struct
{
	u_long		version;
	u_long		score;
	short		realLevel;
	short		numLives;
	float		health;
	short		numGoldClovers;
}SaveGameType;


		/* PLAYFIELD HEADER */

typedef struct
{
	NumVersion	version;							// version of file
	long		numItems;							// # items in map
	long		mapWidth;							// width of map
	long		mapHeight;							// height of map
	float		tileSize;							// 3D unit size of a tile
	float		minY,maxY;							// min/max height values
	long		numSplines;							// # splines
	long		numFences;							// # fences
	long		numUniqueSuperTiles;				// # unique supertile
	long        numWaterPatches;                    // # water patches
	long		numCheckpoints;						// # checkpoints
	long        unused[10];
}PlayfieldHeaderType;


		/* FENCE STRUCTURE IN FILE */
		//
		// note: we copy this data into our own fence list
		//		since the game uses a slightly different
		//		data structure.
		//

typedef	struct
{
	long		x,z;
}FencePointType;


typedef struct
{
	u_short			type;				// type of fence
	short			numNubs;			// # nubs in fence
	FencePointType	**nubList;			// handle to nub list
	Rect			bBox;				// bounding box of fence area
}FileFenceDefType;


		/* TUNNEL HEADER */

typedef struct
{
	NumVersion	version;							// version of file
	Boolean		fullPipe;							// flag true if 360 degree pipe
	long			numNubs;							// # spline nubs
	long			numSplinePoints;					// # points in giant spline
	long			numSections;						// # pieces of geometry to load
	long			numItems;							// # items on spline
	long        unused[16];
}TunnelFileHeaderType;


/**********************/
/*     VARIABLES      */
/**********************/


float	g3DTileSize, g3DMinY, g3DMaxY;

static 	FSSpec		gSavedGameSpec;



/****************** SET DEFAULT DIRECTORY ********************/
//
// This function needs to be called for OS X because OS X doesnt automatically
// set the default directory to the application directory.
//

void SetDefaultDirectory(void)
{
ProcessSerialNumber serial;
ProcessInfoRec info;
FSSpec	app_spec;
WDPBRec wpb;
OSErr	iErr;

	serial.highLongOfPSN = 0;
	serial.lowLongOfPSN = kCurrentProcess;


	info.processInfoLength = sizeof(ProcessInfoRec);
	info.processName = NULL;
	info.processAppSpec = &app_spec;

	iErr = GetProcessInformation(&serial, & info);

	wpb.ioVRefNum = app_spec.vRefNum;
	wpb.ioWDDirID = app_spec.parID;
	wpb.ioNamePtr = NULL;

	iErr = PBHSetVolSync(&wpb);


		/* ALSO SET SAVED GAME SPEC TO DESKTOP */

	iErr = FindFolder(kOnSystemDisk,kDesktopFolderType,kDontCreateFolder,			// locate the desktop folder
					&gSavedGameSpec.vRefNum,&gSavedGameSpec.parID);
	gSavedGameSpec.name[0] = 0;

}



/******************* LOAD SKELETON *******************/
//
// Loads a skeleton file & creates storage for it.
//
// NOTE: Skeleton types 0..NUM_CHARACTERS-1 are reserved for player character skeletons.
//		Skeleton types NUM_CHARACTERS and over are for other skeleton entities.
//
// OUTPUT:	Ptr to skeleton data
//

SkeletonDefType *LoadSkeletonFile(short skeletonType, OGLSetupOutputType *setupInfo)
{
QDErr		iErr;
short		fRefNum;
FSSpec		fsSpec;
SkeletonDefType	*skeleton;
const Str63	fileNames[MAX_SKELETON_TYPES] =
{
	"\p:Skeletons:Skip_Explore.skeleton",
	"\p:Skeletons:Skip_Tunnel.skeleton",
	"\p:Skeletons:Skip_Title.skeleton",
	"\p:Skeletons:Snail.skeleton",
	"\p:Skeletons:Gnome.skeleton",
	"\p:Skeletons:HouseFly.skeleton",
	"\p:Skeletons:EvilPlant.skeleton",
	"\p:Skeletons:Chipmunk.skeleton",
	"\p:Skeletons:SnakeHead.skeleton",
	"\p:Skeletons:BuddyBug.skeleton",
	"\p:Skeletons:Checkpoint.skeleton",
	"\p:Skeletons:Flea.skeleton",
	"\p:Skeletons:Tick.skeleton",
	"\p:Skeletons:MouseTrap.skeleton",
	"\p:Skeletons:Mouse.skeleton",
	"\p:Skeletons:Soldier.skeleton",
	"\p:Skeletons:OttoToy.skeleton",
	"\p:Skeletons:BumbleBee.skeleton",
	"\p:Skeletons:HoboBag.skeleton",
	"\p:Skeletons:DragonFly.skeleton",
	"\p:Skeletons:Frog.skeleton",
	"\p:Skeletons:Moth.skeleton",
	"\p:Skeletons:ComputerBug.skeleton",
	"\p:Skeletons:Roach.skeleton",
	"\p:Skeletons:Ant.skeleton",
	"\p:Skeletons:Fish.skeleton",
};


	if (skeletonType < MAX_SKELETON_TYPES)
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, fileNames[skeletonType], &fsSpec);
	else
		DoFatalAlert("\pLoadSkeleton: Unknown skeletonType!");


			/* OPEN THE FILE'S REZ FORK */

	fRefNum = FSpOpenResFile(&fsSpec,fsRdPerm);
	if (fRefNum == -1)
	{
		iErr = ResError();
		DoAlert("\pError opening Skel Rez file");
		ShowSystemErr(iErr);
	}

	UseResFile(fRefNum);
	if (ResError())
		DoFatalAlert("\pError using Rez file!");


			/* ALLOC MEMORY FOR SKELETON INFO STRUCTURE */

	skeleton = (SkeletonDefType *)AllocPtr(sizeof(SkeletonDefType));
	if (skeleton == nil)
		DoFatalAlert("\pCannot alloc SkeletonInfoType");


			/* READ SKELETON RESOURCES */

	ReadDataFromSkeletonFile(skeleton,&fsSpec,skeletonType,setupInfo);
	PrimeBoneData(skeleton);

			/* CLOSE REZ FILE */

	CloseResFile(fRefNum);

	return(skeleton);
}


/************* READ DATA FROM SKELETON FILE *******************/
//
// Current rez file is set to the file.
//

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType, OGLSetupOutputType *setupInfo)
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
		DoFatalAlert("\pReadDataFromSkeletonFile: Error reading header resource!");
	headerPtr = (SkeletonFile_Header_Type *)*hand;
	version = SwizzleShort(&headerPtr->version);
	if (version != SKELETON_FILE_VERS_NUM)
		DoFatalAlert("\pSkeleton file has wrong version #");

	numAnims = skeleton->NumAnims = SwizzleShort(&headerPtr->numAnims);			// get # anims in skeleton
	numJoints = skeleton->NumBones = SwizzleShort(&headerPtr->numJoints);		// get # joints in skeleton
	ReleaseResource(hand);

	if (numJoints > MAX_JOINTS)										// check for overload
		DoFatalAlert("\pReadDataFromSkeletonFile: numJoints > MAX_JOINTS");


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
			LoadBonesReferenceModel(&target,skeleton, skeletonType, setupInfo);
		else
			DoFatalAlert("\pReadDataFromSkeletonFile: Cannot find Skeleton's BG3D file!");
		ReleaseResource((Handle)alias);
	}
	else
		DoFatalAlert("\pReadDataFromSkeletonFile: file is missing the Alias resource");



		/***********************************/
		/*  READ BONE DEFINITION RESOURCES */
		/***********************************/

	for (i=0; i < numJoints; i++)
	{
		File_BoneDefinitionType	*bonePtr;
		u_short					*indexPtr;

			/* READ BONE DATA */

		hand = GetResource('Bone',1000+i);
		if (hand == nil)
			DoFatalAlert("\pError reading Bone resource!");
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

		skeleton->Bones[i].pointList = (u_short *)AllocPtr(sizeof(u_short) * (int)skeleton->Bones[i].numPointsAttachedToBone);
		if (skeleton->Bones[i].pointList == nil)
			DoFatalAlert("\pReadDataFromSkeletonFile: AllocPtr/pointList failed!");

		skeleton->Bones[i].normalList = (u_short *)AllocPtr(sizeof(u_short) * (int)skeleton->Bones[i].numNormalsAttachedToBone);
		if (skeleton->Bones[i].normalList == nil)
			DoFatalAlert("\pReadDataFromSkeletonFile: AllocPtr/normalList failed!");

			/* READ POINT INDEX ARRAY */

		hand = GetResource('BonP',1000+i);
		if (hand == nil)
			DoFatalAlert("\pError reading BonP resource!");
		HLock(hand);
		indexPtr = (u_short *)(*hand);

			/* COPY POINT INDEX ARRAY INTO BONE STRUCT */

		for (j=0; j < skeleton->Bones[i].numPointsAttachedToBone; j++)
			skeleton->Bones[i].pointList[j] = SwizzleUShort(&indexPtr[j]);
		ReleaseResource(hand);


			/* READ NORMAL INDEX ARRAY */

		hand = GetResource('BonN',1000+i);
		if (hand == nil)
			DoFatalAlert("\pError reading BonN resource!");
		HLock(hand);
		indexPtr = (u_short *)(*hand);

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
	if (hand == nil)
		DoFatalAlert("\pError reading RelP resource!");
	HLock(hand);
	pointPtr = (OGLPoint3D *)*hand;

	i = GetHandleSize(hand) / sizeof(OGLPoint3D);
	if (i != skeleton->numDecomposedPoints)
		DoFatalAlert("\p# of points in Reference Model has changed!");
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
			DoFatalAlert("\pError getting anim header resource");
		HLock(hand);
		animHeaderPtr = (SkeletonFile_AnimHeader_Type *)*hand;

		skeleton->NumAnimEvents[i] = SwizzleShort(&animHeaderPtr->numAnimEvents);			// copy # anim events in anim
		ReleaseResource(hand);

			/* READ ANIM-EVENT DATA */

		hand = GetResource('Evnt',1000+i);
		if (hand == nil)
			DoFatalAlert("\pError reading anim-event data resource!");
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
			DoFatalAlert("\pError reading # keyframes/joint resource!");
		for (j=0; j < numJoints; j++)
			skeleton->JointKeyframes[j].numKeyFrames[i] = (*hand)[j];
		ReleaseResource(hand);
	}


	for (j=0; j < numJoints; j++)
	{
				/* ALLOC 2D ARRAY FOR KEYFRAMES */

		Alloc_2d_array(JointKeyframeType,skeleton->JointKeyframes[j].keyFrames,	numAnims,MAX_KEYFRAMES);

		if ((skeleton->JointKeyframes[j].keyFrames == nil) || (skeleton->JointKeyframes[j].keyFrames[0] == nil))
			DoFatalAlert("\pReadDataFromSkeletonFile: Error allocating Keyframe Array.");

					/* READ THIS JOINT'S KF'S FOR EACH ANIM */

		for (i=0; i < numAnims; i++)
		{
			numKeyframes = skeleton->JointKeyframes[j].numKeyFrames[i];					// get actual # of keyframes for this joint
			if (numKeyframes > MAX_KEYFRAMES)
				DoFatalAlert("\pError: numKeyframes > MAX_KEYFRAMES");

					/* READ A JOINT KEYFRAME */

			hand = GetResource('KeyF',1000+(i*100)+j);
			if (hand == nil)
				DoFatalAlert("\pError reading joint keyframes resource!");
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

OSErr LoadPrefs(PrefsType *prefBlock)
{
OSErr		iErr;
short		refNum;
FSSpec		file;
long		count;

				/*************/
				/* READ FILE */
				/*************/

#if DEMO
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, "\p:Bugdom2:DemoPreferences3", &file);
#else
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, "\p:Bugdom2:Preferences3", &file);
#endif
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);
	if (iErr)
		return(iErr);

	count = sizeof(PrefsType);
	iErr = FSRead(refNum, &count,  (Ptr)prefBlock);		// read data from file
	if (iErr)
	{
		FSClose(refNum);
		return(iErr);
	}

	FSClose(refNum);

			/****************/
			/* VERIFY PREFS */
			/****************/

	if ((gGamePrefs.depth != 16) && (gGamePrefs.depth != 32))
		goto err;

	if (gGamePrefs.version != CURRENT_PREFS_VERS)
		goto err;


		/* THEY'RE GOOD, SO ALSO RESTORE THE HID CONTROL SETTINGS */

	RestoreHIDControlSettings(&gGamePrefs.controlSettings);


	return(noErr);

err:
	InitDefaultPrefs();
	return(noErr);
}



/******************** SAVE PREFS **********************/

void SavePrefs(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

		/* GET THE CURRENT CONTROL SETTINGS */

	if (!gHIDInitialized)								// can't save prefs unless HID is initialized!
		return;

	BuildHIDControlSettings(&gGamePrefs.controlSettings);

				/* CREATE BLANK FILE */

#if DEMO
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, "\p:Bugdom2:DemoPreferences3", &file);
#else
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, "\p:Bugdom2:Preferences3", &file);
#endif
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
	FSWrite(refNum, &count, &gGamePrefs);
	FSClose(refNum);


}

#pragma mark -




/**************** DRAW PICTURE INTO GWORLD ***********************/
//
// Uses Quicktime to load any kind of picture format file and draws
// it into the GWorld
//
//
// INPUT: myFSSpec = spec of image file
//
// OUTPUT:	theGWorld = gworld contining the drawn image.
//

OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld, short depth)
{
OSErr						iErr;
GraphicsImportComponent		gi;
Rect						r;
ComponentResult				result;
PixMapHandle 				hPixMap;


			/* PREP IMPORTER COMPONENT */

	result = GetGraphicsImporterForFile(myFSSpec, &gi);		// load importer for this image file
	if (result != noErr)
	{
		DoAlert("\pDrawPictureIntoGWorld: GetGraphicsImporterForFile failed!  You do not have Quicktime properly installed, reinstall Quicktime and do a FULL install.");
		return(result);
	}
	if (GraphicsImportGetBoundsRect(gi, &r) != noErr)		// get dimensions of image
		DoFatalAlert("\pDrawPictureIntoGWorld: GraphicsImportGetBoundsRect failed!");


			/* MAKE GWORLD */

	iErr = NewGWorld(theGWorld, depth, &r, nil, nil, 0);					// try app mem
	if (iErr)
	{
		DoAlert("\pDrawPictureIntoGWorld: using temp mem");
		iErr = NewGWorld(theGWorld, depth, &r, nil, nil, useTempMem);		// try sys mem
		if (iErr)
		{
			DoAlert("\pDrawPictureIntoGWorld: MakeMyGWorld failed");
			return(1);
		}
	}

	if (depth == 32)
	{
		hPixMap = GetGWorldPixMap(*theGWorld);				// get gworld's pixmap
		(**hPixMap).cmpCount = 4;							// we want full 4-component argb (defaults to only rgb)
	}


			/* DRAW INTO THE GWORLD */

	DoLockPixels(*theGWorld);
	GraphicsImportSetGWorld(gi, *theGWorld, nil);			// set the gworld to draw image into
	GraphicsImportSetQuality(gi,codecLosslessQuality);		// set import quality

	result = GraphicsImportDraw(gi);						// draw into gworld
	CloseComponent(gi);										// cleanup
	if (result != noErr)
	{
		DoAlert("\pDrawPictureIntoGWorld: GraphicsImportDraw failed!");
		ShowSystemErr(result);
		DisposeGWorld (*theGWorld);
		*theGWorld= nil;
		return(result);
	}
	return(noErr);
}


#pragma mark -



/******************* GET DEMO TIMER *************************/

void GetDemoTimer(void)
{
	OSErr				iErr;
	short				refNum;
	FSSpec				file;
	long				count;

				/* READ TIMER FROM FILE */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, "\pSysCheck89", &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);
	if (iErr)
	{
		gDemoVersionTimer = 0;
	}
	else
	{
		count = sizeof(float);
		iErr = FSRead(refNum, &count,  &gDemoVersionTimer);			// read data from file
		if (iErr)
		{
			FSClose(refNum);
			FSpDelete(&file);										// file is corrupt, so delete
			gDemoVersionTimer = 0;
			return;
		}
		FSClose(refNum);
	}

//	{
//		Str255	s;
//
//		FloatToString(gDemoVersionTimer, s);
//		DoAlert(s);
//	}

		/* SEE IF TIMER HAS EXPIRED */

	if (gDemoVersionTimer > (60 * 90))								// let play for n minutes
	{
		DoDemoExpiredScreen();
	}
}


/************************ SAVE DEMO TIMER ******************************/

void SaveDemoTimer(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

				/* CREATE BLANK FILE */

	if (FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, "\pSysCheck89", &file) == noErr)
		FSpDelete(&file);														// delete any existing file
	iErr = FSpCreate(&file, '????', 'xxxx', smSystemScript);					// create blank file
	if (iErr)
		return;


				/* OPEN FILE */

	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
		return;

				/* WRITE DATA */

	count = sizeof(float);
	FSWrite(refNum, &count, &gDemoVersionTimer);
	FSClose(refNum);
}


#pragma mark -

/******************* LOAD PLAYFIELD *******************/

void LoadPlayfield(FSSpec *specPtr, OGLSetupOutputType *setupInfo)
{

	gDisableHiccupTimer = true;

			/* READ PLAYFIELD RESOURCES */

	ReadDataFromPlayfieldFile(specPtr, setupInfo);


				/* DO ADDITIONAL SETUP */

	CreateSuperTileMemoryList();				// allocate memory for the supertile geometry
	CalculateSplitModeMatrix();					// precalc the tile split mode matrix
	InitSuperTileGrid();						// init the supertile state grid

	BuildTerrainItemList();						// build list of items & find player start coords


			/* CAST ITEM SHADOWS */

	DoItemShadowCasting(setupInfo);
}


/********************** READ DATA FROM PLAYFIELD FILE ************************/

static void ReadDataFromPlayfieldFile(FSSpec *specPtr, OGLSetupOutputType *setupInfo)
{
Handle					hand;
PlayfieldHeaderType		**header;
long					row,col,j,i,size;
float					yScale;
float					*src;
short					fRefNum;
OSErr					iErr;
Ptr						tempBuffer16 = nil,tempBuffer24 = nil, tempBuffer32 = nil;

				/* OPEN THE REZ-FORK */

	fRefNum = FSpOpenResFile(specPtr,fsCurPerm);
	if (fRefNum == -1)
		DoFatalAlert("\pLoadPlayfield: FSpOpenResFile failed.  You seem to have a corrupt or missing file.  Please reinstall the game.");
	UseResFile(fRefNum);


			/************************/
			/* READ HEADER RESOURCE */
			/************************/

	hand = GetResource('Hedr',1000);
	if (hand == nil)
	{
		DoAlert("\pReadDataFromPlayfieldFile: Error reading header resource!");
		return;
	}

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
		DoFatalAlert("\pReadDataFromPlayfieldFile: terrain width not a supertile multiple");
	if ((gTerrainTileDepth % SUPERTILE_SIZE) != 0)
		DoFatalAlert("\pReadDataFromPlayfieldFile: terrain depth not a supertile multiple");


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

	if (gSuperTileTextureGrid)														// free old array
		Free_2d_array(gSuperTileTextureGrid);
	Alloc_2d_array(short, gSuperTileTextureGrid, gNumSuperTilesDeep, gNumSuperTilesWide);

	hand = GetResource('STgd',1000);												// load grid from rez
	if (hand == nil)
		DoFatalAlert("\pReadDataFromPlayfieldFile: Error reading supertile rez resource!");
	else																			// copy rez into 2D array
	{
		short *src = (short *)*hand;

		for (row = 0; row < gNumSuperTilesDeep; row++)
			for (col = 0; col < gNumSuperTilesWide; col++)
			{
				gSuperTileTextureGrid[row][col] = SwizzleShort(src++);
			}
		ReleaseResource(hand);
	}



			/* READ HEIGHT DATA MATRIX */

	yScale = gTerrainPolygonSize / g3DTileSize;											// need to scale original geometry units to game units

	if (gLevelNum == LEVEL_NUM_PARK)			// modify y scale for this level since we need more range
		yScale *= 2.0f;


	Alloc_2d_array(float, gMapYCoords, gTerrainTileDepth+1, gTerrainTileWidth+1);			// alloc 2D array for map
	Alloc_2d_array(float, gMapYCoordsOriginal, gTerrainTileDepth+1, gTerrainTileWidth+1);	// and the copy of it

	hand = GetResource('YCrd',1000);
	if (hand == nil)
		DoAlert("\pReadDataFromPlayfieldFile: Error reading height data resource!");
	else
	{
		src = (float *)*hand;
		for (row = 0; row <= gTerrainTileDepth; row++)
			for (col = 0; col <= gTerrainTileWidth; col++)
				gMapYCoordsOriginal[row][col] = gMapYCoords[row][col] = SwizzleFloat(src++) * yScale;
		ReleaseResource(hand);
	}

				/**************************/
				/* ITEM RELATED RESOURCES */
				/**************************/

				/* READ ITEM LIST */

	hand = GetResource('Itms',1000);
	if (hand == nil)
		DoAlert("\pReadDataFromPlayfieldFile: Error reading itemlist resource!");
	else
	{
		TerrainItemEntryType   *rezItems;

		DetachResource(hand);							// lets keep this data around
		HLockHi(hand);									// LOCK this one because we have the lookup table into this
		gMasterItemList = (TerrainItemEntryType **)hand;
		rezItems = (TerrainItemEntryType *)*hand;

				/* CONVERT COORDINATES */

		for (i = 0; i < gNumTerrainItems; i++)
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
		SplineDefType	*splinePtr = (SplineDefType *)*hand;

		DetachResource(hand);
		HLockHi(hand);
		gSplineList = (SplineDefType **)hand;

		for (i = 0; i < gNumSplines; i++)
		{
			(*gSplineList)[i].numNubs = SwizzleShort(&splinePtr[i].numNubs);
			(*gSplineList)[i].numPoints = SwizzleLong(&splinePtr[i].numPoints);
			(*gSplineList)[i].numItems = SwizzleShort(&splinePtr[i].numItems);

			(*gSplineList)[i].bBox.top = SwizzleShort(&splinePtr[i].bBox.top);
			(*gSplineList)[i].bBox.bottom = SwizzleShort(&splinePtr[i].bBox.bottom);
			(*gSplineList)[i].bBox.left = SwizzleShort(&splinePtr[i].bBox.left);
			(*gSplineList)[i].bBox.right = SwizzleShort(&splinePtr[i].bBox.right);
		}

	}
	else
		gNumSplines = 0;


			/* READ SPLINE POINT LIST */

	for (i = 0; i < gNumSplines; i++)
	{
		SplineDefType	*spline = &(*gSplineList)[i];									// point to Nth spline

		hand = GetResource('SpPt',1000+i);
		if (hand)
		{
			SplinePointType	*ptList = (SplinePointType *)*hand;

			DetachResource(hand);
			HLockHi(hand);
			(*gSplineList)[i].pointList = (SplinePointType **)hand;

			for (j = 0; j < spline->numPoints; j++)			// swizzle
			{
				(*spline->pointList)[j].x = SwizzleFloat(&ptList[j].x);
				(*spline->pointList)[j].z = SwizzleFloat(&ptList[j].z);
			}

		}
		else
			DoFatalAlert("\pReadDataFromPlayfieldFile: cant get spline points rez");
	}


			/* READ SPLINE ITEM LIST */

	for (i = 0; i < gNumSplines; i++)
	{
		SplineDefType	*spline = &(*gSplineList)[i];									// point to Nth spline

		hand = GetResource('SpIt',1000+i);
		if (hand)
		{
			SplineItemType	*itemList = (SplineItemType *)*hand;

			DetachResource(hand);
			HLockHi(hand);
			(*gSplineList)[i].itemList = (SplineItemType **)hand;

			for (j = 0; j < spline->numItems; j++)			// swizzle
			{
				(*spline->itemList)[j].placement = SwizzleFloat(&itemList[j].placement);
				(*spline->itemList)[j].type	= SwizzleUShort(&itemList[j].type);
				(*spline->itemList)[j].flags	= SwizzleUShort(&itemList[j].flags);
			}

		}
		else
			DoFatalAlert("\pReadDataFromPlayfieldFile: cant get spline items rez");
	}

			/****************************/
			/* FENCE RELATED RESOURCES */
			/****************************/

			/* READ FENCE LIST */

	hand = GetResource('Fenc',1000);
	if (hand)
	{
		FileFenceDefType *inData;

		gFenceList = (FenceDefType *)AllocPtr(sizeof(FenceDefType) * gNumFences);	// alloc new ptr for fence data
		if (gFenceList == nil)
			DoFatalAlert("\pReadDataFromPlayfieldFile: AllocPtr failed");

		inData = (FileFenceDefType *)*hand;								// get ptr to input fence list

		for (i = 0; i < gNumFences; i++)								// copy data from rez to new list
		{
			gFenceList[i].type 		= SwizzleUShort(&inData[i].type);
			gFenceList[i].numNubs 	= SwizzleShort(&inData[i].numNubs);
			gFenceList[i].nubList 	= nil;
			gFenceList[i].sectionVectors = nil;
		}
		ReleaseResource(hand);
	}
	else
		gNumFences = 0;


			/* READ FENCE NUB LIST */

	for (i = 0; i < gNumFences; i++)
	{
		hand = GetResource('FnNb',1000+i);					// get rez
		HLock(hand);
		if (hand)
		{
   			FencePointType *fileFencePoints = (FencePointType *)*hand;

			gFenceList[i].nubList = (OGLPoint3D *)AllocPtr(sizeof(FenceDefType) * gFenceList[i].numNubs);	// alloc new ptr for nub array
			if (gFenceList[i].nubList == nil)
				DoFatalAlert("\pReadDataFromPlayfieldFile: AllocPtr failed");



			for (j = 0; j < gFenceList[i].numNubs; j++)		// convert x,z to x,y,z
			{
				gFenceList[i].nubList[j].x = SwizzleLong(&fileFencePoints[j].x);
				gFenceList[i].nubList[j].z = SwizzleLong(&fileFencePoints[j].z);
				gFenceList[i].nubList[j].y = 0;
			}
			ReleaseResource(hand);
		}
		else
			DoFatalAlert("\pReadDataFromPlayfieldFile: cant get fence nub rez");
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

		for (i = 0; i < gNumWaterPatches; i++)						// swizzle
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

			for (j = 0; j < gWaterList[i].numNubs; j++)
			{
				gWaterList[i].nubList[j].x = SwizzleFloat(&gWaterList[i].nubList[j].x);
				gWaterList[i].nubList[j].y = SwizzleFloat(&gWaterList[i].nubList[j].y);
			}
		}


	}
	else
		gNumWaterPatches = 0;



			/*************************/
			/* LINE MARKER RESOURCES */
			/*************************/

	if (gNumLineMarkers > 0)
	{
		if (gNumLineMarkers > MAX_LINEMARKERS)
			DoFatalAlert("\pReadDataFromPlayfieldFile: gNumLineMarkers > MAX_LINEMARKERS");

				/* READ CHECKPOINT LIST */

		hand = GetResource('CkPt',1000);
		if (hand)
		{
			HLock(hand);
			BlockMove(*hand, &gLineMarkerList[0], GetHandleSize(hand));
			ReleaseResource(hand);

						/* CONVERT COORDINATES */

			for (i = 0; i < gNumLineMarkers; i++)
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

	size = SUPERTILE_TEXMAP_SIZE * SUPERTILE_TEXMAP_SIZE * 2;						// calc size of supertile 16-bit texture
	tempBuffer16 = AllocPtr(size);
	if (tempBuffer16 == nil)
		DoFatalAlert("\pReadDataFromPlayfieldFile: AllocPtr failed!");

	tempBuffer24 = AllocPtr(SUPERTILE_TEXMAP_SIZE * SUPERTILE_TEXMAP_SIZE * 3);		// alloc for 24bit pixels
	if (tempBuffer24 == nil)
		DoFatalAlert("\pReadDataFromPlayfieldFile: AllocPtr failed!");

	tempBuffer32 = AllocPtr(SUPERTILE_TEXMAP_SIZE * SUPERTILE_TEXMAP_SIZE * 4);		// alloc for 32bit pixels
	if (tempBuffer32 == nil)
		DoFatalAlert("\pReadDataFromPlayfieldFile: AllocPtr failed!");



				/* OPEN THE DATA FORK */

	iErr = FSpOpenDF(specPtr, fsCurPerm, &fRefNum);
	if (iErr)
		DoFatalAlert("\pReadDataFromPlayfieldFile: FSpOpenDF failed!");



	for (i = 0; i < gNumUniqueSuperTiles; i++)
	{
		static long	sizeoflong = 4;
		long	compressedSize,decompressedSize;
		long	width,height;
		MOMaterialData	matData;


				/* READ THE SIZE OF THE NEXT COMPRESSED SUPERTILE TEXTURE */

		iErr = FSRead(fRefNum, &sizeoflong, &compressedSize);
		if (iErr)
			DoFatalAlert("\pReadDataFromPlayfieldFile: FSRead failed!");

		compressedSize = SwizzleLong(&compressedSize);


				/* READ & DECOMPRESS IT */

		decompressedSize = LZSS_Decode(fRefNum, tempBuffer16, compressedSize);
		if (decompressedSize != size)
      			DoFatalAlert("\pReadDataFromPlayfieldFile: LZSS_Decode size is wrong!");

		width = SUPERTILE_TEXMAP_SIZE;
		height = SUPERTILE_TEXMAP_SIZE;



				/**************************/
				/* CREATE MATERIAL OBJECT */
				/**************************/


			/* USE PACKED PIXEL TYPE */

		ConvertTexture16To16((u_short *)tempBuffer16, width, height);
		matData.pixelSrcFormat 	= GL_BGRA_EXT;
		matData.pixelDstFormat 	= GL_RGBA;
		matData.textureName[0] 	= OGL_TextureMap_Load(tempBuffer16, width, height,
												 GL_BGRA_EXT, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV);

			/* INIT NEW MATERIAL DATA */

		matData.setupInfo				= setupInfo;								// remember which draw context this material is assigned to
		matData.flags 					= 	BG3D_MATERIALFLAG_CLAMP_U|
											BG3D_MATERIALFLAG_CLAMP_V|
											BG3D_MATERIALFLAG_TEXTURED;

		matData.multiTextureMode		= MULTI_TEXTURE_MODE_REFLECTIONSPHERE;
		matData.multiTextureCombine		= MULTI_TEXTURE_COMBINE_ADD;
		matData.diffuseColor.r			= 1;
		matData.diffuseColor.g			= 1;
		matData.diffuseColor.b			= 1;
		matData.diffuseColor.a			= 1;
		matData.numMipmaps				= 1;										// 1 texture
		matData.width					= width;
		matData.height					= height;
		matData.texturePixels[0] 		= nil;										// the original pixels are gone (or will be soon)
		gSuperTileTextureObjects[i] 	= MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);		// create the new object


	}

			/* CLOSE THE FILE */

	FSClose(fRefNum);
	if (tempBuffer16)
		SafeDisposePtr(tempBuffer16);
	if (tempBuffer24)
		SafeDisposePtr(tempBuffer24);
	if (tempBuffer32)
		SafeDisposePtr(tempBuffer32);
}




/*********************** CONVERT TEXTURE; 16 TO 16 ***********************************/
//
// Simply flips Y since OGL Textures are screwey
//

static void	ConvertTexture16To16(u_short *textureBuffer, int width, int height)
{
int		x,y;
u_short	pixel,*bottom;
u_short	*dest;


	bottom = textureBuffer + ((height - 1) * width);

	for (y = 0; y < height / 2; y++)
	{
		dest = bottom;

		for (x = 0; x < width; x++)
		{
			pixel = textureBuffer[x];						// get 16bit pixel from top
#if __BIG_ENDIAN__
			pixel |= 0x8000;
#else
			pixel = SwizzleUShort(&pixel);
			pixel |= 0x8000;
#endif

			textureBuffer[x] = bottom[x];					// copy bottom to top
#if __BIG_ENDIAN__
			textureBuffer[x] |= 0x8000;
#else
			textureBuffer[x] = SwizzleUShort(&textureBuffer[x]);
			textureBuffer[x] |= 0x8000;
#endif

			bottom[x] = pixel;								// save top into bottom
		}

		textureBuffer += width;
		bottom -= width;
	}
}


#pragma mark -


/******************** NAV SERVICES: GET DOCUMENT ***********************/

static OSErr GetFileWithNavServices(FSSpec *documentFSSpec)
{
NavDialogOptions 	dialogOptions;
AEDesc 				defaultLocation;
NavObjectFilterUPP filterProc 	= nil; //NewNavObjectFilterUPP(myFilterProc);
OSErr 				anErr 		= noErr;

			/* Specify default options for dialog box */

	anErr = NavGetDefaultDialogOptions(&dialogOptions);
	if (anErr == noErr)
	{
			/* Adjust the options to fit our needs */

		dialogOptions.dialogOptionFlags |= kNavSelectDefaultLocation;	// Set default location option
		dialogOptions.dialogOptionFlags ^= kNavAllowPreviews;			// Clear preview option
		dialogOptions.dialogOptionFlags ^= kNavAllowMultipleFiles;		// Clear multiple files option

		dialogOptions.location.h = dialogOptions.location.v = -1;		// use default position
		CopyPStr("\pSelect A Saved Game File", dialogOptions.windowTitle);

				/* make descriptor for default location */

		anErr = AECreateDesc(typeFSS,&gSavedGameSpec, sizeof(FSSpec), &defaultLocation);
//		if (anErr ==noErr)
		{
			/* Get 'open'resource.  A nil handle being returned is OK, this simply means no automatic file filtering. */

			static NavTypeList	typeList = {kGameID, 0, 1, 'B2sv'};		// set types to filter
			NavTypeListPtr 		typeListPtr = &typeList;
			NavReplyRecord 		reply;


			/* Call NavGetFile() with specified options and declare our app-defined functions and type list */

			anErr = NavGetFile(&defaultLocation, &reply, &dialogOptions, nil, nil, filterProc, &typeListPtr,nil);
			if ((anErr == noErr) && (reply.validRecord))
			{
					/* Deal with multiple file selection */

				long 	count;

				anErr = AECountItems(&(reply.selection),&count);


					/* Set up index for file list */

				if (anErr == noErr)
				{
					long i;

					for (i = 1; i <= count; i++)
					{
						AEKeyword 	theKeyword;
						DescType 	actualType;
						Size 		actualSize;

						/* Get a pointer to selected file */

						anErr = AEGetNthPtr(&(reply.selection), i, typeFSS,&theKeyword, &actualType,
											documentFSSpec, sizeof(FSSpec), &actualSize);
					}
				}


				/* Dispose of NavReplyRecord,resources,descriptors */

				anErr = NavDisposeReply(&reply);
			}

			(void)AEDisposeDesc(&defaultLocation);
		}
	}


		/* CLEAN UP */

	if (filterProc)
	{
		DisposeNavObjectFilterUPP(filterProc);
		filterProc = nil;
	}

	return anErr;
}


/********************** PUT FILE WITH NAV SERVICES *************************/

static OSErr PutFileWithNavServices(NavReplyRecord *reply, FSSpec *outSpec)
{
OSErr 				anErr 			= noErr;
NavDialogOptions 	dialogOptions;
OSType 				fileTypeToSave 	='PSav';
OSType 				creatorType 	= kGameID;
Str255				name = "\pBugdom 2 Saved Game";
AEDesc 				defaultLocation;

	anErr = NavGetDefaultDialogOptions(&dialogOptions);
	if (anErr == noErr)
	{
		CopyPStr(name, dialogOptions.savedFileName);					// set default name

		dialogOptions.location.h = dialogOptions.location.v = -1;		// use default position


				/* TRY TO CREATE DEFAULT LOCATION */

		AECreateDesc(typeFSS,&gSavedGameSpec, sizeof(gSavedGameSpec), &defaultLocation);

					/* PUT FILE */

		anErr = NavPutFile(&defaultLocation, reply, &dialogOptions, nil, fileTypeToSave, creatorType, nil);
		if ((anErr == noErr) && (reply->validRecord))
		{
			AEKeyword	theKeyword;
			DescType 	actualType;
			Size 		actualSize;

			anErr = AEGetNthPtr(&(reply->selection),1,typeFSS, &theKeyword,&actualType, outSpec, sizeof(FSSpec), &actualSize);
		}
	}

	return anErr;
}





#pragma mark -


/***************************** SAVE GAME ********************************/
//
// Returns true if saving was successful
//

Boolean SaveGame(void)
{
SaveGameType	saveData;
short			fRefNum;
FSSpec			*specPtr;
NavReplyRecord	navReply;
long			count;
Boolean			success = false;

	Enter2D();

			/*************************/
			/* CREATE SAVE GAME DATA */
			/*************************/

	saveData.version		= SAVE_GAME_VERSION;				// save file version #
	saveData.version		= SwizzleULong(&saveData.version);

	saveData.score 			= gScore;
	saveData.score			= SwizzleULong(&saveData.score);

	saveData.realLevel		= gLevelNum+1;						// save @ beginning of next level
	saveData.realLevel		= SwizzleShort(&saveData.realLevel);

	saveData.numLives 		= gPlayerInfo.lives;
	saveData.numLives		= SwizzleShort(&saveData.numLives);

	saveData.health			= gPlayerInfo.health;
	saveData.health			= SwizzleFloat(&saveData.health);

	saveData.numGoldClovers	 = gPlayerInfo.numGoldClovers;		// save # gold clovers we have at this point
	saveData.numGoldClovers		= SwizzleShort(&saveData.numGoldClovers);


		/*******************/
		/* DO NAV SERVICES */
		/*******************/

	if (PutFileWithNavServices(&navReply, &gSavedGameSpec))
		goto bail;
	specPtr = &gSavedGameSpec;
	if (navReply.replacing)										// see if delete old
		FSpDelete(specPtr);


				/* CREATE & OPEN THE REZ-FORK */

	if (FSpCreate(specPtr, kGameID,'B2sv',nil) != noErr)
	{
		DoAlert("\pError creating Save file");
		goto bail;
	}

	FSpOpenDF(specPtr,fsRdWrPerm, &fRefNum);
	if (fRefNum == -1)
	{
		DoAlert("\pError opening Save file");
		goto bail;
	}


				/* WRITE TO FILE */

	count = sizeof(SaveGameType);
	if (FSWrite(fRefNum, &count, (Ptr)&saveData) != noErr)
	{
		DoAlert("\pError writing Save file");
		FSClose(fRefNum);
		goto bail;
	}

			/* CLOSE FILE */

	FSClose(fRefNum);


			/* CLEANUP NAV SERVICES */

	NavCompleteSave(&navReply, kNavTranslateInPlace);

	success = true;

bail:
	NavDisposeReply(&navReply);
	HideRealCursor();
	Exit2D();
	return(success);
}


/***************************** LOAD SAVED GAME ********************************/

Boolean LoadSavedGame(void)
{
SaveGameType	saveData;
short			fRefNum;
long			count;
Boolean			success = false;
//short			oldSong;

//	oldSong = gCurrentSong;							// turn off playing music to get around bug in OS X
//	KillSong();

	Enter2D();
	MyFlushEvents();


				/* GET FILE WITH NAVIGATION SERVICES */

	if (GetFileWithNavServices(&gSavedGameSpec) != noErr)
		goto bail;


				/* OPEN THE REZ-FORK */

	FSpOpenDF(&gSavedGameSpec,fsRdPerm, &fRefNum);
	if (fRefNum == -1)
	{
		DoAlert("\pError opening Save file");
		goto bail;
	}

				/* READ FROM FILE */

	count = sizeof(SaveGameType);
	if (FSRead(fRefNum, &count, &saveData) != noErr)
	{
		DoAlert("\pError reading Save file");
		FSClose(fRefNum);
		goto bail;
	}

			/* CLOSE FILE */

	FSClose(fRefNum);


			/**********************/
			/* USE SAVE GAME DATA */
			/**********************/

	gLoadedScore = gScore = SwizzleULong(&saveData.score);

	gLevelNum			= SwizzleShort(&saveData.realLevel);

	gPlayerInfo.lives 	= SwizzleShort(&saveData.numLives);
	gPlayerInfo.health	= SwizzleFloat(&saveData.health);
	gPlayerInfo.numGoldClovers = SwizzleShort(&saveData.numGoldClovers);

	success = true;


bail:
	Exit2D();
	HideRealCursor();

//	if (!success)								// user cancelled, so start song again before returning
//		PlaySong(oldSong, true);

	return(success);
}


#pragma mark -

/************************* LOAD TUNNEL ***************************/

void LoadTunnel(FSSpec *inSpec, FSSpec *bg3dSpec, OGLSetupOutputType *setupInfo)
{
OSErr		iErr;
short		fRefNum;

				/* OPEN THE DATA FORK */

	iErr = FSpOpenDF(inSpec, fsCurPerm, &fRefNum);
	if (iErr)
		DoFatalAlert("\pLoadTunnel: FSpOpenDF failed!");



			/* READ DATA FROM FILE */

	ReadDataFromTunnelFile(inSpec, bg3dSpec, fRefNum, setupInfo);

	FSClose(fRefNum);


}


/************** READ DATA FROM TUNNEL FILE *****************/

static void ReadDataFromTunnelFile(FSSpec *tunnelSpec, FSSpec *bg3dSpec, short fRefNum, OGLSetupOutputType *setupInfo)
{
TunnelFileHeaderType	header;
OSErr					iErr;
long					size,w,h;
Ptr						buffer;
long					aliasSize;
int						i, j;
MOVertexArrayData		data;

#pragma unused (tunnelSpec)

			/***************/
			/* READ HEADER */
			/***************/

	size = sizeof(header);
	iErr = FSRead(fRefNum, &size, &header);
	if (iErr)
		DoFatalAlert("\pReadDataFromTunnelFile: error reading!");

	header.numNubs			= SwizzleLong(&header.numNubs);


			/* EXTRACT HEADER DATA */

	gTunnelIsFullPipe 			= header.fullPipe;
	gNumTunnelItems				= SwizzleLong(&header.numItems);
	gNumTunnelSplinePoints		= SwizzleLong(&header.numSplinePoints);
	gNumTunnelSections			= SwizzleLong(&header.numSections);

	if (gNumTunnelSections > MAX_TUNNEL_SECTIONS)
		DoFatalAlert("\pReadDataFromTunnelFile: gNumTunnelSections > MAX_TUNNEL_SECTIONS");


			/****************************/
			/* READ ALIAS TO BG3D FILE */
			/****************************/

			/* READ THE SIZE OF THE ALIAS DATA */

	size = sizeof(aliasSize);
	FSRead(fRefNum, &size, &aliasSize);
	aliasSize = SwizzleLong(&aliasSize);


				/* SKIP ALIAS */
				//
				// Swizzling an Alias would be too difficult, so for verison 3.0 I've removed this
				// and will now manually deal with the path to the BG3D file.
				//

	SetFPos(fRefNum, fsFromMark, aliasSize);

	ImportBG3D(bg3dSpec, MODEL_GROUP_LEVELSPECIFIC, setupInfo);



			/*************/
			/* SKIP NUBS */
			/*************/

	SetFPos(fRefNum, fsFromMark, sizeof(TunnelSplineNubType) * header.numNubs);


			/*****************/
			/* READ TEXTURES */
			/*****************/

		/* READ TUNNEL TEXTURE */

	size = sizeof(long);												// read this texture's dimensions
	FSRead(fRefNum, &size, &w);
	w = SwizzleLong(&w);
	FSRead(fRefNum, &size, &h);
	h = SwizzleLong(&h);

	size = w * h * 4;													// read the pixel buffer
	buffer = AllocPtr(size);
	FSRead(fRefNum, &size, buffer);

	gTunnelTextureObj = MO_CreateTextureObjectFromBuffer(setupInfo, w, h, buffer);	// create material object from buffer


		/* READ WATER TEXTURE */

	size = sizeof(long);												// read this texture's dimensions
	FSRead(fRefNum, &size, &w);
	w = SwizzleLong(&w);
	FSRead(fRefNum, &size, &h);
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
		FSRead(fRefNum, &size, gTunnelItemList);						// read data into it

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
	FSRead(fRefNum, &size, gTunnelSplinePoints);						// read data into it

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
		iErr = FSRead(fRefNum, &size, &data.bBox);
		SwizzlePoint3D(&data.bBox.min);
		SwizzlePoint3D(&data.bBox.max);

		size = sizeof(long);									// read # vertices
		iErr |= FSRead(fRefNum, &size, &data.numPoints);
		data.numPoints = SwizzleLong(&data.numPoints);
		size = sizeof(long);									// read # triangles
		iErr |= FSRead(fRefNum, &size, &data.numTriangles);
		data.numTriangles = SwizzleLong(&data.numTriangles);

		size = sizeof(OGLPoint3D) * data.numPoints;
		data.points = AllocPtr(size);							// alloc coord list
		iErr |= FSRead(fRefNum, &size, data.points);			// read points coords
		for (i = 0; i < data.numPoints; i++)
			SwizzlePoint3D(&data.points[i]);

		size = sizeof(OGLVector3D) * data.numPoints;
		data.normals = AllocPtr(size);							// alloc normals list
		iErr |= FSRead(fRefNum, &size, data.normals);			// read normals
		for (i = 0; i < data.numPoints; i++)
			SwizzleVector3D(&data.normals[i]);

		size = sizeof(OGLTextureCoord) * data.numPoints;
		data.uvs[0] = AllocPtr(size);							// alloc uvs list
		iErr |= FSRead(fRefNum, &size, data.uvs[0]);			// read uvs
		for (i = 0; i < data.numPoints; i++)
			SwizzleUV(&data.uvs[0][i]);

		size = sizeof(MOTriangleIndecies) * data.numTriangles;
		data.triangles = AllocPtr(size);						// alloc triangle list
		iErr |= FSRead(fRefNum, &size, data.triangles);			// read triangles
		for (i = 0; i < data.numTriangles; i++)
		{
			data.triangles[i].vertexIndices[0] = SwizzleULong(&data.triangles[i].vertexIndices[0]);
			data.triangles[i].vertexIndices[1] = SwizzleULong(&data.triangles[i].vertexIndices[1]);
			data.triangles[i].vertexIndices[2] = SwizzleULong(&data.triangles[i].vertexIndices[2]);
		}

		if (iErr)
			DoFatalAlert("\pReadDataFromTunnelFile: FSRead failed!");

		data.numMaterials 	= 1;
		data.materials[0] = gTunnelTextureObj;					// assign illegal ref (made legal below)

		gTunnelSectionObjects[j] = MO_CreateNewObjectOfType(MO_TYPE_GEOMETRY, MO_GEOMETRY_SUBTYPE_VERTEXARRAY, &data);	// make metaobject



				/* READ WATER GEOMETRY FOR THIS SECTION */

		size = sizeof(OGLBoundingBox);							// read bbox
		iErr = FSRead(fRefNum, &size, &data.bBox);
		SwizzlePoint3D(&data.bBox.min);
		SwizzlePoint3D(&data.bBox.max);

		size = sizeof(long);									// read # vertices
		iErr |= FSRead(fRefNum, &size, &data.numPoints);
		data.numPoints = SwizzleLong(&data.numPoints);

		size = sizeof(long);									// read # triangles
		iErr |= FSRead(fRefNum, &size, &data.numTriangles);
		data.numTriangles = SwizzleLong(&data.numTriangles);

		size = sizeof(OGLPoint3D) * data.numPoints;
		data.points = AllocPtr(size);							// alloc coord list
		iErr |= FSRead(fRefNum, &size, data.points);			// read points coords
		for (i = 0; i < data.numPoints; i++)
			SwizzlePoint3D(&data.points[i]);

		data.normals = nil;										// no normals on H2O

		size = sizeof(OGLTextureCoord) * data.numPoints;
		data.uvs[0] = AllocPtr(size);							// alloc uvs list
		iErr |= FSRead(fRefNum, &size, data.uvs[0]);			// read uvs
		for (i = 0; i < data.numPoints; i++)
			SwizzleUV(&data.uvs[0][i]);

		size = sizeof(MOTriangleIndecies) * data.numTriangles;
		data.triangles = AllocPtr(size);						// alloc triangle list
		iErr |= FSRead(fRefNum, &size, data.triangles);			// read triangles
		for (i = 0; i < data.numTriangles; i++)
		{
			data.triangles[i].vertexIndices[0] = SwizzleULong(&data.triangles[i].vertexIndices[0]);
			data.triangles[i].vertexIndices[1] = SwizzleULong(&data.triangles[i].vertexIndices[1]);
			data.triangles[i].vertexIndices[2] = SwizzleULong(&data.triangles[i].vertexIndices[2]);
		}

		if (iErr)
			DoFatalAlert("\pReadDataFromTunnelFile: FSRead failed!");

		data.numMaterials = -1;
		data.materials[0] = nil;


		gTunnelSectionWaterObjects[j] = MO_CreateNewObjectOfType(MO_TYPE_GEOMETRY, MO_GEOMETRY_SUBTYPE_VERTEXARRAY, &data);	// make metaobject

	}


}











