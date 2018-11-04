#include "computeEditor.h"
#include "gpu.h"
#include "imgui/TextEditor.h"

ComputeEditor::ComputeEditor(GpuProgram *& in_program)
	: program(in_program)
{
	textEditor = new TextEditor();

	if (program != nullptr)
	{
		textEditor->SetText(program->source);
		
		oldText = textEditor->GetText();
	}
}

ComputeEditor::~ComputeEditor()
{
	delete textEditor;
	textEditor = nullptr;
}

void ComputeEditor::Render()
{
	TextEditor::ErrorMarkers errorMarkers;
	if (sourceIsValid == false)
		errorMarkers[1] = "Compile error";
	textEditor->SetErrorMarkers(errorMarkers);
	
	textEditor->Render("");
	
	const std::string newText = textEditor->GetText();

	if (newText != oldText)
	{
		if (program != nullptr)
		{
			sourceIsValid = program->updateSource(newText.c_str());
		}
		
		oldText = newText;
	}
}
