#pragma once

#include <string>

struct StructuredType;
struct TypeDB;

class Surface;

struct FileEditor
{
	std::string path;
	
	Surface * editorSurface = nullptr; // reference to the editor surface this file editor is asked to draw into. note that we don't own this surface ourselves
	
	virtual ~FileEditor();
	
	void clearSurface(const int r, const int g, const int b, const int a);
	
	void tickBegin(Surface * in_editorSurface);
	void tickEnd();
	
	void showErrorMessage(const char * forAction, const char * format, ...);
	
	virtual bool reflect(TypeDB & typeDB, StructuredType & type)
	{
		return false;
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
