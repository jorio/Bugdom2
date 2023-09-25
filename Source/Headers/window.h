//
// windows.h
//

void InitWindowStuff(void);

void Enter2D(void);
void Exit2D(void);

void HideRealCursor(void);
void ShowRealCursor(void);

void GetDefaultWindowSize(int display, int* width, int* height);
void SetFullscreenMode(bool enforceDisplayPref);
