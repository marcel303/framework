#include "renderDrawableList.h"

#include "Log.h"

static void testDrawableListSort()
{
	RenderDrawableAllocator allocator(8 * 1024);
	
	RenderDrawableList drawableList;
	
	drawableList.captureBegin(Mat4x4(true), &allocator);
	{
		for (int i = 0; i < 10; ++i)
		{
			drawableList.add(nullptr)
				.viewZ((rand() % 1000) / 100.f - 5.f);
		}
	}
	drawableList.captureEnd();
	
	LOG_DBG("sort using IncreasingViewZ..");
	drawableList.sort<RenderDrawableCompare_IncreasingViewZ>();
	for (auto * drawable = drawableList.drawable_head; drawable != nullptr; drawable = drawable->next)
		LOG_DBG("viewZ: %.2f", drawable->viewZ);
	
	LOG_DBG("sort using DecreasingViewZ..");
	drawableList.sort<RenderDrawableCompare_DecreasingViewZ>();
	for (auto * drawable = drawableList.drawable_head; drawable != nullptr; drawable = drawable->next)
		LOG_DBG("viewZ: %.2f", drawable->viewZ);
	
	LOG_DBG("done");
	
	allocator.Reset();
}
