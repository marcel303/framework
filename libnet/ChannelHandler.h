#pragma once

#include "libnet_forward.h"

class ChannelHandler
{
public:
	virtual void SV_OnChannelConnect(Channel * channel) = 0;
	virtual void SV_OnChannelDisconnect(Channel * channel) = 0;
	virtual void CL_OnChannelConnect(Channel * channel) = 0;
	virtual void CL_OnChannelDisconnect(Channel * channel) = 0;
};
