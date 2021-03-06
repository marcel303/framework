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

#include "framework.h"

#if ENABLE_METAL

#include "internal.h" // fopen_s
#include "shaderPreprocess.h"
#include "StringEx.h"

extern void splitString(const std::string & str, std::vector<std::string> & result, char c);

static bool loadFileContents(const char * filename, bool normalizeLineEndings, char *& bytes, size_t & numBytes)
{
	bool result = true;

	bytes = 0;
	numBytes = 0;

	const char * text = nullptr;

	FILE * file = 0;

	const char * resolved_filename = framework.resolveResourcePath(filename);
	
	if (fopen_s(&file, resolved_filename, "rb") == 0)
	{
		// load source from file

		Verify(fseek(file, 0, SEEK_END) == 0);
		numBytes = ftell(file);
		Verify(fseek(file, 0, SEEK_SET) == 0);

		bytes = new char[numBytes];

		if (fread(bytes, 1, numBytes, file) != (size_t)numBytes)
		{
			result = false;
		}

		fclose(file);
	}
	else if (framework.tryGetShaderSource(filename, text))
	{
		const size_t textLength = strlen(text);

		bytes = new char[textLength];
		numBytes = textLength;

		memcpy(bytes, text, textLength * sizeof(char));
	}
	else
	{
		result = false;
	}

	if (result)
	{
		if (normalizeLineEndings)
		{
			const char * src = bytes;
			      char * dst = bytes;
			
			int dst_index = 0;
			
			for (int i = 0; i < numBytes; ++i)
				if (src[i] != '\r')
					dst[dst_index++] = src[i];
			
			numBytes = dst_index;
		}
	}
	else
	{
		if (bytes)
		{
			delete [] bytes;
			bytes = 0;
		}

		numBytes = 0;
	}

	return result;
}

bool preprocessShader(
	const std::string & source,
	std::string & destination,
	const int flags,
	std::vector<std::string> & errorMessages,
	std::vector<std::string> & includedFiles,
	int & fileId)
{
	bool result = true;

	std::vector<std::string> lines;
	
	splitString(source, lines, '\n');
	
	for (size_t i = 0; i < lines.size(); ++i)
	{
		const std::string & line = lines[i];
		const std::string trimmedLine = String::TrimLeft(lines[i]);

		const char * includeStr = "include ";

		if (strstr(trimmedLine.c_str(), includeStr) == trimmedLine.c_str())
		{
			const char * filename = trimmedLine.c_str() + strlen(includeStr);

			includedFiles.push_back(filename);
			
			char * bytes;
			size_t numBytes;

			if (!loadFileContents(filename, true, bytes, numBytes))
			{
				errorMessages.push_back(String::FormatC("failed to load include file %s", filename));
				
				logError("failed to load include file %s", filename);
				
				result = false;
			}
			else
			{
				std::string temp(bytes, numBytes);
				
				int nextFileId = fileId + 1;

				if (!preprocessShader(temp, destination, flags, errorMessages, includedFiles, nextFileId))
				{
					result = false;
				}

				delete [] bytes;
				bytes = 0;
				numBytes = 0;
			}
		}
		else
		{
			destination.append(line);
			destination.append("\n");
		}
	}

	return result;
}

bool preprocessShaderFromFile(
	const char * filename,
	std::string & destination,
	const int flags,
	std::vector<std::string> & errorMessages,
	std::vector<std::string> & includedFiles)
{
	bool result = true;
	
	char * bytes;
	size_t numBytes;

	if (!loadFileContents(filename, true, bytes, numBytes))
	{
		errorMessages.push_back(String::FormatC("failed to load file %s", filename));
		
		logError("failed to load shader file %s", filename);
		
		result = false;
	}
	else
	{
		std::string temp(bytes, numBytes);
		
		delete [] bytes;
		bytes = 0;
		numBytes = 0;
		
		//
		
		int fileId = 0;
		
		if (!preprocessShader(temp, destination, flags, errorMessages, includedFiles, fileId))
		{
			result = false;
		}
	}
	
	return result;
}

#endif
