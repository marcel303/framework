#include <allegro.h>
#include <assert.h>
#include "GkWidget.h"

GkWidget::GkWidget()
{
	pos.p[0] = 0;
	pos.p[1] = 0;
	size.p[0] = 0;
	size.p[1] = 0;

	parent = 0;
	ctx = 0;
}

GkWidget::~GkWidget()
{
	Invalidate();
}

void GkWidget::Dispose()
{
	if (parent)
		parent->DisposeOf(this);
	else
		assert(0);
}

void GkWidget::DisposeOf(GkWidget* widget)
{
	assert(widget);

	disposed.push_back(widget);
}

void GkWidget::Add(GkWidget* widget)
{
	assert(widget);

	children.push_back(widget);

	widget->SetParent(this);
	widget->SetContext(ctx);

	widget->Invalidate();
}

void GkWidget::Remove(GkWidget* widget)
{
	assert(widget);

	for (std::vector<GkWidget*>::iterator i = children.begin(); i != children.end(); ++i)
	{
		GkWidget* w = *i;

		if (w == widget)
		{
			children.erase(i);

			delete w;

			return;
		}
	}

	assert(0);
}

GkWidget* GkWidget::Find(const std::string& name)
{
	if (this->name == name)
		return this;

	for (size_t i = 0; i < children.size(); ++i)
	{
		GkWidget* result = children[i]->Find(name);

		if (result)
			return result;
	}

	return 0;
}

void GkWidget::Paint()
{
	// Draw back to front, because we suck.

	DoPaint();

	for (size_t i = 0; i < children.size(); ++i)
		children[i]->Paint();

	// FIXME: Only if solid.
	// Cannot validate. We suck !
	//Validate();
}

void GkWidget::Update(float dt)
{
	DoUpdate(dt);

	for (size_t i = 0; i < children.size(); ++i)
		children[i]->Update(dt);

	// Delete disposed children.
	if (disposed.size() > 0)
	{
		for (size_t i = 0; i < disposed.size(); ++i)
			Remove(disposed[i]);

		disposed.clear();
	}
}

void GkWidget::DoPaint()
{
	//clear(gk_buf);
}

void GkWidget::DoUpdate(float dt)
{
}

void GkWidget::SetPos(int x, int y)
{
	Invalidate();

	this->pos.p[0] = x;
	this->pos.p[1] = y;

	Invalidate();
}

void GkWidget::SetSize(int w, int h)
{
	Invalidate();

	this->size.p[0] = w;
	this->size.p[1] = h;

	Invalidate();
}

void GkWidget::SetName(const std::string& name)
{
	this->name = name;
}

void GkWidget::SetParent(GkWidget* parent)
{
	this->parent = parent;
}

void GkWidget::SetContext(GkContext* ctx)
{
	this->ctx = ctx;

	Invalidate();
}

void GkWidget::Invalidate(const Rect& rect)
{
	if (ctx)
	{
		Rect temp = ToG(rect);

		//printf("i: %d %d - %d %d\n", temp.x1, temp.y1, temp.x2, temp.y2);

		ctx->Invalidate(temp);
	}
}

void GkWidget::Invalidate()
{
	if (size.p[0] <= 0 || size.p[1] <= 0)
		return;

	Rect rect = GetRect();

	Invalidate(rect);
}

void GkWidget::Validate()
{
	if (ctx)
	{
		Rect rect = GetRectG();

		//printf("v: %d %d - %d %d\n", rect.x1, rect.y1, rect.x2, rect.y2);

		ctx->Validate(rect);
	}
}

Rect GkWidget::ToG(const Rect& rect)
{
	Rect result = rect;

	result.x1 += pos.p[0];
	result.y1 += pos.p[1];
	result.x2 += pos.p[0];
	result.y2 += pos.p[1];

	return result;
}

Rect GkWidget::GetRect()
{
	Rect result;

	result.x1 = 0;
	result.y1 = 0;
	result.x2 = size.p[0] - 1;
	result.y2 = size.p[1] - 1;

	return result;
}

Rect GkWidget::GetRectG()
{
	Rect result;

	result = GetRect();

	result = ToG(result);

	return result;
}

void GkWidget::HandleEvent(const Event& e)
{
}
