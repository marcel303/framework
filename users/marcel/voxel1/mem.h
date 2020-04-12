#pragma once

extern void * operator new(size_t size);
extern void operator delete(void * p) noexcept;
extern void operator delete[](void * p) noexcept;
