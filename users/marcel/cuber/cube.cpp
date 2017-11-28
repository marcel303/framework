#include "cube.h"
#include "framework.h"
#include "StringEx.h"

bool EffectCtxImpl::keyIsDown(const SDLKey key) const
{
	return keyboard.isDown(key);
}

bool EffectCtxImpl::keyWentDown(const SDLKey key) const
{
	return keyboard.wentDown(key);
}

bool EffectCtxImpl::keyWentUp(const SDLKey key) const
{
	return keyboard.wentUp(key);
}

int EffectCtxImpl::mouseX() const
{
	return mouse.x;
}

int EffectCtxImpl::mouseY() const
{
	return mouse.y;
}

void EffectCtxImpl::setColor(const int r, const int g, const int b, const int a)
{
	::setColor(r, g, b, a);
}

void EffectCtxImpl::drawText(const float x, const float y, const int size, const float alignX, const float alignY, const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);

	::drawText(x, y, size, alignX, alignY, text);
}

void EffectCtxImpl::drawLine(const float x1, const float y1, const float x2, const float y2)
{
	::drawLine(x1, y1, x2, y2);
}
