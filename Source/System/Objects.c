/*********************************/
/*    OBJECT MANAGER 		     */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      	 */
/*********************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void FlushObjectDeleteQueue(void);
static void DrawCollisionBoxes(ObjNode *theNode, Boolean old);
static void DrawBoundingBoxes(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	OBJ_DEL_Q_SIZE	200
#define	MAX_POOLED_OBJS	2500


/**********************/
/*     VARIABLES      */
/**********************/

											// OBJECT LIST
ObjNode		*gFirstNodePtr = nil;

ObjNode		*gCurrentNode,*gMostRecentlyAddedNode,*gNextNode;


NewObjectDefinitionType	gNewObjectDefinition;

OGLPoint3D	gCoord;
OGLVector3D	gDelta;

float		gAutoFadeStartDist,gAutoFadeEndDist,gAutoFadeRange_Frac;

OGLMatrix4x4	*gCurrentObjMatrix;

int					gNumObjectNodes;
static Pool			*gObjectPool = NULL;
static ObjNode		gObjectMemory[MAX_POOLED_OBJS];
static int			gNumObjsInDeleteQueue = 0;
static ObjNode		*gObjectDeleteQueue[OBJ_DEL_Q_SIZE];

//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT CREATION ------

/************************ INIT OBJECT MANAGER **********************/

void InitObjectManager(void)
{

				/* INIT LINKED LIST */


	gCurrentNode = nil;

					/* CLEAR ENTIRE OBJECT LIST */

	gFirstNodePtr = nil;									// no node yet

	gNumObjectNodes = 0;
	gNumObjsInDeleteQueue = 0;


	GAME_ASSERT(!gObjectPool);
	gObjectPool = Pool_New(MAX_POOLED_OBJS);
}


void DisposeObjectManager(void)
{
	DeleteAllObjects();
	Pool_Free(gObjectPool);
	gObjectPool = NULL;
}


/*********************** MAKE NEW OBJECT ******************/
//
// MAKE NEW OBJECT & RETURN PTR TO IT
//
// The linked list is sorted from smallest to largest!
//

ObjNode	*MakeNewObject(NewObjectDefinitionType *newObjDef)
{
ObjNode	*newNodePtr;
long	slot;
uint32_t flags = newObjDef->flags;

	int pooledIndex = Pool_AllocateIndex(gObjectPool);
	if (pooledIndex < 0)
	{
				/* ALLOCATE NEW NODE(CLEARED TO 0'S) */

		newNodePtr = (ObjNode *)AllocPtrClear(sizeof(ObjNode));
		SDL_Log("ObjNode pool full (%d). Allocating ObjNode on heap.\n", MAX_POOLED_OBJS);
	}
	else
	{
		newNodePtr = &gObjectMemory[pooledIndex];
		SDL_zerop(newNodePtr);
	}


	GAME_ASSERT(newNodePtr);
	newNodePtr->pooledIndex = pooledIndex;





			/* INITIALIZE ALL OF THE FIELDS */

	slot = newObjDef->slot;

	newNodePtr->Slot 		= slot;
	newNodePtr->Type 		= newObjDef->type;
	newNodePtr->Group 		= newObjDef->group;
	newNodePtr->MoveCall 	= newObjDef->moveCall;
	newNodePtr->CustomDrawFunction	= newObjDef->drawCall;

	if (flags & STATUS_BIT_ONSPLINE)
		newNodePtr->SplineMoveCall = newObjDef->moveCall;				// save spline move routine
	else
		newNodePtr->SplineMoveCall = nil;

	newNodePtr->Genre = newObjDef->genre;
	newNodePtr->Coord = newNodePtr->InitCoord = newNodePtr->OldCoord = newObjDef->coord;		// save coords
	newNodePtr->StatusBits = flags;

	for (int i = 0; i < MAX_NODE_SPARKLES; i++)								// no sparkles
		newNodePtr->Sparkles[i] = -1;


	newNodePtr->BBox.isEmpty = true;


	newNodePtr->Rot.y =  newObjDef->rot;
	newNodePtr->Scale.x =
	newNodePtr->Scale.y =
	newNodePtr->Scale.z = newObjDef->scale;


	newNodePtr->BoundingSphereRadius = 100;


	newNodePtr->EffectChannel = -1;						// no streaming sound effect
	newNodePtr->ParticleGroup = -1;						// no particle group

	newNodePtr->SplineObjectIndex = -1;					// no index yet

	newNodePtr->ColorFilter.r =
	newNodePtr->ColorFilter.g =
	newNodePtr->ColorFilter.b =
	newNodePtr->ColorFilter.a = 1.0;

			/* MAKE SURE SCALE != 0 */

	if (newNodePtr->Scale.x == 0.0f)
		newNodePtr->Scale.x = 0.0001f;
	if (newNodePtr->Scale.y == 0.0f)
		newNodePtr->Scale.y = 0.0001f;
	if (newNodePtr->Scale.z == 0.0f)
		newNodePtr->Scale.z = 0.0001f;


				/* INSERT NODE INTO LINKED LIST */

	newNodePtr->StatusBits |= STATUS_BIT_DETACHED;		// its not attached to linked list yet
	AttachObject(newNodePtr, false);

	gNumObjectNodes++;


				/* CLEANUP */

	gMostRecentlyAddedNode = newNodePtr;					// remember this
	return(newNodePtr);
}

/************* MAKE NEW DISPLAY GROUP OBJECT *************/
//
// This is an ObjNode who's BaseGroup is a group, therefore these objects
// can be transformed (scale,rot,trans).
//

ObjNode *MakeNewDisplayGroupObject(NewObjectDefinitionType *newObjDef)
{
ObjNode	*newObj;
Byte	group,type;


	newObjDef->genre = DISPLAY_GROUP_GENRE;

	newObj = MakeNewObject(newObjDef);
	GAME_ASSERT(newObj);

			/* MAKE BASE GROUP & ADD GEOMETRY TO IT */

	CreateBaseGroup(newObj);											// create group object
	group = newObjDef->group;											// get group #
	type = newObjDef->type;												// get type #
	GAME_ASSERT(type < gNumObjectsInBG3DGroupList[group]);				// see if illegal

	AttachGeometryToDisplayGroupObject(newObj,gBG3DGroupList[group][type]);


			/* SET BOUNDING BOX */

	newObj->BBox = gObjectGroupBBoxList[group][type];	// get this model's local bounding box


	return(newObj);
}




/******************* RESET DISPLAY GROUP OBJECT *********************/
//
// If the ObjNode's "Type" field has changed, call this to dispose of
// the old BaseGroup and create a new one with the correct model attached.
//

void ResetDisplayGroupObject(ObjNode *theNode)
{
	DisposeObjectBaseGroup(theNode);									// dispose of old group
	CreateBaseGroup(theNode);											// create new group object

	if (theNode->Type >= gNumObjectsInBG3DGroupList[theNode->Group])							// see if illegal
		DoFatalAlert("ResetDisplayGroupObject: type > gNumObjectsInGroupList[]!");

	AttachGeometryToDisplayGroupObject(theNode,gBG3DGroupList[theNode->Group][theNode->Type]);	// attach geometry to group

			/* SET BOUNDING BOX */

	theNode->BBox = gObjectGroupBBoxList[theNode->Group][theNode->Type];
}



/************************* ADD GEOMETRY TO DISPLAY GROUP OBJECT ***********************/
//
// Attaches a geometry object to the BaseGroup object. MakeNewDisplayGroupObject must have already been
// called which made the group & transforms.
//

void AttachGeometryToDisplayGroupObject(ObjNode *theNode, MetaObjectPtr geometry)
{
	MO_AppendToGroup(theNode->BaseGroup, geometry);
}



/***************** CREATE BASE GROUP **********************/
//
// The base group contains the base transform matrix plus any other objects you want to add into it.
// This routine creates the new group and then adds a transform matrix.
//
// The base is composed of BaseGroup & BaseTransformObject.
//

void CreateBaseGroup(ObjNode *theNode)
{
OGLMatrix4x4			transMatrix,scaleMatrix,rotMatrix;
MOMatrixObject			*transObject;


				/* CREATE THE BASE GROUP OBJECT */

	theNode->BaseGroup = MO_CreateNewObjectOfType(MO_TYPE_GROUP, 0, nil);
	if (theNode->BaseGroup == nil)
		DoFatalAlert("CreateBaseGroup: MO_CreateNewObjectOfType failed!");


					/* SETUP BASE MATRIX */

	if ((theNode->Scale.x == 0) || (theNode->Scale.y == 0) || (theNode->Scale.z == 0))
		DoFatalAlert("CreateBaseGroup: A scale component == 0");


	OGLMatrix4x4_SetScale(&scaleMatrix, theNode->Scale.x, theNode->Scale.y,		// make scale matrix
							theNode->Scale.z);

	OGLMatrix4x4_SetRotate_XYZ(&rotMatrix, theNode->Rot.x, theNode->Rot.y,		// make rotation matrix
								 theNode->Rot.z);

	OGLMatrix4x4_SetTranslate(&transMatrix, theNode->Coord.x, theNode->Coord.y,	// make translate matrix
							 theNode->Coord.z);

	OGLMatrix4x4_Multiply(&scaleMatrix,											// mult scale & rot matrices
						 &rotMatrix,
						 &theNode->BaseTransformMatrix);

	OGLMatrix4x4_Multiply(&theNode->BaseTransformMatrix,						// mult by trans matrix
						 &transMatrix,
						 &theNode->BaseTransformMatrix);


					/* CREATE A MATRIX XFORM */

	transObject = MO_CreateNewObjectOfType(MO_TYPE_MATRIX, 0, &theNode->BaseTransformMatrix);	// make matrix xform object
	if (transObject == nil)
		DoFatalAlert("CreateBaseGroup: MO_CreateNewObjectOfType/Matrix Failed!");

	MO_AttachToGroupStart(theNode->BaseGroup, transObject);						// add to base group
	theNode->BaseTransformObject = transObject;									// keep extra LEGAL ref (remember to dispose later)
}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT PROCESSING ------


/*******************************  MOVE OBJECTS **************************/

void MoveObjects(void)
{
ObjNode		*thisNodePtr;


	if (gFirstNodePtr == nil)								// see if there are any objects
		return;

	thisNodePtr = gFirstNodePtr;

	do
	{
				/* VERIFY NODE */

		GAME_ASSERT(thisNodePtr->CType != INVALID_NODE_FLAG);

		gCurrentNode = thisNodePtr;							// set current object node
		gNextNode	 = thisNodePtr->NextNode;				// get next node now (cuz current node might get deleted)


		if (gGamePaused && !(thisNodePtr->StatusBits & STATUS_BIT_MOVEINPAUSE))
			goto next;


				/* UPDATE ANIMATION */

		UpdateSkeletonAnimation(thisNodePtr);


					/* NEXT TRY TO MOVE IT */

		if (!(thisNodePtr->StatusBits & STATUS_BIT_ONSPLINE)		// make sure don't call a move call if on spline
			&& (!(thisNodePtr->StatusBits & STATUS_BIT_NOMOVE))
			&& (thisNodePtr->MoveCall != nil))
		{
			KeepOldCollisionBoxes(thisNodePtr);				// keep old boxes & other stuff
			thisNodePtr->MoveCall(thisNodePtr);				// call object's move routine
		}

next:
		thisNodePtr = gNextNode;							// next node
	}
	while (thisNodePtr != nil);



			/* FLUSH THE DELETE QUEUE */

	FlushObjectDeleteQueue();
}



/**************************** DRAW OBJECTS ***************************/

void DrawObjects(void)
{
ObjNode		*theNode;
unsigned long	statusBits;
Boolean			noLighting = false;
Boolean			noZBuffer = false;
Boolean			noZWrites = false;
Boolean			glow = false;
Boolean			noCullFaces = false;
Boolean			texWrap = false;
Boolean			noFog = false;
Boolean			clipAlpha= false;
float			cameraX, cameraZ;


	if (gFirstNodePtr == nil)									// see if there are any objects
		return;


				/* FIRST DO OUR CULLING */

	CullTestAllObjects();

	theNode = gFirstNodePtr;


			/* GET CAMERA COORDS */

	cameraX = gGameView->cameraPlacement.cameraLocation.x;
	cameraZ = gGameView->cameraPlacement.cameraLocation.z;

			/***********************/
			/* MAIN NODE TASK LOOP */
			/***********************/
	do
	{
		statusBits = theNode->StatusBits;						// get obj's status bits

		if (statusBits & (STATUS_BIT_ISCULLED|STATUS_BIT_HIDDEN))	// see if is culled or hidden
			goto next;


		if (theNode->CType == INVALID_NODE_FLAG)				// see if already deleted
			goto next;

		gGlobalTransparency = theNode->ColorFilter.a;			// get global transparency
		if (gGlobalTransparency <= 0.0f)						// see if invisible
			goto next;

		gGlobalColorFilter.r = theNode->ColorFilter.r;			// set color filter
		gGlobalColorFilter.g = theNode->ColorFilter.g;
		gGlobalColorFilter.b = theNode->ColorFilter.b;


			/******************/
			/* CHECK AUTOFADE */
			/******************/

		if (gAutoFadeStartDist != 0.0f)							// see if this level has autofade
		{
			if (statusBits & STATUS_BIT_AUTOFADE)
			{
				float		dist;

				dist = CalcQuickDistance(cameraX, cameraZ, theNode->Coord.x, theNode->Coord.z);			// see if in fade zone

				if (!theNode->BBox.isEmpty)
					dist += (theNode->BBox.max.x - theNode->BBox.min.x) * .2f;		// adjust dist based on size of object in order to fade big objects closer

				if (dist >= gAutoFadeStartDist)
				{
					dist -= gAutoFadeStartDist;							// calc xparency %
					dist *= gAutoFadeRange_Frac;
					if (dist < 0.0f)
						goto next;

					gGlobalTransparency -= dist;
					if (gGlobalTransparency <= 0.0f)
					{
						theNode->StatusBits |= STATUS_BIT_ISCULLED;		// set culled flag to that any related Sparkles wont be drawn either
						goto next;
					}
				}
			}
		}


			/*******************/
			/* CHECK BACKFACES */
			/*******************/

		if (statusBits & STATUS_BIT_DOUBLESIDED)
		{
			if (!noCullFaces)
			{
				glDisable(GL_CULL_FACE);
				glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
				noCullFaces = true;
			}
		}
		else
		if (noCullFaces)
		{
			noCullFaces = false;
			glEnable(GL_CULL_FACE);
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
		}


			/*********************/
			/* CHECK NULL SHADER */
			/*********************/

		if (statusBits & STATUS_BIT_NOLIGHTING)
		{
			if (!noLighting)
			{
				OGL_DisableLighting();
				noLighting = true;
			}
		}
		else
		if (noLighting)
		{
			noLighting = false;
			OGL_EnableLighting();
		}

 			/****************/
			/* CHECK NO FOG */
			/****************/

		if (gGameView->useFog)
		{
			if (statusBits & STATUS_BIT_NOFOG)
			{
				if (!noFog)
				{
					glDisable(GL_FOG);
					noFog = true;
				}
			}
			else
			if (noFog)
			{
				noFog = false;
				glEnable(GL_FOG);
			}
		}

			/********************/
			/* CHECK GLOW BLEND */
			/********************/

		if (statusBits & STATUS_BIT_GLOW)
		{
			if (!glow)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				glow = true;
			}
		}
		else
		if (glow)
		{
			glow = false;
		    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}


			/**********************/
			/* CHECK TEXTURE WRAP */
			/**********************/

		if (statusBits & STATUS_BIT_NOTEXTUREWRAP)
		{
			if (!texWrap)
			{
				gGlobalMaterialFlags |= BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V;
				texWrap = true;
			}
		}
		else
		if (texWrap)
		{
			texWrap = false;
		    gGlobalMaterialFlags &= ~(BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V);
		}




			/****************/
			/* CHECK ZWRITE */
			/****************/

		if (statusBits & STATUS_BIT_NOZWRITES)
		{
			if (!noZWrites)
			{
				glDepthMask(GL_FALSE);
				noZWrites = true;
			}
		}
		else
		if (noZWrites)
		{
			glDepthMask(GL_TRUE);
			noZWrites = false;
		}


			/*****************/
			/* CHECK ZBUFFER */
			/*****************/

		if (statusBits & STATUS_BIT_NOZBUFFER)
		{
			if (!noZBuffer)
			{
				glDisable(GL_DEPTH_TEST);
				noZBuffer = true;
			}
		}
		else
		if (noZBuffer)
		{
			noZBuffer = false;
			glEnable(GL_DEPTH_TEST);
		}


			/*****************************/
			/* CHECK EDGE ALPHA CLIPPING */
			/*****************************/

		if ((statusBits & STATUS_BIT_CLIPALPHA) && (gGlobalTransparency == 1.0f))
		{
			if (!clipAlpha)
			{
				glAlphaFunc(GL_EQUAL, 1);	// draw any pixel who's Alpha == 1, skip semi-transparent pixels
				clipAlpha = true;
			}
		}
		else
		if (clipAlpha)
		{
			clipAlpha = false;
			glAlphaFunc(GL_NOTEQUAL, 0);	// draw any pixel who's Alpha != 0
		}


			/* AIM AT CAMERA */

		if (statusBits & STATUS_BIT_AIMATCAMERA)
		{
			theNode->Rot.y = PI+CalcYAngleFromPointToPoint(theNode->Rot.y,
														theNode->Coord.x, theNode->Coord.z,
														cameraX, cameraZ);

			UpdateObjectTransforms(theNode);

		}



			/************************/
			/* SHOW COLLISION BOXES */
			/************************/

		if (gDebugMode == 2)
		{
			DrawCollisionBoxes(theNode,false);
//			DrawBoundingBoxes(theNode);
		}


			/***************************/
			/* SEE IF DO U/V TRANSFORM */
			/***************************/

		if (statusBits & STATUS_BIT_UVTRANSFORM)
		{
			glMatrixMode(GL_TEXTURE);					// set texture matrix
			glTranslatef(theNode->TextureTransformU, theNode->TextureTransformV, 0);
			glMatrixMode(GL_MODELVIEW);
		}




			/***********************/
			/* SUBMIT THE GEOMETRY */
			/***********************/

		gCurrentObjMatrix = &theNode->BaseTransformMatrix;			// get global pointer to our matrix

 		if (noLighting || (theNode->Scale.y == 1.0f))				// if scale == 1 or no lighting, then dont need to normalize vectors
 			glDisable(GL_NORMALIZE);
 		else
 			glEnable(GL_NORMALIZE);

		if (theNode->CustomDrawFunction)							// if has custom draw function, then override and use that
			goto custom_draw;

		switch(theNode->Genre)
		{

			case	SKELETON_GENRE:
					DrawSkeleton(theNode);
					break;

			case	DISPLAY_GROUP_GENRE:
			case	QUADMESH_GENRE:
					if (theNode->BaseGroup)
					{
						MO_DrawObject(theNode->BaseGroup);
					}
					break;


			case	SPRITE_GENRE:
					if (theNode->SpriteMO)
					{
						OGL_PushState();								// keep state

						SetInfobarSpriteState();

						theNode->SpriteMO->objectData.coord = theNode->Coord;	// update Meta Object's coord info
						theNode->SpriteMO->objectData.scaleX = theNode->Scale.x;
						theNode->SpriteMO->objectData.scaleY = theNode->Scale.y;
						theNode->SpriteMO->objectData.rot = theNode->Rot.y;

						MO_DrawObject(theNode->SpriteMO);
						OGL_PopState();									// restore state
					}
					break;

			case	TEXTMESH_GENRE:
					if (theNode->BaseGroup)
					{
						OGL_PushState();	//--
						SetInfobarSpriteState();	//--

						MO_DrawObject(theNode->BaseGroup);

						if (gDebugMode >= 2)
						{
							TextMesh_DrawExtents(theNode);
						}

						OGL_PopState();	//--
					}
					break;

			case	CUSTOM_GENRE:
custom_draw:
					if (theNode->CustomDrawFunction)
					{
						theNode->CustomDrawFunction(theNode);
					}
					break;
		}


				/***************************/
				/* SEE IF END UV TRANSFORM */
				/***************************/

		if (statusBits & STATUS_BIT_UVTRANSFORM)
		{
			glMatrixMode(GL_TEXTURE);					// set texture matrix
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
		}



			/* NEXT NODE */
next:
		theNode = theNode->NextNode;
	}while (theNode != nil);


				/*****************************/
				/* RESET SETTINGS TO DEFAULT */
				/*****************************/

	if (noLighting)
		OGL_EnableLighting();

	if (gGameView->useFog && noFog)
		glEnable(GL_FOG);

	if (glow)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (noZBuffer)
	{
		glEnable(GL_DEPTH_TEST);
//		glDepthMask(GL_TRUE);
	}

	if (noZWrites)
		glDepthMask(GL_TRUE);

	if (noCullFaces)
	{
		glEnable(GL_CULL_FACE);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
	}

	if (texWrap)
	    gGlobalMaterialFlags &= ~(BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V);

	if (clipAlpha)
		glAlphaFunc(GL_NOTEQUAL, 0);


	gGlobalTransparency = 			// reset this in case it has changed
	gGlobalColorFilter.r =
	gGlobalColorFilter.g =
	gGlobalColorFilter.b = 1.0;

	glEnable(GL_NORMALIZE);
}


/************************ DRAW COLLISION BOXES ****************************/

static void DrawCollisionBoxes(ObjNode *theNode, Boolean old)
{
int					n,i;
CollisionBoxType	*c;
float				left,right,top,bottom,front,back;


			/* SET LINE MATERIAL */

	glDisable(GL_TEXTURE_2D);


		/* SCAN EACH COLLISION BOX */

	n = theNode->NumCollisionBoxes;							// get # collision boxes
	c = &theNode->CollisionBoxes[0];						// pt to array

	for (i = 0; i < n; i++)
	{
			/* GET BOX PARAMS */

		if (old)
		{
			left 	= c[i].oldLeft;
			right 	= c[i].oldRight;
			top 	= c[i].oldTop;
			bottom 	= c[i].oldBottom;
			front 	= c[i].oldFront;
			back 	= c[i].oldBack;
		}
		else
		{
			left 	= c[i].left;
			right 	= c[i].right;
			top 	= c[i].top;
			bottom 	= c[i].bottom;
			front 	= c[i].front;
			back 	= c[i].back;
		}

			/* DRAW TOP */

		glBegin(GL_LINE_LOOP);
		glColor3f(1,0,0);
		glVertex3f(left, top, back);
		glColor3f(1,1,0);
		glVertex3f(left, top, front);
		glVertex3f(right, top, front);
		glColor3f(1,0,0);
		glVertex3f(right, top, back);
		glEnd();

			/* DRAW BOTTOM */

		glBegin(GL_LINE_LOOP);
		glColor3f(1,0,0);
		glVertex3f(left, bottom, back);
		glColor3f(1,1,0);
		glVertex3f(left, bottom, front);
		glVertex3f(right, bottom, front);
		glColor3f(1,0,0);
		glVertex3f(right, bottom, back);
		glEnd();


			/* DRAW LEFT */

		glBegin(GL_LINE_LOOP);
		glColor3f(1,0,0);
		glVertex3f(left, top, back);
		glColor3f(1,0,0);
		glVertex3f(left, bottom, back);
		glColor3f(1,1,0);
		glVertex3f(left, bottom, front);
		glVertex3f(left, top, front);
		glEnd();


			/* DRAW RIGHT */

		glBegin(GL_LINE_LOOP);
		glColor3f(1,0,0);
		glVertex3f(right, top, back);
		glVertex3f(right, bottom, back);
		glColor3f(1,1,0);
		glVertex3f(right, bottom, front);
		glVertex3f(right, top, front);
		glEnd();

	}

	glColor4f(1,1,1,1);
}

/************************ DRAW BOUNDING BOXES ****************************/

static void DrawBoundingBoxes(ObjNode *theNode)
{
float	left,right,top,bottom,front,back;
int		i;

			/* SET LINE MATERIAL */

	glDisable(GL_TEXTURE_2D);


			/***************************/
			/* TRANSFORM BBOX TO WORLD */
			/***************************/

	if (theNode->Genre == SKELETON_GENRE)			// skeletons are already oriented, just need translation
	{
		left 	= theNode->BBox.min.x + theNode->Coord.x;
		right 	= theNode->BBox.max.x + theNode->Coord.x;
		top 	= theNode->BBox.max.y + theNode->Coord.y;
		bottom 	= theNode->BBox.min.y + theNode->Coord.y;
		front 	= theNode->BBox.max.z + theNode->Coord.z;
		back 	= theNode->BBox.min.z + theNode->Coord.z;
	}
	else											// non-skeletons need full transform
	{
		OGLPoint3D	corners[8];

		corners[0].x = theNode->BBox.min.x;			// top far left
		corners[0].y = theNode->BBox.max.y;
		corners[0].z = theNode->BBox.min.z;

		corners[1].x = theNode->BBox.max.x;			// top far right
		corners[1].y = theNode->BBox.max.y;
		corners[1].z = theNode->BBox.min.z;

		corners[2].x = theNode->BBox.max.x;			// top near right
		corners[2].y = theNode->BBox.max.y;
		corners[2].z = theNode->BBox.max.z;

		corners[3].x = theNode->BBox.min.x;			// top near left
		corners[3].y = theNode->BBox.max.y;
		corners[3].z = theNode->BBox.max.z;

		corners[4].x = theNode->BBox.min.x;			// bottom far left
		corners[4].y = theNode->BBox.min.y;
		corners[4].z = theNode->BBox.min.z;

		corners[5].x = theNode->BBox.max.x;			// bottom far right
		corners[5].y = theNode->BBox.min.y;
		corners[5].z = theNode->BBox.min.z;

		corners[6].x = theNode->BBox.max.x;			// bottom near right
		corners[6].y = theNode->BBox.min.y;
		corners[6].z = theNode->BBox.max.z;

		corners[7].x = theNode->BBox.min.x;			// bottom near left
		corners[7].y = theNode->BBox.min.y;
		corners[7].z = theNode->BBox.max.z;

		OGLPoint3D_TransformArray(corners, &theNode->BaseTransformMatrix, corners, 8);

					/* FIND NEW BOUNDS */

		left 	= corners[0].x;
		for (i = 1; i < 8; i ++)
			if (corners[i].x < left)
				left = corners[i].x;

		right 	= corners[0].x;
		for (i = 1; i < 8; i ++)
			if (corners[i].x > right)
				right = corners[i].x;

		bottom 	= corners[0].y;
		for (i = 1; i < 8; i ++)
			if (corners[i].y < bottom)
				bottom = corners[i].y;

		top 	= corners[0].y;
		for (i = 1; i < 8; i ++)
			if (corners[i].y > top)
				top = corners[i].y;

		back 	= corners[0].z;
		for (i = 1; i < 8; i ++)
			if (corners[i].z < back)
				back = corners[i].z;


		front 	= corners[0].z;
		for (i = 1; i < 8; i ++)
			if (corners[i].z > front)
				front = corners[i].z;
	}


		/* DRAW TOP */

	glBegin(GL_LINE_LOOP);
	glColor3f(1,0,0);
	glVertex3f(left, top, back);
	glColor3f(1,1,0);
	glVertex3f(left, top, front);
	glVertex3f(right, top, front);
	glColor3f(1,0,0);
	glVertex3f(right, top, back);
	glEnd();

		/* DRAW BOTTOM */

	glBegin(GL_LINE_LOOP);
	glColor3f(1,0,0);
	glVertex3f(left, bottom, back);
	glColor3f(1,1,0);
	glVertex3f(left, bottom, front);
	glVertex3f(right, bottom, front);
	glColor3f(1,0,0);
	glVertex3f(right, bottom, back);
	glEnd();


		/* DRAW LEFT */

	glBegin(GL_LINE_LOOP);
	glColor3f(1,0,0);
	glVertex3f(left, top, back);
	glColor3f(1,0,0);
	glVertex3f(left, bottom, back);
	glColor3f(1,1,0);
	glVertex3f(left, bottom, front);
	glVertex3f(left, top, front);
	glEnd();


		/* DRAW RIGHT */

	glBegin(GL_LINE_LOOP);
	glColor3f(1,0,0);
	glVertex3f(right, top, back);
	glVertex3f(right, bottom, back);
	glColor3f(1,1,0);
	glVertex3f(right, bottom, front);
	glVertex3f(right, top, front);
	glEnd();
}


/********************* MOVE STATIC OBJECT **********************/

void MoveStaticObject(ObjNode *theNode)
{

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	UpdateShadow(theNode);										// prime it
}

/********************* MOVE STATIC OBJECT2 **********************/
//
// Keeps object conformed to terrain curves
//

void MoveStaticObject2(ObjNode *theNode)
{

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	RotateOnTerrain(theNode, 0, nil);							// set transform matrix
	SetObjectTransformMatrix(theNode);

	CalcObjectBoxFromNode(theNode);

	UpdateShadow(theNode);										// prime it
}


/********************* MOVE STATIC OBJECT 3 **********************/
//
// Keeps at current terrain height
//

void MoveStaticObject3(ObjNode *theNode)
{

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - (theNode->BBox.min.y * theNode->Scale.y);

	UpdateObjectTransforms(theNode);

	if (theNode->NumCollisionBoxes > 0)
		CalcObjectBoxFromNode(theNode);

	UpdateShadow(theNode);										// prime it
}



//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT DELETING ------


/******************** DELETE ALL OBJECTS ********************/

void DeleteAllObjects(void)
{
	GAME_ASSERT(gObjectPool);

	while (gFirstNodePtr != nil)
		DeleteObject(gFirstNodePtr);

	FlushObjectDeleteQueue();
	GAME_ASSERT(gNumObjsInDeleteQueue == 0);

	GAME_ASSERT(Pool_Empty(gObjectPool));
}


/************************ DELETE OBJECT ****************/

void DeleteObject(ObjNode	*theNode)
{
	if (theNode == nil)								// see if passed a bogus node
		return;

	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Attempted to double delete object");
		return;
	}

			/* RECURSIVE DELETE OF CHAIN NODE & SHADOW NODE */
			//
			// should do these first so that base node will have appropriate nextnode ptr
			// since it's going to be used next pass thru the moveobjects loop.  This
			// assumes that all chained nodes are later in list.
			//

	if (theNode->ChainNode)
		DeleteObject(theNode->ChainNode);

	if (theNode->ShadowNode)
		DeleteObject(theNode->ShadowNode);


			/* SEE IF NEED TO FREE UP SPECIAL MEMORY */

	switch(theNode->Genre)
	{
		case	SKELETON_GENRE:
				FreeSkeletonBaseData(theNode->Skeleton);		// purge all alloced memory for skeleton data
				theNode->Skeleton = nil;
				break;

		case	SPRITE_GENRE:
				MO_DisposeObjectReference(theNode->SpriteMO);	// dispose reference to sprite meta object
		   		theNode->SpriteMO = nil;
				break;
	}

	for (int i = 0; i < MAX_NODE_SPARKLES; i++)				// free sparkles
	{
		if (theNode->Sparkles[i] != -1)
		{
			DeleteSparkle(theNode->Sparkles[i]);
			theNode->Sparkles[i] = -1;
		}
	}

			/* SEE IF STOP SOUND CHANNEL */

	StopAChannel(&theNode->EffectChannel);


		/* SEE IF NEED TO DEREFERENCE A QD3D OBJECT */

	DisposeObjectBaseGroup(theNode);


			/* REMOVE NODE FROM LINKED LIST */

	DetachObject(theNode, false);

	GAME_ASSERT(theNode != gFirstNodePtr);


			/* SEE IF MARK AS NOT-IN-USE IN ITEM LIST */

	if (theNode->TerrainItemPtr)
	{
		theNode->TerrainItemPtr->flags &= ~ITEM_FLAGS_INUSE;		// clear the "in use" flag
	}

		/* OR, IF ITS A SPLINE ITEM, THEN UPDATE SPLINE OBJECT LIST */

	if (theNode->StatusBits & STATUS_BIT_ONSPLINE)
	{
		RemoveFromSplineObjectList(theNode);
	}


			/* DELETE THE NODE BY ADDING TO DELETE QUEUE */

	theNode->CType = INVALID_NODE_FLAG;							// INVALID_NODE_FLAG indicates its deleted


	gObjectDeleteQueue[gNumObjsInDeleteQueue++] = theNode;
	if (gNumObjsInDeleteQueue >= OBJ_DEL_Q_SIZE)
	{
		FlushObjectDeleteQueue();
		GAME_ASSERT(gNumObjsInDeleteQueue == 0);
	}
}


/****************** DETACH OBJECT ***************************/
//
// Simply detaches the objnode from the linked list, patches the links
// and keeps the original objnode intact.
//

void DetachObject(ObjNode *theNode, Boolean subrecurse)
{
	if (theNode == nil)
		return;

	if (theNode->StatusBits & STATUS_BIT_DETACHED)	// make sure not already detached
		return;

	if (theNode == gNextNode)						// if its the next node to be moved, then fix things
		gNextNode = theNode->NextNode;

	if (theNode->PrevNode == nil)					// special case 1st node
	{
		gFirstNodePtr = theNode->NextNode;
		if (gFirstNodePtr)
			gFirstNodePtr->PrevNode = nil;
	}
	else
	if (theNode->NextNode == nil)					// special case last node
	{
		theNode->PrevNode->NextNode = nil;
	}
	else											// generic middle deletion
	{
		theNode->PrevNode->NextNode = theNode->NextNode;
		theNode->NextNode->PrevNode = theNode->PrevNode;
	}

	theNode->PrevNode = nil;						// seal links on original node
	theNode->NextNode = nil;

	theNode->StatusBits |= STATUS_BIT_DETACHED;

			/* SUBRECURSE CHAINS & SHADOW */

	if (subrecurse)
	{
		if (theNode->ChainNode)
			DetachObject(theNode->ChainNode, subrecurse);

		if (theNode->ShadowNode)
			DetachObject(theNode->ShadowNode, subrecurse);
	}
}


/****************** ATTACH OBJECT ***************************/

void AttachObject(ObjNode *theNode, Boolean recurse)
{
short	slot;

	if (theNode == nil)
		return;

	if (!(theNode->StatusBits & STATUS_BIT_DETACHED))		// see if already attached
		return;

	slot = theNode->Slot;

	if (gFirstNodePtr == nil)						// special case only entry
	{
		gFirstNodePtr = theNode;
		theNode->PrevNode = nil;
		theNode->NextNode = nil;
	}

			/* INSERT AS FIRST NODE */
	else
	if (slot < gFirstNodePtr->Slot)
	{
		theNode->PrevNode = nil;					// no prev
		theNode->NextNode = gFirstNodePtr; 			// next pts to old 1st
		gFirstNodePtr->PrevNode = theNode; 			// old pts to new 1st
		gFirstNodePtr = theNode;
	}
		/* SCAN FOR INSERTION PLACE */
	else
	{
		ObjNode	 *reNodePtr, *scanNodePtr;

		reNodePtr = gFirstNodePtr;
		scanNodePtr = gFirstNodePtr->NextNode;		// start scanning for insertion slot on 2nd node

		while (scanNodePtr != nil)
		{
			if (slot < scanNodePtr->Slot)					// INSERT IN MIDDLE HERE
			{
				theNode->NextNode = scanNodePtr;
				theNode->PrevNode = reNodePtr;
				reNodePtr->NextNode = theNode;
				scanNodePtr->PrevNode = theNode;
				goto out;
			}
			reNodePtr = scanNodePtr;
			scanNodePtr = scanNodePtr->NextNode;			// try next node
		}

		theNode->NextNode = nil;							// TAG TO END
		theNode->PrevNode = reNodePtr;
		reNodePtr->NextNode = theNode;
	}
out:


	theNode->StatusBits &= ~STATUS_BIT_DETACHED;



			/* SUBRECURSE CHAINS & SHADOW */

	if (recurse)
	{
		if (theNode->ChainNode)
			AttachObject(theNode->ChainNode, recurse);

		if (theNode->ShadowNode)
			AttachObject(theNode->ShadowNode, recurse);
	}

}


/***************** FLUSH OBJECT DELETE QUEUE ****************/

static void FlushObjectDeleteQueue(void)
{
	for (int i = 0; i < gNumObjsInDeleteQueue; i++)
	{
		if (gObjectDeleteQueue[i]->pooledIndex < 0)			// allocated on heap
		{
			SafeDisposePtr((Ptr) gObjectDeleteQueue[i]);
		}
		else
		{
			Pool_ReleaseIndex(gObjectPool, gObjectDeleteQueue[i]->pooledIndex);
		}
	}

	gNumObjectNodes -= gNumObjsInDeleteQueue;
	gNumObjsInDeleteQueue = 0;
}


/****************** DISPOSE OBJECT BASE GROUP **********************/

void DisposeObjectBaseGroup(ObjNode *theNode)
{
	if (theNode->BaseGroup != nil)
	{
		MO_DisposeObjectReference(theNode->BaseGroup);

		theNode->BaseGroup = nil;
	}

	if (theNode->BaseTransformObject != nil)							// also nuke extra ref to transform object
	{
		MO_DisposeObjectReference(theNode->BaseTransformObject);
		theNode->BaseTransformObject = nil;
	}
}




//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT INFORMATION ------


/********************** GET OBJECT INFO *********************/

void GetObjectInfo(ObjNode *theNode)
{
	gCoord = theNode->Coord;
	gDelta = theNode->Delta;

}


/********************** UPDATE OBJECT *********************/

void UpdateObject(ObjNode *theNode)
{
	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
		return;

	theNode->Coord = gCoord;
	theNode->Delta = gDelta;
	UpdateObjectTransforms(theNode);

	CalcObjectBoxFromNode(theNode);


		/* UPDATE ANY SHADOWS */

	UpdateShadow(theNode);
}



/****************** UPDATE OBJECT TRANSFORMS *****************/
//
// This updates the skeleton object's base translate & rotate transforms
//

void UpdateObjectTransforms(ObjNode *theNode)
{
OGLMatrix4x4	m,m2;
OGLMatrix4x4	mx,my,mz,mxz;
uint32_t			bits;

	if (theNode->CType == INVALID_NODE_FLAG)		// see if already deleted
		return;

				/********************/
				/* SET SCALE MATRIX */
				/********************/

	OGLMatrix4x4_SetScale(&m, theNode->Scale.x,	theNode->Scale.y, theNode->Scale.z);


			/*****************************/
			/* NOW ROTATE & TRANSLATE IT */
			/*****************************/

	bits = theNode->StatusBits;

				/* USE ALIGNMENT MATRIX */

	if (bits & STATUS_BIT_USEALIGNMENTMATRIX)
	{
		m2 = theNode->AlignmentMatrix;
	}
	else
	{
					/* DO XZY ROTATION */

		if (bits & STATUS_BIT_ROTXZY)
		{

			OGLMatrix4x4_SetRotate_X(&mx, theNode->Rot.x);
			OGLMatrix4x4_SetRotate_Y(&my, theNode->Rot.y);
			OGLMatrix4x4_SetRotate_Z(&mz, theNode->Rot.z);

			OGLMatrix4x4_Multiply(&mx,&mz, &mxz);
			OGLMatrix4x4_Multiply(&mxz,&my, &m2);
		}
					/* DO YZX ROTATION */

		else
		if (bits & STATUS_BIT_ROTYZX)
		{
			OGLMatrix4x4_SetRotate_X(&mx, theNode->Rot.x);
			OGLMatrix4x4_SetRotate_Y(&my, theNode->Rot.y);
			OGLMatrix4x4_SetRotate_Z(&mz, theNode->Rot.z);

			OGLMatrix4x4_Multiply(&my,&mz, &mxz);
			OGLMatrix4x4_Multiply(&mxz,&mx, &m2);
		}

					/* DO ZXY ROTATION */

		else
		if (bits & STATUS_BIT_ROTZXY)
		{
			OGLMatrix4x4_SetRotate_X(&mx, theNode->Rot.x);
			OGLMatrix4x4_SetRotate_Y(&my, theNode->Rot.y);
			OGLMatrix4x4_SetRotate_Z(&mz, theNode->Rot.z);

			OGLMatrix4x4_Multiply(&mz,&mx, &mxz);
			OGLMatrix4x4_Multiply(&mxz,&my, &m2);
		}

					/* STANDARD XYZ ROTATION */
		else
		{
			OGLMatrix4x4_SetRotate_XYZ(&m2, theNode->Rot.x, theNode->Rot.y, theNode->Rot.z);
		}
	}


	m2.value[M03] = theNode->Coord.x;
	m2.value[M13] = theNode->Coord.y;
	m2.value[M23] = theNode->Coord.z;

	OGLMatrix4x4_Multiply(&m,&m2, &theNode->BaseTransformMatrix);


				/* UPDATE TRANSFORM OBJECT */

	SetObjectTransformMatrix(theNode);
}


/***************** SET OBJECT TRANSFORM MATRIX *******************/
//
// This call simply resets the base transform object so that it uses the latest
// base transform matrix
//

void SetObjectTransformMatrix(ObjNode *theNode)
{
MOMatrixObject	*mo = theNode->BaseTransformObject;

	if (theNode->CType == INVALID_NODE_FLAG)		// see if invalid
		return;

	if (mo)				// see if this has a trans obj
	{
		mo->matrix =  theNode->BaseTransformMatrix;
	}
}




/********************* FIND CLOSEST CTYPE *****************************/

ObjNode *FindClosestCType(OGLPoint3D *pt, uint32_t ctype, Boolean notThruSolid)
{
ObjNode		*thisNodePtr,*best = nil;
float	d,minDist = 10000000;
ObjNode	*heldObj = gPlayerInfo.heldObject;

	thisNodePtr = gFirstNodePtr;

	do
	{
		if (thisNodePtr->Slot >= SLOT_OF_DUMB)					// see if reach end of usable list
			break;

		if (thisNodePtr != heldObj)								// skip anything player is holding
		{
			if (thisNodePtr->CType & ctype)
			{
				d = CalcQuickDistance(pt->x,pt->z,thisNodePtr->Coord.x, thisNodePtr->Coord.z);
				if (d < minDist)
				{
					if (notThruSolid)							// see if skip anything that's not passable
					{
						if (SeeIfLineSegmentHitsAnything(pt, &thisNodePtr->Coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))
							goto next;
					}

					minDist = d;
					best = thisNodePtr;
				}
			}
		}
next:
		thisNodePtr = thisNodePtr->NextNode;		// next node
	}
	while (thisNodePtr != nil);

	return(best);
}


/********************* FIND CLOSEST CTYPE 3D *****************************/

ObjNode *FindClosestCType3D(OGLPoint3D *pt, uint32_t ctype)
{
ObjNode		*thisNodePtr,*best = nil;
float	d,minDist = 10000000;


	thisNodePtr = gFirstNodePtr;

	do
	{
		if (thisNodePtr->Slot >= SLOT_OF_DUMB)					// see if reach end of usable list
			break;

		if (thisNodePtr->CType & ctype)
		{
			d = OGLPoint3D_Distance(pt, &thisNodePtr->Coord);
			if (d < minDist)
			{
				minDist = d;
				best = thisNodePtr;
			}
		}
		thisNodePtr = thisNodePtr->NextNode;		// next node
	}
	while (thisNodePtr != nil);

	return(best);
}


/********************* MAKE NEW DRIVER OBJECT *****************************/

ObjNode* MakeNewDriverObject(int slot, void (*drawCall)(ObjNode *), void (*moveCall)(ObjNode *))
{
	NewObjectDefinitionType def =
	{
		.flags = STATUS_BIT_DONTCULL|STATUS_BIT_NOCOLLISION|STATUS_BIT_DONTPURGE,
		.slot = slot,
		.scale = 1,
		.genre = CUSTOM_GENRE,
		.drawCall = drawCall,
		.moveCall = moveCall,
	};
	return MakeNewObject(&def);
}
