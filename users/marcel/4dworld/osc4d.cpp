#include "osc4d.h"
#include "StringEx.h"

void Osc4D::beginSource(const char * name)
{
	char text[1024];
	sprintf_s(text, sizeof(text), "/source%d/%s", source + 1, name);
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

// todo : sourceGesture###
// todo : sourcePath###
// todo : sourceModulation###

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

// todo : sourceVantagePoint###

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
		beginSource("distanceIntensity/treshold");
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
		beginSource("distanceDamping/treshold");
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
		beginSource("distanceDiffusion/treshold");
		f(treshold);
		end();

		beginSource("distanceDiffusion/curve");
		f(curve);
		end();
	}
}

// todo : sourceAngleFilter

// todo : sourceElevationFilter

// todo : times is integer ?
// todo : pattern ?
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

		beginSource("spatialDelay/times");
		i(times);
		end();

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

// todo : sourceSub###

// todo : return###

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
