#pragma once

struct RealtimeShaderEditor;

#include "fileEditor.h"

struct FileEditor_Shader : FileEditor
{
	RealtimeShaderEditor * shaderEditor = nullptr;
	
	bool showEditor = true;
	
	FileEditor_Shader(const char * path);
	virtual ~FileEditor_Shader() override;
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
