#ifndef __LgenFilterConvolution_h__
#define __LgenFilterConvolution_h__

#include "LgenFilter.h"

namespace Lgen
{
	
class FilterConvolution : public Filter
{

	public:

    FilterConvolution();
    ~FilterConvolution();

	public:

    virtual bool Apply(Lgen* src, Lgen* dst);
    virtual bool SetOption(std::string name, char* value);

	public:

    int ex, ey;
    float** matrix; // [ex * 2 + 1][ey * 2 + 1]
    
	public:
 
    void SetExtents(int ex, int ey);
    void LoadIdentity();

};

};

#endif // !__LgenFilterConvolution_h__
