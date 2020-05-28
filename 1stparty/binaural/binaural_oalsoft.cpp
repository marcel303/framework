/*
	Copyright (C) 2020 Marcel Smit
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

#include "binaural.h"
#include "binaural_oalsoft.h"

#include "FileStream.h"
#include "StreamReader.h"

#include <string.h> // memcmp

namespace binaural
{
	inline float lerpFloat(const float a, const float b, const float t)
	{
		return a * (1.f - t) + b * t;
	}
	
	bool loadHRIRSampleSet_Oalsoft(const char * filename, HRIRSampleSet & sampleSet)
	{
		// The OpenAL Soft project includes a utility app that takes a HRIR data set
		// (ircam, mit, and any other), and post-processes it to create a equalised,
		// minimum-phase output. The resulting HRIRs are better usable for linear
		// interpolating (less comb-filter-esque issues) and are smaller than other
		// representations, as it encodes to arrival time (inoput sampling delay)
		// separate from the HRIR data. The result is encoded in a practical file
		// format which is documented below.

		// source: https://github.com/kcat/openal-soft/blob/master/docs/hrtf.txt
		#if 0
			ALchar   magic[8] = "MinPHR03";
			ALuint   sampleRate;
			ALubyte  channelType; /* Can be 0 (mono) or 1 (stereo). */
			ALubyte  hrirSize;    /* Can be 8 to 128 in steps of 8. */
			ALubyte  fdCount;     /* Can be 1 to 16. */

			struct {
				ALushort distance;        /* Can be 50mm to 2500mm. */
				ALubyte evCount;          /* Can be 5 to 128. */
				ALubyte azCount[evCount]; /* Each can be 1 to 128. */
			} fields[fdCount];

			/* NOTE: ALbyte3 is a packed 24-bit sample type,
			 * hrirCount is the sum of all azCounts.
			 * channels can be 1 (mono) or 2 (stereo) depending on channelType.
			 */
			ALbyte3 coefficients[hrirCount][hrirSize][channels];
			ALubyte delays[hrirCount][channels]; /* Each can be 0 to 63. */
		#endif

		try
		{
			FileStream stream(filename, OpenMode_Read);
			StreamReader reader(&stream, false);
			
			uint8_t magic[8];
			reader.ReadBytes(magic, 8);
			
			if (memcmp(magic, "MinPHR03", 8) != 0) // 'minimum-phase head response protocol 03' ?
				return false;
			
			const uint32_t sampleRate = reader.ReadUInt32();
			const uint8_t channelType = reader.ReadUInt8();
			const uint8_t hrirSize = reader.ReadUInt8();
			const uint8_t fieldCount = reader.ReadUInt8();
			
			struct Field
			{
				uint16_t distance;
				uint8_t elevationCount;
				std::vector<uint8_t> azimuthCounts;
			};
			
			std::vector<Field> fields;
			fields.resize(fieldCount);
			
			for (int i = 0; i < fieldCount; ++i)
			{
				auto & field = fields[i];
				field.distance = reader.ReadUInt16();
				field.elevationCount = reader.ReadUInt8();
				field.azimuthCounts.resize(field.elevationCount);
				for (int i = 0; i < field.elevationCount; ++i)
					field.azimuthCounts[i] = reader.ReadUInt8();
			}
			
			const uint8_t channelCount = channelType + 1; // 0 = mono, 1 = stereo, so just add one to get the count
			
			bool first = true;
			
			for (auto & field : fields)
			{
				uint32_t hrirCount = 0;
				for (auto & azimuthCount : field.azimuthCounts)
					hrirCount += azimuthCount;
				uint8_t * coefficients = new uint8_t[hrirCount * hrirSize * channelCount * 3];
				uint8_t * delays = new uint8_t[hrirCount * channelCount];
				
				reader.ReadBytes(coefficients, hrirCount * hrirSize * channelCount * 3 * sizeof(coefficients[0]));
				reader.ReadBytes(delays, hrirCount * channelCount * sizeof(delays[0]));
				
				// Convert and add the HRIR sample we read from file.
				// Note : We're only interested in the nearest field (stored first).
				// Note : We read until the end of the file to ensure the loader works correctly. We could actually stop reading after the first field..
				if (first)
				{
					first = false;
					
					const uint8_t * coefficients_ptr = coefficients;
					
					for (int e = 0; e < field.elevationCount; ++e )
					{
						const float elevation = lerpFloat(-90.f, +90.f, e / float(field.elevationCount - 1));
						
						// We may need to mirror the right channel to the value at '360 - azimuth', if the file contains mono samples.
						// Therefore, we first capture an array of HRIRs for this elevation level, so we may later go over this array
						// again and mirror the data.
						std::vector<HRIRSample*> samplesForElevation;
						
						for (int a = 0; a < field.azimuthCounts[e]; ++a)
						{
							const float azimuth =
								field.azimuthCounts[e] == 1
								? 0 // This sample will be duplicated at azimuth -180 and +180 later.
								: lerpFloat(-180.f, 180.f, fmodf(.5f + a / float(field.azimuthCounts[e] - 1), 1.f));
							
							HRIRSample * sample = new HRIRSample();
							sample->init(elevation, azimuth);
							
							memset(&sample->sampleData, 0, sizeof(sample->sampleData));
							for (int i = 0; i < hrirSize; ++i)
							{
								for (int c = 0; c < channelCount; ++c)
								{
									int32_t value =
										(coefficients_ptr[0] <<  8) |
										(coefficients_ptr[1] << 16) |
										(coefficients_ptr[2] << 24);
									
									// perform a shift right to sign-extend the 24 bit number (which we packed into the top-most 24 bits of a 32 bits number)
									value >>= 8;
									
									const float value_float = value / float(1 << 23);
									
									if (c == 0) sample->sampleData.lSamples[i] = value_float;
									if (c == 1) sample->sampleData.rSamples[i] = value_float;
									
									coefficients_ptr += 3;
								}
							}
							
							if (field.azimuthCounts[e] > 1)
							{
								samplesForElevation.push_back(sample);
							}
							else
							{
								// The sample set lookup method requires that convex
								// hull surrounding all of the sample points describe
								// a 2D map with extents (-180, -90) to (+180, +90)
								// azimuth, elevation. Extend the polar samples here
								// to ensure they cover the entire upper region.
								HRIRSample * sampleL = sample;
								HRIRSample * sampleR = new HRIRSample();
								*sampleR = *sampleL;
								
								sampleL->azimuth = -180.f;
								sampleR->azimuth = +180.f;
								
								samplesForElevation.push_back(sampleL);
								samplesForElevation.push_back(sampleR);
							}
						}
						
						if (channelCount == 1)
						{
							// Create stereo data by mirroring channel data at '360 - azimuth'.
							for (int i = 0; i < samplesForElevation.size(); ++i)
							{
								auto * src = samplesForElevation[i];
								auto * dst = samplesForElevation[samplesForElevation.size() - 1 - i];
								
								if (src == dst)
									continue;

								memcpy(
									dst->sampleData.rSamples,
									src->sampleData.lSamples,
									hrirSize * sizeof(float));
							}
						}
						
						for (auto * sample : samplesForElevation)
						{
							sampleSet.samples.push_back(sample);
						}
					}
				}
				
				//
				
				delete [] coefficients;
				coefficients = nullptr;
				
				delete [] delays;
				delays = nullptr;
			}
			
			return true;
		}
		catch (std::exception & e)
		{
			(void)e;

			return false;
		}
	}
}
