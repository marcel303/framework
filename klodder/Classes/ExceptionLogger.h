#pragma once

#include "Exception.h"

class ExceptionLogger
{
public:
	static void Log(const std::exception& e);
};
