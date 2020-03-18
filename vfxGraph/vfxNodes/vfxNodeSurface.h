/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "vfxNodeBase.h"

class Surface;

struct VfxNodeSurface : VfxNodeBase
{
	enum Format
	{
		kFormat_RGBA8,
		kFormat_RGBA16F
	};
	
	enum ViewMode
	{
		kViewMode_Screen,
		kViewMode_Perspective
	};
	
	enum Input
	{
		kInput_Draw,
		kInput_Format,
		kInput_Width,
		kInput_Height,
		kInput_Clear,
		kInput_ClearColor,
		kInput_DepthClear,
		kInput_DepthValue,
		kInput_Darken,
		kInput_DarkenColor,
		kInput_Multiply,
		kInput_MultiplyColor,
		kInput_ViewMode,
		kInput_FOV,
		kInput_ZNear,
		kInput_ZFar,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};

	Surface * surface;
	
	mutable Surface * oldSurface;
	
	mutable VfxImage_Texture imageOutput;
	
	VfxNodeSurface();
	virtual ~VfxNodeSurface() override;
	
	void allocSurface(const int sx, const int sy, const SURFACE_FORMAT format, const bool withDepthBuffer);
	void freeSurface();
	
	virtual void tick(const float dt) override;
	
	virtual void customTraverseDraw(const int traversalId) const override;
	virtual void beforeDraw() const override;
	virtual void afterDraw() const override;
	
	virtual const VfxImageBase * getImage() const override { return &imageOutput; }
};
