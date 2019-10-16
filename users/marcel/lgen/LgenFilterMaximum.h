#ifndef __LgenFilterMaximum_h__
#define __LgenFilterMaximum_h__

#include "LgenFilter.h"

namespace Lgen
{
	
class FilterMaximum : public Filter
{

	public:

	FilterMaximum();
	virtual ~FilterMaximum();

	public:

	virtual bool Apply(Lgen* src, Lgen* dst);
	virtual bool SetOption(std::string name, char* value);

	public:

    int matrixW;
	int matrixH;

};

};

#endif // !__LgenFilterMaximum_h__
