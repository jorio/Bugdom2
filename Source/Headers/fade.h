#pragma once

ObjNode* MakeFadeEvent(Boolean fadeIn, float fadeSpeed);
void OGL_FadeOutScene(void (*drawCall)(void), void (*moveCall)(void));
