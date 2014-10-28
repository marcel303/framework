#pragma once

#include <string>
#include <vector>
#include "Coord.h"
#include "GkContext.h"
#include "Rect.h"

class GkWidget
{
public:
	GkWidget();
	virtual ~GkWidget();
	void Dispose();
	void DisposeOf(GkWidget* widget);
	void Add(GkWidget* widget);
	void Remove(GkWidget* widget);
	GkWidget* Find(const std::string& name);
	void Paint();
	void Update(float dt);
	virtual void DoPaint();
	virtual void DoUpdate(float dt);
	void SetPos(int x, int y);
	void SetSize(int w, int h);
	void SetName(const std::string& name);
	void SetParent(GkWidget* parent);
	void SetContext(GkContext* ctx);
	void Invalidate(const Rect& rect);
	void Invalidate();
	void Validate();
	Rect ToG(const Rect& rect);
	Rect GetRect();
	Rect GetRectG();
	virtual void HandleEvent(const Event& e);

	icoord_t pos;
	icoord_t size;
	std::string name;

	std::vector<GkWidget*> children;
	std::vector<GkWidget*> disposed;

	GkWidget* parent;
	GkContext* ctx;
};
