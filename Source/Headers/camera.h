//
// camera.h
//


void InitCamera_Terrain(void);
void InitCamera_Tunnel(void);

void UpdateCamera(void);
void DrawLensFlare(OGLSetupOutputType *setupInfo);


void PrepAnaglyphCameras(void);
void RestoreCamerasFromAnaglyph(void);
void CalcAnaglyphCameraOffset(short pass);
