#pragma once

#undef MB_RIGHT // Window compatibility fix.

namespace Gui
{
	typedef bool GuiResult;

	enum MouseButton
	{
		MouseButton_Left,
		MouseButton_Right,
		MouseButton_Middle
	};

	enum ButtonState
	{
		ButtonState_Up   = 0, // NOTE: 0 = Default during DeSerialize.
		ButtonState_Down = 1
	};

	enum Alignment
	{
		Alignment_None   = 0, // NOTE: 0 = Default during DeSerialize.
		Alignment_Left   = 1,
		Alignment_Right  = 2,
		Alignment_Top    = 3,
		Alignment_Bottom = 4,
		Alignment_Center = 5,
		Alignment_Client = 10
	};

	enum Anchor
	{
		Anchor_Left   = 0x01,
		Anchor_Right  = 0x02,
		Anchor_Top    = 0x04,
		Anchor_Bottom = 0x08,
		Anchor_All    = 0xFF
	};

	enum STDCOLOR
	{
		COLOR_USER    = 0,
		COLOR_DEFAULT = 1,
		COLOR_AQUA    = 0x1000,
		COLOR_BLACK,
		COLOR_BLUE,
		COLOR_DARKGRAY,
		COLOR_FUCHSIA,
		COLOR_GRAY,
		COLOR_GREEN,
		COLOR_LIME,
		COLOR_LIGHTGRAY,
		COLOR_MAROON,
		COLOR_NAVY,
		COLOR_OLIVE,
		COLOR_PURPLE,
		COLOR_RED,
		COLOR_SILVER,
		COLOR_TEAL,
		COLOR_WHITE,
		COLOR_YELLOW
	};
};
