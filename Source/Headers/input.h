//
// input.h
//

#pragma once


		/* NEEDS */

enum
{
	kNeed_TurnLeft,
	kNeed_TurnRight,
	kNeed_Forward,
	kNeed_Backward,
	kNeed_Kick,
	kNeed_PickupDrop,
	kNeed_AutoWalk,
	kNeed_Jump,
	kNeed_LaunchBuddy,
	kNeed_CameraMode,
	kNeed_CameraLeft,
	kNeed_CameraRight,
	NUM_REMAPPABLE_NEEDS,

	kNeed_UIUp,
	kNeed_UIDown,
	kNeed_UIPrev,
	kNeed_UINext,
	kNeed_UIConfirm,
	kNeed_UIDelete,
	kNeed_UIStart,
	kNeed_UIBack,
	kNeed_UIPause,
	kNeed_UITextLeft,
	kNeed_UITextRight,
	kNeed_UITextRightOrDone,
	kNeed_UITextBksp,
	kNeed_UITextDone,
	kNeed_UITextNextCh,
	kNeed_UITextPrevCh,
	NUM_CONTROL_NEEDS
};
