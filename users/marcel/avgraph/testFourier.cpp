#include "framework.h"
#include "image.h"
#include "Timer.h"
#include <math.h>

extern int GFX_SX;
extern int GFX_SY;

static void fft(double * dreal, double * dimag, const int size, const int log2n, const bool inverse, const bool normalize)
{
	const double pi2 = M_PI * 2.0;
	const double scale = 1.0 / size;
	
	int n = 1;
	
	for (int k = 0; k < log2n; ++k)
	{
		const int n2 = n;
		
		n <<= 1;
		
		const double angle = (inverse) ? pi2 / n : -pi2 / n;
		double wtmp = std::sin(0.5 * angle);
		double wpr = -2.0 * wtmp * wtmp;
		double wpi = std::sin(angle);
		double wr = 1.0;
		double wi = 0.0;

		for (int m = 0; m < n2; ++m)
		{
			for (int i = m; i < size; i += n)
			{
				const int j = i + n2;
				
				const double tcreal = wr * dreal[j] - wi * dimag[j];
				const double tcimag = wr * dimag[j] + wi * dreal[j];
				
				dreal[j] = dreal[i] - tcreal;
				dimag[j] = dimag[i] - tcimag;
				
				dreal[i] += tcreal;
				dimag[i] += tcimag;
			}
			
			wtmp = wr;
			wr = wtmp * wpr - wi * wpi + wr;
			wi = wi * wpr + wtmp * wpi + wi;
		}
	}
	
	if (normalize)
	{
		for (int i = 0; i < n; ++i)
		{
			dreal[i] *= scale;
			dimag[i] *= scale;
		}
	}
}

static int reverseBits(const int value, const int numBits)
{
	int smask = 1 << (numBits - 1);
	int dmask = 1;
	
	int result = 0;
	
	for (int i = 0; i < numBits; ++i, smask >>= 1, dmask <<= 1)
	{
		if (value & smask)
			result |= dmask;
	}
	
	return result;
}

static int upperPowerOf2(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

static int integerLog2(int v)
{
	int r = 0;
	while (v > 1)
	{
		v >>= 1;
		r++;
	}
	return r;
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
	
	// perform fast fourier transform on 64 samples of complex data
	
	const int kN = 64;
	const int kLog2N = integerLog2(kN);
	
	double dreal[2][kN];
	double dimag[2][kN];
	
	int a = 0; // we need to reverse bits of data indices before doing the fft. to do so we ping-pong between two sets of data, dreal/dimag[0] and dreal/dimag[1]
	
	for (int i = 0; i < kN; ++i)
	{
		dreal[a][i] = std::cos(i / 100.0);
		dimag[a][i] = 0.0;
	}
	
	for (int i = 0; i < kN; ++i)
	{
		logDebug("[%03d] inv=1: dreal=%g, dimag=%g", i, dreal[i], dimag[i]);
	}
	
	for (int i = 0; i < kN; ++i)
	{
		const int sindex = i;
		const int dindex = reverseBits(i, kLog2N);
		
		dreal[1 - a][dindex] = dreal[a][sindex];
		dimag[1 - a][dindex] = dimag[a][sindex];
	}
	
	a = 1 - a;
	
	fft(dreal[a], dimag[a], kN, kLog2N, false, false);
	
	for (int i = 0; i < kN; ++i)
	{
		logDebug("[%03d] inv=0: dreal=%g, dimag=%g", i, dreal[a][i], dimag[a][i]);
	}
	
	for (int i = 0; i < kN; ++i)
	{
		const int sindex = i;
		const int dindex = reverseBits(i, kLog2N);
		
		dreal[1 - a][dindex] = dreal[a][sindex];
		dimag[1 - a][dindex] = dimag[a][sindex];
	}
	
	a = 1 - a;
	
	fft(dreal[a], dimag[a], kN, kLog2N, true, true);
	
	for (int i = 0; i < kN; ++i)
	{
		logDebug("[%03d] inv=1: dreal=%g, dimag=%g", i, dreal[a][i], dimag[a][i]);
	}
	
	// perform 2d fast fourier transform on image data

	const int imageSx = image->sx;
	const int imageSy = image->sy;
	const int transformSx = upperPowerOf2(imageSx);
	const int transformSy = upperPowerOf2(imageSy);

	double * freal = new double[transformSx * transformSy];
	double * fimag = new double[transformSx * transformSy];
	
	{
		// note : the 2d fourier transform is seperable, meaning we could do the transform
		//        on all samples at once, or first for each row, and then for each column
		//        of the already transformed results of the previous row-passes
		
		const int numBitsX = integerLog2(transformSx);
		const int numBitsY = integerLog2(transformSy);
		
		auto fast_t1 = g_TimerRT.TimeUS_get();
		
		// perform FFT on each row
		
		for (int y = 0; y < imageSy; ++y)
		{
			double * __restrict rreal = &freal[y * transformSx];
			double * __restrict rimag = &fimag[y * transformSx];
			
			auto line = image->getLine(y);
			
			for (int x = 0; x < imageSx; ++x)
			{
				const auto & pixel = line[x];
				const double value = (pixel.r + pixel.g + pixel.b) / 3.0;
				
				const int index = reverseBits(x, numBitsX);
				rreal[index] = value;
				rimag[index] = 0.0;
			}
			
			for (int x = imageSx; x < transformSx; ++x)
			{
				const int index = reverseBits(x, numBitsX);
				rreal[index] = 0.0;
				rimag[index] = 0.0;
			}
			
			fft(rreal, rimag, transformSx, numBitsX, false, false);
		}
		
		// perform FFT on each column
		
		// complex values for current column
		double * creal = new double[transformSy];
		double * cimag = new double[transformSy];
		
		for (int x = 0; x < transformSx; ++x)
		{
			double * __restrict frealc = &freal[x];
			double * __restrict fimagc = &fimag[x];
			
			for (int y = 0; y < imageSy; ++y)
			{
				const int index = reverseBits(y, numBitsY);
				
				creal[index] = *frealc;
				cimag[index] = *fimagc;
				
				frealc += transformSx;
				fimagc += transformSx;
			}
			
			for (int y = imageSy; y < transformSy; ++y)
			{
				const int index = reverseBits(y, numBitsY);
				creal[index] = 0.0;
				cimag[index] = 0.0;
			}
			
			fft(creal, cimag, transformSy, numBitsY, false, false);
			
			//
			
			frealc = &freal[x];
			fimagc = &fimag[x];
			
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
			
			freal[y * transformSx + x] = coss / double(image->sx * image->sy);
			fimag[y * transformSx + x] = sins / double(image->sx * image->sy);
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
	
	do
	{
		framework.process();
		
		framework.beginDraw(0, 0, 0, 0);
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
						
						const double c = freal[sy * transformSx + sx];
						const double s = fimag[sy * transformSx + sx];
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
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	delete[] freal;
	delete[] fimag;
}
