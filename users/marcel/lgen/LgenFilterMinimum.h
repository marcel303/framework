#ifndef __LgenFilterMinimum_h__
#define __LgenFilterMinimum_h__

#include "LgenFilter.h"

namespace Lgen
{
	
class FilterMinimum : public Filter
{

	public:

    FilterMinimum();
    virtual ~FilterMinimum();

	public:

    virtual bool Apply(Lgen* src, Lgen* dst);
    virtual bool SetOption(std::string name, char* value);

	public:

    int matrixW;
	int matrixH;

};

};

#endif // !__LgenFilterMinimum_h__
