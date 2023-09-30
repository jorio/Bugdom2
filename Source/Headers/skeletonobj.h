//
// SkeletonObj.h
//

#pragma once

enum
{
	SKELETON_TYPE_SKIP_EXPLORE = 0,
	SKELETON_TYPE_SKIP_TUNNEL,
	SKELETON_TYPE_SKIP_TITLE,
	SKELETON_TYPE_SNAIL,
	SKELETON_TYPE_GNOME,
	SKELETON_TYPE_HOUSEFLY,
	SKELETON_TYPE_EVILPLANT,
	SKELETON_TYPE_CHIPMUNK,
	SKELETON_TYPE_SNAKEHEAD,
	SKELETON_TYPE_BUDDYBUG,
	SKELETON_TYPE_CHECKPOINT,
	SKELETON_TYPE_FLEA,
	SKELETON_TYPE_TICK,
	SKELETON_TYPE_MOUSETRAP,
	SKELETON_TYPE_MOUSE,
	SKELETON_TYPE_TOYSOLDIER,
	SKELETON_TYPE_OTTO,
	SKELETON_TYPE_BUMBLEBEE,
	SKELETON_TYPE_HOBOBAG,
	SKELETON_TYPE_DRAGONFLY,
	SKELETON_TYPE_FROG,
	SKELETON_TYPE_MOTH,
	SKELETON_TYPE_COMPUTERBUG,
	SKELETON_TYPE_ROACH,
	SKELETON_TYPE_ANT,
	SKELETON_TYPE_FISH,

	MAX_SKELETON_TYPES
};




//===============================

ObjNode	*MakeNewSkeletonObject(NewObjectDefinitionType *newObjDef);
void AllocSkeletonDefinitionMemory(SkeletonDefType *skeleton);
void InitSkeletonManager(void);
void DisposeSkeletonManager(void);
void LoadASkeleton(Byte num);
extern	void FreeSkeletonFile(Byte skeletonType);
extern	void FreeAllSkeletonFiles(short skipMe);
extern	void FreeSkeletonBaseData(SkeletonObjDataType *data);
void DrawSkeleton(ObjNode *theNode);
