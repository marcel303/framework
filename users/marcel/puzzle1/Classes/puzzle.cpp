#include "Calc.h"
#include "Log.h"
#include "puzzle.h"

Puzzle::Puzzle()
{
	mMode = PuzzleMode_Undefined;
	mPieces = 0;
}

Puzzle::~Puzzle()
{
	Size_set(0, 0, PuzzleMode_Undefined);
}

void Puzzle::Size_set(int sx, int sy, PuzzleMode mode)
{
	LOG_DBG("puzzle: size_set: %dx%d", sx, sy);
	
	mMode = PuzzleMode_Undefined;
	delete[] mPieces;
	mPieces = 0;
	mSize.SetZero();
	
	if (sx * sy > 0)
	{
		mMode = mode;
		mPieces = new PuzzlePiece[sx * sy];
		mSize.Set(sx, sy);
		mRect.Setup(Vec2I(), mSize - Vec2I(1, 1));
		mFreeLocation.Set(0, 0);
		
		for (int y = 0; y < sy; ++y)
		{
			for (int x = 0; x < sx; ++x)
			{
				Vec2I location(x, y);
				
				PuzzlePiece* piece = Piece_get(location);
				
				piece->Setup(location);
			}
		}
		
		Piece_get(mFreeLocation)->mIsFree = true;
	}
}

Vec2I Puzzle::Size_get()
{
	return mSize;
}

PuzzleMode Puzzle::Mode_get()
{
	return mMode;
}

void Puzzle::Shuffle(int count)
{
	LOG_DBG("puzzle: shuffle", 0);
	
	while (count > 0)
	{
		Vec2I next = ShuffleTarget_get();
		
		Move(next);
		
		--count;
	}
}

Vec2I Puzzle::ShuffleTarget_get()
{
	Assert(mSize[0] * mSize[1] > 1);
	
	while (true)
	{
		Vec2I direction(Calc::Random(-1, +1), Calc::Random(-1, +1));
		
		if (direction.DistanceSq(-mPrevShuffleDirection) == 0)
			continue;
		
		Vec2I destination = mFreeLocation + direction;

		if (!mRect.IsInside(destination))
			continue;
		if (mFreeLocation.DistanceSq(destination) != 1)
			continue;
		
		mPrevShuffleDirection = direction;
		
		return destination;
	}
}

bool Puzzle::TryMove(Vec2I location)
{
	bool adjacent = IsAdjacent(location, mFreeLocation);
	
	if (adjacent)
		Move(location);
	
	return adjacent;
}

void Puzzle::Move(Vec2I location)
{
	Switch(location, mFreeLocation);
	
	/*LOG_DBG("puzzle: move: %d,%d", location[0], location[1]);
	
	PuzzlePiece* piece1 = FindPieceByLocation(mFreeLocation);
	PuzzlePiece* piece2 = FindPieceByLocation(location);
	
	std::swap(piece1->mLocation, piece2->mLocation);
	piece1->mLocationPrev = piece2->mLocation;
	piece2->mLocationPrev = piece1->mLocation;*/
	
	mFreeLocation = location;
}

void Puzzle::Switch(Vec2I location1, Vec2I location2)
{
	LOG_DBG("puzzle: switch: %d,%d - %d,%d", location1[0], location1[1], location2[0], location2[1]);
	
	PuzzlePiece* piece1 = FindPieceByLocation(location1);
	PuzzlePiece* piece2 = FindPieceByLocation(location2);
	
	std::swap(piece1->mLocation, piece2->mLocation);
	piece1->mLocationPrev = piece2->mLocation;
	piece2->mLocationPrev = piece1->mLocation;
}

bool Puzzle::IsAdjacent(Vec2I location1, Vec2I location2)
{
	int delta = (location2 - location1).LengthSq_get();
	
	return delta == 1;
}

PuzzlePiece* Puzzle::Piece_get(Vec2I location)
{
	int index = location[0] + location[1] * mSize[0];
	
	return &mPieces[index];
}

PuzzlePiece* Puzzle::FindPieceByImageLocation(Vec2I location)
{
	for (int i = 0; i < mSize[0] * mSize[1]; ++i)
	{
		if (mPieces[i].mImageLocation[0] == location[0] && mPieces[i].mImageLocation[1] == location[1])
			return &mPieces[i];
	}
	
	LOG_DBG("puzzle: piece not found", 0);
	
	return 0;
}

PuzzlePiece* Puzzle::FindPieceByLocation(Vec2I location)
{
	for (int i = 0; i < mSize[0] * mSize[1]; ++i)
	{
		if (mPieces[i].mLocation[0] == location[0] && mPieces[i].mLocation[1] == location[1])
			return &mPieces[i];
	}
	
	LOG_DBG("puzzle: piece not found", 0);
	
	return 0;
}

bool Puzzle::CheckWin()
{
	for (int i = 0; i < mSize[0] * mSize[1]; ++i)
	{
		if (mPieces[i].mLocation[0] != mPieces[i].mImageLocation[0] || mPieces[i].mLocation[1] != mPieces[i].mImageLocation[1])
			return false;
	}
	
	return true;
}
