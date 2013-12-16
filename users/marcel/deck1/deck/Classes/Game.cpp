#include "Calc.h"
#include "Game.h"
#include "GameObject.h"
#include "Input.h"
#include "Log.h"
#include "render.h"
#include "Shape.h"
#include "Sound.h"
#include "Texture.h"
#include "World.h"

#include "SpriteGfx.h"

Game* gGame = 0;

Slot::Slot(int x, int y, int sx, int sy, const Color& color)
{
	Shape shape;
	shape.Make_Rect(0, 0, sx, sy);
	shape.fixed = true;
//	mObjectId = gWorld->Add(&shape, Vec2F(x, y), 0.0f);
	mColor = new Color(color);
	
	mRect.Setup(Vec2I(x, y), Vec2I(sx, sy));
}

Slot::~Slot()
{
//	gWorld->Remove(mObjectId);
	delete mColor;
	mColor = 0;
}
	
void Slot::Update(float dt)
{
}

void Slot::Render()
{
	gRender->Quad(mRect.m_Position[0], mRect.m_Position[1], mRect.m_Position[0] + mRect.m_Size[0], mRect.m_Position[1] + mRect.m_Size[1], *mColor);
}

//

void Game::Begin()
{
	// create border
	
	int size = 160;
	int spx = (320 - size) / 2;
	int spy = (480 - size) / 2;
	
/*	MakeBorder(0, 0, spx, 30); // top-left
	MakeBorder(320-spx, 0, spx, 30); // top-right
	MakeBorder(0, 0, 30, spy); // left-top
	MakeBorder(0, 480 - spy, 30, spy); // left-bottom
	MakeBorder(290, 0, 30, spy); // right-top
	MakeBorder(290, 480 - spy, 30, spy); // right-bottom
	MakeBorder(0, 450, spx, 30); // bottom-left
	MakeBorder(320 - spx, 450, spx, 30); // bottom-right
	*/
	MakeTriangle(0, 0, spx, 0, 0, spy); // top-left
	MakeTriangle(320 - spx, 0, 320, 0, 320, spy); // top-right
	MakeTriangle(0, 480 - spy, spx, 480, 0, 480); // bottom-left
	MakeTriangle(320, 480 - spy, 320, 480, 320 - spx, 480); // bottom-right
	
	// create slots
	
	mSlot[ScreenSide_Left] = new Slot(0, 140, 30, 200, Color(1.0f, 0.0f, 0.0f));
	mSlot[ScreenSide_Top] = new Slot(60, 0, 200, 30, Color(0.0f, 1.0f, 0.0f));
	mSlot[ScreenSide_Right] = new Slot(290, 140, 30, 200, Color(0.0f, 0.0f, 1.0f));
	mSlot[ScreenSide_Bottom] = new Slot(60, 450, 200, 30, Color(1.0f, 1.0f, 0.0f));
	
	// randomize prim types for sides
	
	PrimType types[4] = { PrimType_Square, PrimType_Circle, PrimType_Side5, PrimType_Side6 };
	
	Calc::Shuffle<PrimType>(types, 4);
	
	mSideTypes[ScreenSide_Left] = PrimType_Side5;
	mSideTypes[ScreenSide_Right] = PrimType_Side6;
	mSideTypes[ScreenSide_Top] = PrimType_Square;
	mSideTypes[ScreenSide_Bottom] = PrimType_Circle;
	
	// add a bunch of objects
	
	int count = 20;
	
	for (int i = 0; i < count; ++i)
	{
		PrimType type = types[rand() % 4];
		//PrimType type = PrimType_Circle;
		
		Shape* shape = new Shape();
		
		//float radius = Calc::Random(30.0f, 70.0f);
//		float radius = Calc::Random(60.0f, 70.0f);
		float radius = Calc::Random(20.0f, 40.0f);
		
		switch (type)
		{
			case PrimType_Circle:
				shape->Make_Circle(radius);
				break;
			case PrimType_Square:
				shape->Make_Convex(4, radius);
				break;
			case PrimType_Side5:
				shape->Make_Convex(5, radius);
				break;
			case PrimType_Side6:
				shape->Make_Convex(6, radius);
				break;
			default:
				throw ExceptionNA();
		}
		
		Vec2F position = Vec2F(Calc::Random(0.0f, 320.0f), Calc::Random(0.0f, 480.0f));
		float angle = Calc::Random(Calc::m2PI);
		
		int objectId = gWorld->Add(shape, position, angle);
		
		GameObject* object = new GameObject(shape, true, objectId, type);
		
		mGameObjectList.push_back(object);
	}
	
	LoadTexture("background.png", mBackTexture);
}

void Game::End()
{
	// todo: remove game objects
	
	// remove slots
	
	for (int i = 0; i < 4; ++i)
	{
		delete mSlot[i];
		mSlot[i] = 0;
	}
}

void Game::Update(float dt)
{
//	LOG_DBG("game: update", 0);
	
	// update game objects
	
	for (std::vector<GameObject*>::iterator i = mGameObjectList.begin(); i != mGameObjectList.end();)
	{
		GameObject* object = *i;
		
		object->Update(dt);
			
		if (object->mIsDead)
		{
			LOG_DBG("game: purging dead go", 0);
			
			if (object->mObjectId == mFocusObjectId)
				mFocusObjectId = -1;
			
			gWorld->Remove(object->mObjectId);
			
			delete object;
			
			i = mGameObjectList.erase(i);
		}
		else
		{
			++i;
		}
	}
	
	// update touch interaction
	
	if (gInput->mTouchActive && mFocusObjectId < 0)
	{
		mFocusObjectId = gWorld->TestPoint(Vec2F(gInput->mTouchPos[0], gInput->mTouchPos[1]), mFocusPointInObjectSpace);
										 
		LOG_DBG("point test: %d", mFocusObjectId);
		
		if (mFocusObjectId >= 0)
		{
			PlaySound("sel");
			
			gInput->TouchDelta_get();
			
			LOG_DBG("point in object: %f, %f", mFocusPointInObjectSpace[0], mFocusPointInObjectSpace[1]);
		}
	}
	else if (!gInput->mTouchActive && mFocusObjectId >= 0)
	{
		PlaySound("desel");
		
		mFocusObjectId = -1;
	}
	
	if (mFocusObjectId >= 0)
	{	
		Vec2F position1 = gWorld->ObjectToWorld(mFocusObjectId, mFocusPointInObjectSpace);
		Vec2F position2 = gInput->mTouchPos.ToF();
		
		Vec2F delta = position2 - position1;
		
		Vec2F force = delta * 10.0f;
		
		gWorld->ApplyForce(mFocusObjectId, mFocusPointInObjectSpace, force, true);
		gWorld->SetLinearVelocity(mFocusObjectId, delta * 5.0f);
	}
	
	// update physics
	
	gWorld->Update(dt);
	
	// check if any objects crossed a side
	
	for (std::vector<GameObject*>::iterator i = mGameObjectList.begin(); i != mGameObjectList.end(); ++i)
	{
		GameObject* object = *i;
		
		Vec2F position = gWorld->Describe(object->mObjectId).position;
		
		ScreenSide side = ScreenSide_Undefined;
		
		if (position[0] < 0.0f)
			side = ScreenSide_Left;
		if (position[1] < 0.0f)
			side = ScreenSide_Top;
		if (position[0] > 320.0f)
			side = ScreenSide_Right;
		if (position[1] > 480.0f)
			side = ScreenSide_Bottom;

		if (side != ScreenSide_Undefined)
		{
			if (mSideTypes[side] == object->mType)
				PlaySound("right");
			else
				PlaySound("wrong");
			
			object->Disappear(side);
		}
	}
}

void Game::Render()
{
	// render background
	
	if (1)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mBackTexture.textureId);
		SpriteGfx gfx;
		gfx.Setup(20, 20);
		gfx.Reserve(4, 6);
		gfx.WriteBegin();
		int rgba = SpriteColor_Make(255, 255, 255, 255).rgba;
		gfx.WriteVertex(0.0f, 0.0f, rgba, 0.0f, 0.0f);
		gfx.WriteVertex(320.0f, 0.0f, rgba, mBackTexture.s[0], 0.0f);
		gfx.WriteVertex(320.0f, 480.0f, rgba, mBackTexture.s[0], mBackTexture.s[1]);
		gfx.WriteVertex(0.0f, 480.0f, rgba, 0.0f, mBackTexture.s[1]);
		gfx.WriteIndex3(0, 1, 2);
		gfx.WriteIndex3(0, 2, 3);
		gfx.WriteEnd();
		gfx.Flush();
		glDisable(GL_TEXTURE_2D);
	}
	else
	{
		gRender->Clear(Color(0.0f, 0.2f, 0.0f, 0.0f));
	}
	
	// render sides
	
//	for (int i = 0; i < 4; ++i)
//		mSlot[i]->Render();
	
	// render objects
	
	for (std::vector<GameObject*>::iterator i = mGameObjectList.begin(); i != mGameObjectList.end(); ++i)
	{
		GameObject* object = *i;
		
		object->Render();
	}
	
	if (mFocusObjectId >= 0)
	{
		Vec2F position1 = gWorld->ObjectToWorld(mFocusObjectId, mFocusPointInObjectSpace);
		Vec2F position2 = gInput->mTouchPos.ToF();
		
		gRender->Line(position1[0], position1[1], position2[0], position2[1], Color(0.0f, 0.0f, 1.0f));
	}
}

PrimType Game::GetPrimTypeForSide(ScreenSide side)
{
	return mSideTypes[side];
}

/*void Game::MakeBorder(int x, int y, int sx, int sy)
{
	Shape shape;
	shape.Make_Rect(0, 0, sx, sy);
	shape.fixed = true;
	int objectId = gWorld->Add(&shape, Vec2F(x, y), 0.0f);
	mBorderIds.push_back(objectId);
}
*/
void Game::MakeTriangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
	Shape shape;
	shape.type = ShapeType_Convex;
	shape.convex.coordCount = 3;
	shape.convex.coords[0].Set(x1, y1);
	shape.convex.coords[1].Set(x2, y2);
	shape.convex.coords[2].Set(x3, y3);
	shape.fixed = true;
	int objectId = gWorld->Add(&shape, Vec2F(0.0f, 0.0f), 0.0f);
	mBorderIds.push_back(objectId);
}
