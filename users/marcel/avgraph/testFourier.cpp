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
		logDebug("[%03d] inv=1: dreal=%+.4f, dimag=%+.4f", i, dreal[a][i], dimag[a][i]);
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
		logDebug("[%03d] inv=0: dreal=%+.4f, dimag=%+.4f", i, dreal[a][i], dimag[a][i]);
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
		logDebug("[%03d] inv=1: dreal=%+.4f, dimag=%+.4f", i, dreal[a][i], dimag[a][i]);
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
		logDebug("[%03d] inv=1: dreal=%+.4f, dimag=%+.4f", i, dreal[i], dimag[i]);
	}
	
	Fourier::fft1D_slow(dreal, dimag, kSize, kTransformSize, false, false);
	
	for (int i = 0; i < kTransformSize; ++i)
	{
		logDebug("[%03d] inv=0: dreal=%+.4f, dimag=%+.4f", i, dreal[i], dimag[i]);
	}
	
	Fourier::fft1D_slow(dreal, dimag, kTransformSize, kTransformSize, true, true);
	
	for (int i = 0; i < kTransformSize; ++i)
	{
		logDebug("[%03d] inv=1: dreal=%+.4f, dimag=%+.4f", i, dreal[i], dimag[i]);
	}
}

void testFourier1d()
{
	testFourier1d_fast();
	
	testFourier1d_slow();
}

template <typename real>
static uint64_t doFourier2D_fast(const ImageData * image, real *& dreal, real *& dimag, int & transformSx, int & transformSy, const bool inverse, const bool normalize)
{
	const int imageSx = image->sx;
	const int imageSy = image->sy;
	transformSx = Fourier::upperPowerOf2(imageSx);
	transformSy = Fourier::upperPowerOf2(imageSy);

	dreal = new real[transformSx * transformSy];
	dimag = new real[transformSx * transformSy];
	
	real * creal = new real[transformSy];
	real * cimag = new real[transformSy];
	
	uint64_t t1 = g_TimerRT.TimeUS_get();
	
	const int numBitsX = Fourier::integerLog2(transformSx);
	
	int * xReversedLUT = (int*)alloca(sizeof(int) * imageSx);
	for (int x = 0; x < imageSx; ++x)
		xReversedLUT[x] = Fourier::reverseBits(x, numBitsX);
	
	// calculate initial data set
	
	for (int y = 0; y < imageSy; ++y)
	{
		real * __restrict rreal = &dreal[y * transformSx];
		real * __restrict rimag = &dimag[y * transformSx];
		
		const ImageData::Pixel * __restrict line = image->getLine(y);
		
		for (int x = 0; x < imageSx; ++x)
		{
			const auto & pixel = line[x];
			const real value = pixel.r + pixel.g + pixel.b;
			
			const int xReversed = xReversedLUT[x];
			
			rreal[xReversed] = value;
			rimag[xReversed] = 0.0;
		}
	}
	
	Fourier::fft2D(dreal, dimag, imageSx, transformSx, imageSy, transformSy, inverse, normalize, creal, cimag);
	
	uint64_t t2 = g_TimerRT.TimeUS_get();
	
	delete[] creal;
	creal = nullptr;
	
	delete[] cimag;
	cimag = nullptr;
	
	return t2 - t1;
}

template <typename real>
static uint64_t doFourier2D_slow(const ImageData * image, real *& dreal, real *& dimag, int & transformSx, int & transformSy, const bool inverse, const bool normalize)
{
	const int imageSx = image->sx;
	const int imageSy = image->sy;
	transformSx = Fourier::upperPowerOf2(imageSx);
	transformSy = Fourier::upperPowerOf2(imageSy);

	dreal = new real[transformSx * transformSy];
	dimag = new real[transformSx * transformSy];
	
	real * creal = new real[transformSy];
	real * cimag = new real[transformSy];
	
	uint64_t t1 = g_TimerRT.TimeUS_get();
	
	// calculate initial data set
	
	for (int y = 0; y < imageSy; ++y)
	{
		real * __restrict rreal = &dreal[y * imageSx];
		real * __restrict rimag = &dimag[y * imageSx];
		
		const ImageData::Pixel * __restrict line = image->getLine(y);
		
		for (int x = 0; x < imageSx; ++x)
		{
			const auto & pixel = line[x];
			const real value = pixel.r + pixel.g + pixel.b;
			
			rreal[x] = value;
			rimag[x] = 0.0;
		}
	}
	
	Fourier::fft2D_slow(dreal, dimag, imageSx, transformSx, imageSy, transformSy, inverse, normalize, creal, cimag);
	
	uint64_t t2 = g_TimerRT.TimeUS_get();
	
	delete[] creal;
	creal = nullptr;
	
	delete[] cimag;
	cimag = nullptr;
	
	return t2 - t1;
}

template <typename real>
static uint64_t doFourier2D_reference(const ImageData * image, real *& dreal, real *& dimag, int & transformSx, int & transformSy, const bool inverse, const bool normalize)
{
	const int imageSx = image->sx;
	const int imageSy = image->sy;
	transformSx = Fourier::upperPowerOf2(imageSx);
	transformSy = Fourier::upperPowerOf2(imageSy);

	dreal = new real[transformSx * transformSy];
	dimag = new real[transformSx * transformSy];
	
	uint64_t t1 = g_TimerRT.TimeUS_get();
	
	// note : the 2d fourier transform is seperable, meaning we could do the transform
	//        on all samples at once, or first for each row, and then for each column
	//        of the already transformed results of the previous row-passes
	
	const int numBitsX = Fourier::integerLog2(transformSx);
	const int numBitsY = Fourier::integerLog2(transformSy);
	
	// perform FFT on each row
	
	int * xReversedLUT = (int*)alloca(sizeof(int) * imageSx);
	for (int x = 0; x < imageSx; ++x)
		xReversedLUT[x] = Fourier::reverseBits(x, numBitsX);
	
	for (int y = 0; y < imageSy; ++y)
	{
		real * __restrict rreal = &dreal[y * transformSx];
		real * __restrict rimag = &dimag[y * transformSx];
		
		const ImageData::Pixel * __restrict line = image->getLine(y);
		
		// todo : do memset on rimag instead of explicitly setting every element to 0.0
		
		for (int x = 0; x < imageSx; ++x)
		{
			const auto & pixel = line[x];
			const real value = pixel.r + pixel.g + pixel.b;
			
			const int index = xReversedLUT[x];
			
			rreal[index] = value;
			rimag[index] = 0.0;
		}
		
		Fourier::fft1D(rreal, rimag, imageSx, transformSx, false, false);
	}
	
	// perform FFT on each column
	
	// complex values for current column
	real * creal = new real[transformSy];
	real * cimag = new real[transformSy];
	
	int * yReversedLUT = (int*)alloca(sizeof(int) * imageSy);
	for (int y = 0; y < imageSy; ++y)
		yReversedLUT[y] = Fourier::reverseBits(y, numBitsY);
	
	for (int x = 0; x < transformSx; ++x)
	{
		real * __restrict frealc = &dreal[x];
		real * __restrict fimagc = &dimag[x];
		
		for (int y = 0; y < imageSy; ++y)
		{
			const int index = yReversedLUT[y];
			
			creal[index] = *frealc;
			cimag[index] = *fimagc;
			
			frealc += transformSx;
			fimagc += transformSx;
		}
		
		Fourier::fft1D(creal, cimag, imageSy, transformSy, false, false);
		
		//
		
		frealc = &dreal[x];
		fimagc = &dimag[x];
		
		const real scale = 1.0 / real(transformSx * transformSy);
		
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

	uint64_t t2 = g_TimerRT.TimeUS_get();
	
	return t2 - t1;
}

template <typename real>
static void fft2D_scale(real * __restrict dreal, real * __restrict dimag, const int transformSx, const int transformSy)
{
	const int numValues = transformSx * transformSy;
	const real scale = 1.0 / real(numValues);
	
	for (int i = 0; i < numValues; ++i)
	{
		dreal[i] *= scale;
		dimag[i] *= scale;
	}
}

template <typename real>
static void fft2D_draw(const real * __restrict dreal, const real * __restrict dimag, const int transformSx, const int transformSy, const char * impl, const uint64_t usecs)
{
	gxPushMatrix();
	{
		const real scaleX =  10.0 * mouse.x / real(GFX_SX);
		const real scaleY = 100.0 * mouse.y / real(GFX_SX);
		
		gxBegin(GL_POINTS);
		{
			for (int y = 0; y < transformSy; ++y)
			{
				for (int x = 0; x < transformSx; ++x)
				{
					// sample from the middle as it looks nicer when presenting the result ..
					
					const int sx = (x + transformSx/2) % transformSx;
					const int sy = (y + transformSy/2) % transformSy;
					
					const real c = dreal[sy * transformSx + sx];
					const real s = dimag[sy * transformSx + sx];
					const real m = std::log10(10.0 + std::hypot(c, s)) - 1.0;
					
					const real r = c * scaleX;
					const real b = s * scaleX;
					const real g = m * scaleY;
					
					gxColor4f(r, g, b, 1.f);
					//gxColor4f(b, b, b, 1.f);
					gxVertex2f(x+0, y+0);
				}
			}
		}
		gxEnd();
	}
	gxPopMatrix();
	
	setFont("calibri.ttf");
	setColor(colorWhite);
	drawText(5, 5, 18, +1, +1, "method: %s. time=%gms", impl, usecs / 1000.0);
}

template <typename real>
void testFourier2dImpl()
{
	//ImageData * image = loadImage("rainbow-pow2.png");
	//ImageData * image = loadImage("rainbow-small.png");
	//ImageData * image = loadImage("rainbow.png");
	//ImageData * image = loadImage("rainbow.jpg");
	//ImageData * image = loadImage("picture.jpg");
	//const char * filename = "happysun.jpg";
	const char * filename = "happysun2.png";
	ImageData * image = loadImage(filename);
	
	if (image == nullptr)
	{
		logDebug("failed to load image!");
		return;
	}
	
	do
	{
	// do reference 2D fft (fast)
	
	real * dreal_reference = nullptr;
	real * dimag_reference = nullptr;
	int transformSx_reference = 0;
	int transformSy_reference = 0;
	
	const uint64_t t_reference = doFourier2D_reference(
		image,
		dreal_reference, dimag_reference,
		transformSx_reference, transformSy_reference,
		false, false);
	
	//printf("doFourier2D_reference (fast) took %gms\n", t_reference / 1000.0);
	
	// do fast 2D fft
	
	real * dreal_fast = nullptr;
	real * dimag_fast = nullptr;
	int transformSx_fast = 0;
	int transformSy_fast = 0;
	
	const uint64_t t_fast = doFourier2D_fast(
		image,
		dreal_fast, dimag_fast,
		transformSx_fast, transformSy_fast,
		false, false);
	
	fft2D_scale(dreal_fast, dimag_fast, transformSx_fast, transformSy_fast);
	
	//printf("doFourier2D_fast took %gms\n", t_fast / 1000.0);
	
	// do slow 2D fft
	
	real * dreal_slow = nullptr;
	real * dimag_slow = nullptr;
	int transformSx_slow = 0;
	int transformSy_slow = 0;
	
	const uint64_t t_slow = doFourier2D_slow(
		image,
		dreal_slow, dimag_slow,
		transformSx_slow, transformSy_slow,
		false, false);
	
	fft2D_scale(dreal_slow, dimag_slow, transformSx_slow, transformSy_slow);
	
	//printf("doFourier2D_slow took %gms\n", t_slow / 1000.0);
	
#if 0
	auto ref_t1 = g_TimerRT.TimeUS_get();
	for (int y = 0; y < image->sy; ++y)
	{
		printf("%d/%d\n", y + 1, image->sy);
		
		for (int x = 0; x < image->sx; ++x)
		{
			real coss = 0.0;
			real sins = 0.0;
			
			for (int v = 0; v < image->sy; ++v)
			{
				real phaseV = v * y / real(image->sy);
				
				for (int u = 0; u < image->sx; ++u)
				{
					real phaseU = u * x / real(image->sx);
					
					real phase = (2.0 * M_PI) * (phaseV + phaseU);
					
					real cosv = +std::cos(phase);
					real sinv = -std::sin(phase);
					
					real value = image->getLine(v)[u].r + image->getLine(v)[u].g + image->getLine(v)[u].b);
					
					coss += cosv * value;
					sins += sinv * value;
				}
			}
			
			dreal_reference[y * transformSx_reference + x] = coss / real(image->sx * image->sy);
			dimag_reference[y * transformSx_reference + x] = sins / real(image->sx * image->sy);
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
			real & fc = f[y * transformSx * 2 + x * 2 + 0];
			real & fs = f[y * transformSx * 2 + x * 2 + 1];
			
			const int sx = (x + image->sx/2) % image->sx;
			const int sy = (y + image->sy/2) % image->sy;
			
			const real dx = sx - image->sx/2;
			const real dy = sy - image->sy/2;
			const real ds = std::hypot(dx, dy);
			
			const real w = std::pow(ds / 50.0, 4.0);
			const real s = w < 0.0 ? 0.0 : w > 1.0 ? 1.0 : w;
			
			fc *= s;
			fs *= s;
		}
	}
	
	// todo : reconstruct image from coefficients
#endif
	
	do
	{
		framework.process();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			gxPushMatrix();
			{
				fft2D_draw(dreal_reference, dimag_reference, transformSx_reference, transformSy_reference, "reference", t_reference);
				gxTranslatef(0, transformSy_reference, 0);
				fft2D_draw(dreal_fast, dimag_fast, transformSx_fast, transformSy_fast, "fast", t_fast);
				gxTranslatef(0, transformSy_fast, 0);
				fft2D_draw(dreal_slow, dimag_slow, transformSx_slow, transformSy_slow, "slow", t_slow);
				gxTranslatef(0, transformSy_slow, 0);
			}
			gxPopMatrix();
			
			gxTranslatef(transformSx_reference, 0, 0);
			Sprite(filename).draw();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE) && false);
	
	delete[] dreal_reference;
	delete[] dimag_reference;
	delete[] dreal_fast;
	delete[] dimag_fast;
	delete[] dreal_slow;
	delete[] dimag_slow;
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	delete image;
	image = nullptr;
}

void testFourier2d()
{
	//testFourier2dImpl<float>();
	
	testFourier2dImpl<double>();
}
