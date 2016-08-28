#pragma once

#include <vector>
#include "Bitmap.h"
#include "Filter.h"
#include "MacImage.h"
#include "Types.h"

#define MAX_LAYERS 3

enum LayerMode
{
	LayerMode_Undefined,
	LayerMode_Brush,
	LayerMode_Eraser,
	LayerMode_Direct
};

extern void RenderCheckerBoard(MacImage & __restrict dst, const MacRgba & backColor1, const MacRgba & backColor2, const uint32_t size);

class LayerMgr
{
public:
	LayerMgr();

	void Setup(const int layerCount, const int sx, const int sy, const Rgba & backColor1, const Rgba & backColor2);
	
	void RenderMerged(MacImage & __restrict dst, const MacRgba & backColor1, const MacRgba & backColor2, const int layerBegin, const int layerEnd);
	void RenderMergedFinal(MacImage & __restrict dst);
	
	// editing
	
	void SetMode_Brush();
	void SetMode_Eraser();
	void SetMode_Direct();
	
	void FlattenBrush(const RectI & rect);
	void FlattenEraser(const RectI & rect);
	void ClearBrushOverlay(const RectI & rect);
private:
	void DataLayerAcquire(const int index, const MacImage * image);
public:
	void DataLayerAcquireWithTransform(const int index, MacImage * image, const BlitTransform & transform);
	void DataLayerClear(const int index, const Rgba & color);
	void DataLayerMerge(const int index1, const int index2); // merges layer1 atop of layer2 and stores the result int layer2. layer1 gets cleared

	void Invalidate(const int x, const int y, const int sx, const int sy);
	AreaI Validate();
	
	// properties
	
	Vec2I Size_get() const;
	MacImage * DataLayer_get(const int index);
	MacImage * Layer_get(const int layer);
	int LayerCount_get() const;
	void LayerOrder_set(const std::vector<int> & order);
	std::vector<int> LayerOrder_get() const;
	int LayerOrder_get(const int layer) const;
	void DataLayerOpacity_set(const int index, const float opacity);
	float DataLayerOpacity_get(const int index) const;
	void DataLayerVisibility_set(const int index, const bool visibility);
	bool DataLayerVisibility_get(const int index) const;
	MacImage * Merged_get();
	Bitmap * EditingBuffer_get();
	const Bitmap * EditingBuffer_get() const;
	Filter * EditingBrush_get();
	const Rgba & BrushColor_get() const;
	void BrushColor_set(const Rgba & color);
	float BrushOpacity_get() const;
	void BrushOpacity_set(const float opacity);
	MacRgba BackColor1_get() const;
	MacRgba BackColor2_get() const;

	void ValidateVisible(const int x, const int y, const int sx, const int sy);
	void ValidateLayer(const int x, const int y, const int sx, const int sy);
	void CopyLayerToEditingBuffer(const int x, const int y, const int sx, const int sy);
	void CopyEditingBufferToLayer(const int x, const int y, const int sx, const int sy);
	void ActiveDataLayer_set(const int index);
	int ActiveDataLayer_get() const;
//	void SwapLayerOrder(int layer1, int layer2);
private:// helpers
	void RebuildCaches();
	void RebuildBack();
	void RebuildFront();
public:
	void EditingBegin(const bool rebuildCaches); // rebuild caches should be true if layer data other than the active layer has been changed
	void EditingEnd();
	bool EditingIsEnabled_get() const;
	inline AreaI & EditingDirty_get() { return mEditingDirty; }

private:
	int IndexToLayer(const int index) const;
	int LayerToIndex(const int layer) const;
	
	// layers
	int mLayerCount;
	int mActiveDataLayer;
	MacImage mLayerList[MAX_LAYERS];
	int mLayerOpacity[MAX_LAYERS];
	int mLayerOrder[MAX_LAYERS];
	bool mLayerVisibility[MAX_LAYERS];
	MacRgba mBackColor1;
	MacRgba mBackColor2;
	MacImage mCacheBack;
	MacImage mCacheFront;

	// editing
	bool mEditingEnabled;
	LayerMode mMode;
	MacImage mMerged;
	Bitmap mEditingBuffer;
	Filter mBmpBrush;
	Rgba mBrushColor;
	float mBrushOpacity;
	int mBrushOpacity255i;
	float mBrushOpacity255f;
	AreaI mEditingDirty;
};
