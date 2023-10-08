// MENU.C
// (c)2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"
#include "menu.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static ObjNode* MakeTextAtRowCol(const char* text, int row, int col);
static void LayOutMenu(int menuID);
static const MenuItem* LookUpMenu(int menuID);
static ObjNode* LayOutCyclerValueText(int row);
static void BounceCursorDot(void);

#define SpecialRow					Special[0]
#define SpecialCol					Special[1]
#define SpecialPulsateTimer			SpecialF[0]
#define SpecialSweepTimer			SpecialF[1]

/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_MENU_ROWS	32
#define MAX_MENU_COLS	5
#define MAX_REGISTERED_MENUS	32
#define MAX_STACK_LENGTH		16		// for history

#define kSfxCycle		EFFECT_ACORNKICKED
#define kSfxError		EFFECT_SKIPLAND
#define kSfxDelete		EFFECT_POPACORN

const int16_t kJoystickDeadZone_BindingThreshold = (75 * 32767 / 100);

enum
{
	kMenuStateOff,
	kMenuStateFadeIn,
	kMenuStateReady,
	kMenuStateFadeOut,
	kMenuStateAwaitingKeyPress,
	kMenuStateAwaitingPadPress,
	kMenuStateAwaitingMouseClick,
};

#define GS 0.3f
const MenuStyle kDefaultMenuStyle =
{
	.darkenPaneScaleY	= 480,
	.darkenPaneOpacity	= .75f,
	.fadeInSpeed		= 3.0f,
	.fadeOutSpeed		= 3.0f,
	.asyncFadeOut		= true,
	.centeredText		= false,
	.titleColor			= {1.0f, 1.0f, 0.2f, 1.0f},
	.inactiveColor		= {0.3f, 0.7f, 0.2f, 1.0f},
	.inactiveColor2		= {0.2f, 0.4f, 0.8f, 0.5f},
	.standardScale		= GS * 1.0f,
	.titleScale			= GS * 1.25f,
	.subtitleScale		= GS * .8f,
	.rowHeight			= 13*1.5f,
	.uniformXExtent		= 0,
	.playMenuChangeSounds	= true,
	.startButtonExits	= false,
	.isInteractive		= true,
	.canBackOutOfRootMenu = true,
	.offset				= {640/2, 480/2},		// Bugdom 2
};

/*********************/
/*    VARIABLES      */
/*********************/

static const MenuItem*		gMenu = nil;
static const MenuStyle*		gMenuStyle = nil;
static int					gNumMenuEntries;
static int					gMenuRow = 0;
static int					gKeyColumn = 0;
static int					gPadColumn = 0;
static float				gMenuColXs[MAX_MENU_COLS] = { 0, 190, 300, 430, 560 };
static float				gMenuRowYs[MAX_MENU_ROWS];
static float				gMenuFadeAlpha = 0;
static int					gMenuState = kMenuStateOff;
static int					gMenuPick = -1;
static ObjNode*				gMenuObjects[MAX_MENU_ROWS][MAX_MENU_COLS];
static ObjNode*				gMenuCursorDot = nil;

static bool					gMouseHoverValidRow = false;
static int					gMouseHoverColumn = -1;

static float				gMenuAsyncFadeOutSpeed = 2.0f;

struct { int menuID, row; }	gMenuHistory[MAX_STACK_LENGTH];
int							gMenuHistoryPos;

static int gNumMenusRegistered = 0;
const MenuItem* gMenuRegistry[MAX_REGISTERED_MENUS];

/****************************/
/*    MENU UTILITIES        */
/****************************/
#pragma mark - Utilities

static OGLColorRGBA PulsateColor(float* time)
{
	*time += gFramesPerSecondFrac;
	float intensity = 0.6f + 0.4f * SDL_sinf(*time * 10.0f);
	return (OGLColorRGBA) {
		.3f * intensity + 1.0f * (1.0f - intensity),
		.7f * intensity + 1.0f * (1.0f - intensity),
		.2f * intensity + 1.0f * (1.0f - intensity),
		1};
}

static InputBinding* GetBindingAtRow(int row)
{
	return &gGamePrefs.bindings[gMenu[row].kb];
}

static const char* GetKeyBindingName(int row, int col)
{
	int16_t scancode = GetBindingAtRow(row)->key[col];

	switch (scancode)
	{
		case 0:
			return Localize(STR_UNBOUND_PLACEHOLDER);
		case SDL_SCANCODE_COMMA:				// on a US layout, it makes more sense to show "<" for camera left
			return "<";
		case SDL_SCANCODE_PERIOD:				// on a US layout, it makes more sense to show ">" for camera right
			return ">";
		case SDL_SCANCODE_LGUI:
			return "Left ⌘";
		case SDL_SCANCODE_RGUI:
			return "Right ⌘";
		case SDL_SCANCODE_LSHIFT:
			return "Left ⇧";
		case SDL_SCANCODE_RSHIFT:
			return "Right ⇧";
#if __APPLE__
		case SDL_SCANCODE_LALT:
			return "Left ⌥";
		case SDL_SCANCODE_RALT:
			return "Right ⌥";
#endif
		default:
			return SDL_GetScancodeName(scancode);
	}
}

static const char* GetPadBindingName(int row, int col)
{
	InputBinding* kb = GetBindingAtRow(row);

	switch (kb->pad[col].type)
	{
		case kInputTypeUnbound:
			return Localize(STR_UNBOUND_PLACEHOLDER);

		case kInputTypeButton:
			switch (kb->pad[col].id)
			{
				case SDL_CONTROLLER_BUTTON_INVALID:			return Localize(STR_UNBOUND_PLACEHOLDER);
				case SDL_CONTROLLER_BUTTON_A:				return "Ⓐ";
				case SDL_CONTROLLER_BUTTON_B:				return "Ⓑ";
				case SDL_CONTROLLER_BUTTON_X:				return "Ⓧ";
				case SDL_CONTROLLER_BUTTON_Y:				return "Ⓨ";
				case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:	return "LB";
				case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:	return "RB";
				case SDL_CONTROLLER_BUTTON_LEFTSTICK:		return "Push LS";
				case SDL_CONTROLLER_BUTTON_RIGHTSTICK:		return "Push RS";
				case SDL_CONTROLLER_BUTTON_DPAD_UP:			return "D-pad up";
				case SDL_CONTROLLER_BUTTON_DPAD_DOWN:		return "D-pad down";
				case SDL_CONTROLLER_BUTTON_DPAD_LEFT:		return "D-pad left";
				case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:		return "D-pad right";
				default:
					return SDL_GameControllerGetStringForButton(kb->pad[col].id);
			}
			break;

		case kInputTypeAxisPlus:
			switch (kb->pad[col].id)
			{
				case SDL_CONTROLLER_AXIS_LEFTX:				return "LS right";
				case SDL_CONTROLLER_AXIS_LEFTY:				return "LS down";
				case SDL_CONTROLLER_AXIS_RIGHTX:			return "RS right";
				case SDL_CONTROLLER_AXIS_RIGHTY:			return "RS down";
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:		return "LT";
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:		return "RT";
				default:
					return SDL_GameControllerGetStringForAxis(kb->pad[col].id);
			}
			break;

		case kInputTypeAxisMinus:
			switch (kb->pad[col].id)
			{
				case SDL_CONTROLLER_AXIS_LEFTX:				return "LS left";
				case SDL_CONTROLLER_AXIS_LEFTY:				return "LS up";
				case SDL_CONTROLLER_AXIS_RIGHTX:			return "RS left";
				case SDL_CONTROLLER_AXIS_RIGHTY:			return "RS up";
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:		return "LT";
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:		return "RT";
				default:
					return SDL_GameControllerGetStringForAxis(kb->pad[col].id);
			}
			break;

		default:
			return "???";
	}
}

static const char* GetMouseBindingName(int row)
{
	static char buf[16];

	InputBinding* kb = GetBindingAtRow(row);

	switch (kb->mouseButton)
	{
		case 0:							return Localize(STR_UNBOUND_PLACEHOLDER);
		case SDL_BUTTON_LEFT:			return Localize(STR_MOUSE_BUTTON_LEFT);
		case SDL_BUTTON_MIDDLE:			return Localize(STR_MOUSE_BUTTON_MIDDLE);
		case SDL_BUTTON_RIGHT:			return Localize(STR_MOUSE_BUTTON_RIGHT);
		case SDL_BUTTON_WHEELUP:		return Localize(STR_MOUSE_WHEEL_UP);
		case SDL_BUTTON_WHEELDOWN:		return Localize(STR_MOUSE_WHEEL_DOWN);
		default:
			SDL_snprintf(buf, sizeof(buf), "%s %d", Localize(STR_BUTTON), kb->mouseButton);
			return buf;
	}
}

static bool IsMenuItemTypeSelectable(int type)
{
	switch (type)
	{
		case kMISpacer:
		case kMILabel:
		case kMITitle:
		case kMISubtitle:
			return false;

		default:
			return true;
	}
}

static void ReplaceMenuText(LocStrID originalTextInMenuDefinition, LocStrID newText)
{
	for (int i = 0; i < MAX_MENU_ROWS && gMenu[i].type != kMISENTINEL; i++)
	{
		if (gMenu[i].text == originalTextInMenuDefinition)
		{
			MakeTextAtRowCol(Localize(newText), i, 0);
		}
	}
}

static void PlayNavigateEffect(void)
{
	PlayEffect_Parms(EFFECT_CHANGESELECT,FULL_CHANNEL_VOLUME/4,FULL_CHANNEL_VOLUME/4,NORMAL_CHANNEL_RATE);
}

static void PlayMenuChangeEffect(void)
{
	PlayEffect_Parms(EFFECT_CHANGESELECT,FULL_CHANNEL_VOLUME/4,FULL_CHANNEL_VOLUME/4,NORMAL_CHANNEL_RATE*3/2);
}

/****************************/
/*    MENU MOVE CALLS       */
/****************************/
#pragma mark - Move calls

static void MoveDarkenPane(ObjNode* node)
{
	node->ColorFilter.a = gMenuFadeAlpha * gMenuStyle->darkenPaneOpacity;
}

static void MoveGenericMenuItem(ObjNode* node)
{
	node->SpecialSweepTimer += gFramesPerSecondFrac * 5;

	if (node->SpecialSweepTimer < 0)
	{
		node->ColorFilter.a = 0;
	}
	else if (node->SpecialSweepTimer < 1)
	{
		node->ColorFilter.a *= node->SpecialSweepTimer;

//		float xBackup = node->Coord.x;
//
//		float p = (1.0f - node->SpecialSweepTimer);
//		node->Coord.x -= p*p * 50.0f;
//		UpdateObjectTransforms(node);
//
//		node->Coord.x = xBackup;
	}
	else
	{
		UpdateObjectTransforms(node);
	}
}

static void MoveLabel(ObjNode* node)
{
	node->ColorFilter.a = gMenuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MoveAction(ObjNode* node)
{
	if (node->SpecialRow == gMenuRow)
		node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
	else
		node->ColorFilter = gMenuStyle->inactiveColor;

	node->ColorFilter.a *= gMenuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MoveKeyBinding(ObjNode* node)
{
	if (node->SpecialRow == gMenuRow && node->SpecialCol == (gKeyColumn+1))
	{
		if (gMenuState == kMenuStateAwaitingKeyPress)
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
		else
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
	}
	else
		node->ColorFilter = gMenuStyle->inactiveColor;

	node->ColorFilter.a *= gMenuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MovePadBinding(ObjNode* node)
{
	if (node->SpecialRow == gMenuRow && node->SpecialCol == (gPadColumn+1))
	{
		if (gMenuState == kMenuStateAwaitingPadPress)
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
		else
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
	}
	else
		node->ColorFilter = gMenuStyle->inactiveColor;

	node->ColorFilter.a *= gMenuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MoveMouseBinding(ObjNode* node)
{
	if (node->SpecialRow == gMenuRow)
	{
		if (gMenuState == kMenuStateAwaitingMouseClick)
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
		else
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
	}
	else
		node->ColorFilter = gMenuStyle->inactiveColor;

	node->ColorFilter.a *= gMenuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MoveAsyncFadeOutAndDelete(ObjNode *theNode)
{
	theNode->ColorFilter.a -= gFramesPerSecondFrac * gMenuAsyncFadeOutSpeed;
	if (theNode->ColorFilter.a < 0.0f)
		DeleteObject(theNode);
}

/****************************/
/*    MENU HISTORY          */
/****************************/
#pragma mark - Menu history

static void SaveSelectedRowInHistory(void)
{
	gMenuHistory[gMenuHistoryPos].row = gMenuRow;
}

static void GoBackInHistory(void)
{
	MyFlushEvents();

	if (gMenuHistoryPos != 0)
	{
//		PlayBackEffect();
		gMenuHistoryPos--;

//		gNav->sweepRTL = true;
		LayOutMenu(gMenuHistory[gMenuHistoryPos].menuID);
//		gNav->sweepRTL = false;
	}
	else if (gMenuStyle->canBackOutOfRootMenu)
	{
//		PlayBackEffect();
		gMenuState = kMenuStateFadeOut;
	}
	else
	{
		PlayEffect(kSfxError);
	}
}

/****************************/
/*    MENU NAVIGATION       */
/****************************/
#pragma mark - Menu navigation

static void NavigateSettingEntriesVertically(int delta)
{
	int browsed = 0;
	bool skipEntry = false;
	
	bool mute = gMenuRow < 0;

	do
	{
		gMenuRow += delta;
		gMenuRow = PositiveModulo(gMenuRow, (unsigned int)gNumMenuEntries);

		skipEntry = !IsMenuItemTypeSelectable(gMenu[gMenuRow].type);

		if (browsed++ > gNumMenuEntries)
		{
			// no entries are selectable
			return;
		}
	} while (skipEntry);

	if (!mute)
		PlayNavigateEffect();
	gMouseHoverValidRow = false;
}

static void NavigateSettingEntriesMouseHover(void)
{
	if (!gMouseMotionNow)
		return;

	OGLPoint2D mp = GetMouseCoordsIn2DLogicalRect();
	float mx = mp.x;
	float my = mp.y;

	gMouseHoverValidRow = false;
	gMouseHoverColumn = -1;

	for (int row = 0; row < gNumMenuEntries; row++)
	{
		if (!IsMenuItemTypeSelectable(gMenu[row].type))
			continue;

		OGLRect fullExtents;
		fullExtents.top		= fullExtents.left	= 100000;
		fullExtents.bottom	= fullExtents.right	= -100000;

		for (int col = 0; col < MAX_MENU_COLS; col++)
		{
			ObjNode* textNode = gMenuObjects[row][col];
			if (!textNode)
				continue;

			OGLRect extents = TextMesh_GetExtents(textNode);
			if (extents.top		< fullExtents.top	) fullExtents.top		= extents.top;
			if (extents.left	< fullExtents.left	) fullExtents.left		= extents.left;
			if (extents.bottom	> fullExtents.bottom) fullExtents.bottom	= extents.bottom;
			if (extents.right	> fullExtents.right	) fullExtents.right		= extents.right;

			if (my >= extents.top
				&& my <= extents.bottom
				&& mx >= extents.left - 10
				&& mx <= extents.right + 10)
			{
				gMouseHoverColumn = col;
			}
		}

		if (my >= fullExtents.top &&
			my <= fullExtents.bottom &&
			mx >= fullExtents.left - 10 &&
			mx <= fullExtents.right + 10)
		{
			gMouseHoverValidRow = true;
			SetSystemCursor(SDL_SYSTEM_CURSOR_HAND);

			if (gMenuRow != row)
			{
				gMenuRow = row;
//				PlayNavigateEffect();
			}

			return;
		}
	}

	GAME_ASSERT(!gMouseHoverValidRow);		// if we got here, we're not hovering over anything

	SetSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
}

static void NavigatePick(const MenuItem* entry)
{
	bool validClick = (gMouseHoverValidRow && IsClickDown(SDL_BUTTON_LEFT));

	if (IsNeedDown(kNeed_UIConfirm) || validClick)
	{
//		if (!validClick)
//			gNav->mouseState = kMouseOff;		// exit mouse control if didn't get click

//		gNav->idleTime = 0;
		gMenuPick = entry->id;

		if (entry->callback)
		{
			entry->callback();
		}

		switch (entry->next)
		{
			case 0:
			case 'NOOP':
				PlayEffect(kSfxCycle);
				BounceCursorDot();
				break;

			case 'EXIT':
				PlayMenuChangeEffect();
				gMenuState = kMenuStateFadeOut;
				break;

			case 'BACK':
				PlayMenuChangeEffect();
				GoBackInHistory();
				break;

			default:
				SaveSelectedRowInHistory();  // remember which row we were on

				// advance history
				gMenuHistoryPos++;
				GAME_ASSERT(gMenuHistoryPos < MAX_STACK_LENGTH);
				gMenuHistory[gMenuHistoryPos].menuID = entry->next;
				gMenuHistory[gMenuHistoryPos].row = 0;  // don't reuse stale row value

				PlayMenuChangeEffect();
				LayOutMenu(entry->next);
		}
	}
}

static void NavigateCycler(const MenuItem* entry)
{
	int delta = 0;

	if (IsNeedDown(kNeed_UIPrev)
		|| (gMouseHoverValidRow && IsClickDown(SDL_BUTTON_RIGHT)))
	{
		delta = -1;
	}
	else if (IsNeedDown(kNeed_UINext)
		|| IsNeedDown(kNeed_UIConfirm)
		|| (gMouseHoverValidRow && IsClickDown(SDL_BUTTON_LEFT)))
	{
		delta = 1;
	}

	if (delta != 0)
	{
		PlayEffect_Parms(kSfxCycle, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE + (RandomFloat2() * 0x3000));

		BounceCursorDot();

		if (entry->cycler.valuePtr && !entry->cycler.callbackSetsValue)
		{
			unsigned int value = (unsigned int)*entry->cycler.valuePtr;
			value = PositiveModulo(value + delta, entry->cycler.generateNumChoices? entry->cycler.generateNumChoices(): entry->cycler.numChoices);
			*entry->cycler.valuePtr = value;
		}

		if (entry->callback)
			entry->callback();

		LayOutCyclerValueText(gMenuRow);
	}
}

static void NavigateKeyBinding(const MenuItem* entry)
{
	if (gMouseHoverValidRow && (gMouseHoverColumn == 1 || gMouseHoverColumn == 2))
		gKeyColumn = gMouseHoverColumn - 1;

	if (IsNeedDown(kNeed_UIPrev))
	{
		gKeyColumn = PositiveModulo(gKeyColumn - 1, MAX_USER_BINDINGS_PER_NEED);
		PlayNavigateEffect();
		gMouseHoverValidRow = false;
		return;
	}

	if (IsNeedDown(kNeed_UINext))
	{
		gKeyColumn = PositiveModulo(gKeyColumn + 1, MAX_USER_BINDINGS_PER_NEED);
		PlayNavigateEffect();
		gMouseHoverValidRow = false;
		return;
	}

	// Past this point we must have a valid column
	if (gKeyColumn < 0 || gKeyColumn >= MAX_USER_BINDINGS_PER_NEED)
	{
		return;
	}

	if (IsNeedDown(kNeed_UIDelete)
		|| (gMouseHoverValidRow && IsClickDown(SDL_BUTTON_MIDDLE)))
	{
		gGamePrefs.bindings[entry->kb].key[gKeyColumn] = 0;

		PlayEffect(kSfxDelete);
		MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), gMenuRow, gKeyColumn+1);
		return;
	}

	if (IsKeyDown(SDL_SCANCODE_RETURN)
		|| (gMouseHoverValidRow && IsClickDown(SDL_BUTTON_LEFT)))
	{
		PlayEffect(kSfxCycle);
		BounceCursorDot();

		gMenuState = kMenuStateAwaitingKeyPress;
		MakeTextAtRowCol(Localize(STR_PRESS), gMenuRow, gKeyColumn+1);

		// Change subtitle to help message
		ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP_CANCEL);

		return;
	}
}

static void NavigatePadBinding(const MenuItem* entry)
{
	if (gMouseHoverValidRow && (gMouseHoverColumn == 1 || gMouseHoverColumn == 2))
		gPadColumn = gMouseHoverColumn - 1;

	if (IsNeedDown(kNeed_UIPrev))
	{
		gPadColumn = PositiveModulo(gPadColumn - 1, MAX_USER_BINDINGS_PER_NEED);
		PlayNavigateEffect();
		gMouseHoverValidRow = false;
		return;
	}

	if (IsNeedDown(kNeed_UINext))
	{
		gPadColumn = PositiveModulo(gPadColumn + 1, MAX_USER_BINDINGS_PER_NEED);
		PlayNavigateEffect();
		gMouseHoverValidRow = false;
		return;
	}

	if (IsNeedDown(kNeed_UIDelete)
		|| (gMouseHoverValidRow && IsClickDown(SDL_BUTTON_MIDDLE)))
	{
		gGamePrefs.bindings[entry->kb].pad[gPadColumn].type = kInputTypeUnbound;

		PlayEffect(kSfxDelete);
		MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), gMenuRow, gPadColumn+1);
		return;
	}

	if (IsNeedDown(kNeed_UIConfirm)
		|| (gMouseHoverValidRow && IsClickDown(SDL_BUTTON_LEFT)))
	{
		while (GetNeedState(kNeed_UIConfirm))
		{
			UpdateInput();
			SDL_Delay(30);
		}

		PlayEffect(kSfxCycle);
		BounceCursorDot();

		gMenuState = kMenuStateAwaitingPadPress;
		MakeTextAtRowCol(Localize(STR_PRESS), gMenuRow, gPadColumn+1);

		// Change subtitle to help message
		ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_CONFIGURE_GAMEPAD_HELP_CANCEL);

		return;
	}
}

static void NavigateMouseBinding(const MenuItem* entry)
{
	if (IsNeedDown(kNeed_UIDelete)
		|| (gMouseHoverValidRow && IsClickDown(SDL_BUTTON_MIDDLE)))
	{
		gGamePrefs.bindings[entry->kb].mouseButton = 0;

		PlayEffect(kSfxDelete);
		MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), gMenuRow, 1);
		return;
	}

	if (IsNeedDown(kNeed_UIConfirm)
		|| (gMouseHoverValidRow && IsClickDown(SDL_BUTTON_LEFT)))
	{
		while (GetNeedState(kNeed_UIConfirm))
		{
			UpdateInput();
			SDL_Delay(30);
		}

		PlayEffect(kSfxCycle);
		BounceCursorDot();

		gMenuState = kMenuStateAwaitingMouseClick;
		MakeTextAtRowCol(Localize(STR_CLICK), gMenuRow, 1);
		return;
	}
}

static void NavigateMenu(void)
{
	GAME_ASSERT(gMenuStyle->isInteractive);

	if (IsNeedDown(kNeed_UIBack))
	{
		PlayMenuChangeEffect();
		GoBackInHistory();
		return;
	}

	if (IsNeedDown(kNeed_UIUp))
	{
		NavigateSettingEntriesVertically(-1);
		SaveSelectedRowInHistory();
	}

	if (IsNeedDown(kNeed_UIDown))
	{
		NavigateSettingEntriesVertically(1);
		SaveSelectedRowInHistory();
	}

	NavigateSettingEntriesMouseHover();

	const MenuItem* entry = &gMenu[gMenuRow];

	switch (entry->type)
	{
		case kMIPick:
			NavigatePick(entry);
			break;

		case kMICycler:
			NavigateCycler(entry);
			break;

		case kMIKeyBinding:
			NavigateKeyBinding(entry);
			break;

		case kMIPadBinding:
			NavigatePadBinding(entry);
			break;

		case kMIMouseBinding:
			NavigateMouseBinding(entry);

		default:
			//DoFatalAlert("Not supposed to be hovering on this menu item!");
			break;
	}
}

/****************************/
/* INPUT BINDING CALLBACKS  */
/****************************/
#pragma mark - Input binding callbacks

static void UnbindScancodeFromAllRemappableInputNeeds(int16_t sdlScancode)
{
	for (int row = 0; row < gNumMenuEntries; row++)
	{
		if (gMenu[row].type != kMIKeyBinding)
			continue;

		InputBinding* binding = GetBindingAtRow(row);
		for (int j = 0; j < MAX_USER_BINDINGS_PER_NEED; j++)
		{
			if (binding->key[j] == sdlScancode)
			{
				binding->key[j] = 0;
				MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), row, j+1);
			}
		}
	}
}

static void UnbindPadButtonFromAllRemappableInputNeeds(int8_t type, int8_t id)
{
	for (int row = 0; row < gNumMenuEntries; row++)
	{
		if (gMenu[row].type != kMIPadBinding)
			continue;

		InputBinding* binding = GetBindingAtRow(row);

		for (int j = 0; j < MAX_USER_BINDINGS_PER_NEED; j++)
		{
			if (binding->pad[j].type == type && binding->pad[j].id == id)
			{
				binding->pad[j].type = kInputTypeUnbound;
				binding->pad[j].id = 0;
				MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), row, j+1);
			}
		}
	}
}

static void UnbindMouseButtonFromAllRemappableInputNeeds(int8_t id)
{
	for (int row = 0; row < gNumMenuEntries; row++)
	{
		if (gMenu[row].type != kMIMouseBinding)
			continue;

		InputBinding* binding = GetBindingAtRow(row);

		if (binding->mouseButton == id)
		{
			binding->mouseButton = 0;
			MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), row, 1);
		}
	}
}

static void AwaitKeyPress(void)
{
	if (IsKeyDown(SDL_SCANCODE_ESCAPE))
	{
		MakeTextAtRowCol(GetKeyBindingName(gMenuRow, gKeyColumn), gMenuRow, 1 + gKeyColumn);
		gMenuState = kMenuStateReady;
		PlayEffect(kSfxError);
		ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP);
		return;
	}

	InputBinding* kb = GetBindingAtRow(gMenuRow);

	for (int16_t scancode = 0; scancode < SDL_NUM_SCANCODES; scancode++)
	{
		if (IsKeyDown(scancode))
		{
			UnbindScancodeFromAllRemappableInputNeeds(scancode);
			kb->key[gKeyColumn] = scancode;

			MakeTextAtRowCol(GetKeyBindingName(gMenuRow, gKeyColumn), gMenuRow, gKeyColumn+1);
			gMenuState = kMenuStateReady;
			PlayEffect(kSfxCycle);
			ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP);
			return;
		}
	}
}

static void AwaitPadPress(void)
{
	SDL_GameController* gSDLController = GetController();

	if (IsKeyDown(SDL_SCANCODE_ESCAPE)
		|| (gSDLController && SDL_GameControllerGetButton(gSDLController, SDL_CONTROLLER_BUTTON_START)))
	{
		MakeTextAtRowCol(GetPadBindingName(gMenuRow, gPadColumn), gMenuRow, 1 + gPadColumn);
		gMenuState = kMenuStateReady;
		PlayEffect(kSfxError);
		ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_CONFIGURE_GAMEPAD_HELP);
		return;
	}

	InputBinding* kb = GetBindingAtRow(gMenuRow);

	if (!gSDLController)
		return;

	for (int8_t button = 0; button < SDL_CONTROLLER_BUTTON_MAX; button++)
	{
#if 0
		switch (button)
		{
			case SDL_CONTROLLER_BUTTON_DPAD_UP:			// prevent binding those
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				continue;
		}
#endif

		if (SDL_GameControllerGetButton(gSDLController, button))
		{
			UnbindPadButtonFromAllRemappableInputNeeds(kInputTypeButton, button);
			kb->pad[gPadColumn].type = kInputTypeButton;
			kb->pad[gPadColumn].id = button;

			MakeTextAtRowCol(GetPadBindingName(gMenuRow, gPadColumn), gMenuRow, gPadColumn+1);
			gMenuState = kMenuStateReady;
			PlayEffect(kSfxCycle);
			ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_CONFIGURE_GAMEPAD_HELP);
			return;
		}
	}

	for (int8_t axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; axis++)
	{
#if 0
		switch (axis)
		{
			case SDL_CONTROLLER_AXIS_LEFTX:				// prevent binding those
			case SDL_CONTROLLER_AXIS_LEFTY:
			case SDL_CONTROLLER_AXIS_RIGHTX:
			case SDL_CONTROLLER_AXIS_RIGHTY:
				continue;
		}
#endif

		int16_t axisValue = SDL_GameControllerGetAxis(gSDLController, axis);
		if (abs(axisValue) > kJoystickDeadZone_BindingThreshold)
		{
			int axisType = axisValue < 0 ? kInputTypeAxisMinus : kInputTypeAxisPlus;
			UnbindPadButtonFromAllRemappableInputNeeds(axisType, axis);
			kb->pad[gPadColumn].type = axisType;
			kb->pad[gPadColumn].id = axis;

			MakeTextAtRowCol(GetPadBindingName(gMenuRow, gPadColumn), gMenuRow, gPadColumn+1);
			gMenuState = kMenuStateReady;
			PlayEffect(kSfxCycle);
			ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP_CANCEL, STR_CONFIGURE_GAMEPAD_HELP);
			return;
		}
	}
}

static void AwaitMouseClick(void)
{
	if (IsKeyDown(SDL_SCANCODE_ESCAPE))
	{
		MakeTextAtRowCol(GetMouseBindingName(gMenuRow), gMenuRow, 1);
		gMenuState = kMenuStateReady;
		PlayEffect(kSfxError);
		return;
	}

	InputBinding* kb = GetBindingAtRow(gMenuRow);

	for (int8_t mouseButton = 0; mouseButton < NUM_SUPPORTED_MOUSE_BUTTONS; mouseButton++)
	{
		if (IsClickDown(mouseButton))
		{
			UnbindMouseButtonFromAllRemappableInputNeeds(mouseButton);
			kb->mouseButton = mouseButton;

			MakeTextAtRowCol(GetMouseBindingName(gMenuRow), gMenuRow, 1);
			gMenuState = kMenuStateReady;
			PlayEffect(kSfxCycle);
			return;
		}
	}
}

/****************************/
/*    CURSOR DOT            */
/****************************/
#pragma mark - Cursor Dot

static void MoveCursorDot(ObjNode* theNode)
{
	static OGLPoint3D targetPoint;
	static OGLPoint3D currentPoint;
	static float flutter = 0;
	static float flutterMag = 1.25f;
	static float flutterSpeed = 3;
	static const float trackSpeed = 8.0f;

	if (gMenuRow < 0)
		return;

	int column = 0;

	if (gMenu[gMenuRow].type == kMIKeyBinding)
		column = 1+gKeyColumn;
	else if (gMenu[gMenuRow].type == kMIPadBinding)
		column = 1+gPadColumn;
	else
		column = 0;

	targetPoint = gMenuObjects[gMenuRow][column]->Coord;
	targetPoint.x -= 16;
	targetPoint.y += 3;

	if (!theNode->Flag[0])
	{
		// Initial position
		currentPoint = targetPoint;
		theNode->Flag[0] = true;
	}
	else
	{
		float diffX = targetPoint.x - currentPoint.x;
		float diffY = targetPoint.y - currentPoint.y;

		if (fabsf(diffX) < .5f)
			currentPoint.x = targetPoint.x;
		else
			currentPoint.x += gFramesPerSecondFrac * diffX * trackSpeed;

		if (fabsf(diffY) < .5f)
		{
			currentPoint.y = targetPoint.y;
			theNode->Rot.y = diffY * 0;
		}
		else
		{
			currentPoint.y += gFramesPerSecondFrac * diffY * trackSpeed;
			theNode->Rot.y = diffY * .033f;
			theNode->Rot.y = SDL_clamp(theNode->Rot.y, -PI / 2, PI / 2);
		}
	}

	theNode->Coord = currentPoint;

	theNode->Coord.x -= 3 * fabsf(sinf(flutter * 2));
//		theNode->Coord.x += flutterMag * sinf(-flutter);
	theNode->Coord.y += flutterMag * cosf(-flutter);
	theNode->Rot.y += 0.1f * sinf(-flutter);

	flutter += gFramesPerSecondFrac * flutterSpeed;
//		flutterMag = sinf(flutter * 2);


	float bounce = theNode->SpecialF[0];
	if (bounce > 0)
	{
		const float bounceDuration = 0.25f;
		float bounceMag = bounce * 16;
		theNode->Coord.x -= bounceMag * SDL_fabsf(SDL_sinf(PI * bounce));
		bounce -= gFramesPerSecondFrac * (1.0f / bounceDuration);
		theNode->SpecialF[0] = bounce;
	}


	UpdateObjectTransforms(theNode);
}

static void BounceCursorDot(void)
{
	if (!gMenuCursorDot)
	{
		return;
	}

	gMenuCursorDot->SpecialF[0] = 1;
	gMenuCursorDot->Flag[1] = true;
}

static ObjNode* MakeMenuCursorDot(void)
{
	NewObjectDefinitionType def =
	{
		.genre		= SPRITE_GENRE,
		.group		= SPRITE_GROUP_GLOBAL,
		.type		= GLOBAL_SObjType_LeafCursor,
		.scale		= 20,
		.slot		= MENU_SLOT,
		.moveCall	= MoveCursorDot,
		.flags		= STATUS_BIT_MOVEINPAUSE,
	};
	ObjNode* cursorDot = MakeSpriteObject(&def);
	cursorDot->Flag[0] = false;

	return cursorDot;
}

/****************************/
/*    PAGE LAYOUT           */
/****************************/
#pragma mark - Page Layout

#if 0
static ObjNode* MakeMenuDarkenPane(void)
{
	ObjNode* pane;
	
	NewObjectDefinitionType def =
	{
		.genre = CUSTOM_GENRE,
		.flags = STATUS_BIT_NOZWRITES | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOFOG | STATUS_BIT_NOTEXTUREWRAP |
						STATUS_BIT_KEEPBACKFACES | STATUS_BIT_MOVEINPAUSE,
		.slot = MENU_SLOT - 1,
		.scale = 1;
		.moveCall = MoveDarkenPane,
		.drawCall = DrawDarkenPane,
	};

	pane = MakeNewObject(&def);
	pane->ColorFilter = (OGLColorRGBA) {0, 0, 0, 0};
	pane->Scale.y = gMenuStyle->darkenPaneScaleY;

	return pane;
}
#endif

static void DeleteAllText(void)
{
	for (int row = 0; row < MAX_MENU_ROWS; row++)
	{
		for (int col = 0; col < MAX_MENU_COLS; col++)
		{
			if (gMenuObjects[row][col])
			{
				DeleteObject(gMenuObjects[row][col]);
				gMenuObjects[row][col] = nil;
			}
		}
	}
}

static ObjNode* MakeTextAtRowCol(const char* text, int row, int col)
{
	ObjNode* node = gMenuObjects[row][col];

	if (node)
	{
		// Recycling existing text lets us keep the move call, color and
		int alignment = gMenuStyle->centeredText? kTextMeshAlignCenter: kTextMeshAlignLeft;
		TextMesh_Update(text, alignment, node);
	}
	else
	{
		float startX = gMenuStyle->centeredText ? 0 : -170;
		startX += gMenuStyle->offset.x;
		gNewObjectDefinition.coord = (OGLPoint3D) { startX + gMenuColXs[col], gMenuRowYs[row], 0 };
		int alignment = gMenuStyle->centeredText? kTextMeshAlignCenter: kTextMeshAlignLeft;
		node = TextMesh_New(text, alignment, &gNewObjectDefinition);
		node->SpecialRow = row;
		node->SpecialCol = col;
		node->StatusBits |= STATUS_BIT_MOVEINPAUSE;
		gMenuObjects[row][col] = node;
	}

	if (!gMenuStyle->centeredText)
	{
		int paddedRightOff = ((gMenuColXs[col+1]-170+gMenuStyle->offset.x) - node->Coord.x) / node->Scale.x;
		if (paddedRightOff > node->RightOff)
			node->RightOff = paddedRightOff;
	}

	if (gMenuStyle->uniformXExtent)
	{
		if (-gMenuStyle->uniformXExtent < node->LeftOff)
			node->LeftOff = -gMenuStyle->uniformXExtent;
		if (gMenuStyle->uniformXExtent > node->RightOff)
			node->RightOff = gMenuStyle->uniformXExtent;
	}

	return node;
}

static const float kMenuItemHeightMultipliers[kMI_COUNT] =
{
	[kMISENTINEL]     = 0.0f,
	[kMITitle]        = 1.4f,
	[kMISubtitle]     = 0.8f,
	[kMILabel]        = 1,
	[kMISpacer]       = 0.5f,
	[kMICycler]       = 1,
	[kMIPick]         = 1,
	[kMIKeyBinding]   = 1,
	[kMIPadBinding]   = 1,
	[kMIMouseBinding] = 1,
};

static const char* GetMenuItemLabel(const MenuItem* entry)
{
	if (entry->rawText)
		return entry->rawText;
	else if (entry->generateText)
		return entry->generateText();
	else
		return Localize(entry->text);
}

static ObjNode* LayOutCyclerValueText(int row)
{
	static char buf[64];
	const MenuItem* entry = &gMenu[row];

	int numChoices = entry->cycler.numChoices;
	if (entry->cycler.generateNumChoices)
		numChoices = entry->cycler.generateNumChoices();

	if (numChoices <= 0)
		return NULL;

	Byte value = *entry->cycler.valuePtr;
	const char* valueText = NULL;

	if (entry->cycler.generateChoiceString)
	{
		valueText = entry->cycler.generateChoiceString(buf, sizeof(buf), value);
	}
	else
	{
		GAME_ASSERT(numChoices <= MAX_MENU_CYCLER_CHOICES);
		valueText = Localize(entry->cycler.choices[value]);
	}

	ObjNode* node2 = MakeTextAtRowCol(valueText, row, 1);
	node2->MoveCall = MoveAction;
	return node2;
}

static void LayOutCycler(int row, float sweepFactor)
{
	static char buf[64];

	const MenuItem* entry = &gMenu[row];

	SDL_snprintf(buf, sizeof(buf), "%s:", GetMenuItemLabel(entry));

	ObjNode* node1 = MakeTextAtRowCol(buf, row, 0);
	node1->MoveCall = MoveAction;
	node1->SpecialSweepTimer = sweepFactor;

	ObjNode* node2 = LayOutCyclerValueText(row);
	node2->SpecialSweepTimer = sweepFactor;
}

static void LayOutMenu(int menuID)
{
	const MenuItem* menuStartSentinel = LookUpMenu(menuID);

	GAME_ASSERT(menuStartSentinel);
	GAME_ASSERT(menuStartSentinel->id == menuID);
	GAME_ASSERT(menuStartSentinel->type == kMISENTINEL);

	static char buf[64];

	// Get first item in menu
	const MenuItem* menu = menuStartSentinel+1;

	// Remember we've been in this menu
	gMenuHistory[gMenuHistoryPos].menuID = menuID;

//	bool enteringNewMenu = menu != gMenu;
//
//	if (gMenu == gRootMenu)				// save position in root menu
//		gLastRowOnRootMenu = gMenuRow;

	gMenu			= menu;
	gNumMenuEntries	= 0;
	gMenuPick		= -1;

//	if (enteringNewMenu)
//	{
//		gMenuRow		= -1;
//
//		if (menu == gRootMenu && gLastRowOnRootMenu >= 0)				// restore position in root menu
//			gMenuRow = gLastRowOnRootMenu;
//	}

	DeleteAllText();

	SDL_zero(gNewObjectDefinition);
	gNewObjectDefinition.group		= ATLAS_GROUP_FONT1;
	gNewObjectDefinition.scale		= gMenuStyle->standardScale;
	gNewObjectDefinition.slot		= MENU_SLOT;

	float totalHeight = 0;
	for (int row = 0; menu[row].type != kMISENTINEL; row++)
	{
		totalHeight += kMenuItemHeightMultipliers[menu[row].type] * gMenuStyle->rowHeight;
	}

	float y = -totalHeight/2.0f;
	y += gMenuStyle->offset.y;

	float sweepFactor = 0.0f;

	for (int row = 0; menu[row].type != kMISENTINEL; row++)
	{
		gMenuRowYs[row] = y;

		const MenuItem* entry = &menu[row];

		gNewObjectDefinition.scale = gMenuStyle->standardScale;

		switch (entry->type)
		{
			case kMISpacer:
				break;

			case kMITitle:
			{
				gNewObjectDefinition.scale = gMenuStyle->titleScale;
				ObjNode* label = MakeTextAtRowCol(GetMenuItemLabel(entry), row, 0);
				label->ColorFilter = gMenuStyle->titleColor;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = .5f;		// Title appears sooner than the rest
				break;
			}

			case kMISubtitle:
			{
				gNewObjectDefinition.scale = gMenuStyle->subtitleScale;
				ObjNode* label = MakeTextAtRowCol(GetMenuItemLabel(entry), row, 0);
				label->ColorFilter = gMenuStyle->titleColor;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = .5f;		// Title appears sooner than the rest
				break;
			}

			case kMILabel:
			{
				ObjNode* label = MakeTextAtRowCol(GetMenuItemLabel(entry), row, 0);
				label->ColorFilter = gMenuStyle->inactiveColor;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = sweepFactor;
				break;
			}

			case kMIPick:
			{
				ObjNode* node = MakeTextAtRowCol(GetMenuItemLabel(entry), row, 0);
				node->MoveCall = MoveAction;
				node->SpecialSweepTimer = sweepFactor;
				break;
			}

			case kMICycler:
			{
				LayOutCycler(row, sweepFactor);
				break;
			}

			case kMIKeyBinding:
			{
				SDL_snprintf(buf, sizeof(buf), "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->kb));

//				gNewObjectDefinition.scale = GS*0.6f;
				ObjNode* label = MakeTextAtRowCol(buf, row, 0);
				label->ColorFilter = gMenuStyle->inactiveColor2;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = sweepFactor;

				for (int j = 0; j < MAX_USER_BINDINGS_PER_NEED; j++)
				{
					ObjNode* keyNode = MakeTextAtRowCol(GetKeyBindingName(row, j), row, j + 1);
					keyNode->MoveCall = MoveKeyBinding;
					keyNode->SpecialSweepTimer = sweepFactor;
				}
				break;
			}

			case kMIPadBinding:
			{
				SDL_snprintf(buf, sizeof(buf), "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->kb));

				ObjNode* label = MakeTextAtRowCol(buf, row, 0);
				label->ColorFilter = gMenuStyle->inactiveColor2;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = sweepFactor;

				for (int j = 0; j < MAX_USER_BINDINGS_PER_NEED; j++)
				{
					ObjNode* keyNode = MakeTextAtRowCol(GetPadBindingName(row, j), row, j + 1);
					keyNode->MoveCall = MovePadBinding;
					keyNode->SpecialSweepTimer = sweepFactor;
				}
				break;
			}

			case kMIMouseBinding:
			{
				SDL_snprintf(buf, sizeof(buf), "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->kb));

				ObjNode* label = MakeTextAtRowCol(buf, row, 0);
				label->ColorFilter = gMenuStyle->inactiveColor2;
				label->MoveCall = MoveAction;
				label->SpecialSweepTimer = sweepFactor;

				ObjNode* keyNode = MakeTextAtRowCol(GetMouseBindingName(row), row, 1);
				keyNode->MoveCall = MoveMouseBinding;
				keyNode->SpecialSweepTimer = sweepFactor;
				break;
			}

			default:
				DoFatalAlert("Unsupported menu item type");
				break;
		}

		y += kMenuItemHeightMultipliers[entry->type] * gMenuStyle->rowHeight;

//		if (entry->type != kMISpacer)
//			sweepFactor -= .2f;

		gNumMenuEntries++;
		GAME_ASSERT(gNumMenuEntries < MAX_MENU_ROWS);
	}

	// Restore old focus row from history
	gMenuRow = gMenuHistory[gMenuHistoryPos].row;

	// If there was no valid focus row in history, fall back to first interactable row
	if (gMenuRow <= 0)
	{
		// Scroll down to first pickable entry
		gMenuRow = -1;
		NavigateSettingEntriesVertically(1);
	}

	// Now that the contents of the menu have been laid out,
	// call start sentinel's callback, if any
	if (menuStartSentinel->callback)
	{
		menuStartSentinel->callback();
	}
}

void LayoutCurrentMenuAgain(void)
{
	int currentMenu = GetCurrentMenuID();
	GAME_ASSERT(currentMenu != 0);

	SaveSelectedRowInHistory();
	LayOutMenu(currentMenu);
}

#pragma mark - Menu registry

void RegisterMenu(const MenuItem* menuTree)
{
	for (const MenuItem* menuItem = menuTree; menuItem->type || menuItem->id; menuItem++)
	{
		if (menuItem->type == 0)
		{
			if (menuItem->id == 0)			// end sentinel
			{
				break;
			}

			if (LookUpMenu(menuItem->id))	// already registered
			{
				continue;
			}

			GAME_ASSERT(gNumMenusRegistered < MAX_REGISTERED_MENUS);
			gMenuRegistry[gNumMenusRegistered] = menuItem;
			gNumMenusRegistered++;

//			printf("Registered menu '%s'\n", FourccToString(menuItem->id));
		}
	}
}

static const MenuItem* LookUpMenu(int menuID)
{
	for (int i = 0; i < gNumMenusRegistered; i++)
	{
		if (gMenuRegistry[i]->id == menuID)
			return gMenuRegistry[i];
	}

	return NULL;
}

#pragma mark - Start

int StartMenu(
		const MenuItem* menu,
		const MenuStyle* style,
		void (*moveCall)(void),
		void (*drawCall)(void))
{
	int cursorStateBeforeMenu = SDL_ShowCursor(SDL_QUERY);
	SetSystemCursor(SDL_SYSTEM_CURSOR_ARROW);

		/* INITIALIZE MENU STATE */

	SDL_zeroa(gMenuObjects);
	gMenuStyle			= style? style: &kDefaultMenuStyle;
	gMenuState			= kMenuStateFadeIn;
	gMenuFadeAlpha		= 0;
	gMenuRow			= -1;

	gNumMenusRegistered = 0;
	RegisterMenu(menu);

	gMenuHistoryPos		= 0;
	SDL_zeroa(gMenuHistory);

		/* LAY OUT MENU COMPONENTS */

	ObjNode* pane = nil;

	gMenuCursorDot = MakeMenuCursorDot();

	if (gMenuStyle->darkenPaneOpacity > 0)
	{
		pane = MakeDarkenPane();
		pane->MoveCall = MoveDarkenPane;
		pane->StatusBits |= STATUS_BIT_MOVEINPAUSE;
	}

	LayOutMenu(menu[0].id);

		/* SHOW IN ANIMATED LOOP */

	while (gMenuState != kMenuStateOff)
	{
		UpdateInput();

		if (gMenuStyle->startButtonExits && IsNeedDown(kNeed_UIStart))
			gMenuState = kMenuStateFadeOut;

		switch (gMenuState)
		{
			case kMenuStateFadeIn:
				gMenuFadeAlpha += gFramesPerSecondFrac * gMenuStyle->fadeInSpeed;
				if (gMenuFadeAlpha >= 1.0f)
				{
					gMenuFadeAlpha = 1.0f;
					gMenuState = kMenuStateReady;
				}
				break;

			case kMenuStateFadeOut:
				if (gMenuStyle->asyncFadeOut)
				{
					gMenuState = kMenuStateOff;		// exit loop
				}
				else
				{
					gMenuFadeAlpha -= gFramesPerSecondFrac * gMenuStyle->fadeOutSpeed;
					if (gMenuFadeAlpha <= 0.0f)
					{
						gMenuFadeAlpha = 0.0f;
						gMenuState = kMenuStateOff;
					}
				}
				break;

			case kMenuStateReady:
				if (gMenuStyle->isInteractive)
				{
					NavigateMenu();
				}
				else if (UserWantsOut())
				{
					PlayMenuChangeEffect();
					GoBackInHistory();
				}
				break;

			case kMenuStateAwaitingKeyPress:
				AwaitKeyPress();
				break;

			case kMenuStateAwaitingPadPress:
				AwaitPadPress();
				break;

			case kMenuStateAwaitingMouseClick:
				AwaitMouseClick();
				break;

			default:
				break;
		}

			/* DRAW STUFF */

		CalcFramesPerSecond();
		MoveObjects();
		if (moveCall)
			moveCall();
		OGL_DrawScene(drawCall);
	}


		/* CLEANUP */

	if (gMenuCursorDot)
	{
		DeleteObject(gMenuCursorDot);
		gMenuCursorDot = NULL;
	}

	if (gMenuStyle->asyncFadeOut)
	{
		gMenuAsyncFadeOutSpeed = gMenuStyle->fadeOutSpeed;

		if (pane)
		{
			pane->MoveCall = MoveAsyncFadeOutAndDelete;
		}

		for (int row = 0; row < MAX_MENU_ROWS; row++)
		{
			for (int col = 0; col < MAX_MENU_COLS; col++)
			{
				if (gMenuObjects[row][col])
					gMenuObjects[row][col]->MoveCall = MoveAsyncFadeOutAndDelete;
			}
		}

		SDL_memset(gMenuObjects, 0, sizeof(gMenuObjects));
	}
	else
	{
		DeleteAllText();

		if (pane)
		{
			DeleteObject(pane);
			pane = NULL;
		}
	}

	UpdateInput();
	MyFlushEvents();

	gMenu = nil;

	SetSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	SDL_ShowCursor(cursorStateBeforeMenu);

	return gMenuPick;
}

int GetCurrentMenuID(void)
{
	if (gMenu == NULL)
		return 0;

	GAME_ASSERT(gMenu[-1].type == kMISENTINEL);
	GAME_ASSERT(gMenu[-1].id != 0);
	return gMenu[-1].id;
}
