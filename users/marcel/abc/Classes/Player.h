#pragma once

#include "Tile.h"
#include "TouchDLG.h"
#include "Types.h"

class InputButton
{
public:
	InputButton();
	void Setup(int prio, Vec2F position, Vec2F size);
	
	void Render();
	
	bool HitTest(Vec2F position);
	
	static bool HandleTouchBegin(void* obj, const TouchInfo& ti);
	static bool HandleTouchEnd(void* obj, const TouchInfo& ti);
	static bool HandleTouchMove(void* obj, const TouchInfo& ti);
	
	Vec2F mPosition;
	Vec2F mSize;
	bool mIsDown;
};

class Player
{
public:
	Player();
	void Setup();
	
	void Update(float dt);
	void UpdateInput();
	void UpdateGravity(float dt);
	void UpdateMove(float dt);
	void UpdateCollision();
	void UpdateBounce();
	void Render();
	void RenderUI();
	
	void HandleBounce();
	void HandleDie();
	void HandleRing();
	void HandleAcceleration(float x, float y, float z);
	
	bool OnSolidGround();
	
	static void HandleCollision(void* obj, Tile& tile);
	
	Vec2F mPosition;
	Vec2F mSpeed;
	
	struct
	{
		bool mJump;
		float mSpeedX;
		float mTiltAngle;
	} mInput;
	
	InputButton mButtonJump;
	InputButton mButtonMoveL;
	InputButton mButtonMoveR;
};
