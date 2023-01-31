#include "framework.h"
#include "gx_texture.h"
#include "oklab.h"

float srgb_transfer_function(float a)
{
    return .0031308f >= a ? 12.92f * a : 1.055f * powf(a, .4166666666666667f) - .055f;
}

float srgb_transfer_function_inv(float a)
{
    return .04045f < a ? powf((a + .055f) / 1.055f, 2.4f) : a / 12.92f;
}

oklab::RGB srgb_transfer_function(const oklab::RGB & rgb)
{
    return {
        srgb_transfer_function(rgb.r),
        srgb_transfer_function(rgb.g),
        srgb_transfer_function(rgb.b),
    };
}

oklab::RGB srgb_transfer_function_inv(const oklab::RGB & rgb)
{
    return {
        srgb_transfer_function_inv(rgb.r),
        srgb_transfer_function_inv(rgb.g),
        srgb_transfer_function_inv(rgb.b),
    };
}

int main(int argc, char * argv[])
{
    setupPaths(CHIBI_RESOURCE_PATHS);
    
    if (!framework.init(800, 600))
        return -1;
    
    pushFontMode(FONT_SDF);
    
    for (;;)
    {
        framework.process();
        
        if (framework.quitRequested)
            break;
    
        framework.beginDraw(0, 0, 0, 0);
        {
            const oklab::RGB fromRgb_gamma {
                1.f,
                0.f,
                0.f,
            };
            
            const oklab::RGB toRgb_gamma {
                0.f,
                0.f,
                1.f
            };
            
            const oklab::RGB fromRgb_linear = srgb_transfer_function_inv(fromRgb_gamma);
            const oklab::RGB toRgb_linear = srgb_transfer_function_inv(toRgb_gamma);
            
            const oklab::Lab fromLab = oklab::linear_srgb_to_oklab(fromRgb_linear);
            const oklab::Lab toLab = oklab::linear_srgb_to_oklab(toRgb_linear);
            
            for (int i = 0; i < 800; ++i)
            {
                const float t = i / float(800 - 1);
                
                /* oklab */ {
                    const oklab::Lab lab = oklab::mix(fromLab, toLab, t);
                    const oklab::RGB srgb_linear = oklab::oklab_to_linear_srgb(lab);
                    const oklab::RGB srgb_gamma = srgb_transfer_function(srgb_linear);
                    
                    setColorf(srgb_gamma.r, srgb_gamma.g, srgb_gamma.b);
                    drawRect(i, 0, i + 1, 90);
                }
                
                /* srgb_gamma */ {
                    const oklab::RGB srgb_gamma = mix(fromRgb_gamma, toRgb_gamma, t);
                    
                    setColorf(srgb_gamma.r, srgb_gamma.g, srgb_gamma.b);
                    drawRect(i, 100, i + 1, 190);
                }
                
                /* srgb_linear */ {
                    const oklab::RGB srgb_linear = oklab::mix(fromRgb_linear, toRgb_linear, t);
                    const oklab::RGB srgb_gamma = srgb_transfer_function(srgb_linear);
                    
                    setColorf(srgb_gamma.r, srgb_gamma.g, srgb_gamma.b);
                    drawRect(i, 200, i + 1, 290);
                }
            }
            
            setColor(colorWhite);
            drawText(5, 5, 16, +1, +1, "OkLab");
            
            setColor(colorWhite);
            drawText(5, 5 + 100, 16, +1, +1, "sRGB, gamma");
            
            setColor(colorWhite);
            drawText(5, 5 + 200, 16, +1, +1, "sRGB, linear");
        }
        framework.endDraw();
    }
    
    popFontMode();
    
    framework.shutdown();
    
    return 0;
}
