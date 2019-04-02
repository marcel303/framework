#pragma once

#include "fileEditor.h"

struct FileEditor_Spriter : FileEditor
{
	Spriter spriter;
	SpriterState spriterState;
	
	bool firstFrame = true;
	
	FrameworkImGuiContext guiContext;
	
	FileEditor_Spriter(const char * path)
		: spriter(path)
	{
		spriterState.startAnim(spriter, "Idle");
		
		guiContext.init();
	}
	
	virtual ~FileEditor_Spriter() override
	{
		guiContext.shut();
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		if (hasFocus == false)
			return;
		
		if (firstFrame)
		{
			firstFrame = false;
			
			spriterState.x = sx/2.f;
			spriterState.y = sy/2.f;
		}
		
		spriterState.updateAnim(spriter, dt);
		
		//
		
		guiContext.processBegin(dt, sx, sy, inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(8, 8));
			ImGui::SetNextWindowSize(ImVec2(240, 0));
			if (ImGui::Begin("Spriter"))
			{
				const int animCount = spriter.getAnimCount();
				
				if (animCount > 0)
				{
					const char ** animItems = (const char**)alloca(animCount * sizeof(char*));
					int animIndex = spriterState.animIndex;
					for (int i = 0; i < animCount; ++i)
						animItems[i]= spriter.getAnimName(i);
					if (ImGui::Combo("Animation", &animIndex, animItems, animCount))
						spriterState.startAnim(spriter, animIndex);
					if (ImGui::Button("Restart animation"))
						spriterState.startAnim(spriter, spriterState.animIndex);
				}
			}
			ImGui::End();
		}
		guiContext.processEnd();
		
		if (inputIsCaptured == false && mouse.isDown(BUTTON_LEFT))
		{
			inputIsCaptured = true;
			
			spriterState.x = mouse.x;
			spriterState.y = mouse.y;
		}
		
		// draw
		
		clearSurface(0, 0, 0, 0);
		
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		setColor(colorWhite);
		spriter.draw(spriterState);
		
		guiContext.draw();
	}
};
