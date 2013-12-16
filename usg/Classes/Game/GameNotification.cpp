#include "GameNotification.h"
#include "GameState.h"
#include "TempRender.h"
#include "UsgResources.h"

namespace Game
{
	GameNotification::GameNotification()
	{
		Initialize();
	}

	void GameNotification::Initialize()
	{
		mText1 = 0;
		mText2 = 0;
		mLife = 0.0f;
	}

	void GameNotification::Setup(Vec2F pos, Vec2F size)
	{
		mPos = pos;
		mSize = size;
	}
	
	void GameNotification::Update(float dt)
	{
		mLife -= dt;
	}

	void GameNotification::Render()
	{
		if (mLife > 0.0f)
		{
			Vec2F pos = mPos + Vec2F(0.0f, mSize[1] * 0.5f);
			
			RenderText(pos, mSize, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColors::White, TextAlignment_Center, TextAlignment_Top, true, mText1);
			
			if (mText2)
				RenderText(pos, mSize, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_Make(230, 230, 230, 255), TextAlignment_Center, TextAlignment_Bottom, true, mText2);
		}
	}

	void GameNotification::Show(const char* text1, const char* text2)
	{
		mText1 = text1;
		mText2 = text2;
		mLife = 3.0f;
	}
}
