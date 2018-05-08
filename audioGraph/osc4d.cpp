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

#include "osc4d.h"
#include "StringEx.h"

void Osc4D::beginSource(const char * name)
{
	char text[1024];
	int textLength = 0;
	for (int i = 0; sourceOscName[i]; ++i)
		text[textLength++] = sourceOscName[i];
	for (int i = 0; name[i]; ++i)
		text[textLength++] = name[i];
	text[textLength] = 0;
	
	begin(text);
}

void Osc4D::beginReturn(const char * name)
{
	char text[1024];
	int textLength = 0;
	for (int i = 0; returnOscName[i]; ++i)
		text[textLength++] = returnOscName[i];
	for (int i = 0; name[i]; ++i)
		text[textLength++] = name[i];
	text[textLength] = 0;
	
	begin(text);
}

void Osc4D::f3(const float x, const float y, const float z)
{
	f(x);
	f(y);
	f(z);
}

void Osc4D::setSource(const int _source)
{
	source = _source;
	
	sprintf_s(sourceOscName, sizeof(sourceOscName), "/source%d/", source + 1);
}

void Osc4D::sourceColor(const float r, const float g, const float b)
{
	beginSource("color");
	f3(r, g, b);
	end();
}

void Osc4D::sourceName(const char * name)
{
	beginSource("name");
	s(name);
	end();
}

void Osc4D::sourcePosition(const float x, const float y, const float z)
{
	beginSource("position");
	f3(x, y, z);
	end();
}

void Osc4D::sourceInvert(const bool x, const bool y, const bool z)
{
	// would be nice to have invert that operates on all three axis

	beginSource("invertX");
	b(x);
	end();

	beginSource("invertY");
	b(y);
	end();

	beginSource("invertZ");
	b(z);
	end();
}

void Osc4D::sourceDimensions(const float x, const float y, const float z)
{
	beginSource("dimensions");
	f3(x, y, z);
	end();
}

void Osc4D::sourceRotation(const float x, const float y, const float z)
{
	beginSource("rotation");
	f3(x, y, z);
	end();
}

void Osc4D::sourceOrientationMode(const OrientationMode mode, const float centerX, const float centerY, const float centerZ)
{
	beginSource("orientationMode");
	i(mode);
	end();

	beginSource("orientationCenter");
	f3(centerX, centerY, centerZ);
	end();
}

void Osc4D::sourceGlobalEnable(const bool enabled)
{
	beginSource("globalEnable");
	b(enabled);
	end();
}

void Osc4D::sourceProjectionMode(const ProjectionMode mode)
{
	beginSource("projectionMode");
	i(mode);
	end();
}

void Osc4D::sourceArticulation(const float articulation)
{
	beginSource("articulation");
	f(articulation);
	end();
}

void Osc4D::sourceSpatialCompressor(const bool enable, const float attack, const float release, const float minimum, const float maximum, const float curve, const bool invert)
{
	beginSource("spatialCompressor/enable");
	b(enable);
	end();

	if (enable)
	{
		beginSource("spatialCompressor/attack");
		f(attack);
		end();

		beginSource("spatialCompressor/release");
		f(release);
		end();

		beginSource("spatialCompressor/minimum");
		f(minimum);
		end();

		beginSource("spatialCompressor/maximum");
		f(maximum);
		end();

		beginSource("spatialCompressor/curve");
		f(curve);
		end();

		beginSource("spatialCompressor/invert");
		b(invert);
		end();

	}
}

void Osc4D::sourceDoppler(const bool enable, const float scale, const float smooth)
{
	beginSource("doppler/enable");
	b(enable);
	end();

	if (enable)
	{
		beginSource("doppler/scale");
		f(scale);
		end();

		beginSource("doppler/smooth");
		f(smooth);
		end();
	}
}

void Osc4D::sourceDistanceIntensity(const bool enable, const float treshold, const float curve)
{
	beginSource("distanceIntensity/enable");
	b(enable);
	end();

	if (enable)
	{
		beginSource("distanceIntensity/threshold");
		f(treshold);
		end();

		beginSource("distanceIntensity/curve");
		f(curve);
		end();
	}
}

void Osc4D::sourceDistanceDamping(const bool enable, const float treshold, const float curve)
{
	beginSource("distanceDamping/enable");
	b(enable);
	end();

	if (enable)
	{
		beginSource("distanceDamping/threshold");
		f(treshold);
		end();

		beginSource("distanceDamping/curve");
		f(curve);
		end();
	}
}

void Osc4D::sourceDistanceDiffusion(const bool enable, const float treshold, const float curve)
{
	beginSource("distanceDiffusion/enable");
	b(enable);
	end();

	if (enable)
	{
		beginSource("distanceDiffusion/threshold");
		f(treshold);
		end();

		beginSource("distanceDiffusion/curve");
		f(curve);
		end();
	}
}

void Osc4D::sourceSpatialDelay(const bool enable, const SpatialDelayMode mode, const int times, const float feedback, const float drywet, const float smooth, const float scale, const float noiseDepth, const float noiseFrequency)
{
	beginSource("spatialDelay/enable");
	b(enable);
	end();

	if (enable)
	{
		beginSource("spatialDelay/mode");
		i(mode);
		end();

		//beginSource("spatialDelay/times");
		//i(times);
		//end();

		beginSource("spatialDelay/feedback");
		f(feedback);
		end();

		beginSource("spatialDelay/drywet");
		f(drywet);
		end();

		beginSource("spatialDelay/smooth");
		f(smooth);
		end();

		beginSource("spatialDelay/scale");
		f(scale);
		end();

		beginSource("spatialDelay/noiseDepth");
		f(noiseDepth);
		end();

		beginSource("spatialDelay/noiseFreq");
		f(noiseFrequency);
		end();

		//beginSource("spatialDelay/pattern");
		//i(0);
		//end();
	}
}

void Osc4D::sourceSend(const bool enabled)
{
	beginSource("send1");
	f(enabled ? 1.f : 0.f);
	end();
}

void Osc4D::sourceSubBoost(const SubBoost boost)
{
	beginSource("subBoost/mode");
	i(boost);
	end();
}

void Osc4D::setReturn(const int index, const char * path)
{
	sprintf_s(returnOscName, sizeof(returnOscName), "/return%d/%s/", index + 1, path);
}

void Osc4D::returnDistanceIntensity(const int index, const bool enable, const float treshold, const float curve)
{
	setReturn(index, "distanceIntensity");
	
	beginReturn("enable");
	b(enable);
	end();
	
	if (enable)
	{
		beginReturn("threshold");
		f(treshold);
		end();
		
		beginReturn("curve");
		f(curve);
		end();
	}
}

void Osc4D::returnDistanceDamping(const int index, const bool enable, const float treshold, const float curve)
{
	setReturn(index, "distanceDamping");
	
	beginReturn("enable");
	b(enable);
	end();
	
	if (enable)
	{
		beginReturn("threshold");
		f(treshold);
		end();
		
		beginReturn("curve");
		f(curve);
		end();
	}
}

void Osc4D::returnSide(const int index, const ReturnSide side, const bool enable, const float distance, const float scatter)
{
	/*
	kReturnSide_Left,
	kReturnSide_Right,
	kReturnSide_Top,
	kReturnSide_Bottom,
	kReturnSide_Front,
	kReturnSide_Back,
	*/
	
	const char * sideName[kReturnSide_COUNT] =
	{
		"left",
		"right",
		"top",
		"bottom",
		"front",
		"back"
	};
	
	setReturn(index, sideName[side]);
	
	beginReturn("enable");
	b(enable);
	end();
	
	if (enable)
	{
		beginReturn("distance");
		f(distance);
		end();
		
		beginReturn("scatter");
		f(scatter);
		end();
	}
}

void Osc4D::globalPosition(const float x, const float y, const float z)
{
	begin("/global/position");
	f3(x, y, z);
	end();
}

void Osc4D::globalDimensions(const float x, const float y, const float z)
{
	begin("/global/dimensions");
	f3(x, y, z);
	end();
}

void Osc4D::globalRotation(const float x, const float y, const float z)
{
	begin("/global/rotation");
	f3(x, y, z);
	end();
}

void Osc4D::globalPlode(const float x, const float y, const float z)
{
	begin("/global/plode");
	f3(x, y, z);
	end();
}

void Osc4D::globalOrigin(const float x, const float y, const float z)
{
	begin("/global/origin");
	f3(x, y, z);
	end();
}

void Osc4D::globalMasterPhase(const float v)
{
	begin("/global/masterPhase");
	f(v);
	end();
}

//

#include "Debugging.h"
#include "Log.h"

//#include <functional>

#define OSC_SEND_BUNDLES 1

Osc4DStream::Osc4DStream()
	: stream(buffer, OSC_BUFFER_SIZE)
	, transmitSocket(nullptr)
	, hasMessage(false)
	, bundleIsInvalid(false)
{
}

Osc4DStream::~Osc4DStream()
{
	shut();
}

void Osc4DStream::init(const char * ipAddress, const int udpPort)
{
	shut();
	
	//
	
	try
	{
		Assert(transmitSocket == nullptr);
		transmitSocket = new UdpTransmitSocket(IpEndpointName(ipAddress, udpPort));
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to create UDP transmit socket: %s", e.what());
		Assert(transmitSocket == nullptr);
	}
}

void Osc4DStream::shut()
{
	delete transmitSocket;
	transmitSocket = nullptr;
}

bool Osc4DStream::isReady() const
{
	return transmitSocket != nullptr;
}

void Osc4DStream::setEndpoint(const char * ipAddress, const int udpPort)
{
	shut();
	
	init(ipAddress, udpPort);
}

void Osc4DStream::beginBundle()
{
#if OSC_SEND_BUNDLES
	try
	{
		bundleIsInvalid = false;
		
		stream << osc::BeginBundleImmediate;
	}
	catch (std::exception & e)
	{
		LOG_ERR("beginBundle failed: %s", e.what());
		
		bundleIsInvalid = true;
	}
#endif
}

void Osc4DStream::endBundle()
{
#if OSC_SEND_BUNDLES
	try
	{
		stream << osc::EndBundle;
	}
	catch (std::exception & e)
	{
		LOG_ERR("endBundle failed: %s", e.what());
		
		bundleIsInvalid = true;
	}
	
	flush();
#endif
}

void Osc4DStream::flush()
{
	if (hasMessage && stream.IsReady() && bundleIsInvalid == false)
	{
		try
		{
			transmitSocket->Send(stream.Data(), stream.Size());
		}
		catch (std::exception & e)
		{
			LOG_ERR("flush failed: %s", e.what());
		}
	}
	
	stream = osc::OutboundPacketStream(buffer, OSC_BUFFER_SIZE);
	hasMessage = false;
}

void Osc4DStream::begin(const char * name)
{
#if OSC_SEND_BUNDLES == 0
	stream << osc::BeginBundleImmediate;
#endif

	stream << osc::BeginMessage(name);
	
	hasMessage = true;
}

void Osc4DStream::end()
{
	stream << osc::EndMessage;
	
#if OSC_SEND_BUNDLES == 0
	stream << osc::EndBundle;
	
	flush();
#endif
}

void Osc4DStream::b(const bool v)
{
	stream << v;
}

void Osc4DStream::i(const int v)
{
	stream << v;
}

void Osc4DStream::f(const float v)
{
	stream << v;
}

void Osc4DStream::s(const char * v)
{
	stream << v;
}

