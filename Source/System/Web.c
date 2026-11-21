#ifdef __EMSCRIPTEN__
#include "game.h"

int WebGetKey(int key)
{
    switch (key)
    {
        case kNeed_TurnLeft: return WebGetButton(WEB_LEFT);
        case kNeed_TurnRight: return WebGetButton(WEB_RIGHT);
        case kNeed_Forward: return WebGetButton(WEB_UP);
        case kNeed_Backward: return WebGetButton(WEB_DOWN);
        case kNeed_Kick: return WebGetButton(WEB_KICK);
        case kNeed_Jump: return WebGetButton(WEB_JUMP);
        case kNeed_PickupDrop: return WebGetButton(WEB_PICKUP);
        case kNeed_LaunchBuddy: return WebGetButton(WEB_BUDDY);
        case kNeed_CameraLeft: return WebGetButton(WEB_CAMERA_LEFT);
        case kNeed_CameraRight: return WebGetButton(WEB_CAMERA_RIGHT);
        case kNeed_CameraMode: return WebGetButton(WEB_CAMERA_MODE);
        case kNeed_UIUp: return WebGetButton(WEB_UP);
        case kNeed_UIDown: return WebGetButton(WEB_DOWN);
        case kNeed_UIPrev: return WebGetButton(WEB_LEFT);
        case kNeed_UINext: return WebGetButton(WEB_RIGHT);
        case kNeed_UIConfirm: return WebGetButton(WEB_CONFIRM);
        case kNeed_UIStart: return WebGetButton(WEB_CONFIRM);
        case kNeed_UIBack: return WebGetButton(WEB_PAUSE);
        case kNeed_UIPause: return WebGetButton(WEB_PAUSE);
        default: return 0;
    }
}
float WebGetAnalog(int key)
{
    float x,y; WebGetMovement(&x,&y);
    switch (key)
    {
        case kNeed_TurnLeft: return fmaxf(0, -x);
        case kNeed_TurnRight: return fmaxf(0, x);
        case kNeed_Forward: return fmaxf(0, -y);
        case kNeed_Backward: return fmaxf(0, y);
        default: return WebGetKey(key) ? 1.0f : 0.0f;
    }
}
void WebGameResize(int width, int height) { gGameWindowWidth = width; gGameWindowHeight = height; }
void WebEndFrame(void) { WebPresent(WebIsGameplayActive(), gGamePaused); }
#endif
