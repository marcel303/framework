#pragma once

#define OSC_SEND_BUNDLES 1

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
	
	int source;
	char sourceOscName[64];

	Osc4D()
		: source(0)
		, sourceOscName()
	{
	}
	
	virtual void begin(const char * name) = 0;
	virtual void end() = 0;
	virtual void b(const bool v) = 0;
	virtual void i(const int v) = 0;
	virtual void f(const float v) = 0;
	virtual void s(const char * v) = 0;

	void beginSource(const char * name);
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

	// todo : sourceGesture###
	// todo : sourcePath###
	// todo : sourceModulation###

	void sourceGlobalEnable(const bool enabled);
	void sourceProjectionMode(const ProjectionMode mode);
	void sourceArticulation(const float articulation);

	// todo : sourceVantagePoint###

	void sourceSpatialCompressor(const bool enable, const float attack, const float release, const float minimum, const float maximum, const float curve, const bool invert);
	void sourceDoppler(const bool enable, const float scale, const float smooth);
	
	void sourceDistanceIntensity(const bool enable, const float treshold, const float curve);
	void sourceDistanceDamping(const bool enable, const float treshold, const float curve);
	void sourceDistanceDiffusion(const bool enable, const float treshold, const float curve);

	// todo : sourceAngleFilter

	// todo : sourceElevationFilter

	void sourceSpatialDelay(const bool enable, const SpatialDelayMode mode, const int times, const float feedback, const float drywet, const float smooth, const float scale, const float noiseDepth, const float noiseFrequency);
	
	// todo : subReduction###
	
	void sourceSubBoost(const SubBoost boost);

	// todo : return###

	void globalPosition(const float x, const float y, const float z);
	void globalDimensions(const float x, const float y, const float z);
	void globalRotation(const float x, const float y, const float z);
	void globalPlode(const float x, const float y, const float z);
	void globalOrigin(const float x, const float y, const float z);
	void globalMasterPhase(const float v);
};

#include "Debugging.h"
#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"
#include <functional>

#define OSC_BUFFER_SIZE (64*1024)

struct Osc4DStream : Osc4D
{
	char buffer[OSC_BUFFER_SIZE];
	osc::OutboundPacketStream stream;
	
	UdpTransmitSocket * transmitSocket;
	
	bool hasMessage;

	Osc4DStream()
		: stream(buffer, OSC_BUFFER_SIZE)
		, transmitSocket(nullptr)
		, hasMessage(false)
	{
	}
	
	~Osc4DStream()
	{
		shut();
	}
	
	void init(const char * ipAddress, const int udpPort)
	{
		shut();
		
		//
		
		Assert(transmitSocket == nullptr);
		transmitSocket = new UdpTransmitSocket(IpEndpointName(ipAddress, udpPort));
	}
	
	void shut()
	{
		delete transmitSocket;
		transmitSocket = nullptr;
	}
	
	void setEndpoint(const char * ipAddress, const int udpPort)
	{
		shut();
		
		init(ipAddress, udpPort);
	}
	
	void beginBundle()
	{
	#if OSC_SEND_BUNDLES
		stream << osc::BeginBundleImmediate;
	#endif
	}
	
	void endBundle()
	{
	#if OSC_SEND_BUNDLES
		stream << osc::EndBundle;
		
		flush();
	#endif
	}
	
	void flush()
	{
		if (hasMessage && stream.IsReady())
		{
			transmitSocket->Send(stream.Data(), stream.Size());
		}
		
		stream = osc::OutboundPacketStream(buffer, OSC_BUFFER_SIZE);
		hasMessage = false;
	}

	virtual void begin(const char * name) override
	{
	#if OSC_SEND_BUNDLES == 0
		stream << osc::BeginBundleImmediate;
	#endif
	
		stream << osc::BeginMessage(name);
		
		hasMessage = true;
	}
	
	virtual void end() override
	{
		stream << osc::EndMessage;
		
	#if OSC_SEND_BUNDLES == 0
		stream << osc::EndBundle;
		
		flush();
	#endif
	}

	virtual void b(const bool v) override
	{
		stream << v;
	}

	virtual void i(const int v) override
	{
		stream << v;
	}

	virtual void f(const float v) override
	{
		stream << v;
	}
	
	virtual void s(const char * v) override
	{
		stream << v;
	}
};

extern Osc4DStream * g_oscStream;
