#include "colorGradingLutDesigner.h"
#include "imgui.h"

#include "framework.h"
#include "gx_texture.h"

namespace ImGui
{
	ColorGradingEditor::ColorGradingEditor()
	{
		applyIdentity(lookupTable);
		
		textureFilename[0] = 0;
		previewFilename[0] = 0;
	}

	void ColorGradingEditor::applyTransform(
		const TransformParams & transformParams,
		const LookupTable src,
		      LookupTable dst)
	{
		LookupTable temp;
		
		switch (transformParams.transformType)
		{
		case kTransformType_None:
			memcpy(temp, src, sizeof(LookupTable));
			break;
		
		case kTransformType_Identity:
			applyIdentity(temp);
			break;
			
		case kTransformType_ColorTemperature:
			applyColorTemperature(src, temp,
				transformParams.colorTemperature);
			break;
			
		case kTransformType_Colorize:
			applyColorize(src, temp,
				transformParams.colorizeHue,
				transformParams.colorizeSaturation,
				transformParams.colorizeLightness);
			break;
			
		case kTransformType_ColorGradient:
			applyColorGradient(src, temp,
				transformParams.colorGradient.taps,
				2);
			break;
			
		case kTransformType_ContrastBrightness:
			applyContrastBrightness(src, temp,
				transformParams.contrast,
				transformParams.brightness);
			break;
		
		case kTransformType_Gamma:
			applyGamma(src, temp,
				transformParams.gamma);
			break;
		}
		
		applyBlend(src, temp, dst,
			transformParams.transformAmount);
	}

	void ColorGradingEditor::applyBlend(
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

	void ColorGradingEditor::applyIdentity(
		LookupTable lookupTable)
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
					// note : [x][y][z] actually index into the
					//        [z][y][x] pixels of the 3d lookup
					//        texture. this 'technicality' is not
					//        so important for the per-pixel
					//        operations performed by other
					//        transforms, but for the identity
					//        transforms we need to map the
					//        right pixel to the right color,
					//        hence the mapping indexing here
					//        looks a little bit weird
					
					lookupTable[x][y][z][0] = value[z];
					lookupTable[x][y][z][1] = value[y];
					lookupTable[x][y][z][2] = value[x];
				}
			}
		}
	}

	void ColorGradingEditor::colorTemperatureToRgb(
		const float in_temperatureInKelvins, float * rgb)
	{
		const float temperatureInKelvins = fminf(fmaxf(in_temperatureInKelvins, 1000.f), 40000.f) / 100.f;
		
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

	void ColorGradingEditor::applyColorTemperature(
		const LookupTable src,
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

	void ColorGradingEditor::applyColorize(
		const LookupTable src,
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

	void ColorGradingEditor::applyColorGradient(
		const LookupTable src,
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

	void ColorGradingEditor::applyContrastBrightness(
		const LookupTable src,
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

	void ColorGradingEditor::applyGamma(
		const LookupTable src,
		      LookupTable dst,
		const float gamma)
	{
		for (int x = 0; x < kLookupSize; ++x)
		{
			for (int y = 0; y < kLookupSize; ++y)
			{
				for (int z = 0; z < kLookupSize; ++z)
				{
					dst[x][y][z][0] = powf(fmaxf(0.f, src[x][y][z][0]), gamma);
					dst[x][y][z][1] = powf(fmaxf(0.f, src[x][y][z][1]), gamma);
					dst[x][y][z][2] = powf(fmaxf(0.f, src[x][y][z][2]), gamma);
				}
			}
		}
	}
	
	bool ColorGradingEditor::Edit()
	{
		// todo : add import & export option
		// todo : add a Help Text about color grading
		
		bool result = false;
		
		const char * transform_items[] =
		{
			"(Select)",
			"Identity",
			"Color temperature",
			"Colorize",
			"Color gradient",
			"Contrast & Brightness",
			"Gamma"
		};
		
		int transform_index = transformParams.transformType;
		
		if (ImGui::Combo("Transform", &transform_index, transform_items, sizeof(transform_items) / sizeof(transform_items[0])))
		{
			transformParams.transformType = (TransformType)transform_index;
		}
		
		if (transformParams.transformType == kTransformType_Identity)
		{
		}
		
		if (transformParams.transformType == kTransformType_ColorTemperature)
		{
			ImGui::SliderFloat("Temperature (K)", &transformParams.colorTemperature, 0.f, 10000.f);
		}
		
		if (transformParams.transformType == kTransformType_Colorize)
		{
			ImGui::SliderFloat("Hue", &transformParams.colorizeHue, 0.f, 1.f);
			ImGui::SliderFloat("Saturation", &transformParams.colorizeSaturation, 0.f, 1.f);
			ImGui::SliderFloat("Lightness", &transformParams.colorizeLightness, 0.f, 1.f);
		}
		
		if (transformParams.transformType == kTransformType_ColorGradient)
		{
			ImGui::ColorEdit3("Color 1", transformParams.colorGradient.taps[0].rgb);
			ImGui::ColorEdit3("Color 2", transformParams.colorGradient.taps[1].rgb);
		}

		if (transformParams.transformType == kTransformType_ContrastBrightness)
		{
			ImGui::SliderFloat("Contrast", &transformParams.contrast, 0.f, 4.f);
			ImGui::SliderFloat("Brightness", &transformParams.brightness, -1.f, 1.f);
		}
		
		if (transformParams.transformType == kTransformType_Gamma)
		{
			ImGui::SliderFloat("Gamma", &transformParams.gamma, 0.f, 3.f);
		}
		
		if (transformParams.transformType != kTransformType_None)
		{
			ImGui::SliderFloat("Amount", &transformParams.transformAmount, 0.f, 1.f);
			
			if (Button("Apply"))
			{
				applyTransform(
					transformParams,
					lookupTable,
					lookupTable);
				
				transformParams.transformType = kTransformType_None;
				result = true;
			}
		}
		
		return result;
	}
	
	void ColorGradingEditor::DrawPreview()
	{
		LookupTable previewLookupTable;
		
		applyTransform(
			transformParams,
			lookupTable,
			previewLookupTable);
		
		GxTexture3d currentTexture;
		currentTexture.allocate(
			kLookupSize,
			kLookupSize,
			kLookupSize,
			GX_RGB32_FLOAT);
		currentTexture.upload(lookupTable, 4, 0);
		
		GxTexture3d previewTexture;
		previewTexture.allocate(
			kLookupSize,
			kLookupSize,
			kLookupSize,
			GX_RGB32_FLOAT);
		previewTexture.upload(previewLookupTable, 4, 0);
		
		pushBlend(BLEND_OPAQUE);
		{
			Shader shader("renderOne/postprocess/color-grade");
			setShader(shader);
			{
				shader.setTexture("colorTexture", 0, getTexture(previewFilename), true);

				shader.setTexture3d("lutTexture", 1, currentTexture.id, true, true);
				drawRect(0, 0, 400, 400);
				
				shader.setTexture3d("lutTexture", 1, previewTexture.id, true, true);
				drawRect(400, 0, 800, 400);
			}
			clearShader();
		}
		popBlend();
		
		currentTexture.free();
		previewTexture.free();
	}
}
