#pragma once

#include "PolledTimer.h"
#include "Traveller.h"
#include "Types.h"

namespace Game
{
	class MissileTrail
	{
	public:
		MissileTrail();
		~MissileTrail();
		void Initialize();
		void Setup(const Vec2F& pos);
		
		void Update(float dt);
		void Position_set(const Vec2F& position);
		
	private:
		static void HandleTravelEvent(void* obj, void* arg);

		Traveller m_Traveller;
	};
}
