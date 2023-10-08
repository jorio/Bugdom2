//
// misc.h
//

void	DoAlert(const char* fmt, ...);
SDL_NORETURN void DoFatalAlert(const char* fmt, ...);
SDL_NORETURN void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
uint32_t MyRandomLong(void);
void *AllocPtr(long size);
void *AllocPtrClear(long size);
extern	void InitMyRandomSeed(void);
extern	float RandomFloat(void);
uint16_t	RandomRange(unsigned short min, unsigned short max);
void CalcFramesPerSecond(void);
void ResetFramesPerSecond(void);
Boolean IsPowerOf2(int num);
float RandomFloat2(void);

void SafeDisposePtr(void *ptr);
void MyFlushEvents(void);

int16_t SwizzleShort(const int16_t *shortPtr);
int32_t SwizzleLong(const int32_t *longPtr);
float SwizzleFloat(const float *floatPtr);
uint16_t SwizzleUShort(const uint16_t *shortPtr);
uint32_t SwizzleULong(const uint32_t* longPtr);
uint64_t SwizzleULong64(const uint64_t* longPtr);
void SwizzlePoint3D(OGLPoint3D *pt);
void SwizzleVector3D(OGLVector3D *pt);
void SwizzleUV(OGLTextureCoord *pt);

uint8_t FSReadByte(short refNum);
int16_t FSReadBEShort(short refNum);
uint16_t FSReadBEUShort(short refNum);
int32_t FSReadBELong(short refNum);
uint32_t FSReadBEULong(short refNum);
float FSReadBEFloat(short refNum);

#define SQUARED(x) ((x)*(x))

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

#define GAME_ASSERT(condition) do { if (!(condition)) DoFatalAlert("%s:%d: %s", __func__, __LINE__, #condition); } while(0)
#define GAME_ASSERT_MESSAGE(condition, message) do { if (!(condition)) DoFatalAlert("%s:%d: %s", __func__, __LINE__, message); } while(0)

#if _DEBUG
	#define GAME_DEBUGASSERT			GAME_ASSERT
	#define GAME_DEBUGASSERT_MESSAGE	GAME_ASSERT_MESSAGE
#else
	#define GAME_DEBUGASSERT(...)
	#define GAME_DEBUGASSERT_MESSAGE(...)
#endif

