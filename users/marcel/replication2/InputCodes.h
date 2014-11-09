#ifndef INPUTCODES_H
#define INPUTCODES_H
#pragma once

// Event specific codes.
enum INPUT_AXIS
{
	INPUT_AXIS_X = 0,
	INPUT_AXIS_Y = 1,
	INPUT_AXIS_Z = 2
};

enum INPUT_BUTTON
{
	INPUT_BUTTON1 = 0,
	INPUT_BUTTON2 = 1,
	INPUT_BUTTON3 = 2
};

enum INPUT_BUTTON_STATE
{
	BUTTON_UP = 0,
	BUTTON_DOWN = 1,
};

enum INPUT_KEY
{
	// a-z
	IK_a = 'a',
	IK_b = 'b',
	IK_c = 'c',
	IK_d = 'd',
	IK_e = 'e',
	IK_f = 'f',
	IK_g = 'g',
	IK_h = 'h',
	IK_i = 'i',
	IK_j = 'j',
	IK_k = 'k',
	IK_l = 'l',
	IK_m = 'm',
	IK_n = 'n',
	IK_o = 'o',
	IK_p = 'p',
	IK_q = 'q',
	IK_r = 'r',
	IK_s = 's',
	IK_t = 't',
	IK_u = 'u',
	IK_v = 'v',
	IK_w = 'w',
	IK_x = 'x',
	IK_y = 'y',
	IK_z = 'z',
	// numeric keys
	IK_0 = '0',
	IK_1 = '1',
	IK_2 = '2',
	IK_3 = '3',
	IK_4 = '4',
	IK_5 = '5',
	IK_6 = '6',
	IK_7 = '7',
	IK_8 = '8',
	IK_9 = '9',
	// numeric keys (keypad)
	IK_KP_0 = 1000,
	IK_KP_1,
	IK_KP_2,
	IK_KP_3,
	IK_KP_4,
	IK_KP_5,
	IK_KP_6,
	IK_KP_7,
	IK_KP_8,
	IK_KP_9,
	// function keys
	IK_F1 = 2000,
	IK_F2,
	IK_F3,
	IK_F4,
	IK_F5,
	IK_F6,
	IK_F7,
	IK_F8,
	IK_F9,
	IK_F10,
	IK_F11,
	IK_F12,
	// symbols
	IK_PLUS = '+',
	IK_MINUS = '-',
	IK_ASTERISK = '*',
	IK_SPACE = ' ',
	// special keys
	IK_ESCAPE = 3000,
	IK_ENTER,
	IK_DELETE,
	IK_BACKSPACE,
	IK_SHIFTL,
	IK_SHIFTR,
	IK_ALTL,
	IK_ALTR,
	IK_CONTROLL,
	IK_CONTROLR,
	IK_HOME,
	IK_END,
	IK_PAGEUP,
	IK_PAGEDOWN,
	IK_UP,
	IK_DOWN,
	IK_LEFT,
	IK_RIGHT
};

#endif
