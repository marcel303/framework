#pragma once

namespace lgen
{
	struct Generator
	{
		int ** height = nullptr;
		int w = 0;
		int h = 0;

		virtual ~Generator();

		virtual bool setSize(int w, int h);
		virtual void clear();
		virtual bool generate();

		void clamp(int min, int max);
		void rerange(int min, int max);
		void copy(Generator * dst) const;
		bool getSizePowers(int & pw, int & ph) const;
	};
}

#include "Generator_DiamondSquare.h"
#include "Generator_OffsetSquare.h"
