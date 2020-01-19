#pragma once

#include "fileEditor.h"
#include "reflection.h" // reflect
#include "ui.h" // drawUiRectCheckered

struct FileEditor_Spriter : FileEditor
{
	Spriter spriter;
	SpriterState spriterState;
	
	bool showAxis = false;
	
	bool firstFrame = true;
	
	FrameworkImGuiContext guiContext;
	
	FileEditor_Spriter(const char * path)
		: spriter(path)
	{
		spriterState.startAnim(spriter, 0);
		
		guiContext.init();
	}
	
	virtual ~FileEditor_Spriter() override
	{
		guiContext.shut();
	}
	
	virtual bool reflect(TypeDB & typeDB, StructuredType & type) override
	{
		type.add("showAxis", &FileEditor_Spriter::showAxis);
		
		return true;
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
					// animation selection
					
					const char ** animItems = (const char**)alloca(animCount * sizeof(char*));
					int animIndex = spriterState.animIndex;
					for (int i = 0; i < animCount; ++i)
						animItems[i] = spriter.getAnimName(i);
					if (ImGui::Combo("Animation", &animIndex, animItems, animCount))
						spriterState.startAnim(spriter, animIndex);
					
					if (ImGui::Button("Restart animation"))
						spriterState.startAnim(spriter, spriterState.animIndex);
					
					// previous and next animation
					
					ImGui::SameLine();
					if (ImGui::Button("-"))
					{
						animIndex = (animIndex - 1 + animCount) % animCount;
						spriterState.startAnim(spriter, animIndex);
					}
					ImGui::SameLine();
					if (ImGui::Button("+"))
					{
						animIndex = (animIndex + 1) % animCount;
						spriterState.startAnim(spriter, animIndex);
					}
					
					// animation controls
					
					ImGui::SameLine();
					ImGui::Checkbox("", &spriterState.animLoop);
					
					ImGui::SliderFloat("Speed", &spriterState.animSpeed, 0.f, 10.f);
				}
				
				ImGui::SliderFloat("Angle", &spriterState.angle, -360.f, +360.f);
				ImGui::SliderFloat("Scale", &spriterState.scale, 0.f, 10.f);
				ImGui::Checkbox("Flip X", &spriterState.flipX);
				ImGui::Checkbox("Flip Y", &spriterState.flipY);
				
				ImGui::NewLine();
				ImGui::Text("Helpers");
				ImGui::Checkbox("Show axis", &showAxis);
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
		
		if (showAxis)
		{
			gxPushMatrix();
			{
				// draw axis
				
				gxTranslatef(spriterState.x, spriterState.y, 0);
				gxRotatef(spriterState.angle, 0, 0, 1);
				
				setColor(colorRed);
				drawLine(-10, 0, +10, 0);
				
				setColor(colorGreen);
				drawLine(0, -10, 0, +10);
			}
			gxPopMatrix();
		}
		
		// draw spriter
		
		setColor(colorWhite);
		spriter.draw(spriterState);
		
		guiContext.draw();
	}
};
