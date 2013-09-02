//
// infobar.h
//



void InitInfobar(OGLSetupOutputType *setupInfo);
void DrawInfobar(OGLSetupOutputType *setupInfo);
void DisposeInfobar(void);
void DrawInfobarSprite(float x, float y, float size, short texNum, const OGLSetupOutputType *setupInfo);
void DrawInfobarSprite2_Centered(float x, float y, float size, short group, short texNum, const OGLSetupOutputType *setupInfo);
void DrawInfobarSprite2(float x, float y, float size, short group, short texNum, const OGLSetupOutputType *setupInfo);

void SetInfobarSpriteState(void);
