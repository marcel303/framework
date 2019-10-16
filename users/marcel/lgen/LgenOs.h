#ifndef __LgenOs_h__
#define __LgenOs_h__

#include "Lgen.h"

namespace Lgen
{
	
class LgenOs : public Lgen
{

	public:

	LgenOs();
	virtual ~LgenOs();

	public:

	virtual bool Generate(); ///< Generate offset-square heightfield.

};

};

#endif // !__LgenOs_h__
