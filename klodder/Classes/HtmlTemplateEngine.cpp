#if BUILD_HTTPSERVER

#include "Exception.h"
#include "FileStream.h"
#include "HtmlTemplateEngine.h"
#include "KlodderSystem.h"
#include "StreamReader.h"
#include "StringEx.h"

static char SafeRead(const std::string& text, int index)
{
	if (index < 0 || index >= (int)text.length())
		return 0;
	else
		return text[index];
}

HtmlTemplateEngine::HtmlTemplateEngine(RenderCallback renderCallback)
{
	mRenderCallback = renderCallback;
}

void HtmlTemplateEngine::SetKey(std::string key, int value)
{
	SetKey(key, String::Format("%d", value));
}

void HtmlTemplateEngine::SetKey(std::string key, std::string value)
{
	mKVPs[key] = value;
}

void HtmlTemplateEngine::SetKey(std::string key, float value, int decimals)
{
	std::string formatText = String::Format("%%.%df", decimals);
	
	SetKey(key, String::Format(formatText, value));
}

void HtmlTemplateEngine::Begin()
{
	mKVPs.clear();
	mText.clear();
}

void HtmlTemplateEngine::IncludeText(std::string text)
{
	for (int i = 0; i < (int)text.size();)
	{
		if (SafeRead(text, i) == '[')
		{
			++i;
			
			bool isMethod = false;
			
			if (SafeRead(text, i) == '!')
			{
				++i;
				
				isMethod = true;
			}
			
			std::string inner;
			
			// scan till ']' found
			
			while (SafeRead(text, i) != ']')
			{
				char c = SafeRead(text, i);
				
				if (c == 0)
					throw ExceptionVA("syntax error: no closing ']' found: %s", inner.c_str());
				
				inner.push_back(c);
				
				++i;
			}
			
			++i;
			
			if (isMethod)
			{
				if (!mRenderCallback)
					throw ExceptionVA("render callback not set");
				
				std::vector<std::string> elements = String::Split(inner, ':');
				
				if (elements.size() == 0)
					throw ExceptionVA("syntax error: missing method name: %s", inner.c_str());
				
				std::string func = elements[0];
				
				std::vector<std::string> args;
				
				for (size_t j = 1; j < elements.size(); ++j)
					args.push_back(elements[j]);
				
				mRenderCallback(*this, func, args);
			}
			else
			{
				std::map<std::string, std::string>::iterator j = mKVPs.find(inner);
				
				if (j == mKVPs.end())
					throw ExceptionVA("no value found for key %s", inner.c_str());
				
				std::string content = j->second;
				
				mText.append(content);
			}
		}
		else
		{
			mText.push_back(SafeRead(text, i));
			
			++i;
		}
	}
}

void HtmlTemplateEngine::IncludeResource(std::string resourceFileName)
{
	resourceFileName = gSystem.GetResourcePath(resourceFileName.c_str());
	
	FileStream stream;
	
	stream.Open(resourceFileName.c_str(), OpenMode_Read);
	
	StreamReader reader(&stream, false);
	
	std::string text = reader.ReadAllText();
	
	IncludeText(text);
}

std::string HtmlTemplateEngine::ToString()
{
	return mText;
}

#endif

