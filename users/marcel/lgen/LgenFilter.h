#pragma once

#include "LgenGenerator.h"
#include "LgenHeightfield.h"
#include <string>

namespace lgen
{
	enum FilterBorderMode
	{
		bmClamp, ///< Clamp values that pass border.
		bmWrap, ///< Wrap values that pass border.
		bmMirror ///< Mirror values that pass border.
	};

	struct Filter
	{
		FilterBorderMode borderMode = bmClamp;
	    
	    int clipX1 = -1; ///< Clipping rectangle.
		int clipY1 = -1;
		int clipX2 = -1;
		int clipY2 = -1;

		virtual ~Filter();

	    int getHeight(const Heighfield & heightfield, int x, int y) const; ///< Return height at (x, y). Applies borderMode setting.
	    void getClippingRect(const Heighfield & heightfield, int & x1, int & y1, int & x2, int & y2) const;
	    void setClippingRect(int x1, int y1, int x2, int y2);
	    
	    virtual bool apply(const Heighfield & src, Heighfield & dst) = 0; ///< Apply filter. Store result in dst.
	    virtual bool setOption(const std::string & name, const char * value); ///< Set filter specific option.
	};
}

#include "LgenFilterMean.h"
#include "LgenFilterMedian.h"
#include "LgenFilterMaximum.h"
#include "LgenFilterMinimum.h"
#include "LgenFilterRerange.h"
#include "LgenFilterHighpass.h"
#include "LgenFilterConvolution.h"
