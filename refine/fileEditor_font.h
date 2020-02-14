#pragma once

#include "fileEditor.h"

struct FileEditor_Font : FileEditor
{
	std::string path;
	
	FileEditor_Font(const char * in_path);
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
