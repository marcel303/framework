#include <stdlib.h>
#include <string.h>
#include "LgenFilterRerange.h"

namespace Lgen
{
	
FilterRerange :: FilterRerange() : Filter()
{

    min = 0;
    max = 255;

}

FilterRerange :: ~FilterRerange()
{

}

bool FilterRerange::Apply(Lgen* src, Lgen* dst)
{

    src->Rerange(min, max);
    src->Copy(dst);

    return true;
    
}

bool FilterRerange::SetOption(std::string name, char* value)
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
        return Filter::SetOption(name, value);
	}
        
    return true;

}

};
