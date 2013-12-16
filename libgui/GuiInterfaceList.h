#pragma once

#include <vector>

namespace Gui
{
	template <class T>
	class InterfaceList
	{
	public:
		InterfaceList();

		void Add(T* _interface);
		void Remove(T* _interface);

		void operator+=(T* _interface);
		void operator-=(T* _interface);
		const std::vector<T*>& GetInterfaces();

		size_t GetInterfaceCount() const;
		T* GetInterface(size_t index);

	private:
		std::vector<T*> m_interfaces;
	};

	template <class T>
	class InterfaceIterator
	{
	public:
		InterfaceIterator(InterfaceList<T>& interfaceList)
		{
			m_interfaceList = &interfaceList.GetInterfaces();
			m_index = 0;
		}
		T* operator->()
		{
			return (*m_interfaceList)[m_index];
		}
		bool operator()()
		{
			return m_index < m_interfaceList->size();
		}
		void operator++()
		{
			m_index++;
		}
	private:
		const std::vector<T*>* m_interfaceList;
		size_t m_index;
	};

	#define INTERFACE_LIST_CALL(list, function, args) \
	{ \
		for (size_t i = 0; i < list.GetInterfaceCount(); ++i) \
			list.GetInterface(i)->function args; \
	}
};

#include "GuiInterfaceList.inl"
