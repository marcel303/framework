#pragma once

class Buffer
{
public:
	Buffer();
	~Buffer();

	void SetSize(int sx, int sy);
	void Clear(float v);
	Buffer* DownScale(int scale) const;
	Buffer* Scale(int scale) const;
	void Combine(Buffer* other);
	void Combine(Buffer* other, int x, int y);
	void Modulate(const float* rgb);
	void ModulateAlpha(float a);
	Buffer* Blur(int strength);
	void DemultiplyAlpha();

	inline float* GetLine(int y)
	{
		return m_Values + y * m_Sx * 4;
	}

	inline const float* GetLine(int y) const
	{
		return m_Values + y * m_Sx * 4;
	}
	
	inline void GetValue_Ex(int x, int y, float** out_Value, bool& out_HasValue)
	{
		if (x < 0 || y < 0 || x > m_Sx -1 || y > m_Sy - 1)
		{
			*out_Value = 0;
			out_HasValue = false;
			return;
		}
		
		*out_Value = GetLine(y) + x * 4;
		out_HasValue = true;
	}

	float* m_Values;
	int m_Sx;
	int m_Sy;
};
