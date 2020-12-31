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
		
		float lookupTable[kLookupSize][kLookupSize][kLookupSize][3];
		
		char textureFilename[1024];
		char previewFilename[1024];
		
		TransformParams transformParams;
		
		ColorGradingEditor();
		
		static void applyTransform(
			const TransformParams & transformParams,
			const LookupTable src,
			      LookupTable dst);
		
		static void applyBlend(
			const LookupTable from,
			const LookupTable to,
			      LookupTable dst,
			const float weight);
		
		static void applyIdentity(
			LookupTable lookupTable);
		
		static void colorTemperatureToRgb(
			const float temperatureInKelvins,
			float * rgb);
		
		static void applyColorTemperature(
			const LookupTable src,
			      LookupTable dst,
			const float temperatureInKelvins);
		
		static void applyColorize(
			const LookupTable src,
			      LookupTable dst,
			const float hue,
			const float saturation,
			const float lightness);
		
		static void applyColorGradient(
			const LookupTable src,
			      LookupTable dst,
			const TransformParams::ColorGradient::Tap * taps,
			const int numTaps);
		
		static void applyContrastBrightness(
			const LookupTable src,
			      LookupTable dst,
			const float contrast,
			const float brightness);
		
		static void applyGamma(
			const LookupTable src,
			      LookupTable dst,
			const float gamma);
		
		bool Edit();
		
		void DrawPreview();
	};
}
