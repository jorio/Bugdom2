//
// input.h
//

#ifndef __INPUT_H
#define __INPUT_H


							// KEYBOARD DEFINES
							//=================================

#define KEY_A				0x00
#define KEY_B				0x0b
#define	KEY_C				0x08
#define KEY_D				0x02
#define KEY_E				0x0e
#define KEY_F				0x03
#define KEY_G				0x05
#define KEY_H				0x04
#define KEY_I				0x22
#define KEY_J				0x26
#define KEY_K				0x28
#define KEY_L				0x25
#define KEY_M				0x2e
#define KEY_N				0x2d
#define KEY_O				0x1f
#define	KEY_P				0x23
#define KEY_Q				0x0c
#define KEY_R				0x0f
#define KEY_S				0x01
#define KEY_T				0x11
#define KEY_U				0x20
#define KEY_V				0x09
#define KEY_W				0x0d
#define KEY_X				0x07
#define KEY_Y				0x10
#define KEY_Z				0x06

#define	KEY_1				0x12
#define KEY_2				0x13
#define KEY_3				0x14
#define KEY_4				0x15
#define KEY_5				0x17
#define KEY_6				0x16
#define KEY_7				0x1a
#define KEY_8				0x1c
#define KEY_9				0x19
#define KEY_0				0x1d

#define	KEY_K0				0x52
#define	KEY_K1				0x53
#define	KEY_K2				0x54
#define	KEY_K3				0x55
#define	KEY_K4				0x56
#define	KEY_K5				0x57
#define	KEY_K6				0x58
#define	KEY_K7				0x59
#define	KEY_K8				0x5b
#define	KEY_K9				0x5c

#define KEY_PERIOD			0x2f
#define	KEY_QMARK			0x2c
#define	KEY_COMMA			0X2b

#define KEY_TAB				0x30
#define KEY_ESC				0x35
#define	KEY_CAPSLOCK		0x39
#define KEY_APPLE			0x37
#define KEY_SPACE			0x31
#define KEY_OPTION			0x3a
#define	KEY_CTRL			0x3b
#define	KEY_UP				0x7e
#define	KEY_DOWN			0x7d
#define	KEY_LEFT			0x7b
#define	KEY_RIGHT			0x7c
#define	KEY_SHIFT			0x38
#define	KEY_DELETE			0x33
#define	KEY_RETURN			0x24
#define	KEY_MINUS			0x1b
#define	KEY_PLUS			0x18

#define KEY_F1				0x7a
#define KEY_F2				0x78
#define KEY_F3				0x63
#define KEY_F4				0x76
#define KEY_F5				0x60
#define KEY_F6				0x61
#define KEY_F7				0x62
#define KEY_F8				0x64
#define KEY_F9				0x65
#define KEY_F10				0x6d
#define KEY_F11				0x67
#define KEY_F12				0x6f
#define KEY_F13				0x69
#define KEY_F14				0x6b
#define KEY_F15				0x71

#define KEY_TILDE			0x32

#define KEY_HELP			0x72

enum
{									// remember to update gNeedToKey!!!
	kKey_Jump			= KEY_SPACE,
	kKey_Kick			= KEY_APPLE,
	kKey_AutoWalk		= KEY_SHIFT,
	kKey_Pickup			= KEY_OPTION,
	kKey_CameraRight	= KEY_COMMA,
	kKey_CameraLeft		= KEY_PERIOD,
	kKey_CameraMode		= KEY_RETURN,
	kKey_Left			= KEY_LEFT,
	kKey_Right			= KEY_RIGHT,
	kKey_Forward		= KEY_UP,
	kKey_Backward		= KEY_DOWN,
	kKey_BuddyAttack	= KEY_TAB
};


		/* NEEDS */

enum
{
	kNeed_TurnLeft_Key	= 0,
	kNeed_TurnRight_Key,
	kNeed_Forward,
	kNeed_Backward,
	kNeed_XAxis,
	kNeed_YAxis,
	kNeed_Kick,
	kNeed_PickupDrop,
	kNeed_AutoWalk,
	kNeed_Jump,
	kNeed_LaunchBuddy,
	kNeed_CameraMode,
	kNeed_CameraLeft,
	kNeed_CameraRight,

	NUM_CONTROL_NEEDS
};


		/* HID STUFF */

#define	MAX_HID_DEVICES				8			// max # of HID devices we'll allow
#define	MAX_HID_ELEMENTS			150			// max # of elements per HID device

typedef struct
{
	Byte		maxNeeds;						// copies of the code constants
	Byte		maxDevices;
	Byte		maxElement;

	Byte		numDevices;						// # devices we're saving info for

	Boolean		isActive[MAX_HID_DEVICES];		// flags indicating which HID devices in our list are set to Active by the user
	long		vendorID[MAX_HID_DEVICES];
	long		productID[MAX_HID_DEVICES];
	Boolean		hasDuplicate[MAX_HID_DEVICES];  // true if there might be another identical device
	long		locationID[MAX_HID_DEVICES];
	long		calibrationMin[MAX_HID_DEVICES][MAX_HID_ELEMENTS];
	long		calibrationMax[MAX_HID_DEVICES][MAX_HID_ELEMENTS];
	long		calibrationCenter[MAX_HID_DEVICES][MAX_HID_ELEMENTS];
	Boolean		reverseAxis[MAX_HID_DEVICES][MAX_HID_ELEMENTS];

	long		needsElementNum[NUM_CONTROL_NEEDS][MAX_HID_DEVICES];

}HIDControlSettingsType;


			/* NEEDS DATA TYPES */

typedef struct
{
	short				elementNum;						// index into device's element list
	long				elementCurrentValue;			// the current polled value of this element
}NeedElementInfoType;

typedef struct
{
	char					*name;							// name of the Need
	short					defaultKeyboardKey;				// default keyboard key
	NeedElementInfoType		elementInfo[MAX_HID_DEVICES];	// for each possible device, all the info about its assigned element
	long					value;							// current element value
	long					oldValue;						// element's value on previous frame
	Boolean					newButtonPress;					// true when value is > 0 and oldValue == 0
}InputNeedType;




			/* EXTERNS */

extern	InputNeedType	gControlNeeds[];
extern	Boolean			gMouseButtonState, gMouseNewButtonState, gHIDInitialized;




//============================================================================================


void InitInput(void);
void UpdateInput(void);
Boolean GetNewKeyState(unsigned short key);
Boolean GetKeyState(unsigned short key);
Boolean AreAnyNewKeysPressed(void);


void ShutdownHID(void);

void DoInputConfigDialog(void);

void BuildHIDControlSettings(HIDControlSettingsType *settings);
void RestoreHIDControlSettings(HIDControlSettingsType *settings);



#endif


