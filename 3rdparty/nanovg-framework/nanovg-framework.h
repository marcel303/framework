#pragma once

struct NVGcontext;

enum NVGcreateFlags
{
	// Flag indicating if geometry based anti-aliasing is used (may not be needed when using MSAA).
	NVG_ANTIALIAS        = 1<<0,
	
	// Flag indicating if strokes should be drawn using stencil buffer. The rendering will be a little
	// slower, but path overlaps (i.e. self-intersecting or sharp turns) will be drawn just once.
	NVG_STENCIL_STROKES  = 1<<1,
	
	// Flag indicating that additional debug checks are done.
	NVG_DEBUG            = 1<<2,
	
	// Flag indicating that dithering should be used when drawing gradients. Dithering is useful when
	// drawing to e.g. an RGBA8 render target. When the render target has limited precision, banding may
	// occur when drawing gradients. The addition of dithering adds a little bit of noise, which helps
	// to increase the perceptual image quality.
	// NOTE: When you're drawing to a high-precision render target, it may be beneficial to perform
	//       dithering at the end of all of the drawing, right before presenting the image onto the screen.
	NVG_DITHER_GRADIENTS = 1<<3
};

NVGcontext * nvgCreateFramework(int flags);
void nvgDeleteFramework(NVGcontext * ctx);
