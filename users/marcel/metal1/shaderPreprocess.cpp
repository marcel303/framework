//#include "internal.h"
#include "Log.h"
#include "shaderPreprocess.h"
#include "StringEx.h"

#define FRAMEWORK_INTEGRATION 0

static bool loadFileContents(const char * filename, bool normalizeLineEndings, char *& bytes, int & numBytes)
{
	bool result = true;

	bytes = 0;
	numBytes = 0;

	const char * text = nullptr;

	FILE * file = 0;

	if ((file = fopen(filename, "rb")) != 0)
	{
		// load source from file

		fseek(file, 0, SEEK_END);
		numBytes = ftell(file);
		fseek(file, 0, SEEK_SET);

		bytes = new char[numBytes];

		if (fread(bytes, 1, numBytes, file) != (size_t)numBytes)
		{
			result = false;
		}

		fclose(file);
	}
#if FRAMEWORK_INTEGRATION
	else if (framework.tryGetShaderSource(filename, text))
	{
		const size_t textLength = strlen(text);

		bytes = new char[textLength];
		numBytes = textLength;

		memcpy(bytes, text, textLength * sizeof(char));
	}
#endif
	else
	{
		result = false;
	}

	if (result)
	{
		if (normalizeLineEndings)
		{
			for (int i = 0; i < numBytes; ++i)
				if (bytes[i] == '\r')
					bytes[i] = '\n';
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

static void addLineAndFileMarker(std::string & destination, const int lineNumber, const int fileId)
{
	char lineText[128];
	sprintf_s(lineText, sizeof(lineText), "#line %d %d\n", int(lineNumber), fileId);
	destination.append(lineText);
}

bool preprocessShader(
	const std::string & source,
	std::string & destination,
	const int flags,
	std::vector<std::string> & errorMessages,
	int & fileId)
{
	bool result = true;

	std::vector<std::string> lines = String::Split(source, '\n');
	
	for (size_t i = 0; i < lines.size(); ++i)
	{
		if ((flags & kPreprocessShader_AddOpenglLineAndFileMarkers) != 0 && i == 0)
		{
			addLineAndFileMarker(destination, i, fileId);
		}
		
		const std::string & line = lines[i];
		const std::string trimmedLine = String::TrimLeft(lines[i]);

		const char * includeStr = "include ";

		if (strstr(trimmedLine.c_str(), includeStr) == trimmedLine.c_str())
		{
			const char * filename = trimmedLine.c_str() + strlen(includeStr);

			char * bytes;
			int numBytes;

			if (!loadFileContents(filename, true, bytes, numBytes))
			{
				errorMessages.push_back(String::FormatC("failed to load include file %s", filename));
				
				LOG_ERR("failed to load include file %s", filename);
				
				result = false;
			}
			else
			{
				std::string temp(bytes, numBytes);
				
				int nextFileId = fileId + 1;

				if (!preprocessShader(temp, destination, flags, errorMessages, nextFileId))
				{
					result = false;
				}

				delete [] bytes;
				bytes = 0;
				numBytes = 0;
				
				if ((flags & kPreprocessShader_AddOpenglLineAndFileMarkers) != 0)
				{
					addLineAndFileMarker(destination, i + 1, fileId);
				}
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
