#pragma once

#include <math.h>

namespace oklab
{
    struct Lab {float L; float a; float b;};
    struct RGB {float r; float g; float b;};

    inline Lab linear_srgb_to_oklab(const RGB & c)
    {
        float l = 0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b;
        float m = 0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b;
        float s = 0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b;
        
        float l_ = cbrtf(l);
        float m_ = cbrtf(m);
        float s_ = cbrtf(s);
        
        return {
            0.2104542553f*l_ + 0.7936177850f*m_ - 0.0040720468f*s_,
            1.9779984951f*l_ - 2.4285922050f*m_ + 0.4505937099f*s_,
            0.0259040371f*l_ + 0.7827717662f*m_ - 0.8086757660f*s_,
        };
    }

    inline RGB oklab_to_linear_srgb(const Lab & c)
    {
        float l_ = c.L + 0.3963377774f * c.a + 0.2158037573f * c.b;
        float m_ = c.L - 0.1055613458f * c.a - 0.0638541728f * c.b;
        float s_ = c.L - 0.0894841775f * c.a - 1.2914855480f * c.b;
        
        float l = l_*l_*l_;
        float m = m_*m_*m_;
        float s = s_*s_*s_;
        
        return {
            +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
            -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
            -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s,
        };
    }

    inline RGB mix(const RGB & from, const RGB & to, const float t)
    {
        return {
            from.r * (1.f - t) + to.r * t,
            from.g * (1.f - t) + to.g * t,
            from.b * (1.f - t) + to.b * t,
        };
    }

    inline Lab mix(const Lab & from, const Lab & to, const float t)
    {
        return {
            from.L * (1.f - t) + to.L * t,
            from.a * (1.f - t) + to.a * t,
            from.b * (1.f - t) + to.b * t,
        };
    }
}
