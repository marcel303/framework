#pragma once

#ifdef IPHONEOS
	#include <CoreGraphics/CGImage.h>
#endif
#ifdef WIN32
#if 0
	#include <allegro.h>
#endif
#endif
#include <stdint.h>
#include "klodder_forward.h"
#include "libgg_forward.h"
#include "Types.h"

typedef struct MacRgba
{
	uint8_t rgba[4];
} MacRgba;

inline MacRgba MacRgba_Make(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	MacRgba result = { { r, g, b, a } };
	
	return result;
}

class MacImage
{
public:
	MacImage();
	~MacImage();
	
	void Clear(MacRgba color);
	void ExtractTo(MacImage* dst, int x, int y, int sx, int sy) const;
	void Blit(MacImage* dst) const;
	void Blit(MacImage* dst, int dpx, int dpy) const;
	void Blit(MacImage* dst, int spx, int spy, int dpx, int dpy, int sx, int sy) const;
	void BlitAlpha(MacImage* dst, int opacity) const;
	void BlitAlpha(MacImage* dst, int spx, int spy, int dpx, int dpy, int sx, int sy) const;
	void BlitAlpha(MacImage* dst, int opacity, int spx, int spy, int dpx, int dpy, int sx, int sy) const;
#ifdef USE_CXIMAGE
	void Blit_Resampled(MacImage* dst, bool includeAlpha) const;
#endif
#if USE_AGG
	void Blit_Transformed(MacImage* dst, const BlitTransform& transform);
#endif
	void Downsample(MacImage* dst, int scale) const;
	MacImage* FlipY() const;
	void FlipY_InPlace();
	
	void Save(Stream* stream) const;
	void Load(Stream* stream);

	void Size_set(int sx, int sy, bool clear);
	Vec2I Size_get() const;
	
	inline MacRgba* Line_get(int y);
	inline const MacRgba* Line_get(int y) const;
	inline int Sx_get() const;
	inline int Sy_get() const;
	inline MacRgba* Data_get();

#ifdef IPHONEOS
	CGImageRef Image_get();
	CGImageRef ImageWithAlpha_get() const;
#endif
	
private:
#ifdef IPHONEOS
	CGImageRef CreateImage();
#endif
	
	MacRgba* mData;
	int mSx;
	int mSy;
#ifdef IPHONEOS
	CGImageRef mImage;
#endif
};

inline MacRgba* MacImage::Line_get(int y)
{
	return mData + mSx * (mSy - 1 - y);
}

inline const MacRgba* MacImage::Line_get(int y) const
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

inline MacRgba* MacImage::Data_get()
{
	return mData;
}

