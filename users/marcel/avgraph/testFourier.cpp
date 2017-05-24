#include "framework.h"
#include "image.h"
#include "Timer.h"
#include "vfxNodes/fourier.h"
#include <math.h>

extern int GFX_SX;
extern int GFX_SY;

static void testFourier1d_fast()
{
	logDebug("testFourier1d_fast");
	
	// perform fast fourier transform on 48 (64 output) samples of complex data
	
	const int kSize = 48;
	const int kTransformSize = 64;
	const int kNumBits = Fourier::integerLog2(kTransformSize);
	
	double dreal[2][kTransformSize];
	double dimag[2][kTransformSize];
	
	int a = 0; // we need to reverse bits of data indices before doing the fft. to do so we ping-pong between two sets of data, dreal/dimag[0] and dreal/dimag[1]
	
	for (int i = 0; i < kSize; ++i)
	{
		dreal[a][i] = std::cos(i / 100.0);
		dimag[a][i] = 0.0;
	}
	
	for (int i = 0; i < kSize; ++i)
	{
		logDebug("[%03d] inv=1: dreal=%g, dimag=%g", i, dreal[a][i], dimag[a][i]);
	}
	
	for (int i = 0; i < kSize; ++i)
	{
		const int sindex = i;
		const int dindex = Fourier::reverseBits(i, kNumBits);
		
		dreal[1 - a][dindex] = dreal[a][sindex];
		dimag[1 - a][dindex] = dimag[a][sindex];
	}
	
	a = 1 - a;
	
	Fourier::fft1D(dreal[a], dimag[a], kSize, kTransformSize, false, false);
	
	for (int i = 0; i < kTransformSize; ++i)
	{
		logDebug("[%03d] inv=0: dreal=%g, dimag=%g", i, dreal[a][i], dimag[a][i]);
	}
	
	for (int i = 0; i < kTransformSize; ++i)
	{
		const int sindex = i;
		const int dindex = Fourier::reverseBits(i, kNumBits);
		
		dreal[1 - a][dindex] = dreal[a][sindex];
		dimag[1 - a][dindex] = dimag[a][sindex];
	}
	
	a = 1 - a;
	
	Fourier::fft1D(dreal[a], dimag[a], kTransformSize, kTransformSize, true, true);
	
	for (int i = 0; i < kTransformSize; ++i)
	{
		logDebug("[%03d] inv=1: dreal=%g, dimag=%g", i, dreal[a][i], dimag[a][i]);
	}
}

static void testFourier1d_slow()
{
	logDebug("testFourier1d_slow");
	
	// perform fast fourier transform on 64 samples of complex data
	
	const int kSize = 48;
	const int kTransformSize = 64;
	
	double dreal[kTransformSize];
	double dimag[kTransformSize];
	
	for (int i = 0; i < kSize; ++i)
	{
		dreal[i] = std::cos(i / 100.0);
		dimag[i] = 0.0;
	}
	
	for (int i = 0; i < kSize; ++i)
	{
		logDebug("[%03d] inv=1: dreal=%g, dimag=%g", i, dreal[i], dimag[i]);
	}
	
	Fourier::fft1D_slow(dreal, dimag, kSize, kTransformSize, false, false);
	
	for (int i = 0; i < kTransformSize; ++i)
	{
		logDebug("[%03d] inv=0: dreal=%g, dimag=%g", i, dreal[i], dimag[i]);
	}
	
	Fourier::fft1D_slow(dreal, dimag, kTransformSize, kTransformSize, true, true);
	
	for (int i = 0; i < kTransformSize; ++i)
	{
		logDebug("[%03d] inv=1: dreal=%g, dimag=%g", i, dreal[i], dimag[i]);
	}
}

void testFourier1d()
{
	testFourier1d_fast();
	
	testFourier1d_slow();
}

static void doFourier2D_fast(const ImageData * image, double *& dreal, double *& dimag, int & transformSx, int & transformSy, const bool inverse, const bool normalize)
{
	const int imageSx = image->sx;
	const int imageSy = image->sy;
	transformSx = Fourier::upperPowerOf2(imageSx);
	transformSy = Fourier::upperPowerOf2(imageSy);

	dreal = new double[transformSx * transformSy];
	dimag = new double[transformSx * transformSy];
	
	const int numBitsX = Fourier::integerLog2(transformSx);
	
	// calculate initial data set
	
	for (int y = 0; y < imageSy; ++y)
	{
		double * __restrict rreal = &dreal[y * transformSx];
		double * __restrict rimag = &dimag[y * transformSx];
		
		const ImageData::Pixel * __restrict line = image->getLine(y);
		
		for (int x = 0; x < imageSx; ++x)
		{
			const auto & pixel = line[x];
			const double value = (pixel.r + pixel.g + pixel.b) / 3.0;
			
			const int xReversed = Fourier::reverseBits(x, numBitsX);
			
			rreal[xReversed] = value;
			rimag[xReversed] = 0.0;
		}
	}
	
	Fourier::fft2D(dreal, dimag, imageSx, transformSx, imageSy, transformSy, inverse, normalize);
}

static void doFourier2D_slow(const ImageData * image, double *& dreal, double *& dimag, int & transformSx, int & transformSy, const bool inverse, const bool normalize)
{
	const int imageSx = image->sx;
	const int imageSy = image->sy;
	transformSx = Fourier::upperPowerOf2(imageSx);
	transformSy = Fourier::upperPowerOf2(imageSy);

	dreal = new double[transformSx * transformSy];
	dimag = new double[transformSx * transformSy];
	
	// calculate initial data set
	
	for (int y = 0; y < imageSy; ++y)
	{
		double * __restrict rreal = &dreal[y * imageSx];
		double * __restrict rimag = &dimag[y * imageSx];
		
		const ImageData::Pixel * __restrict line = image->getLine(y);
		
		for (int x = 0; x < imageSx; ++x)
		{
			const auto & pixel = line[x];
			const double value = (pixel.r + pixel.g + pixel.b) / 3.0;
			
			rreal[x] = value;
			rimag[x] = 0.0;
		}
	}
	
	Fourier::fft2D_slow(dreal, dimag, imageSx, transformSx, imageSy, transformSy, inverse, normalize);
}

static void fft2D_scale(double * __restrict dreal, double * __restrict dimag, const int transformSx, const int transformSy)
{
	const int numValues = transformSx * transformSy;
	const double scale = 1.0 / double(numValues);
	
	for (int i = 0; i < numValues; ++i)
	{
		dreal[i] *= scale;
		dimag[i] *= scale;
	}
}

static void fft2D_draw(const double * __restrict dreal, const double * __restrict dimag, const int transformSx, const int transformSy, const char * impl)
{
	gxPushMatrix();
	{
		const double scaleX = 100.0 * mouse.x / double(GFX_SX);
		const double scaleY = 100.0 * mouse.y / double(GFX_SX);
		
		const int displayScaleX = std::max(1, GFX_SX / transformSx);
		const int displayScaleY = std::max(1, GFX_SY / transformSy);
		const int displayScale = std::min(displayScaleX, displayScaleY);
		gxScalef(displayScale, displayScale, 1);
		
		gxBegin(GL_QUADS);
		{
			for (int y = 0; y < transformSy; ++y)
			{
				for (int x = 0; x < transformSx; ++x)
				{
					// sample from the middle as it looks nicer when presenting the result ..
					
					const int sx = (x + transformSx/2) % transformSx;
					const int sy = (y + transformSy/2) % transformSy;
					
					const double c = dreal[sy * transformSx + sx];
					const double s = dimag[sy * transformSx + sx];
					const double m = std::log10(10.0 + std::hypot(c, s)) - 1.0;
					
					const double r = c * scaleX;
					const double g = s * scaleX;
					const double b = m * scaleY;
					
					gxColor4f(r, g, b, 1.f);
					//gxColor4f(b, b, b, 1.f);
					gxVertex2f(x+0, y+0);
					gxVertex2f(x+1, y+0);
					gxVertex2f(x+1, y+1);
					gxVertex2f(x+0, y+1);
				}
			}
		}
		gxEnd();
	}
	gxPopMatrix();
	
	setFont("calibri.ttf");
	setColor(colorWhite);
	drawText(5, 5, 18, +1, +1, "method: %s", impl);
}

void testFourier2d()
{
	//ImageData * image = loadImage("rainbow-pow2.png");
	//ImageData * image = loadImage("rainbow-small.png");
	//ImageData * image = loadImage("rainbow.png");
	//ImageData * image = loadImage("rainbow.jpg");
	ImageData * image = loadImage("picture.jpg");
	//ImageData * image = loadImage("wordcloud.png");
	//ImageData * image = loadImage("antigrain.png");
	
	if (image == nullptr)
	{
		logDebug("failed to load image!");
		return;
	}
	
	// do fast 2D fft
	
	double * dreal_fast = nullptr;
	double * dimag_fast = nullptr;
	int transformSx_fast = 0;
	int transformSy_fast = 0;
	
	doFourier2D_fast(
		image,
		dreal_fast, dimag_fast,
		transformSx_fast, transformSy_fast,
		false, false);
	
	fft2D_scale(dreal_fast, dimag_fast, transformSx_fast, transformSy_fast);
	
	// do slow 2D fft
	
	double * dreal_slow = nullptr;
	double * dimag_slow = nullptr;
	int transformSx_slow = 0;
	int transformSy_slow = 0;
	
	doFourier2D_slow(
		image,
		dreal_slow, dimag_slow,
		transformSx_slow, transformSy_slow,
		false, false);
	
	fft2D_scale(dreal_slow, dimag_slow, transformSx_slow, transformSy_slow);
	
	// do reference 2D fft (fast)

	const int imageSx = image->sx;
	const int imageSy = image->sy;
	const int transformSx = Fourier::upperPowerOf2(imageSx);
	const int transformSy = Fourier::upperPowerOf2(imageSy);

	double * dreal = new double[transformSx * transformSy];
	double * dimag = new double[transformSx * transformSy];
	
	{
		// note : the 2d fourier transform is seperable, meaning we could do the transform
		//        on all samples at once, or first for each row, and then for each column
		//        of the already transformed results of the previous row-passes
		
		const int numBitsX = Fourier::integerLog2(transformSx);
		const int numBitsY = Fourier::integerLog2(transformSy);
		
		auto fast_t1 = g_TimerRT.TimeUS_get();
		
		// perform FFT on each row
		
		for (int y = 0; y < imageSy; ++y)
		{
			double * __restrict rreal = &dreal[y * transformSx];
			double * __restrict rimag = &dimag[y * transformSx];
			
			const ImageData::Pixel * __restrict line = image->getLine(y);
			
			// todo : do memset on rimag instead of explicitly setting every element to 0.0
			
			for (int x = 0; x < imageSx; ++x)
			{
				const auto & pixel = line[x];
				const double value = (pixel.r + pixel.g + pixel.b) / 3.0;
				
				const int index = Fourier::reverseBits(x, numBitsX);
				rreal[index] = value;
				rimag[index] = 0.0;
			}
			
			Fourier::fft1D(rreal, rimag, imageSx, transformSx, false, false);
		}
		
		// perform FFT on each column
		
		// complex values for current column
		double * creal = new double[transformSy];
		double * cimag = new double[transformSy];
		
		for (int x = 0; x < transformSx; ++x)
		{
			double * __restrict frealc = &dreal[x];
			double * __restrict fimagc = &dimag[x];
			
			for (int y = 0; y < imageSy; ++y)
			{
				const int index = Fourier::reverseBits(y, numBitsY);
				
				creal[index] = *frealc;
				cimag[index] = *fimagc;
				
				frealc += transformSx;
				fimagc += transformSx;
			}
			
			Fourier::fft1D(creal, cimag, imageSy, transformSy, false, false);
			
			//
			
			frealc = &dreal[x];
			fimagc = &dimag[x];
			
			const double scale = 1.0 / double(transformSx * transformSy);
			
			for (int y = 0; y < transformSy; ++y)
			{
				*frealc = creal[y] * scale;
				*fimagc = cimag[y] * scale;
				
				frealc += transformSx;
				fimagc += transformSx;
			}
		}
		
		delete[] creal;
		delete[] cimag;
		
		auto fast_t2 = g_TimerRT.TimeUS_get();
		printf("fast fourier transform took %gms\n", (fast_t2 - fast_t1) / 1000.0);
	}
	
#if 0
	auto ref_t1 = g_TimerRT.TimeUS_get();
	for (int y = 0; y < image->sy; ++y)
	{
		printf("%d/%d\n", y + 1, image->sy);
		
		for (int x = 0; x < image->sx; ++x)
		{
			double coss = 0.0;
			double sins = 0.0;
			
			for (int v = 0; v < image->sy; ++v)
			{
				double phaseV = v * y / double(image->sy);
				
				for (int u = 0; u < image->sx; ++u)
				{
					double phaseU = u * x / double(image->sx);
					
					double phase = (2.0 * M_PI) * (phaseV + phaseU);
					
					double cosv = std::cos(phase);
					double sinv = std::sin(phase);
					
					double value = (image->getLine(v)[u].r + image->getLine(v)[u].g + image->getLine(v)[u].b) / 3.0;
					
					coss += cosv * value;
					sins += sinv * value;
				}
			}
			
			dreal[y * transformSx + x] = coss / double(image->sx * image->sy);
			dimag[y * transformSx + x] = sins / double(image->sx * image->sy);
		}
	}
	auto ref_t2 = g_TimerRT.TimeUS_get();
	printf("reference fourier transform took %lluus\n", (ref_t2 - ref_t1));
#endif
	
#if 0
	for (int y = 0; y < image->sy; ++y)
	{
		for (int x = 0; x < image->sx; ++x)
		{
			double & fc = f[y * transformSx * 2 + x * 2 + 0];
			double & fs = f[y * transformSx * 2 + x * 2 + 1];
			
			const int sx = (x + image->sx/2) % image->sx;
			const int sy = (y + image->sy/2) % image->sy;
			
			const double dx = sx - image->sx/2;
			const double dy = sy - image->sy/2;
			const double ds = std::hypot(dx, dy);
			
			const double w = std::pow(ds / 50.0, 4.0);
			const double s = w < 0.0 ? 0.0 : w > 1.0 ? 1.0 : w;
			
			fc *= s;
			fs *= s;
		}
	}
	
	// todo : reconstruct image from coefficients
#endif
	
	int display = 0;
	
	do
	{
		framework.process();
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			display = (display + 1) % 3;
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			if (display == 0)
				fft2D_draw(dreal, dimag, transformSx, transformSy, "reference");
			else if (display == 1)
				fft2D_draw(dreal_fast, dimag_fast, transformSx_fast, transformSy_fast, "fast");
			else if (display == 2)
				fft2D_draw(dreal_slow, dimag_slow, transformSx_slow, transformSy_slow, "slow");
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	delete[] dreal_fast;
	delete[] dimag_fast;
	delete[] dreal_slow;
	delete[] dimag_slow;
	delete[] dreal;
	delete[] dimag;
}
