#pragma once

#include "Types.h"

class PuzzlePiece
{
public:	
	void Setup(Vec2I imageLocation)
	{
		mImageLocation = imageLocation;
		mLocation = imageLocation;
		mLocationPrev = imageLocation;
		mIsFree = false;
		mAnimationTimer = 1.0f;
	}
	
	Vec2I mImageLocation;
	Vec2I mLocation;
	Vec2I mLocationPrev;
	bool mIsFree;
	float mAnimationTimer;
};

enum PuzzleMode
{
	PuzzleMode_Undefined,
	PuzzleMode_Switch,
	PuzzleMode_Free
};

class Puzzle
{
public:
	Puzzle();
	~Puzzle();
	
	void Size_set(int sx, int sy, PuzzleMode mode);
	Vec2I Size_get();
	PuzzleMode Mode_get();

	void Shuffle(int count);
	Vec2I ShuffleTarget_get();
	bool TryMove(Vec2I location);
	void Move(Vec2I location);
	void Switch(Vec2I location1, Vec2I location2);
	bool IsAdjacent(Vec2I location1, Vec2I location2);
	PuzzlePiece* Piece_get(Vec2I location);
	PuzzlePiece* FindPieceByImageLocation(Vec2I location);
	PuzzlePiece* FindPieceByLocation(Vec2I location);
	bool CheckWin();

private:
	PuzzleMode mMode;
	Vec2I mSize;
	RectI mRect;
	PuzzlePiece* mPieces;
public:
	Vec2I mFreeLocation;
	Vec2I mPrevShuffleDirection;
};
