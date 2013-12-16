#include "Calc.h"
#include "GameSettings.h"
#include "GameState.h"
#include "SoundEffectMgr.h"
#include "TouchZoomController.h"
#include "UsgResources.h"

#define ZOOM_IN_DURATION 0.2f
#define ZOOM_OUT_DURATION 0.2f
#define ZOOM_OUT_TRESHOLD (1.0f / 1.3f)

namespace Game
{
	ZoomInfo::ZoomInfo()
	{
		speed = 0.8f;
		isActive = false;
		zoom = 1.0f;
	}
	
	ZoomInfo ZoomInfo::Interpolate(ZoomInfo zoom1, ZoomInfo zoom2, float t)
	{
		ZoomInfo result;
		
		result.position = zoom1.position.LerpTo(zoom2.position, t);
		result.zoom = Calc::Lerp(zoom1.zoom, zoom2.zoom, t);
		
		return result;
	}
	
	//
	
	TouchZoomController::TouchZoomController()
	{
		Initialize();
	}
	
	void TouchZoomController::Initialize()
	{
		m_MinZoom = 1.0f;
		m_MaxZoom = 1.0f;
		
		m_ManualZoomMode = ManualZoomMode_None;
		m_ManualZoomState = ManualZoomState_Undefined;
		m_CurrentTouchDistance = 0.0f;
	}
	
	void TouchZoomController::Setup(float minZoom, float maxZoom, Vec2F position, float zoom)
	{
		// constraints
		
		m_MinZoom = minZoom;
		m_MaxZoom = maxZoom;
		
		// current state
		
		m_ZoomInfo.position = position;
		m_ZoomInfo.zoom = zoom;
		
		m_ZoomInfoList[ZoomTarget_ZoomedOut].position = Vec2F(WORLD_SX / 2.0f, WORLD_SY / 2.0f);
		m_ZoomInfoList[ZoomTarget_ZoomedOut].zoom = minZoom;
	}
	
	void TouchZoomController::Update(float dt)
	{
		ZoomInfo target = ActiveZoomTargetInfo_get();
		
		const float v1 = powf(1.0f - target.speed, dt);
		const float v2 = 1.0f - v1;
		
		m_ZoomInfo.position = m_ZoomInfo.position * v1 + target.position * v2;
		m_ZoomInfo.zoom = m_ZoomInfo.zoom * v1 + target.zoom * v2;
	}

	// target

	void TouchZoomController::Activate(ZoomTarget target, float speed)
	{
		m_ZoomInfoList[target].speed = speed;
		m_ZoomInfoList[target].isActive = true;
	}
	
	void TouchZoomController::Deactivate(ZoomTarget target)
	{
		m_ZoomInfoList[target].isActive = false;
		
		if (target == ZoomTarget_ZoomedOut)
		{
			g_GameState->m_SoundEffects->Play(Resources::SOUND_ZOOM_IN, SfxFlag_MustFinish);
		}
	}
	
	bool TouchZoomController::IsActive(ZoomTarget target) const
	{
		return m_ZoomInfoList[target].isActive;
	}
	
	void TouchZoomController::UpdateTarget(ZoomTarget target, float zoom, Vec2F position)
	{
		ZoomInfo& zi = m_ZoomInfoList[target];
		
		zi.zoom = zoom;
		zi.position = position;
	}
	
	// manual zoom manipulation
	
	void TouchZoomController::Begin(Vec2F position1, float zoom, Vec2F position)
	{
		Activate(ZoomTarget_Manual, 1.0f);
		
		m_ManualZoomMode = ManualZoomMode_Slide;
		
		m_TouchPosition[0] = position1;
		m_TouchPosition[1] = position1;
		
		m_BeginTouchPosition[0] = position1;
		m_BeginTouchPosition[1] = position1;
		
		m_CurrentTouchDistance = Distance_get();
		
		m_ZoomInfoList[ZoomTarget_Manual].zoom = zoom;
		m_ZoomInfoList[ZoomTarget_Manual].position = position;
	}
	
	void TouchZoomController::Begin(Vec2F position1, Vec2F position2, float zoom, Vec2F position)
	{
		Activate(ZoomTarget_Manual, 1.0f);
		
		m_ManualZoomMode = ManualZoomMode_Pinch;
		
		m_TouchPosition[0] = position1;
		m_TouchPosition[1] = position2;
		
		m_BeginTouchPosition[0] = position1;
		m_BeginTouchPosition[1] = position2;
		
		m_CurrentTouchDistance = Distance_get();
		
		m_ZoomInfoList[ZoomTarget_Manual].zoom = zoom;
		m_ZoomInfoList[ZoomTarget_Manual].position = position;
	}
	
	void TouchZoomController::End(bool autoSnap)
	{
		if (autoSnap && m_ManualZoomMode == ManualZoomMode_Pinch)
		{
			if (m_ZoomInfo.zoom > ZOOM_OUT_TRESHOLD)
			{
				Deactivate(ZoomTarget_Manual);
				
				m_ManualZoomState = ManualZoomState_None;
				
				g_GameState->m_SoundEffects->Play(Resources::SOUND_ZOOM_IN, SfxFlag_MustFinish);
			}
			else
			{
				//m_ManualZoomState = ManualZoomState_ZoomedOut;
				
				//m_ZoomInfoList[ZoomTarget_Manual].zoom = m_MinZoom;
				
				Deactivate(ZoomTarget_Manual);
				
				Activate(ZoomTarget_ZoomedOut, 0.95f);
				
				g_GameState->m_SoundEffects->Play(Resources::SOUND_ZOOM_OUT, SfxFlag_MustFinish);
			}
		}
		else
		{
			Deactivate(ZoomTarget_Manual);
			
			m_ManualZoomState = ManualZoomState_None;
			
			g_GameState->m_SoundEffects->Play(Resources::SOUND_ZOOM_IN, SfxFlag_MustFinish);
		}
	}
	
	void TouchZoomController::Update(int fingerIndex, Vec2F position)
	{
		ZoomInfo& zoomInfo = m_ZoomInfoList[ZoomTarget_Manual];
		
		Vec2F delta = position - m_TouchPosition[fingerIndex];
		
		m_TouchPosition[fingerIndex] = position;
		
		if (m_ManualZoomMode == ManualZoomMode_Slide)
		{
//			m_SlideSpeed = - delta;
			
//			zoomInfo.position += m_SlideSpeed;
			zoomInfo.position -= delta;
			
//			LOG(LogLevel_Debug, "slide speed=(%f, %f)", m_SlideSpeed[0], m_SlideSpeed[1]);
		}
		else if (m_ManualZoomMode == ManualZoomMode_Pinch)
		{
			// calculate movement
			
			const float distance = Distance_get();
			
			// update zoom
			
			const float deltaLength = distance - m_CurrentTouchDistance;
			
			const float zoomDelta = deltaLength / (float)ZOOM_PIXELS;

			// update current touch distance
			
			m_CurrentTouchDistance = distance;
			
			// update zoom info
			
			zoomInfo.zoom += zoomDelta;
			
			// constrain zoom
			
			if (zoomInfo.zoom < m_MinZoom)
				zoomInfo.zoom = m_MinZoom;
			if (zoomInfo.zoom > m_MaxZoom)
				zoomInfo.zoom = m_MaxZoom;
		
			// incorporate movement
			
			zoomInfo.position -= delta * 0.5f;
		}
	}
	
	float TouchZoomController::Distance_get() const
	{
		return m_TouchPosition[0].Distance(m_TouchPosition[1]);
	}
	
	// properties
		
	const ZoomInfo& TouchZoomController::ZoomInfo_get() const
	{
		return m_ZoomInfo;
	}

	ZoomTarget TouchZoomController::ActiveZoomTarget_get() const
	{
		for (int i =0; i < ZoomTarget__Count; ++i)
		{
			if (m_ZoomInfoList[i].isActive)
			{
				return (ZoomTarget)i;
			}
		}
		
		return ZoomTarget_Player;
	}
	
	const ZoomInfo& TouchZoomController::ActiveZoomTargetInfo_get() const
	{
		return m_ZoomInfoList[ActiveZoomTarget_get()];
	}
};
