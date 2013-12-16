#pragma once

#include "Types.h"

namespace Game
{
	class GameNotification
	{
	public:
		GameNotification();
		void Initialize();
		void Setup(Vec2F pos, Vec2F size);
		
		void Update(float dt);
		void Render();
		
		void Show(const char* text1, const char* text2);
		
	private:
		Vec2F mPos;
		Vec2F mSize;
		const char* mText1;
		const char* mText2;
		float mLife;
	};
}
