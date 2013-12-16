#include "FireController.h"

namespace Game
{
	FireController::FireController()
	{
//		Initialize();
	}
	
	void FireController::Initialize(ITimer* timer)
	{
		m_Arg = 0;
		m_State = FireControllerState_Idle;
		m_FireCondition = XFALSE;
		m_FireTimer.FireImmediately_set(XTRUE);
		m_Ammo = 0;
		m_MaxAmmo = 0;
		m_Mode = FireControllerMode_FireUntillEmpty;
		
		m_FireTimer.Initialize(timer);
		m_RechargeTimer.Initialize(timer);
	}
	
	void FireController::Setup(int ammo, int maxAmmo, float fireFrequency, float rechargeInterval, FireControllerMode mode, CallBack HandleFire, void* arg)
	{
		m_Ammo = ammo;
		m_MaxAmmo = maxAmmo;
		m_FireTimer.SetFrequency(fireFrequency);
		m_RechargeTimer.SetInterval(rechargeInterval / m_MaxAmmo);
		m_Mode = mode;
		m_OnFire = HandleFire;
		m_Arg = arg;
	}
	
	void FireController::Update()
	{
		switch (m_State)
		{
			case FireControllerState_Idle:
				if (m_FireCondition)
				{
					BeginFire();
				}
				break;
				
			case FireControllerState_Firing:
				if (m_Mode == FireControllerMode_FireUntillConditionChange && !m_FireCondition)
				{
					EndFire();
				}
				else
				{
					while (m_FireTimer.ReadTick())
					{
						if (m_OnFire.IsSet())
							m_OnFire.Invoke(m_Arg);
						
						m_Ammo--;
						
						if (m_Ammo == 0)
						{
							EndFire();
							
							break;
						}
					}
				}
				break;
				
			case FireControllerState_Rechargin:
				{
					while (m_RechargeTimer.ReadTick())
					{
						if (m_Ammo < m_MaxAmmo)
							m_Ammo++;
						
						if (m_Ammo == m_MaxAmmo)
						{
							EndRecharge();
							
							break;
						}
					}
				}
				break;
		}
	}
	
	void FireController::FireCondition_set(XBOOL value)
	{
		m_FireCondition = value;
	}
	
	void FireController::BeginFire()
	{
		m_State = FireControllerState_Firing;
		
		m_FireTimer.Start();
	}
	
	void FireController::EndFire()
	{
		m_FireTimer.Stop();
		
		m_State = FireControllerState_Idle;
		
		BeginRecharge();
	}
	
	void FireController::BeginRecharge()
	{
		m_State = FireControllerState_Rechargin;
		
		m_RechargeTimer.Start();
	}
	
	void FireController::EndRecharge()
	{
		m_RechargeTimer.Stop();
		
		m_State = FireControllerState_Idle;
	}
};
