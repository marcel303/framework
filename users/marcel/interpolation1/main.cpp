#include "framework.h"
#include <functional>

void showInterpolation(
	const float * values, const int num_values,
	const std::function<float(const float * values, const int num_values, const float pos)> & sample_function)
{
	pushLineSmooth(true);
	gxBegin(GX_LINE_STRIP);
	{
		for (int i = 0; i < 800; ++i)
		{
			const float pos = i / 800.f * (num_values - 1);
			
			const float value = sample_function(values, num_values, pos);

			gxVertex2f(i, value);
		}
	}
	gxEnd();
	popLineSmooth();
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 600))
		return -1;

	auto interp_linear = [](const float * values, const int num_values, const float pos)
	{
		const float i1f = floorf(pos);
		const int i1 = (int)i1f;
		const int i2 = i1 + 1;
		const float t = pos - i1f;
		
		const float v1 = values[i1];
		const float v2 = values[i2];
		
		//
		
		return v1 * (1.f - t) + v2 * t;
	};
	
	auto interp_cosine = [](const float * values, const int num_values, const float pos)
	{
		const float i1f = floorf(pos);
		const int i1 = (int)i1f;
		const int i2 = i1 + 1;
		const float t = pos - i1f;
		
		const float v1 = values[i1];
		const float v2 = values[i2];
		
		//
		
		const float c = (1.f - cosf(t * float(M_PI))) / 2.f;
		
		return v1 * (1.f - c) + v2 * c;
	};
	
	auto interp_cubic = [](const float * values, const int num_values, const float pos)
	{
		// classic cubic
		const float i2f = floorf(pos);
		const int i1 = (int)i2f;
		const int i0 = i1 - 1 >= 0 ? i1 - 1 : 0;
		const int i2 = i1 + 1 <= num_values - 1 ? i1 + 1 : num_values - 1;
		const int i3 = i1 + 2 <= num_values - 1 ? i1 + 2 : num_values - 1;
		const float t = pos - i2f;
		
		const float value0 = values[i0];
		const float value1 = values[i1];
		const float value2 = values[i2];
		const float value3 = values[i3];
		
		//
		
		const float t2 = t * t;
		
		const float a0 = value3 - value2 - value0 + value1;
		const float a1 = value0 - value1 - a0;
		const float a2 = value2 - value0;
		const float a3 = value1;

		const float value =
			a0 * t * t2 +
			a1 * t2 +
			a2 * t +
			a3;
		
		return value;
	};
	
	auto interp_cubic_catmullRom = [](const float * values, const int num_values, const float pos)
	{
		// catmull-rom cubic
		const float i2f = floorf(pos);
		const int i1 = (int)i2f;
		const int i0 = i1 - 1 >= 0 ? i1 - 1 : 0;
		const int i2 = i1 + 1 <= num_values - 1 ? i1 + 1 : num_values - 1;
		const int i3 = i1 + 2 <= num_values - 1 ? i1 + 2 : num_values - 1;
		const float t = pos - i2f;
		
		const float value0 = values[i0];
		const float value1 = values[i1];
		const float value2 = values[i2];
		const float value3 = values[i3];
		
		//
		
		const float t2 = t * t;
		
		const float a0 = -0.5f * value0 + 1.5f * value1 - 1.5f * value2 + 0.5f * value3;
		const float a1 = value0 - 2.5f * value1 + 2.0f * value2 - 0.5f * value3;
		const float a2 = -0.5f * value0 + 0.5f * value2;
		const float a3 = value1;

		const float value =
			a0 * t * t2 +
			a1 * t2 +
			a2 * t +
			a3;
		
		return value;
	};
	
	auto interp_cubic_hermite_base = [](
		const float value0,
		const float value1,
		const float value2,
		const float value3,
		const float mu,
		const float tension, const float bias)
	{
		const float mu2 = mu * mu;
		const float mu3 = mu2 * mu;
		
		const float m0 =
			(value1-value0) * (1+bias) * (1-tension)/2 +
			(value2-value1) * (1-bias) * (1-tension)/2;
		const float m1 =
			(value2-value1) * (1+bias) * (1-tension)/2 +
			(value3-value2) * (1-bias) * (1-tension)/2;
		
		const float a0 =  2 * mu3 - 3 * mu2 + 1;
		const float a1 =      mu3 - 2 * mu2 + mu;
		const float a2 =      mu3 -     mu2;
		const float a3 = -2 * mu3 + 3 * mu2;

		return
			a0 * value1 +
			a1 * m0 +
			a2 * m1 +
			a3 * value2;
	};

	auto interp_cubic_hermite = [&](const float * values, const int num_values, const float pos)
	{
		const float i2f = floorf(pos);
		const int i1 = (int)i2f;
		const int i0 = i1 - 1 >= 0 ? i1 - 1 : 0;
		const int i2 = i1 + 1 <= num_values - 1 ? i1 + 1 : num_values - 1;
		const int i3 = i1 + 2 <= num_values - 1 ? i1 + 2 : num_values - 1;
		
		const float t = pos - i2f;
		
		const float value0 = values[i0];
		const float value1 = values[i1];
		const float value2 = values[i2];
		const float value3 = values[i3];
		
		//
		
		const float tension = sinf(framework.time);
		const float bias = 0.f;
		
		return interp_cubic_hermite_base(
			value0,
			value1,
			value2,
			value3,
			t,
			tension, bias);
	};
	
	auto interp_gap = [&](const float * values, const int num_values, const float pos)
	{
		const float i1f = floorf(pos);
		const int i1 = (int)i1f;
		const int i2 = i1 + 1;
		
		float t = pos - i1f;
		
		if (t < .5f)
		{
			t = 4.f * t * t * t;
		}
		else
		{
			t = 1.f - t;
			t = 1.f - 4.f * t * t * t;
		}
		
		const float v1 = values[i1];
		const float v2 = values[i2];
		
		//
		
		return v1 * (1.f - t) + v2 * t;
	};
	
	const int num_values = 10;
	float values[num_values];
	values[0] = 300.f;
	values[1] = 200.f;
	values[2] = 400.f;
	values[3] = 300.f;
	for (int i = 4; i < num_values; ++i)
		values[i] = random<float>(100.f, 500.f);

	std::function<float(const float * values, const int num_values, const float pos)> function = interp_linear;
	
	int selected_value = -1;
	
	const float circle_radius = 6.f;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;

		if (keyboard.wentDown(SDLK_1))
			function = interp_linear;
		if (keyboard.wentDown(SDLK_2))
			function = interp_cosine;
		if (keyboard.wentDown(SDLK_3))
			function = interp_cubic;
		if (keyboard.wentDown(SDLK_4))
			function = interp_cubic_catmullRom;
		if (keyboard.wentDown(SDLK_5))
			function = interp_cubic_hermite;
		if (keyboard.wentDown(SDLK_6))
			function = interp_gap;
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			for (int i = 0; i < num_values; ++i)
			{
				const float x = i * 800 / (num_values - 1);
				const float y = values[i];
				
				const float dx = mouse.x - x;
				const float dy = mouse.y - y;
				
				if (hypotf(dx, dy) <= circle_radius)
					selected_value = i;
			}
		}
		
		if (selected_value != -1)
		{
			if (mouse.wentUp(BUTTON_LEFT))
				selected_value = -1;
			else
			{
				values[selected_value] = mouse.y;
			}
		}
		
		framework.beginDraw(200, 200, 200, 255);
		{
			setColor(colorBlack);
			showInterpolation(values, num_values, function);
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int i = 0; i < num_values; ++i)
				{
					setColor(i == selected_value ? colorWhite : colorRed);
					hqFillCircle(i * 800 / (num_values - 1), values[i], circle_radius);
				}
			}
			hqEnd();
		}
		framework.endDraw();
	}

	framework.shutdown();

	return 0;
}
