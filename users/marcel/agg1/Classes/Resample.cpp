#include "agg_image_accessors.h"
#include "agg_pixfmt_rgba.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_renderer_scanline.h"
#include "agg_scanline_u.h"
#include "agg_span_allocator.h"
#include "agg_span_image_filter_rgba.h"
#include "agg_span_interpolator_linear.h"
#include "agg_trans_affine.h"

#include <stdio.h>
#include "Resample.h"
#include "MacImage.h"
#include "Mat3x2.h"

#define image_filter_2x2_type      agg::span_image_filter_rgba_2x2
#define image_resample_affine_type agg::span_image_resample_rgba_affine

typedef agg::pixfmt_rgba32 PixelFormat;

class Buffer
{
public:
	Buffer(MacImage* image)
	{
		mImage = image;
		mRenderingBuffer.attach((agg::int8u*)mImage->Data_get(), mImage->Sx_get(), mImage->Sy_get(), mImage->Sx_get() * 4);
		mPixelFormat = PixelFormat(mRenderingBuffer);
	}
	
	~Buffer()
	{
		mImage = 0;
	}
	
	MacImage* mImage;
	PixelFormat mPixelFormat;
	agg::rendering_buffer mRenderingBuffer;
};

void Resample(MacImage* src, MacImage* dst)
{
	static float t = 0.0f;
	t += 0.06f;
	float angle = t;
	float scale = 1.0f - sinf(t * 2.0f) * 0.8f;
	
	Mat3x2 matT1;
	Mat3x2 matR;
	Mat3x2 matS;
	Mat3x2 matT2;
	matT1.MakeTranslation(Vec2F(dst->Sx_get() / 2.0f, dst->Sy_get() / 2.0f));
	matR.MakeRotation(angle);
	matS.MakeScaling(scale, scale);
	matT2.MakeTranslation(Vec2F(-src->Sx_get() / 2.0f, -src->Sy_get() / 2.0f));
	Mat3x2 mat = matT1 * matR * matS * matT2;
	
	// create rasterizer
	
	agg::rasterizer_scanline_aa<> rasterizer;
	agg::scanline_u8 scanline;
	agg::image_filter_bilinear filterKernel;
    agg::image_filter_lut filter(filterKernel, true);
	
	// create buffers
	
	Buffer dstBuffer(dst);
	Buffer texBuffer(src);
	
	// setup transform
	
	Vec2F points[4] =
	{
		Vec2F(0.0f, 0.0f),
		Vec2F(texBuffer.mImage->Sx_get(), 0.0f),
		Vec2F(texBuffer.mImage->Sx_get(), texBuffer.mImage->Sy_get()),
		Vec2F(0.0f, texBuffer.mImage->Sy_get())
	};
	
	for (int i = 0; i < 4; ++i)
	{
		points[i] = mat * points[i];
	}
	
	float temp[8];
	for (int i = 0; i < 4; ++i)
	{
		temp[i * 2 + 0] = points[i][0];
		temp[i * 2 + 1] = points[i][1];
	}
	agg::trans_affine transform(temp, 0.0, 0.0, texBuffer.mImage->Sx_get(), texBuffer.mImage->Sy_get());
	
	// prepare rendering
	
	typedef agg::image_accessor_clone<PixelFormat> ImageAccessor;
	ImageAccessor imageAccessor(texBuffer.mPixelFormat);
	typedef PixelFormat::color_type Color;
		
	typedef agg::renderer_base<PixelFormat> RendererBase;
	RendererBase rendererBase(dstBuffer.mPixelFormat);
	
	//rendererBase.clear(agg::rgba(2, 2, 2, 255));
	
	typedef agg::span_allocator<Color> SpanAllocator;
	SpanAllocator spanAllocator;

	typedef agg::span_interpolator_linear<agg::trans_affine> Interpolator;
	Interpolator interpolator(transform);
	
	typedef image_resample_affine_type<ImageAccessor> SpanGenerator;
//	typedef image_filter_2x2_type<ImageAccessor, Interpolator> SpanGenerator;
	SpanGenerator spanGenerator(imageAccessor, interpolator, filter);
	
	// render
	
	rasterizer.clip_box(0.0, 0.0, dstBuffer.mImage->Sx_get(), dstBuffer.mImage->Sy_get());
	rasterizer.reset();
	
	rasterizer.move_to_d(points[0][0], points[0][1]);
	rasterizer.line_to_d(points[1][0], points[1][1]);
	rasterizer.line_to_d(points[2][0], points[2][1]);
	rasterizer.line_to_d(points[3][0], points[3][1]);
	
	agg::render_scanlines_aa(rasterizer, scanline, rendererBase, spanAllocator, spanGenerator);
}
