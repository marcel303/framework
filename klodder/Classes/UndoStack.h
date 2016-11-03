#pragma once

#include <deque>
#include <vector>
#include "Bitmap.h"
#include "MacImage.h"

// two options:
// - naive implementation: take a snapshot after every action
// - optimized: take a snapshot, but only the deltas
//   - requires special support for redo feature

class UndoState
{
public:
	UndoState();
	
	void SetReplay();
	void SetDataLayerImage(int imageIndex, int index, const MacImage* image, int x, int y, int sx, int sy);
	void SetCommandStreamLocation(size_t location);
	void SetDataStreamLocation(size_t location);
	void SetDataLayerOpacity(int index, float opacity);
	void SetLayerOrder(std::vector<int> order);
	void SetEditingDataLayer(int index);
	void SetDataLayerClear(int index, Rgba color);
	void SetDataLayerMerge(int index1, int index2);
	void SetDataLayerVisibility(int index, bool visibility);

#if KLODDER_LITE==0
	void DBG_SetEditingDataLayer(int index);
#else
	void DBG_SetEditingDataLayer(int index) { }
#endif
	
	int EstimateByteCount_get() const;
	
	bool mHasReplay;

	struct DataLayerImage
	{
		bool mHasImage;
		int mImageDataLayer;
		MacImage mImage;
		Vec2I mImageLocation;
	};

	DataLayerImage mDataLayerImage[2];

	bool mHasCommandStreamLocation;
	size_t mCommandStreamLocation;
	
	bool mHasDataStreamLocation;
	size_t mDataStreamLocation;
	
	bool mHasLayerOrder;
	std::vector<int> mLayerOrder;
	
	bool mHasEditingDataLayer;
	int mEditingDataLayer;
	
	bool mHasLayerClear;
	int mLayerClearDataLayer;
	Rgba mLayerClearColor;
	
	bool mHasLayerMerge;
	int mLayerMergeDataLayer1;
	int mLayerMergeDataLayer2;
	
	bool mHasLayerOpacity;
	int mLayerOpacityIndex;
	float mLayerOpacityValue;
	
	bool mHasLayerVisibility;
	int mLayerVisibilityIndex;
	bool mLayerVisibilityValue;
	
#if KLODDER_LITE==0
	bool mHasDbgEditingDataLayer;
	int mDbgEditingDataLayer;
#endif
};

class UndoBuffer
{
public:
	UndoBuffer();
	~UndoBuffer();
	
	int EstimateByteCount_get() const;
	
	UndoState mPrev;
	UndoState mNext;
};

class UndoStack
{
public:
	UndoStack();
	~UndoStack();
	void Initialize(int maxDepth, int maxByteCount);
	
	void Commit(UndoBuffer* undo);
	void Clear();
	
	bool HasUndo_get() const;
	bool HasRedo_get() const;
	
	UndoBuffer* GetPrevBuffer();
	
	void Seek(int direction);
	
private:
	bool AreConstraintsSatisfied() const;
	void PurgeTillConstrained();
	void PurgeFuture();
	void UpdateStats(UndoBuffer* buffer, int count);
	void RemoveHead();
	void RemoveTail();
	
	// constraints
	int mMaxDepth;
	int mMaxByteCount;
	
	// stack
	std::deque<UndoBuffer*> mUndoStack;
	size_t mUndoLocation;
	
	// stats
	int mByteCount;
};
