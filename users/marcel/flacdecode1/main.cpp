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

struct BitInputStream
{
	const uint8_t * __restrict bytes = nullptr;
	size_t numBytes = 0;
	
	size_t nextBit = 0;
	uint8_t currentByte = 0;
	
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
	}

	void alignToByte()
	{
		nextBit  += 7;
		nextBit >>= 3;
		nextBit <<= 3;
	}
	
	void skip(const size_t numBits)
	{
		for (size_t i = 0; i < numBits; ++i)
			readBit();
	}
	
	bool eof() const
	{
		return (nextBit >> 3) >= numBytes;
	}
	
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
	
	int64_t readRiceSignedInt(const uint8_t param)
	{
		int64_t val = 0;
		
		while (readBit() == 0)
			val++;
		
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

struct Integer
{
	static int numberOfLeadingZeros(const uint32_t value)
	{
		return __builtin_clz(value);
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
		decodeSubframes(in, sampleDepth, chanAsgn, samples);
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
	
	static void decodeSubframes(BitInputStream & in, const int sampleDepth, const int chanAsgn, std::vector<std::vector<int>> & result)
	{
		const int blockSize = result[0].size();
		
		std::vector<std::vector<int64_t>> subframes;
		subframes.resize(result.size());
		for (auto & block : subframes)
			block.resize(blockSize);
		
		if (0 <= chanAsgn && chanAsgn <= 7)
		{
			for (int ch = 0; ch < result.size(); ch++)
				decodeSubframe(in, sampleDepth, subframes[ch]);
		}
		else if (8 <= chanAsgn && chanAsgn <= 10)
		{
			decodeSubframe(in, sampleDepth + (chanAsgn == 9 ? 1 : 0), subframes[0]);
			decodeSubframe(in, sampleDepth + (chanAsgn == 9 ? 0 : 1), subframes[1]);
			
			if (chanAsgn == 8)
			{
				for (int i = 0; i < blockSize; i++)
					subframes[1][i] = subframes[0][i] - subframes[1][i];
			}
			else if (chanAsgn == 9)
			{
				for (int i = 0; i < blockSize; i++)
					subframes[0][i] += subframes[1][i];
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
		}
		else
		{
			throw ExceptionVA("Reserved channel assignment");
		}
		
		for (int ch = 0; ch < result.size(); ch++)
		{
			for (int i = 0; i < blockSize; i++)
				result[ch][i] = int32_t(subframes[ch][i]);
		}
	}
	
	static void decodeSubframe(BitInputStream & in, int sampleDepth, std::vector<int64_t> & result)
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
			std::fill(result.begin(), result.end(), in.readSignedInt(sampleDepth));
		}
		else if (type == 1) // Verbatim coding
		{
			for (int i = 0; i < result.size(); i++)
				result[i] = in.readSignedInt(sampleDepth);
		}
		else if (8 <= type && type <= 12)
		{
			decodeFixedPredictionSubframe(in, type - 8, sampleDepth, result);
		}
		else if (32 <= type && type <= 63)
		{
			decodeLinearPredictiveCodingSubframe(in, type - 31, sampleDepth, result);
		}
		else
		{
			throw ExceptionVA("Reserved subframe type");
		}
		
		if (shift != 0) // Don't shift if shift is zero
		{
			for (int i = 0; i < result.size(); i++)
			{
				result[i] <<= shift;
			}
		}
	}
	
	static void decodeFixedPredictionSubframe(BitInputStream & in, const int predOrder, const int sampleDepth, std::vector<int64_t> & result)
	{
		for (int i = 0; i < predOrder; i++)
		{
			result[i] = in.readSignedInt(sampleDepth);
		}
			
		decodeResiduals(in, predOrder, result);
		
		restoreLinearPrediction(result, FIXED_PREDICTION_COEFFICIENTS[predOrder], 0);
	}
	
	static void decodeLinearPredictiveCodingSubframe(BitInputStream & in, const int lpcOrder, const int sampleDepth, std::vector<int64_t> & result)
	{
		for (int i = 0; i < lpcOrder; i++)
		{
			result[i] = in.readSignedInt(sampleDepth);
		}
		
		const int precision = in.readUint(4) + 1;
		const int shift = in.readSignedInt(5);
		
		std::vector<int> coefs;
		coefs.resize(lpcOrder);
		for (int i = 0; i < coefs.size(); i++)
			coefs[i] = in.readSignedInt(precision);
		
		decodeResiduals(in, lpcOrder, result);
		
		restoreLinearPrediction(result, coefs, shift);
	}
	
	static void decodeResiduals(BitInputStream & in, const int warmup, std::vector<int64_t> & result)
	{
		const int method = in.readUint(2);
		if (method >= 2)
			throw ExceptionVA("Reserved residual coding method");
			
		const int paramBits = method == 0 ? 4 : 5;
		const int escapeParam = method == 0 ? 0xF : 0x1F;
		
		const int partitionOrder = in.readUint(4);
		const int numPartitions = 1 << partitionOrder;
		if (result.size() % numPartitions != 0)
			throw ExceptionVA("Block size not divisible by number of Rice partitions");
			
		const int partitionSize = result.size() / numPartitions;
		
		for (int i = 0; i < numPartitions; i++)
		{
			const int start = i * partitionSize + (i == 0 ? warmup : 0);
			const int end = (i + 1) * partitionSize;
			
			const int param = in.readUint(paramBits);
			
			if (param < escapeParam)
			{
				for (int j = start; j < end; j++)
					result[j] = in.readRiceSignedInt(param);
			}
			else
			{
				int numBits = in.readUint(5);
				for (int j = start; j < end; j++)
					result[j] = in.readSignedInt(numBits);
			}
		}
	}
	
	static void restoreLinearPrediction(std::vector<int64_t> & result, const std::vector<int> & coefs, const int shift)
	{
		for (int i = coefs.size(); i < result.size(); i++)
		{
			int64_t sum = 0;
			for (int j = 0; j < coefs.size(); j++)
				sum += result[i - 1 - j] * coefs[j];
				
			result[i] += sum >> shift;
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
	
	return 0;
}
