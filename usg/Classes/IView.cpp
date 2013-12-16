#include "IView.h"

IView::~IView()
{
}

void IView::Initialize()
{
}

void IView::HandleFocus()
{
}

void IView::HandleFocusLost()
{
}

void IView::Update(float dt)
{
}

void IView::UpdateAnimation(float dt)
{
}

void IView::Render()
{
}

void IView::Render_Additive()
{
}

int IView::RenderMask_get()
{
	return 0;
}

float IView::FadeTime_get()
{
	return -1.0f;
}

bool IView::IsFadeActive(float transitionTime, float time)
{
	float fadeTime = FadeTime_get();
	
	if (fadeTime <= 0.0f)
		return false;
	
	if (time >= transitionTime + fadeTime)
		return false;
	
	return true;
}
