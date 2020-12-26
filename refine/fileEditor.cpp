#include "fileEditor.h"
#include "framework.h"
#include "StringEx.h" // vsprintf_s
#include <SDL2/SDL.h> // SDL_ShowSimpleMessageBox

FileEditor::~FileEditor()
{
}

void FileEditor::clearSurface(const int r, const int g, const int b, const int a)
{
	editorSurface->clear(r, g, b, a);
	
	editorSurface->clearDepth(1.f);
}

void FileEditor::tickBegin(Surface * in_editorSurface)
{
	editorSurface = in_editorSurface;
}

void FileEditor::tickEnd()
{
	editorSurface = nullptr;
}

void FileEditor::showErrorMessage(const char * forAction, const char * format, ...)
{
	va_list va;
	va_start(va, format);
	char text[4096];
	vsprintf_s(text, sizeof(text), format, va);
	va_end(va);
	
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, forAction, text, nullptr);
}
