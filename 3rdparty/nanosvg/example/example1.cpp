//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include <stdio.h>
#include <string.h>
#include <float.h>

#include "framework.h"

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"

NSVGimage* g_image = NULL;

static unsigned char bgColor[4] = {205,202,200,255};
static unsigned char lineColor[4] = {0,160,192,255};

static float distPtSeg(float x, float y, float px, float py, float qx, float qy)
{
	float pqx, pqy, dx, dy, d, t;
	pqx = qx-px;
	pqy = qy-py;
	dx = x-px;
	dy = y-py;
	d = pqx*pqx + pqy*pqy;
	t = pqx*dx + pqy*dy;
	if (d > 0) t /= d;
	if (t < 0) t = 0;
	else if (t > 1) t = 1;
	dx = px + t*pqx - x;
	dy = py + t*pqy - y;
	return dx*dx + dy*dy;
}

static void cubicBez(float x1, float y1, float x2, float y2,
					 float x3, float y3, float x4, float y4,
					 float tol, int level)
{
	float x12,y12,x23,y23,x34,y34,x123,y123,x234,y234,x1234,y1234;
	float d;
	
	if (level > 12) return;

	x12 = (x1+x2)*0.5f;
	y12 = (y1+y2)*0.5f;
	x23 = (x2+x3)*0.5f;
	y23 = (y2+y3)*0.5f;
	x34 = (x3+x4)*0.5f;
	y34 = (y3+y4)*0.5f;
	x123 = (x12+x23)*0.5f;
	y123 = (y12+y23)*0.5f;
	x234 = (x23+x34)*0.5f;
	y234 = (y23+y34)*0.5f;
	x1234 = (x123+x234)*0.5f;
	y1234 = (y123+y234)*0.5f;

	d = distPtSeg(x1234, y1234, x1,y1, x4,y4);
	if (d > tol*tol) {
		cubicBez(x1,y1, x12,y12, x123,y123, x1234,y1234, tol, level+1); 
		cubicBez(x1234,y1234, x234,y234, x34,y34, x4,y4, tol, level+1); 
	} else {
		gxVertex2f(x4, y4);
	}
}

void drawPath(float* pts, int npts, char closed, float tol)
{
	int i;
	gxBegin(GX_LINE_STRIP);
	gxColor4ub(lineColor[0], lineColor[1], lineColor[2], lineColor[3]);
	gxVertex2f(pts[0], pts[1]);
	for (i = 0; i < npts-1; i += 3) {
		float* p = &pts[i*2];
		cubicBez(p[0],p[1], p[2],p[3], p[4],p[5], p[6],p[7], tol, 0);
	}
	if (closed) {
		gxVertex2f(pts[0], pts[1]);
	}
	gxEnd();
}

void drawControlPts(float* pts, int npts)
{
	int i;

	// Control lines
	gxColor4ub(lineColor[0], lineColor[1], lineColor[2], lineColor[3]);
	gxBegin(GX_LINES);
	for (i = 0; i < npts-1; i += 3) {
		float* p = &pts[i*2];
		gxVertex2f(p[0],p[1]);
		gxVertex2f(p[2],p[3]);
		gxVertex2f(p[4],p[5]);
		gxVertex2f(p[6],p[7]);
	}
	gxEnd();

	// Points
	gxColor4ub(lineColor[0], lineColor[1], lineColor[2], lineColor[3]);

	hqBegin(HQ_FILLED_CIRCLES);
	hqFillCircle(pts[0],pts[1], 3.f);
	for (i = 0; i < npts-1; i += 3) {
		float* p = &pts[i*2];
		hqFillCircle(p[6],p[7], 3.f);
	}
	hqEnd();

	// Points
	hqBegin(HQ_FILLED_CIRCLES);
	gxColor4ub(bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
	hqFillCircle(pts[0],pts[1], 1.5f);
	for (i = 0; i < npts-1; i += 3) {
		float* p = &pts[i*2];
		gxColor4ub(lineColor[0], lineColor[1], lineColor[2], lineColor[3]);
		hqFillCircle(p[2],p[3], 1.5f);
		hqFillCircle(p[4],p[5], 1.5f);
		gxColor4ub(bgColor[0], bgColor[1], bgColor[2], bgColor[3]);
		hqFillCircle(p[6],p[7], 1.5f);
	}
	hqEnd();
}

void drawframe()
{
	int width = 0, height = 0;
	float view[4], cx, cy, hw, hh, aspect, px;
	NSVGshape* shape;
	NSVGpath* path;

	// Fit view to bounds
	cx = g_image->width*0.5f;
	cy = g_image->height*0.5f;
	hw = g_image->width*0.5f;
	hh = g_image->height*0.5f;

	if (width/hw < height/hh) {
		aspect = (float)height / (float)width;
		view[0] = cx - hw * 1.2f;
		view[2] = cx + hw * 1.2f;
		view[1] = cy - hw * 1.2f * aspect;
		view[3] = cy + hw * 1.2f * aspect;
	} else {
		aspect = (float)width / (float)height;
		view[0] = cx - hh * 1.2f * aspect;
		view[2] = cx + hh * 1.2f * aspect;
		view[1] = cy - hh * 1.2f;
		view[3] = cy + hh * 1.2f;
	}
	
	// Size of one pixel.
	px = (view[2] - view[1]) / (float)width;

	setColor(colorWhite);

	// Draw bounds
	setColor(0,0,0,64);
	gxBegin(GX_LINE_LOOP);
	gxVertex2f(0, 0);
	gxVertex2f(g_image->width, 0);
	gxVertex2f(g_image->width, g_image->height);
	gxVertex2f(0, g_image->height);
	gxEnd();

	for (shape = g_image->shapes; shape != NULL; shape = shape->next) {
		for (path = shape->paths; path != NULL; path = path->next) {
			drawPath(path->pts, path->npts, path->closed, px * 1.5f);
			drawControlPts(path->pts, path->npts);
		}
	}
}

int main()
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 600))
		return -1;
		

	g_image = nsvgParseFromFile("../example/nano.svg", "px", 96.0f);
	if (g_image == NULL) {
		printf("Could not open SVG image.\n");
		return -1;
	}

	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		framework.beginDraw(220, 220, 220, 255);
		{
			pushLineSmooth(true);
			{
				drawframe();
			}
			popLineSmooth();
		}
		framework.endDraw();
	}
	
	nsvgDelete(g_image);

	framework.shutdown();
	
	return 0;
}
