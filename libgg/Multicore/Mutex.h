#pragma once

// a recursive mutex (meaning you can lock() twice from the same thread without dead-locking)
// upon construction, it doesn't allocate anything or perform any action
// you need to explicitly call alloc/free to create and destroy the underlying mutex

class Mutex
{
	void * opaque = nullptr;

public:
	~Mutex();
	
	void alloc();
	void free();

	void lock() const;
	void unlock() const;
};
