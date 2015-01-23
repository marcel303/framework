#pragma once

#include <stddef.h>

void HeapInit();
void* HeapAlloc(size_t size);
void HeapFree(void* p);
