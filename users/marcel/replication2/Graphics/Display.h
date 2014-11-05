#ifndef DISPLAY_H
#define DISPLAY_H

#include <string>

class Display
{
public:
	Display(int x, int y, int w, int h, bool fullscreen);
	virtual ~Display();
	
	virtual int GetWidth() const = 0;
	virtual int GetHeight() const = 0;
	
	virtual void* Get(const std::string& name) = 0; ///< Get window specific attribute.
	virtual bool Update() = 0;

protected:
	int x;
	int y;
	int w;
	int h;
};

#endif