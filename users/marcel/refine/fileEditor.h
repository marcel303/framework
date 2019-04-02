#pragma once

#include "framework.h"
#include <string>

struct FileEditor
{
	std::string path;
	
	Surface * editorSurface = nullptr; // reference to the editor surface this file editor is asked to draw into. note that we don't own this surface ourselves
	
	virtual ~FileEditor()
	{
	}
	
	void clearSurface(const int r, const int g, const int b, const int a)
	{
		editorSurface->clear(r, g, b, a);
		
		editorSurface->clearDepth(1.f);
	}
	
	void tickBegin(Surface * in_editorSurface)
	{
		editorSurface = in_editorSurface;
	}
	
	void tickEnd()
	{
		editorSurface = nullptr;
	}
	
	virtual bool wantsTick(const bool hasFocus, const bool inputIsCaptured)
	{
		return hasFocus;
	}
	
	virtual void doButtonBar()
	{
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) = 0;
};
