#include "fileEditor_gltf.h"
#include "gltf-draw.h"
#include "gltf-loader.h"
#include "reflection.h"
#include "ui.h" // drawUiRectCheckered

// todo : add option : draw mode (lit, colors, normals, specular, roughness, metallic, glossiness, ..)

static const char * s_gltfWireframePs = R"SHADER(

	include engine/ShaderPS.txt

	uniform vec4 color;

	void main()
	{
		shader_fragColor = color;
	}

)SHADER";

FileEditor_Gltf::FileEditor_Gltf(const char * path)
{
	gltf::loadScene(path, scene);
	bufferCache.init(scene);
	
	shaderSource("refine/gltfWireframe.ps", s_gltfWireframePs);

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
	type.add("scale", &FileEditor_Gltf::desiredScale);
	
	typeDB.addEnum<gltf::AlphaMode>("gltf::AlphaMode")
		.add("alphaBlend", gltf::kAlphaMode_AlphaBlend)
		.add("alphaToCoverage", gltf::kAlphaMode_AlphaToCoverage);
	type.add("alphaMode", &FileEditor_Gltf::alphaMode);
	type.add("sortDrawablesByViewDistance", &FileEditor_Gltf::sortDrawablesByViewDistance);
	type.add("enableBufferCache", &FileEditor_Gltf::enableBufferCache);
	type.add("wireframe", &FileEditor_Gltf::wireframe);
	type.add("wireframeColor", &FileEditor_Gltf::wireframeColor);
	
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
			
			if (ImGui::CollapsingHeader("Draw"))
			{
				int alpha_item = alphaMode == gltf::kAlphaMode_AlphaBlend ? 0 : 1;
				const char * alpha_items[2] = { "Alphe blend", "Alpha to coverage" };
				ImGui::Combo("Alpha mode", &alpha_item, alpha_items, 2);
				alphaMode = alpha_item == 0 ? gltf::kAlphaMode_AlphaBlend : gltf::kAlphaMode_AlphaToCoverage;
				
				ImGui::Checkbox("Sort drawables by view distance", &sortDrawablesByViewDistance);
				ImGui::Checkbox("Use buffer cache", &enableBufferCache);
				ImGui::Checkbox("Wireframe", &wireframe);
				ImGui::ColorEdit4("Wireframe color", &wireframeColor[0]);
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
	
	pushBlend(BLEND_OPAQUE);
	setColor(colorWhite);
	drawUiRectCheckered(0, 0, sx, sy, 8);
	popBlend();
	
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
			gxPushMatrix();
			{
				gxTranslatef(0, 0, 2.f);
				gxRotatef(rotationX, 1.f, 0.f, 0.f);
				gxRotatef(rotationY, 0.f, 1.f, 0.f);
				
				// draw axis helper
				
				if (showAxis)
				{
					pushDepthTest(true, DEPTH_LESS);
					pushBlend(BLEND_OPAQUE);
					{
						setColor(colorRed);
						drawLine3d(0);
						
						setColor(colorGreen);
						drawLine3d(1);
						
						setColor(colorBlue);
						drawLine3d(2);
					}
					popBlend();
					popDepthTest();
				}
		
				// automatically fit to bounding box
				
				gxScalef(1.f / maxAxis, 1.f / maxAxis, 1.f / maxAxis);
				gxScalef(currentScale, currentScale, currentScale);
				gxScalef(1, 1, -1); // apply scale (1, 1, -1) at the scene draw & minmax level
				gxTranslatef(-mid[0], -mid[1], -mid[2]);
				
				if (showBoundingBox)
				{
					pushDepthTest(true, DEPTH_LESS);
					pushBlend(BLEND_OPAQUE);
					{
						setColor(colorWhite);
						lineCube(mid, extents);
					}
					popBlend();
					popDepthTest();
				}
				
				// draw model
				
				Shader metallicRoughnessShader("gltf/shaders/shader-pbr-metallicRoughness");
				Shader specularGlossinessShader("gltf/shaders/shader-pbr-specularGlossiness");
				
			// todo : add a nicer way to set lighting for the GLTF library's built-in shaders
				Shader * shaders[2] = { &metallicRoughnessShader, &specularGlossinessShader };
				for (auto * shader : shaders)
				{
					setShader(*shader);
					
					Mat4x4 objectToView;
					gxGetMatrixf(GX_MODELVIEW, objectToView.m_v);
					
					const float dx = cosf(framework.time / 1.56f);
					const float dz = sinf(framework.time / 1.67f);
					const Vec3 lightDir_world(dx, -2.f, dz);
					const Vec3 lightDir_view = objectToView.Mul3(lightDir_world).CalcNormalized();
			
					shader->setImmediate("scene_lightParams1",
						lightDir_view[0],
						lightDir_view[1],
						lightDir_view[2],
						0.f /* directional */);
					shader->setImmediate("scene_lightParams2",
						1.f, 1.f, 1.f,
						2.f);
					shader->setImmediate("scene_ambientLightColor", .2f, .2f, .2f);
					
					clearShader();
				}
				
				gltf::MaterialShaders materialShaders;
				materialShaders.pbr_specularGlossiness = &specularGlossinessShader;
				materialShaders.pbr_metallicRoughness = &metallicRoughnessShader;
				materialShaders.fallbackShader = &metallicRoughnessShader;
				
				gltf::DrawOptions drawOptions;
				drawOptions.alphaMode = alphaMode;
				drawOptions.sortPrimitivesByViewDistance = sortDrawablesByViewDistance;
				
				pushDepthTest(true, DEPTH_LESS);
				pushBlend(BLEND_OPAQUE);
				{
					gltf::drawScene(
						scene,
						enableBufferCache
						? &bufferCache
						: nullptr,
						materialShaders,
						true,
						scene.activeScene,
						&drawOptions);
				}
				popBlend();
				popDepthTest();
				
				pushDepthTest(true, DEPTH_LESS);
				pushBlend(BLEND_ALPHA);
				{
					gltf::drawScene(
						scene,
						enableBufferCache
						? &bufferCache
						: nullptr,
						materialShaders,
						false,
						scene.activeScene,
						&drawOptions);
				}
				popBlend();
				popDepthTest();
				
				if (wireframe)
				{
					setDepthBias(-1, -1);
					
					pushDepthTest(true, DEPTH_LEQUAL, false);
					pushBlend(BLEND_ALPHA);
					pushWireframe(true);
					{
						Shader shader("refine/gltfWireframe", "engine/Generic.vs", "refine/gltfWireframe.ps");
						setShader(shader);
						shader.setImmediate("color", wireframeColor[0], wireframeColor[1], wireframeColor[2], wireframeColor[3]);
						clearShader();
						
						gltf::MaterialShaders materialShaders;
						materialShaders.fallbackShader = &shader;
						
						gltf::drawScene(
							scene,
							enableBufferCache
							? &bufferCache
							: nullptr,
							materialShaders,
							true,
							scene.activeScene);
						
						gltf::drawScene(
							scene,
							enableBufferCache
							? &bufferCache
							: nullptr,
							materialShaders,
							false,
							scene.activeScene);
					}
					popWireframe();
					popBlend();
					popDepthTest();
					
					setDepthBias(0, 0);
				}
			}
			gxPopMatrix();
		}
	}
	projectScreen2d();
	
	guiContext.draw();
}
