inline int Matrix::Index(int x, int y) const
{

	return y + x * s;
	
}

inline const float& Matrix::operator()(int x, int y) const
{

	return v[Index(x, y)];	
	
}

inline float& Matrix::operator()(int x, int y)
{

	return v[Index(x, y)];	
	
}
	
inline Matrix::operator float*()
{

	return v;

}