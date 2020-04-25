#include "fileEditor_ies.h"
#include "ies_loader.h"
#include "reflection.h"
#include "TextIO.h"
#include "ui.h"

static const int kPreviewResolution = 512;

FileEditor_Ies::FileEditor_Ies(const char * path)
{
	guiContext.init();

	//

	char * text = nullptr;
	size_t size;
	if (TextIO::loadFileContents(path, text, size))
	{
		IESLoadHelper helper;
		IESFileInfo info;
		
		if (helper.load(text, size, info))
		{
			uint8_t * data = new uint8_t[kPreviewResolution * kPreviewResolution];
			
			if (helper.saveAsPreview(info, data, kPreviewResolution, kPreviewResolution, 1))
			{
				freeTexture(texture);
				
				texture = createTextureFromR8(data, kPreviewResolution, kPreviewResolution, true, true);
			}
			
			delete [] data;
			data = nullptr;
		}
		
		delete [] text;
		text = nullptr;
	}
}

FileEditor_Ies::~FileEditor_Ies()
{
	freeTexture(texture);
	
	guiContext.shut();
}
	
bool FileEditor_Ies::reflect(TypeDB & typeDB, StructuredType & type)
{
	type.add("exploreIn3d", &FileEditor_Ies::exploreIn3d);
	
	return true;
}

void FileEditor_Ies::tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured)
{
	if (hasFocus == false)
		return;

	guiContext.processBegin(dt, sx, sy, inputIsCaptured);
	{
		ImGui::SetNextWindowPos(ImVec2(8, 8), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(.6f);
		if (ImGui::Begin("IES Light Profile", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::PushItemWidth(120.f);
			{
				//ImGui::Checkbox("Explore in 3d", &exploreIn3d);
			}
			ImGui::PopItemWidth();
		}
		ImGui::End();
	}
	guiContext.processEnd();

	camera.tick(dt, inputIsCaptured == false);

	// draw
	
	clearSurface(0, 0, 0, 0);
	
	setColor(colorWhite);
	drawUiRectCheckered(0, 0, sx, sy, 8);
	
	if (texture != 0)
	{
		const float scaleX = sx / float(kPreviewResolution);
		const float scaleY = sy / float(kPreviewResolution);
		const float scale = fminf(scaleX, scaleY);
		const float x = (sx - scale * kPreviewResolution) / 2.f;
		const float y = (sy - scale * kPreviewResolution) / 2.f;
		gxSetTexture(texture);
		gxSetTextureSampler(GX_SAMPLE_LINEAR, true);
		pushColorPost(POST_SET_RGB_TO_R);
		setColor(colorWhite);
		drawRect(
			x,
			y,
			x + kPreviewResolution * scale,
			y + kPreviewResolution * scale);
		popColorPost();
		gxSetTextureSampler(GX_SAMPLE_NEAREST, false);
		gxSetTexture(0);
	}
	
	guiContext.draw();
}
