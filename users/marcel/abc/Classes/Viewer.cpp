#include "HackRender.h"
#include "Viewer.h"

Viewer::Viewer()
{
}

void Viewer::Setup(Vec2F worldSize, Vec2F viewSize)
{
	mWorldSize = worldSize;
	mViewSize = viewSize;
}

void Viewer::SetFocus(Vec2F position)
{
	mPosition = position;
}

void Viewer::Apply()
{
	Vec2F min;
	Vec2F max;
	
	GetViewRect(min, max);
	
	HR_SetTranslation(-min);
}

void Viewer::GetViewRect(Vec2F& oMin, Vec2F& oMax)
{
	oMin = mPosition - mViewSize * 0.5f;
	oMax = mPosition + mViewSize * 0.5f;
}