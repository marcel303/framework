#include "Game.h"
#include "HackRender.h"
#include "Player.h"

#define SPEED_X 100.0f
#define GRAVITY Vec2F(0.0f, 300.0f)
#define PLAYER_SIZE 32.0f
#define SPEED_BOUNCE -100.0f
#define SPEED_JUMP (SPEED_BOUNCE * 2.5f)

Player::Player()
{
	mInput.mJump = false;
	mInput.mSpeedX = 0.0f;
}

void Player::Setup()
{
	mButtonJump.Setup(0, Vec2F(260.0f, 400.0f), Vec2F(60.0f, 60.0f));
	mButtonMoveL.Setup(1, Vec2F(0.0f, 400.0f), Vec2F(80.0f, 60.0f));
	mButtonMoveR.Setup(2, Vec2F(80.0f, 400.0f), Vec2F(80.0f, 60.0f));
}

void Player::Update(float dt)
{
	UpdateInput();
	UpdateGravity(dt);
	UpdateMove(dt);
	UpdateCollision();
	UpdateBounce();
}

void Player::UpdateInput()
{
	mInput.mJump = mButtonJump.mIsDown;
	
	mInput.mSpeedX = 0.0f;
	
	mInput.mSpeedX = sinf(mInput.mTiltAngle) * 2.0f;
	
	if (mButtonMoveL.mIsDown)
		mInput.mSpeedX -= 1.0f;
	if (mButtonMoveR.mIsDown)
		mInput.mSpeedX += 1.0f;
	
	if (mInput.mSpeedX < -1.0f)
		mInput.mSpeedX = -1.0f;
	if (mInput.mSpeedX > +1.0f)
		mInput.mSpeedX = +1.0f;
	
	mSpeed[0] = mInput.mSpeedX * SPEED_X;
}

void Player::UpdateGravity(float dt)
{
	mSpeed += GRAVITY * dt;
}

void Player::UpdateMove(float dt)
{
	mPosition += mSpeed * dt;
}

void Player::UpdateCollision()
{
	//Vec2F min = mPosition + Vec2F(-PLAYER_SIZE * 0.5f, -PLAYER_SIZE);
	//Vec2F max = mPosition + Vec2F(+PLAYER_SIZE * 0.5f, 0.0f);
	Vec2F min = mPosition;
	Vec2F max = mPosition;
	
	gGame->mMap.TileQuery(min, max, HandleCollision, this);
}

void Player::UpdateBounce()
{
	if (mSpeed[1] >= 0.0f && OnSolidGround())
		HandleBounce();
}

void Player::Render()
{
	HR_Circle(mPosition - Vec2F(0.0f, TILE_SX * 0.5f), TILE_SX * 0.5f);
}

void Player::RenderUI()
{
	mButtonJump.Render();
	mButtonMoveL.Render();
	mButtonMoveR.Render();
}

void Player::HandleBounce()
{
	if (mInput.mJump)
		mSpeed[1] = SPEED_JUMP;
	else
		mSpeed[1] = SPEED_BOUNCE;
}

void Player::HandleDie()
{
	// todo: decrease lives
	
	// todo: start invincibility timer
	
	// todo: play sound
}

void Player::HandleRing()
{
	// todo: increase score
	
	// todo: check if all rings picked up, open exit
	
	// todo: play sound
}

void Player::HandleAcceleration(float x, float y, float z)
{
	// todo: calculate tilt
	
	mInput.mTiltAngle = atan2(x, y);
}

bool Player::OnSolidGround()
{
	Tile* tile = gGame->mMap.GetTile(mPosition);
	
	return tile && tile->HitTest(mPosition);
}

void Player::HandleCollision(void* obj, Tile& tile)
{
//	Player* self = (Player*)obj;
	
	tile.HandleHit();
}

//

InputButton::InputButton()
{
	mIsDown = false;
}

void InputButton::Setup(int prio, Vec2F position, Vec2F size)
{
	mPosition = position;
	mSize = size;
	
	TouchListener listener;
	listener.Setup(this, HandleTouchBegin, HandleTouchEnd, HandleTouchMove);
	gGame->mTouchDelegator.Register(prio, listener);
	gGame->mTouchDelegator.Enable(prio);
}

void InputButton::Render()
{
	HR_RectLine(mPosition, mPosition + mSize);
}

bool InputButton::HitTest(Vec2F position)
{
	return
		position[0] >= mPosition[0] &&
		position[1] >= mPosition[1] &&
		position[0] <= mPosition[0] + mSize[0] &&
		position[1] <= mPosition[1] + mSize[1];
}

bool InputButton::HandleTouchBegin(void* obj, const TouchInfo& ti)
{
	InputButton* self = (InputButton*)obj;
	
	if (self->mIsDown)
		return false;
	
	if (!self->HitTest(ti.m_Location))
		return false;
	
	self->mIsDown = true;
	
	return true;
}

bool InputButton::HandleTouchEnd(void* obj, const TouchInfo& ti)
{
	InputButton* self = (InputButton*)obj;
	
	self->mIsDown = false;
	
	return true;
}

bool InputButton::HandleTouchMove(void* obj, const TouchInfo& ti)
{
	return true;
}
