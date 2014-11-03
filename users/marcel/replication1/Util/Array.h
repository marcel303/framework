#ifndef ARRAY_H
#define ARRAY_H
#pragma once

template <class T>
class Array
{
public:
	Array()
	{
		Zero();
	}

	Array(int size)
	{
		Zero();

		SetSize(size);
	}

	~Array()
	{
		SetSize(0);
	}

	inline int GetSize() const
	{
		return m_size;
	}

	void SetSize(int size)
	{
		if (m_data)
		{
			delete[] m_data;
			m_data = 0;
			m_size = 0;
		}

		if (size == 0)
			return;

		m_size = size;
		m_data = new T[m_size];
	}

	inline T* GetData()
	{
		return m_data;
	}

	inline const T* GetData() const
	{
		return m_data;
	}

	inline T& GetItem(int index)
	{
		return m_data[index];
	}

	inline const T& GetItem(int index) const
	{
		return m_data[index];
	}

	inline T& operator[](int index)
	{
		return GetItem(index);
	}

	inline const T& operator[](int index) const
	{
		return GetItem(index);
	}

private:
	void Zero()
	{
		m_size = 0;
		m_data = 0;
	}

	int m_size;
	T* m_data;
};

#endif
