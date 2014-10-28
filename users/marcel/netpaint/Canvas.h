#pragma once

#include "Calc.h"
#include "Debugging.h"
#include "Rect.h"
#include "RectSet.h"
#include "Sample.h"
#include "Validatable.h"

struct BITMAP;

#ifdef DEBUG
	#define DEBUG_INVALIDATION 0
#else
	#define DEBUG_INVALIDATION 0 // do not change!
#endif

enum BLEND_MODE
{
	BM_REPLACE,
	BM_ADD,
	BM_SUB,
	BM_BURN, // ??
	BM_MULTIPLY,
	BM_DIFFERENCE, // abs(dst - src)
	BM_SCREEN, // 1 - (1-src * 1-dst) (??)
	BM_OVERLAY, // ??
	BM_COLORIZE
};

class Canvas : public Validatable
{
public:
	Canvas();

	~Canvas();

	void Create(int w, int h, int channels);

	bool Load(const char* filename, bool greyscale);

	inline bool Outside(int x, int y)
	{
		return x < 0 || y < 0 || x >= w || y >= h;
	}

	inline void Paint(int x, int y, float value, const float* color, float opacity)
	{
		if (Outside(x, y))
			return;

		sample_t* dst = GetPix(x, y);

		float temp[MAX_CHANNELS];

#ifdef DEBUG // FIXME: Remove once all blend modes implemented.
		for (int i = 0; i < MAX_CHANNELS; ++i)
			temp[i] = i % 2;
#endif

		Blend(color, dst, opacity * value, temp);

		for (int i = 0; i < channels; ++i)
			dst[i] = temp[i];
	}

	inline sample_t* GetPix(int x, int y)
	{
		if (Outside(x, y))
			return 0;

		const int index = (x + y * w) * channels;

		return &samples[index];
	}

	inline void GetPixAA(float x, float y, sample_t** out_pix, float* out_w)
	{
		const int ix1 = FL(x);
		const int iy1 = FL(y);
		const int ix2 = ix1 + 1;
		const int iy2 = iy1 + 1;
		out_pix[0] = GetPix(ix1, iy1);
		out_pix[1] = GetPix(ix2, iy1);
		out_pix[2] = GetPix(ix1, iy2);
		out_pix[3] = GetPix(ix2, iy2);
		const float wx2 = x - ix1;
		const float wy2 = y - iy1;
		const float wx1 = 1.0f - wx2;
		const float wy1 = 1.0f - wy2;
		out_w[0] = wx1 * wy1;
		out_w[1] = wx2 * wy1;
		out_w[2] = wx1 * wy2;
		out_w[3] = wx2 * wy2;

#if 0
		float w = 0.0f;
		for (int i = 0; i < 4; ++i)
			if (out_pix[i])
				w += out_w[i];
		if (w != 0.0f)
		{
			const float w_1 = 1.0f / w;
			for (int i = 0; i < 4; ++i)
				out_w[i] *= w_1;
		}
#endif
	}

	inline void GetPixAA(float x, float y, sample_t* out_pix)
	{
		sample_t* pix[4];
		float w[4];

		GetPixAA(x, y, pix, w);

		for (int i = 0; i < channels; ++i)
			out_pix[i] = 0.0f;

		for (int j = 0; j < 4; ++j)
		{
			if (pix[j])
				for (int i = 0; i < channels; ++i)
					out_pix[i] += pix[j][i] * w[j];
		}
	}

	inline void Blend(const sample_t* src, const sample_t* dst, float opacity, sample_t* out)
	{
		#define LOOP() for (int i = 0; i < channels; ++i)

		switch (blend_mode)
		{
		case BM_REPLACE:
			LOOP()
			{
				out[i] = src[i];
			}
			break;
		case BM_ADD:
			LOOP()
			{
				out[i] = dst[i] + src[i];
			}
			break;
		case BM_SUB:
			LOOP()
			{
				out[i] = dst[i] - src[i];
			}
			break;
		case BM_BURN:
			// TODO.
			break;
		case BM_MULTIPLY:
			LOOP()
			{
				out[i] = src[i] * dst[i];
			}
			break;
		case BM_DIFFERENCE:
			LOOP()
			{
				if (src[i] > dst[i])
					out[i] = src[i] - dst[i];
				else
					out[i] = dst[i] - src[i];
			}
			break;
		case BM_SCREEN:
			LOOP()
			{
				out[i] = 1.0f - (1.0f - src[i]) * (1.0f - dst[i]);
			}
			break;
		case BM_OVERLAY:
			// TODO.
			break;
		case BM_COLORIZE:
			{
#if 0
				float h1, s1, v1;
				float h2, s2, v2;

				// TODO: Optimize speed. Use own method. :)
				rgb_to_hsv(FTOI(src[0]), FTOI(src[1]), FTOI(src[2]), &h1, &s1, &v1);
				rgb_to_hsv(FTOI(dst[0]), FTOI(dst[1]), FTOI(dst[2]), &h2, &s2, &v2);

				h2 = h1;
				s2 = s1;

				int r, g, b;

				hsv_to_rgb(h2, s2, v2, &r, &g, &b);

				out[0] = ITOF(r);
				out[1] = ITOF(g);
				out[2] = ITOF(b);
#else
				if (channels == 3)
				{
					float v1 = (dst[0] + dst[1] + dst[2]) / 3.0f;
					float v2 = (src[0] + src[1] + src[2]) / 3.0f;
					float v = v1 / v2;

					out[0] = src[0] * v;
					out[1] = src[1] * v;
					out[2] = src[2] * v;
				}
#endif
			}
			break;
		default:
			LOG_ERR("unknown blend mode", 0);
			Assert(false);
			break;
		}

		const float opacity_1 = 1.0f - opacity;

		LOOP()
		{
			out[i] =
				dst[i] * opacity_1 +
				out[i] * opacity;

			if (1)
				out[i] = Saturate(out[i]);
		}
	}

	void Clear(sample_t* color)
	{
		int sample_count = w * h;
		sample_t* sample = samples;

		for (int i = 0; i < sample_count; ++i)
			for (int j = 0; j < channels; ++j)
				*sample++ = color[j];

		Invalidate();
	}

	sample_t* samples;
	int w, h;
	int channels;

	BLEND_MODE blend_mode;

	BITMAP* bmp;

	RectSet invalidated_rectset;

	virtual void Invalidate(const Rect& rect);
	void Invalidate();

	typedef void (*handle_update_cb)(void* up, Canvas* canvas, const Rect& rect);

	void SetUpdateCB(handle_update_cb handle_update, void* up)
	{
		this->handle_update = handle_update;
		this->handle_update_up = up;
	}

	bool Validate();
	void ValidateRect(const Rect& rect);

private:
	handle_update_cb handle_update;
	void* handle_update_up;
};
