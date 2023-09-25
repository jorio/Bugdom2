/****************************/
/*   	  INPUT.C	   	    */
/* By Brian Greenstone      */
/* (c)2003 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/

/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/**********************/
/*     PROTOTYPES     */
/**********************/


/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/


			/**************/
			/* NEEDS LIST */
			/**************/


/********************** UPDATE INPUT ******************************/
//
// This is the master input update function which reads all input values including those from HID devices, GetKeys(), and the Mouse.
//

void UpdateInput(void)
{
float	mouseDX, mouseDY;

	DoSDLMaintenance();


			/****************************************/
			/* UPDATE ALL THE NEEDS CONTROLS VALUES */
			/****************************************/


			/****************************/
			/* SET PLAYER AXIS CONTROLS */
			/****************************/

			/* FIRST CHECK ANALOG AXES */

	gPlayerInfo.analogControlX = GetNeedAnalogSteering(kNeed_TurnLeft_Key, kNeed_TurnRight_Key);
	gPlayerInfo.analogControlZ = GetNeedAnalogSteering(kNeed_Forward, kNeed_Backward);


		/* AND FINALLY SEE IF MOUSE DELTAS ARE BEST */
		//
		// The mouse is only used for Player 1
		//
	
	OGLVector2D mouseDelta = GetMouseDelta();

	mouseDX = mouseDelta.x * 0.015f;							// scale down deltas for our use
	mouseDY = mouseDelta.y * 0.015f;

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



#pragma mark -
#pragma mark ========== MOUSE DELTAS ============


/***************** UPDATE MOUSE DELTA *****************/
//
// Call this only once per frame.
//

#if 0
static void UpdateMouseDeltas(void)
{
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
}

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
