/******************************/
/*	LEVEL INTRO: SEWER.C */
/* (c)2002 Pangea Software    */
/* By Brian Greenstone        */
/******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupLevelIntroScreen(void);
static void FreeLevelIntroScreen(void);
static void ProcessLevelIntro(void);
static void MoveIntroDrain(ObjNode *pipe);
static void MakeSewerGas(ObjNode *pipe);
static void MoveSewerText(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	PIPE_MODE_START,
	PIPE_MODE_OPEN,
	PIPE_MODE_STEAM,
	PIPE_MODE_TEXT
};


/*********************/
/*    VARIABLES      */
/*********************/



/********************** DO LEVELINTRO SCREEN **************************/

void DoLevelIntroScreen_Sewer(void)
{
			/* SETUP */

	SetupLevelIntroScreen();
	MakeFadeEvent(true, 1);

	ProcessLevelIntro();


			/* CLEANUP */

	OGL_FadeOutScene(DrawObjects, NULL);
	FreeLevelIntroScreen();
}



/********************* SETUP LEVELINTRO SCREEN **********************/

static void SetupLevelIntroScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
static const OGLVector3D	fillDirection1 = { -1.0, -.6, -.7 };
ObjNode	*newObj, *pipe, *grate;

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.1;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 3000;

	viewDef.styles.useFog			= true;
	viewDef.styles.fogStart			= viewDef.camera.yon * .5f;
	viewDef.styles.fogEnd			= viewDef.camera.yon * 1.1f;

	viewDef.view.clearBackBuffer	= true;
	viewDef.view.clearColor.r 		= 1;
	viewDef.view.clearColor.g 		= 1;
	viewDef.view.clearColor.b		= 1;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.y		= 600;
	viewDef.camera.from.z		= 500;

	viewDef.camera.to.y 		= 300.0f;

	viewDef.lights.ambientColor.r = .3;
	viewDef.lights.ambientColor.g = .3;
	viewDef.lights.ambientColor.b = .3;

	viewDef.lights.numFillLights 	= 1;

	viewDef.lights.fillDirection[0] = fillDirection1;
	viewDef.lights.fillColor[0].r 	= .9;
	viewDef.lights.fillColor[0].g 	= .9;
	viewDef.lights.fillColor[0].b 	= .8;

	OGL_SetupWindow(&viewDef, &gGameView);


				/************/
				/* LOAD ART */
				/************/

			/* LOAD SPRITES */

	InitParticleSystem();



			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:LevelIntro.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELINTRO);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:Level2_Sidewalk.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SIDEWALK_ObjType_Grate,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sheen);

	LoadFoliage();


				/* LOAD AUDIO */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Garden.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);



			/*******************/
			/* MAKE BACKGROUND */
			/*******************/

				/* GROUND */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= LEVELINTRO_ObjType_Level2Ground;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= -400;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 5;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 9.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* CYC */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= GARDEN_ObjType_Cyclorama;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot 		= TERRAIN_SLOT+1;					// draw after terrain for better performance since terrain blocks much of the pixels
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= gGameView->yon * .995f / 100.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawCyclorama;

	newObj->TargetOff.y = -300.0f;

			/******************/
			/* DRAIN ENTRANCE */
			/******************/

				/* PIPE */

	gNewObjectDefinition.group		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SIDEWALK_ObjType_DrainPipe;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveIntroDrain;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.0f;
	pipe = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	pipe->Mode = PIPE_MODE_START;
	pipe->Timer	= 1.0f;

				/* GRATE */

	gNewObjectDefinition.type 		= SIDEWALK_ObjType_Grate;
	gNewObjectDefinition.coord.x 	+= 79.0f * gNewObjectDefinition.scale;
	gNewObjectDefinition.coord.y 	+= 110.0f * gNewObjectDefinition.scale;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	grate = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	pipe->ChainNode = grate;

}


/********************** FREE LEVELINTRO SCREEN **********************/

static void FreeLevelIntroScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUND_BANK_LEVELSPECIFIC);
}



/**************** PROCESS LEVELINTRO ********************/

static void ProcessLevelIntro(void)
{
float	timer;

	CalcFramesPerSecond();
	UpdateInput();

	timer = 13.0f;

	while(!UserWantsOut())
	{
		const float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();

				/* MOVE CAMERA */

		gGameView->cameraPlacement.cameraLocation.z += 35.0f * fps;
		gGameView->cameraPlacement.cameraLocation.y -= 25.0f * fps;


				/* MOVE */

		MoveObjects();


				/* DRAW */

		OGL_DrawScene(DrawObjects);

		timer -= fps;
		if (timer < 0.0f)
			break;
	}
}



#pragma mark -

/************************* MOVE INTRO DRAIN ***********************/

static void MoveIntroDrain(ObjNode *pipe)
{
const float fps = gFramesPerSecondFrac;
ObjNode	*grate = pipe->ChainNode;

	switch(pipe->Mode)
	{
		case	PIPE_MODE_START:
				pipe->Timer -= fps;
				if (pipe->Timer <= 0.0f)
				{
					pipe->Mode = PIPE_MODE_OPEN;
				}
				break;


		case	PIPE_MODE_OPEN:
				grate->Rot.z -= fps * 4.0f;
				if (grate->Rot.z <= -2.0f)
				{
					grate->Rot.z = -2.0f;
					pipe->Mode = PIPE_MODE_STEAM;
					pipe->Timer = 4.0f;
				}
				UpdateObjectTransforms(grate);
				break;

		case	PIPE_MODE_STEAM:
				MakeSewerGas(pipe);

						/* LEVEL NAME TEXT */

				pipe->Timer -= fps;
				if (pipe->Timer <= 0.0f)
				{
					ObjNode	*newObj;
					int		i;

					for (i = 0; i < 6; i++)						// make n overlapping copies
					{
						gNewObjectDefinition.group		= MODEL_GROUP_LEVELINTRO;
						gNewObjectDefinition.type 		= LEVELINTRO_ObjType_SewerText;
						gNewObjectDefinition.coord.x 	= pipe->Coord.x;
						gNewObjectDefinition.coord.y 	= pipe->Coord.y + 650.0f;
						gNewObjectDefinition.coord.z 	= pipe->Coord.z;
						gNewObjectDefinition.flags 		= STATUS_BIT_DOUBLESIDED | STATUS_BIT_NOLIGHTING | STATUS_BIT_DONTCULL |
															STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_GLOW | STATUS_BIT_NOZBUFFER;
						gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
						gNewObjectDefinition.moveCall 	= MoveSewerText;
						gNewObjectDefinition.rot 		= 0;
						gNewObjectDefinition.scale 		= .5;
						newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

						newObj->DeltaRot.y = RandomFloat() * PI2;
						newObj->SpecialF[0] = RandomFloat() * PI2;

						newObj->ColorFilter.a = 0;
					}

					pipe->Mode = PIPE_MODE_TEXT;
				}
				break;


		case	PIPE_MODE_TEXT:
				MakeSewerGas(pipe);
				break;
	}

}


/********************* MOVE SEWER TEXT ***************************/

static void MoveSewerText(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	r,s;

	GetObjectInfo(theNode);

			/* FADE IN */

	theNode->ColorFilter.a += fps * .2f;
	if (theNode->ColorFilter.a > .11f)
		theNode->ColorFilter.a = .11f;

				/* SCALE UP */

	s = theNode->Scale.x += fps * .2f;
	if (s > 2.0f)
		s = 2.0f;
	theNode->Scale.x =
	theNode->Scale.y =
	theNode->Scale.z = s;


			/* ROTATE AROUND INIT COORD */

	r = (theNode->SpecialF[0] += theNode->DeltaRot.y * fps);

	gCoord.x = theNode->InitCoord.x + sin(r) * 15.0f;
	gCoord.y = theNode->InitCoord.y + cos(r) * 15.0f;

	UpdateObject(theNode);
}


/********************* MAKE SEWER GAS *****************************/

static void MakeSewerGas(ObjNode *pipe)
{
float				fps = gFramesPerSecondFrac;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;
float				x,y,z;

	x = pipe->Coord.x;
	y = pipe->Coord.y + 50.0f;
	z = pipe->Coord.z;

	pipe->ParticleTimer -= fps;
	if (pipe->ParticleTimer <= 0.0f)
	{
		pipe->ParticleTimer += 0.06f;

		long particleGroup 	= pipe->ParticleGroup;
		long magicNum 		= pipe->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			pipe->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_GRAVITOIDS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND | PARTICLE_FLAGS_ALLAIM;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 85000;
			groupDef.baseScale				= 20;
			groupDef.decayRate				= -2.4;
			groupDef.fadeRate				= .15;
			groupDef.particleTextureNum		= PARTICLE_SObjType_GreenFumes;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			pipe->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (int i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * 40.0f;
				p.y = y + RandomFloat() * 50.0f;
				p.z = z + RandomFloat2() * 40.0f;

				d.x = RandomFloat2() * 130.0f;
				d.y = 40.0f + RandomFloat() * 120.0f;
				d.z = RandomFloat2() * 130.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat()*PI2;
				newParticleDef.rotDZ		= 0;
				newParticleDef.alpha		= .4;
				if (AddParticleToGroup(&newParticleDef))
				{
					pipe->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}

