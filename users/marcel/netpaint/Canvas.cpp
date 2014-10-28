#include <allegro.h>
#include "Calc.h"
#include "Canvas.h"
#include "Image.h"

Canvas::Canvas()
{
	samples = 0;

	w = h = 0;
	channels = 0;

	blend_mode = BM_REPLACE;

	bmp = 0;

	handle_update = 0;
	handle_update_up = 0;
}

Canvas::~Canvas()
{
	Create(0, 0, 0);
}

void Canvas::Create(int w, int h, int channels)
{
	if (samples)
	{
		delete[] samples;
		samples = 0;

		this->w = 0;
		this->h = 0;
		this->channels = 0;

		destroy_bitmap(bmp);
		bmp = 0;

		invalidated_rectset.Clear();
	}

	int count = w * h * channels;

	if (count == 0)
		return;

	samples = new sample_t[count];

	this->w = w;
	this->h = h;
	this->channels = channels;

	bmp = create_bitmap(w, h);

	Invalidate();
}

bool Canvas::Load(const char* filename, bool greyscale)
{
	Create(0, 0, 0);

	BITMAP* bmp = load_bitmap_il(filename);

	if (bmp == 0)
	{
		return false;
	}

	// FIXME: Channel count.

	int channels = greyscale ? 1 : 3;

	Create(bmp->w, bmp->h, channels);

	for (int y = 0; y < bmp->h; ++y)
	{
		for (int x = 0; x < bmp->w; ++x)
		{
			sample_t* samples = GetPix(x, y);

			int c = getpixel(bmp, x, y);

			int r, g, b;

			r = getr(c);
			g = getg(c);
			b = getb(c);

			if (greyscale)
			{
				int v = (r + g + b) / 3;

				r = g = b = v;
			}

			float values[3];

			values[0] = ITOF(r);
			values[1] = ITOF(g);
			values[2] = ITOF(b);

			samples[0] = values[0];
			samples[1] = values[1];
			samples[2] = values[2];
		}
	}

	Invalidate();

	destroy_bitmap(bmp);

	return true;
}

void Canvas::Invalidate(const Rect& _rect)
{
	Rect rect = _rect;
	const Rect temp(0, 0, w - 1, h - 1);

	if (!rect.Clip(temp))
	{
		return;
	}

#if DEBUG_INVALIDATION
	printf("Committing rect (%d, %d) (%d, %d)\n", rect.x1, rect.y1, rect.x2, rect.y2);
#endif

	invalidated_rectset.AddClip(rect);

#if 0
	if (invalidated_rectset.m_rects.m_count > 50)
		Validate(handle_update);
#endif
}

void Canvas::Invalidate()
{
	Rect rect;

	rect.min[0] = 0;
	rect.min[1] = 0;
	rect.max[0] = w - 1;
	rect.max[1] = h - 1;

	Invalidate(rect);
}

bool Canvas::Validate()
{
	bool result = false;

	acquire_bitmap(bmp);

	if (invalidated_rectset.m_rects.m_count > 0)
	{
	#if DEBUG_INVALIDATION
		printf("Validating %d rects.\n", invalidated_rectset.m_rects.m_count);
	#endif

		for (RectNode* node = invalidated_rectset.m_rects.m_head; node; node = node->next)
		{
			const Rect& rect = node->rect;

			ValidateRect(rect);
		}

		invalidated_rectset.Clear();

		result = true;
	}

	release_bitmap(bmp);

	return result;
}

void Canvas::ValidateRect(const Rect& rect)
{
#if DEBUG_INVALIDATION
	printf("Validating (%d, %d) - (%d, %d).\n", rect.x1, rect.y1, rect.x2, rect.y2);
#endif

	// TODO: Clamp rect and remove test in inner loop.

	for (int y = rect.min[1]; y <= rect.max[1]; ++y)
	{
		for (int x = rect.min[0]; x <= rect.max[0]; ++x)
		{
			const sample_t* samples = GetPix(x, y);

			const float values[3] =
			{
				Saturate(samples[0]),
				Saturate(samples[1]),
				Saturate(samples[2])
			};

			const int temp[3] =
			{
				FTOI(values[0]),
				FTOI(values[1]),
				FTOI(values[2])
			};

			const int c = makecol32(temp[0], temp[1], temp[2]);

			_putpixel32(bmp, x, y, c);
		}
	}

#if DEBUG_INVALIDATION
	::rect(bmp, rect.min[0], rect.min[1], rect.max[0], rect.max[1], makecol(0, 255, 0));
#endif

	if (handle_update)
		handle_update(handle_update_up, this, rect);
}
