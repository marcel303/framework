#pragma once

#include "fileEditor.h"

struct FileEditor_Sprite : FileEditor
{
	float desiredScale = 1.f;
	
	Sprite sprite;
	std::string path;
	char sheetFilename[64];
	
	FrameworkImGuiContext guiContext;
	
	bool firstFrame = true;
	
	FileEditor_Sprite(const char * in_path)
		: sprite(in_path)
		, path(in_path)
	{
		sheetFilename[0] = 0;
		
		guiContext.init();
	}
	
	virtual ~FileEditor_Sprite() override
	{
		guiContext.shut();
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		if (hasFocus == false)
			return;
		
		// update smoothed changes
		
		const float retainAnimTick = .8f;
		const float retainThisTick = powf(retainAnimTick, dt * 100.f);
		
		sprite.scale = lerp<float>(sprite.scale, desiredScale, 1.f - retainThisTick);
		
		// update sprite
		
		sprite.update(dt);
		
		// draw
		
		clearSurface(0, 0, 0, 0);
		
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		if (firstFrame)
		{
			firstFrame = false;
			const float scaleX = sx / float(sprite.getWidth());
			const float scaleY = sy / float(sprite.getHeight());
			const float scale = fminf(1.f, fminf(scaleX, scaleY));
			
			sprite.scale = scale;
		}
		
		setColor(colorWhite);
		sprite.pixelpos = false;
		sprite.x = (sx - sprite.getWidth() * sprite.scale) / 2;
		sprite.y = (sy - sprite.getHeight() * sprite.scale) / 2;
		sprite.draw();
		
		guiContext.processBegin(dt, sx, sy, inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(8, 8));
			if (ImGui::Begin("Sprite", nullptr,
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_AlwaysAutoResize))
			{
				if (ImGui::InputText("Sheet", sheetFilename, sizeof(sheetFilename)))
				{
					if (sheetFilename[0] == 0)
						sprite = Sprite(path.c_str());
					else
						sprite = Sprite(path.c_str(), 0.f, 0.f, sheetFilename);
				}
				
				ImGui::SliderFloat("angle", &sprite.angle, -360.f, +360.f);
				ImGui::SliderFloat("scale", &desiredScale, 0.f, 100.f, "%.2f", 2);
				ImGui::Checkbox("flip X", &sprite.flipX);
				ImGui::Checkbox("flip Y", &sprite.flipY);
				ImGui::InputFloat("pivot X", &sprite.pivotX);
				ImGui::InputFloat("pivot Y", &sprite.pivotY);
				
				auto animList = sprite.getAnimList();
				
				if (!animList.empty())
				{
					const char ** animItems = (const char**)alloca(animList.size() * sizeof(char*));
					auto & animName = sprite.getAnim();
					int animIndex = -1;
					for (int i = 0; i < (int)animList.size(); ++i)
					{
						if (animList[i] == animName)
							animIndex = i;
						animItems[i]= animList[i].c_str();
					}
					if (ImGui::Combo("Animation", &animIndex, animItems, animList.size()))
						sprite.startAnim(animItems[animIndex]);
				}
			}
			ImGui::End();
		}
		guiContext.processEnd();
		
		guiContext.draw();
	}
};
