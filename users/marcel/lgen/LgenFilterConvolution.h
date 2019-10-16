#pragma once

#include "LgenFilter.h"

namespace lgen
{	
    struct FilterConvolution : Filter
    {
        int ex = -1;
        int ey = -1;
        float ** matrix = nullptr; // [ex * 2 + 1][ey * 2 + 1]

        virtual ~FilterConvolution() override;

        virtual bool apply(const Generator * src, Generator * dst) override;
        virtual bool setOption(const std::string & name, char * value) override;
     
        void setExtents(int ex, int ey);
        void loadIdentity();
    };
}
