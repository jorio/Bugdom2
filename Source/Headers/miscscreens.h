//
// miscscreens.h
//


enum
{
	LEVELINTRO_ObjType_Level1Ground = 0,
	LEVELINTRO_ObjType_Level1Text,
	LEVELINTRO_ObjType_MudSplotch,

	LEVELINTRO_ObjType_Level2Ground,
	LEVELINTRO_ObjType_Level2Text,

	LEVELINTRO_ObjType_PlayroomGround,
	LEVELINTRO_ObjType_LetterBlockP,
	LEVELINTRO_ObjType_LetterBlockL,
	LEVELINTRO_ObjType_LetterBlockA,
	LEVELINTRO_ObjType_LetterBlockY,
	LEVELINTRO_ObjType_LetterBlockR,
	LEVELINTRO_ObjType_LetterBlockO,
	LEVELINTRO_ObjType_LetterBlockM,

	LEVELINTRO_ObjType_PlaneBanner,
	LEVELINTRO_ObjType_GliderSky,

	LEVELINTRO_ObjType_SewerText,

	LEVELINTRO_ObjType_DogBone,

	LEVELINTRO_ObjType_ClosetRoom,
	LEVELINTRO_ObjType_GutterText,
	LEVELINTRO_ObjType_ParkText,

	LEVELINTRO_ObjType_GarbageText,
	LEVELINTRO_ObjType_GarbageGround,
	LEVELINTRO_ObjType_GarbageCan

};


enum
{
	FILE_SCREEN_TYPE_LOAD,
	FILE_SCREEN_TYPE_SAVE,
};

#define	NUM_SCORES		15
#define	MAX_NAME_LENGTH	15

#define	SCORE_TEXT_SPACING	15.0f
#define	SCORE_DIGITS		9


typedef struct
{
	char			name[MAX_NAME_LENGTH+1];
	uint32_t		score;
}HighScoreType;

ObjNode *MakeDarkenPane(void);

void KeepTerrainAlive(void);
void DoPaused(void);

Boolean DoFailedMenu(const Str31	headerString);
void DoLegalScreen(void);
void DoGameOptionsDialog(void);

void DoBonusScreen(void);
void DoTitleScreen(void);
void MoveBumbleBeeOnSpline_Title(ObjNode *theNode);

void DoMainMenuScreen(void);
void DoLoseScreen(void);

void NewScore(void);
void LoadHighScores(void);
void ClearHighScores(void);
void DrawScoreText(const char* cstr, float x, float y);

void DoWinScreen(void);


		/* LEVEL INTROS */

void DoLevelIntroScreen_FrontYard(void);
void DoLevelIntroScreen_BackYard(void);
void DoLevelIntroScreen_Sewer(void);
void DoLevelIntroScreen_Playroom(void);
void DoLevelIntroScreen_Balsa(void);
void DoLevelIntroScreen_Fido(void);
void DoLevelIntroScreen_Closet(void);
void DoLevelIntroScreen_Gutter(void);
void DoLevelIntroScreen_Garbage(void);
void DoLevelIntroScreen_Park(void);
void DoLevelIntro(void);







void DoSettingsOverlay(void (*updateRoutine)(void),
					   void (*backgroundDrawRoutine)(void));


bool DoFileScreen(int fileScreenType, void (*backgroundDrawRoutine)(void));
