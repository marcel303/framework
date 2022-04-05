#include "fileEditor_ies.h"
#include "ies_loader.h"
#include "reflection.h"
#include "TextIO.h"
#include "ui.h"

static const int kPreviewResolution = 512;
static const int kLookupResolution = 256;

FileEditor_Ies::FileEditor_Ies(const char * path)
{
	guiContext.init();

	//
	
	camera.position[2] = -.5f;
	camera.position[1] = .5f;
	
	//

	char * text = nullptr;
	size_t size;
	if (TextIO::loadFileContents(path, text, size))
	{
		IESLoadHelper helper;
		IESFileInfo info;
		
		if (helper.load(text, size, info))
		{
			{
				uint8_t * data = new uint8_t[kPreviewResolution * kPreviewResolution];
				
				if (helper.saveAsPreview(info, data, kPreviewResolution, kPreviewResolution, 1))
				{
					freeTexture(previewTexture);
					
					previewTexture = createTextureFromR8(data, kPreviewResolution, kPreviewResolution, true, true);
				}
				
				delete [] data;
				data = nullptr;
			}
			
			{
				float * data = new float[kLookupResolution * kLookupResolution];
	
				if (helper.saveAs2D(info, data, kLookupResolution, kLookupResolution, 1))
				{
					freeTexture(lookupTexture);
					
					lookupTexture = createTextureFromR32F(data, kLookupResolution, kLookupResolution, true, true);
				}
				
				delete [] data;
				data = nullptr;
			}
		}
		
		delete [] text;
		text = nullptr;
	}
}

FileEditor_Ies::~FileEditor_Ies()
{
	freeTexture(previewTexture);
	freeTexture(lookupTexture);
	
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
				ImGui::Checkbox("Explore in 3d", &exploreIn3d);
			}
			ImGui::PopItemWidth();
		}
		ImGui::End();
	}
	guiContext.processEnd();

	if (exploreIn3d)
	{
		camera.tick(dt, inputIsCaptured == false && mouse.isDown(BUTTON_LEFT));
	}

	// draw
	
	clearSurface(0, 0, 0, 0);
	
	if (exploreIn3d)
	{
		const Vec3 lightPosition_world = Vec3(0, 1, 0);
		const Mat4x4 viewToWorld = camera.getWorldMatrix();
	
	#if true
		projectPerspective3d(90.f, .01f, 100.f);

		camera.pushViewMatrix();
		pushBlend(BLEND_OPAQUE);
		{
			if (lookupTexture != 0)
			{
				Shader shader("fileEditor_ies/ies-light");
				setShader(shader);
				{
					shader.setTexture("ies_texture", 0, lookupTexture, true, true);
					shader.setImmediateMatrix4x4("viewToWorld", viewToWorld.m_v);
					shader.setImmediate("lightPosition_world",
						lightPosition_world[0],
						lightPosition_world[1],
						lightPosition_world[2]);
					
					gxPushMatrix();
					{
						gxScalef(4, 4, 4);
						
						setColor(colorWhite);
						drawGrid3dLine(100, 100, 0, 2);
					}
					gxPopMatrix();
				}
				clearShader();
			}
		}
		popBlend();
		camera.popViewMatrix();
	#endif
	
		projectScreen2d();
	
		pushBlend(BLEND_ADD);
		{
			if (lookupTexture != 0)
			{
				Shader shader("fileEditor_ies/ies-light-volume");
				setShader(shader);
				{
					shader.setTexture("ies_texture", 0, lookupTexture, true, true);
					shader.setImmediateMatrix4x4("viewToWorld", viewToWorld.m_v);
					shader.setImmediate("lightPosition_world",
						lightPosition_world[0],
						lightPosition_world[1],
						lightPosition_world[2]);
					
					drawRect(0, 0, sx, sy);
				}
				clearShader();
			}
		}
		popBlend();
	}
	else
	{
		pushBlend(BLEND_OPAQUE);
		
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		if (previewTexture != 0)
		{
			const float scaleX = sx / float(kPreviewResolution);
			const float scaleY = sy / float(kPreviewResolution);
			const float scale = fminf(scaleX, scaleY);
			const float x = (sx - scale * kPreviewResolution) / 2.f;
			const float y = (sy - scale * kPreviewResolution) / 2.f;
			gxSetTexture(previewTexture, GX_SAMPLE_LINEAR, true);
			pushColorPost(POST_SET_RGB_TO_R);
			setColor(colorWhite);
			drawRect(
				x,
				y,
				x + kPreviewResolution * scale,
				y + kPreviewResolution * scale);
			popColorPost();
			gxClearTexture();
		}
		
		popBlend();
	}
	
	guiContext.draw();
}
