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

#include "imu9250.h"
#include "Log.h"
#include <string.h>

//

#define LOG_READ(...) do { } while (false)
//#define LOG_READ(...) LOG_DBG(__VA_ARGS__)

//

void IMU9250::sendMessage(const Message message, const uint16_t value)
{
	uint8_t msg[5] =
	{
		0xff, 0xaa, uint8_t(message), uint8_t(value & 0xff), uint8_t((value >> 8) & 0xff)
	};

	send(msg, 5);
}

void IMU9250::saveConfig()
{
	sendMessage(kMessage_Save, 0);
}

void IMU9250::setReturnRate(const ReturnRate rate)
{
	sendMessage(kMessage_SetReturnRate, rate);
}

void IMU9250::setBaudRate(const BaudRate rate)
{
	sendMessage(kMessage_SetBaudRate, rate);
}

void IMU9250::setReturnContent(const uint16_t mask)
{
	sendMessage(kMessage_SetReturnContent, mask);
}

void IMU9250::setAccelOffsetX(const int16_t bias)
{
	sendMessage(kMessage_SetAccelOffsetX, bias);
}

void IMU9250::setAccelOffsetY(const int16_t bias)
{
	sendMessage(kMessage_SetAccelOffsetY, bias);
}

void IMU9250::setAccelOffsetZ(const int16_t bias)
{
	sendMessage(kMessage_SetAccelOffsetZ, bias);
}

void IMU9250::setAccelOffsets(const int16_t biasX, const int16_t biasY, const int16_t biasZ)
{
	setAccelOffsetX(biasX);
	setAccelOffsetY(biasY);
	setAccelOffsetZ(biasZ);
}

void IMU9250::setGyroOffsetX(const int16_t bias)
{
	sendMessage(kMessage_SetGyroOffsetX, bias);
}

void IMU9250::setGyroOffsetY(const int16_t bias)
{
	sendMessage(kMessage_SetGyroOffsetY, bias);
}

void IMU9250::setGyroOffsetZ(const int16_t bias)
{
	sendMessage(kMessage_SetGyroOffsetZ, bias);
}

void IMU9250::setGyroOffsets(const int16_t biasX, const int16_t biasY, const int16_t biasZ)
{
	setGyroOffsetX(biasX);
	setGyroOffsetY(biasY);
	setGyroOffsetZ(biasZ);
}

void IMU9250::setMagnetOffsetX(const int16_t bias)
{
	sendMessage(kMessage_SetMagnetOffsetX, bias);
}

void IMU9250::setMagnetOffsetY(const int16_t bias)
{
	sendMessage(kMessage_SetMagnetOffsetY, bias);
}

void IMU9250::setMagnetOffsetZ(const int16_t bias)
{
	sendMessage(kMessage_SetMagnetOffsetZ, bias);
}

void IMU9250::setMagnetOffsets(const int16_t biasX, const int16_t biasY, const int16_t biasZ)
{
	setMagnetOffsetX(biasX);
	setMagnetOffsetY(biasY);
	setMagnetOffsetZ(biasZ);
}

void IMU9250::setLedStatus(const LedStatus status)
{
	sendMessage(kMessage_SetLedStatus, status);
}

void IMU9250::setCalibrationMode(const CalibrationMode mode)
{
	sendMessage(kMessage_SetCalibrationMode, mode);
}

//

IMU9250::ReturnMessageReader::ReturnMessageReader()
{
	memset(this, 0, sizeof(*this));
}

IMU9250::ReturnType IMU9250::ReturnMessageReader::handleByte(const uint8_t c)
{
	readBuffer[counter] = c;

	// wait for the message to start with 0x55

	if (counter == 0 && readBuffer[0] != 0x55)
		return kReturnType_Invalid;

	counter++;

	// wait to receive a complete 11-byte message

	if (counter == 11)
	{
		counter = 0;
		
		// decode the received message

		return decodeMessage();
	}
	else
	{
		return kReturnType_Invalid;
	}
}

IMU9250::ReturnType IMU9250::ReturnMessageReader::decodeMessage()
{
	if (readBuffer[0] == 0x55)
	{
		const ReturnType type = (ReturnType)readBuffer[1];
		
		switch (type)
		{
		case kReturnType_Time:
			break;
			
		case kReturnType_Acceleration:
			acceleration[0] = (short(readBuffer[3] << 8 | readBuffer[2]));
			acceleration[1] = (short(readBuffer[5] << 8 | readBuffer[4]));
			acceleration[2] = (short(readBuffer[7] << 8 | readBuffer[6]));
			
			accelerationf[0] = acceleration[0] / 32768.0 * 16;
			accelerationf[1] = acceleration[1] / 32768.0 * 16;
			accelerationf[2] = acceleration[2] / 32768.0 * 16;
			temperature = (short(readBuffer[9] << 8 | readBuffer[8])) / 100.0;
			LOG_READ("acceleration: %2.f, %.2f, %.2f", accelerationf[0], accelerationf[1], accelerationf[2]);
			break;
			
		case kReturnType_AngularVelocity:
			angularVelocity[0] = (short(readBuffer[3] << 8 | readBuffer[2]));
			angularVelocity[1] = (short(readBuffer[5] << 8 | readBuffer[4]));
			angularVelocity[2] = (short(readBuffer[7] << 8 | readBuffer[6]));
			angularVelocityf[0] = angularVelocity[0] / 32768.0 * 2000;
			angularVelocityf[1] = angularVelocity[1] / 32768.0 * 2000;
			angularVelocityf[2] = angularVelocity[2] / 32768.0 * 2000;
			temperature = (short(readBuffer[9] << 8 | readBuffer[8])) / 100.0;
			LOG_READ("angular velocity: %.2f, %.2f, %.2f", angularVelocityf[0], angularVelocityf[1], angularVelocityf[2]);
			break;
			
		case kReturnType_Angle:
			angle[0] = (short(readBuffer[3] << 8 | readBuffer[2]));
			angle[1] = (short(readBuffer[5] << 8 | readBuffer[4]));
			angle[2] = (short(readBuffer[7] << 8 | readBuffer[6]));
			anglef[0] = angle[0] / 32768.0 * 180;
			anglef[1] = angle[1] / 32768.0 * 180;
			anglef[2] = angle[2] / 32768.0 * 180;
			temperature = (short(readBuffer[9] << 8 | readBuffer[8])) / 100.0;
			LOG_READ("angle: %.2f, %.2f, %.2f", anglef[0], anglef[1], anglef[2]);
			break;
			
		case kReturnType_Magnet:
			magnet[0] = (short(readBuffer[3] << 8 | readBuffer[2]));
			magnet[1] = (short(readBuffer[5] << 8 | readBuffer[4]));
			magnet[2] = (short(readBuffer[7] << 8 | readBuffer[6]));
			magnetf[0] = magnet[0] / 32768.0 * 4912;
			magnetf[1] = magnet[1] / 32768.0 * 4912;
			magnetf[2] = magnet[2] / 32768.0 * 4912;
			temperature = (short(readBuffer[9] << 8 | readBuffer[8])) / 100.0;
			LOG_READ("magnet: %d, %d, %d", magnet[0], magnet[1], magnet[2]);
			break;
			
		case kReturnType_AtmosphericPressureAndHeight:
			pressure =
				(readBuffer[5] << 24) |
				(readBuffer[4] << 16) |
				(readBuffer[3] << 8) |
				readBuffer[2];
			height =
				(readBuffer[9] << 24) |
				(readBuffer[8] << 16) |
				(readBuffer[7] << 8) |
				readBuffer[6];
			LOG_READ("pressure: %d (Pa), height: %d (cm)", pressure, height);
			break;
			
		case kReturnType_LongitudeAndLatitude:
			longitude = (readBuffer[5]<<24) | (readBuffer[4]<<16) | (readBuffer[3]<<8) | readBuffer[2];
			latitude  = (readBuffer[9]<<24) | (readBuffer[8]<<16) | (readBuffer[7]<<8) | readBuffer[6];
			LOG_READ("longitude: %d, latitude: %d", longitude, latitude);
			break;
			
		case kReturnType_Quaternion:
			quaternion[0] = ((readBuffer[3] << 8) | readBuffer[2]);
			quaternion[1] = ((readBuffer[5] << 8) | readBuffer[4]);
			quaternion[2] = ((readBuffer[7] << 8) | readBuffer[6]);
			quaternion[3] = ((readBuffer[9] << 8) | readBuffer[8]);
			quaternionf[0] = quaternion[0] / 32768.f;
			quaternionf[1] = quaternion[1] / 32768.f;
			quaternionf[2] = quaternion[2] / 32768.f;
			quaternionf[3] = quaternion[3] / 32768.f;
			LOG_READ("quaternion: %.2f, %.2f, %.2f, %.2f", quaternionf[0], quaternionf[1], quaternionf[2], quaternionf[3]);
			break;
			
		default:
			LOG_READ("unhandled return type: %x", type);
			break;
		}
		
		return type;
	}
	else
	{
		return kReturnType_Invalid;
	}
}
