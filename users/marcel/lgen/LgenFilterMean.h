#ifndef __LgenFilterMean_h__
#define __LgenFilterMean_h__

#include "LgenFilter.h"

namespace Lgen
{
	
class FilterMean : public Filter
{

	public:
 
    FilterMean();
    virtual ~FilterMean();
    
	public:
 
    virtual bool Apply(Lgen* src, Lgen* dst);
    virtual bool SetOption(std::string name, char* value);
    
	public:
 
    int matrixW;
	int matrixH;
 
};

};

#endif // !__LgenFilterMean_h__
