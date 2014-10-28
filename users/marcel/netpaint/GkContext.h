#pragma once

#include <string>
//#include <vector>
#include "Event.h"
#include "RectSet.h"

struct BITMAP;

class GkWidget;

extern BITMAP* gk_buf;

class GkContext
{
public:
	GkContext();

	void Paint();
	void Update(float dt);

	GkWidget* Find(const std::string& name);

	void Invalidate(const Rect& rect);
	void Validate(const Rect& rect);

	void HandleEvent(const Event& e);
	static void HandleEventST(void* up, const Event& e);

	RectSet invalidated;

	GkWidget* root;
	//std::vector<GkWidget*> widgets;
};
