#if BUILD_HTTPSERVER

#pragma once

#include <map>
#include <string>
#include <vector>

// requirements:
// - template header
// - template footer
// - build tables using begin/row/end templates
// - support string substitution using dictionary type

class HtmlTemplateEngine
{
public:
	typedef void (*RenderCallback)(HtmlTemplateEngine& engine, std::string func, std::vector<std::string> args);
	
	HtmlTemplateEngine(RenderCallback renderCallback);
	
	void SetKey(std::string key, int value);
	void SetKey(std::string key, std::string value);
	void SetKey(std::string key, float value, int decimals);
	
	void Begin();
	void IncludeText(std::string text);
	void IncludeResource(std::string resourceFileName);
	
	std::string ToString();
	
private:
//	std::string Transform(std::string text);
	
	RenderCallback mRenderCallback;
	std::map<std::string, std::string> mKVPs;
	std::string mText;
};

#endif
