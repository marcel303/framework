#pragma once

#include "GuiInterfaceList.h"

namespace Gui
{
	template <class T>
	InterfaceList<T>::InterfaceList()
	{
	}

	template <class T>
	void InterfaceList<T>::Add(T* _interface)
	{
		m_interfaces.push_back(_interface);
	}

	template <class T>
	void InterfaceList<T>::Remove(T* _interface)
	{
		typename std::vector<T*>::iterator itr = m_interfaces.end();

		for (typename std::vector<T*>::iterator i = m_interfaces.begin(); i != m_interfaces.end(); ++i)
			if ((*i) == _interface)
				itr = i;

		if (itr != m_interfaces.end())
			m_interfaces.erase(itr);
	}

	template <class T>
	void InterfaceList<T>::operator+=(T* _interface)
	{
		Add(_interface);
	}

	template <class T>
	void InterfaceList<T>::operator-=(T* _interface)
	{
		Remove(_interface);
	}

	template <class T>
	const std::vector<T*>& InterfaceList<T>::GetInterfaces()
	{
		return m_interfaces;
	}

	template <class T>
	size_t InterfaceList<T>::GetInterfaceCount() const
	{
		return m_interfaces.size();
	}

	template <class T>
	T* InterfaceList<T>::GetInterface(size_t index)
	{
		return m_interfaces[index];
	}
};
