#pragma once

#include "fileEditor.h"
#include "framework.h"
#include "imgui-framework.h"

struct FileEditor_Ies : FileEditor
{
	GxTextureId texture = 0;
	
	Camera3d camera;

	FrameworkImGuiContext guiContext;
	
	bool exploreIn3d = true;

	FileEditor_Ies(const char * path);
	virtual ~FileEditor_Ies() override;
	
	virtual bool reflect(TypeDB & typeDB, StructuredType & type) override;
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
