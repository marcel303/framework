#pragma once

#include <vector>
#include "deck_forward.h"
#include "GameObjectType.h"
#include "ScreenSide.h"
#include "TextureDesc.h"
#include "Types.h"

//
// game = 4 sides, with designated type
// bunch of (draggable) shapes
// collision detection between shapes
// if shape center outside world -> check side
// good side? score
// wrong side? game over
//

class Slot
{
public:
	Slot(int x, int y, int sx, int sy, const Color& color);
	~Slot();
	
	void Update(float dt);
	void Render();
	
	int mObjectId;
	RectI mRect;
	Color* mColor;
};

class Game
{
public:
	void Begin();
	void End();
	
	void Update(float dt);
	void Render();
	
	PrimType GetPrimTypeForSide(ScreenSide side);
	
private:
//	void MakeBorder(int x, int y, int sx, int sy);
	void MakeTriangle(int x1, int y1, int x2, int y2, int x3, int y3);
	
	PrimType mSideTypes[4];
	Slot* mSlot[4];
	std::vector<int> mBorderIds;
	
	std::vector<GameObject*> mGameObjectList;
	
	int mFocusObjectId;
	Vec2F mFocusPointInObjectSpace;
	
	TextureDesc mBackTexture;
};

extern Game* gGame;
