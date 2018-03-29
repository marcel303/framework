#include "AmbisonicEncoder.h"
#include "AmbisonicEncoderDist.h"
#include "AmbisonicMicrophone.h"
#include "framework.h"
#include <math.h>

#define USE_3D 1
#define USE_DIST 1

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 256

#define NUM_BUFFERS (SAMPLE_RATE*60/BUFFER_SIZE)

#define NUM_CHANNELS 4

#define DO_PREVIEW 1

#define MICGRID_SX 40
#define MICGRID_SY 30

const int GFX_SX = 1024;
const int GFX_SY = 768;

static float samples[NUM_CHANNELS][BUFFER_SIZE * NUM_BUFFERS];

/*

PcmAudioCapture
---------------

a simple helper object to capture multi-channel audio and save it to a raw PCM file

it first needs to be initialized with the number of channels one wishes to capture

it then needs to be fed sample data. before feeding it data, beginSegment must be called with the number of samples
that will be added. next, the data itself should be submitted. the data can be presented as planar data for a single
channel, in which case it needs to be fed data for each channel individually, or as interleaved data for all channels
at once

planar data example:
	PcmAudioCapture audioCapture;
	audioCapture.init(2);
 
	for (int i = 0; i < 10; ++i)
	{
		// generate some audio
		float leftSamples[256];
		float rightSamples[256];
		generateNoise_Planar(leftSamples);
		generateNoise_Planar(rightSamples);
 
		// and capture it
		audioCapture.beginSegment(256);
		audioCapture.addSamples_Planar(0, leftSamples);
		audioCapture.addSamples_Planar(1, rightSamples);
	}
 
	audioCapture.savePcmData("raw.pcm");
	audioCapture.shut();

interleaved data example:
	PcmAudioCapture audioCapture;
	audioCapture.init(2);
 
	for (int i = 0; i < 10; ++i)
	{
		// generate some audio
		float samples[256 * 2];
		generateNoise_Interleaved(samples);
 
		// and capture it
		audioCapture.beginSegment(256);
		audioCapture.addSamples_Interleaved(samples);
	}
 
	audioCapture.savePcmData("raw.pcm");
	audioCapture.shut();
 
*/

struct PcmAudioCapture
{
	struct Segment
	{
		Segment * next;
		
		float * samples;
		int numSamples;
		
		Segment()
		{
			memset(this, 0, sizeof(*this));
		}
	};
	
	Segment * firstSegment;
	Segment * currentSegment;
	
	int numChannels;
	
	PcmAudioCapture();
	~PcmAudioCapture();
	
	void init(const int numChannels);
	void shut();
	
	void beginSegment(const int numSamples);
	void addSamples_Planar(const int channel, const float * samples);
	void addSamples_Interleaved(const float * samples);
	
	bool savePcmData(const char * filename) const;
};

PcmAudioCapture::PcmAudioCapture()
{
	memset(this, 0, sizeof(*this));
}

PcmAudioCapture::~PcmAudioCapture()
{
	shut();
}

void PcmAudioCapture::init(const int _numChannels)
{
	shut();
	
	//
	
	numChannels = _numChannels;
}

void PcmAudioCapture::shut()
{
	Segment * segment = firstSegment;
	
	while (segment != nullptr)
	{
		Segment * next = segment->next;
		
		//
		
		delete [] segment->samples;
		segment->samples = nullptr;
		
		delete segment;
		segment = nullptr;
		
		//
		
		segment = next;
	}
	
	firstSegment = nullptr;
	currentSegment = nullptr;
	
	numChannels = 0;
}

void PcmAudioCapture::beginSegment(const int numSamples)
{
	Segment * segment = new Segment();
	segment->samples = new float[numChannels * numSamples];
	segment->numSamples = numSamples;
	
	if (currentSegment)
	{
		currentSegment->next = segment;
		currentSegment = segment;
	}
	else
	{
		firstSegment = segment;
		currentSegment = segment;
	}
}

void PcmAudioCapture::addSamples_Planar(const int channel, const float * samples)
{
	for (int i = 0; i < currentSegment->numSamples; ++i)
	{
		currentSegment->samples[i * numChannels + channel] = samples[i];
	}
}

void PcmAudioCapture::addSamples_Interleaved(const float * samples)
{
	memcpy(currentSegment->samples, samples, numChannels * currentSegment->numSamples);
}

bool PcmAudioCapture::savePcmData(const char * filename) const
{
	bool result = true;
	
	FILE * file = fopen(filename, "wb");
	
	if (file == nullptr)
	{
		result = false;
	}
	else
	{
		for (Segment * segment = firstSegment; segment != nullptr; segment = segment->next)
		{
			const int numValues = numChannels * segment->numSamples;
			
			if (fwrite(segment->samples, sizeof(float), numValues, file) != numValues)
			{
				result = false;
				break;
			}
		}
		
		fclose(file);
		file = nullptr;
	}
	
	return result;
}

int main(int argc, char * argv[])
{
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	PcmAudioCapture audioCapture;
	audioCapture.init(NUM_CHANNELS);
	
	const double dt = 1.0 / SAMPLE_RATE;
	
	double azimuth = 0.0;
	double azimuthSpeed = 0.3;
	
	double p = 0.0;
	double f = 100.0;
	
	// b-format descriptor
	CBFormat myBFormat;
	myBFormat.Configure(1, USE_3D, BUFFER_SIZE);
	
	Assert(myBFormat.GetChannelCount() == NUM_CHANNELS);
	
	// ambisonic encoder
#if USE_DIST
	CAmbisonicEncoderDist myEncoder;
	myEncoder.SetRoomRadius(5.f);
	myEncoder.Configure(1, USE_3D, SAMPLE_RATE);
#else
	CAmbisonicEncoder myEncoder;
	myEncoder.Configure(1, USE_3D, 0);
#endif

	CAmbisonicMicrophone mics[MICGRID_SX][MICGRID_SY];
	
	for (int x = 0; x < MICGRID_SX; ++x)
	{
		for (int y = 0; y < MICGRID_SY; ++y)
		{
			const float azimuth = (x + .5f) / MICGRID_SX * 2.f * M_PI;
			const float elevation = ((y + .5f) / MICGRID_SY -.5f) * M_PI;
			
			PolarPoint position;
			position.fAzimuth = azimuth;
			position.fElevation = elevation;
			position.fDistance = 0.f;
			
			mics[x][y].Configure(1, true, 0);
			mics[x][y].SetPosition(position);
			mics[x][y].Refresh();
		}
	}
	
	for (int b = 0; b < NUM_BUFFERS; ++b)
	{
		float buffer[BUFFER_SIZE];
		
		// generate a sine signal with some noise
		
		for (int i = 0; i < BUFFER_SIZE; ++i)
		{
			f += dt * 100.0;
			p += f * dt;
			
			const float sine = (float)sin(p * (M_PI * 2));
			const float noise = ((rand() % 100) / 100.f - .5f) * 2.f;
			
			buffer[i] = sine * .1f + noise * .5f;
		}
		
		// set ambisonic source location in the sound field
		
		azimuth += azimuthSpeed * dt * BUFFER_SIZE;
		
		PolarPoint position;
		position.fAzimuth = azimuth * (M_PI * 2.0);
		position.fElevation = 0.f;
		position.fDistance = 10.f;
		myEncoder.SetPosition(position);
		myEncoder.Refresh();
		
		// encode source signal into b-format
		
		myEncoder.Process(buffer, BUFFER_SIZE, &myBFormat);
		
		// extract the b-format data and put it in the sample buffer
		
		for (int i = 0; i < NUM_CHANNELS; ++i)
		{
			myBFormat.ExtractStream(samples[i] + b * BUFFER_SIZE, i, BUFFER_SIZE);
		}
		
		// extract the b-format data and capture it
		
		audioCapture.beginSegment(BUFFER_SIZE);
		
		for (int i = 0; i < NUM_CHANNELS; ++i)
		{
			float samples[BUFFER_SIZE];
			
			myBFormat.ExtractStream(samples, i, BUFFER_SIZE);
			
			audioCapture.addSamples_Planar(i, samples);
		}
		
		//
		
	#if DO_PREVIEW
		if ((b % 4) == 0 && !keyboard.isDown(SDLK_ESCAPE))
		{
			framework.process();
			
			framework.beginDraw(0, 0, 0, 0);
			{
				gxPushMatrix();
				{
					gxTranslatef(0, GFX_SY/2.f, 0.f);
					gxScalef(GFX_SX / float(BUFFER_SIZE), GFX_SY/2.f, 1.f);
					
					for (int c = 0; c < NUM_CHANNELS; ++c)
					{
						gxPushMatrix();
						gxTranslatef(0.f, -1.f + 2.f / NUM_CHANNELS * (c + .5f), 0.f);
						gxScalef(1.f, 1.f / NUM_CHANNELS, 0.f);
						
						setColor(colorWhite);
						hqBegin(HQ_LINES, true);
						{
							for (int i = 0; i < BUFFER_SIZE - 1; ++i)
							{
								const float x1 = i + 0;
								const float x2 = i + 1;
								const float y1 = samples[c][b * BUFFER_SIZE + i + 0];
								const float y2 = samples[c][b * BUFFER_SIZE + i + 1];
								//const float y1 = buffer[i + 0];
								//const float y2 = buffer[i + 1];
								
								hqLine(x1, y1, 2.f, x2, y2, 2.f);
							}
						}
						hqEnd();
						
						gxPopMatrix();
					}
				}
				gxPopMatrix();
				
				gxPushMatrix();
				{
					gxScalef(GFX_SX / float(MICGRID_SX), GFX_SY / float(MICGRID_SY), 1.f);
					
					hqBegin(HQ_FILLED_CIRCLES);
					
					for (int x = 0; x < MICGRID_SX; ++x)
					{
						for (int y = 0; y < MICGRID_SY; ++y)
						{
							float values[BUFFER_SIZE];
							
							mics[x][y].Process(&myBFormat, BUFFER_SIZE, values);
							
							float mag = 0.f;
							
							for (int i = 0; i < BUFFER_SIZE; ++i)
								mag += values[i] * values[i];
							//mag = sqrtf(mag);
							mag /= BUFFER_SIZE;
							
							hqFillCircle(x, y, mag * 40.f);
						}
					}
					
					hqEnd();
				}
				gxPopMatrix();
			}
			framework.endDraw();
		}
	#endif
	}
	
	// save to raw PCM stream
	
	printf("writing B-format. numChannels=%d, numSamples=%d\n", myBFormat.GetChannelCount(), myBFormat.GetSampleCount());
	
	FILE * file = fopen("raw1.pcm", "wb");
	
	if (file != nullptr)
	{
		for (int s = 0; s < BUFFER_SIZE * NUM_BUFFERS; ++s)
		{
			for (int c = 0; c < NUM_CHANNELS; ++c)
			{
				fwrite(&samples[c][s], sizeof(float), 1, file);
			}
		}
		
		fclose(file);
		file = nullptr;
	}
	
	audioCapture.savePcmData("raw2.pcm");
	audioCapture.shut();
	
	framework.shutdown();
	
	return 0;
}
