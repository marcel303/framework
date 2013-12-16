#pragma once

namespace Deployment
{
	#define String(name, value) extern const char* name;
	#include "Deployment.inc"
	#undef String
}

#define NS(text) [NSString stringWithCString:text encoding:NSASCIIStringEncoding]
