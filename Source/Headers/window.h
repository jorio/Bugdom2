//
// windows.h
//

#define	USE_DSP			1
#define	ALLOW_FADE		(1 && USE_DSP)


extern	float			gGammaFadePercent;

#if 0
extern	Boolean			gPlayFullScreen;
extern	GDHandle 		gGDevice;
extern	GrafPtr			gGameWindowGrafPtr;
extern	CGrafPtr		gDisplayContextGrafPtr;
extern	float			gGammaFadePercent;
extern	WindowRef 		gDialogWindow;
extern	EventHandlerUPP gWinEvtHandler;
#endif

//=================================


extern void	InitWindowStuff(void);
extern void	DumpGWorld2(GWorldPtr, WindowPtr, Rect *);
extern void	DoLockPixels(GWorldPtr);
#if 0
void MakeFadeEvent(Boolean fadeIn, float fadeSpeed);
void DumpGWorld2(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *destRect);
#endif

extern	void CleanupDisplay(void);
extern	void GammaFadeOut(void);
extern	void GammaFadeIn(void);
extern	void GammaOn(void);
void GammaOff(void);

extern	void GameScreenToBlack(void);

#if 0
void DoLockPixels(GWorldPtr world);
void DoScreenModeDialog(void);

void Wait(u_long ticks);
void DoScreenModeDialog(void);
#endif

void Enter2D(void);
void Exit2D(void);

#if 0
void BuildControlMenu(WindowRef window, SInt32 controlSig, SInt32 id, Str255 textList[], short numItems, short defaultSelection);
#endif
void HideRealCursor(void);
void ShowRealCursor(void);
