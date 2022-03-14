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
	uint8_t * bytes = nullptr;
	size_t numBytes = 0;
	
	size_t nextBit = 0;
	uint8_t currentByte = 0;
	
	~BitInputStream()
	{
		close();
	}
	
	void open(const char * path)
	{
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
		delete [] bytes;
		bytes = nullptr;
		
		new (this) BitInputStream();
	}
	
	uint8_t readBit()
	{
		if ((nextBit & 7) == 0)
		{
			const size_t nextByte = nextBit >> 3;
			
			if (nextByte >= numBytes)
			{
				throw ExceptionVA("read beyond end of stream");
			}
			
			currentByte = bytes[nextByte];
		}
		
		const uint8_t result = (currentByte >> 7) & 1;
		
		nextBit++;
		
		currentByte <<= 1;
		
		return result;
	}
	
#if 0
	public int readByte() throws IOException {
		if (bitBufferLen >= 8)
			return readUint(8);
		else
			return in.read();
	}
	
	readUint:
		while (bitBufferLen < n) {
			int temp = in.read();
			if (temp == -1)
				throw new EOFException();
			bitBuffer = (bitBuffer << 8) | temp;
			bitBufferLen += 8;
		}
		bitBufferLen -= n;
		int result = (int)(bitBuffer >>> bitBufferLen);
		if (n < 32)
			result &= (1 << n) - 1;
		return result;
		
	public int readSignedInt(int n) throws IOException {
		return (readUint(n) << (32 - n)) >> (32 - n);
	}
	
	
	public long readRiceSignedInt(int param) throws IOException {
		long val = 0;
		while (readUint(1) == 0)
			val++;
		val = (val << param) | readUint(param);
		return (val >>> 1) ^ -(val & 1);
	}
#endif

	void alignToByte()
	{
		nextBit  += 7;
		nextBit >>= 3;
		nextBit <<= 3;
	}
	
	int32_t readByte()
	{
		if ((nextBit >> 3) >= numBytes)
			return -1;
		/*
	// fixme : why this branch ?
		if (bitBufferLen >= 8)
			return readUint(8);
		else
			return in.read();
		*/
		
		return readUint(8);
	}
	
	uint32_t readUint(const uint8_t numBits)
	{
		Assert(numBits <= 32);
		
	#if 1
		uint32_t result = 0;
		
		for (uint8_t i = 0; i < numBits; ++i)
		{
			result <<= 1;
			
			result |= readBit();
		}
		
		return result;
	#else
		uint64_t result = 0;
		
		for (uint8_t i = 0; i < numBits; ++i)
		{
			result |= uint64_t(readBit()) << 32;
			
			result >>= 1;
		}
		
		result >>= 32 - numBits;
		
		const uint32_t result_32 = uint32_t(result);
		
		return result_32;
	#endif
	}
	
	int32_t readSignedInt(uint8_t n)
	{
		// read unsigned
		
		int32_t result = int32_t(readUint(n));
		
		// perform sign extension to 32-bit signed int
		
		result <<= 32 - n;
		result >>= 32 - n;
		
		// return the signed result
		
		return result;
	}
	
	int32_t readRiceSignedInt(int param)
	{
		int32_t val = 0;
		
		while (readUint(1) == 0)
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
		long numSamples = -1;
		for (bool last = false; !last; )
		{
			last = in.readUint(1) != 0;
			
			int type = in.readUint(7);
			int length = in.readUint(24);
			
			if (type == 0) // Stream info block
			{
				in.readUint(16);
				in.readUint(16);
				in.readUint(24);
				in.readUint(24);
				
				sampleRate = in.readUint(20);
				numChannels = in.readUint(3) + 1;
				sampleDepth = in.readUint(5) + 1;
				numSamples = (long)in.readUint(18) << 18 | in.readUint(18);
				
				for (int i = 0; i < 16; i++)
				{
					in.readUint(8);
				}
			}
			else
			{
				for (int i = 0; i < length; i++)
				{
					in.readUint(8);
				}
			}
		}
		
		if (sampleRate == -1)
			throw ExceptionVA("Stream info metadata block absent");
		if (sampleDepth % 8 != 0)
			throw ExceptionVA("Sample depth not supported");
		
		// Start writing WAV file headers
		int sampleDataLen = numSamples * numChannels * (sampleDepth / 8);
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
	
	
	static void writeLittleInt(int numBytes, int val, OutputStream & out)
	{
		for (int i = 0; i < numBytes; i++)
		{
			out.write(val & 0xff);
			
			val >>= 8;
		}
	}
	
	static void writeString(const char * s, OutputStream &out)
	{
		out.write(s, strlen(s));
	}
	
	static bool decodeFrame(BitInputStream & in, int numChannels, int sampleDepth, OutputStream & out)
	{
		// Read a ton of header fields, and ignore most of them
		int temp = in.readByte();
		if (temp == -1)
			return false;
			
		int sync = temp << 6 | in.readUint(6);
		if (sync != 0x3FFE)
			throw ExceptionVA("Sync code expected");
		
		in.readUint(1);
		in.readUint(1);
		int blockSizeCode = in.readUint(4);
		int sampleRateCode = in.readUint(4);
		int chanAsgn = in.readUint(4);
		in.readUint(3);
		in.readUint(1);
		
		temp = Integer::numberOfLeadingZeros(~(in.readUint(8) << 24)) - 1;
		for (int i = 0; i < temp; i++)
			in.readUint(8);
		
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
			in.readUint(8);
		else if (sampleRateCode == 13 || sampleRateCode == 14)
			in.readUint(16);
		
		in.readUint(8);
		
		// Decode each channel's subframe, then skip footer
		std::vector<std::vector<int>> samples;
		samples.resize(numChannels);
		for (auto & block : samples)
			block.resize(blockSize);
		decodeSubframes(in, sampleDepth, chanAsgn, samples);
		in.alignToByte();
		in.readUint(16);
		
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
	
	static void decodeSubframes(BitInputStream & in, int sampleDepth, int chanAsgn, std::vector<std::vector<int>> & result)
	{
		int blockSize = result[0].size();
		
		std::vector<std::vector<int>> subframes;
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
					long side = subframes[1][i];
					long right = subframes[0][i] - (side >> 1);
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
				result[ch][i] = (int)subframes[ch][i];
		}
	}
	
	static void decodeSubframe(BitInputStream & in, int sampleDepth, std::vector<int> & result)
	{
		in.readUint(1);
		int type = in.readUint(6);
		int shift = in.readUint(1);
		
		if (shift == 1)
		{
			while (in.readUint(1) == 0)
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
		
		for (int i = 0; i < result.size(); i++)
		{
			result[i] <<= shift; // todo : don't shift if shift is zero
		}
	}
	
	static void decodeFixedPredictionSubframe(BitInputStream & in, int predOrder, int sampleDepth, std::vector<int> & result)
	{
		for (int i = 0; i < predOrder; i++)
		{
			result[i] = in.readSignedInt(sampleDepth);
		}
			
		decodeResiduals(in, predOrder, result);
		
		restoreLinearPrediction(result, FIXED_PREDICTION_COEFFICIENTS[predOrder], 0);
	}
	
	static void decodeLinearPredictiveCodingSubframe(BitInputStream & in, int lpcOrder, int sampleDepth, std::vector<int> & result)
	{
		for (int i = 0; i < lpcOrder; i++)
		{
			result[i] = in.readSignedInt(sampleDepth);
		}
		
		int precision = in.readUint(4) + 1;
		int shift = in.readSignedInt(5);
		
		std::vector<int> coefs;
		coefs.resize(lpcOrder);
		for (int i = 0; i < coefs.size(); i++)
			coefs[i] = in.readSignedInt(precision);
		
		decodeResiduals(in, lpcOrder, result);
		
		restoreLinearPrediction(result, coefs, shift);
	}
	
	static void decodeResiduals(BitInputStream & in, int warmup, std::vector<int> & result)
	{
		int method = in.readUint(2);
		if (method >= 2)
			throw ExceptionVA("Reserved residual coding method");
			
		int paramBits = method == 0 ? 4 : 5;
		int escapeParam = method == 0 ? 0xF : 0x1F;
		
		int partitionOrder = in.readUint(4);
		int numPartitions = 1 << partitionOrder;
		if (result.size() % numPartitions != 0)
			throw ExceptionVA("Block size not divisible by number of Rice partitions");
			
		int partitionSize = result.size() / numPartitions;
		
		for (int i = 0; i < numPartitions; i++)
		{
			int start = i * partitionSize + (i == 0 ? warmup : 0);
			int end = (i + 1) * partitionSize;
			
			int param = in.readUint(paramBits);
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
	
	static void restoreLinearPrediction(std::vector<int> & result, const std::vector<int> & coefs, int shift)
	{
		for (int i = coefs.size(); i < result.size(); i++)
		{
			long sum = 0;
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
