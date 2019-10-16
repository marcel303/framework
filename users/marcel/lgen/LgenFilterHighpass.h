#ifndef __LgenFilterHighpass_h__
#define __LgenFilterHighpass_h__

#include "LgenFilter.h"

namespace Lgen
{
	
class FilterHighpass : public Filter
{

	public:

    FilterHighpass();
    virtual ~FilterHighpass();

	public:

    virtual bool Apply(Lgen* src, Lgen* dst);
    virtual bool SetOption(std::string name, char* value);

	public:

    int matrixW;
	int matrixH;

};

};

#endif // !__LgenFilterHighpass_h__
