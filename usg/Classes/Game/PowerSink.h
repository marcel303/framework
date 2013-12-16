#pragma once

#include "Gauge.h"

namespace Game
{
	class PowerSink;
	
	// note: such a lame as class name :s
	class PowerSink_PumpAction
	{
	public:
		PowerSink_PumpAction();
		void Initialize();
		
		void Setup(PowerSink* source, PowerSink* target, float amountPerSecond);
		void Commit(float dt);
		
		bool IsAllocated_get() const;
		
	private:
		PowerSink* m_Source;
		PowerSink* m_Target;
		float m_AmountPerSecond;
	};
	
	class PowerSink
	{
	public:
		PowerSink();
		void Initialize();
		
		void Setup(float max, float value);
		
		void Dump(float amount);
		float Take(float amount);
		float TakeAll();
		
		void SchedulePump(float amountPerSecond, PowerSink* target);
		void Commit(float dt);
		
		bool HasAmount(float amount) const;
		
		float Amount_get() const;
		
		void Max_set(float max);
		
		float Usage_get() const;
		
		float ToUsage(float amount);
		
	private:
		PowerSink_PumpAction* AllocatePumpAction();
		
		Gauge m_PowerGauge;
		PowerSink_PumpAction m_PumpActions[10];
	};
}
