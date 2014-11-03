#ifndef D3DRESULT_H
#define D3DRESULT_H
#pragma once

#include <d3d9.h>

class D3DResult
{
public:
	inline D3DResult()
	{
		m_lastResult = m_result = 0;
	}

	inline int GetError()
	{
		m_lastResult = m_result;

		HRESULT r;

		if (FAILED(m_result))
			r = m_result;
		else
			r = 0;

		m_result = 0;

		return r;
	}

	inline void operator=(HRESULT result)
	{
		if (m_result == 0)
			m_result = result;
	}

	HRESULT m_result;
	HRESULT m_lastResult;
};

#endif
