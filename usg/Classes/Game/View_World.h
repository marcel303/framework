#pragma once

#include "IView.h"
#include "PolledTimer.h"
#include "UiCriticalIndicator.h"
#include "UiGrsRank.h"

namespace Game
{
	class View_World : public IView
	{
	public:
		View_World();
		
		void GrsClient_set(GRS::HttpClient* client);
		
		void HandleWaveBegin();
		
	private:
		// --------------------
		// View
		// --------------------
		virtual void Update(float dt);
		virtual void Render();
		
		virtual int RenderMask_get();
		
		virtual void HandleFocus();
		virtual void HandleFocusLost();
		
		// --------------------
		// Critical gauge
		// --------------------
		UiCriticalGauge m_CriticalGauge;
		
		// --------------------
		// Real-time high score
		// --------------------
		void RefreshRank();
		static void HandleQueryRankResult(void* obj, void* arg);
		
		//GRS::HttpClient* m_GrsClient;
		
		PolledTimer m_GrsUpdateTimer;
		UiGrsRank m_GrsRank;
	};
}
