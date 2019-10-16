#include "LgenFilterRerange.h"
#include <stdlib.h>

namespace lgen
{
    bool FilterRerange::apply(const Heighfield & src, Heighfield & dst)
    {
    	src.copyTo(dst);
        dst.rerange(min, max);

        return true;
    }

    bool FilterRerange::setOption(const std::string & name, const char * value)
    {
        if (name == "min")
        {
            min = atoi(value);
        }
    	else if (name == "max")
        {
            max = atoi(value);
        }
    	else
        {
            return Filter::setOption(name, value);
    	}
            
        return true;
    }
}
