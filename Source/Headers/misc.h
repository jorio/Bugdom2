//
// misc.h
//

#define SERIAL_LENGTH      12


extern	Boolean			gAltivec;


//======================================================


void	DoAlert(const char* fmt, ...);
void	DoFatalAlert(const char* fmt, ...);
extern unsigned char	*NumToHex(unsigned short);
extern unsigned char	*NumToHex2(unsigned long, short);
extern unsigned char	*NumToDec(unsigned long);
extern	void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
extern	unsigned long MyRandomLong(void);
extern	void FloatToString(float num, Str255 string);
extern	Handle	AllocHandle(long size);
extern	void *AllocPtr(long size);
void *AllocPtrClear(long size);
float StringToFloat(Str255 textStr);
extern	void DrawCString(char *string);
extern	void InitMyRandomSeed(void);
extern	float RandomFloat(void);
u_short	RandomRange(unsigned short min, unsigned short max);
void CalcFramesPerSecond(void);
Boolean IsPowerOf2(int num);
float RandomFloat2(void);

void SafeDisposePtr(void *ptr);
void MyFlushEvents(void);


short SwizzleShort(short *shortPtr);
long SwizzleLong(long *longPtr);
float SwizzleFloat(float *floatPtr);
u_long SwizzleULong(u_long *longPtr);
u_short SwizzleUShort(u_short *shortPtr);
void SwizzlePoint3D(OGLPoint3D *pt);
void SwizzleVector3D(OGLVector3D *pt);
void SwizzleUV(OGLTextureCoord *pt);

void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v );
void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v );

#if _DEBUG
#define IMPLEMENT_ME_SOFT() SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "IMPLEMENT ME: %s:%d\n", __func__, __LINE__)
#else
#define IMPLEMENT_ME_SOFT()
#endif
#define IMPLEMENT_ME() DoFatalAlert("IMPLEMENT ME: %s:%d", __func__, __LINE__)

#define GAME_ASSERT(condition) do { if (!(condition)) DoFatalAlert("%s:%d: %s", __func__, __LINE__, #condition); } while(0)
#define GAME_ASSERT_MESSAGE(condition, message) do { if (!(condition)) DoFatalAlert("%s:%d: %s", __func__, __LINE__, message); } while(0)
