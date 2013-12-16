#define ALLEGRO_NO_MAGIC_MAIN
#include <allegro.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <math.h>

#include <stdio.h>
#include <windows.h>

float Skew(float v)
{
	return (sinf(v) + 1.2f) / 2.2f;
}

static float Trans(float v)
{
	return sin(v / 5.678f) * 6.0f;
}

static int Abs(int v)
{
	return v >= 0 ? v : -v;
}

static int Value(int index, int repeat)
{
	return Abs((index % repeat) - repeat / 2);
}

static void roto(BITMAP* buffer, float angle, int frame, unsigned int amount)
{
	MATRIX_f transform_angle;
	get_rotation_matrix_f(&transform_angle, 0.0f, 0.0f, angle);

	float skewX = Skew(angle / 20.0f / 2.34567f);
	float skewY = Skew(angle / 20.0f / 1.12345f);
	MATRIX_f transform_skew;
	get_scaling_matrix_f(&transform_skew, skewX, skewY, 1.0f);

	float transX = Trans(angle / 13.0f);
	float transY = Trans(angle / 17.0f);
	MATRIX_f transform_trans;
	get_translation_matrix_f(&transform_trans, transX, transY, 0.0f);

	MATRIX_f temp;
	matrix_mul_f(&transform_angle, &transform_skew, &temp);
	matrix_mul_f(&temp, &transform_trans, &temp);

	MATRIX_f transform;
	//matrix_mul_f(&transform_angle, &transform_skew, &transform);
	//transform = transform_angle;
	transform = temp;

	float scale = 40.0f;

#if 0
	unsigned int r = 127;
	unsigned int g = 255;
	unsigned int b = 255;
#elif 0
	unsigned int r = 11;
	unsigned int g = 13;
	unsigned int b = 17;
#else
	unsigned int r = Value(frame, 31);
	unsigned int g = Value(frame, 43);
	unsigned int b = Value(frame, 51);
#endif

	unsigned int c = (r << 0) | (g << 8) | (b << 16);

	unsigned int amount_r = amount;
	unsigned int amount_g = amount >> 1;
	unsigned int amount_b = amount >> 2;

	__m128i amountVec;

	for (int i = 0; i < 4; ++i)
	{
		amountVec.m128i_u8[i*4+0] = amount_r;
		amountVec.m128i_u8[i*4+1] = amount_g;
		amountVec.m128i_u8[i*4+2] = amount_b;
		amountVec.m128i_u8[i*4+3] = 0;
	}

#pragma omp parallel for
	for (int y = 0; y < buffer->h; ++y)
	{
		float fy = (y + 0.5f - buffer->h/2.0f) / scale;
		float fx1 = (0.5f - buffer->w/2.0f) / scale;
		float fx2 = (buffer->w - 0.5f - buffer->w/2.0f) / scale;

		float u1f;
		float v1f;
		float u2f;
		float v2f;
		float temp;

		apply_matrix_f(&transform, fx1, fy, 0.0f, &u1f, &v1f, &temp);
		apply_matrix_f(&transform, fx2, fy, 0.0f, &u2f, &v2f, &temp);

		fixed u1 = ftofix(u1f);
		fixed v1 = ftofix(v1f);
		fixed u2 = ftofix(u2f);
		fixed v2 = ftofix(v2f);

		fixed du = u2 - u1;
		fixed dv = v2 - v1;
		fixed dx = buffer->w;
		fixed du_dx = du / dx;
		fixed dv_dx = dv / dx;

		fixed u = u1;
		fixed v = v1;

		__m128i * __restrict line128 = reinterpret_cast<__m128i*>(buffer->line[y]);

		// non-sse: 190 ms / 100 calls

		__m128i vdu_dx;
		__m128i vdv_dx;
		__m128i vu;
		__m128i vv;

		vdu_dx = _mm_set1_epi32(du_dx * 4);
		vdv_dx = _mm_set1_epi32(dv_dx * 4);

		for (int i = 0; i < 4; ++i)
		{
			vu.m128i_i32[i] = u + du_dx * i;
			vv.m128i_i32[i] = v + dv_dx * i;
		}

		for (int x = buffer->w >> 3; x != 0; --x)
		{
			__m128i bittest1 = _mm_set1_epi32(1 << 16);
			__m128i bittest2 = _mm_set1_epi32(1 << 12);
			__m128i c128 = _mm_set1_epi32(c);

			{
				__m128i filled1 = _mm_cmpeq_epi32(_mm_and_si128(vu, bittest1), bittest1);
				__m128i filled2 = _mm_cmpeq_epi32(_mm_and_si128(vv, bittest2), bittest2);
				__m128i mask = _mm_andnot_si128(filled1, filled2);
				__m128i v;
				v = _mm_loadu_si128(line128);
				//v = _mm_blendv_epi8(v, c128, mask);
				v = _mm_adds_epu8(v, _mm_and_si128(c128, mask));
				v = _mm_subs_epu8(v, amountVec);
				_mm_storeu_si128(line128, v);
				line128 += 1;
				vu = _mm_add_epi32(vu, vdu_dx);
				vv = _mm_add_epi32(vv, vdv_dx);
			}

			{
				__m128i filled1 = _mm_cmpeq_epi32(_mm_and_si128(vu, bittest1), bittest1);
				__m128i filled2 = _mm_cmpeq_epi32(_mm_and_si128(vv, bittest2), bittest2);
				__m128i mask = _mm_andnot_si128(filled1, filled2);
				__m128i v;
				v = _mm_loadu_si128(line128);
				//v = _mm_blendv_epi8(v, c128, mask);
				v = _mm_adds_epu8(v, _mm_and_si128(c128, mask));
				v = _mm_subs_epu8(v, amountVec);
				_mm_storeu_si128(line128, v);
				line128 += 1;
				vu = _mm_add_epi32(vu, vdu_dx);
				vv = _mm_add_epi32(vv, vdv_dx);
			}
		}
	}
}

int main(int argc, const char* argv[])
{
#if 1
	unsigned int sx = 1024;
	unsigned int sy = 768;
#else
	unsigned int sx = 1920;
	unsigned int sy = 1080;
#endif

	allegro_init();
	set_color_depth(32);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, sx, sy, 0, 0);
	//set_gfx_mode(GFX_AUTODETECT_FULLSCREEN, sx, sy, 0, 0);
	install_keyboard();

	BITMAP* buffer = create_bitmap_ex(32, sx, sy);
	clear_to_color(buffer, makecol(0,0,0));

	float t = 0.0f;

	float angle = 0.0f;

	int frame = 0;

	while (!key[KEY_ESC])
	{
		//float dt = 1.0f / 30.0f;
		float dt = 1.0f / 60.0f;
		//float dt = 1.0f / 200.0f;
		//float angleSpeed = (sin(t * 4.0f) + 2.0f) * 30.0f;
		//float angleSpeed = (sin(t * 1.0f) + 0.5f) * 60.0f;
		float angleSpeed = (sin(t * 1.0f) + 1.2f) * 30.0f;
		angle += angleSpeed * dt;
		t += dt;

		acquire_bitmap(buffer);

		int t0 = timeGetTime();

		int t1 = timeGetTime();

		//for (int i = 0; i < 1000; ++i)
		roto(buffer, angle, frame, (frame / 20) % 40);

		int t2 = timeGetTime();

		int t01 = t1 - t0;
		int t12 = t2 - t1;
		printf("time 01: %d ms, time 12: %d ms\n", t01, t12);

		release_bitmap(buffer);

		vsync();
		blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);

		++frame;
	}

	destroy_bitmap(buffer);
	buffer = 0;

	allegro_exit();

	return 0;
}
