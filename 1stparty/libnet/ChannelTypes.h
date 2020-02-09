#pragma once

#include <stdint.h>

enum ChannelType
{
	ChannelType_Connection, // a connection to/from a client
	ChannelType_Listen      // a connection which binds to a port to listen for incoming connections
};

enum ChannelPool_
{
	ChannelPool_Client, // channel is created on the client side
	ChannelPool_Server, // channel is created on the server side
	ChannelPool_CUSTOM = 8
};

enum ChannelState
{
	ChannelState_Disconnected,
	ChannelState_Connecting,
	ChannelState_Connected,
	ChannelState_Listening
};

enum ChannelSendFlag
{
	ChannelSendFlag_Unreliable = 0x1,
	ChannelSendFlag_SendImmediately = 0x2
};

typedef int ChannelPool;
