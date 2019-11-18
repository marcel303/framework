#include "opticalFlow.h"

void OpticalFlow::init(const int sx, const int sy)
{
	for (int i = 0; i < 2; ++i)
	{
		// note : the luminance map is double buffered so it can be post processed/blurred
		
		luminance[i].init(sx, sy, SURFACE_R16F /* 16F so we get extra precision out of doing the gaussian blur */, false, true);
		luminance[i].setName("OpticalFlow.Luminance");
		luminance[i].clear();
		
		sobel[i].init(sx, sy, SURFACE_RG8, false, false);
		sobel[i].setName("OpticalFlow.Sobel");
		sobel[i].clear(127, 127, 0, 0);
	}
	
	opticalFlow.init(sx, sy, SURFACE_RG16F, false, false);
	opticalFlow.setName("OpticalFlow.flow");
	opticalFlow.clear();
}

void OpticalFlow::shut()
{
	for (int i = 0; i < 2; ++i)
	{
		luminance[i].free();
		
		sobel[i].free();
	}

	opticalFlow.free();
}

void OpticalFlow::update(const GxTextureId source)
{
	// convert source image into luminance map
	
	current_luminance = (current_luminance + 1) % 2;
	
	pushSurface(&luminance[current_luminance]);
	{
		pushBlend(BLEND_OPAQUE);
		Shader shader("filter-rgbToLuminance");
		setShader(shader);
		{
			shader.setTexture("source", 0, source, false, true);
			drawRect(0, 0,
				luminance[current_luminance].getWidth(),
				luminance[current_luminance].getHeight());
		}
		clearShader();
		popBlend();
	}
	popSurface();
	
#if 1 // todo : fix issue with Metal where shader buffer params are not set
	// blur the luminance map
	
	pushBlend(BLEND_OPAQUE);
	{
		setShader_GaussianBlurH(luminance[current_luminance].getTexture(), 11, sourceFilter.blurRadius);
		luminance[current_luminance].postprocess();
		clearShader();
		
		setShader_GaussianBlurV(luminance[current_luminance].getTexture(), 11, sourceFilter.blurRadius);
		luminance[current_luminance].postprocess();
		clearShader();
	}
	popBlend();
#endif
	
	// apply horizontal + vertical sobel filter
	
	current_sobel = (current_sobel + 1) % 2;
	
	pushSurface(&sobel[current_sobel]);
	{
		pushBlend(BLEND_OPAQUE);
		Shader shader("filter-sobel");
		setShader(shader);
		{
			shader.setTexture("source", 0, luminance[current_luminance].getTexture(), false, true);
			drawRect(0, 0,
				sobel[current_sobel].getWidth(),
				sobel[current_sobel].getHeight());
		}
		clearShader();
		popBlend();
	}
	popSurface();
	
	// apply the optical flow filter
	
	const int previous_luminance = 1 - current_luminance;
	const int previous_sobel = 1 - current_sobel;
	
	pushSurface(&opticalFlow);
	{
		pushBlend(BLEND_OPAQUE);
		Shader shader("filter-opticalFlow");
		setShader(shader);
		{
			shader.setTexture("luminance_prev", 0, luminance[previous_luminance].getTexture(), false, true);
			shader.setTexture("luminance_curr", 1, luminance[current_luminance].getTexture(), false, true);
			shader.setTexture("sobel_prev", 2, sobel[previous_sobel].getTexture(), false, true);
			shader.setTexture("sobel_curr", 3, sobel[current_sobel].getTexture(), false, true);
			shader.setImmediate("scale", flowFilter.strength);
			
			drawRect(0, 0, sobel[current_sobel].getWidth(), sobel[current_sobel].getHeight());
		}
		clearShader();
		popBlend();
	}
	popSurface();
}
