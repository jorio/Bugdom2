//
// sprites.h
//



enum
{
	SPRITE_FLAG_GLOW = (1)
};


// Read in from file
typedef struct
{
	int32_t			width,height;
	float			aspectRatio;			// h/w
	int32_t			srcFormat, destFormat;
	MetaObjectPtr	materialObject;
}SpriteType;


void InitSpriteManager(void);
void DisposeAllSpriteGroups(void);
void DisposeSpriteGroup(int groupNum);
void LoadSpriteFile(FSSpec *spec, int groupNum);
ObjNode *MakeSpriteObject(NewObjectDefinitionType *newObjDef);
void BlendAllSpritesInGroup(short group);
void ModifySpriteObjectFrame(ObjNode *theNode, short type);
void DrawSprite(int	group, int type, float x, float y, float scale, float rot, uint32_t flags);
void BlendASprite(int group, int type);
