#include "framework.h"
#include "Noise.h"
#include "thermalizer.h"

const double kCelcius = 273.0;

Thermalizer::Thermalizer()
	: thermalToView(true)
{
}

Thermalizer::~Thermalizer()
{
	shut();
}

void Thermalizer::init(const int _size)
{
	shut();
	
	//
	
	if (_size > 0)
	{
		size = _size;
		
		thermalToView = Mat4x4(true)
			.Scale(3, 6, 1)
			.Translate(0, -kCelcius, 0)
			.Translate(-size/2.f, 0, 0);
		
		heat = new double[size];
		bang = new double[size];
		
		for (int i = 0; i < size; ++i)
		{
			heat[i] = kCelcius + 12.0;
			bang[i] = 0.0;
		}
	}
}

void Thermalizer::shut()
{
	delete [] heat;
	heat = nullptr;
	
	delete [] bang;
	bang = nullptr;
	
	size = 0;
}

void Thermalizer::applyHeat(const int index, const double _heat, const double dt)
{
	if (index < 0 || index >= size)
		return;
	
	const double kRetainPerSecond = 0.7;
	
	const double retain = std::pow(kRetainPerSecond, dt);
	
	heat[index] = heat[index] * retain + _heat * (1.0 - retain);
}

double Thermalizer::sampleHeat(const int index) const
{
	if (index < 0)
		return heat[0];
	else if (index > size - 1)
		return heat[size -1];
	else
		return heat[index];
}

void Thermalizer::diffuseHeat(const double dt)
{
	double * oldHeat = (double*)alloca(size * sizeof(double));
	memcpy(oldHeat, heat, size * sizeof(double));
	
	const double kRetainPerMS = 0.8;
	
	const double retain = std::pow(kRetainPerMS, dt * 1000.0);
	const double spread = (1.0 - retain) * 0.5;
	
	for (int i = 0; i < size; ++i)
	{
		const double value = spread * oldHeat[i];
		
		if (i > 0)
		{
			heat[i - 1] += value;
			heat[i] -= value;
		}
		
		if (i < size - 1)
		{
			heat[i + 1] += value;
			heat[i] -= value;
		}
	}
}

void Thermalizer::applyHeatAtViewPos(const float x, const float y, const float dt)
{
	const Mat4x4 viewToThermal = thermalToView.CalcInv();
	const Vec2 p = viewToThermal.Mul4(Vec2(x, y));

	const int i = int(std::round(p[0]));
	
	if (i >= 0 && i <= size - 1)
	{
		const float noiseX = i;
		const float noiseY = framework.time * 1.f;
		
		const float noise = scaled_octave_noise_2d(8, .5f, 1.f, 0.f, kCelcius + 12.0, noiseX, noiseY);
		
		applyHeat(i, noise, dt);
	}
}

void Thermalizer::tick(const float dt)
{
	double * oldHeat = (double*)alloca(size * sizeof(double));
	memcpy(oldHeat, heat, size * sizeof(double));
	
	const bool doApplyHeat = mouse.isDown(BUTTON_LEFT);
	
	// apply heat
	
	if (doApplyHeat)
	{
		applyHeatAtViewPos(mouse.x, mouse.y, dt);
		
		for (int i = 0; i < 2; ++i)
		{
			const float noiseX = i;
			const float noiseY = framework.time * .2f;
			
			const float noise = scaled_octave_noise_2d(8, .5f, 1.f, kCelcius + 12.0, kCelcius + 72.0, noiseX, noiseY);
			
			applyHeat(i, noise, dt);
		}
		
		for (int i = size - 2; i < size; ++i)
		{
			const float noiseX = i;
			const float noiseY = framework.time * .3f;
			
			const float noise = scaled_octave_noise_2d(8, .5f, 1.f, kCelcius + 12.0, kCelcius + 72.0, noiseX, noiseY);
			
			applyHeat(i, noise, dt);
		}
	}
	
	// diffuse
	
#if 0
	diffuseHeat(dt);
#else
	for (int i = 0; i < 10; ++i)
		diffuseHeat(dt / 10.0);
#endif

	const double decayPerSecond = 0.5;
	const double decay = std::pow(decayPerSecond, dt);

	for (int i = 0; i < size; ++i)
	{
		bang[i] *= decay;
		
		const double heatOld = oldHeat[i];
		const double heatNew = heat[i];
		
		const int shift1 = int(std::floor(heatOld / 1.0));
		const int shift2 = int(std::floor(heatNew / 1.0));
		
		if (shift1 != shift2)
			bang[i] = 1.0;
	}
}

void Thermalizer::draw2d() const
{
	gxPushMatrix();
	{
		gxMultMatrixf(thermalToView.m_v);
		
		const float spacing = .03f;
		
		hqBegin(HQ_FILLED_ROUNDED_RECTS);
		{
			for (int i = 0; i < size; ++i)
			{
				setLumif(std::abs(heat[i]));
				hqFillRoundedRect(i + spacing, kCelcius, i + 1 - spacing * 2, heat[i], .25f);
			}
		}
		hqEnd();
		
		setColor(255, 127, 63);
		pushBlend(BLEND_ALPHA);
		hqBegin(HQ_FILLED_ROUNDED_RECTS);
		{
			for (int i = 0; i < size; ++i)
			{
				setAlphaf(bang[i]);
				hqFillRoundedRect(i + spacing, kCelcius, i + 1 - spacing * 2, heat[i], .25f);
			}
		}
		hqEnd();
		popBlend();
	}
	gxPopMatrix();
}
