#if USE_AGG

#include "agg_image_accessors.h"
#include "agg_pixfmt_rgba.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_renderer_scanline.h"
#include "agg_scanline_u.h"
#include "agg_span_allocator.h"
#include "agg_span_image_filter_rgba.h"
#include "agg_span_interpolator_linear.h"
#include "agg_trans_affine.h"
#include "BlitTransform.h"
#include "Calc.h"
#include "ImageResampling.h"
#include "MacImage.h"
#include "Mat3x2.h"

#define image_filter_2x2_type      agg::span_image_filter_rgba_2x2
#define image_resample_affine_type agg::span_image_resample_rgba_affine

typedef agg::pixfmt_rgba32 PixelFormat;

template <class T>
class Buffer
{
public:
	Buffer(T image)
	{
		mImage = image;
		//mRenderingBuffer.attach((agg::int8u*)mImage->Data_get(), mImage->Sx_get(), mImage->Sy_get(), mImage->Sx_get() * 4);
		mRenderingBuffer.attach((agg::int8u*)mImage->Data_get(), mImage->Sx_get(), mImage->Sy_get(), -mImage->Sx_get() * 4);
		mPixelFormat = PixelFormat(mRenderingBuffer);
	}
	
	~Buffer()
	{
		mImage = 0;
	}
	
	T mImage;
	PixelFormat mPixelFormat;
	agg::rendering_buffer mRenderingBuffer;
};

void ImageResampling::Blit_Transformed(MacImage & src, MacImage & dst, const BlitTransform & blitTransform)
{
	Mat3x2 matrix;
	
	// note: fix Y axis
	
	BlitTransform blitTransform2 = blitTransform;
//	blitTransform2.angle += Calc::mPI;
	blitTransform2.y = dst.Sy_get() - blitTransform.y;
	blitTransform2.ToMatrix(matrix);
	Mat3x2 invY;
//	invY.MakeScaling(1.0f, -1.0f);
//	matrix = invY * matrix;
	
	// create rasterizer
	
	agg::rasterizer_scanline_aa<> rasterizer;
	agg::scanline_u8 scanline;
	agg::image_filter_bilinear filterKernel;
    agg::image_filter_lut filter(filterKernel, true);
	
	// create buffers
	
	Buffer<MacImage*> srcBuffer(&src);
	Buffer<MacImage*> dstBuffer(&dst);
	
	// setup transform
	
	Vec2F points[4] =
	{
		Vec2F(0.0f, 0.0f),
		Vec2F((float)srcBuffer.mImage->Sx_get(), 0.0f),
		Vec2F((float)srcBuffer.mImage->Sx_get(), (float)srcBuffer.mImage->Sy_get()),
		Vec2F(0.0f, (float)srcBuffer.mImage->Sy_get())
	};
	
	for (int i = 0; i < 4; ++i)
	{
		points[i] = matrix * points[i];
	}
	
	double temp[8];
	for (int i = 0; i < 4; ++i)
	{
		temp[i * 2 + 0] = points[i][0];
		temp[i * 2 + 1] = points[i][1];
	}
	agg::trans_affine transform(temp, 0.0, 0.0, srcBuffer.mImage->Sx_get(), srcBuffer.mImage->Sy_get());
	
	// prepare rendering
	
	typedef agg::image_accessor_clone<PixelFormat> ImageAccessor;
	ImageAccessor imageAccessor(srcBuffer.mPixelFormat);
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

#else

#ifdef GCC
#warning USE_AGG not set
#endif

#include "Exception.h"
#include "ImageResampling.h"

void ImageResampling::Blit_Transformed(MacImage& src, MacImage& dst, const BlitTransform& blitTransform)
{
	throw ExceptionNA();
}

#endif
