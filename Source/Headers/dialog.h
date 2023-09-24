//
// dialog.h
//


#define	DEFAULT_DIALOG_ACTIVATE_DIST	300.0f


void InitDialogManager(void);


void DoDialogMessage(int messNum, int priority, float duration, OGLPoint3D *fromWhere);
void DrawDialogMessage(void);


int CharToSprite(uint32_t c);
float GetCharSpacing(uint32_t c, float spacingScale);
