#include "Debugging.h"
#include "PsdLog.h"
#include "PsdRect.h"
#include "PsdTypes.h"
#include "StreamReader.h"
#include "StreamWriter.h"

PsdRect::PsdRect()
{
	x1 = y1 = x2 = y2 = 0;
}

PsdRect::PsdRect(int x, int y, int sx, int sy)
{
	x1 = x;
	y1 = y;
	x2 = x + sx;
	y2 = y + sy;
}

void PsdRect::Read(StreamReader& reader)
{
	y1 = SwapU32(reader.ReadUInt32());
	x1 = SwapU32(reader.ReadUInt32());
	y2 = SwapU32(reader.ReadUInt32());
	x2 = SwapU32(reader.ReadUInt32());

	Assert(x2 >= x1);
	Assert(y2 >= y1);

	Assert(x1 <= 30000);
	Assert(y1 <= 30000);
	Assert(x2 <= 30000);
	Assert(y2 <= 30000);
	
	PSD_LOG_DBG("rect: read: (%lu, %lu), (%lu, %lu)", x1, y1, x2, y2);
}

void PsdRect::Write(StreamWriter& writer)
{
	PSD_LOG_DBG("rect: write: (%lu, %lu), (%lu, %lu)", x1, y1, x2, y2);
	
	Assert(x2 >= x1);
	Assert(y2 >= y1);
	
	Assert(x1 <= 30000);
	Assert(y1 <= 30000);
	Assert(x2 <= 30000);
	Assert(y2 <= 30000);

	writer.WriteUInt32(SwapU32(y1));
	writer.WriteUInt32(SwapU32(x1));
	writer.WriteUInt32(SwapU32(y2));
	writer.WriteUInt32(SwapU32(x2));
}
