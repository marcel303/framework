#include "PowerSink.h"

namespace Game
{
	PowerSink_PumpAction::PowerSink_PumpAction()
	{
		Initialize();
	}
	
	void PowerSink_PumpAction::Initialize()
	{
		m_Source = 0;
		m_Target = 0;
		m_AmountPerSecond = 0.0f;
	}
	
	void PowerSink_PumpAction::Setup(PowerSink* source, PowerSink* target, float amountPerSecond)
	{
		m_Source = source;
		m_Target = target;
		m_AmountPerSecond = amountPerSecond;
	}
	
	void PowerSink_PumpAction::Commit(float dt)
	{
		float amount = m_Source->Take(m_AmountPerSecond * dt);
		
		m_Target->Dump(amount);
		
		//
		
		m_Source = 0;
		m_Target = 0;
		m_AmountPerSecond = 0.0f;
	}
	
	bool PowerSink_PumpAction::IsAllocated_get() const
	{
		return m_Source != 0;
	}
	
	//
	
	PowerSink::PowerSink()
	{
		Initialize();
	}
	
	void PowerSink::Initialize()
	{
	}
	
	void PowerSink::Setup(float max, float value)
	{
		m_PowerGauge.Setup(0.0f, max, value);
	}
	
	void PowerSink::Dump(float amount)
	{
		m_PowerGauge.Increase(amount);
	}
	
	float PowerSink::Take(float amount)
	{
		if (amount >= m_PowerGauge.Value_get())
		{
			amount = m_PowerGauge.Value_get();
			
			m_PowerGauge.Value_set(0.0f);
		}
		else
		{
			m_PowerGauge.Decrease(amount);
		}
		
		return amount;
	}
	
	float PowerSink::TakeAll()
	{
		return Take(m_PowerGauge.Value_get());
	}
	
	void PowerSink::SchedulePump(float amountPerSecond, PowerSink* target)
	{
		PowerSink_PumpAction* action = AllocatePumpAction();
		
		if (!action)
			return;
		
		action->Setup(this, target, amountPerSecond);
	}
	
	void PowerSink::Commit(float dt)
	{
		for (int i = 0; i < 10; ++i)
		{
			if (!m_PumpActions[i].IsAllocated_get())
				continue;
			
			m_PumpActions[i].Commit(dt);
		}
	}
	
	bool PowerSink::HasAmount(float amount) const
	{
		return m_PowerGauge.Value_get() >= amount;
	}
	
	float PowerSink::Amount_get() const
	{
		return m_PowerGauge.Value_get();
	}
	
	void PowerSink::Max_set(float max)
	{
		m_PowerGauge.Max_set(max);
	}
	
	float PowerSink::Usage_get() const
	{
		return m_PowerGauge.Usage_get();
	}
	
	float PowerSink::ToUsage(float amount)
	{
		return m_PowerGauge.ToUsage(amount);
	}
	
	PowerSink_PumpAction* PowerSink::AllocatePumpAction()
	{
		for (int i = 0; i < 10; ++i)
			if (!m_PumpActions[i].IsAllocated_get())
				return &m_PumpActions[i];
		
		return 0;
	}
};
