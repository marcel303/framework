#ifndef __LgenFilterMedian_h__
#define __LgenFilterMedian_h__

#include "LgenFilter.h"

namespace Lgen
{
	
class FilterMedian : public Filter
{
	
	public:

    FilterMedian();
    virtual ~FilterMedian();

	public:

    virtual bool Apply(Lgen* src, Lgen* dst);
    virtual bool SetOption(std::string name, char* value);

	public:

    int matrixW;
	int matrixH;

};

};

#endif
