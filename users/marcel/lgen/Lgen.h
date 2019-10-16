#ifndef __Lgen_h__
#define __Lgen_h__

namespace Lgen
{

class Lgen
{

	public:

	Lgen();
	virtual ~Lgen();

	public:

	int** height;
	int w;
	int h;
 
	public:

	virtual bool SetSize(int w, int h); ///< Setting a size of (0, 0) will free memory.
	virtual void Clear(); ///< Set height values to 0.
	virtual bool Generate(); ///< Generate heightfield.
	void Clamp(int min, int max); ///< Clamp the height values to specified range.
	void Rerange(int min, int max); ///< Scales and translates heightvalues into range [min, max].
	void Copy(Lgen* dst); ///< Copy heightfield into dst.
	bool GetSizePowers(int& pw, int& ph); ///< 2**pw = w and 2**ph = h. Return false if w or h isn't a power of 2.
        
};

};

#include "LgenOs.h"
#include "LgenDs.h"

#endif // !__Lgen_h__
