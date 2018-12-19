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

#include "Debugging.h"
#include "TextIO.h"

#if 1
#include <sys/stat.h>
#endif

namespace TextIO
{
	bool loadTextWithCallback(const char * text, LineEndings & lineEndings, LineCallback lineCallback, void * userData)
	{
		bool found_cr = false;
		bool found_lf = false;

		int lineBegin = 0;
		int lineSize = 0;
		
		//

		for (int i = 0; text[i] != 0; )
		{
			bool is_linebreak = false;

			if (text[i] == '\r')
			{
				is_linebreak = true;

				found_cr = true;

				i++;

				if (text[i] == '\n')
				{
					found_lf = true;

					i++;
				}
			}
			else if (text[i] == '\n')
			{
				is_linebreak = true;

				found_lf = true;
				
				i++;
			}
			else
			{
				lineSize++;

				i++;
			}

			if (is_linebreak)
			{
				assert(lineBegin != -1);
				
				const char * begin = text + lineBegin;
				const char * end = begin + lineSize;
				*const_cast<char*>(end) = 0;
				
				lineCallback(userData, begin, end);
				
				lineBegin = i;
				lineSize = 0;
			}
		}
		
		if (lineSize > 0)
		{
			const char * begin = text + lineBegin;
			const char * end = begin + lineSize;
			*const_cast<char*>(end) = 0;
			
			lineCallback(userData, begin, end);
		}
		
		//
		
		lineEndings =
			found_cr && found_lf
			? kLineEndings_Windows
			: kLineEndings_Unix;

		return true;
	}
	
	bool loadText(const char * text, std::vector<std::string> & lines, LineEndings & lineEndings)
	{
		bool found_cr = false;
		bool found_lf = false;

		std::string line;
		
		//

		for (int i = 0; text[i] != 0; )
		{
			bool is_linebreak = false;

			if (text[i] == '\r')
			{
				is_linebreak = true;

				found_cr = true;

				i++;

				if (text[i] != 0 && text[i] == '\n')
				{
					found_lf = true;

					i++;
				}
			}
			else if (text[i] == '\n')
			{
				is_linebreak = true;

				found_lf = true;
				
				i++;
			}
			else
			{
				line.push_back(text[i]);

				i++;
			}

			if (is_linebreak)
			{
				lines.emplace_back(std::move(line));
			}
		}
		
		if (line.empty() == false)
		{
			lines.emplace_back(std::move(line));

			line.clear();
		}
		
		//
		
		lineEndings =
			found_cr && found_lf
			? kLineEndings_Windows
			: kLineEndings_Unix;

		return true;
	}
	
	bool loadFileContents(const char * filename,
		char *& text,
	#if WINDOWS
		long & size)
	#else
		ssize_t & size)
	#endif
	{
		bool result = false;
		
		FILE * file = nullptr;
		
		//
		
		file = fopen(filename, "rb");

		if (file == nullptr)
			goto cleanup;

	#if 1
		struct stat buffer;
		if (fstat(fileno(file), &buffer) != 0)
			goto cleanup;
		
		size = buffer.st_size;
	#else
		if (fseek(file, 0, SEEK_END) != 0)
			goto cleanup;

		size = ftell(file);
		
		if (size < 0)
			goto cleanup;

		if (fseek(file, 0, SEEK_SET) != 0)
			goto cleanup;
	#endif
	
		text = new char[size + 1];
		
		if (fread(text, 1, size, file) != size)
			goto cleanup;

		text[size] = 0;
		
		result = true;
		
	cleanup:
		if (result == false)
		{
			delete [] text;
			text = nullptr;
		}

		if (file != nullptr)
		{
			fclose(file);
			file = nullptr;
		}

		return result;
	}
	
	bool load(const char * filename, std::vector<std::string> & lines, LineEndings & lineEndings)
	{
		bool result = false;
		
	#if WINDOWS
		long size = 0;
	#else
		ssize_t size = 0;
	#endif
		char * text = nullptr;

		std::string line;
		
		//

		if (loadFileContents(filename, text, size) == false)
			goto cleanup;

		if (loadText(text, lines, lineEndings) == false)
			goto cleanup;
	
		result = true;
	
	cleanup:
		delete [] text;
		text = nullptr;

		return result;
	}
	
	bool loadWithCallback(const char * filename, char *& text, LineEndings & lineEndings, LineCallback lineCallback, void * userData)
	{
		bool result = false;
		
	#if WINDOWS
		long size = 0;
	#else
		ssize_t size = 0;
	#endif
	
		if (loadFileContents(filename, text, size) == false)
			goto cleanup;
		
		if (loadTextWithCallback(text, lineEndings, lineCallback, userData) == false)
			goto cleanup;
		
		result = true;

	cleanup:
		if (result == false)
		{
			delete [] text;
			text = nullptr;
		}
		
		return result;
	}

	bool save(const char * filename, const std::vector<std::string> & lines, const LineEndings lineEndings)
	{
		bool result = false;
		
		FILE * file = nullptr;
		
		file = fopen(filename, "wb");
		
		if (file == nullptr)
			goto cleanup;
		
		for (size_t i = 0; i < lines.size(); ++i)
		{
			const std::string & line = lines[i];
			
			if (fwrite(line.c_str(), 1, line.size(), file) != line.size())
				goto cleanup;
			
			if (i + 1 < lines.size())
			{
				if (lineEndings == kLineEndings_Unix)
				{
					if (fwrite("\n", 1, 1, file) != 1)
						goto cleanup;
				}
				else if (lineEndings == kLineEndings_Windows)
				{
					if (fwrite("\r\n", 1, 2, file) != 2)
						goto cleanup;
				}
			}
		}
		
		result = true;
		
	cleanup:
		if (file != nullptr)
		{
			fclose(file);
			file = nullptr;
		}
		
		return result;
	}
}
