#ifndef DEBUG_H
#define DEBUG_H
#pragma once

#include <assert.h>
#include <stdio.h>

#define FASSERT(expr) assert(expr)
static inline void FVERIFY(bool expr)
{
	FASSERT(expr);
}

#define INITSTATE        bool m_initialized
#define INITINIT         m_initialized = false
#define INITCHECK(state) FASSERT(m_initialized == state)
#define INITSET(state)   m_initialized = state

// TODO: Use OutputDebugString..

// FIXME: VA list: first expand str arg/list, then printf..
#if FDEBUG
	#define DB_LOG printf
	#define DB_ERR(str, ...) DB_LOG("%s: Error: " ## str ## "\n", __FUNCTION__, __VA_ARGS__)
	#define DB_TRACE(str, ...) DB_LOG("%s: " ## str ## "\n", __FUNCTION__, __VA_ARGS__)
#else
	#define DB_LOG(str, ...) do { } while (false)
	#define DB_ERR(str, ...) do { } while (false)
	#define DB_TRACE(str, ...) do { } while (false)
#endif

#endif
