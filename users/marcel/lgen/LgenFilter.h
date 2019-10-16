#ifndef __LgenFilter_h__
#define __LgenFilter_h__

#include <string>
#include "Lgen.h"

namespace Lgen
{

enum FilterBorderMode
{
	bmClamp, ///< Clamp values that pass border.
	bmWrap, ///< Wrap values that pass border.
	bmMirror ///< Mirror values that pass border.
};

class Filter
{

	public:
 
    Filter();
    virtual ~Filter();

	public:

    FilterBorderMode borderMode; ///< Border mode.
    
    int clipX1; ///< Clipping rectangle.
	int clipY1;
	int clipX2;
	int clipY2;

	public:
		
    int GetHeight(Lgen* lgen, int x, int y); ///< Return height at (x, y). Applies borderMode setting.
    void GetClippingRect(Lgen* lgen, int& x1, int& y1, int& x2, int& y2); 
    
	public:

    virtual bool Apply(Lgen* src, Lgen* dst); ///< Apply filter. Store result in dst.
    virtual bool SetOption(std::string name, char* value); ///< Set filter specific option.

};

};

#include "LgenFilterMean.h"
#include "LgenFilterMedian.h"
#include "LgenFilterMaximum.h"
#include "LgenFilterMinimum.h"
#include "LgenFilterRerange.h"
#include "LgenFilterHighpass.h"
#include "LgenFilterConvolution.h"

#endif // !__LgenFilter_h__
