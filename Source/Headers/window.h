//
// windows.h
//

extern void	InitWindowStuff(void);

extern	void CleanupDisplay(void);
extern	void GammaFadeOut(void);
extern	void GammaFadeIn(void);
extern	void GammaOn(void);
void GammaOff(void);

extern	void GameScreenToBlack(void);

void Enter2D(void);
void Exit2D(void);

void HideRealCursor(void);
void ShowRealCursor(void);

void MakeFadeEvent(Boolean fadeIn, float fadeSpeed);

void GetDefaultWindowSize(int display, int* width, int* height);
void SetFullscreenMode(bool enforceDisplayPref);
