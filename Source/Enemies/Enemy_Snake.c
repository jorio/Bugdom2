/****************************/
/*   	ENEMY: SNAKE.C		*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveSnakeGenerator(ObjNode *theNode);
static Boolean StartSnake(ObjNode *generator);
static void UpdateSnakes(ObjNode *theNode);
static void DrawSnakes(ObjNode *theNode);
static void SnakeAtePlayer(int snakeNum, ObjNode *head, ObjNode *player);




/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	SNAKE_ANIM_SLITHER,
	SNAKE_ANIM_OPEN,
	SNAKE_ANIM_CARRY
};


#define	SNAKE_SPEED			800.0f

#define	SNAKE_ATTACK_DIST	5000.0f

#define	MAX_SNAKES			1

#define	MAX_SPLINE_POINTS	150

#define	SKIP_FACTOR			5

#define	MAX_SNAKE_RINGS	(MAX_SPLINE_POINTS/SKIP_FACTOR)
#define NUM_RING_POINTS			9

#define	SNAKE_RADIUS		40.0f

typedef struct
{
	Boolean		isUsed;

	OGLPoint3D	base,tipCoord;

	float		rot;
	float		growTimer;
	float		delayToGoAway;

	OGLPoint3D	splinePoints[MAX_SPLINE_POINTS];
	int			headPointIndex;

	ObjNode		*head;

	float		newBaseTimer;

}SnakeType;

#define	SNAKE_JOINT_HOLD	7

/*********************/
/*    VARIABLES      */
/*********************/

static	int	gNumSnakes;

static	SnakeType	gSnakes[MAX_SNAKES];

#define	HasSnake		Flag[0]
#define SnakeIndex	Special[0]

#define	Vulnerable	Flag[0]
#define	ShootNow	Flag[1]

static OGLPoint3D			gSnakePoints[MAX_SNAKE_RINGS * NUM_RING_POINTS];
static OGLVector3D			gSnakeNormals[MAX_SNAKE_RINGS * NUM_RING_POINTS];
static OGLTextureCoord		gSnakeUVs[MAX_SNAKE_RINGS * NUM_RING_POINTS];
static MOTriangleIndecies	gSnakeTriangles[MAX_SNAKE_RINGS * (NUM_RING_POINTS-1) * 2];

static MOVertexArrayData	gSnakeMesh;

static	OGLPoint3D			gRingPoints[NUM_RING_POINTS];
static	OGLVector3D			gRingNormals[NUM_RING_POINTS];

Boolean	gEnableSnakes;


/******************* INIT SNAKE STUFF **************************/

void InitSnakeStuff(void)
{
int	i;
float	r;
ObjNode	*newObj;

	gNumSnakes = 0;
	gEnableSnakes = false;

	for (i = 0; i < MAX_SNAKES; i++)
		gSnakes[i].isUsed = false;

			/* INIT MESH DATA */

	gSnakeMesh.numMaterials 	= 1;
	gSnakeMesh.materials[0] 	= gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][SIDEWALK_SObjType_SnakeSkin].materialObject;	// set illegal ref to material
	gSnakeMesh.points 			= gSnakePoints;
	gSnakeMesh.triangles 		= gSnakeTriangles;
	gSnakeMesh.normals			= gSnakeNormals;
	gSnakeMesh.uvs[0]			= gSnakeUVs;
	gSnakeMesh.colorsByte		= nil;
	gSnakeMesh.colorsFloat		= nil;

	SetSphereMapInfoOnVertexArrayData(&gSnakeMesh, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);


			/* INIT RING DATA */

	r = 0;
	for (i = 0; i < NUM_RING_POINTS; i++)
	{
		gRingNormals[i].x = sin(r);									// set vector
		gRingNormals[i].z = cos(r);
		gRingNormals[i].y = 0;

		gRingPoints[i].x = gRingNormals[i].x * SNAKE_RADIUS;		// set ring point
		gRingPoints[i].z = gRingNormals[i].z * SNAKE_RADIUS;
		gRingPoints[i].y = 0;

		r += PI2 / (NUM_RING_POINTS-1);
	}

			/* MAKE DRAW EVENT */

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SNAKE_SLOT;
	gNewObjectDefinition.moveCall 	= UpdateSnakes;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawSnakes;
}


/*********************** ADD SNAKE GENERATOR ***************************/

Boolean AddSnakeGenerator(TerrainItemEntryType *itemPtr, float  x, float z)
{
ObjNode	*newObj;

			/* MAKE EVENT OBJECT */

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x	= x;
	gNewObjectDefinition.coord.z	= z;
	gNewObjectDefinition.coord.y	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveSnakeGenerator;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->HasSnake 	= false;
	newObj->Timer 		= RandomFloat() * 3.0f;

	return(true);
}


/*********************** MOVE SNAKE GENERATOR *************************/

static void MoveSnakeGenerator(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	if (gPlayerInfo.ridingBall || (!gEnableSnakes) || (gPlayerInfo.snake != -1))	// snakes don't attack if player is riding the ball, not in snake zone, or already eaten
		return;

	if (!theNode->HasSnake)
	{
		theNode->Timer -= gFramesPerSecondFrac;				// check delay
		if (theNode->Timer < 0.0f)
		{

					/* IF PLAYER CLOSE ENOUGH THEN START NEW SNAKE */

			if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < SNAKE_ATTACK_DIST)
			{
				StartSnake(theNode);
				theNode->Timer = RandomFloat() * 4.0f;
			}
		}
	}
}



/*********************** START SNAKE ***************************/

static Boolean StartSnake(ObjNode *generator)
{
int		i,j;
ObjNode	*head;

			/************************/
			/* FIND FREE SNAKE SLOT */
			/************************/

	for (i = 0; i < MAX_SNAKES; i++)
	{
		if (!gSnakes[i].isUsed)
			goto got_it;

	}
	return(false);


			/**************************/
			/* INIT THIS SNAKE'S INFO */
			/**************************/
got_it:

	gSnakes[i].isUsed 		= true;								// mark this slot as used

	gSnakes[i].base 		= generator->Coord;					// set base coord
	gSnakes[i].tipCoord 	= generator->Coord;
	gSnakes[i].rot			= CalcYAngleFromPointToPoint(0, gSnakes[i].base.x, gSnakes[i].base.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);

	gSnakes[i].growTimer 	= 0;
	gSnakes[i].delayToGoAway = 1.0;							// # seconds to stay @ full size before going away

	gSnakes[i].newBaseTimer = 0;

			/* INIT ALL SPLINE POINTS TO INIT COORD */

	for (j = 0; j < MAX_SPLINE_POINTS; j++)
		gSnakes[i].splinePoints[j] = generator->Coord;

	gSnakes[i].headPointIndex = 0;							// head starts @ index 0

	generator->HasSnake 	= true;
	generator->SnakeIndex 	= i;


			/***********************/
			/* CREATE THE HEAD OBJ */
			/***********************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_SNAKEHEAD;
	gNewObjectDefinition.animNum 	= SNAKE_ANIM_SLITHER;
	gNewObjectDefinition.coord		= gSnakes[i].base;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SNAKE_SLOT+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.scale 		= 1.5f;
	gNewObjectDefinition.rot 		= 0;
	head = MakeNewSkeletonObject(&gNewObjectDefinition);


	gSnakes[i].head	= head;

	return(true);
}


/********************** UPDATE SNAKES ***************************/

static void UpdateSnakes(ObjNode *theNode)
{
int		i,q,n;
float	fps = gFramesPerSecondFrac;
float	fps2,dist,turnSpeed;
float	r,px,pz;
ObjNode	*player = gPlayerInfo.objNode;
ObjNode	*head;
Boolean	runAway;

	(void) theNode;


	runAway = gPlayerInfo.ridingBall || (!gEnableSnakes) || (gPlayerInfo.snake != -1);						// see if snakes should run away now
	if (!runAway)																// if not running away then get the player's coord as the target
	{
		px = player->Coord.x;
		pz = player->Coord.z;
	}


	for (i = 0; i < MAX_SNAKES; i++)
	{
		if (!gSnakes[i].isUsed)													// see if skip this slot
			continue;

		head = gSnakes[i].head;													// get head object

		if (runAway)															// if running away then get the run away coord
		{
			if (gPlayerInfo.snake != -1)										// snake has player then do random base changes
			{
				gSnakes[i].newBaseTimer -= fps;
				if (gSnakes[i].newBaseTimer <= 0.0f)
				{
					gSnakes[i].newBaseTimer = .3f + RandomFloat() * .5f;

					gSnakes[i].base.x = head->Coord.x + RandomFloat2() * 4000.0f;	// calc new base coord
					gSnakes[i].base.z = head->Coord.z + RandomFloat2() * 4000.0f;
				}
			}

			px = gSnakes[i].base.x;												// target is the base coord
			pz = gSnakes[i].base.z;
		}


		fps2 = fps * .1f;
		for (q = 0; q < 10; q++)												// do this several times to increase the resolution nicely
		{
				/*****************/
				/* UPDATE GROWTH */
				/*****************/

			dist = CalcQuickDistance(gSnakes[i].tipCoord.x, gSnakes[i].tipCoord.z, px, pz);	// turn snake faster when closer to player
			if (dist == 0.0f)
				dist = .01f;
			turnSpeed = 400.0f / dist;

			if (turnSpeed < 4.0f)
				turnSpeed = 4.0f;
			else
			if (turnSpeed > 12.0f)
				turnSpeed = 12.0f;

			n = gSnakes[i].headPointIndex;										// get head's index into spline point list
			TurnPointTowardPoint(&gSnakes[i].rot, &gSnakes[i].splinePoints[n-1], px, pz, turnSpeed * fps2);	// rotate toward player


				/* CALC A NEW POINT TOWARD ME */

			r = gSnakes[i].rot;
			gSnakes[i].tipCoord.x -= sin(r) * (SNAKE_SPEED * fps2);
			gSnakes[i].tipCoord.z -= cos(r) * (SNAKE_SPEED * fps2);
			gSnakes[i].tipCoord.y = SNAKE_RADIUS + GetTerrainY(gSnakes[i].tipCoord.x,gSnakes[i].tipCoord.z);


				/* CHECK GROW TIMER TO SEE IF SHOULD ADD THE NEW POINT */

			gSnakes[i].growTimer -= fps2;
			if (gSnakes[i].growTimer <= 0.0f)
			{
				n++;																// move head to next index in point list
				if (n >= MAX_SPLINE_POINTS)											// see if wrap back around
					n = 0;

				gSnakes[i].growTimer += .02f;

				gSnakes[i].splinePoints[n] = gSnakes[i].tipCoord;					// add the point to the list
				gSnakes[i].headPointIndex = n;										// update snake's head index
			}
		}

				/***************/
				/* UPDATE HEAD */
				/***************/

		head->Rot.y = gSnakes[i].rot;
		head->Coord = gSnakes[i].tipCoord;
		UpdateObjectTransforms(head);



				/* SEE IF EATEN BY HEAD */

		dist = OGLPoint3D_Distance(&head->Coord, &player->Coord);
		if (dist < 60.0f)
		{
			SnakeAtePlayer(i, head, player);
		}

				/* SEE IF CLOSE ENOUGH TO OPEN MOUTH */

		else
		if (gPlayerInfo.snake != i)												// only do this if player not being eaten by this snake
		{
			if (dist < 600.0f)
			{
				if (head->Skeleton->AnimNum != SNAKE_ANIM_OPEN)
					MorphToSkeletonAnim(head->Skeleton, SNAKE_ANIM_OPEN, 4.0f);
			}

					/* MAKE SURE MOUTH CLOSED */
			else
			if (head->Skeleton->AnimNum != SNAKE_ANIM_SLITHER)
				MorphToSkeletonAnim(head->Skeleton, SNAKE_ANIM_SLITHER, 4.0f);
		}


	}	// for snakes
}


/******************** DRAW SNAKES ****************************/

static void DrawSnakes(ObjNode *theNode)
{
int				i,p,j,pointNum,numRingsCreated;
float			u,v,s;
OGLVector3D		dir;
OGLMatrix4x4	m,m2;
const OGLVector3D from = {0,1,0};
MOTriangleIndecies	*triPtr;
Boolean			runAway = gPlayerInfo.ridingBall || (!gEnableSnakes);						// see if snakes are running away

	(void) theNode;

	gGlobalTransparency = 1.0f;

	OGL_PushState();

	OGL_EnableLighting();


	for (i = 0; i < MAX_SNAKES; i++)
	{
		if (!gSnakes[i].isUsed)										// see if skip this slot
			continue;

		gSnakeMesh.numPoints 	= 0;								// clear # points in mesh
		gSnakeMesh.numTriangles = -(NUM_RING_POINTS-1)*2;			// clear # triangles in mesh (note: start -1 since 1st ring doesn't add triangles)


					/**************************************************/
					/* FIRST BUILD ALL OF THE POINTS FOR THE GEOMETRY */
					/**************************************************/
					//
					// Remember that the last and 1st ring points overlap so that we can
					// do proper uv wrapping all the way around.
					//

		numRingsCreated = 0;															// no rings calculated yet
		s = .3f;																		// start scale narrow @ head

		p = gSnakes[i].headPointIndex;													// start @ head	index
		for (j = 0; j < MAX_SNAKE_RINGS; j++)
		{
			int	prevIndex = p-SKIP_FACTOR;													// get index of next seg toward tail
			if (prevIndex < 0)															// see if wrap back around
				prevIndex = MAX_SPLINE_POINTS + prevIndex;

					/* CALC VECTOR TO NEXT RING */

			dir.x = gSnakes[i].splinePoints[prevIndex].x - gSnakes[i].splinePoints[p].x;
			dir.y = gSnakes[i].splinePoints[prevIndex].y - gSnakes[i].splinePoints[p].y;
			dir.z = gSnakes[i].splinePoints[prevIndex].z - gSnakes[i].splinePoints[p].z;
			OGLVector3D_Normalize(&dir, &dir);


					/* ROTATE RING INTO POSITION */

			OGLMatrix4x4_SetScale(&m2, s, s, s);
			OGLCreateFromToRotationMatrix(&m, &from, &dir);										// generate a rotation matrix
			OGLMatrix4x4_Multiply(&m2, &m, &m);

			m.value[M03] = gSnakes[i].splinePoints[p].x;										// insert translate
			m.value[M13] = gSnakes[i].splinePoints[p].y;
			m.value[M23] = gSnakes[i].splinePoints[p].z;

			OGLPoint3D_TransformArray(&gRingPoints[0], &m, &gSnakePoints[j * NUM_RING_POINTS],  NUM_RING_POINTS);		// transform the points into master array
			OGLVector3D_TransformArray(&gRingNormals[0], &m, &gSnakeNormals[j * NUM_RING_POINTS],  NUM_RING_POINTS);	// transform the normals into master array

			p -= SKIP_FACTOR;													// next ring
			if (p < 0)															// see if wrap back around
				p = MAX_SPLINE_POINTS + p;

			gSnakeMesh.numPoints	+= NUM_RING_POINTS;							// inc # points in mesh
			gSnakeMesh.numTriangles += (NUM_RING_POINTS-1) * 2;					// inc # triangles in mesh

			numRingsCreated++;													// inc ring counter

						/* UPDATE TIP SCALE */

			if (j < 4)															// scale UP head/front tip
			{
				s += .8f;
				if (s > 1.0f)
					s = 1.0f;
			}
			else																// scale DOWN the butt tip
			if (j > (MAX_SNAKE_RINGS-11))
			{
				if (s <= 0.001f)													// when done, make this the end
					break;
				else
				{
					s -= .12f;
					if (s <= .001f)												// keep small for next pass for final end tip
						s = .001f;
				}
			}
		}


							/* BUILD BBOX & CULL */

		OGLPoint3D_CalcBoundingBox(gSnakeMesh.points, gSnakeMesh.numPoints, &gSnakeMesh.bBox);
		if (!OGL_IsBBoxVisible(&gSnakeMesh.bBox, nil))
		{
			if (runAway)															// delete the culled snake if running away
			{
				DeleteObject(gSnakes[i].head);										// delete the head obj
				gSnakes[i].head = nil;
				gSnakes[i].isUsed = false;											// delete this snake
			}
			continue;
		}

				/*******************************************/
				/* NOW BUILD THE TRIANGLES FROM THE POINTS */
				/*******************************************/

		triPtr = &gSnakeTriangles[0];											// point to triangle list

		for (p = 0; p < numRingsCreated; p++)									// for each segment
		{
			pointNum = p * NUM_RING_POINTS;										// calc index into point list to start this ring

			for (j = 0; j < (NUM_RING_POINTS-1); j++)							// build 2 triangles for each point in the ring
			{
						/* TRIANGLE A */

				triPtr->vertexIndices[0] = pointNum + j;
				triPtr->vertexIndices[1] = pointNum + j + 1;				// next pt over
				triPtr->vertexIndices[2] = pointNum + j + NUM_RING_POINTS;		// pt in next segment
				triPtr++;														// point to next triangle

						/* TRIANGLE B */

				triPtr->vertexIndices[0] = pointNum + j + 1;					// next pt over
				triPtr->vertexIndices[1] = pointNum + j + 1 + NUM_RING_POINTS;		// pt in next segment

				triPtr->vertexIndices[2] = pointNum + j + NUM_RING_POINTS;
				triPtr++;														// point to next triangle
			}
		}

				/*************************/
				/* SET RING COLORS & UVS */
				/*************************/

		v = 0;
		for (p = 0; p < numRingsCreated; p++)
		{
			OGLTextureCoord	*uvs 	= &gSnakeUVs[p * NUM_RING_POINTS];


			u = 0;
			for (j = 0; j < NUM_RING_POINTS; j++)
			{
				uvs[j].u = u;
				uvs[j].v = v;

				u += 3.0f / (float)(NUM_RING_POINTS-1);
			}
			v += .5f;
		}

				/***********************/
				/* SUBMIT VERTEX ARRAY */
				/***********************/

		MO_DrawGeometry_VertexArray(&gSnakeMesh);


	}

	OGL_PopState();
}



/***************** SNAKE ATE PLAYER **************************/

static void SnakeAtePlayer(int snakeNum, ObjNode *head, ObjNode *player)
{
	if (player->Skeleton->AnimNum == PLAYER_ANIM_EATENBYSNAKE)					// see if already being eaten
		return;

	KillPlayer(PLAYER_DEATH_TYPE_EATENBYSNAKE);

	player->Coord = head->Coord;
	player->Rot.y = head->Rot.y;
	UpdateObjectTransforms(player);

	gPlayerInfo.snake = snakeNum;

	gFreezeCameraFromXZ = true;
	gFreezeCameraFromY = true;

	MorphToSkeletonAnim(head->Skeleton, SNAKE_ANIM_CARRY, 10);
}


/******************** MOVE PLAYER: EATEN BY SNAKE ***********************/

void MovePlayer_EatenBySnake(ObjNode *player)
{
	if (gLevelNum == LEVEL_NUM_PARK)						// see if actually eaten by fish
	{
		MovePlayer_EatenByFish(player);

	}
	else
	{
		int	snakeNum = gPlayerInfo.snake;
		ObjNode	*head = gSnakes[snakeNum].head;
		float	oldTimer;

		FindCoordOfJoint(head, SNAKE_JOINT_HOLD, &gCoord);			// calc coord to put player
		gPlayerInfo.coord = gCoord;
		player->Rot.y = head->Rot.y;

			/* UPDATE */

		UpdateObject(player);

		oldTimer = gDeathTimer;
		gDeathTimer -= gFramesPerSecondFrac;
		if (gDeathTimer <= 0.0f)
		{
			if (oldTimer > 0.0f)						// if just now crossed zero then start fade
				MakeFadeEvent(false, 1);
			else
			if (gGammaFadeFrac <= 0.0f)				// once fully faded out reset player @ checkpoint
				ResetPlayerAtBestCheckpoint();
		}
	}
}













