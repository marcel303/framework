/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "vfxNodeBase.h"
#include <initializer_list>

struct VfxChannelZipper
{
	const static int kMaxChannels = 16;
	
	VfxChannelZipper(std::initializer_list<const VfxChannel*> _channels)
		: numChannels(0)
		, sx(0)
		, sy(0)
		, x(0)
		, y(0)
	{
		ctor(_channels.begin(), _channels.size());
	}
	
	VfxChannelZipper(const VfxChannel * const * _channels, const int _numChannels)
		: numChannels(0)
		, sx(0)
		, sy(0)
		, x(0)
		, y(0)
	{
		ctor(_channels, _numChannels);
	}
	
	void ctor(const VfxChannel * const * _channels, const int _numChannels)
	{
		for (int i = 0; i < _numChannels; ++i)
		{
			const VfxChannel * channel = _channels[i];
			
			if (numChannels < kMaxChannels)
			{
				channels[numChannels] = channel;
				
				if (channel != nullptr)
				{
					sx = channel->sx > sx ? channel->sx : sx;
					sy = channel->sy > sy ? channel->sy : sy;
				}
				
				numChannels++;
			}
		}
	}
	
	int size() const
	{
		return sx * sy;
	}
	
	bool doneX() const
	{
		return x == sx;
	}
	
	void nextX()
	{
		Assert(!doneX());
		
		x = x + 1;
	}
	
	bool doneY() const
	{
		return y == sy;
	}
	
	void nextY()
	{
		Assert(!doneY());
		
		y = y + 1;
		
		if (!doneY())
		{
			x = 0;
		}
	}
	
	bool done() const
	{
		if (size() == 0)
			return true;
		
		return doneX() && doneY();
	}
	
	void next()
	{
		nextX();
		
		if (doneX())
		{
			nextY();
		}
	}
	
	void restart()
	{
		x = 0;
		y = 0;
	}
	
	float read(const int channelIndex, const float defaultValue) const
	{
		const VfxChannel * channel = channels[channelIndex];
		
		if (channel == nullptr || channel->size == 0)
		{
			return defaultValue;
		}
		else
		{
			const int rx = x % channel->sx;
			const int ry = y % channel->sy;
			
			const float result = channel->data[ry * channel->sx + rx];
			
			return result;
		}
	}
	
	const VfxChannel * channels[kMaxChannels];
	int numChannels;
	
	int sx;
	int sy;
	
	int x;
	int y;
};

struct VfxChannelZipper_Cartesian
{
	const static int kMaxChannels = 16;
	
	const VfxChannel * channels[kMaxChannels];
	int numChannels;
	
	int cartesianSize;
	
	int progress[kMaxChannels] = { };
	
	bool _done = false;
	
	VfxChannelZipper_Cartesian(std::initializer_list<const VfxChannel*> _channels)
		: numChannels()
		, cartesianSize(0)
	{
		ctor(_channels.begin(), _channels.size());
	}
	
	VfxChannelZipper_Cartesian(const VfxChannel * const * _channels, const int _numChannels)
		: numChannels()
		, cartesianSize(0)
	{
		ctor(_channels, _numChannels);
	}
	
	void ctor(const VfxChannel * const * _channels, const int _numChannels)
	{
		cartesianSize = 1;
		
		for (int i = 0; i < _numChannels; ++i)
		{
			const VfxChannel * channel = _channels[i];
			
			if (numChannels < kMaxChannels)
			{
				if (channel != nullptr && channel->size > 0)
				{
					channels[numChannels] = channel;
					
					cartesianSize *= channel->size;
				}
				else
				{
					channels[numChannels] = nullptr;
				}
				
				numChannels++;
			}
		}
	}
	
	int nextChannel(const int channel) const
	{
		int result = channel;
		
		while (result < numChannels && channels[result] == nullptr)
			result++;
		
		return result;
	}
	
	int size() const
	{
		return cartesianSize;
	}
	
	bool done() const
	{
		if (size() == 0)
			return true;
		
		return _done;
	}
	
	void next()
	{
		Assert(!done());
		
		for (int channel = nextChannel(0); channel != numChannels; channel = nextChannel(channel + 1))
		{
			progress[channel]++;
			
			if (progress[channel] == channels[channel]->size)
				progress[channel] = 0;
			else
				return;
		}
		
		_done = true;
	}
	
	void restart()
	{
		memset(progress, 0, sizeof(progress));
	}
	
	float read(const int channelIndex, const float defaultValue) const
	{
		const VfxChannel * channel = channels[channelIndex];
		
		if (channel == nullptr || channel->size == 0)
		{
			return defaultValue;
		}
		else
		{
			const int index = progress[channelIndex];
			
			const float result = channel->data[index];
			
			return result;
		}
	}
};
