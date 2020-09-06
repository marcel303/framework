#include "framework.h"
#include "image.h"
#include <string.h>
#include <vector>

// reading: https://people.clarkson.edu/~hudsonb/courses/cs611/

static void sampleBilinear(const ImageData * image, const int x, const int y, const float tx, const float ty, ImageData::Pixel & pixel)
{
	const int x1 = x;
	const int y1 = y;
	const int x2 = x1 + 1 < image->sx ? x1 + 1 : x1;
	const int y2 = y1 + 1 < image->sy ? y1 + 1 : y1;
	
	auto & pixel_11 = image->getLine(y1)[x1];
	auto & pixel_12 = image->getLine(y1)[x2];
	auto & pixel_21 = image->getLine(y2)[x1];
	auto & pixel_22 = image->getLine(y2)[x2];
	
	float r_1 = pixel_11.r * (1.f - ty) + pixel_21.r * ty;
	float g_1 = pixel_11.g * (1.f - ty) + pixel_21.g * ty;
	float b_1 = pixel_11.b * (1.f - ty) + pixel_21.b * ty;
	float a_1 = pixel_11.a * (1.f - ty) + pixel_21.a * ty;
	
	float r_2 = pixel_12.r * (1.f - ty) + pixel_22.r * ty;
	float g_2 = pixel_12.g * (1.f - ty) + pixel_22.g * ty;
	float b_2 = pixel_12.b * (1.f - ty) + pixel_22.b * ty;
	float a_2 = pixel_12.a * (1.f - ty) + pixel_22.a * ty;
	
	float r = r_1 * (1.f - tx) + r_2 * tx;
	float g = g_1 * (1.f - tx) + g_2 * tx;
	float b = b_1 * (1.f - tx) + b_2 * tx;
	float a = a_1 * (1.f - tx) + a_2 * tx;
	
	pixel.r = r;
	pixel.g = g;
	pixel.b = b;
	pixel.a = a;
}

static void testHierachicalHoleFilling()
{
	// create an image
	
	ImageData * image = new ImageData(64, 64);
	
	// give it a gradient
	
	for (int x = 0; x < image->sx; ++x)
	{
		for (int y = 0; y < image->sy; ++y)
		{
			auto & pixel = image->getLine(y)[x];
			
			pixel.r = x;
			pixel.g = y;
			pixel.b = x + y;
			pixel.a = 0xff;
		}
	}
	
	// add some holes to it
	
	for (int i = 0; i < 1000; ++i)
	{
		const int x = rand() % image->sx;
		const int y = rand() % image->sy;
		
		auto & pixel = image->getLine(y)[x];
		
		memset(&pixel, 0, sizeof(pixel));
	}
	
	// -- begin hole filling --
	
	// create array of down sampled images
	
	std::vector<ImageData*> images;
	
	images.push_back(image);
	
	for (;;)
	{
		ImageData * image = images.back();
		
		// can we down sample more ?
		
		if (image->sx <= 1 || image->sy <= 1)
			break;
		
		const int ds_sx = image->sx > 1 ? image->sx/2 : 1;
		const int ds_sy = image->sy > 1 ? image->sy/2 : 1;
		
		ImageData * ds = new ImageData(ds_sx, ds_sy);
		
		images.push_back(ds);
		
		bool hasHoles = false;
		
		for (int ds_x = 0; ds_x < ds_sx; ++ds_x)
		{
			for (int ds_y = 0; ds_y < ds_sy; ++ds_y)
			{
			// todo : perhaps perform a 5x5 gaussian as described in the article ..
			
				int r = 0;
				int g = 0;
				int b = 0;
				int a = 0;
				int numSamples = 0;
				
				for (int ox = 0; ox < 2; ++ox)
				{
					for (int oy = 0; oy < 2; ++oy)
					{
						const int image_x = ds_x * 2 + ox;
						const int image_y = ds_y * 2 + oy;
						
						if (image_x < image->sx && image_y < image->sy)
						{
							auto & pixel = image->getLine(image_y)[image_x];
							
							if ((pixel.r | pixel.g | pixel.b | pixel.a) != 0)
							{
								r += pixel.r;
								g += pixel.g;
								b += pixel.b;
								a += pixel.a;
								
								numSamples++;
							}
						}
					}
				}
				
				auto & pixel = ds->getLine(ds_y)[ds_x];
				
				if (numSamples == 0)
				{
					pixel.r = pixel.g = pixel.b = pixel.a = 0;
					
					hasHoles = true;
				}
				else
				{
					pixel.r = r / numSamples;
					pixel.g = g / numSamples;
					pixel.b = b / numSamples;
					pixel.a = a / numSamples;
				}
			}
		}
		
		// early out when we no longer witnessed any holes
		
		if (hasHoles == false)
			break;
	}
	
	int draw_x = 0;
	int draw_y = 0;
	
	for (auto * image : images)
	{
		GxTextureId texture = createTextureFromRGBA8(image->imageData, image->sx, image->sy, false, true);
		
		setColor(colorWhite);
		gxSetTexture(texture);
		drawRect(draw_x, draw_y, draw_x + image->sx, draw_y + image->sy);
		gxSetTexture(0);
		
		draw_x += image->sx;
		
		freeTexture(texture);
	}
	
	draw_x = 0;
	draw_y += image->sy;
	
	for (size_t i = images.size() - 1; i > 0; --i)
	{
		auto * image = images[i - 1];
		auto * ds = images[i];
		
		for (int x = 0; x < image->sx; ++x)
		{
			for (int y = 0; y < image->sy; ++y)
			{
				auto & pixel = image->getLine(y)[x];
				
				if ((pixel.r | pixel.g | pixel.b | pixel.a) == 0)
				{
				#if 1
					const int ds_x = x/2;
					const int ds_y = y/2;
					
					Assert(ds_x < ds->sx);
					Assert(ds_y < ds->sy);
					
					sampleBilinear(
						ds,
						ds_x,
						ds_y,
						(x & 1) ? .5f : 0.f,
						(y & 1) ? .5f : 0.f,
						pixel);
				#else
					const int ds_x = x/2;
					const int ds_y = y/2;
					
					Assert(ds_x < ds->sx);
					Assert(ds_y < ds->sy);
					
					pixel = ds->getLine(ds_y)[ds_x];
				#endif
				}
			}
		}
	}
	
	for (auto * image : images)
	{
		GxTextureId texture = createTextureFromRGBA8(image->imageData, image->sx, image->sy, false, true);
		
		setColor(colorWhite);
		gxSetTexture(texture);
		drawRect(draw_x, draw_y, draw_x + image->sx, draw_y + image->sy);
		gxSetTexture(0);
		
		draw_x += image->sx;
		
		freeTexture(texture);
	}
	
	logDebug("done");
	
	for (auto * image : images)
		delete image;
	images.clear();
}

int main(int artgc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 600))
		return -1;

	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		framework.beginDraw(0, 0, 0, 0);
		{
			testHierachicalHoleFilling();
		}
		framework.endDraw();
	}
	
	framework.shutdown();

	return 0;
}
