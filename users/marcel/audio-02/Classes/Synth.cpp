#include "Synth.h"

#include <cmath>
#include <vector>

//

SynthSourceBase::SynthSourceBase(SynthSourceType type, double frequency)
{
	Initialize(type, frequency);
}

void SynthSourceBase::Initialize(SynthSourceType type, double frequency)
{
	m_Type = type;
	m_Frequency = frequency;
	m_PhaseNorm = 0.0;
}

void SynthSourceBase::Generate(double* buffer, int count, double dt)
{
	double phaseStep = m_Frequency * dt;
			
#define GEN_BEGIN \
	for (int i = 0; i < count; ++i) \
	{ \
		double v;
				
	#define GEN_END \
		buffer[i] = v; \
		\
		m_PhaseNorm += phaseStep; \
		\
		if (m_PhaseNorm > 1.0) \
			m_PhaseNorm -= 1.0; \
	}
			
	switch (m_Type)
	{
		case SynthSourceType_Triangle:
			GEN_BEGIN
			if (m_PhaseNorm < 0.5)
				v = -1.0 + m_PhaseNorm * 4.0;
			else
				v = +1.0 - (m_PhaseNorm - 0.5) * 4.0;
			GEN_END
			break;
			
		case SynthSourceType_Square:
			GEN_BEGIN
			if (m_PhaseNorm < 0.5)
				v = 0.0;
			else
				v = 1.0;
			GEN_END
			break;
			
		case SynthSourceType_Sine:
			GEN_BEGIN
			v = sin(m_PhaseNorm * 2.0 * M_PI);
			GEN_END
			break;
			
		case SynthSourceType_Saw:
			GEN_BEGIN
			v = m_PhaseNorm;
			GEN_END
			break;
	}
}

//

SynthEnvNote::SynthEnvNote()
{
	Initialize();
}

void SynthEnvNote::Initialize()
{
	Reset();
}

void SynthEnvNote::Reset()
{
	m_Stage = 0;
	m_Time = 0.0;
	m_IsDone = false;
}

void SynthEnvNote::Update(double dt)
{
	m_Time += dt;
	
	const EnvValue* envValue = GetEnvValue();
	
	int classification = envValue->Classify(m_Time);
	
	if (classification == +1)
	{
		m_Stage = m_Stage + 1;
		
		if (m_Stage > 3)
		{
			m_Stage = 3;
			m_IsDone = true;
		}
		else
		{
			m_Time = 0.0;
		}
	}
}

//

SynthSource_Note::SynthSource_Note()
{
	Initialize();
}

void SynthSource_Note::Initialize()
{
	m_Enabled = false;
}

void SynthSource_Note::Generate(double* buffer, int count, double dt)
{
	if (m_Enabled && !m_Note.m_IsDone)
	{
		m_Source->Generate(buffer, count, dt);
		
		for (int i = 0; i < count; ++i)
		{
			double v = m_Note.GetValue();
			
			buffer[i] *= v;
			
			m_Note.Update(dt);
		}
	}
	else
	{
		for (int i = 0; i < count; ++i)
		{
			buffer[i] = 0.0;
		}
	}
}

void SynthSource_Note::Play()
{
	m_Note.Reset();
	m_Enabled = true;
}

//

SynthSource_Mix::SynthSource_Mix()
{
}

void SynthSource_Mix::Generate(double* buffer, int count, double dt)
{
	for (int i = 0; i < count; ++i)
	{
		buffer[i] = 0;
	}
	
	double* temp = new double[count];
	
	for (int i = 0; i < m_Sources.size(); ++i)
	{
		m_Sources[i]->Generate(temp, count, dt);
		
		for (int j = 0; j < count; ++j)
		{
			buffer[j] += temp[j];
		}
	}
	
	delete[] temp;
}

//

Synth::Synth()
{
	Initialize();
}

void Synth::Initialize()
{
	m_Mix = new SynthSource_Mix();
}
