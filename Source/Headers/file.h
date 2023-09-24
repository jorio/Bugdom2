//
// file.h
//

		/***********************/
		/* RESOURCE STURCTURES */
		/***********************/

			/* Hedr */

typedef struct
{
	short	version;			// 0xaa.bb
	short	numAnims;			// gNumAnims
	short	numJoints;			// gNumJoints
	short	num3DMFLimbs;		// gNumLimb3DMFLimbs
}SkeletonFile_Header_Type;

			/* Bone resource */
			//
			// matches BoneDefinitionType except missing
			// point and normals arrays which are stored in other resources.
			// Also missing other stuff since arent saved anyway.

typedef struct
{
	int32_t 				parentBone;			 		// index to previous bone
	char					name[32];					// text string name for bone
	OGLPoint3D				coord;						// absolute coord (not relative to parent!)
	uint16_t				numPointsAttachedToBone;	// # vertices/points that this bone has
	uint16_t				numNormalsAttachedToBone;	// # vertex normals this bone has
	uint32_t				reserved[8];				// reserved for future use
}File_BoneDefinitionType;



			/* AnHd */

typedef struct
{
	Str32	animName;
	int16_t	numAnimEvents;
}SkeletonFile_AnimHeader_Type;



		/* PREFERENCES */

#define	MAX_HTTP_NOTES	1000

#define	CURRENT_PREFS_VERS	0xA0E1

typedef struct
{
	Byte	difficulty;
	Boolean	showScreenModeDialog;
	short	depth;
	int		screenWidth;
	int		screenHeight;
	double	hz;
	Byte	language;
	Boolean	kiddieMode;
	Boolean	deepZ;
	// DateTimeRec	lastVersCheckDate;
	Byte	didThisNote[MAX_HTTP_NOTES];
	Boolean	anaglyph;
	Boolean	anaglyphColor;
	uint32_t	version;
	Boolean	dontUseHID;
	// HIDControlSettingsType	controlSettings;

	Byte	monitorNum;
	Byte	antialiasingLevel;
	Boolean	vsync;
}PrefsType;



		/* SAVE PLAYER */

typedef struct
{
	Byte		numAgesCompleted;		// encode # ages in lower 4 bits, and stage in upper 4 bits
	Str255		playerName;
}SavePlayerType;





//=================================================

SkeletonDefType *LoadSkeletonFile(short skeletonType);
extern	void	OpenGameFile(Str255 filename,short *fRefNumPtr, Str255 errString);
extern	OSErr LoadPrefs(PrefsType *prefBlock);
void SavePrefs(void);

void LoadPlayfield(FSSpec *specPtr);
OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld, short depth);
void SetDefaultDirectory(void);

Boolean SaveGame(void);
Boolean LoadSavedGame(void);


void LoadTunnel(FSSpec *inSpec, FSSpec *bg3dSpec);

void LoadLevelArt_Explore(void);
void LoadLevelArt_Tunnel(void);

void LoadFoliage(void);

Ptr LoadDataFile(const char* path, long* outLength);
char* LoadTextFile(const char* spec, long* outLength);
