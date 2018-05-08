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

/*

note : the following OSC controls from the 4DSOUND OSC sheet were not implemented,

sourceGesture###
sourcePath###
sourceModulation###
sourceVantagePoint###
sourceAngleFilter
sourceElevationFilter
subReduction###

*/

struct Osc4D
{
	enum OrientationMode
	{
		kOrientation_Static = 0,
		kOrientation_Movement = 1,
		kOrientation_Center = 2
	};

	enum ProjectionMode
	{
		kProjection_Perspective = 0,
		kProjection_Orthogonal = 1
	};

	enum SpatialDelayMode
	{
		kSpatialDelay_Random = 0,
		kSpatialDelay_Grid = 1
	};
	
	enum SubBoost
	{
		kBoost_None,
		kBoost_Level1,
		kBoost_Level2,
		kBoost_Level3
	};
	
	enum ReturnSide
	{
		kReturnSide_Left,
		kReturnSide_Right,
		kReturnSide_Top,
		kReturnSide_Bottom,
		kReturnSide_Front,
		kReturnSide_Back,
		kReturnSide_COUNT
	};
	
	int source;
	char sourceOscName[64];
	char returnOscName[64];

	Osc4D()
		: source(0)
		, sourceOscName()
		, returnOscName()
	{
	}
	
	virtual ~Osc4D()
	{
	}
	
	virtual void begin(const char * name) = 0;
	virtual void end() = 0;
	virtual void b(const bool v) = 0;
	virtual void i(const int v) = 0;
	virtual void f(const float v) = 0;
	virtual void s(const char * v) = 0;

	void beginSource(const char * name);
	void beginReturn(const char * name);
	void f3(const float x, const float y, const float z);

	//

	void setSource(const int source);

	void sourceColor(const float r, const float g, const float b);
	void sourceName(const char * name);
	void sourcePosition(const float x, const float y, const float z);
	
	void sourceInvert(const bool x, const bool y, const bool z);
	void sourceDimensions(const float x, const float y, const float z);
	void sourceRotation(const float x, const float y, const float z);
	
	void sourceOrientationMode(const OrientationMode mode, const float centerX, const float centerY, const float centerZ);
	void sourceGlobalEnable(const bool enabled);
	void sourceProjectionMode(const ProjectionMode mode);
	void sourceArticulation(const float articulation);
	void sourceSpatialCompressor(const bool enable, const float attack, const float release, const float minimum, const float maximum, const float curve, const bool invert);
	void sourceDoppler(const bool enable, const float scale, const float smooth);

	void sourceDistanceIntensity(const bool enable, const float treshold, const float curve);
	void sourceDistanceDamping(const bool enable, const float treshold, const float curve);
	void sourceDistanceDiffusion(const bool enable, const float treshold, const float curve);

	void sourceSpatialDelay(const bool enable, const SpatialDelayMode mode, const int times, const float feedback, const float drywet, const float smooth, const float scale, const float noiseDepth, const float noiseFrequency);
	
	void sourceSend(const bool enabled);
	
	void sourceSubBoost(const SubBoost boost);
	
	void setReturn(const int index, const char * side);
	void returnDistanceIntensity(const int index, const bool enable, const float treshold, const float curve);
	void returnDistanceDamping(const int index, const bool enable, const float treshold, const float curve);
	void returnSide(const int index, const ReturnSide side, const bool enable, const float distance, const float scatter);

	void globalPosition(const float x, const float y, const float z);
	void globalDimensions(const float x, const float y, const float z);
	void globalRotation(const float x, const float y, const float z);
	void globalPlode(const float x, const float y, const float z);
	void globalOrigin(const float x, const float y, const float z);
	void globalMasterPhase(const float v);
};

//

#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"

struct Osc4DStream : Osc4D
{
	static const int OSC_BUFFER_SIZE = 2048;

	char buffer[OSC_BUFFER_SIZE];
	osc::OutboundPacketStream stream;
	
	UdpTransmitSocket * transmitSocket;
	
	bool hasMessage;
	bool bundleIsInvalid;

	Osc4DStream();
	virtual ~Osc4DStream() override;
	
	void init(const char * ipAddress, const int udpPort);
	void shut();

	bool isReady() const;
	void setEndpoint(const char * ipAddress, const int udpPort);
	
	void beginBundle();
	void endBundle();
	void flush();

	virtual void begin(const char * name) override;
	virtual void end() override;

	virtual void b(const bool v) override;
	virtual void i(const int v) override;
	virtual void f(const float v) override;
	virtual void s(const char * v) override;
};

extern Osc4DStream * g_oscStream;
