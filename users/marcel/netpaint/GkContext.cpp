#include <allegro.h>
#include <assert.h>
#include "GkContext.h"
#include "GkWidget.h"

static void HandleEventR(const Event& e, GkWidget* w);

BITMAP* gk_buf = 0;

GkContext::GkContext()
{
	root = new GkWidget;

	root->SetContext(this);

	gk_buf = create_bitmap(SCREEN_W, SCREEN_H); // fixme.
}

void GkContext::Paint()
{
	if (invalidated.m_rects.m_count == 0)
		return;

#if DEBUG_INVALIDATION
	printf("Validating %d rects.\n", invalidated.m_rects.m_count);
#endif

	for (RectNode* node = invalidated.m_rects.m_head; node; node = node->next)
	{
		Rect rect = node->rect;

		set_clip_rect(gk_buf, rect.x1, rect.y1, rect.x2, rect.y2);

		root->Paint();

		int w = rect.x2 - rect.x1 + 1;
		int h = rect.y2 - rect.y1 + 1;

		blit(gk_buf, screen, rect.x1, rect.y1, rect.x1, rect.y1, w, h);
	}

	set_clip_rect(gk_buf, 0, 0, gk_buf->w - 1, gk_buf->h - 1);

	invalidated.Clear();
}

void GkContext::Update(float dt)
{
	root->Update(dt);
}

GkWidget* GkContext::Find(const std::string& name)
{
	return root->Find(name);
}

void GkContext::Invalidate(const Rect& rect)
{
	invalidated.AddClip(rect);
}

void GkContext::Validate(const Rect& rect)
{
	invalidated.Sub(rect);
}

void GkContext::HandleEvent(const Event& e)
{
	// TODO: hover & active.

	/*
	if (e.type == EV_MOUSEDOWN)
	{
		// Select active widget.
	}*/

	/*
	GkWidget* active = root; // fixme.

	Event e2 = e;

	if (e2.type == EV_MOUSEMOVE)
	{
		e2.a1 -= active->pos.p[0];
		e2.a2 -= active->pos.p[1];
	}

	if (e2.type == EV_MOUSEDOWN || e2.type == EV_MOUSEUP)
	{
		e2.a2 -= active->pos.p[0];
		e2.a3 -= active->pos.p[1];
	}
*/
	//active->HandleEvent(e2);

	HandleEventR(e, root);
}

void GkContext::HandleEventST(void* up, const Event& e)
{
	GkContext* me = (GkContext*)up;

	me->HandleEvent(e);
}

static void HandleEventR(const Event& e, GkWidget* w)
{
	Event e2 = e;

	if (e2.type == EV_MOUSEMOVE)
	{
		e2.a1 -= w->pos.p[0];
		e2.a2 -= w->pos.p[1];
	}

	if (e2.type == EV_MOUSEDOWN || e2.type == EV_MOUSEUP)
	{
		e2.a2 -= w->pos.p[0];
		e2.a3 -= w->pos.p[1];
	}

	w->HandleEvent(e2);

	for (size_t i = 0; i < w->children.size(); ++i)
		HandleEventR(e, w->children[i]);
}
