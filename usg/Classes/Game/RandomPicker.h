#pragma once

#include "Calc.h"
#include "Debugging.h"
#include "Exception.h"

namespace Game
{
	template <class T, int C>
	class RandomPicker
	{
	public:
		RandomPicker()
		{
			m_ItemCount = 0;
			m_WeightTotal = 0.0f;
		}
		
		void Add(T type, float weight)
		{
			Assert(m_ItemCount < C);
			
			if (weight < 0.0f)
				return;
			
			Item item;
			
			item.m_Type = type;
			item.m_Weight = weight;
			
			m_Items[m_ItemCount] = item;
			m_ItemCount++;

			m_WeightTotal += weight;
		}
		
		T Get() const
		{
			Assert(m_ItemCount != 0);

#ifndef IPHONEOS
			if (m_ItemCount == 0)
			{
				throw ExceptionVA("random picker count == 0", 0);
			}
#endif

			float weight = (Calc::g_RandomHQ.Next() & 4095) / 4095.0f * m_WeightTotal;
			
			for (int i = 0; i < m_ItemCount; ++i)
			{
				weight -= m_Items[i].m_Weight;
				
				if (weight <= 0.0f)
					return m_Items[i].m_Type;
			}
			
			return m_Items[0].m_Type;
		}
		
	private:
		class Item
		{
		public:
			T m_Type;
			float m_Weight;
		};
		
		//std::vector<Item> m_Items;
		Item m_Items[C];
		int m_ItemCount;
		float m_WeightTotal;
	};
}
