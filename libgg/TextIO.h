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

#include <string>
#include <vector>

namespace TextIO
{
	enum LineEndings
	{
		kLineEndings_Unix,
		kLineEndings_Windows
	};
	
	typedef void (*LineCallback)(void * userData, const char * begin, const char * end);
	
	bool loadText(const char * text, std::vector<std::string> & lines, LineEndings & lineEndings);
	bool loadTextWithCallback(const char * text, LineEndings & lineEndings, LineCallback lineCallback, void * userData);
	
	bool load(const char * filename, std::vector<std::string> & lines, LineEndings & lineEndings);
	bool loadWithCallback(const char * filename, char *& text, LineEndings & lineEndings, LineCallback lineCallback, void * userData);
	bool save(const char * filename, const std::vector<std::string> & lines, const LineEndings lineEndings);
}
