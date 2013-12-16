#pragma once

namespace Game
{
	class Gauge
	{
	public:
		Gauge();
		void Initialize();
		
		void Setup(float min, float max, float value);
		
		void Increase(float amount);
		void Decrease(float amount);
		
		float Max_get() const;
		void Max_set(float max);
		
		float Value_get() const;
		void Value_set(float value);
		
		bool IsEmpty_get() const;
		bool IsFull_get() const;
		
		float Usage_get() const;
		
		float ToUsage(float amount) const;
		
	private:
		void Clamp();
		
		float m_Min;
		float m_Max;
		float m_Value;
	};
}
