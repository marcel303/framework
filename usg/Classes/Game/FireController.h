#pragma once

#include "CallBack.h"
#include "PolledTimer.h"
#include "Timer.h"

namespace Game
{
	enum FireControllerState
	{
		FireControllerState_Idle,
		FireControllerState_Firing,
		FireControllerState_Rechargin
	};
	
	enum FireControllerMode
	{
		FireControllerMode_FireUntillConditionChange,
		FireControllerMode_FireUntillEmpty
	};
	
	class FireController
	{
	public:
		FireController();
		void Initialize(ITimer* timer);
		
		void Setup(int ammo, int maxAmmo, float fireFrequency, float rechargeInterval, FireControllerMode mode, CallBack HandleFire, void* arg);
		
		void Update();
		
		void FireCondition_set(XBOOL value);
		
		void BeginFire();
		void EndFire();
		void BeginRecharge();
		void EndRecharge();
		
	private:
		CallBack m_OnFire;
		void* m_Arg;
		
		FireControllerState m_State;
		XBOOL m_FireCondition;
		int m_Ammo;
		int m_MaxAmmo;
		PolledTimer m_FireTimer;
		PolledTimer m_RechargeTimer;
		FireControllerMode m_Mode;
	};
};
