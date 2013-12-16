#pragma once

#include "AnimTimer.h"
#include "Types.h"

namespace Game
{
	enum ZoomTarget
	{
		ZoomTarget_Scripted,
		ZoomTarget_ZoomedOut,
		ZoomTarget_Manual,
		ZoomTarget_Player,
		ZoomTarget__Count
	};
	
	enum ManualZoomMode
	{
		ManualZoomMode_None,
		ManualZoomMode_Decide,
		ManualZoomMode_Pinch,
		ManualZoomMode_Slide
	};
	
	enum ManualZoomState
	{
		ManualZoomState_Undefined,
		ManualZoomState_None,
		ManualZoomState_Controlling,
		ManualZoomState_ZoomedOut
	};
	
	class ZoomInfo
	{
	public:
		ZoomInfo();
		static ZoomInfo Interpolate(ZoomInfo zoom1, ZoomInfo zoom2, float t);
		
		float speed;
		bool isActive;
		Vec2F position;
		float zoom;
	};
	
	class TouchZoomController
	{
	public:
		TouchZoomController();
		void Initialize();
		
		void Setup(float minZoom, float maxZoom, Vec2F position, float zoom);
		
		void Update(float dt);
		
		// target
		
		void Activate(ZoomTarget target, float speed);
		void Deactivate(ZoomTarget target);
		bool IsActive(ZoomTarget target) const;
		void UpdateTarget(ZoomTarget target, float zoom, Vec2F position);
		
		// manual zoom manipulation
		
		void Begin(Vec2F position1, float zoom, Vec2F position);
		void Begin(Vec2F position1, Vec2F position2, float zoom, Vec2F position);
		void End(bool autoSnap);
		void Update(int fingerIndex, Vec2F position);
		
		float Distance_get() const;
		
		ManualZoomState ManualZoomState_get() const
		{
			return m_ManualZoomState;
		}

		// properties
		
		const ZoomInfo& ZoomInfo_get() const;
		ZoomTarget ActiveZoomTarget_get() const;
		const ZoomInfo& ActiveZoomTargetInfo_get() const;
		
	private:
		// constraints
		
		float m_MinZoom;
		float m_MaxZoom;
		
		// targets
		
		ZoomInfo m_ZoomInfoList[ZoomTarget__Count];

		// current state
		
		ZoomInfo m_ZoomInfo;
		
		// manual control
		
		ManualZoomMode m_ManualZoomMode;
		ManualZoomState m_ManualZoomState;
		
		// manual control tracking
		
		Vec2F m_BeginTouchPosition[2];
		Vec2F m_TouchPosition[2];
		float m_CurrentTouchDistance;
	};
}
