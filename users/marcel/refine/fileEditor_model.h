#pragma once

#include "fileEditor.h"

struct FileEditor_Model : FileEditor
{
	Model model;
	Vec3 min;
	Vec3 max;
	float rotationX = 0.f;
	float rotationY = 0.f;
	
	bool showColorNormals = true;
	bool showColorTexCoords = false;
	bool showNormals = false;
	float normalsScale = 1.f;
	bool showBones = false;
	bool showBindPose = false;
	bool showUnskinned = false;
	bool showHardskinned = false;
	bool showColorBlendIndices = false;
	bool showColorBlendWeights = false;
	bool showBoundingBox = false;
	
	FrameworkImGuiContext guiContext;
	
	FileEditor_Model(const char * path)
		: model(path)
	{
		model.calculateAABB(min, max, true);
		
		guiContext.init();
	}
	
	virtual ~FileEditor_Model() override
	{
		guiContext.shut();
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
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
};
