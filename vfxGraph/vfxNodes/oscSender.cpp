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

#include "Log.h"
#include "oscSender.h"

#include "ip/UdpSocket.h"

OscSender::OscSender()
	: ipAddress()
	, udpPort(0)
	, transmitSocket(nullptr)
{
}

bool OscSender::isAddressValid(const char * ipAddress, const int udpPort) const
{
	if (ipAddress == nullptr || ipAddress[0] == 0)
		return false;
	if (udpPort == 0)
		return false;
	
	return true;
}

bool OscSender::isAddressChange(const char * _ipAddress, const int _udpPort) const
{
	return _ipAddress != ipAddress || _udpPort != udpPort;
}

bool OscSender::init(const char * _ipAddress, const int _udpPort)
{
	try
	{
		shut();
		
		//
		
		ipAddress = _ipAddress;
		udpPort = _udpPort;
		
		LOG_DBG("creating OSC UDP send socket @ %s:%d", ipAddress.c_str(), udpPort);
		
		if (isAddressValid(ipAddress.c_str(), udpPort) == false)
		{
			LOG_WRN("invalid OSC send address: %s:%d", ipAddress.c_str(), udpPort);
			
			return false;
		}
		else
		{
			transmitSocket = new UdpSocket();
			transmitSocket->SetEnableBroadcast(true);
			transmitSocket->SetAllowReuse(true);
			transmitSocket->Bind(IpEndpointName(ipAddress.c_str(), udpPort));
			
			return true;
		}
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to init OSC sender: %s. ipAddress=%s, udpPort=%d", e.what(), ipAddress.c_str(), udpPort);
		
		shut();
		
		return false;
	}
}

bool OscSender::shut()
{
	try
	{
		if (transmitSocket != nullptr)
		{
			delete transmitSocket;
			transmitSocket = nullptr;
		}
		
		return true;
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to shut down OSC sender: %s", e.what());
		
		return false;
	}
}

void OscSender::send(const void * data, const int dataSize)
{
	if (transmitSocket != nullptr)
	{
		transmitSocket->Send((char*)data, dataSize);
	}
}
