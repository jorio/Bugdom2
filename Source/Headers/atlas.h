#pragma once

enum
{
	ATLAS_GROUP_NULL				=	0,
	ATLAS_GROUP_FONT1				,
//	ATLAS_GROUP_FONT2				,
//	ATLAS_GROUP_FONT3				,
	MAX_ATLASES
};

// A "codepoint page" is a block of 256 codepoints. For Western European
// languages, we only need codepoint page 0, which covers Unicode blocks
// "Basic Latin" and "Latin-1 Supplement".
#define MAX_CODEPOINT_PAGES 64

#define MAX_KERNPAIRS		256

enum
{
	kTextMeshAlignCenter				= 0,		// default if horizontal alignment not set
	kTextMeshAlignLeft					= 1<<1,
	kTextMeshAlignRight					= 1<<2,
	kTextMeshAlignMiddle				= 0,		// default if vertical alignment not set
	kTextMeshAlignTop					= 1<<3,
	kTextMeshAlignBottom				= 1<<4,
	kTextMeshGlow						= 1<<5,
	kTextMeshKeepCurrentProjection		= 1<<6,
	kTextMeshAllCaps					= 1<<7,
	kTextMeshSmallCaps					= 1<<8,
	kTextMeshUserFlag1					= 1<<9,
};

enum
{
	kAtlasLoadAsSingleSprite	= 1,
	kAtlasLoadFont				= 2,
	kAtlasLoadAltSkin1			= 4,
	kAtlasLoadFontIsUpperCaseOnly	= 8,
};

typedef struct
{
	float w;
	float h;
	float xoff;
	float yoff;
	float xadv;
	float yadv;

	float u1;
	float v1;
	float u2;
	float v2;

	uint16_t	kernTableOffset;
	int8_t		numKernPairs;
} AtlasGlyph;

typedef struct Atlas
{
	char name[32];
	MOMaterialObject* material;
	int textureWidth;
	int textureHeight;
	float lineHeight;
	uint32_t maxPages;
	AtlasGlyph** glyphPages;

	uint16_t* kernPairs;
	uint8_t* kernTracking;

	bool isASCIIFont;
	bool isASCIIFontUpperCaseOnly;
} Atlas;

Atlas* Atlas_Load(const char* atlasName, int flags);
void Atlas_Dispose(Atlas* atlas);

const AtlasGlyph* Atlas_GetGlyph(const Atlas* atlas, uint32_t codepoint);


ObjNode* TextMesh_NewEmpty(int capacity, NewObjectDefinitionType *newObjDef);
ObjNode* TextMesh_New(const char *text, int flags, NewObjectDefinitionType *newObjDef);
void TextMesh_Update(const char* text, int flags, ObjNode* textNode);
OGLRect TextMesh_GetExtents(ObjNode* textNode);
void TextMesh_DrawExtents(ObjNode* textNode);

void Atlas_ImmediateDraw(int groupNum, const char* text, uint32_t flags);

void Atlas_DrawString2(
	int groupNum,
	const char* text,
	float x,
	float y,
	float scaleX,
	float scaleY,
	float rot,
	uint32_t flags);

#define GameFont_DrawString(text, x, y, scale, flags) \
	Atlas_DrawString2(ATLAS_GROUP_FONT1, text, x, y, scale, scale, 0, flags)

void LoadSpriteAtlas(int groupNum, const char* atlasName, int flags);
void DisposeSpriteAtlas(int groupNum);
void DisposeAllSpriteAtlases(void);
const AtlasGlyph* GetAtlasSpriteInfo(int groupNum, int spriteNum);
