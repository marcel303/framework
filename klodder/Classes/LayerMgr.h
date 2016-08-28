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

extern void RenderCheckerBoard(MacImage & __restrict dst, MacRgba backColor1, MacRgba backColor2, uint32_t size);

class LayerMgr
{
public:
	LayerMgr();

	void Setup(int layerCount, int sx, int sy, Rgba backColor1, Rgba backColor2);
	
	void RenderMerged(MacImage& dst, MacRgba backColor1, MacRgba backColor2, int layerBegin, int layerEnd);
	void RenderMergedFinal(MacImage& dst);
	
	// editing
	
	void SetMode_Brush();
	void SetMode_Eraser();
	void SetMode_Direct();
	
	void FlattenBrush(RectI rect);
	void FlattenEraser(RectI rect);
	void ClearBrushOverlay(RectI rect);
private:
	void DataLayerAcquire(int index, const MacImage* image);
public:
	void DataLayerAcquireWithTransform(int index, MacImage* image, const BlitTransform& transform);
	void DataLayerClear(int index, Rgba color);
	void DataLayerMerge(int index1, int index2); // merges layer1 atop of layer2 and stores the result int layer2. layer1 gets cleared

	void Invalidate(int x, int y, int sx, int sy);
	AreaI Validate();
	
	// properties
	
	Vec2I Size_get() const;
	MacImage* DataLayer_get(int index);
	MacImage* Layer_get(int layer);
	int LayerCount_get() const;
	void LayerOrder_set(std::vector<int> order);
	std::vector<int> LayerOrder_get() const;
	int LayerOrder_get(int layer) const;
	void DataLayerOpacity_set(int index, float opacity);
	float DataLayerOpacity_get(int index) const;
	void DataLayerVisibility_set(int index, bool visibility);
	bool DataLayerVisibility_get(int index) const;
	MacImage* Merged_get();
	Bitmap* EditingBuffer_get();
	const Bitmap* EditingBuffer_get() const;
	Filter* EditingBrush_get();
	const Rgba & BrushColor_get() const;
	void BrushColor_set(const Rgba & color);
	float BrushOpacity_get() const;
	void BrushOpacity_set(float opacity);
	MacRgba BackColor1_get() const;
	MacRgba BackColor2_get() const;

	void ValidateVisible(int x, int y, int sx, int sy);
	void ValidateLayer(int x, int y, int sx, int sy);
	void CopyLayerToEditingBuffer(int x, int y, int sx, int sy);
	void CopyEditingBufferToLayer(int x, int y, int sx, int sy);
	void ActiveDataLayer_set(int index);
	int ActiveDataLayer_get() const;
//	void SwapLayerOrder(int layer1, int layer2);
private:// helpers
	void RebuildCaches();
	void RebuildBack();
	void RebuildFront();
public:
	void EditingBegin(bool rebuildCaches); // rebuild caches should be true if layer data other than the active layer has been changed
	void EditingEnd();
	bool EditingIsEnabled_get() const;
	inline AreaI& EditingDirty_get() { return mEditingDirty; }

private:
	int IndexToLayer(int index) const;
	int LayerToIndex(int layer) const;
	
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
