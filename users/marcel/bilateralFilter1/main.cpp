#include "recursive-bf.hpp"

#include "framework.h"
#include "image.h"

#include "Benchmark.h"

#include <math.h>

struct Mat
{
	int sx = 0;
	int sy = 0;
	float * values = nullptr;
	
	void alloc(
		const int in_sx,
		const int in_sy,
		const bool clearToZero)
	{
		free();
		
		//
		
		sx = in_sx;
		sy = in_sy;
		
		if (clearToZero)
			values = (float*)calloc(sx * sy, 4);
		else
			values = (float*)malloc(sx * sy * 4);
	}
	
	void free()
	{
		sx = 0;
		sy = 0;
		
		::free(values);
		values = nullptr;
	}
	
	float & at(int x, int y)
	{
		return values[x + y * sx];
	}
	
	const float at(int x, int y) const
	{
		return values[x + y * sx];
	}
};

#define USE_GAUSSIAN_SQ 1

static float distance(
	const int x1,
	const int y1,
	const int x2,
	const int y2)
{
	const int dx = x2 - x1;
	const int dy = y2 - y1;
	const int dSquared = dx * dx + dy * dy;
	return sqrtf(float(dSquared));
}

static int distanceSq(
	const int x1,
	const int x2)
{
	const int dx = x2 - x1;
	return dx * dx;
}

static int distanceSq(
	const int x1,
	const int y1,
	const int x2,
	const int y2)
{
	const int dx = x2 - x1;
	const int dy = y2 - y1;
	return dx * dx + dy * dy;
}

static double gaussian(
	const float x,
	const double sigma)
{
    return exp(-(x * x) / (2 * (sigma * sigma))) / (2 * M_PI * (sigma * sigma));
}

static double gaussianSq(
	const float xSq,
	const double sigmaSq)
{
    return exp(-(xSq) / (2 * (sigmaSq))) / (2 * M_PI * (sigmaSq));
}

// -- brute force bilateral filter with N x N kernel size

void applyBilateralFilter(
	const Mat & source,
	Mat & filteredImage,
	const int x,
	const int y,
	const int diameter,
	const double sigmaI,
	const double sigmaS)
{
    const int half = diameter / 2;
	
    double iFiltered = 0.0;
    double wP = 0.0;
	
#if USE_GAUSSIAN_SQ
	const double sigmaISq = sigmaI * sigmaI;
	const double sigmaSSq = sigmaS * sigmaS;
#endif

	for (int j = 0; j < diameter; ++j)
	{
		const int neighbor_y = y - half + j;
		
		for (int i = 0; i < diameter; ++i)
		{
            const int neighbor_x = x - half + i;
			
		#if USE_GAUSSIAN_SQ
			const float delta = source.at(neighbor_x, neighbor_y) - source.at(x, y);
			
			const double gi = gaussianSq(
            	delta * delta,
            	sigmaISq);
			
			const double gs = gaussianSq(
            	distanceSq(x, y, neighbor_x, neighbor_y),
            	sigmaSSq);
		#else
            const double gi = gaussian(
            	source.at(neighbor_x, neighbor_y) - source.at(x, y),
            	sigmaI);
			
            const double gs = gaussian(
            	distance(x, y, neighbor_x, neighbor_y),
            	sigmaS);
		#endif
			
			// in bilateral filtering, higher weights are given
			// to samples that are closer spatially and in value
			// to the sample in the center (which is at (x, y) in
			// this case
			
            const double w = gi * gs;
			
            iFiltered += source.at(neighbor_x, neighbor_y) * w;
			
            wP += w;
        }
    }
	
    iFiltered /= wP;
	
    filteredImage.at(x, y) = iFiltered;
}

void bilateralFilter(
	const Mat & source,
	Mat & filteredImage,
	int diameter,
	double sigmaI,
	double sigmaS)
{
	Benchmark bm("bilateralFilter");
	
    filteredImage.alloc(source.sx, source.sy, true);
	
	const int half = diameter / 2;
	
    const int width = source.sx;
    const int height = source.sy;

	for (int j = half; j < height - half; j++)
	{
    	for (int i = half; i < width - half; i++)
    	{
            applyBilateralFilter(
            	source,
            	filteredImage,
            	i,
            	j,
            	diameter,
            	sigmaI,
            	sigmaS);
        }
    }
}

// -- 'separable' bilateral filter with N x N kernel size

void applyBilateralFilter_H(
	const Mat & source,
	Mat & filteredImage,
	const int x,
	const int y,
	const int diameter,
	const double sigmaI,
	const double sigmaS)
{
    const int half = diameter / 2;
	
    double iFiltered = 0.0;
    double wP = 0.0;
	
	const double sigmaISq = sigmaI * sigmaI;
	const double sigmaSSq = sigmaS * sigmaS;

	for (int i = 0; i < diameter; ++i)
	{
		const int neighbor_x = x - half + i;
		
		const float delta = source.at(neighbor_x, y) - source.at(x, y);
		
	// todo : intensity distance gaussian could be optimized using a simple lut
		const double gi = gaussianSq(
			delta * delta,
			sigmaISq);
		
	// todo : neighbor distance gaussian could be optimized using a simple lut
		const double gs = gaussianSq(
			distanceSq(x, neighbor_x),
			sigmaSSq);
		
		// in bilateral filtering, higher weights are given
		// to samples that are closer spatially and in value
		// to the sample in the center (which is at (x, y) in
		// this case
		
		const double w = gi * gs;
		
		iFiltered += source.at(neighbor_x, y) * w;
		
		wP += w;
	}
	
    iFiltered /= wP;
	
    filteredImage.at(x, y) = iFiltered;
}

void applyBilateralFilter_V(
	const Mat & source,
	Mat & filteredImage,
	const int x,
	const int y,
	const int diameter,
	const double sigmaI,
	const double sigmaS)
{
    const int half = diameter / 2;
	
    double iFiltered = 0.0;
    double wP = 0.0;
	
	const double sigmaISq = sigmaI * sigmaI;
	const double sigmaSSq = sigmaS * sigmaS;
	
	for (int i = 0; i < diameter; ++i)
	{
		const int neighbor_y = y - half + i;
		
		const float delta = source.at(x, neighbor_y) - source.at(x, y);
		
		const double gi = gaussianSq(
			delta * delta,
			sigmaISq);
		
		const double gs = gaussianSq(
			distanceSq(y, neighbor_y),
			sigmaSSq);
		
		// in bilateral filtering, higher weights are given
		// to samples that are closer spatially and in value
		// to the sample in the center (which is at (x, y) in
		// this case
		
		const double w = gi * gs;
		
		iFiltered += source.at(x, neighbor_y) * w;
		
		wP += w;
	}
	
    iFiltered /= wP;
	
    filteredImage.at(x, y) = iFiltered;
}

void bilateralFilter_Separate(
	const Mat & source,
	Mat & temp,
	Mat & filteredImage,
	int diameter,
	double sigmaI,
	double sigmaS)
{
	Benchmark bm("bilateralFilter");
	
	temp.alloc(source.sx, source.sy, true);
    filteredImage.alloc(source.sx, source.sy, true);
	
	const int half = diameter / 2;
	
    const int width = source.sx;
    const int height = source.sy;

	// horizontal pass
	
	for (int j = half; j < height - half; j++)
	{
    	for (int i = half; i < width - half; i++)
    	{
            applyBilateralFilter_H(
            	source,
            	temp,
            	i,
            	j,
            	diameter,
            	sigmaI,
            	sigmaS);
        }
    }
	
	// vertical pass
	
	for (int j = half; j < height - half; j++)
	{
    	for (int i = half; i < width - half; i++)
    	{
            applyBilateralFilter_V(
            	temp,
            	filteredImage,
            	i,
            	j,
            	diameter,
            	sigmaI,
            	sigmaS);
        }
    }
}

// -- O(1) bilateral filter

// adapted from: https://github.com/fstasel/fastBilateral3d/blob/master/bf3.m

struct MatI
{
	int sx = 0;
	int sy = 0;
	uint8_t * values = nullptr;
	
	~MatI()
	{
		assert(values == nullptr);
	}
	
	void alloc(
		const int in_sx,
		const int in_sy,
		const bool clearToZero)
	{
		free();
		
		//
		
		sx = in_sx;
		sy = in_sy;
		
		if (clearToZero)
			values = (uint8_t*)calloc(sx * sy, sizeof(uint8_t));
		else
			values = (uint8_t*)malloc(sx * sy * sizeof(uint8_t));
	}
	
	void free()
	{
		sx = 0;
		sy = 0;
		
		::free(values);
		values = nullptr;
	}
	
	uint8_t & at(int x, int y)
	{
		return values[x + y * sx];
	}
	
	const uint8_t at(int x, int y) const
	{
		return values[x + y * sx];
	}
	
	uint8_t * begin()
	{
		return values;
	}
	
	uint8_t * end()
	{
		return values + sx * sy;
	}
};

void gkernel(
	const float sigma,
	const int numBins,
	const float R,
	float * h)
{
    float hSum = 0.f;
	
    for (int i = 0; i < numBins; ++i)
    {
    	const float s = i / sigma;
    	const float sSquared = s * s;
    	h[i] = expf(-.5f * R * sSquared);
    	hSum += h[i];
	}
	
	for (int i = 0; i < numBins; ++i)
		h[i] /= hSum;
}

static void integralImage2(
	const uint8_t * values,
	const int sx,
	const int sy,
	int64_t * dstValues)
{
	for (int i = 0; i < sx * sy; ++i)
		dstValues[i] = values[i];
	
	for (int y = 0; y < sy; ++y)
	{
		int64_t * valueLine0 = dstValues + (y - 1) * sx - 1;
		int64_t * valueLine1 = dstValues + (y - 0) * sx - 1;

		for (int x = 0; x < sx; ++x)
		{
			int64_t value = 0;

			if ((x > 0) & (y > 0))
			{
				value =
					- valueLine0[0]
					+ valueLine0[1]
					+ valueLine1[0]
					+ valueLine1[1];
			}
			else
			{
				for (int dx = -1; dx <= 0; ++dx)
				{
					for (int dy = -1; dy <= 0; ++dy)
					{
						const int px = x + dx;
						const int py = y + dy;

						if (px >= 0 && py >= 0)
						{
							const uint8_t * srcLine = values + py * sx;
							const int16_t srcValue =
								(dx < 0 && dy < 0)
									? -srcLine[px]
									: +srcLine[px];
						
							value += srcValue;
						}
					}
				}
			}

			valueLine0++;
			valueLine1++;

			valueLine1[0] = value;
		}
	}
}

void bf2(
	const MatI & I,
	MatI & Ibf,
	const int diameter,
	const float sigmaS)
{
	Benchmark bm("bilateralFilter-o(1)");
	
    const int BINS = 16;
    const float R = 256.f / BINS;

	const int sx = I.sx;
	const int sy = I.sy;
	
	float k[BINS];
    gkernel(sigmaS, BINS, R, k);

#if 1
	MatI Iq;
	Iq.alloc(sx, sy, false);
	for (int i = 0; i < sx * sy; ++i)
		Iq.values[i] = uint8_t(I.values[i] / R);
	
#define Iidx(y, x) (x + sx * (y)) // index into 2d array
#define Cidx(v, sv) (v == -1 ? 0 : v) // clamp index
#define Bidx(b, y, x) (Cidx(x, sx) + sx * (Cidx(y, sy) + sy * (b))) // index into bucketed 2d array + sampling clamp

	int64_t * Ibins_int = (int64_t*)malloc(sx * sy * BINS * sizeof(int64_t));
	uint8_t * Ibins = (uint8_t*)malloc(sx * sy * sizeof(uint8_t));
	
    for (int b = 0; b < BINS; ++b)
    {
		for (int y = 0; y < sy; ++y)
			for (int x = 0; x < sx; ++x)
				Ibins[Iidx(y, x)] = Iq.values[Iidx(y, x)] == b;
		
		//printf("Bidx(b): %d\n", Bidx(b, 0, 0));
		
		integralImage2(Ibins, sx, sy, Ibins_int + Bidx(b, 0, 0));
    }
	
    free(Ibins);
    Ibins = nullptr;

    const int half = diameter / 2;
	
	for (int y = 0; y < sy; ++y)
	{
		int minY = y - half;
		int maxY = y + half;
		if (minY < 0)
			minY = 0;
		if (maxY > sy - 1)
			maxY = sy - 1;
		
		for (int x = 0; x < sx; ++x)
		{
			int minX = x - half;
			int maxX = x + half;
			if (minX < 0)
				minX = 0;
			if (maxX > sx - 1)
				maxX = sx - 1;
			
			float tw = 0; // total weight
			float ws = 0; // weighted sample
			
			for (int b = 0; b < BINS; ++b)
			{
				const int d = abs(Iq.values[Iidx(y,x)] - b);
				
			#if 1
				const int64_t s =
					+ Ibins_int[Bidx(b,maxY  ,maxX  )]  // + bottom-right
					- Ibins_int[Bidx(b,maxY  ,minX-1)]  // -  bottom-left
					- Ibins_int[Bidx(b,minY-1,maxX  )]  // -    top-right
					+ Ibins_int[Bidx(b,minY-1,minX-1)]; // -     top-left
			#else
				const int s =
					+ Ibins_int[Bidx(b,eR+1,eC+1,eP+1)]
					- Ibins_int[Bidx(b,eR+1,eC+1,sP  )]
					- Ibins_int[Bidx(b,eR+1,sC  ,eP+1)]
					- Ibins_int[Bidx(b,sR  ,eC+1,eP+1)]
					+ Ibins_int[Bidx(b,sR  ,sC  ,eP+1)]
					+ Ibins_int[Bidx(b,sR  ,eC+1,sP  )]
					+ Ibins_int[Bidx(b,eR+1,sC  ,sP  )]
					- Ibins_int[Bidx(b,sR  ,sC  ,sP  )];
			#endif
			
				assert(s >= 0);
			
				assert(d >= 0 && d < BINS);
				const float w = s * k[d];
				
				tw = tw + w;
				ws = ws + b * w;
			}
			
			ws = ws / (tw * (BINS - 1));
			
			assert(ws >= 0.f && ws <= 1.f);
			
			Ibf.values[Iidx(y,x)] = uint8_t(255 * ws);
		}
	}
	
    free(Ibins_int);
    Ibins_int = nullptr;
	
    Iq.free();
#endif
}

static bool testBf2()
{
	// load the source image
	
    ImageData * image = loadImage("lenna.png");
	
    if (image == nullptr)
    {
        logError("failed to load image");
        return false;
    }

	// convert the source image
	
	MatI src;
	src.alloc(image->sx, image->sy, false);
	
	for (int y = 0; y < image->sy; ++y)
	{
		for (int x = 0; x < image->sx; ++x)
		{
			const uint8_t value =
				(
					image->getLine(y)[x].r +
					image->getLine(y)[x].g +
					image->getLine(y)[x].b
				) / 3;
			
			src.at(x, y) = value;
		}
	}
	
	// filter the image
	
	MatI filteredImage;
	filteredImage.alloc(src.sx, src.sy, false);
	
	bf2(src, filteredImage, 20, 16.0);

	// convert and save the filtered image
	
	for (int y = 0; y < image->sy; ++y)
	{
		ImageData::Pixel * line = image->getLine(y);
		for (int x = 0; x < image->sx; ++x)
		{
			const uint8_t value = uint8_t(filteredImage.at(x, y));
			line[x].r = value;
			line[x].g = value;
			line[x].b = value;
		}
	}
	
	saveImage(image, "lenna-filtered-o1.png");
	
	delete image;
	image = nullptr;
	
	filteredImage.free();
	src.free();
	
	return true;
}

static bool testRecursiveBf()
{
	// load the source image
	
    ImageData * image = loadImage("lenna-color.png");
	
    if (image == nullptr)
    {
        logError("failed to load image");
        return false;
    }
	
    // calculate luminance
	
	// filter the image
	
	ImageData filteredImage;
	filteredImage.imageData = new ImageData::Pixel[image->sx * image->sy];
	filteredImage.sx = image->sx;
	filteredImage.sy = image->sy;
	
	{
		Benchmark bm("bilateralFilter-recursive");
		
		uint8_t * dst = (uint8_t*)filteredImage.imageData;
		
		recursive_bf<4>(
			(uint8_t*)image->imageData,
			(uint8_t*)image->imageData,
			dst,
			30.0/256,
			20.0/256,
			image->sx,
			image->sy);
	}

	for (int y = 0; y < filteredImage.sy; ++y)
	{
		ImageData::Pixel * line = filteredImage.getLine(y);
		for (int x = 0; x < filteredImage.sx; ++x)
		{
			line[x].a = 255;
		}
	}
	
	// convert and save the filtered image
	
	saveImage(&filteredImage, "lenna-filtered-recursive-bf.png");
	
	delete image;
	image = nullptr;
	
	return true;
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	testBf2();
	
	testRecursiveBf();
	
	// load the source image
	
    ImageData * image = loadImage("lenna.png");
	
    if (image == nullptr)
    {
        logError("failed to load image");
        return -1;
    }

	// convert the source image
	
	Mat src;
	src.alloc(image->sx, image->sy, false);
	
	for (int y = 0; y < image->sy; ++y)
	{
		for (int x = 0; x < image->sx; ++x)
		{
			const uint8_t value =
				(
					image->getLine(y)[x].r +
					image->getLine(y)[x].g +
					image->getLine(y)[x].b
				) / 3;
			
			src.at(x, y) = value;
		}
	}
	
	// filter the image
	
#if 0
	Mat filteredImage;
    bilateralFilter(src, filteredImage, 20, 12.0, 16.0);
#else
    Mat temp;
	Mat filteredImage;
    bilateralFilter_Separate(src, temp, filteredImage, 20, 12.0, 16.0);
#endif

	// convert and save the filtered image
	
	for (int y = 0; y < image->sy; ++y)
	{
		ImageData::Pixel * line = image->getLine(y);
		for (int x = 0; x < image->sx; ++x)
		{
			const uint8_t value = uint8_t(filteredImage.at(x, y));
			line[x].r = value;
			line[x].g = value;
			line[x].b = value;
		}
	}
	
	saveImage(image, "lenna-filtered.png");
	
	delete image;
	image = nullptr;

    return 0;
}
