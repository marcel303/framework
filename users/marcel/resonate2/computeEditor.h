#pragma once

#include <string>

struct GpuProgram;
class TextEditor;

struct ComputeEditor
{
	GpuProgram *& program;
	
	TextEditor * textEditor = nullptr;
	
	bool sourceIsValid = true;
	
	std::string oldText;
	
	ComputeEditor(GpuProgram *& in_program);
	~ComputeEditor();
	
	void Render();
};
