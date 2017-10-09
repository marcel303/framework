/*
	Copyright (C) 2017 Marcel Smit
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

namespace tinyxml2
{
	class XMLElement;
	class XMLPrinter;
}

struct VfxTimeline
{
	static const int kMaxKeys = 1024;

	struct Key
	{
		float beat;
		float value;

		Key();

		bool operator<(const Key & other) const;
		bool operator==(const Key & other) const;
		bool operator!=(const Key & other) const;
	};
	
	float length;
	int bpm;

	Key keys[kMaxKeys];
	int numKeys;

	VfxTimeline();

	bool allocKey(Key *& key);
	void freeKey(Key *& key);
	void clearKeys();
	Key * sortKeys(Key * keyToReturn = 0);

	void save(tinyxml2::XMLPrinter * printer);
	void load(tinyxml2::XMLElement * elem);
};

struct VfxSwizzle
{
	static const int kMaxChannels = 16;

	struct Channel
	{
		int sourceIndex;
		int elemIndex;
	};

	Channel channels[kMaxChannels];
	int numChannels;

	VfxSwizzle();

	void reset();
	bool parse(const char * text);
};

struct VfxOscPath
{
	static const int kMaxPath = 256;
	
	char path[kMaxPath];
	
	VfxOscPath();
	
	void save(tinyxml2::XMLPrinter * printer);
	void load(tinyxml2::XMLElement * elem);
};
