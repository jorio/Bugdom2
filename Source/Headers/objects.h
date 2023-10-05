//
// Object.h
//

#define INVALID_NODE_FLAG	0xdeadbeef			// put into CType when node is deleted

enum
{
	TERRAIN_SLOT			= 1,
	CYC_SLOT				= 2,				// draw after terrain for better performance since terrain blocks much of the pixels
	TRIGGER_SLOT			= 4,				// needs to be early in the collision list
	FENCE_SLOT				= 4,
	LURE_SLOT				= 9,
	SCARECROW_SLOT			= 60,
	CHIPMUNK_SLOT			= 85,				// must be BEFORE powerup so that chains work correctly!
	POW_SLOT				= 87,
	PLAYER_SLOT				= 200,
	ENEMY_SLOT				= 210,
	SNAIL_SLOT				= 260,
	SILICONDOOR_SLOT		= 2970,
	SNAKE_SLOT				= 2997,
	SLOT_OF_DUMB			= 3000,				// ============== anything past this slot won't be collided against ===================
	RIPPLE_SLOT				= 3500,
	WATER_SLOT				= 3510,
	SHARDS_SLOT				= 3600, 
	CONFETTI_SLOT			= 3601,				// do confetti before particles since particles are xparent
	SPARKLES_SLOT			= 3602,
	PARTICLE_SLOT			= 3603,
	WATERPARK_SLOT			= 3604,				// in park, need to do water *after* particles so that bubbles are seen
	PLAYERWING_SLOT			= 3650,
	LENSFLARE_SLOT			= 3660,
	CLOUD_SLOT				= 3660,
	SPRITE_SLOT				= 3700,
	DARKENPANE_SLOT			= 3800, 
	INFOBAR_SLOT			= 3900,
	MENU_SLOT				= 3950,
	FADEPANE_SLOT			= 4000,
};

enum
{
	SKELETON_GENRE = 0,
	DISPLAY_GROUP_GENRE,
	SPRITE_GENRE,
	CUSTOM_GENRE,
	EVENT_GENRE,
	TEXTMESH_GENRE,
	QUADMESH_GENRE,
	ILLEGAL_GENRE = 0xFF,
};


enum
{
	SHADOW_TYPE_CIRCULAR,
	SHADOW_TYPE_BALSAPLANE,
	SHADOW_TYPE_CIRCULARDARK,
	SHADOW_TYPE_SQUARE
};


enum
{
	WHAT_UNDEFINED = 0,

	WHAT_SCARECROW,
	WHAT_SCARECROWHEAD,
	WHAT_SNAIL,
	WHAT_ACORN,
	WHAT_MARBLE,
	WHAT_RIDEBALL,
	WHAT_CHECKPOINT,
	WHAT_MOUSETRAP,
	WHAT_SOLDIER,
	WHAT_PUZZLE,
	WHAT_ANTHILL,
	WHAT_MOTHBALL,
	WHAT_KINDLING
};


#define	DEFAULT_GRAVITY		5000.0f


#define	ShadowScaleX	SpecialF[0]
#define	ShadowScaleZ	SpecialF[1]
#define	CheckForBlockers	Flag[0]

//========================================================

void InitObjectManager(void);
void DisposeObjectManager(void);
extern	ObjNode	*MakeNewObject(NewObjectDefinitionType *newObjDef);
extern	void MoveObjects(void);
void DrawObjects(void);

extern	void DeleteAllObjects(void);
extern	void DeleteObject(ObjNode	*theNode);
void DetachObject(ObjNode *theNode, Boolean subrecurse);
extern	void GetObjectInfo(ObjNode *theNode);
extern	void UpdateObject(ObjNode *theNode);
extern	ObjNode *MakeNewDisplayGroupObject(NewObjectDefinitionType *newObjDef);
extern	void AttachGeometryToDisplayGroupObject(ObjNode *theNode, MetaObjectPtr geometry);
extern	void CreateBaseGroup(ObjNode *theNode);
extern	void UpdateObjectTransforms(ObjNode *theNode);
extern	void SetObjectTransformMatrix(ObjNode *theNode);
extern	void DisposeObjectBaseGroup(ObjNode *theNode);
extern	void ResetDisplayGroupObject(ObjNode *theNode);
void AttachObject(ObjNode *theNode, Boolean recurse);

void MoveStaticObject(ObjNode *theNode);
void MoveStaticObject2(ObjNode *theNode);
void MoveStaticObject3(ObjNode *theNode);

void CalcNewTargetOffsets(ObjNode *theNode, float scale);

ObjNode* MakeNewDriverObject(int slot, void (*drawCall)(ObjNode *), void (*moveCall)(ObjNode *));

//===================


extern	void CalcObjectBoxFromNode(ObjNode *theNode);
extern	void CalcObjectBoxFromGlobal(ObjNode *theNode);
void SetObjectCollisionBounds(ObjNode *theNode, float top, float bottom, float left,
							 float right, float front, float back);
ObjNode	*AttachStaticShadowToObject(ObjNode *theNode, int shadowType, float scaleX, float scaleZ);
extern	void UpdateShadow(ObjNode *theNode);
extern	void CullTestAllObjects(void);
ObjNode	*AttachShadowToObject(ObjNode *theNode, int shadowType, float scaleX, float scaleZ, Boolean checkBlockers);
void CreateCollisionBoxFromBoundingBox(ObjNode *theNode, float tweakXZ, float tweakY);
void CreateCollisionBoxFromBoundingBox_Maximized(ObjNode *theNode);
void CreateCollisionBoxFromBoundingBox_Rotated(ObjNode *theNode, float tweakXZ, float tweakY);
void CreateCollisionBoxFromBoundingBox_Update(ObjNode *theNode, float tweakXZ, float tweakY);
ObjNode *FindClosestCType(OGLPoint3D *pt, uint32_t ctype, Boolean notThruSolid);
ObjNode *FindClosestCType3D(OGLPoint3D *pt, uint32_t ctype);
extern	void KeepOldCollisionBoxes(ObjNode *theNode);
void AddCollisionBoxToObject(ObjNode *theNode, float top, float bottom, float left,
							 float right, float front, float back);
void DoObjectFriction(ObjNode *theNode, float friction);







