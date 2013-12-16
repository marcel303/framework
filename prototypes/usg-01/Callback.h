#pragma once

typedef void (*CallBackCB)(void* obj, void* arg);

class CallBack
{
public:
	inline CallBack()
	{
		m_Obj = 0;
		m_CallBackCB = 0;
	}

	inline CallBack(void* obj, CallBackCB cb)
	{
		m_Obj = obj;
		m_CallBackCB = cb;
	}

	inline void Invoke(void* arg) const
	{
		m_CallBackCB(m_Obj, arg);
	}

	inline bool IsSet() const
	{
		return m_CallBackCB != 0;
	}

	void* m_Obj;
	CallBackCB m_CallBackCB;
};
