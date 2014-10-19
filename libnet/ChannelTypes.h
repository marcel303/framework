#pragma once

#include <stdint.h>

enum ChannelType
{
	ChannelType_Client, // a connection to/from a client
	ChannelType_Server  // a connection that only exists on the server. only used to create the listen channel
};

enum ChannelSide
{
	ChannelSide_Client, // channel is created on the client side
	ChannelSide_Server  // channel is created on the server side
};
