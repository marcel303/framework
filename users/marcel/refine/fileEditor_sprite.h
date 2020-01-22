#pragma once

#include "fileEditor.h"
#include "imgui.h"
#include "reflection.h" // reflect
#include "ui.h" // drawUiRectCheckered

struct FileEditor_Sprite : FileEditor
{
	enum SizeMode
	{
		kSizeMode_SpriteScale,
		kSizeMode_Contain,
		kSizeMode_Fill
	};
	
	float desiredScale = 1.f;
	
	Sprite sprite;
	std::string path;
	char sheetFilename[64];
	
	FrameworkImGuiContext guiContext;
	
	bool firstFrame = true;
	
	SizeMode sizeMode = kSizeMode_SpriteScale;
	
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
	
	virtual bool reflect(TypeDB & typeDB, StructuredType & type) override
	{
		typeDB.addEnum<FileEditor_Sprite::SizeMode>("FileEditor_Sprite::SizeMode")
			.add("spriteScale", kSizeMode_SpriteScale)
			.add("contain", kSizeMode_Contain)
			.add("fill", kSizeMode_Fill);
		
		type.add("sizeMode", &FileEditor_Sprite::sizeMode);
		
		return true;
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		if (hasFocus == false)
			return;
		
		if (firstFrame)
		{
			firstFrame = false;
			
			// work out the initial scale
			
			const float scaleX = sx / float(sprite.getWidth());
			const float scaleY = sy / float(sprite.getHeight());
			const float scale = fminf(1.f, fminf(scaleX, scaleY));
			
			desiredScale = scale;
		}
		
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
		
		setColor(colorWhite);
		const float oldScale = sprite.scale;
		sprite.scale = calculateSpriteScale(sx, sy);
		sprite.pixelpos = false;
		sprite.x = (sx - sprite.getWidth() * sprite.scale) / 2;
		sprite.y = (sy - sprite.getHeight() * sprite.scale) / 2;
		sprite.draw();
		sprite.scale = oldScale;
		
		guiContext.processBegin(dt, sx, sy, inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(8, 8));
			ImGui::SetNextWindowBgAlpha(.6f);
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
		
		if (inputIsCaptured == false)
		{
			// process input
			
			inputIsCaptured = true;
			
			if (keyboard.wentDown(SDLK_0))
				desiredScale *= 1.f;
			if (keyboard.wentDown(SDLK_EQUALS, true))
				desiredScale *= 1.5f;
			if (keyboard.wentDown(SDLK_MINUS, true))
				desiredScale /= 1.5f;
			
			desiredScale *= powf(1.2f, mouse.scrollY);
		}
		
		guiContext.draw();
	}
	
	float calculateSpriteScale(const int sx, const int sy) const
	{
		const int spriteSx = sprite.getWidth();
		const int spriteSy = sprite.getHeight();
		
		const float fillScaleX = sx / float(spriteSx);
		const float fillScaleY = sy / float(spriteSy);
	
		if (sizeMode == kSizeMode_SpriteScale)
		{
			return sprite.scale;
		}
		if (sizeMode == kSizeMode_Fill)
		{
			return fmaxf(fillScaleX, fillScaleY);
		}
		else if (sizeMode == kSizeMode_Contain)
		{
			return fminf(fillScaleX, fillScaleY);
		}
		else
		{
			Assert(false);
			return 1.f;
		}
	}
	
	virtual void doButtonBar() override
	{
		if (ImGui::BeginMenu("Scale"))
		{
			if (ImGui::MenuItem("Sprite scale", nullptr, sizeMode == kSizeMode_SpriteScale))
				sizeMode = kSizeMode_SpriteScale;
			if (ImGui::MenuItem("Contain", nullptr, sizeMode == kSizeMode_Contain))
				sizeMode = kSizeMode_Contain;
			if (ImGui::MenuItem("Fill", nullptr, sizeMode == kSizeMode_Fill))
				sizeMode = kSizeMode_Fill;
			
			ImGui::EndMenu();
		}
	}
};
