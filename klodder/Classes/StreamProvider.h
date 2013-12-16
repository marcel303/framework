#pragma once

#include "libgg_forward.h"

enum StreamType
{
	StreamType_Data
};

class StreamProvider
{
public:
	virtual ~StreamProvider();
	
	virtual Stream* OpenStream(StreamType type) = 0;
	virtual void CloseStream(StreamType type, Stream* stream) = 0;
};
