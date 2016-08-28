#pragma once

#ifdef IPHONEOS
	#include <CoreGraphics/CGImage.h>
#endif
#include <stdint.h>
#include "klodder_forward.h"
#include "libgg_forward.h"
#include "Types.h"

typedef struct MacRgba
{
	uint8_t rgba[4];
} MacRgba;

inline MacRgba MacRgba_Make(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a)
{
	MacRgba result = { { r, g, b, a } };
	
	return result;
}

class MacImage
{
public:
	MacImage();
	~MacImage();
	
	void Clear(const MacRgba & color);
	void ExtractTo(MacImage * dst, const int x, const int y, const int sx, const int sy) const;
	void Blit(MacImage * dst) const;
	void Blit(MacImage * dst, const int dpx, const int dpy) const;
	void Blit(MacImage * dst, const int spx, const int spy, const int dpx, const int dpy, const int sx, const int sy) const;
	void BlitAlpha(MacImage * dst, const int opacity) const;
	void BlitAlpha(MacImage * dst, const int spx, const int spy, const int dpx, const int dpy, const int sx, const int sy) const;
	void BlitAlpha(MacImage * dst, const int opacity, const int spx, const int spy, const int dpx, const int dpy, const int sx, const int sy) const;
#ifdef USE_CXIMAGE
	void Blit_Resampled(MacImage * dst, const bool includeAlpha) const;
#endif
#if USE_AGG
	void Blit_Transformed(MacImage * dst, const BlitTransform & transform);
#endif
	void Downsample(MacImage * dst, const int scale) const;
	MacImage * FlipY() const;
	void FlipY_InPlace();
	
	void Save(Stream * stream) const;
	void Load(Stream * stream);

	void Size_set(const int sx, const int sy, const bool clear);
	Vec2I Size_get() const;
	
	inline MacRgba * Line_get(const int y);
	inline const MacRgba * Line_get(const int y) const;
	inline int Sx_get() const;
	inline int Sy_get() const;
	inline MacRgba * Data_get();

#ifdef IPHONEOS
	CGImageRef Image_get();
	CGImageRef ImageWithAlpha_get() const;
#endif
	
private:
#ifdef IPHONEOS
	CGImageRef CreateImage();
#endif
	
	MacRgba * mData;
	int mSx;
	int mSy;
#ifdef IPHONEOS
	CGImageRef mImage;
#endif
};

inline MacRgba * MacImage::Line_get(const int y)
{
	return mData + mSx * (mSy - 1 - y);
}

inline const MacRgba * MacImage::Line_get(const int y) const
{
	return mData + mSx * (mSy - 1 - y);
}

inline int MacImage::Sx_get() const
{
	return mSx;
}

inline int MacImage::Sy_get() const
{
	return mSy;
}

inline MacRgba * MacImage::Data_get()
{
	return mData;
}

