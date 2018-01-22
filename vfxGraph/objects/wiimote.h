/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#if ENABLE_WIIMOTE

#include <stdint.h>

enum WiimoteButton
{
	kWiimoteButton_Two         = 0x0001,
	kWiimoteButton_One         = 0x0002,
	kWiimoteButton_B           = 0x0004,
	kWiimoteButton_A           = 0x0008,
	kWiimoteButton_Minus       = 0x0010,
	kWiimoteButton_ZACCEL_BIT6 = 0x0020,
	kWiimoteButton_ZACCEL_BIT7 = 0x0040,
	kWiimoteButton_Home        = 0x0080,
	kWiimoteButton_Left        = 0x0100,
	kWiimoteButton_Right       = 0x0200,
	kWiimoteButton_Down        = 0x0400,
	kWiimoteButton_Up          = 0x0800,
	kWiimoteButton_Plus        = 0x1000,
	kWiimoteButton_ZACCEL_BIT4 = 0x2000,
	kWiimoteButton_ZACCEL_BIT5 = 0x4000
};

struct WiimoteData
{
	struct Motion
	{
		uint8_t forces[3];
	};

	struct MotionPlus
	{
		uint16_t yaw;
		uint16_t pitch;
		uint16_t roll;
	};

	WiimoteData();
	
	bool connected;
	
	uint16_t buttons;

	Motion motion;
	MotionPlus motionPlus;
	
	uint8_t ledValues;

	void decodeButtons(const uint8_t * data);
	void decodeMotion(const uint8_t * data);
	void decodeMotionPlus(const uint8_t * data);
};

struct Wiimotes
{
	void * discoveryObject; // on MacOS we use an Objective-C object which does the discovery for us
	void * connectionObject;

	WiimoteData wiimoteData;

	Wiimotes();
	~Wiimotes();
	
	void findAndConnect();
	void shut();
	
	void process();
	
	bool getLedEnabled(const uint8_t index) const;
	void setLedValues(const uint8_t values);
	void setLedEnabled(const uint8_t index, const bool enabled);
};

#endif
