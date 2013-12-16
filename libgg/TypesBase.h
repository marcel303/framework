#pragma once

#include <stdint.h>

typedef char XBOOL;
#define XTRUE 1
#define XFALSE 0

#define UsingBegin(x) { volatile x;
#define UsingEnd() }
