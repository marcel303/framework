#include "LgenFilterRerange.h"
#include <stdlib.h>
#include <string.h>

namespace lgen
{
    bool FilterRerange::apply(Lgen * src, Lgen * dst)
    {
        src->rerange(min, max);
        src->copy(dst);

        return true;
    }

    bool FilterRerange::setOption(const std::string & name, char * value)
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
