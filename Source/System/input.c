/****************************/
/*   	  INPUT.C	   	    */
/* (c)2003 Pangea Software  */
/* By Brian Greenstone      */
/****************************/

/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

extern	AGLContext		gAGLContext;
extern	float			gFramesPerSecondFrac,gFramesPerSecond,gScratchF;
extern	PrefsType			gGamePrefs;


/**********************/
/*     PROTOTYPES     */
/**********************/

static void Install_MouseEventHandler(void);
static void UpdateMouseDeltas(void);
static void MyInitHID(void);
static void PollAllHIDInputNeeds(void);
static void UpdateKeyMap(void);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	AXIS_RANGE	128.0f				// calibrate all axis values to be in +/- AXIS_RANGE values (-128 to +128)


/**********************/
/*     VARIABLES      */
/**********************/

Boolean	gHIDInitialized = false;

static	long					gMouseDeltaX = 0;
static	long					gMouseDeltaY = 0;

static	float					gReadMouseDeltasTimer = 0;

Boolean		gMouseButtonState = false, gMouseNewButtonState = false;

KeyMapByteArray gKeyMap,gNewKeys,gOldKeys;



			/**************/
			/* NEEDS LIST */
			/**************/

InputNeedType	gControlNeeds[NUM_CONTROL_NEEDS] =
{
#if 0
			/* PLAYER 1 CONTROLS */
	{										// kNeed_TurnLeft_Key
		"Turn Left (Key)",
		kHIDUsage_KeyboardLeftArrow,
	},

	{										// kNeed_TurnRight_Key
		"Turn Right (Key)",
		kHIDUsage_KeyboardRightArrow,
	},

	{										// kNeed_Forward
		"Move Forward (Key)",
		kHIDUsage_KeyboardUpArrow,
	},

	{										// kNeed_Backward
		"Move Backwards (Key)",
		kHIDUsage_KeyboardDownArrow,
	},

	{										// kNeed_XAxis
		"X-Axis",
		0,
	},

	{										// kNeed_YAxis
		"Y-Axis",
		0,
	},

	{										// kNeed_Kick
		"Kick",
		kHIDUsage_KeyboardLeftGUI,
	},

	{										// kNeed_PickupDrop
		"Pickup/Drop",
		kHIDUsage_KeyboardLeftAlt,
	},

	{										// kNeed_AutoWalk
		"Auto Walk",
		kHIDUsage_KeyboardLeftShift,
	},

	{										// kNeed_Jump
		"Jump",
		kHIDUsage_KeyboardSpacebar,
	},

	{										// kNeed_LaunchBuddy
		"Launch Buddy Bug",
		kHIDUsage_KeyboardTab,
	},

	{										// kNeed_CameraMode
		"Camera Mode",
		kHIDUsage_KeyboardC,
	},

	{										// kNeed_CameraLeft
		"Camera Swing Left",
		kHIDUsage_KeyboardComma,
	},

	{										// kNeed_CameraRight
		"Camera Swing Right",
		kHIDUsage_KeyboardPeriod,
	},
#endif
};





/************************* INIT INPUT *********************************/

void InitInput(void)
{
short	i;

	for (i = 0; i < NUM_CONTROL_NEEDS; i++)							// init current control values
		gControlNeeds[i].oldValue = gControlNeeds[i].value = 0;

	// Install_MouseEventHandler();									// install our carbon even mouse handler
	MyInitHID();


}


/********************** UPDATE INPUT ******************************/
//
// This is the master input update function which reads all input values including those from HID devices, GetKeys(), and the Mouse.
//

void UpdateInput(void)
{
float	analog, mouseDX, mouseDY;

	UpdateKeyMap();												// always read real keyboard anyway via GetKeys()
	UpdateMouseDeltas();							// get mouse deltas


			/****************************************/
			/* UPDATE ALL THE NEEDS CONTROLS VALUES */
			/****************************************/

	PollAllHIDInputNeeds();



			/****************************/
			/* SET PLAYER AXIS CONTROLS */
			/****************************/

	gPlayerInfo.analogControlX = 											// assume no control input
	gPlayerInfo.analogControlZ = 0.0f;

			/* FIRST CHECK ANALOG AXES */

					/* PLAYER 1 */

	analog = gControlNeeds[kNeed_XAxis].value;
	if (analog != 0.0f)														// is X-Axis being used?
	{
		analog *= 1.0f / AXIS_RANGE;										// convert to  -1.0 to 1.0 range
		gPlayerInfo.analogControlX = analog;
	}

	analog = gControlNeeds[kNeed_YAxis].value;
	if (analog != 0.0f)														// is Y-Axis being used?
	{
		analog *= 1.0f / AXIS_RANGE;										// convert to  -1.0 to 1.0 range
		gPlayerInfo.analogControlZ = analog;
	}

			/* NEXT CHECK THE DIGITAL KEYS */


	if (gControlNeeds[kNeed_TurnLeft_Key].value)							// is Left Key pressed?
		gPlayerInfo.analogControlX = -1.0f;
	else
	if (gControlNeeds[kNeed_TurnRight_Key].value)						// is Right Key pressed?
		gPlayerInfo.analogControlX = 1.0f;


	if (gControlNeeds[kNeed_Forward].value)							// is Up Key pressed?
		gPlayerInfo.analogControlZ = -1.0f;
	else
	if (gControlNeeds[kNeed_Backward].value)						// is Down Key pressed?
		gPlayerInfo.analogControlZ = 1.0f;



		/* AND FINALLY SEE IF MOUSE DELTAS ARE BEST */
		//
		// The mouse is only used for Player 1
		//

	mouseDX = gMouseDeltaX * 0.015f;											// scale down deltas for our use
	mouseDY = gMouseDeltaY * 0.015f;

	if (mouseDX > 1.0f)											// keep x values pinned
		mouseDX = 1.0f;
	else
	if (mouseDX < -1.0f)
		mouseDX = -1.0f;

	if (fabs(mouseDX) > fabs(gPlayerInfo.analogControlX))		// is the mouse delta better than what we've got from the other devices?
		gPlayerInfo.analogControlX = mouseDX;


	if (mouseDY > 1.0f)											// keep y values pinned
		mouseDY = 1.0f;
	else
	if (mouseDY < -1.0f)
		mouseDY = -1.0f;

	if (fabs(mouseDY) > fabs(gPlayerInfo.analogControlZ))		// is the mouse delta better than what we've got from the other devices?
		gPlayerInfo.analogControlZ = mouseDY;

}




//=================================================================================================================
//=================================================================================================================
//=================================================================================================================
//=================================================================================================================
//=================================================================================================================


#pragma mark ======= HID INITIALIZATION ============



/************************ MY INIT HID *************************/

static void MyInitHID(void)
{
	if (gHIDInitialized)
		DoFatalAlert("MyInitHID: HID already initialized!");


	gHIDInitialized = true;
}


/********************* SHUTDOWN HID ****************************/
//
// Called when exiting the app.  It should dispose of all the HID stuff
// that's allocated.
//

void ShutdownHID(void)
{
	if (!gHIDInitialized)
		return;

	gHIDInitialized = false;

}

#pragma mark -
#pragma mark ========= HID DEVICE POLLING ===========


/******************** POLL ALL INPUT NEEDS ***********************/
//
// This is the main input update function which should be called once per loop.
// It scans the Needs list and updates the status of all of the Elements assigned to each Need.
//

static void PollAllHIDInputNeeds(void)
{
short					n, d, e;
long					bestValue, value, elementType;

	if (!gHIDInitialized)
		return;


			/* SEE IF USER HAS HID DISABLED FOR MANUAL GETKEYS() KEYBOARD READS */
			//
			// This is a hack I had to put in so that people who couldn't get the game to work with HID could still use GetKeys() to do it
			//

	// if (gGamePrefs.dontUseHID)
	{
		for (n = 0; n < NUM_CONTROL_NEEDS; n++)
		{
			uint32_t value;

			gControlNeeds[n].oldValue = gControlNeeds[n].value;				// remember what the old value was before we update it

			switch(n)
			{
				case	0:								// kNeed_TurnLeft_Key
						gControlNeeds[n].value = value= GetKeyState(KEY_LEFT);
						break;


				case	1:								// kNeed_TurnRight_Key
						gControlNeeds[n].value = value = GetKeyState(KEY_RIGHT);
						break;

				case	2:								// kNeed_Forward
						gControlNeeds[n].value = value = GetKeyState(KEY_UP);
						break;

				case	3:								// kNeed_Backward
						gControlNeeds[n].value = value = GetKeyState(KEY_DOWN);
						break;

				case	6:								// kNeed_Kick
						gControlNeeds[n].value = value = GetKeyState(KEY_APPLE);
						break;

				case	7:								// kNeed_PickupDrop
						gControlNeeds[n].value = value = GetKeyState(KEY_OPTION);
						break;

				case	8:								// kNeed_AutoWalk
						gControlNeeds[n].value = value = GetKeyState(KEY_SHIFT);
						break;

				case	9:								// kNeed_Jump
						gControlNeeds[n].value = value = GetKeyState(KEY_SPACE);
						break;

				case	10:								// kNeed_LaunchBuddy
						gControlNeeds[n].value = value = GetKeyState(KEY_TAB);
						break;

				case	11:								// kNeed_CameraMode
						gControlNeeds[n].value = value = GetKeyState(KEY_C);
						break;

				case	12:								// kNeed_CameraLeft
						gControlNeeds[n].value = value = GetKeyState(KEY_COMMA);
						break;

				case	13:								// kNeed_CameraRight
						gControlNeeds[n].value = value = GetKeyState(KEY_PERIOD);
						break;


				default:
						value = 0;


			}

			if (value && (gControlNeeds[n].oldValue == 0))			// is this a new button press?
				gControlNeeds[n].newButtonPress	= true;
			else
				gControlNeeds[n].newButtonPress	= false;

		}
		return;
	}



#if 0
			/******************************************/
			/* SCAN THRU ALL OF THE NEEDS IN OUR LIST */
			/******************************************/

	for (n = 0; n < NUM_CONTROL_NEEDS; n++)
	{
		gControlNeeds[n].oldValue = gControlNeeds[n].value;				// remember what the old value was before we update it

		bestValue = 0;													// init the best value


				/* SCAN THRU ALL OF THE DEVICES */

		for (d = 0; d < gNumHIDDevices; d++)
		{
			if (gHIDDeviceList[d].isActive == false)					// skip any inactive devices
				continue;


				/*********************************************/
				/* UPDATE THE STATUS OF THE ASSIGNED ELEMENT */
				/*********************************************/


			e					= gControlNeeds[n].elementInfo[d].elementNum;			// get the index into the device's Element list
			if (e == -1)																// if it's UNDEFINED then skip it
				continue;

			elementType 		= gHIDDeviceList[d].elements[e].elementType;			// get element type
			hidDeviceInterface 	= gHIDDeviceList[d].hidDeviceInterface;					// get hid interface for this device
			cookie				= gHIDDeviceList[d].elements[e].cookie;					// get cookie


						/* GET ELEMENT'S VALUE */

			result = (*hidDeviceInterface)->getElementValue(hidDeviceInterface,	cookie, &hidEvent);		// poll this element
			if (result != noErr)
				value = hidEvent.value = 0;						// NOTE:  HID is buggy as hell!  Sometimes devices return errors until a button has been pressed
			else
				value = hidEvent.value;

						/* CALIBRATE THE VALUE */

			value = CalibrateElementValue(value, d, e, elementType);


						/* SEE IF IT'S THE BEST VALUE SO FAR */

			if (abs(value) > abs(bestValue))
				bestValue = value;
		}


				/* SPECIAL CHECK FOR MOUSE BUTTON FIRE */

		if (n == kNeed_Kick)
		{
			if (gMouseButtonState)
				bestValue = 1;
		}

				/*************************************/
				/* SAVE THE BEST VALUE AS THE RESULT */
				/*************************************/

		gControlNeeds[n].value = bestValue;

		if ((bestValue != 0) && (gControlNeeds[n].oldValue == 0))			// is this a new button press?
			gControlNeeds[n].newButtonPress	= true;
		else
			gControlNeeds[n].newButtonPress	= false;
	}
#endif
}



#pragma mark -
#pragma mark =========== KEYMAP ===========

/**************** UPDATE KEY MAP *************/
//
// This reads the KeyMap and sets a bunch of new/old stuff.
//

static void UpdateKeyMap(void)
{
int		i;

	GetKeys((void *)gKeyMap);

			/* CALC WHICH KEYS ARE NEW THIS TIME */

	for (i = 0; i <  16; i++)
		gNewKeys[i] = (gOldKeys[i] ^ gKeyMap[i]) & gKeyMap[i];


			/* REMEMBER AS OLD MAP */

	for (i = 0; i <  16; i++)
		gOldKeys[i] = gKeyMap[i];



		/*****************************************************/
		/* WHILE WE'RE HERE LET'S DO SOME SPECIAL KEY CHECKS */
		/*****************************************************/

				/* SEE IF QUIT GAME */

	if (GetKeyState(KEY_Q) && GetKeyState(KEY_APPLE))			// see if real key quit
		CleanQuit();


}


/****************** GET KEY STATE ***********/

Boolean GetKeyState(unsigned short key)
{
	//return (BitTst(gKeyMap, key));

	return ( ( gKeyMap[key>>3] >> (key & 7) ) & 1);


//unsigned char *keyMap;

//	keyMap = (unsigned char *)&gKeyMap;
//	return ( ( keyMap[key>>3] >> (key & 7) ) & 1);
}


/****************** GET NEW KEY STATE ***********/

Boolean GetNewKeyState(unsigned short key)
{
//  return !!(BitTst(gNewKeys, (sizeof(KeyMapByteArray)*8) - key));

	return ( ( gNewKeys[key>>3] >> (key & 7) ) & 1);



//unsigned char *keyMap;

//	keyMap = (unsigned char *)&gNewKeys;
//	return ( ( keyMap[key>>3] >> (key & 7) ) & 1);
}


/***************** ARE ANY NEW KEYS PRESSED ****************/

Boolean AreAnyNewKeysPressed(void)
{
int		i;

	for (i = 0; i < 16; i++)
	{
		if (gNewKeys[i])
			return(true);
	}

	return(false);
}


#pragma mark -
#pragma mark ========== MOUSE DELTAS ============


/***************** UPDATE MOUSE DELTA *****************/
//
// Call this only once per frame.
//

static void UpdateMouseDeltas(void)
{
	IMPLEMENT_ME_SOFT();
#if 0
EventRecord   theEvent;

				/* UPDATE DELTAS */

	gReadMouseDeltasTimer -= gFramesPerSecondFrac;			// regulate the rate that we read mouse deltas so we don't go too fast and get just 0's
	if (gReadMouseDeltasTimer <= 0.0f)
	{
		gReadMouseDeltasTimer += .1f;						// read 10 times per second

		gMouseDeltaX = gMouseDeltaY = 0;		 			// assume no deltas will be gotten

		GetNextEvent(kEventMouseMoved, &theEvent);			// getting a Mouse Moved event should trigger the Mouse Event Handler below

		FlushEvents(everyEvent, 0);							// keep the event queue flushed of other key events and stuff
	}

		/* WHILE WE'RE HERE UPDATE THE MOUSE BUTTON STATE */

	if (Button())											// is mouse button down?
	{
		if (!gMouseButtonState)								// is this a new click?
			gMouseNewButtonState = gMouseButtonState = true;
		else
			gMouseNewButtonState = false;
	}
	else
	{
		gMouseButtonState = gMouseNewButtonState = false;
	}
#endif
}


#if 0
/**************** MY MOUSE EVENT HANDLER *************************/
//
// Every time WaitNextEvent() is called this callback will be invoked.
//

static pascal OSStatus MyMouseEventHandler(EventHandlerCallRef eventhandler, EventRef pEventRef, void *userdata)
{
OSStatus			result = eventNotHandledErr;
OSStatus			theErr = noErr;
Point				qdPoint;
#pragma unused (eventhandler, userdata)

	switch (GetEventKind(pEventRef))
	{
		case	kEventMouseMoved:
		case	kEventMouseDragged:
				theErr = GetEventParameter(pEventRef, kEventParamMouseDelta, typeQDPoint,
										nil, sizeof(Point), nil, &qdPoint);

				gMouseDeltaX = qdPoint.h;
				gMouseDeltaY = qdPoint.v;

				result = noErr;
				break;

	}
     return(result);
}
#endif
