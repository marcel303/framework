#import <assert.h>
#import <UIKit/UIKit.h>
#import "OpenGLCompat.h"
#import "Texture.h"

#define Assert assert

static unsigned int nextPOT(unsigned int x)
{
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >>16);
    return x + 1;
}
 
 
// This is not a fully generalized image loader. It is an example of how to use
// CGImage to directly access decompressed image data. Only the most commonly
// used image formats are supported. It will be necessary to expand this code
// to account for other uses, for example cubemaps or compressed textures.
//
// If the image format is supported, this loader will Gen a OpenGL 2D texture object
// and upload texels from it, padding to POT if needed. For image processing purposes,
// border pixels are also replicated here to ensure proper filtering during e.g. blur.
//
// The caller of this function is responsible for deleting the GL texture object.
void LoadTexture(const char* name, TextureDesc& desc)
{
    GLuint texID = 0, components, x, y;
    GLuint imgWide, imgHigh;      // Real image size
    GLuint rowBytes, rowPixels;   // Image size padded by CGImage
    GLuint POTWide, POTHigh;      // Image size padded to next power of two
    CGBitmapInfo info;            // CGImage component layout info
    CGColorSpaceModel colormodel; // CGImage colormodel (RGB, CMYK, paletted, etc)
    GLenum internal, format;
    GLubyte *pixels, *temp = NULL;
    
    CGImageRef CGImage = [UIImage imageNamed:[NSString stringWithUTF8String:name]].CGImage;
    Assert(CGImage);
    if (!CGImage)
        return;
	
	CGImageRetain(CGImage);
    
    // Parse CGImage info
    info       = CGImageGetBitmapInfo(CGImage);     // CGImage may return pixels in RGBA, BGRA, or ARGB order
    colormodel = CGColorSpaceGetModel(CGImageGetColorSpace(CGImage));
    size_t bpp = CGImageGetBitsPerPixel(CGImage);
    if (bpp < 8 || bpp > 32 || (colormodel != kCGColorSpaceModelMonochrome && colormodel != kCGColorSpaceModelRGB))
    {
        // This loader does not support all possible CGImage types, such as paletted images
        CGImageRelease(CGImage);
        return;
    }
    components = bpp>>3;
    rowBytes   = CGImageGetBytesPerRow(CGImage);    // CGImage may pad rows
    rowPixels  = rowBytes / components;
    imgWide    = CGImageGetWidth(CGImage);
    imgHigh    = CGImageGetHeight(CGImage);
	int imgWide2 = 	rowPixels;
/*    img->wide  = rowPixels;
    img->high  = imgHigh;
    img->s     = (float)imgWide / rowPixels;
    img->t     = 1.0;*/
	desc.s[0] = (float)imgWide / rowPixels;
	desc.s[1] = 1.0f;
 
    // Choose OpenGL format
    switch(bpp)
    {
        default:
            Assert(0 && "Unknown CGImage bpp");
        case 32:
        {
            internal = GL_RGBA;
            switch(info & kCGBitmapAlphaInfoMask)
            {
                case kCGImageAlphaPremultipliedFirst:
                case kCGImageAlphaFirst:
                case kCGImageAlphaNoneSkipFirst:
                    format = GL_BGRA;
                    break;
                default:
                    format = GL_RGBA;
            }
            break;
        }
        case 24:
            internal = format = GL_RGB;
            break;
        case 16:
            internal = format = GL_LUMINANCE_ALPHA;
            break;
        case 8:
            internal = format = GL_LUMINANCE;
            break;
    }
 
    // Get a pointer to the uncompressed image data.
    //
    // This allows access to the original (possibly unpremultiplied) data, but any manipulation
    // (such as scaling) has to be done manually. Contrast this with drawing the image
    // into a CGBitmapContext, which allows scaling, but always forces premultiplication.
    CFDataRef data = CGDataProviderCopyData(CGImageGetDataProvider(CGImage));
    Assert(data);
    pixels = (GLubyte *)CFDataGetBytePtr(data);
    Assert(pixels);
 
    // If the CGImage component layout isn't compatible with OpenGL, fix it.
    // On the device, CGImage will generally return BGRA or RGBA.
    // On the simulator, CGImage may return ARGB, depending on the file format.
    if (format == GL_BGRA)
    {
        uint32_t *p = (uint32_t *)pixels;
        int i, num = imgWide2 * imgHigh;
        
        if ((info & kCGBitmapByteOrderMask) != kCGBitmapByteOrder32Host)
        {
            // Convert from ARGB to BGRA
            for (i = 0; i < num; i++)
                p[i] = (p[i] << 24) | ((p[i] & 0xFF00) << 8) | ((p[i] >> 8) & 0xFF00) | (p[i] >> 24);
        }
        
        // All current iPhoneOS devices support BGRA via an extension.
        //if (!renderer->extension[IMG_texture_format_BGRA8888])
		if (true)
        {
            format = GL_RGBA;
        
            // Convert from BGRA to RGBA
            for (i = 0; i < num; i++)
                #if __LITTLE_ENDIAN__
                p[i] = ((p[i] >> 16) & 0xFF) | (p[i] & 0xFF00FF00) | ((p[i] & 0xFF) << 16);
                #else
                p[i] = ((p[i] & 0xFF00) << 16) | (p[i] & 0xFF00FF) | ((p[i] >> 16) & 0xFF00);
                #endif
        }
    }
 
    // Determine if we need to pad this image to a power of two.
    // There are multiple ways to deal with NPOT images on renderers that only support POT:
    // 1) scale down the image to POT size. Loses quality.
    // 2) pad up the image to POT size. Wastes memory.
    // 3) slice the image into multiple POT textures. Requires more rendering logic.
    //
    // We are only dealing with a single image here, and pick 2) for simplicity.
    //
    // If you prefer 1), you can use CoreGraphics to scale the image into a CGBitmapContext.
    POTWide = nextPOT(imgWide2);
    POTHigh = nextPOT(imgHigh);
 
    if (/*!renderer->extension[APPLE_texture_2D_limited_npot]*/true && (imgWide2 != POTWide || imgHigh != POTHigh))
    {
        GLuint dstBytes = POTWide * components;
        GLubyte *temp = (GLubyte *)malloc(dstBytes * POTHigh);
        
        for (y = 0; y < imgHigh; y++)
            memcpy(&temp[y*dstBytes], &pixels[y*rowBytes], rowBytes);
        
        desc.s[0] *= (float)imgWide2/POTWide;
        desc.s[1] *= (float)imgHigh/POTHigh;
        imgWide2 = POTWide;
        imgHigh = POTHigh;
        pixels = temp;
        rowBytes = dstBytes;
    }
 
    // For filters that sample texel neighborhoods (like blur), we must replicate
    // the edge texels of the original input, to simulate CLAMP_TO_EDGE.
/*    {
        GLuint replicatew = MIN(MAX_FILTER_RADIUS, img->wide-imgWide);
        GLuint replicateh = MIN(MAX_FILTER_RADIUS, img->high-imgHigh);
        GLuint imgRow = imgWide * components;
 
        for (y = 0; y < imgHigh; y++)
            for (x = 0; x < replicatew; x++)
                memcpy(&pixels[y*rowBytes+imgRow+x*components], &pixels[y*rowBytes+imgRow-components], components);
        for (y = imgHigh; y < imgHigh+replicateh; y++)
            memcpy(&pixels[y*rowBytes], &pixels[(imgHigh-1)*rowBytes], imgRow+replicatew*components);
    }*/
    
   // if (img->wide <= renderer->maxTextureSize && img->high <= renderer->maxTextureSize)
	if (true)
    {
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        // Set filtering parameters appropriate for this application (image processing on screen-aligned quads.)
        // Depending on your needs, you may prefer linear filtering, or mipmap generation.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, internal, imgWide2, imgHigh, 0, format, GL_UNSIGNED_BYTE, pixels);
    }
    
    if (temp) free(temp);
    CFRelease(data);
    CGImageRelease(CGImage);
//    img->texID = texID;
	
	desc.textureId = texID;
}
