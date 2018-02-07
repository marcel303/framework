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

#include <stdint.h>

struct IMU9250
{
	enum Message
	{
		kMessage_Save = 0,
		kMessage_SetCalibrationMode = 1,
		kMessage_SetReturnContent = 2,
		kMessage_SetReturnRate = 3,
		kMessage_SetBaudRate = 4,
		kMessage_SetAccelOffsetX = 5,
		kMessage_SetAccelOffsetY = 6,
		kMessage_SetAccelOffsetZ = 7,
		kMessage_SetGyroOffsetX = 8,
		kMessage_SetGyroOffsetY = 9,
		kMessage_SetGyroOffsetZ = 10,
		kMessage_SetMagnetOffsetX = 11,
		kMessage_SetMagnetOffsetY = 12,
		kMessage_SetMagnetOffsetZ = 13,
		kMessage_SetLedStatus = 29
	};

	enum CalibrationMode
	{
		kCalibrationMode_Off = 0,
		kCalibrationMode_GyroAndAccel = 1,
		kCalibrationMode_Magnet = 2,
		kCalibrationMode_CalibrateHeight = 3
	};

	enum ReturnContent
	{
		kReturnContent_Time = 1 << 0,
		kReturnContent_Acceleration = 1 << 1,
		kReturnContent_AngularVelocity = 1 << 2,
		kReturnContent_Angle = 1 << 3,
		kReturnContent_Magnet = 1 << 4,
		kReturnContent_DataPortStatus = 1 << 5,
		kReturnContent_AtmosphericPressureAndHeight = 1 << 6,
		kReturnContent_LongitudeAndAltitude = 1 << 7,
		kReturnContent_GroundSpeed = 1 << 8,
		kReturnContent_Quaternion = 1 << 9
	};

	enum ReturnRate
	{
		kReturnRate_0_1 = 1,
		kReturnRate_0_5 = 2,
		kReturnRate_1 = 3,
		kReturnRate_2 = 4,
		kReturnRate_5 = 5,
		kReturnRate_10 = 6,
		kReturnRate_20 = 7,
		kReturnRate_50 = 8,
		kReturnRate_100 = 9,
		kReturnRate_200 = 10,
		kReturnRate_Single = 11,
		kReturnRate_None = 12
	};

	enum BaudRate
	{
		kBaudRate_2400 = 0,
		kBaudRate_4800 = 1,
		kBaudRate_9600 = 2,
		kBaudRate_19200 = 3,
		kBaudRate_38400 = 4,
		kBaudRate_57600 = 5,
		kBaudRate_115200 = 6,
		kBaudRate_230400 = 7,
		kBaudRate_460800 = 8,
		kBaudRate_921600 = 9
	};

	enum LedStatus
	{
		kLedStatus_Off = 1,
		kLedState_On = 0
	};

	// sends

	virtual void send(const uint8_t * bytes, const int numBytes) = 0;

	void sendMessage(const Message message, const uint16_t value);

	void saveConfig();
	void setReturnRate(const ReturnRate rate);
	void setBaudRate(const BaudRate rate);
	void setReturnContent(const uint16_t mask);
	void setAccelOffsetX(const int16_t bias);
	void setAccelOffsetY(const int16_t bias);
	void setAccelOffsetZ(const int16_t bias);
	void setAccelOffsets(const int16_t biasX, const int16_t biasY, const int16_t biasZ);
	void setGyroOffsetX(const int16_t bias);
	void setGyroOffsetY(const int16_t bias);
	void setGyroOffsetZ(const int16_t bias);
	void setGyroOffsets(const int16_t biasX, const int16_t biasY, const int16_t biasZ);
	void setMagnetOffsetX(const int16_t bias);
	void setMagnetOffsetY(const int16_t bias);
	void setMagnetOffsetZ(const int16_t bias);
	void setMagnetOffsets(const int16_t biasX, const int16_t biasY, const int16_t biasZ);
	void setLedStatus(const LedStatus status);
	void setCalibrationMode(const CalibrationMode mode);

	// receives

	enum ReturnType
	{
		kReturnType_Invalid = 0xff,
		
		kReturnType_Time = 0x50,
		kReturnType_Acceleration = 0x51,
		kReturnType_AngularVelocity = 0x52,
		kReturnType_Angle = 0x53,
		kReturnType_Magnet = 0x54,
		kReturnType_DataPortStatus = 0x55,
		kReturnType_AtmosphericPressureAndHeight = 0x56,
		kReturnType_LongitudeAndLatitude = 0x57,
		kReturnType_GroundSpeed = 0x58,
		kReturnType_Quaternion = 0x59
	};

	struct ReturnMessageReader
	{
		unsigned char readBuffer[11];
		unsigned char counter;
		
		short acceleration[3];
		float accelerationf[3]; // m/s
		
		short angularVelocity[3];
		float angularVelocityf[3]; // degrees/s
		
		short angle[3];
		float anglef[3];
		
		short magnet[3];
		float magnetf[3]; // milliGauss
		
		short quaternion[4];
		float quaternionf[4];
		
		float temperature; // celcius

		int longitude;
		int latitude;
		
		int pressure; // Pascal
		int height; // centimeters
		
		ReturnMessageReader();
		
		ReturnType handleByte(const uint8_t c);
		ReturnType decodeMessage();
	};
};
