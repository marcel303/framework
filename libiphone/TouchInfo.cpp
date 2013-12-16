#include "TouchInfo.h"

TouchInfo::TouchInfo()
{
	m_TouchCount = 0;
	m_FingerIndex = 0;
	m_Location.SetZero();
	m_TapTime = 0.0;
	m_TapInterval = 0.0;
}
