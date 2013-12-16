#pragma once

namespace Paint
{
	typedef struct
	{
		bool m_Pressed;
		int m_X;
		int m_Y;
		int m_Pressure; // 0..255
	} TouchInfo;

	class Touch
	{
	public:
		Touch();

		bool Read(TouchInfo* info);

		bool m_LastPressed;
		int m_LastX;
		int m_LastY;
	};
};
