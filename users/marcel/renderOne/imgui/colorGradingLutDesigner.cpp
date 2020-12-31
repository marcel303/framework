#include "colorGradingLutDesigner.h"
#include "imgui.h"

#include "framework.h"
#include "gx_texture.h"

namespace ImGui
{
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

		ImGui::SameLine();
		if (Button("Reset all"))
		{
			applyIdentity(lookupTable);
			
			transformParams.transformType = kTransformType_None;
			result = true;
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
					&transformParams,
					lookupTable,
					lookupTable);
				
				transformParams.transformType = kTransformType_None;
				result = true;
			}
		}
		
	// todo : show preview image, before and after
		
		return result;
	}
	
	void ColorGradingEditor::DrawPreview()
	{
		LookupTable previewLookupTable;
		
		applyTransform(
			&transformParams,
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
