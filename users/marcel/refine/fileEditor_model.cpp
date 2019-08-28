#include "fileEditor_model.h"
#include "ui.h" // drawUiRectCheckered

static const char * s_basicSkinnedWithLightingPs = R"SHADER(

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
		
		vec3 color = v_color.rgb * total_light_color;
		float opacity = v_color.a;
		
		shader_fragColor = vec4(color, opacity);
		shader_fragNormal = vec4(v_normal, 0.0);
	}

)SHADER";

//

FileEditor_Model::FileEditor_Model(const char * path)
	: model(path)
{
	shaderSource("BasicSkinnedWithLighting.ps", s_basicSkinnedWithLightingPs);
	
	model.calculateAABB(min, max, true);
	
	guiContext.init();
}

FileEditor_Model::~FileEditor_Model()
{
	guiContext.shut();
}

void FileEditor_Model::tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured)
{
	if (hasFocus == false)
		return;
	
	// tick
	
	guiContext.processBegin(dt, sx, sy, inputIsCaptured);
	{
		ImGui::SetNextWindowPos(ImVec2(8, 8), ImGuiCond_Always);
		if (ImGui::Begin("Model", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_AlwaysAutoResize))
		{
			auto animList = model.getAnimList();
			
			const char ** animItems = (const char**)alloca(animList.size() * sizeof(char*));
			const char * animName = model.getAnimName();
			int animIndex = -1;
			for (int i = 0; i < (int)animList.size(); ++i)
			{
				if (animList[i] == animName)
					animIndex = i;
				animItems[i]= animList[i].c_str();
			}
			if (ImGui::Combo("Animation", &animIndex, animItems, animList.size()))
				model.startAnim(animItems[animIndex]);
			
			if (ImGui::Button("Stop animation"))
				model.stopAnim();
			if (ImGui::Button("Restart animation"))
				model.startAnim(model.getAnimName());
			
			ImGui::SliderFloat("Animation speed", &model.animSpeed, 0.f, 10.f, "%.2f", 2.f);
			
			ImGui::Checkbox("Show colored normals", &showColorNormals);
			ImGui::Checkbox("Show colored texture UVs", &showColorTexCoords);
			ImGui::Checkbox("Show normals", &showNormals);
			ImGui::SliderFloat("Normals scale", &normalsScale, 0.f, 100.f, "%.2f", 2.f);
			ImGui::Checkbox("Show bones", &showBones);
			ImGui::Checkbox("Show bind pose", &showBindPose);
			ImGui::Checkbox("Unskinned", &showUnskinned);
			ImGui::Checkbox("Hard skinned", &showHardskinned);
			ImGui::Checkbox("Show colored blend indices", &showColorBlendIndices);
			ImGui::Checkbox("Show colored blend weights", &showColorBlendWeights);
			ImGui::Checkbox("Show bounding box", &showBoundingBox);
			ImGui::Checkbox("Enable lighting", &enableLighting);
			ImGui::ColorEdit3("Ambient light color", &ambientLight_color[0]);
			ImGui::ColorEdit3("Directional light color", &directionalLight_color[0]);
			ImGui::SliderFloat("Scale", &scale, 0.f, 4.f, "%.2f", 2.f);
		}
		ImGui::End();
	}
	guiContext.processEnd();
	
	if (inputIsCaptured == false && mouse.isDown(BUTTON_LEFT))
	{
		inputIsCaptured = true;

		rotationX += mouse.dy;
		rotationY -= mouse.dx;
	}
	
	//
	
	model.animRootMotionEnabled = false;
	
	model.tick(dt);
	
	// draw
	
	clearSurface(0, 0, 0, 0);
	
	setColor(colorWhite);
	drawUiRectCheckered(0, 0, sx, sy, 8);
	
	projectPerspective3d(60.f, .01f, 100.f);
	{
		pushDepthTest(true, DEPTH_LESS);
		
		float maxAxis = 0.f;
		
		for (int i = 0; i < 3; ++i)
			maxAxis = fmaxf(maxAxis, fmaxf(fabsf(min[i]), fabsf(max[i])));
		
		if (maxAxis > 0.f)
		{
			const int drawFlags =
				DrawMesh
				| DrawColorNormals * showColorNormals
				| DrawNormals * showNormals
				| DrawColorTexCoords * showColorTexCoords
				| DrawBones * showBones
				| DrawPoseMatrices * showBindPose
				| DrawUnSkinned * showUnskinned
				| DrawHardSkinned * showHardskinned
				| DrawColorBlendIndices * showColorBlendIndices
				| DrawColorBlendWeights * showColorBlendWeights
				| DrawBoundingBox * showBoundingBox;
			
			model.drawNormalsScale = normalsScale;
			
			model.scale = scale;
			
			Shader shader("BasicSkinnedWithLighting", "engine/BasicSkinned.vs", "BasicSkinnedWithLighting.ps");
			
			setShader(shader);
			{
				shader.setImmediate("ambientLight_color",
					ambientLight_color[0],
					ambientLight_color[1],
					ambientLight_color[2]);
				
				const Vec3 lightDirection = directionalLight_direction.CalcNormalized();
				shader.setImmediate("directionalLight_color",
					directionalLight_color[0],
					directionalLight_color[1],
					directionalLight_color[2]);
				shader.setImmediate("directionalLight_direction",
					lightDirection[0],
					lightDirection[1],
					lightDirection[2]);
			}
			clearShader();
			
			if (enableLighting)
			{
				model.overrideShader = &shader;
			}
			
			gxPushMatrix();
			gxTranslatef(0, 0, 2.f);
			gxScalef(1.f / maxAxis, 1.f / maxAxis, 1.f / maxAxis);
			gxTranslatef(0.f, 0.f, 1.f);
			gxRotatef(rotationY, 0.f, 1.f, 0.f);
			gxRotatef(rotationX, 1.f, 0.f, 0.f);
			model.draw(drawFlags);
			gxPopMatrix();
		}
		
		popDepthTest();
	}
	projectScreen2d();
	
	guiContext.draw();
}
