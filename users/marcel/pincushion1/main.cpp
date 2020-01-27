#include "framework.h"

// pincusion correction
// from: http://marcodiiga.github.io/radial-lens-undistortion-filtering
static void correctPincusion(const float x, const float y, const float alpha_x, const float alpha_y, float & out_x, float & out_y)
{
#if 0
	// Calculate l2 norm
	const float r = x*x + y*y;

	// Calculate the deflated or inflated new coordinate (reverse transform)
	const float x3 = x / (1.0 - alpha_x * r);
	const float y3 = y / (1.0 - alpha_y * r);
	const float x2 = x / (1.0 - alpha_x * (x3 * x3 + y3 * y3));
	const float y2 = y / (1.0 - alpha_y * (x3 * x3 + y3 * y3));
	
	out_x = x2;
	out_y = y2;
#else
	// from: https://www.photonlexicon.com/forums/showthread.php/26099-geometric-correction-algorithm/page4
	// user: mpolak
	out_x = x + alpha_x * (x * y * y);
	out_y = y + alpha_y * (y * x * x);
#endif
}

static void doPincusion(const float x, const float y, const float alpha_x, const float alpha_y, float & out_x, float & out_y)
{
	// Calculate l2 norm
	const float r = x*x + y*y;

	// Forward transform
	const float x2 = x * (1.0 - alpha_x * r);
	const float y2 = y * (1.0 - alpha_y * r);
	
	out_x = x2;
	out_y = y2;
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(640, 480))
		return -1;
	
	float forward_alpha_x = 0.f;
	float forward_alpha_y = 0.f;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		const float alpha_x = (mouse.x - 320) / 500.f;
		const float alpha_y = (mouse.y - 240) / 500.f;
		
		if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
		{
			forward_alpha_x = alpha_x;
			forward_alpha_y = alpha_y;
		}
		
		printf("%g %g\n", alpha_x, alpha_y);

		const int resolution = 10;
		const int num_lines = resolution * 2 + 1;
		const int line_resolution = 100;
		const int line_num_points = line_resolution * 2 + 1;

		Vec2 points[3][2][num_lines][line_num_points];

		for (int y_itr = -resolution; y_itr <= +resolution; ++y_itr)
		{
			const int line_idx = y_itr + resolution;
			
			const float y = y_itr / float(resolution);
			
			for (int x_itr = -line_resolution; x_itr <= +line_resolution; ++x_itr)
			{
				const int point_idx = x_itr + line_resolution;
				
				const float x = x_itr / float(line_resolution);
				
				// untransformed point
				points[0][0][line_idx][point_idx].Set(x, y);
				
				// with pincushion distortion applied
				float px;
				float py;
				doPincusion(x, y, forward_alpha_x, forward_alpha_y, px, py);
				points[1][0][line_idx][point_idx].Set(px, py);
				
				// with pincushion distortion corrected
				float cx;
				float cy;
				correctPincusion(px, py, alpha_x, alpha_y, cx, cy);
				points[2][0][line_idx][point_idx].Set(cx, cy);
			}
		}

		for (int x_itr = -resolution; x_itr <= +resolution; ++x_itr)
		{
			const int line_idx = x_itr + resolution;
			
			const float x = x_itr / float(resolution);
			
			for (int y_itr = -line_resolution; y_itr <= +line_resolution; ++y_itr)
			{
				const int point_idx = y_itr + line_resolution;
				
				const float y = y_itr / float(line_resolution);
				
				// untransformed point
				points[0][1][line_idx][point_idx].Set(x, y);
				
				// with pincushion distortion applied
				float px;
				float py;
				doPincusion(x, y, forward_alpha_x, forward_alpha_y, px, py);
				points[1][1][line_idx][point_idx].Set(px, py);
				
				// with pincushion distortion corrected
				float cx;
				float cy;
				correctPincusion(px, py, alpha_x, alpha_y, cx, cy);
				points[2][1][line_idx][point_idx].Set(cx, cy);
			}
		}

		framework.beginDraw(0, 0, 0, 0);
		{
			gxPushMatrix();
			{
				gxTranslatef(320, 240, 0);
				gxScalef(100, 100, 0);
				
				for (int transform_idx = 0; transform_idx < 3; ++transform_idx)
				{
					if (transform_idx == 0)
						setColor(colorGreen);
					else if (transform_idx == 1)
						setColor(colorRed);
					else
						setColor(colorWhite);
					
					auto & xform = points[transform_idx];
					
					for (int layer_idx = 0; layer_idx < 2; ++layer_idx)
					{
						auto & layer = xform[layer_idx];
						
						for (int line_idx = 0; line_idx < num_lines; ++line_idx)
						{
							auto & line = layer[line_idx];
							
							setAlpha((transform_idx + 1) * 200 / 3);
							
							gxBegin(GX_LINE_STRIP);
							{
								for (int point_idx = 0; point_idx < line_num_points; ++point_idx)
								{
									gxVertex2f(line[point_idx][0], line[point_idx][1]);
								}
							}
							gxEnd();
						}
					}
				}
			}
			gxPopMatrix();
		}
		framework.endDraw();
	}
	
	return 0;
}
