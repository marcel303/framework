#include "Debugging.h"
#include "Exception.h"
#include "StreamProvider_Replay.h"

StreamProvider_Replay::StreamProvider_Replay(Stream* dataStream)
{
	Assert(dataStream != 0);

	mDataStream = dataStream;
}

StreamProvider_Replay::~StreamProvider_Replay()
{
}

Stream* StreamProvider_Replay::OpenStream(StreamType type)
{
	switch (type)
	{
	case StreamType_Data:
		return mDataStream;
	default:
		throw ExceptionVA("unable to provide stream: %d", (int)type);
	}
}

void StreamProvider_Replay::CloseStream(StreamType type, Stream* stream)
{
	switch (type)
	{
	case StreamType_Data:
		// nop
		break;
	default:
		throw ExceptionVA("unknown stream: %d", (int)type);
	}
}
