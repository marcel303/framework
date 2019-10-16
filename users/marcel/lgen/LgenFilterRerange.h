#ifndef __LgenFilterRerange_h__
#define __LgenFilterRerange_h__

#include "LgenFilter.h"

namespace Lgen
{
	
class FilterRerange : public Filter
{

	public:

    FilterRerange();
    ~FilterRerange();

	public:

    virtual bool Apply(Lgen* src, Lgen* dst);
    virtual bool SetOption(std::string name, char* value);

	public:

    int min;
	int max;

};

};

#endif // !__LgenFilterRerange_h__
