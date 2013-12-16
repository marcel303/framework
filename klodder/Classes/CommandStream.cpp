#include "CommandPacket.h"
#include "CommandStream.h"
#include "Log.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"

#include "BlitTransform.h"
#include "Calc.h"
#include "MemoryStream.h" // dbg

CommandStreamWriter::CommandStreamWriter(Stream* stream, bool takeOwnership)
{
//	LOG_DBG("command packet size: %d", sizeof(CommandPacket));
	
	mStream = stream;
	mTakeOwnership = takeOwnership;
	mStreamWriter = new StreamWriter(mStream, false);
}

CommandStreamWriter::~CommandStreamWriter()
{
	delete mStreamWriter;
	mStreamWriter = 0;
	if (mTakeOwnership)
		delete mStream;
	mStream = 0;
}

void CommandStreamWriter::Record(const CommandPacket& packet)
{
	packet.Write(*mStreamWriter);
	
//	LOG_DBG("packet recorded. stream size: %d, cursor: %d", mStream->Length_get(), mStream->Position_get());
}

void CommandStreamWriter::DBG_TestDeserialization(Stream* stream)
{
	int count = 0;
	
	StreamReader reader(stream, false);
	
	while (!stream->EOF_get())
	{
		CommandPacket packet;
		
		packet.Read(reader);
		
		count++;
	}
	
	LOG_DBG("tested %d packets OK", count);
}

void CommandStreamWriter::DBG_TestSerialization()
{
	MemoryStream stream;
	StreamWriter writer(&stream, false);
	
	for (int i = 0; i < 10000; ++i)
	{
#define R1 Calc::Random(1.0f)
		
#define CASE(x, y) \
		if (rand() % 100 == 0) \
		{ \
			CommandPacket packet = CommandPacket::x y; \
			packet.Write(writer); \
		}
		
		CASE(Make_ColorSelect, (R1, R1, R1, R1));
		CASE(Make_ImageSize, (3, 320, 480));
		CASE(Make_DataLayerBlit, (0, BlitTransform()));
		CASE(Make_DataLayerClear, (0, R1, R1, R1, R1));
		CASE(Make_DataLayerMerge, (0, 1));
		CASE(Make_DataLayerOpacity, (0, R1));
//		CASE(Make_LayerOrder)(std::vector<int> layerOrder));
		CASE(Make_DataLayerSelect, (0));
		CASE(Make_DataLayerVisibility, (0, true));
		CASE(Make_StrokeBegin, (rand() % 3, true, true, 0.0f, 0.0f));
		CASE(Make_StrokeEnd, ());
		CASE(Make_StrokeMove, (0.0f, 0.0f));
		CASE(Make_ToolSelect_SoftBrush, (3, R1, R1));
		CASE(Make_ToolSelect_PatternBrush, (1000, 3, R1));
		CASE(Make_ToolSelect_SoftSmudge, (3, R1, R1, R1));
		CASE(Make_ToolSelect_PatternSmudge, (1000, 3, R1, R1));
		CASE(Make_ToolSelect_SoftEraser, (3, R1, R1));
		CASE(Make_ToolSelect_PatternEraser, (1000, 3, R1));
	}
	
	stream.Seek(0, SeekMode_Begin);
	
	DBG_TestDeserialization(&stream);
}
