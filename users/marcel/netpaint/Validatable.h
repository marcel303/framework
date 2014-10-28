#ifndef VALIDATABLE_H
#define VALIDATABLE_H

#include "Rect.h"

class Validatable
{
public:
	virtual void Invalidate(const Rect& rect) = 0;
};

#endif
