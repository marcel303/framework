#ifndef __LgenDs_h__
#define __LgenDs_h__

#include "lgen.h"

namespace Lgen
{
	
class LgenDs : public Lgen
{

	public:

	LgenDs();
	virtual ~LgenDs();

	public:

	virtual bool Generate(); ///< Generate diamond-square heightfield.

};

};

#endif // !__LgenDs_h__

