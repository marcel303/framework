#pragma once

struct NVGcontext;

enum NVGcreateFlags
{
	// Flag indicating if geometry based anti-aliasing is used (may not be needed when using MSAA).
	NVG_ANTIALIAS 		= 1<<0,
	
	// Flag indicating if strokes should be drawn using stencil buffer. The rendering will be a little
	// slower, but path overlaps (i.e. self-intersecting or sharp turns) will be drawn just once.
	NVG_STENCIL_STROKES	= 1<<1,
	
	// Flag indicating that additional debug checks are done.
	NVG_DEBUG 			= 1<<2,
};

NVGcontext * nvgCreateFramework(int flags);
void nvgDeleteFramework(NVGcontext * ctx);
