#include "Calc.h"
#include "Gauge.h"

namespace Game
{
	Gauge::Gauge()
	{
		Initialize();
	}
	
	void Gauge::Initialize()
	{
		m_Min = 0.0f;
		m_Max = 0.0f;
		m_Value = 0.0f;
	}
	
	void Gauge::Setup(float min, float max, float value)
	{
		m_Min = min;
		m_Max = max;
		m_Value = value;
	}
	
	void Gauge::Increase(float amount)
	{
		m_Value += amount;
		
		Clamp();
	}
	
	void Gauge::Decrease(float amount)
	{
		m_Value -= amount;
		
		Clamp();
	}
	
	float Gauge::Max_get() const
	{
		return m_Max;
	}
	
	void Gauge::Max_set(float max)
	{
		m_Max = max;
		
		Clamp();
	}
	
	float Gauge::Value_get() const
	{
		return m_Value;
	}
	
	void Gauge::Value_set(float value)
	{
		m_Value = value;
		
		Clamp();
	}
	
	bool Gauge::IsEmpty_get() const
	{
		return m_Value == m_Min;
	}
	
	bool Gauge::IsFull_get() const
	{
		return m_Value == m_Max;
	}
	
	float Gauge::Usage_get() const
	{
		return ToUsage(m_Value);
	}
	
	float Gauge::ToUsage(float amount) const
	{
		return (amount - m_Min)  / (m_Max - m_Min);
	}
	
	void Gauge::Clamp()
	{
		m_Value = Calc::Clamp(m_Value, m_Min, m_Max);
	}
};
