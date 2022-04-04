/**
 * OpenAL cross platform audio library
 * Copyright (C) 2018 by Raul Herraiz.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

//#define HAVE_SSE_INTRINSICS

#define MAX_OUTPUT_CHANNELS 2

#include "fourier.h"

#include "Log.h"

#ifdef HAVE_SSE_INTRINSICS
#include <emmintrin.h>
#endif

#ifndef ALIGN16
	#if defined(MACOS) || defined(LINUX) || defined(ANDROID)
		#define ALIGN16 __attribute__((aligned(16)))
	#else
		#define ALIGN16 __declspec(align(16))
	#endif
#endif

#include <cmath>
#include <cstdlib>
#include <array>
#include <complex>
#include <algorithm>

#ifndef M_TAU
	#define M_TAU (2.0 * M_PI)
#endif

#define FLOAT_BUFFER_SIZE 512

struct FloatBufferLine
{
	double values[FLOAT_BUFFER_SIZE];
	
	double * begin() { return values; }
	const double * begin() const { return values; }
	
	double * data() { return values; }
	
	void fill_zero(int num_values)
	{
		for (int i = 0; i < num_values; ++i)
			values[i] = 0.0;
	}
};

struct EffectProps
{
	struct
	{
		int CoarseTune = 0;
		int FineTune = 0;
	} Pshifter;
};

struct ALCdevice
{
	uint32_t Frequency = 0;
};

#define FRACTIONBITS 16
#define FRACTIONONE (1 << FRACTIONBITS)

static  uint32_t fastf2u(float value)
{
	return uint32_t(value);
}

static  int double2int(double value)
{
	return int(value);
}

template <typename T> static void complex_fft(T & buffer, double direction)
{
	ALIGN16 double real[buffer.size()];
	ALIGN16 double imag[buffer.size()];
	
	const int numBits = Fourier::integerLog2(buffer.size());
	
	for (size_t i = 0; i < buffer.size(); ++i)
	{
		const int i_reversed = Fourier::reverseBits(i, numBits);
		
		real[i_reversed] = buffer[i].real();
		imag[i_reversed] = buffer[i].imag();
	}

	Fourier::fft1D(
		real,
		imag,
		buffer.size(),
		buffer.size(),
		direction > 0.0,
		false);
	
	for (size_t i = 0; i < buffer.size(); ++i)
	{
		buffer[i] =
			typename T::value_type(
				real[i],
				imag[i]);
	}
}

static size_t minz(size_t a, size_t b)
{
	return a < b ? a : b;
}

namespace {

using complex_d = std::complex<double>;

#define STFT_SIZE      1024
#define STFT_HALF_SIZE (STFT_SIZE>>1)
#define OVERSAMP       (1<<2)

#define STFT_STEP    (STFT_SIZE / OVERSAMP)
#define FIFO_LATENCY (STFT_STEP * (OVERSAMP-1))

/* Define a Hann window, used to filter the STFT input and output. */
static std::array<double,STFT_SIZE> InitHannWindow()
{
    std::array<double,STFT_SIZE> ret;
    /* Create lookup table of the Hann window for the desired size, i.e. STFT_SIZE */
    for(size_t i{0};i < STFT_SIZE>>1;i++)
    {
        constexpr double scale{M_PI / double{STFT_SIZE}};
        const double val{std::sin(static_cast<double>(i+1) * scale)};
        ret[i] = ret[STFT_SIZE-1-i] = val * val;
    }
    return ret;
}

static ALIGN16 const std::array<double,STFT_SIZE> HannWindow = InitHannWindow();

struct FrequencyBin {
    double Amplitude;
    double Frequency;
};

struct PshifterState {

    /* Effect parameters */
    size_t mCount;
    uint32_t mPitchShiftI;
    double mPitchShift;
    double mFreqPerBin;

    /* Effects buffers */
    std::array<double,STFT_SIZE> mFIFO;
    std::array<double,STFT_HALF_SIZE+1> mLastPhase;
    std::array<double,STFT_HALF_SIZE+1> mSumPhase;
    std::array<double,STFT_SIZE> mOutputAccum;

    std::array<complex_d,STFT_SIZE> mFftBuffer;

    std::array<FrequencyBin,STFT_HALF_SIZE+1> mAnalysisBuffer;
    std::array<FrequencyBin,STFT_HALF_SIZE+1> mSynthesisBuffer;

    ALIGN16 FloatBufferLine mBufferOut;

    void deviceUpdate(const ALCdevice *device);
    void update(const EffectProps *props);
    void process(const size_t samplesToDo, const FloatBufferLine & samplesIn, FloatBufferLine & samplesOut);
};

void PshifterState::deviceUpdate(const ALCdevice *device)
{
    /* (Re-)initializing parameters and clear the buffers. */
    mCount       = FIFO_LATENCY;
    mPitchShiftI = FRACTIONONE;
    mPitchShift  = 1.0;
    mFreqPerBin  = device->Frequency / double{STFT_SIZE};

    std::fill(mFIFO.begin(),            mFIFO.end(),            0.0);
    std::fill(mLastPhase.begin(),       mLastPhase.end(),       0.0);
    std::fill(mSumPhase.begin(),        mSumPhase.end(),        0.0);
    std::fill(mOutputAccum.begin(),     mOutputAccum.end(),     0.0);
    std::fill(mFftBuffer.begin(),       mFftBuffer.end(),       complex_d{});
    std::fill(mAnalysisBuffer.begin(),  mAnalysisBuffer.end(),  FrequencyBin{});
    std::fill(mSynthesisBuffer.begin(), mSynthesisBuffer.end(), FrequencyBin{});
}

void PshifterState::update(const EffectProps *props)
{
    const int tune{props->Pshifter.CoarseTune*100 + props->Pshifter.FineTune};
    const float pitch{std::pow(2.0f, static_cast<float>(tune) / 1200.0f)};
    mPitchShiftI = fastf2u(pitch*FRACTIONONE);
    mPitchShift  = mPitchShiftI * double{1.0/FRACTIONONE};
}

void PshifterState::process(const size_t samplesToDo, const FloatBufferLine & samplesIn, FloatBufferLine & samplesOut)
{
    /* Pitch shifter engine based on the work of Stephan Bernsee.
     * http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
     */

    static constexpr double expected{M_TAU / OVERSAMP};
    const double freq_per_bin{mFreqPerBin};

    for(size_t base{0u};base < samplesToDo;)
    {
        const size_t todo{minz(STFT_SIZE-mCount, samplesToDo-base)};

        /* Retrieve the output samples from the FIFO and fill in the new input
         * samples.
         */
        auto fifo_iter = mFIFO.begin() + mCount;
        std::transform(fifo_iter, fifo_iter+todo, mBufferOut.begin()+base,
            [](double d) noexcept -> float { return static_cast<float>(d); });

        std::copy_n(samplesIn.begin()+base, todo, fifo_iter);
        mCount += todo;
        base += todo;

        /* Check whether FIFO buffer is filled with new samples. */
        if(mCount < STFT_SIZE) break;
        mCount = FIFO_LATENCY;

        /* Time-domain signal windowing, store in FftBuffer, and apply a
         * forward FFT to get the frequency-domain signal.
         */
        for(size_t k{0u};k < STFT_SIZE;k++)
            mFftBuffer[k] = mFIFO[k] * HannWindow[k];
        complex_fft(mFftBuffer, -1.0);

        /* Analyze the obtained data. Since the real FFT is symmetric, only
         * STFT_HALF_SIZE+1 samples are needed.
         */
        for(size_t k{0u};k < STFT_HALF_SIZE+1;k++)
        {
            const double amplitude{std::abs(mFftBuffer[k])};
            const double phase{std::arg(mFftBuffer[k])};

            /* Compute phase difference and subtract expected phase difference */
            double tmp{(phase - mLastPhase[k]) - static_cast<double>(k)*expected};

            /* Map delta phase into +/- Pi interval */
            int qpd{double2int(tmp / M_PI)};
            tmp -= M_PI * (qpd + (qpd%2));

            /* Get deviation from bin frequency from the +/- Pi interval */
            tmp /= expected;

            /* Compute the k-th partials' true frequency, twice the amplitude
             * for maintain the gain (because half of bins are used) and store
             * amplitude and true frequency in analysis buffer.
             */
            mAnalysisBuffer[k].Amplitude = 2.0 * amplitude;
            mAnalysisBuffer[k].Frequency = (static_cast<double>(k) + tmp);

            /* Store the actual phase[k] for the next frame. */
            mLastPhase[k] = phase;
        }

        /* Shift the frequency bins according to the pitch adjustment,
         * accumulating the amplitudes of overlapping frequency bins.
         */
        std::fill(mSynthesisBuffer.begin(), mSynthesisBuffer.end(), FrequencyBin{});
        for(size_t k{0u};k < STFT_HALF_SIZE+1;k++)
        {
            const size_t j{(k*mPitchShiftI + (FRACTIONONE>>1)) >> FRACTIONBITS};
            if(j >= STFT_HALF_SIZE+1) break;

            mSynthesisBuffer[j].Amplitude += mAnalysisBuffer[k].Amplitude;
            mSynthesisBuffer[j].Frequency  = mAnalysisBuffer[k].Frequency * mPitchShift;
        }

        /* Reconstruct the frequency-domain signal from the adjusted frequency
         * bins.
         */
        for(size_t k{0u};k < STFT_HALF_SIZE+1;k++)
        {
            /* Compute bin deviation from scaled freq */
            const double tmp{mSynthesisBuffer[k].Frequency};

            /* Calculate actual delta phase and accumulate it to get bin phase */
            mSumPhase[k] += tmp * expected;

            mFftBuffer[k] = std::polar(mSynthesisBuffer[k].Amplitude, mSumPhase[k]);
        }
        /* Clear negative frequencies to recontruct the time-domain signal. */
        std::fill(mFftBuffer.begin()+STFT_HALF_SIZE+1, mFftBuffer.end(), complex_d{});

        /* Apply an inverse FFT to get the time-domain siganl, and accumulate
         * for the output with windowing.
         */
        complex_fft(mFftBuffer, 1.0);
        for(size_t k{0u};k < STFT_SIZE;k++)
            mOutputAccum[k] += HannWindow[k]*mFftBuffer[k].real() * (2.0/STFT_HALF_SIZE/OVERSAMP);

        /* Shift FIFO and accumulator. */
        fifo_iter = std::copy(mFIFO.begin()+STFT_STEP, mFIFO.end(), mFIFO.begin());
        std::copy_n(mOutputAccum.begin(), STFT_STEP, fifo_iter);
        auto accum_iter = std::copy(mOutputAccum.begin()+STFT_STEP, mOutputAccum.end(),
            mOutputAccum.begin());
        std::fill(accum_iter, mOutputAccum.end(), 0.0);
    }
    
    /* Now, mix the processed sound data to the output. */
    for (int i = 0; i < samplesToDo; ++i)
    {
		samplesOut.values[i] += mBufferOut.values[i];
		//samplesOut.values[i] += samplesIn.values[i];
	}
}

} // namespace

//

#include "framework.h"

#include "audiooutput-hd/AudioOutputHD_Native.h"

int main(int argc, char * argv[])
{
	struct MyAudioStream : AudioStreamHD
	{
		bool firstProvide = true;
		
		float phase = 0.f;
		
		PshifterState state;
		
		virtual int Provide(
			const ProvideInfo & provideInfo,
			const StreamInfo & streamInfo) override
		{
			Assert(provideInfo.numFrames <= FLOAT_BUFFER_SIZE);
			
			if (firstProvide)
			{
				firstProvide = false;
				
				ALCdevice device;
				device.Frequency = streamInfo.frameRate;
				state.deviceUpdate(&device);
			}
			
			FloatBufferLine dst;
			dst.fill_zero(provideInfo.numFrames);
			
			FloatBufferLine src;
			src.fill_zero(provideInfo.numFrames);
			
			for (int i = 0; i < provideInfo.numFrames; ++i)
			{
				src.values[i] = phase < .5f ? -1.f : +1.f;
				//src.values[i] = sinf(phase * float(M_TAU));
				
				phase += 440.f / streamInfo.frameRate;
				if (phase > 1.f)
					phase -= 1.f;
			}
			
			const int tune = mouse.x * 4;
			
			EffectProps props;
			props.Pshifter.CoarseTune = tune / 100;
			props.Pshifter.FineTune = tune % 100;
			state.update(&props);
			
			state.process(provideInfo.numFrames, src, dst);
			
			for (int c = 0; c < provideInfo.numOutputChannels; ++c)
			{
				for (int i = 0; i < provideInfo.numFrames; ++i)
				{
					provideInfo.outputSamples[c][i] = dst.values[i];
				}
			}
		}
	};
	
	MyAudioStream stream;
	
	AudioOutputHD_Native audioOutput;
	
	audioOutput.Initialize(0, 2, 48000, 64);
	audioOutput.Play(&stream);
	
	framework.enableSound = false;
	framework.init(300, 300);
	
	for (;;)
	{
		framework.waitForEvents = true;
		
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
			
		if (framework.quitRequested)
			break;
		
		framework.beginDraw(0, 0, 0, 255);
		{
		
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	audioOutput.Stop();
	audioOutput.Shutdown();
	
	return 0;
}
