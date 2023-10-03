//
// file.h
//

#define NUM_SAVE_SLOTS 8

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

#define	CURRENT_PREFS_VERS	0xA0E2
#define PREFS_FILE_NAME		"Preferences4"
#define PREFS_FILE_PATH		(":" PROJECT_NAME ":" PREFS_FILE_NAME)

typedef struct
{
	uint32_t	version;

	Byte	language;
	Boolean	kiddieMode;
	Byte	anaglyph;
	Boolean	music;

	Byte	monitorNum;
	Byte	antialiasingLevel;
	Boolean	vsync;
	Boolean	fullscreen;
	Boolean	uiCentering;

	Byte	rumbleIntensity;
	Byte	mouseSensitivityLevel;
	Boolean	gamepadRumble;
	InputBinding	bindings[NUM_CONTROL_NEEDS];
}PrefsType;

enum
{
	ANAGLYPH_OFF,
	ANAGLYPH_COLOR,
	ANAGLYPH_MONO,
};

		/* SAVE GAME */

#define	SAVE_GAME_VERSION	0x0400		// 4.0

typedef struct
{
	uint64_t	timestamp;
	uint32_t	version;
	uint32_t	score;
	float		health;
	Byte		realLevel;
	Byte		numLives;
	Byte		numGoldClovers;
}SaveGameType;


//=================================================

SkeletonDefType *LoadSkeletonFile(short skeletonType);
extern	void	OpenGameFile(Str255 filename,short *fRefNumPtr, Str255 errString);
extern	OSErr LoadPrefs(void);
void SavePrefs(void);

void LoadPlayfield(FSSpec *specPtr);

Boolean SaveGame(int slot);
Boolean LoadSavedGameStruct(int slot, SaveGameType* outSaveData); 
Boolean LoadSavedGame(int slot);

void LoadTunnel(FSSpec *inSpec);

void LoadLevelArt_Explore(void);
void LoadLevelArt_Tunnel(void);

void LoadFoliage(void);

Ptr LoadDataFile(const char* path, long* outLength);
char* LoadTextFile(const char* spec, long* outLength);
