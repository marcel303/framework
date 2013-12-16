#include "PsdCompression.h"
#include "Stream.h"
#include "StreamWriter.h"

#define RLE_TRESHOLD 3
#define RLE_MAX 127

namespace PsdCompression
{
	int RleRepeatCount(const uint8_t* bytes, int _x, int sx)
	{
		// scan RLE_MAX pixels at most
		
		int scanX1 = _x;
		int scanX2 = _x + RLE_MAX;
		
		if (scanX2 >= sx)
			scanX2 = sx - 1;
		
		const uint8_t value = bytes[_x];
		
		int size = 0;
		
		for (int x = scanX1; x <= scanX2; ++x)
		{
			if (bytes[x] != value)
				break;
			else
				size++;
		}
		
		return size;
	}

	int RleNoRepeatCount(const uint8_t* bytes, int _x, int sx)
	{
		// scan RLE_MAX pixels at most
		
		int scanX1 = _x;
		int scanX2 = _x + RLE_MAX;
		
		if (scanX2 >= sx)
			scanX2 = sx - 1;
		
		int size = 0;
		
		for (int x = scanX1; x <= scanX2; ++x)
		{
			if (RleRepeatCount(bytes, x, sx) >= RLE_TRESHOLD)
				break;
			else
				size++;
		}
		
		return size;
	}

	void RleCompress(uint8_t* bytes, int sx, int sy, Stream* stream, std::vector<int>& rleHeader)
	{
		StreamWriter writer(stream, false);
		
		rleHeader.resize(sy);

		for (int y = 0; y < sy; ++y)
		{
			rleHeader[y] = 0;

			const int begin = stream->Position_get();
			
			for (int x = 0; x < sx;)
			{
				// find # non repeating bytes
				
				int count = 0;

				count = RleNoRepeatCount(bytes, x, sx);

				if (count >= 1)
				{
					// write header
					
					const int header = count - 1;

					writer.WriteUInt8(header);
					
					// write bytes
					
					for (int i = 0; i < count; ++i, ++x)
					{
						writer.WriteUInt8(bytes[x]);
					}
				}
				
				// find # repeating bytes
				
				count = RleRepeatCount(bytes, x, sx);
				
				if (count >= RLE_TRESHOLD)
				{
					// write header
					
					const int header = 257 - count;

					writer.WriteUInt8(header);
					
					// write RLE byte

					writer.WriteUInt8(bytes[x]);
					
					x += count;
				}
			}
			
			const int end = stream->Position_get();
			
			const int size = end - begin;
			
			rleHeader[y] = size;
			
			bytes += sx;
		}
	}
}
