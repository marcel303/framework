#pragma once

#include <exception>

class Exception : public std::exception
{
public:
	Exception(const char* function, int line) throw();
	Exception(const char* function, int line, const char* what, ...) throw();
	virtual ~Exception() throw ();
	
	virtual const char* what() const throw();
	
private:
	char m_What[1024];
};

#ifdef _MSC_VER
	#define ExceptionVA(x, ...) Exception(__FUNCTION__, __LINE__, x, __VA_ARGS__)
#else
	#define ExceptionVA(x...) Exception(__FUNCTION__, __LINE__, x)
#endif
#define ExceptionNA() Exception(__FUNCTION__, __LINE__)
