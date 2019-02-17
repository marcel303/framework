#pragma once

#define DefineLinkage(name) \
	void ensureLinkage ## name () \
	{ \
	}

#define EnsureLinkage(name) \
	extern void ensureLinkage ## name (); \
	struct EnsureLinkage ## name \
	{ \
		EnsureLinkage ## name() \
		{ \
			ensureLinkage ## name(); \
		}\
	}; \
	static EnsureLinkage ## name ensureLinkage ## name ## _instance;
