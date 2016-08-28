#pragma once

#include "klodder_forward.h"
#include "libgg_forward.h"
#include "Types.h"

class CommandStreamWriter
{
public:
	CommandStreamWriter(Stream * stream, const bool takeOwnership);
	~CommandStreamWriter();
	
	void Record(const CommandPacket & packet);
	
	static void DBG_TestDeserialization(Stream * stream);
	static void DBG_TestSerialization();
	
private:
	Stream * mStream;
	bool mTakeOwnership;
	StreamWriter* mStreamWriter;
};
