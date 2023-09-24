//
// infobar.h
//



void InitInfobar(void);
void DrawInfobar(void);
void DisposeInfobar(void);
void DrawInfobarSprite(float x, float y, float size, short texNum);
void DrawInfobarSprite2_Centered(float x, float y, float size, short group, short texNum);
void DrawInfobarSprite2(float x, float y, float size, short group, short texNum);

void SetInfobarSpriteState(void);
