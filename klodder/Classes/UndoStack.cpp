#include "ImageConversion.h"
#include "UndoStack.h"

#define VALIDATE_LOCATION(location) Assert(location >= 0 && location <= mUndoStack.size())

UndoState::UndoState()
{
	mHasImage = false;
	mImageDataLayer = 0;
	
	mHasDualImage = false;
	mDualImageDataLayer1 = 0;
	mDualImageDataLayer2 = 0;
	
	mHasCommandStreamLocation = false;
	mCommandStreamLocation = 0;
	
	mHasDataStreamLocation = false;
	mDataStreamLocation = 0;
	
	mHasLayerOrder = false;
	
	mHasActiveDataLayer = false;
	mActiveDataLayer = 0;
	
	mHasLayerClear = false;
	mLayerClearDataLayer = 0;
	
	mHasLayerMerge = false;
	mLayerMergeDataLayer1 = 0;
	mLayerMergeDataLayer2 = 0;
	
	mHasLayerOpacity = false;
	mLayerOpacityIndex = 0;
	mLayerOpacityValue = 0.0f;
	
	mHasLayerVisibility = false;
	mLayerVisibilityIndex = 0;
	mLayerVisibilityValue = false;
	
	mHasDbgActiveDataLayer = false;
	mDbgActiveDataLayer = 0;
}

void UndoState::SetDataLayerImage(int index, MacImage* image, int x, int y, int sx, int sy)
{
	Assert(!mHasImage);
	Assert(sx > 0);
	Assert(sy > 0);
	mHasImage = true;
	mImageDataLayer = index;
	//BitmapToMacImage(image, &mImage, x, y, sx, sy);
	mImage.Size_set(sx, sy, false);
	image->ExtractTo(&mImage, x, y, sx, sy);
	mImageLocation.Set(x, y);
}

void UndoState::SetDataLayerDualImage(int index1, MacImage* image1, int index2, MacImage* image2)
{
	Assert(!mHasDualImage);
	mHasDualImage = true;
	Assert(index1 >= 0);
	Assert(index2 >= 0);
	Assert(image1->Sx_get() == image2->Sx_get());
	Assert(image1->Sy_get() == image2->Sy_get());
	mDualImageDataLayer1 = index1;
	mDualImage1.Size_set(image1->Sx_get(), image1->Sy_get(), false);
	image1->Blit(&mDualImage1);
	mDualImageDataLayer2 = index2;
	mDualImage2.Size_set(image2->Sx_get(), image2->Sy_get(), false);
	image2->Blit(&mDualImage2);
}

void UndoState::SetCommandStreamLocation(size_t location)
{
	Assert(!mHasCommandStreamLocation);
	mHasCommandStreamLocation = true;
	mCommandStreamLocation = location;
}

void UndoState::SetDataStreamLocation(size_t location)
{
	Assert(!mHasDataStreamLocation);
	mHasDataStreamLocation = true;
	mDataStreamLocation = location;
}

void UndoState::SetDataLayerOpacity(int index, float opacity)
{
	Assert(!mHasLayerOpacity);
	mHasLayerOpacity = true;
	mLayerOpacityIndex = index;
	mLayerOpacityValue = opacity;
}

void UndoState::SetLayerOrder(std::vector<int> order)
{
	Assert(!mHasLayerOrder);
	mHasLayerOrder = true;
	mLayerOrder = order;
}

void UndoState::SetActiveDataLayer(int index)
{
	Assert(!mHasActiveDataLayer);
	mHasActiveDataLayer = true;
	mActiveDataLayer = index;
}

void UndoState::SetDataLayerClear(int index, Rgba color)
{
	Assert(!mHasLayerClear);
	mHasLayerClear = true;
	mLayerClearDataLayer = index;
	mLayerClearColor = color;
}

void UndoState::SetDataLayerMerge(int index1, int index2)
{
	Assert(!mHasLayerMerge);
	mHasLayerMerge = true;
	mLayerMergeDataLayer1 = index1;
	mLayerMergeDataLayer2 = index2;
}

void UndoState::SetDataLayerVisibility(int index, bool visibility)
{
	Assert(!mHasLayerVisibility);
	mHasLayerVisibility = true;
	mLayerVisibilityIndex = index;
	mLayerVisibilityValue = visibility;
}

void UndoState::DBG_SetActiveDataLayer(int index)
{
	Assert(!mHasDbgActiveDataLayer);
	mHasDbgActiveDataLayer = true;
	mDbgActiveDataLayer = index;
}

int UndoState::EstimateByteCount_get() const
{
	int result = sizeof(UndoState) + 1000;
	
	if (mHasImage)
		result += mImage.Sx_get() * mImage.Sy_get() * sizeof(MacRgba);
	
	if (mHasDualImage)
	{
		result += mDualImage1.Sx_get() * mDualImage1.Sy_get() * sizeof(MacRgba);
		result += mDualImage1.Sx_get() * mDualImage1.Sy_get() * sizeof(MacRgba);
	}
	
	return result;
}

//

UndoBuffer::UndoBuffer()
{
}

UndoBuffer::~UndoBuffer()
{
}

int UndoBuffer::EstimateByteCount_get() const
{
	return mPrev.EstimateByteCount_get() + mNext.EstimateByteCount_get();
}

//

UndoStack::UndoStack()
{
	mMaxDepth = 0;
	mMaxByteCount = 0;
	mByteCount = 0;
	mUndoLocation = 0;
}

UndoStack::~UndoStack()
{
	Clear();
}

void UndoStack::Initialize(int maxDepth, int maxByteCount)
{
	mMaxDepth = maxDepth;
	mMaxByteCount = maxByteCount;
	
	mUndoLocation = 0;
}

void UndoStack::Commit(UndoBuffer* undo)
{
	VALIDATE_LOCATION(mUndoLocation);
	
	// remove undo buffers beyond current seek location
	
	PurgeFuture();
	
	// add buffer
	
	mUndoStack.push_back(undo);
	
	// update seek location
	
	mUndoLocation = mUndoStack.size();
	
	// update statistics
	
	UpdateStats(undo, +1);
	
	// make sure undo stack doesn't grow too large
	
	PurgeTillConstrained();
}

void UndoStack::Clear()
{
	while (mUndoStack.size() > 0)
		RemoveTail();
	
	Assert(mUndoLocation == 0);
}

bool UndoStack::HasUndo_get() const
{
	VALIDATE_LOCATION(mUndoLocation);
	
	return mUndoLocation > 0;
}

bool UndoStack::HasRedo_get() const
{
	VALIDATE_LOCATION(mUndoLocation);
	
	return mUndoLocation < mUndoStack.size();
}

UndoBuffer* UndoStack::GetPrevBuffer()
{
	Assert(mUndoStack.size() > 0);
	
	const size_t location = mUndoLocation - 1;
	
	VALIDATE_LOCATION(location);
	
	return mUndoStack[location];
}

void UndoStack::Seek(int direction)
{
	Assert(direction == -1 || direction == +1);
	
	const size_t location = mUndoLocation + direction;
	
	VALIDATE_LOCATION(location);
	
	mUndoLocation = location;
}

bool UndoStack::AreConstraintsSatisfied() const
{
	return mByteCount < mMaxByteCount;
}

void UndoStack::PurgeTillConstrained()
{
	while (!AreConstraintsSatisfied() && mUndoLocation > 0)
	{
		LOG_DBG("enforcing purge due to size/depth constraint", 0);
		
		RemoveHead();
	}
}

void UndoStack::PurgeFuture()
{
	while (mUndoLocation < mUndoStack.size())
	{
		RemoveTail();
	}
}

void UndoStack::UpdateStats(UndoBuffer* buffer, int count)
{
	// update statistics

	int byteCount = buffer->EstimateByteCount_get();
	
	mByteCount += byteCount * count;
	
	LOG_DBG("stats: byteCount: %d", mByteCount);
}

void UndoStack::RemoveHead()
{
	Assert(mUndoStack.size() > 0);
	Assert(mUndoLocation > 0);
	
	// free buffer
	
	UndoBuffer* buffer = mUndoStack.front();
	
	UpdateStats(buffer, -1);
	
	delete buffer;
	
	mUndoStack.pop_front();
	
	// move undo location
	
	mUndoLocation--;
	
	VALIDATE_LOCATION(mUndoLocation);
}

void UndoStack::RemoveTail()
{
	Assert(mUndoStack.size() > 0);
	
	// free buffer
	
	UndoBuffer* buffer = mUndoStack.back();
	
	UpdateStats(buffer, -1);
	
	delete buffer;
	
	mUndoStack.pop_back();
	
	// clamp undo location
	
	if (mUndoLocation > mUndoStack.size())
		mUndoLocation = mUndoStack.size();
	
	VALIDATE_LOCATION(mUndoLocation);
}
