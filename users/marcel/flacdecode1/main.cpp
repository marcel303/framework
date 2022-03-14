/*
 * Simple FLAC decoder (Java)
 *
 * Copyright (c) 2017 Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/simple-flac-implementation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */

// c++ port from the Java decoder found over here:
// https://www.nayuki.io/res/simple-flac-implementation/SimpleDecodeFlacToWav.java

#include "framework.h"

#include "Exception.h"
#include "TextIO.h"

#include <stdio.h>

#define USE_BITBUFFER 1

struct Integer
{
	static int numberOfLeadingZeros(const uint32_t value)
	{
		return __builtin_clz(value);
	}
	
	static int numberOfLeadingZeros64(const uint64_t value)
	{
		return __builtin_clzll(value);
	}
};

struct BitInputStream
{
	const uint8_t * __restrict bytes = nullptr;
	size_t numBytes = 0;
	
#if USE_BITBUFFER == 1
	size_t nextByte = 0;
	uint64_t bitBuffer = 0;
	uint8_t bitBufferLen = 0;
#else
	size_t nextBit = 0;
	uint8_t currentByte = 0;
#endif

	~BitInputStream()
	{
		close();
	}
	
	void open(void * in_bytes, const int in_numBytes)
	{
		close();
		
		bytes = (uint8_t*)in_bytes;
		numBytes = in_numBytes;
	}
	
	void open(const char * path)
	{
		close();
		
		char * text;
		size_t size;
		
		if (TextIO::loadFileContents(path, text, size) == false)
		{
			throw ExceptionVA("failed to read from file");
		}
		
		bytes = (uint8_t*)text;
		numBytes = size;
	}
	
	void close()
	{
		// deallocate memory
		
		delete [] bytes;
		bytes = nullptr;
		
		// reset all the fields
		
		new (this) BitInputStream();
	}
	
	uint8_t readBit()
	{
	#if USE_BITBUFFER == 0
		// should we load the next byte ?
		
		if ((nextBit & 7) == 0)
		{
			const size_t nextByte = nextBit >> 3;
			
			currentByte = bytes[nextByte];
		}
		
		const uint8_t result = (currentByte >> 7) & 1;
		
		nextBit++;
		
		currentByte <<= 1;
		
		return result;
	#else
		if (bitBufferLen == 0)
		{
			const uint8_t temp = bytes[nextByte];
			
			bitBuffer = temp;
			bitBufferLen = 8;
			
			nextByte++;
		}
		
		bitBufferLen -= 1;
		
		uint32_t result = (bitBuffer >> bitBufferLen) & 1;
			
		return result;
	#endif
	}

	void alignToByte()
	{
	#if USE_BITBUFFER == 0
		nextBit  += 7;
		nextBit >>= 3;
		nextBit <<= 3;
	#else
		bitBufferLen -= bitBufferLen & 7;
	#endif
	}
	
	void skip(const size_t numBits)
	{
	#if USE_BITBUFFER == 0
		for (size_t i = 0; i < numBits; ++i)
			readBit();
	#else
		size_t todo = numBits;
		
		const size_t todo_bitBuffer =
			todo < bitBufferLen
				? todo
				: bitBufferLen;
		
		bitBufferLen -= todo_bitBuffer;
		
		todo -= todo_bitBuffer;
		
		const size_t todo_bytes = todo >> 3;
		
		nextByte += todo_bytes;
		
		todo -= todo_bytes << 3;
		
		if (todo > 0)
		{
			readUint(todo);
		}
	#endif
	}
	
	bool eof() const
	{
	#if USE_BITBUFFER == 0
		return (nextBit >> 3) >= numBytes;
	#else
		return nextByte == numBytes;
	#endif
	}
	
#if USE_BITBUFFER == 1
	uint32_t readUint(const uint8_t numBits)
	{
		Assert(numBits <= 32);
		
		while (bitBufferLen < numBits)
		{
			const uint8_t temp = bytes[nextByte];
			
			bitBuffer = (bitBuffer << 8) | temp;
			bitBufferLen += 8;
			
			nextByte++;
		}
		
		bitBufferLen -= numBits;
		
		uint32_t result = bitBuffer >> bitBufferLen;
		
		if (numBits < 32)
			result &= (1 << numBits) - 1;
			
		return result;
	}
#else
	uint32_t readUint(const uint8_t numBits)
	{
		Assert(numBits <= 32);
		
		uint32_t result = 0;
		
		for (uint8_t i = 0; i < numBits; ++i)
		{
			result <<= 1;
			
			result |= readBit();
		}
		
		return result;
	}
#endif
	
	int32_t readSignedInt(const uint8_t n)
	{
		// read unsigned
		
		int32_t result = int32_t(readUint(n));
		
		// perform sign extension to 32-bit signed int
		
		result <<= 32 - n;
		result >>= 32 - n;
		
		// return the signed result
		
		return result;
	}
	
	int64_t readRiceSignedInt()
	{
		int64_t val = 0;
		
	#if 1
		while (bitBufferLen < 64 - 8 && nextByte != numBytes)
		{
			const uint8_t temp = bytes[nextByte];
			
			bitBuffer = (bitBuffer << 8) | temp;
			bitBufferLen += 8;
			
			nextByte++;
		}
		
		uint8_t numZeros = Integer::numberOfLeadingZeros64(bitBuffer << (64 - bitBufferLen));
		
		if (numZeros > bitBufferLen)
			numZeros = bitBufferLen;
		
		val = numZeros;
		
		bitBufferLen -= numZeros;
		
		if (bitBufferLen == 0)
		{
			// the entire bit buffer contained zeros. keep reading individual bits until we find a non-zero one
			
			while (readBit() == 0)
				val++;
		}
		else
		{
			// consume the final non-zero bit
			
			bitBufferLen -= 1;
		}
	#else
		while (readBit() == 0)
			val++;
	#endif
		
		return (val >> 1) ^ -(val & 1);
	}
	
	int64_t readRiceSignedInt(const uint8_t param)
	{
		int64_t val = 0;
		
	#if 1
		while (bitBufferLen < 64 - 8 && nextByte != numBytes)
		{
			const uint8_t temp = bytes[nextByte];
			
			bitBuffer = (bitBuffer << 8) | temp;
			bitBufferLen += 8;
			
			nextByte++;
		}
		
		uint8_t numZeros = Integer::numberOfLeadingZeros64(bitBuffer << (64 - bitBufferLen));
		
		if (numZeros > bitBufferLen)
			numZeros = bitBufferLen;
		
		val = numZeros;
		
		bitBufferLen -= numZeros;
		
		if (bitBufferLen == 0)
		{
			// the entire bit buffer contained zeros. keep reading individual bits until we find a non-zero one
			
			while (readBit() == 0)
				val++;
		}
		else
		{
			// consume the final non-zero bit
			
			bitBufferLen -= 1;
		}
	#else
		while (readBit() == 0)
			val++;
	#endif
	
		val = (val << param) | readUint(param);
		
		return (val >> 1) ^ -(val & 1);
	}
};

struct OutputStream
{
	FILE * file = nullptr;
	
	~OutputStream()
	{
		close();
	}
	
	void open(const char * path)
	{
		close();
		
		file = fopen(path, "wb");
		
		if (file == nullptr)
		{
			throw ExceptionVA("failed to open file");
		}
	}
	
	void close()
	{
		if (file != nullptr)
		{
			fclose(file);
			file = nullptr;
		}
	}
	
	void write(const uint8_t value)
	{
		if (fwrite(&value, 1, 1, file) != 1)
		{
			throw ExceptionVA("failed to write to file");
		}
	}
	
	void write(const void * bytes, const int numBytes)
	{
		if (fwrite(bytes, numBytes, 1, file) != 1)
		{
			throw ExceptionVA("failed to write to file");
		}
	}
};

static const std::vector<std::vector<int>> FIXED_PREDICTION_COEFFICIENTS =
{
	{ },
	{ 1 },
	{ 2, -1 },
	{ 3, -3, 1 },
	{ 4, -6, 4, -1 },
};

struct SimpleDecodeFlacToWav
{
	static void decodeFile(BitInputStream & in, OutputStream & out)
	{
		// Handle FLAC header and metadata blocks
		
		if (in.readUint(32) != 0x664C6143)
			throw ExceptionVA("Invalid magic string");
			
		int sampleRate = -1;
		int numChannels = -1;
		int sampleDepth = -1;
		int64_t numSamples = -1;
		
		for (;;)
		{
			const bool last = in.readBit() != 0;
			
			const int type = in.readUint(7);
			const int length = in.readUint(24);
			
			if (type == 0) // Stream info block
			{
				in.skip(16);
				in.skip(16);
				in.skip(24);
				in.skip(24);
				
				sampleRate = in.readUint(20);
				numChannels = in.readUint(3) + 1;
				sampleDepth = in.readUint(5) + 1;
				numSamples = int64_t(in.readUint(18)) << 18 | in.readUint(18);
				
				in.skip(16 * 8);
			}
			else
			{
				in.skip(length * 8);
			}
			
			if (last)
				break;
		}
		
		if (sampleRate == -1)
			throw ExceptionVA("Stream info metadata block absent");
		if (sampleDepth % 8 != 0)
			throw ExceptionVA("Sample depth not supported");
		
		// Start writing WAV file headers
		const int64_t sampleDataLen = numSamples * numChannels * (sampleDepth / 8);
		writeString("RIFF", out);
		writeLittleInt(4, (int)sampleDataLen + 36, out);
		writeString("WAVE", out);
		writeString("fmt ", out);
		writeLittleInt(4, 16, out);
		writeLittleInt(2, 0x0001, out);
		writeLittleInt(2, numChannels, out);
		writeLittleInt(4, sampleRate, out);
		writeLittleInt(4, sampleRate * numChannels * (sampleDepth / 8), out);
		writeLittleInt(2, numChannels * (sampleDepth / 8), out);
		writeLittleInt(2, sampleDepth, out);
		writeString("data", out);
		writeLittleInt(4, (int)sampleDataLen, out);
		
		// Decode FLAC audio frames and write raw samples
		while (decodeFrame(in, numChannels, sampleDepth, out))
		{
			//
		}
	}
	
	
	static void writeLittleInt(const uint8_t numBytes, int32_t val, OutputStream & out)
	{
		for (uint8_t i = 0; i < numBytes; i++)
		{
			out.write(val & 0xff);
			
			val >>= 8;
		}
	}
	
	static void writeString(const char * s, OutputStream & out)
	{
		out.write(s, strlen(s));
	}
	
	static bool decodeFrame(BitInputStream & in, const int numChannels, const int sampleDepth, OutputStream & out)
	{
		// Read a ton of header fields, and ignore most of them
		if (in.eof())
			return false;;
		const int hdr = in.readUint(8);
			
		const int sync = hdr << 6 | in.readUint(6);
		if (sync != 0x3FFE)
			throw ExceptionVA("Sync code expected");
		
		in.skip(1);
		in.skip(1);
		const int blockSizeCode = in.readUint(4);
		const int sampleRateCode = in.readUint(4);
		const int chanAsgn = in.readUint(4);
		in.skip(3);
		in.skip(1);
		
		const int temp = Integer::numberOfLeadingZeros(~(in.readUint(8) << 24)) - 1;
		if (temp != -1)
			in.skip(temp * 8);
		
		int blockSize;
		
		if (blockSizeCode == 1)
			blockSize = 192;
		else if (2 <= blockSizeCode && blockSizeCode <= 5)
			blockSize = 576 << (blockSizeCode - 2);
		else if (blockSizeCode == 6)
			blockSize = in.readUint(8) + 1;
		else if (blockSizeCode == 7)
			blockSize = in.readUint(16) + 1;
		else if (8 <= blockSizeCode && blockSizeCode <= 15)
			blockSize = 256 << (blockSizeCode - 8);
		else
			throw ExceptionVA("Reserved block size");
		
		if (sampleRateCode == 12)
			in.skip(8);
		else if (sampleRateCode == 13 || sampleRateCode == 14)
			in.skip(16);
		
		in.skip(8);
		
		// Decode each channel's subframe, then skip footer
		std::vector<std::vector<int>> samples;
		samples.resize(numChannels);
		for (auto & block : samples)
			block.resize(blockSize);
		decodeSubframes(in, sampleDepth, chanAsgn, samples, numChannels);
		in.alignToByte();
		in.skip(16);
		
		// Write the decoded samples
		for (int i = 0; i < blockSize; i++)
		{
			for (int j = 0; j < numChannels; j++)
			{
				int val = samples[j][i];
				if (sampleDepth == 8)
					val += 128;
					
				writeLittleInt(sampleDepth / 8, val, out);
			}
		}
		
		return true;
	}
	
	static void decodeSubframes(BitInputStream & in, const int sampleDepth, const int chanAsgn, std::vector<std::vector<int>> & result, const int numChannels)
	{
		const int blockSize = result[0].size();
		
		if (chanAsgn >= 0 && chanAsgn <= 7)
		{
			std::vector<std::vector<int64_t>> subframes;
			subframes.resize(numChannels);
			for (auto & block : subframes)
				block.resize(blockSize);
				
			for (int ch = 0; ch < numChannels; ch++)
			{
				decodeSubframe(in, sampleDepth, subframes[ch].data(), subframes[ch].size());
			}
			
			for (int ch = 0; ch < numChannels; ch++)
			{
				const int64_t * __restrict subframes_ch = subframes[ch].data();
						  int * __restrict result_ch = result[ch].data();
				
				for (int i = 0; i < blockSize; i++)
				{
					result_ch[i] = int32_t(subframes_ch[i]);
				}
			}
		}
		else if (chanAsgn >= 8 && chanAsgn <= 10)
		{
			Assert(numChannels == 2);
			
			int64_t * __restrict subframes[2] =
			{
				(int64_t*)alloca(blockSize * sizeof(int64_t)),
				(int64_t*)alloca(blockSize * sizeof(int64_t))
			};
			
			decodeSubframe(in, sampleDepth + (chanAsgn == 9 ? 1 : 0), subframes[0], blockSize);
			decodeSubframe(in, sampleDepth + (chanAsgn == 9 ? 0 : 1), subframes[1], blockSize);
			
			if (chanAsgn == 8)
			{
				for (int i = 0; i < blockSize; i++)
				{
					subframes[1][i] = subframes[0][i] - subframes[1][i];
				}
			}
			else if (chanAsgn == 9)
			{
				for (int i = 0; i < blockSize; i++)
				{
					subframes[0][i] += subframes[1][i];
				}
			}
			else if (chanAsgn == 10)
			{
				for (int i = 0; i < blockSize; i++)
				{
					const int64_t side  = subframes[1][i];
					const int64_t right = subframes[0][i] - (side >> 1);
					
					subframes[1][i] = right;
					subframes[0][i] = right + side;
				}
			}
			
			for (int ch = 0; ch < numChannels; ch++)
			{
				const int64_t * __restrict subframes_ch = subframes[ch];
						  int * __restrict result_ch = result[ch].data();
				
				for (int i = 0; i < blockSize; i++)
				{
					result_ch[i] = int32_t(subframes_ch[i]);
				}
			}
		}
		else
		{
			throw ExceptionVA("Reserved channel assignment");
		}
	}
	
	static void decodeSubframe(BitInputStream & in, int sampleDepth, int64_t * __restrict result, const int result_size)
	{
		in.skip(1);
		const int type = in.readUint(6);
		int shift = in.readBit();
		
		if (shift == 1)
		{
			while (in.readBit() == 0)
				shift++;
		}
		
		sampleDepth -= shift;
		
		if (type == 0) // Constant coding
		{
			std::fill(result, result + result_size, in.readSignedInt(sampleDepth));
		}
		else if (type == 1) // Verbatim coding
		{
			for (int i = 0; i < result_size; i++)
			{
				result[i] = in.readSignedInt(sampleDepth);
			}
		}
		else if (8 <= type && type <= 12)
		{
			decodeFixedPredictionSubframe(in, type - 8, sampleDepth, result, result_size);
		}
		else if (32 <= type && type <= 63)
		{
			decodeLinearPredictiveCodingSubframe(in, type - 31, sampleDepth, result, result_size);
		}
		else
		{
			throw ExceptionVA("Reserved subframe type");
		}
		
		if (shift != 0) // Don't shift if the shift amount is zero
		{
			for (int i = 0; i < result_size; i++)
			{
				result[i] <<= shift;
			}
		}
	}
	
	static void decodeFixedPredictionSubframe(BitInputStream & in, const int predOrder, const int sampleDepth, int64_t * __restrict result, const int result_size)
	{
		for (int i = 0; i < predOrder; i++)
		{
			result[i] = in.readSignedInt(sampleDepth);
		}
			
		decodeResiduals(in, predOrder, result, result_size);
		
		auto & coefs = FIXED_PREDICTION_COEFFICIENTS[predOrder];
		
		restoreLinearPrediction(result, result_size, coefs.data(), coefs.size(), 0);
	}
	
	static void decodeLinearPredictiveCodingSubframe(BitInputStream & in, const int lpcOrder, const int sampleDepth, int64_t * __restrict result, const int result_size)
	{
		for (int i = 0; i < lpcOrder; i++)
		{
			result[i] = in.readSignedInt(sampleDepth);
		}
		
		const int precision = in.readUint(4) + 1;
		const int shift = in.readSignedInt(5);
		
		const int coefs_size = lpcOrder;
		int * coefs = (int*)alloca(coefs_size * sizeof(int));
		for (int i = 0; i < coefs_size; i++)
			coefs[i] = in.readSignedInt(precision);
		
		decodeResiduals(in, lpcOrder, result, result_size);
		
		restoreLinearPrediction(result, result_size, coefs, coefs_size, shift);
	}
	
	static void decodeResiduals(BitInputStream & in, const int warmup, int64_t * __restrict result, const int result_size)
	{
		const int method = in.readUint(2);
		if (method >= 2)
			throw ExceptionVA("Reserved residual coding method");
			
		const int paramBits = method == 0 ? 4 : 5;
		const int escapeParam = method == 0 ? 0xF : 0x1F;
		
		const int partitionOrder = in.readUint(4);
		const int numPartitions = 1 << partitionOrder;
		if (result_size % numPartitions != 0)
			throw ExceptionVA("Block size not divisible by number of Rice partitions");
			
		const int partitionSize = result_size / numPartitions;
		
		for (int i = 0; i < numPartitions; i++)
		{
			const int start = i * partitionSize + (i == 0 ? warmup : 0);
			const int end = (i + 1) * partitionSize;
			
			const uint8_t param = in.readUint(paramBits);
			
			if (param < escapeParam)
			{
				if (param == 0)
				{
					for (int j = start; j < end; j++)
						result[j] = in.readRiceSignedInt();
				}
				else
				{
					for (int j = start; j < end; j++)
						result[j] = in.readRiceSignedInt(param);
				}
			}
			else
			{
				const uint8_t numBits = in.readUint(5);
				
				for (int j = start; j < end; j++)
					result[j] = in.readSignedInt(numBits);
			}
		}
	}
	
	template <int coefs_size>
	static void restoreLinearPrediction(
		int64_t * __restrict result,
		const int result_size,
		const int * __restrict coefs,
		const int shift)
	{
		for (int i = coefs_size; i < result_size; i++)
		{
			int64_t coef_result[coefs_size];
			for (int j = 0; j < coefs_size; j++)
				coef_result[j] = result[i - 1 - j] * coefs[j];
			
			int64_t sum = 0;
			for (int j = 0; j < coefs_size; j++)
				sum += coef_result[j];
			
			result[i] += sum >> shift;
		}
	}
	
	static void restoreLinearPrediction(
		int64_t * __restrict result,
		const int result_size,
		const int * __restrict coefs,
		const int coefs_size,
		const int shift)
	{
		switch (coefs_size)
		{
	#if 1
		case 1: restoreLinearPrediction<1>(result, result_size, coefs, shift); break;
		case 2: restoreLinearPrediction<2>(result, result_size, coefs, shift); break;
		case 3: restoreLinearPrediction<3>(result, result_size, coefs, shift); break;
		case 4: restoreLinearPrediction<4>(result, result_size, coefs, shift); break;
		case 5: restoreLinearPrediction<5>(result, result_size, coefs, shift); break;
		case 6: restoreLinearPrediction<6>(result, result_size, coefs, shift); break;
		case 7: restoreLinearPrediction<7>(result, result_size, coefs, shift); break;
		case 8: restoreLinearPrediction<8>(result, result_size, coefs, shift); break;
	#endif
		default:
			for (int i = coefs_size; i < result_size; i++)
			{
				int64_t sum = 0;
				for (int j = 0; j < coefs_size; j++)
					sum += result[i - 1 - j] * coefs[j];
					
				result[i] += sum >> shift;
			}
		}
	}
};

//

int main(int argc, char * argv[])
{
	if (argc != 3)
	{
		logError("Usage: java SimpleDecodeFlacToWav InFile.flac OutFile.wav");
		return -1;
	}
	
	//for (int i = 0; i < 100; ++i)
	{
	try
	{
		const char * in_path = argv[1];
		const char * out_path = argv[2];
		
		BitInputStream in;
		in.open(in_path);
		
		OutputStream out;
		out.open(out_path);
		
		SimpleDecodeFlacToWav::decodeFile(in, out);
	}
	catch (std::exception & e)
	{
		logError("error: %s", e.what());
	}
	}
	
	return 0;
}
