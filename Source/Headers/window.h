//
// windows.h
//

#define	USE_DSP			1
#define	ALLOW_FADE		(1 && USE_DSP)


extern	float			gGammaFadePercent;


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
