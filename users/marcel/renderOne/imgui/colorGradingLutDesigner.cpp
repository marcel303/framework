#include "colorGradingLutDesigner.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "nfd.h"

#include "framework.h"
#include "gx_texture.h"
#include "image.h"

#include "Path.h"

namespace ImGui
{
	ColorGradingEditor::ColorGradingEditor()
	{
		applyIdentity(lookupTable);
		
		textureFilename[0] = 0;
		previewFilename[0] = 0;
	}

	void ColorGradingEditor::applyTransform()
	{
		Assert(transformParams.transformType != kTransformType_None);
		
		UndoItem undoItem;
		memcpy(undoItem.lookupTable, lookupTable, sizeof(LookupTable));
		undoStack.push(undoItem);
		redoStack = std::stack<UndoItem>();

		applyTransform(
			transformParams,
			lookupTable,
			lookupTable);

		transformParams.transformType = kTransformType_None;
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
		for (int z = 0; z < kLookupSize; ++z)
			for (int y = 0; y < kLookupSize; ++y)
				for (int x = 0; x < kLookupSize; ++x)
					for (int i = 0; i < 3; ++i)
						dst[z][y][x][i] =
							from[z][y][x][i] * (1.f - weight) +
							to[z][y][x][i] * weight;
	}

	void ColorGradingEditor::applyIdentity(
		LookupTable lookupTable)
	{
		float value[16];
		for (int x = 0; x < kLookupSize; ++x)
			value[x] = x / (kLookupSize - 1.f);
		
		for (int z = 0; z < kLookupSize; ++z)
		{
			for (int y = 0; y < kLookupSize; ++y)
			{
				for (int x = 0; x < kLookupSize; ++x)
				{
					lookupTable[z][y][x][0] = value[x];
					lookupTable[z][y][x][1] = value[y];
					lookupTable[z][y][x][2] = value[z];
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
		
		for (int z = 0; z < kLookupSize; ++z)
		{
			for (int y = 0; y < kLookupSize; ++y)
			{
				for (int x = 0; x < kLookupSize; ++x)
				{
					dst[z][y][x][0] = src[z][y][x][0] * c.r;
					dst[z][y][x][1] = src[z][y][x][1] * c.g;
					dst[z][y][x][2] = src[z][y][x][2] * c.b;
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
		
		for (int z = 0; z < kLookupSize; ++z)
		{
			for (int y = 0; y < kLookupSize; ++y)
			{
				for (int x = 0; x < kLookupSize; ++x)
				{
					dst[z][y][x][0] = src[z][y][x][0] * c.r;
					dst[z][y][x][1] = src[z][y][x][1] * c.g;
					dst[z][y][x][2] = src[z][y][x][2] * c.b;
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
		
		for (int z = 0; z < kLookupSize; ++z)
		{
			for (int y = 0; y < kLookupSize; ++y)
			{
				for (int x = 0; x < kLookupSize; ++x)
				{
					const float luminance =
						wr * src[z][y][x][0] +
						wg * src[z][y][x][1] +
						wb * src[z][y][x][2];
					
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
						dst[z][y][x][i] =
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
		for (int z = 0; z < kLookupSize; ++z)
		{
			for (int y = 0; y < kLookupSize; ++y)
			{
				for (int x = 0; x < kLookupSize; ++x)
				{
					dst[z][y][x][0] = (src[z][y][x][0] - .5f) * contrast + .5f + brightness;
					dst[z][y][x][1] = (src[z][y][x][1] - .5f) * contrast + .5f + brightness;
					dst[z][y][x][2] = (src[z][y][x][2] - .5f) * contrast + .5f + brightness;
				}
			}
		}
	}

	void ColorGradingEditor::applyGamma(
		const LookupTable src,
		      LookupTable dst,
		const float gamma)
	{
		for (int z = 0; z < kLookupSize; ++z)
		{
			for (int y = 0; y < kLookupSize; ++y)
			{
				for (int x = 0; x < kLookupSize; ++x)
				{
					dst[z][y][x][0] = powf(fmaxf(0.f, src[z][y][x][0]), gamma);
					dst[z][y][x][1] = powf(fmaxf(0.f, src[z][y][x][1]), gamma);
					dst[z][y][x][2] = powf(fmaxf(0.f, src[z][y][x][2]), gamma);
				}
			}
		}
	}
	
	void ColorGradingEditor::Edit()
	{
		ImGui::Text("File: %s", Path::GetFileName(textureFilename).c_str());
		
		if (ImGui::Button("Load.."))
		{
			nfdchar_t * path = nullptr;
			
			auto result = NFD_OpenDialog(nullptr, nullptr, &path);
			
			if (result != NFD_CANCEL)
			{
				bool success = false;
				
				if (result == NFD_OKAY)
				{
					auto * image = loadImage(path);
					
					if (image != nullptr &&
						image->sx == kLookupSize * kLookupSize &&
						image->sy == kLookupSize)
					{
						for (int y = 0; y < kLookupSize; ++y)
						{
							auto * line = image->getLine(y);
							
							for (int z = 0; z < kLookupSize; ++z)
							{
								for (int x = 0; x < kLookupSize; ++x, ++line)
								{
									lookupTable[z][y][x][0] = line->r / 255.f;
									lookupTable[z][y][x][1] = line->g / 255.f;
									lookupTable[z][y][x][2] = line->b / 255.f;
								}
							}
						}
						
						strcpy(textureFilename, path);
						
						success = true;
					}
					
					delete image;
					image = nullptr;
				}
				
				if (success == false)
				{
					applyIdentity(lookupTable);
					
					textureFilename[0] = 0;
					
					ImGui::OpenPopup("LoadError_LoadImage");
				}
				
				// reset editing
				transformParams = TransformParams();
				undoStack = std::stack<UndoItem>();
				redoStack = std::stack<UndoItem>();
			}
			
			if (path != nullptr)
			{
				free(path);
				path = nullptr;
			}
		}
		
		if (ImGui::BeginPopupModal("LoadError_LoadImage", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Failed to load image");
			if (ImGui::Button("OK"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		
		bool save = false;
		
		if (textureFilename[0] != 0)
		{
			ImGui::SameLine();
			if (ImGui::Button("Save"))
			{
				save = true;
			}
		}
		
		ImGui::SameLine();
		if (ImGui::Button("Save as.."))
		{
			nfdchar_t * path = nullptr;
			
			auto result = NFD_SaveDialog(nullptr, textureFilename, &path);
			
			if (result != NFD_CANCEL)
			{
				if (result == NFD_OKAY)
				{
					strcpy(textureFilename, path);
					save = true;
				}
				else
				{
					ImGui::OpenPopup("SaveError_Filename");
				}
			}
			
			if (path != nullptr)
			{
				free(path);
				path = nullptr;
			}
		}
		
		if (save)
		{
			if (transformParams.transformType != kTransformType_None)
			{
				applyTransform();
			}
			
			ImageData image;
			image.sx = kLookupSize * kLookupSize;
			image.sy = kLookupSize;
			image.imageData = new ImageData::Pixel[image.sx * image.sy];
			
			for (int y = 0; y < image.sy; ++y)
			{
				ImageData::Pixel * line = image.getLine(y);
				
				const int ly = y;
				
				for (int x = 0; x < image.sx; ++x)
				{
					const int lx = x % kLookupSize;
					const int lz = x / kLookupSize;
					
					line[x].r = int(clamp(lookupTable[lz][ly][lx][0], 0.f, 1.f) * 255.f);
					line[x].g = int(clamp(lookupTable[lz][ly][lx][1], 0.f, 1.f) * 255.f);
					line[x].b = int(clamp(lookupTable[lz][ly][lx][2], 0.f, 1.f) * 255.f);
					line[x].a = 255;
				}
			}
			
			if (saveImage(&image, textureFilename) == false)
			{
				ImGui::OpenPopup("SaveError_SaveImage");
			}
		}
		
		if (ImGui::BeginPopupModal("SaveError_Filename", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Failed to select file for save");
			if (ImGui::Button("OK"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		
		if (ImGui::BeginPopupModal("SaveError_SaveImage", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Failed to save image");
			if (ImGui::Button("OK"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, undoStack.empty());
		{
			if (ImGui::Button("Undo"))
			{
				UndoItem redoItem;
				memcpy(redoItem.lookupTable, lookupTable, sizeof(LookupTable));
				redoStack.push(redoItem);

				auto & undoItem = undoStack.top();
				memcpy(lookupTable, undoItem.lookupTable, sizeof(LookupTable));
				undoStack.pop();
			}
		}
		ImGui::PopItemFlag();
		
		ImGui::SameLine();
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, redoStack.empty());
		{
			if (ImGui::Button("Redo"))
			{
				UndoItem undoItem;
				memcpy(undoItem.lookupTable, lookupTable, sizeof(LookupTable));
				undoStack.push(undoItem);
				
				auto & redoItem = redoStack.top();;
				memcpy(lookupTable, redoItem.lookupTable, sizeof(LookupTable));
				redoStack.pop();
			}
		}
		ImGui::PopItemFlag();
		
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
			transformParams = TransformParams();
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
				applyTransform();
			}
			
			ImGui::SameLine();
			if (Button("Cancel"))
				transformParams.transformType = kTransformType_None;
		}
	}
	
	void ColorGradingEditor::DrawPreview(
		const int sx,
		const int sy)
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
			setColor(colorWhite);
			gxSetTexture(getTexture(previewFilename));
			drawRect(0, 0, sx/2, sy/2);
			gxSetTexture(0);
			
			Shader shader("renderOne/postprocess/color-grade");
			setShader(shader);
			{
				shader.setTexture("colorTexture", 0, getTexture(previewFilename), true);

				shader.setTexture3d("lutTexture", 1, currentTexture.id, true, true);
				drawRect(0, sy/2, sx/2, sy);
				
				shader.setTexture3d("lutTexture", 1, previewTexture.id, true, true);
				drawRect(sx/2, sy/2, sx, sy);
			}
			clearShader();
		}
		popBlend();
		
		currentTexture.free();
		previewTexture.free();
	}
}
