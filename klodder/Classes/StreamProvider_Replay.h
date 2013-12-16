#pragma once

#include "StreamProvider.h"

class StreamProvider_Replay : public StreamProvider
{
public:
	virtual ~StreamProvider_Replay();
	StreamProvider_Replay(Stream* dataStream);

	virtual Stream* OpenStream(StreamType type);
	virtual void CloseStream(StreamType type, Stream* stream);

private:
	Stream* mDataStream;
};
