#pragma once

#include "LgenFilter.h"

namespace lgen
{	
	struct FilterQuantize : Filter
	{
		int numLevels = 8;
		
		bool setNumLevels(const int numLevels);

		virtual bool apply(const Heightfield & src, Heightfield & dst) override;
		virtual bool setOption(const std::string & name, const char * value) override;
	};
	
	bool filterQuantize(const Heightfield & src, Heightfield & dst, const int numLevels);
}
