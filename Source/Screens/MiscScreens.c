/****************************/
/*   	MISCSCREENS.C	    */
/* By Brian Greenstone      */
/* (c)2002 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/


#pragma mark -

/************** DO LEGAL SCREEN *********************/

void DoLegalScreen(void)
{
	OGLSetupInputType	viewDef;
	float	timeout = 8.0f;


			/* SETUP VIEW */

	OGL_NewViewDef(&viewDef);

	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;
	viewDef.styles.useFog			= false;

	OGL_SetupWindow(&viewDef, &gGameView);


			/* CREATE BACKGROUND OBJECT */

	LoadSpriteGroupFromSeries(SPRITE_GROUP_LEVELSPECIFIC, 1, "Pangea");

	NewObjectDefinitionType def =
	{
		.coord = {320, 240, 0},
		.group = SPRITE_GROUP_LEVELSPECIFIC,
		.type = 0,
		.scale = 640,
		.slot = SPRITE_SLOT,
	};
	
	ObjNode* theNode = MakeSpriteObject(&def);
	GAME_ASSERT(theNode);


		/***********/
		/* SHOW IT */
		/***********/


	MakeFadeEvent(true, 1);
	UpdateInput();
	CalcFramesPerSecond();

		/* MAIN LOOP */

	while(!UserWantsOut())
	{
		CalcFramesPerSecond();
		MoveObjects();
		OGL_DrawScene(DrawObjects);

		UpdateInput();
		if (UserWantsOut())
			break;

		timeout -= gFramesPerSecondFrac;
		if (timeout < 0.0f)
			break;
	}

		/* FADE OUT */

	OGL_FadeOutScene(DrawObjects, NULL);

		/* CLEANUP */

	DeleteAllObjects();
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);
}


#pragma mark -

/********************** DO GAME SETTINGS DIALOG ************************/

void DoGameOptionsDialog(void)
{
	IMPLEMENT_ME_SOFT();

#if 0
OSErr			err;
EventTypeSpec	list[] = { { kEventClassCommand,  kEventProcessCommand } };
WindowRef 		dialogWindow = nil;
EventHandlerUPP winEvtHandler;
ControlID 		idControl;
ControlRef 		control;
EventHandlerRef	ref;

const char		*rezNames[MAX_LANGUAGES] =
{
	"Settings_English",
	"Settings_French",
	"Settings_German",
	"Settings_Spanish",
	"Settings_Italian",
	"Settings_Swedish",
	"Settings_Dutch",
};


	if (gGamePrefs.language >= MAX_LANGUAGES)		// verify prefs for the hell of it.
		InitDefaultPrefs();

	Enter2D();
	ShowRealCursor();
//	MyFlushEvents();


    		/***************/
    		/* INIT DIALOG */
    		/***************/

do_again:

	gLanguageChanged  = false;

			/* CREATE WINDOW FROM THE NIB */

    err = CreateWindowFromNib(gNibs,CFStringCreateWithCString(nil, rezNames[gGamePrefs.language],
    						kCFStringEncodingMacRoman), &dialogWindow);
	if (err)
		DoFatalAlert("DoGameOptionsDialog: CreateWindowFromNib failed!");

			/* CREATE NEW WINDOW EVENT HANDLER */

    winEvtHandler = NewEventHandlerUPP(DoGameSettingsDialog_EventHandler);
    InstallWindowEventHandler(dialogWindow, winEvtHandler, GetEventTypeCount(list), list, dialogWindow, &ref);


			/* SET "SHOW VIDEO" CHECKBOX */

    idControl.signature = 'vido';
    idControl.id 		= 0;
    GetControlByID(dialogWindow, &idControl, &control);
	SetControlValue(control, gGamePrefs.showScreenModeDialog);


			/* SET "KIDDIE MODE" CHECKBOX */

    idControl.signature = 'kidm';
    idControl.id 		= 0;
    GetControlByID(dialogWindow, &idControl, &control);
	SetControlValue(control, gGamePrefs.kiddieMode);


			/* SET LANGUAGE  */

    idControl.signature = 'lang';
    idControl.id 		= 0;
    GetControlByID(dialogWindow, &idControl, &control);
	SetControlValue(control, gGamePrefs.language + 1);



			/**********************/
			/* PROCESS THE DIALOG */
			/**********************/

    ShowWindow(dialogWindow);
	RunAppModalLoopForWindow(dialogWindow);


			/*********************/
			/* GET RESULT VALUES */
			/*********************/

			/* GET "SHOW VIDEO" CHECKBOX */

    idControl.signature = 'vido';
    idControl.id 		= 0;
    GetControlByID(dialogWindow, &idControl, &control);
	gGamePrefs.showScreenModeDialog = GetControlValue(control);


			/* GET "KIDDIE MODE" CHECKBOX */

    idControl.signature = 'kidm';
    idControl.id 		= 0;
    GetControlByID(dialogWindow, &idControl, &control);
	gGamePrefs.kiddieMode = GetControlValue(control);



			/* GET LANGUAGE */

    idControl.signature = 'lang';
    idControl.id 		= 0;
    GetControlByID(dialogWindow, &idControl, &control);
	gGamePrefs.language = GetControlValue(control) - 1;


				/***********/
				/* CLEANUP */
				/***********/

	DisposeEventHandlerUPP (winEvtHandler);
	DisposeWindow (dialogWindow);

			/* IF IT WAS JUST A LANGUAGE CHANGE THEN GO BACK TO THE DIALOG */

	if (gLanguageChanged)
		goto do_again;

	HideRealCursor();
	Exit2D();
	SavePrefs();

	CalcFramesPerSecond();				// reset this so things dont go crazy when we return
	CalcFramesPerSecond();
#endif
}


#if 0
/****************** DO GAME SETTINGS DIALOG EVENT HANDLER *************************/

static pascal OSStatus DoGameSettingsDialog_EventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData)
{
#pragma unused (myHandler, userData)
OSStatus			result = eventNotHandledErr;
HICommand 			command;

	switch(GetEventKind(event))
	{

				/*******************/
				/* PROCESS COMMAND */
				/*******************/

		case	kEventProcessCommand:
				GetEventParameter (event, kEventParamDirectObject, kEventParamHICommand, NULL, sizeof(command), NULL, &command);
				switch(command.commandID)
				{
							/* OK BUTTON */

					case	kHICommandOK:
		                    QuitAppModalLoopForWindow((WindowRef) userData);
		                    break;


							/* LANGUAGE */

					case	'lang':
							gLanguageChanged = true;
		                    QuitAppModalLoopForWindow((WindowRef) userData);
							break;


							/* CLEAR HIGH SCORES */

					case	'clhs':
							ClearHighScores();
							break;


							/* CONFIGURE INPUT */

					case	'cinp':
							DoInputConfigDialog();
							break;

				}
				break;
    }

    return (result);
}
#endif



#pragma mark -


/********************* LEVEL INTRO SUBTITLE ***********************/

static void DrawLevelIntroSubtitle(ObjNode* objNode)
{
	objNode->Timer += gFramesPerSecondFrac;

	if (objNode->Timer < 0)
		return;

	OGL_PushState();
	SetInfobarSpriteState();
	gGlobalTransparency = SDL_min(1, 2 * objNode->Timer);
	GameFont_DrawString(Localize(STR_LEVEL1 + gLevelNum), 320, 400, 0.6f, kTextMeshAlignCenter);
	OGL_PopState();

	gGlobalTransparency = 1;		// restore this
}

/*********************** DO LEVEL INTRO ***************************/

void DoLevelIntro(void)
{
			/* SET ANAGLYPH INFO */

	if (gGamePrefs.anaglyph)
	{
		gAnaglyphScaleFactor 	= 1.0f;
		gAnaglyphFocallength	= 200.0f * gAnaglyphScaleFactor;	// set camera info
		gAnaglyphEyeSeparation 	= 25.0f * gAnaglyphScaleFactor;
	}

			/* SHOW NON-ENGLISH SUBTITLE */

	if (gGamePrefs.language != LANGUAGE_ENGLISH
		&& Localize(STR_LEVEL1 + gLevelNum)[0] != '-')		// don't show untranslated subtitles
	{
		static const float timers[] =
		{
			[LEVEL_NUM_GNOMEGARDEN]	= 5,
			[LEVEL_NUM_SIDEWALK]	= 12,
			[LEVEL_NUM_FIDO]		= 3,
			[LEVEL_NUM_PLUMBING]	= 7,
			[LEVEL_NUM_PLAYROOM]	= 5,
			[LEVEL_NUM_CLOSET]		= 6.5f,
			[LEVEL_NUM_GUTTER]		= 8,
			[LEVEL_NUM_GARBAGE]		= 2,
			[LEVEL_NUM_BALSA]		= 5,
			[LEVEL_NUM_PARK]		= 5.5f,
		};
		ObjNode* subtitle = MakeNewDriverObject(INFOBAR_SLOT, DrawLevelIntroSubtitle, 0);
		subtitle->Timer = -timers[gLevelNum];
	}

			/* DO INTRO SCENE */

	switch(gLevelNum)
	{
		case	LEVEL_NUM_GNOMEGARDEN:
				DoLevelIntroScreen_FrontYard();
				break;

		case	LEVEL_NUM_SIDEWALK:
				DoLevelIntroScreen_BackYard();
				break;

		case	LEVEL_NUM_PLUMBING:
				DoLevelIntroScreen_Sewer();
				break;

		case	LEVEL_NUM_PLAYROOM:
				DoLevelIntroScreen_Playroom();
				break;

		case	LEVEL_NUM_CLOSET:
				DoLevelIntroScreen_Closet();
				break;

		case	LEVEL_NUM_BALSA:
				DoLevelIntroScreen_Balsa();
				break;

		case	LEVEL_NUM_FIDO:
				DoLevelIntroScreen_Fido();
				break;

		case	LEVEL_NUM_GUTTER:
				DoLevelIntroScreen_Gutter();
				break;

		case	LEVEL_NUM_GARBAGE:
				DoLevelIntroScreen_Garbage();
				break;

		case	LEVEL_NUM_PARK:
				DoLevelIntroScreen_Park();
				break;
	}

}






