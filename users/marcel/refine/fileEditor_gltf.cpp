#include "fileEditor_gltf.h"
#include "gltf-draw.h"
#include "gltf-loader.h"
#include "reflection.h"
#include "ui.h" // drawUiRectCheckered

static const char * s_gltfBasicSkinnedWithLightingPs = R"SHADER(

	include engine/ShaderPS.txt

	uniform vec3 ambientLight_color;
	uniform vec3 directionalLight_color;
	uniform vec3 directionalLight_direction;

	shader_in vec4 v_color;
	shader_in vec3 v_normal;

	void main()
	{
		vec3 total_light_color = ambientLight_color;
		
		{
			float intensity = max(0.0, dot(directionalLight_direction, v_normal));
			total_light_color += directionalLight_color * intensity;
		}
		
		// todo : use PBR shaders
		
		vec3 color = total_light_color;
		float opacity = .5f;
		
		shader_fragColor = vec4(color, opacity);
		shader_fragNormal = vec4(v_normal, 0.0);
	}

)SHADER";

//

FileEditor_Gltf::FileEditor_Gltf(const char * path)
{
	gltf::loadScene(path, scene);
	bufferCache.init(scene);
	
	shaderSource("gltf/BasicSkinnedWithLighting.ps", s_gltfBasicSkinnedWithLightingPs);
	
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
	type.add("ambientLight_color", &FileEditor_Gltf::ambientLight_color);
	type.add("directionalLight_intensity", &FileEditor_Gltf::directionalLight_intensity);
	type.add("directionalLight_color", &FileEditor_Gltf::directionalLight_color);
	type.add("directionalLight_direction", &FileEditor_Gltf::directionalLight_direction);
	
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
			ImGui::Checkbox("Enable lighting", &enableLighting);
			if (ImGui::CollapsingHeader("Lighting"))
			{
				ImGui::PushID("Ambient light");
				ImGui::Text("Ambient light");
				ImGui::ColorEdit3("Color", &ambientLight_color[0]);
				ImGui::PopID();
				
				ImGui::PushID("Directional light");
				ImGui::Text("Directional light");
				ImGui::SliderFloat("Intensity", &directionalLight_intensity, 0.f, 10.f);
				ImGui::ColorEdit3("Color", &directionalLight_color[0]);
				ImGui::PopID();
			}
			
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
		gltf::calculateNodeMinMaxTraverse(scene, scene.nodes[scene.activeScene], bb);
		
		const Vec3 mid = (bb.min + bb.max) / 2.f;
		const Vec3 extents = (bb.max - bb.min) / 2.f;
		
		float maxAxis = 0.f;
		for (int i = 0; i < 3; ++i)
			maxAxis = fmaxf(maxAxis, extents[i]);
		
		if (maxAxis > 0.f)
		{
			Shader shader(
				"gltf/BasicSkinnedWithLighting",
				"engine/Generic.vs",
				"gltf/BasicSkinnedWithLighting.ps");
			
			setShader(shader);
			{
				shader.setImmediate("ambientLight_color",
					ambientLight_color[0],
					ambientLight_color[1],
					ambientLight_color[2]);
				
				const Vec3 lightColor = directionalLight_color * directionalLight_intensity;
				shader.setImmediate("directionalLight_color",
					lightColor[0],
					lightColor[1],
					lightColor[2]);
				
				const Vec3 lightDirection = directionalLight_direction.CalcNormalized();
				shader.setImmediate("directionalLight_direction",
					lightDirection[0],
					lightDirection[1],
					lightDirection[2]);
			}
			clearShader();
			
			gltf::MaterialShaders materialShaders;
			materialShaders.fallbackShader = &shader;
			
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
