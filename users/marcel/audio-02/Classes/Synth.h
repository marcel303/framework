#pragma once

#include <cmath>
#include <vector>

enum SynthSourceType
{
	SynthSourceType_Triangle,
	SynthSourceType_Square,
	SynthSourceType_Sine,
	SynthSourceType_Saw
};

class SynthSource
{
public:
	virtual void Generate(double* buffer, int count, double dt) = 0;
};

class SynthSourceBase : public SynthSource
{
public:
	SynthSourceBase(SynthSourceType type, double frequency);	
	void Initialize(SynthSourceType type, double frequency);	
	
	virtual void Generate(double* buffer, int count, double dt);	
	
	SynthSourceType m_Type;
	double m_Frequency;
	double m_PhaseNorm;
};

class EnvValue
{
public:
	inline void Set(double time, double value1, double value2)
	{
		m_Time = time;
		m_TimeInv = 1.0 / time;
		m_Value1 = value1;
		m_Value2 = value2;
	}
	
	inline int Classify(double time) const
	{
//		if (time < 0.0)
//			return -1;
		if (time > m_Time)
			return +1;
		
		return 0;
	}
	
	inline double GetValue(double time) const
	{
		double v = time * m_TimeInv;
		
		if (v < 0.0)
			return m_Value1;
		if (v > 1.0)
			return m_Value2;
		
		return m_Value1 + (m_Value2 - m_Value1) * v;
	}
	
	double m_Time;
	double m_TimeInv;
	double m_Value1;
	double m_Value2;
};

class SynthEnv
{
public:
	union
	{
		struct
		{
			EnvValue m_Attack;
			EnvValue m_Decay;
			EnvValue m_Sustain;
			EnvValue m_Release;
		};
		EnvValue m_Stages[4];
	};
};

class SynthEnvNote
{
public:
	SynthEnvNote();	
	void Initialize();
	
	void Reset();
	
	inline const EnvValue* GetEnvValue() const
	{
		return &m_Env.m_Stages[m_Stage];
	}
	
	inline double GetValue() const
	{
		const EnvValue* envValue = GetEnvValue();
		
		return envValue->GetValue(m_Time);
	}
	
	void Update(double dt);
	
	SynthEnv m_Env;
	int m_Stage;
	double m_Time;
	bool m_IsDone;
};

class SynthSource_Note : public SynthSource
{
public:
	SynthSource_Note();	
	void Initialize();
	
	virtual void Generate(double* buffer, int count, double dt);
	
	void Play();
	
	SynthSource* m_Source;
	SynthEnvNote m_Note;
	bool m_Enabled;
};

class SynthSource_Mix : public SynthSource
{
public:
	SynthSource_Mix();
	
	virtual void Generate(double* buffer, int count, double dt);
	
	std::vector<SynthSource*> m_Sources;
};

class Synth
{
public:
	Synth();
	
	void Initialize();
	
	SynthSource_Mix* m_Mix;
};