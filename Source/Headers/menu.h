#pragma once

#define MAX_MENU_CYCLER_CHOICES		8

typedef enum
{
	kMISENTINEL,
	kMIPick,
	kMILabel,
	kMISpacer,
	kMICycler,
	kMITitle,
	kMISubtitle,
	kMIKeyBinding,
	kMIPadBinding,
	kMIMouseBinding,
	kMI_COUNT,
} MenuItemType;

typedef struct MenuItem
{
	MenuItemType			type;
	LocStrID				text;
	const char*				rawText;
	int32_t					id;			// value stored in gMenuOutcome when exiting menu
	int32_t					next;		// next menu ID, or one of 'EXIT', 'BACK' or 0 (no-op)
	const char*				(*generateText)(void);
	void					(*callback)(void);

	union
	{
		struct
		{
			Byte*			valuePtr;
			bool			callbackSetsValue;

			uint8_t			numChoices;
			LocStrID		choices[MAX_MENU_CYCLER_CHOICES];	// localizable strings

			uint8_t			(*generateNumChoices)(void);
			const char*		(*generateChoiceString)(char* buf, int bufSize, Byte value);
		} cycler;

		int 				kb;
	};
} MenuItem;

typedef struct MenuStyle
{
	float			fadeInSpeed;
	float			fadeOutSpeed;
	bool			asyncFadeOut;
	bool			centeredText;
	OGLColorRGBA	titleColor;
	OGLColorRGBA	inactiveColor;
	OGLColorRGBA	inactiveColor2;
	float			standardScale;
	float			titleScale;
	float			subtitleScale;
	float			rowHeight;
	float			uniformXExtent;
	bool			playMenuChangeSounds;
	float			darkenPaneScaleY;
	float			darkenPaneOpacity;
	bool			startButtonExits;
	bool			isInteractive;
	bool			canBackOutOfRootMenu;
	OGLVector2D		offset;
} MenuStyle;

extern const MenuStyle kDefaultMenuStyle;

int StartMenu(
		const MenuItem* menu,
		const MenuStyle* style,
		void (*moveCall)(void),
		void (*drawCall)(void));
void LayoutCurrentMenuAgain(void);
int GetCurrentMenuID(void);
