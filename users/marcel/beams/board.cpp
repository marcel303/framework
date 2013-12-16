#include "AngleController.h"
#include "board.h"
#include "Calc.h"
#include "Debugging.h"
#include "Mat4x4.h"
#include "OpenGLCompat.h"
#include "render.h"
#include "texture.h"

#define SIZE 40
//#define SIZE 80
#define ROTATION_SPEED (Calc::mPI * 1.5f)

BoardTile::BoardTile()
{
	mAngleX = 0;
	mAngleY = 0;
	mAngleControllerX = new AngleController();
	mAngleControllerX->Setup(0.0f, 0.0f, ROTATION_SPEED);
	mAngleControllerY = new AngleController();
	mAngleControllerY->Setup(0.0f, 0.0f, ROTATION_SPEED);
	mAnimationDelay = 0.0f;
	mFlipX1 = false;
	mFlipY1 = false;
	mFlipX2 = false;
	mFlipY2 = false;
}

BoardTile::~BoardTile()
{
	delete mAngleControllerX;
	mAngleControllerX = 0;

	delete mAngleControllerY;
	mAngleControllerY = 0;
}

void BoardTile::Setup(Board* board, int x, int y, int boardSx, int boardSy, bool autoOrient)
{
	mBoard = board;
	
	mTexCoord[0][0] = (x + 0) / (float)boardSx;
	mTexCoord[0][1] = 1.0f - (y + 0) / (float)boardSy;
	mTexCoord[1][0] = (x + 1) / (float)boardSx;
	mTexCoord[1][1] = 1.0f - (y + 1) / (float)boardSy;
	
	mAutoOrient = autoOrient;
}

void BoardTile::Update(float dt)
{
	mAnimationDelay = Calc::Max(0.0f, mAnimationDelay - dt);

	if (mAnimationDelay == 0.0f)
	{
		mAngleControllerX->TargetAngle_set(mAngleX * Calc::mPI);
		mAngleControllerX->Update(dt);
		mAngleControllerY->TargetAngle_set(mAngleY * Calc::mPI);
		mAngleControllerY->Update(dt);
	}
}

void BoardTile::Render(bool drawBack)
{
	const float angleX = mAngleControllerX->Angle_get();
	const float angleY = mAngleControllerY->Angle_get();

	Mat4x4 matX;
	Mat4x4 matY;
	matX.MakeRotationX(angleX);
	matY.MakeRotationY(angleY);
	Mat4x4 mat = matX * matY;
	Vec3 dir = mat * Vec3(0.0f, 0.0f, 1.0f);
	bool frontSideIsVisible = dir[2] >= 0.0f;

	if (frontSideIsVisible && drawBack)
		return;
	if (!frontSideIsVisible && !drawBack)
		return;
	
	// todo: determine picture flip X/Y

	if (frontSideIsVisible)
	{
		if (mFlipX1)
			glScalef(-1.0f, 1.0f, 1.0f);
		if (mFlipY1)
			glScalef(1.0f, -1.0f, 1.0f);
	}
	else
	{
		if (mFlipX2)
			glScalef(-1.0f, 1.0f, 1.0f);
		if (mFlipY2)
			glScalef(1.0f, -1.0f, 1.0f);
	}

	// todo: construct matrix
	// todo: emit quad

	const float s = SIZE / 2.0f;

	const Color color = frontSideIsVisible ? Color(1.0f, 1.0f, 1.0f) : Color(0.5f, 0.5f, 0.5f);

	glPushMatrix();
	
	glRotatef(angleX * 360.0f / Calc::m2PI, 1.0f, 0.0f, 0.0f);
	glRotatef(angleY * 360.0f / Calc::m2PI, 0.0f, 1.0f, 0.0f);
	
	gRender->QuadTex(-s, -s, +s, +s, color, mTexCoord[0][0], mTexCoord[0][1], mTexCoord[1][0], mTexCoord[1][1]);

	glPopMatrix();
}

void BoardTile::RotateX(int direction, float delay)
{
	Assert(direction == -1 || direction == +1);

	mAngleX += direction;

	if (!IsAnimating_get())
	{
		mBoard->HandleAnimationTrigger();
		mAnimationDelay = delay;
	}

	UpdateFlip();
}

void BoardTile::RotateY(int direction, float delay)
{
	Assert(direction == -1 || direction == +1);

	mAngleY += direction;

	if (!IsAnimating_get())
	{
		mBoard->HandleAnimationTrigger();
		mAnimationDelay = delay;
	}

	UpdateFlip();
}

bool BoardTile::IsCorrect_get() const
{
	const int angle = (mAngleX + mAngleY) % 2;

	return angle == 0;
}

bool BoardTile::IsAnimating_get() const
{
	return
		!mAngleControllerX->HasReachedTarget_get() ||
		!mAngleControllerY->HasReachedTarget_get();
}

void BoardTile::UpdateFlip()
{
	if (mAutoOrient)
	{
		if (IsCorrect_get())
		{
			// update front flip

			mFlipX1 = mAngleY % 2;
			mFlipY1 = mAngleX % 2;
		}
		else
		{
			// update back flip

			mFlipX2 = mAngleY % 2;
			mFlipY2 = mAngleX % 2;
		}
	}
	else
	{
		mFlipX1 = 0;
		mFlipY1 = 0;
		mFlipX2 = 0;
		mFlipY2 = 0;
	}
}

//

Board::Board()
{
	mTiles = 0;
	mSx = 0;
	mSy = 0;
	mBackTextureId = 0;
	mPictureTextureId = 0;
}

Board::~Board()
{
	Setup(CallBack(), 0, 0, 0, 0, false);
}

void Board::Setup(CallBack onAnimationTrigger, int sx, int sy, TextureRGBA* back, TextureRGBA* picture, bool autoOrient)
{
	Assert(sx >= 0);
	Assert(sy >= 0);
	Assert(back);
	Assert(picture);
	
	if (mTiles)
		delete[] mTiles;
	mTiles = 0;
	mSx = 0;
	mSy = 0;
	if (mBackTextureId)
		glDeleteTextures(1, &mBackTextureId);
	mBackTextureId = 0;
	if (mPictureTextureId)
		glDeleteTextures(1, &mPictureTextureId);
	mPictureTextureId = 0;

	mOnAnimationTrigger = onAnimationTrigger;
	
	if (sx * sy > 0)
	{
		mTiles = new BoardTile[sx * sy];
		mSx = sx;
		mSy = sy;
		for (int x = 0; x < mSx; ++x)
			for (int y = 0; y < mSy; ++y)
				Tile_get(x, y)->Setup(this, x, y, mSx, mSy, autoOrient);
	}

	if (back)
		ToTexture(back, mBackTextureId);
	if (picture)
		ToTexture(picture, mPictureTextureId);
}


void Board::Update(float dt)
{
	for (int i = 0; i < mSx * mSy; ++i)
		mTiles[i].Update(dt);
}

void Board::Render()
{
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, mBackTextureId);
	Render(true);
	
	glBindTexture(GL_TEXTURE_2D, mPictureTextureId);
	Render(false);

	glDisable(GL_TEXTURE_2D);
}

void Board::HandleAnimationTrigger()
{
	LOG_DBG("Board::HandleAnimationTrigger", 0);
	
	if (mOnAnimationTrigger.IsSet())
		mOnAnimationTrigger.Invoke(0);
}

bool Board::IsCorrect_get() const
{
	for (int i = 0; i < mSx * mSy; ++i)
		if (!mTiles[i].IsCorrect_get())
			return false;

	return true;
}

bool Board::IsAnimating_get() const
{
	for (int i = 0; i < mSx * mSy; ++i)
		if (mTiles[i].IsAnimating_get())
			return true;

	return false;
}

BoardTile* Board::Tile_get(int x, int y)
{
	return mTiles + x + y * mSx;
}

void Board::Render(bool drawBack)
{
	for (int x = 0; x < mSx; ++x)
	{
		for (int y = 0; y < mSy; ++y)
		{
			BoardTile* tile = Tile_get(x, y);

			glPushMatrix();
			glTranslatef((x + 0.5f) * SIZE, (y + 0.5f) * SIZE, 0.0f);

			tile->Render(drawBack);

			glPopMatrix();
		}
	}
}
