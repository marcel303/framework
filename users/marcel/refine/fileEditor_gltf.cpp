#include "fileEditor_gltf.h"
#include "gltf-draw.h"
#include "gltf-loader.h"
#include "reflection.h"
#include "ui.h" // drawUiRectCheckered

FileEditor_Gltf::FileEditor_Gltf(const char * path)
{
	gltf::loadScene(path, scene);
	bufferCache.init(scene);
	
	guiContext.init();
}

FileEditor_Gltf::~FileEditor_Gltf()
{
	guiContext.shut();
}

bool FileEditor_Gltf::reflect(TypeDB & typeDB, StructuredType & type)
{
	type.add("showBoundingBox", &FileEditor_Gltf::showBoundingBox);
	type.add("showAxis", &FileEditor_Gltf::showAxis);
	type.add("enableLighting", &FileEditor_Gltf::enableLighting);
	type.add("scale", &FileEditor_Gltf::desiredScale);
	
	return true;
}

void FileEditor_Gltf::tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured)
{
// todo : always tick. return true if redraw is desired. create separate FileEditor method for updating the visual

	if (hasFocus == false)
		return;
	
	// update smoothed changes
	
	const float retainAnimTick = .8f;
	const float retainThisTick = powf(retainAnimTick, dt * 100.f);

	currentScale = lerp<float>(currentScale, desiredScale, 1.f - retainThisTick);
	
	guiContext.processBegin(dt, sx, sy, inputIsCaptured);
	{
		ImGui::SetNextWindowPos(ImVec2(8, 8), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(.6f);
		if (ImGui::Begin("Model", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::PushItemWidth(120.f);
			
			if (ImGui::CollapsingHeader("Visibility"))
			{
				ImGui::Checkbox("Show bounding box", &showBoundingBox);
				ImGui::Checkbox("Show axis", &showAxis);
			}
			ImGui::SliderFloat("Scale", &desiredScale, 0.f, 4.f, "%.2f", 2.f);
			
			ImGui::PopItemWidth();
		}
		ImGui::End();
	}
	guiContext.processEnd();

	// process input
	
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_0))
	{
		inputIsCaptured = true;
		desiredScale = 1.f;
		rotationX = 0.f;
		rotationY = 0.f;
	}
	
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_EQUALS, true))
	{
		inputIsCaptured = true;
		desiredScale *= 1.5f;
	}
	
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_MINUS, true))
	{
		inputIsCaptured = true;
		desiredScale /= 1.5f;
	}
	
	if (inputIsCaptured == false && mouse.scrollY != 0)
	{
		inputIsCaptured = true;
		desiredScale *= powf(1.2f, mouse.scrollY);
	}
	
	if (inputIsCaptured == false && mouse.isDown(BUTTON_LEFT))
	{
		inputIsCaptured = true;

		rotationX -= mouse.dy;
		rotationY -= mouse.dx;
	}
	
	// draw
	
	clearSurface(0, 0, 0, 0);
	
	setColor(colorWhite);
	drawUiRectCheckered(0, 0, sx, sy, 8);
	
	projectPerspective3d(60.f, .01f, 100.f);
	{
		gltf::BoundingBox bb;
		gltf::calculateSceneMinMaxTraverse(scene, scene.activeScene, bb);
		
		const Vec3 mid = (bb.min + bb.max) / 2.f;
		const Vec3 extents = (bb.max - bb.min) / 2.f;
		
		float maxAxis = 0.f;
		for (int i = 0; i < 3; ++i)
			maxAxis = fmaxf(maxAxis, extents[i]);
		
		if (maxAxis > 0.f)
		{
			Shader metallicRoughnessShader("gltf/shaders/shader-pbr-metallicRoughness");
			Shader specularGlossinessShader("gltf/shaders/shader-pbr-specularGlossiness");
			
		// todo : add a nicer way to set lighting for the GLTF library's built-in shaders
			Shader * shaders[2] = { &metallicRoughnessShader, &specularGlossinessShader };
			for (auto * shader : shaders)
			{
				setShader(*shader);
				shader->setImmediate("scene_camPos",
					0.f,
					0.f,
					0.f);
				
				Mat4x4 objectToView;
				gxGetMatrixf(GX_MODELVIEW, objectToView.m_v);
				
				const float dx = cosf(framework.time / 1.56f);
				const float dz = sinf(framework.time / 1.67f);
				const Vec3 lightDir_world(dx, 0.f, dz);
				const Vec3 lightDir_view = objectToView.Mul3(lightDir_world);
		
				shader->setImmediate("scene_lightDir",
					lightDir_view[0],
					lightDir_view[1],
					lightDir_view[2]);
				
				clearShader();
			}
			
			gltf::MaterialShaders materialShaders;
			materialShaders.pbr_specularGlossiness = &specularGlossinessShader;
			materialShaders.pbr_metallicRoughness = &metallicRoughnessShader;
			materialShaders.fallbackShader = &metallicRoughnessShader;
			
			gxPushMatrix();
			{
				gxTranslatef(0, 0, 2.f);
				gxRotatef(rotationX, 1.f, 0.f, 0.f);
				gxRotatef(rotationY, 0.f, 1.f, 0.f);
				
				// draw axis helper
				
				if (showAxis)
				{
					setColor(colorRed);
					drawLine3d(0);
					
					setColor(colorGreen);
					drawLine3d(1);
					
					setColor(colorBlue);
					drawLine3d(2);
				}
		
				// automatically fit to bounding box
				
				gxScalef(1.f / maxAxis, 1.f / maxAxis, 1.f / maxAxis);
				gxScalef(currentScale, currentScale, currentScale);
				gxScalef(-1, 1, 1); // apply scale (-1, 1, 1) at the scene draw & minmax level
				gxTranslatef(-mid[0], -mid[1], -mid[2]);
				
				if (showBoundingBox)
				{
					setColor(colorWhite);
					lineCube(mid, extents);
				}
				
				// draw model
				
				pushDepthTest(true, DEPTH_LESS);
				pushBlend(BLEND_OPAQUE);
				{
					gltf::drawScene(scene, &bufferCache, materialShaders, true);
				}
				popBlend();
				popDepthTest();
				
				pushDepthTest(true, DEPTH_LESS);
				pushBlend(BLEND_ALPHA);
				{
					gltf::drawScene(scene, &bufferCache, materialShaders, false);
				}
				popBlend();
				popDepthTest();
			}
			gxPopMatrix();
		}
	}
	projectScreen2d();
	
	guiContext.draw();
}
