#pragma once

#include "beams_forward.h"
#include "CallBack.h"
#include "libgg_forward.h"
#include "OpenGLCompat.h"
#include "TextureRGBA.h"

class BoardTile
{
public:
	BoardTile();
	~BoardTile();

	void Setup(Board* board, int x, int y, int boardSx, int boardSy, bool autoOrient);

	void Update(float dt);
	void Render(bool drawBack);

	void RotateX(int direction, float delay);
	void RotateY(int direction, float delay);

	bool IsCorrect_get() const; // return true when picture side is visible
	bool IsAnimating_get() const;

private:
	void UpdateFlip();

	Board* mBoard;
	int mAngleX;
	int mAngleY;
	AngleController* mAngleControllerX;
	AngleController* mAngleControllerY;
	float mAnimationDelay;
	bool mFlipX1;
	bool mFlipY1;
	bool mFlipX2;
	bool mFlipY2;
	float mTexCoord[2][2];
	bool mAutoOrient;
};

class Board
{
public:
	Board();
	~Board();

	void Setup(CallBack onAnimationTrigger, int sx, int sy, TextureRGBA* back, TextureRGBA* picture, bool autoOrient);

	void Update(float dt);
	void Render();
	
	void HandleAnimationTrigger();

	bool IsCorrect_get() const; // return true if all board tiles are correct
	BoardTile* Tile_get(int x, int y);
	bool IsAnimating_get() const; // returns true if any of the board tiles is still animating. assists in reducing OpenGL draw calls

	inline int Sx_get() const
	{
		return mSx;
	}
	inline int Sy_get() const
	{
		return mSy;
	}

private:
	void Render(bool drawBack);
	
	CallBack mOnAnimationTrigger;
	BoardTile* mTiles;
	int mSx;
	int mSy;
	GLuint mBackTextureId;
	GLuint mPictureTextureId;
};
