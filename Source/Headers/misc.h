//
// misc.h
//

void	DoAlert(const char* fmt, ...);
SDL_NORETURN void	DoFatalAlert(const char* fmt, ...);
extern unsigned char	*NumToHex(unsigned short);
extern unsigned char	*NumToHex2(unsigned long, short);
extern unsigned char	*NumToDec(unsigned long);
SDL_NORETURN	void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
uint32_t MyRandomLong(void);
extern	Handle	AllocHandle(long size);
extern	void *AllocPtr(long size);
void *AllocPtrClear(long size);
extern	void DrawCString(char *string);
extern	void InitMyRandomSeed(void);
extern	float RandomFloat(void);
uint16_t	RandomRange(unsigned short min, unsigned short max);
void CalcFramesPerSecond(void);
Boolean IsPowerOf2(int num);
float RandomFloat2(void);

void SafeDisposePtr(void *ptr);
void MyFlushEvents(void);

int16_t SwizzleShort(const int16_t *shortPtr);
int32_t SwizzleLong(const int32_t *longPtr);
float SwizzleFloat(const float *floatPtr);
uint32_t SwizzleULong(const uint32_t *longPtr);
uint16_t SwizzleUShort(const uint16_t *shortPtr);
void SwizzlePoint3D(OGLPoint3D *pt);
void SwizzleVector3D(OGLVector3D *pt);
void SwizzleUV(OGLTextureCoord *pt);

static inline int PositiveModulo(int value, unsigned int m)
{
	int mod = value % (int) m;
	if (mod < 0)
	{
		mod += m;
	}
	return mod;
}

void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v );
void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v );

char* CSVIterator(char** csvCursor, bool* eolOut);

#if _DEBUG
#define IMPLEMENT_ME_SOFT() { static int _warnings = 0; if (!_warnings++) SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "IMPLEMENT ME: %s:%d\n", __func__, __LINE__); }
#else
#define IMPLEMENT_ME_SOFT()
#endif
#define IMPLEMENT_ME() DoFatalAlert("IMPLEMENT ME: %s:%d", __func__, __LINE__)

#define GAME_ASSERT(condition) do { if (!(condition)) DoFatalAlert("%s:%d: %s", __func__, __LINE__, #condition); } while(0)
#define GAME_ASSERT_MESSAGE(condition, message) do { if (!(condition)) DoFatalAlert("%s:%d: %s", __func__, __LINE__, message); } while(0)
