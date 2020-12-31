#pragma once

#include "renderOptions.h"

#include "framework.h"

#include "Debugging.h"
#include <math.h>
#include <string.h>

namespace ImGui
{
	struct ColorGradingEditor
	{
		static const int kLookupSize = rOne::RenderOptions::ColorGrading::kLookupSize;
		
		using LookupTable = float[kLookupSize][kLookupSize][kLookupSize][3];
		
		enum TransformType
		{
			kTransformType_None,
			kTransformType_Identity,
			kTransformType_ColorTemperature,
			kTransformType_Colorize,
			kTransformType_ColorGradient,
			kTransformType_ContrastBrightness,
			kTransformType_Gamma
		};
		
		struct TransformParams
		{
			TransformType transformType = kTransformType_None;
			float transformAmount = 1.f;
			
			float colorTemperature = 6500.f;
			
			float colorizeHue = 0.f;
			float colorizeSaturation = .5f;
			float colorizeLightness = .5f;
			
			struct ColorGradient
			{
				static const int kMaxTabs = 2;
				
				struct Tap
				{
					float luminance;
					float rgb[3];
				};
				
				Tap taps[kMaxTabs];
				int numTaps = 0;
				
				ColorGradient()
				{
					taps[0].luminance = 0.f;
					taps[0].rgb[0] = 0.f;
					taps[0].rgb[1] = 0.f;
					taps[0].rgb[2] = 0.f;
					
					taps[1].luminance = 1.f;
					taps[1].rgb[0] = 1.f;
					taps[1].rgb[1] = 1.f;
					taps[1].rgb[2] = 1.f;
					
					numTaps = 2;
				}
			};
			
			ColorGradient colorGradient;
			
			float contrast = 1.f;
			float brightness = 0.f;
			
			float gamma = 1.f;
		};
		
		TransformParams transformParams;
		
		float lookupTable[kLookupSize][kLookupSize][kLookupSize][3];
		
		char textureFilename[1024];
		char previewFilename[1024];
		
		ColorGradingEditor()
		{
			applyIdentity(lookupTable);
			
			textureFilename[0] = 0;
			previewFilename[0] = 0;
		}
		
		static void applyTransform(
			const TransformParams * transformParams,
			LookupTable src,
			LookupTable dst)
		{
			LookupTable temp;
			
			switch (transformParams->transformType)
			{
			case kTransformType_None:
				memcpy(temp, src, sizeof(LookupTable));
				break;
			
			case kTransformType_Identity:
				applyIdentity(temp);
				break;
				
			case kTransformType_ColorTemperature:
				applyColorTemperature(src, temp,
					transformParams->colorTemperature);
				break;
				
			case kTransformType_Colorize:
				applyColorize(src, temp,
					transformParams->colorizeHue,
					transformParams->colorizeSaturation,
					transformParams->colorizeLightness);
				break;
				
			case kTransformType_ColorGradient:
				applyColorGradient(src, temp,
					transformParams->colorGradient.taps,
					2);
				break;
				
			case kTransformType_ContrastBrightness:
				applyContrastBrightness(src, temp,
					transformParams->contrast,
					transformParams->brightness);
				break;
			
			case kTransformType_Gamma:
				applyGamma(src, temp, transformParams->gamma);
				break;
			}
			
			applyBlend(src, temp, dst, transformParams->transformAmount);
		}
		
		static void applyBlend(
			const LookupTable from,
			const LookupTable to,
			LookupTable dst,
			const float weight)
		{
			for (int x = 0; x < kLookupSize; ++x)
				for (int y = 0; y < kLookupSize; ++y)
					for (int z = 0; z < kLookupSize; ++z)
						for (int i = 0; i < 3; ++i)
							dst[x][y][z][i] =
								from[x][y][z][i] * (1.f - weight) +
								to[x][y][z][i] * weight;
		}
		
		static void applyIdentity(LookupTable lookupTable)
		{
			float value[16];
			for (int x = 0; x < kLookupSize; ++x)
				value[x] = x / (kLookupSize - 1.f);
			
			for (int x = 0; x < kLookupSize; ++x)
			{
				for (int y = 0; y < kLookupSize; ++y)
				{
					for (int z = 0; z < kLookupSize; ++z)
					{
						lookupTable[x][y][z][0] = value[z];
						lookupTable[x][y][z][1] = value[y];
						lookupTable[x][y][z][2] = value[x];
					}
				}
			}
		}
		
		static void colorTemperatureToRgb(float temperatureInKelvins, float * rgb)
		{
			temperatureInKelvins = fminf(fmaxf(temperatureInKelvins, 1000.f), 40000.f) / 100.f;
			
			if (temperatureInKelvins <= 66.f)
			{
				rgb[0] = 1.f;
				rgb[1] = saturate(0.39008157876901960784f * logf(temperatureInKelvins) - 0.63184144378862745098f);
			}
			else
			{
				float t = temperatureInKelvins - 60.f;
				rgb[0] = saturate(1.29293618606274509804 * powf(t, -0.1332047592f));
				rgb[1] = saturate(1.12989086089529411765 * powf(t, -0.0755148492f));
			}
			
			if (temperatureInKelvins >= 66.f)
				rgb[2] = 1.f;
			else if(temperatureInKelvins <= 19.f)
				rgb[2] = 0.f;
			else
				rgb[2] = saturate(0.54320678911019607843f * logf(temperatureInKelvins - 10.f) - 1.19625408914f);
		}
		
		static void applyColorTemperature(
			LookupTable src,
			LookupTable dst,
			const float temperatureInKelvins)
		{
			Color c;
			colorTemperatureToRgb(temperatureInKelvins, &c.r);
			
			for (int x = 0; x < kLookupSize; ++x)
			{
				for (int y = 0; y < kLookupSize; ++y)
				{
					for (int z = 0; z < kLookupSize; ++z)
					{
						dst[x][y][z][0] = src[x][y][z][0] * c.r;
						dst[x][y][z][1] = src[x][y][z][1] * c.g;
						dst[x][y][z][2] = src[x][y][z][2] * c.b;
					}
				}
			}
		}
		
		static void applyColorize(
			LookupTable src,
			LookupTable dst,
			const float hue,
			const float saturation,
			const float lightness)
		{
			const Color c = Color::fromHSL(hue, saturation, lightness);
			
			for (int x = 0; x < kLookupSize; ++x)
			{
				for (int y = 0; y < kLookupSize; ++y)
				{
					for (int z = 0; z < kLookupSize; ++z)
					{
						dst[x][y][z][0] = src[x][y][z][0] * c.r;
						dst[x][y][z][1] = src[x][y][z][1] * c.g;
						dst[x][y][z][2] = src[x][y][z][2] * c.b;
					}
				}
			}
		}
		
		static void applyColorGradient(
			LookupTable src,
			LookupTable dst,
			const TransformParams::ColorGradient::Tap * taps,
			const int numTaps)
		{
			const float wr = 1.f / 3.f;
			const float wg = 1.f / 3.f;
			const float wb = 1.f / 3.f;
			
			for (int x = 0; x < kLookupSize; ++x)
			{
				for (int y = 0; y < kLookupSize; ++y)
				{
					for (int z = 0; z < kLookupSize; ++z)
					{
						const float luminance =
							wr * src[x][y][z][0] +
							wg * src[x][y][z][1] +
							wb * src[x][y][z][2];
						
						int tapIndex = 0;
						
						while (tapIndex + 2 < numTaps && luminance > taps[tapIndex + 1].luminance)
						{
							tapIndex++;
						}
						
						auto lum1 = taps[tapIndex    ].luminance;
						auto lum2 = taps[tapIndex + 1].luminance;
						
						const float a = (lum2 - luminance) / (lum2 - lum1);
						const float b = 1.f - a;
						
						for (int i = 0; i < 3; ++i)
						{
							dst[x][y][z][i] =
								a * taps[tapIndex    ].rgb[i] +
								b * taps[tapIndex + 1].rgb[i];
						}
					}
				}
			}
		}
		
		static void applyContrastBrightness(
			LookupTable src,
			LookupTable dst,
			const float contrast,
			const float brightness)
		{
			for (int x = 0; x < kLookupSize; ++x)
			{
				for (int y = 0; y < kLookupSize; ++y)
				{
					for (int z = 0; z < kLookupSize; ++z)
					{
						dst[x][y][z][0] = (src[x][y][z][0] - .5f) * contrast + .5f + brightness;
						dst[x][y][z][1] = (src[x][y][z][1] - .5f) * contrast + .5f + brightness;
						dst[x][y][z][2] = (src[x][y][z][2] - .5f) * contrast + .5f + brightness;
					}
				}
			}
		}
		
		static void applyGamma(
			LookupTable src,
			LookupTable dst,
			const float gamma)
		{
			for (int x = 0; x < kLookupSize; ++x)
			{
				for (int y = 0; y < kLookupSize; ++y)
				{
					for (int z = 0; z < kLookupSize; ++z)
					{
						dst[x][y][z][0] = powf(src[x][y][z][0], gamma);
						dst[x][y][z][1] = powf(src[x][y][z][1], gamma);
						dst[x][y][z][2] = powf(src[x][y][z][2], gamma);
					}
				}
			}
		}
		
		bool Edit();
		void DrawPreview();
	};
}
