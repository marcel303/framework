#pragma once

#include "Debugging.h"
#include <string.h>

namespace Col
{
	template <typename T>
	struct Array
	{
		T * data;
		int size;

		Array()
			: data(nullptr)
			, size(0)
		{
		}

		Array(int numElements)
			: data(nullptr)
			, size(0)
		{
			resize(numElements);
		}

		~Array()
		{
			resize(0, false);
		}

		void resize(int numElements, bool zeroMemory)
		{
			if (data != nullptr)
			{
				delete [] data;
				data = nullptr;
				size = 0;
			}

			if (numElements != 0)
			{
				data = new T[numElements];
				size = numElements;

				if (zeroMemory)
				{
					memset(data, 0, sizeof(T) * numElements);
				}
			}
		}

		int getSize() const
		{
			return size;
		}

		void setSize(const int size)
		{
			resize(size, false);
		}

		T & operator[](int index)
		{
			Assert(index >= 0);

			return data[index];
		}
	};
}
