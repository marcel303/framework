#pragma once

#include "TriggerTimerEx.h"

namespace Game
{
	class UiCriticalGauge
	{
	public:
		UiCriticalGauge();
		void Initialize();
		void Setup();
		
		void Update(float dt);
		void Render();
		
		void EnableZoom(float duration);
		
	private:
		void UpdateZoom(float dt);
		void UpdateValue(float dt);
		bool IsZoomComplete_get() const;
		void RenderGauge();
		void RenderText();
		float GaugeValue_get() const;
		
		bool m_IsZoomed;
		float m_Zoom;
		TriggerTimerG m_ZoomDisableTrigger;
		float m_Value;
	};
}
