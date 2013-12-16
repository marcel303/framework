#pragma once

extern void * operator new(size_t size);
extern void operator delete(void * p);
extern void operator delete[](void * p);
