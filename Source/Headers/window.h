//
// windows.h
//

void InitWindowStuff(void);

void Enter2D(void);
void Exit2D(void);

int GetNumDisplays(void);
void GetDefaultWindowSize(SDL_DisplayID display, int* width, int* height);
void MoveToPreferredDisplay(void);
void SetFullscreenMode(bool enforceDisplayPref);
