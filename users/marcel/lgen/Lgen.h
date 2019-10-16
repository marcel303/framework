#pragma once

namespace lgen
{
	struct Lgen
	{
		int ** height = nullptr;
		int w = 0;
		int h = 0;

		virtual ~Lgen();

		virtual bool setSize(int w, int h);
		virtual void clear();
		virtual bool generate();

		void clamp(int min, int max);
		void rerange(int min, int max);
		void copy(Lgen* dst);
		bool getSizePowers(int & pw, int & ph);
	};
}

#include "LgenOs.h"
#include "LgenDs.h"
