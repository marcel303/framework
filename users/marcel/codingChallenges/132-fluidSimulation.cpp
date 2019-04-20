#include "framework.h"
#include <vector>

/*
https://mikeash.com/pyblog/fluid-simulation-for-dummies.html
Coding Challenge #132: Fluid Simulation
https://www.youtube.com/watch?v=alhpH6ECFvQ
*/

#define SCALE 3

#define THREE_DIMENSIONAL 1

#define IX_2D(x, y) ((x) + (y) * N)
#define IX_3D(x, y, z) ((x) + (y) * N + (z) * N * N)

static void set_bnd2d(const int b, float * x, const int N)
{
	for (int i = 1; i < N - 1; ++i)
	{
		x[IX_2D(i, 0  )] = b == 2 ? -x[IX_2D(i, 1  )] : x[IX_2D(i, 1  )];
		x[IX_2D(i, N-1)] = b == 2 ? -x[IX_2D(i, N-2)] : x[IX_2D(i, N-2)];
	}
	
	for (int j = 1; j < N - 1; ++j)
	{
		x[IX_2D(0  , j)] = b == 1 ? -x[IX_2D(1  , j)] : x[IX_2D(1  , j)];
		x[IX_2D(N-1, j)] = b == 1 ? -x[IX_2D(N-2, j)] : x[IX_2D(N-2, j)];
	}

    x[IX_2D(0,     0)]   = 0.5f * (x[IX_2D(1,     0)] + x[IX_2D(0,     1)]);
    x[IX_2D(0,   N-1)]   = 0.5f * (x[IX_2D(1,   N-1)] + x[IX_2D(0,   N-2)]);
    x[IX_2D(N-1,   0)]   = 0.5f * (x[IX_2D(N-2,   0)] + x[IX_2D(N-1,   1)]);
    x[IX_2D(N-1, N-1)]   = 0.5f * (x[IX_2D(N-2, N-1)] + x[IX_2D(N-1, N-2)]);
}

static void set_bnd3d(const int b, float * x, const int N)
{
    for(int j = 1; j < N - 1; j++) {
        for(int i = 1; i < N - 1; i++) {
            x[IX_3D(i, j, 0  )] = b == 3 ? -x[IX_3D(i, j, 1  )] : x[IX_3D(i, j, 1  )];
            x[IX_3D(i, j, N-1)] = b == 3 ? -x[IX_3D(i, j, N-2)] : x[IX_3D(i, j, N-2)];
        }
    }

    for(int k = 1; k < N - 1; k++) {
        for(int i = 1; i < N - 1; i++) {
            x[IX_3D(i, 0  , k)] = b == 2 ? -x[IX_3D(i, 1  , k)] : x[IX_3D(i, 1  , k)];
            x[IX_3D(i, N-1, k)] = b == 2 ? -x[IX_3D(i, N-2, k)] : x[IX_3D(i, N-2, k)];
        }
    }

    for(int k = 1; k < N - 1; k++) {
        for(int j = 1; j < N - 1; j++) {
            x[IX_3D(0  , j, k)] = b == 1 ? -x[IX_3D(1  , j, k)] : x[IX_3D(1  , j, k)];
            x[IX_3D(N-1, j, k)] = b == 1 ? -x[IX_3D(N-2, j, k)] : x[IX_3D(N-2, j, k)];
        }
    }

    x[IX_3D(0, 0, 0)]       = 0.333f * (x[IX_3D(  1,   0,   0)] + x[IX_3D(  0,   1,   0)] + x[IX_3D(  0,   0,   1)]);
    x[IX_3D(0, N-1, 0)]     = 0.333f * (x[IX_3D(  1, N-1,   0)] + x[IX_3D(  0, N-2,   0)] + x[IX_3D(  0, N-1,   1)]);
    x[IX_3D(0, 0, N-1)]     = 0.333f * (x[IX_3D(  1,   0, N-1)] + x[IX_3D(  0,   1, N-1)] + x[IX_3D(  0,   0, N-1)]);
    x[IX_3D(0, N-1, N-1)]   = 0.333f * (x[IX_3D(  1, N-1, N-1)] + x[IX_3D(  0, N-2, N-1)] + x[IX_3D(  0, N-1, N-2)]);
    x[IX_3D(N-1, 0, 0)]     = 0.333f * (x[IX_3D(N-2,   0,   0)] + x[IX_3D(N-1,   1,   0)] + x[IX_3D(N-1,   0,   1)]);
    x[IX_3D(N-1, N-1, 0)]   = 0.333f * (x[IX_3D(N-2, N-1,   0)] + x[IX_3D(N-1, N-2,   0)] + x[IX_3D(N-1, N-1,   1)]);
    x[IX_3D(N-1, 0, N-1)]   = 0.333f * (x[IX_3D(N-2,   0, N-1)] + x[IX_3D(N-1,   1, N-1)] + x[IX_3D(N-1,   0, N-2)]);
    x[IX_3D(N-1, N-1, N-1)] = 0.333f * (x[IX_3D(N-2, N-1, N-1)] + x[IX_3D(N-1, N-2, N-1)] + x[IX_3D(N-1, N-1, N-2)]);
}

static void lin_solve2d(const int b, float * x, const float * x0, const float a, const float c, const int iter, const int N)
{
    float cRecip = 1.f / c;

// todo : we're updating x[] inside the loop while also reading from it. this doesn't seem right

    for (int k = 0; k < iter; ++k)
    {
		for (int j = 1; j < N - 1; ++j)
		{
			const float * prev_line = x0 + IX_2D(0, j    );
			const float * src_line1 = x  + IX_2D(0, j - 1);
			const float * src_line2 = x  + IX_2D(0, j    );
			const float * src_line3 = x  + IX_2D(0, j + 1);
			      float * dest_line = x  + IX_2D(0, j    );
			
			for (int i = 1; i < N - 1; ++i)
			{
			#if 1
				dest_line[i] =
					(
						prev_line[i]
						+ a *
							(
								+src_line2[i + 1]
								+src_line2[i - 1]
								+src_line3[i    ]
								+src_line1[i    ]
							)
					) * cRecip;
			#else
				x[IX_2D(i, j)] =
					(
						x0[IX_2D(i, j)]
						+ a *
							(
								+x[IX_2D(i+1, j  )]
								+x[IX_2D(i-1, j  )]
								+x[IX_2D(i  , j+1)]
								+x[IX_2D(i  , j-1)]
							)
					) * cRecip;
			#endif
			}
		}

        set_bnd2d(b, x, N);
    }
}

static void lin_solve2d_xy(float * x, const float * x0, float * y, const float * y0, const float a, const float c, const int iter, const int N)
{
    float cRecip = 1.f / c;

// todo : we're updating x[] inside the loop while also reading from it. this doesn't seem right

    for (int k = 0; k < iter; ++k)
    {
		for (int j = 1; j < N - 1; ++j)
		{
			const int index = IX_2D(0, j);
			
			const float * x0_line = x0 + index;
			      float * x_line  = x  + index;
			
			const float * y0_line = y0 + index;
			      float * y_line  = y  + index;
			
			float prev_x = x_line[0];
			float prev_y = y_line[0];
			
			for (int i = 1; i < N - 1; ++i)
			{
			#if 0
				x_line[i] = (x0_line[i] + a * (x_line[i - 1] + x_line[i + 1] + x_line[i - N] + x_line[i + N])) * cRecip;
				y_line[i] = (y0_line[i] + a * (y_line[i - 1] + y_line[i + 1] + y_line[i - N] + y_line[i + N])) * cRecip;
			#else
				const float curr_x = x_line[i];
				const float curr_y = x_line[i];
				
				x_line[i] = (x0_line[i] + a * ((prev_x + x_line[i + 1]) + (x_line[i - N] + x_line[i + N]))) * cRecip;
				y_line[i] = (y0_line[i] + a * ((prev_y + y_line[i + 1]) + (y_line[i - N] + y_line[i + N]))) * cRecip;
				
				prev_x = curr_x;
				prev_y = curr_y;
			#endif
			}
		}

        set_bnd2d(1, x, N);
        set_bnd2d(2, x, N);
    }
}

static void lin_solve3d(const int b, float * x, const float * x0, const float a, const float c, const int iter, const int N)
{
    float cRecip = 1.f / c;

    for (int k = 0; k < iter; k++) {
        for (int m = 1; m < N - 1; m++) {
            for (int j = 1; j < N - 1; j++) {
                for (int i = 1; i < N - 1; i++) {
                    x[IX_3D(i, j, m)] =
						(x0[IX_3D(i, j, m)]
                            + a *
                            	(
									+ x[IX_3D(i+1, j  , m  )]
									+ x[IX_3D(i-1, j  , m  )]
									+ x[IX_3D(i  , j+1, m  )]
									+ x[IX_3D(i  , j-1, m  )]
									+ x[IX_3D(i  , j  , m+1)]
									+ x[IX_3D(i  , j  , m-1)]
								)
						) * cRecip;
                }
            }
        }

        set_bnd3d(b, x, N);
    }
}

static void diffuse2d(const int b, float * x, const float * x0, const float diff, const float dt, const int iter, const int N)
{
	const float a = dt * diff * (N - 2);
	lin_solve2d(b, x, x0, a, 1 + 4 * a, iter, N);
}

static void diffuse2d_xy(float * x, const float * x0, float * y, const float * y0, const float diff, const float dt, const int iter, const int N)
{
	const float a = dt * diff * (N - 2);
	lin_solve2d_xy(x, x0, y, y0, a, 1 + 4 * a, iter, N);
}

static void diffuse3d(const int b, float * x, const float * x0, const float diff, const float dt, const int iter, const int N)
{
	const float a = dt * diff * (N - 2) * (N - 2);
	lin_solve3d(b, x, x0, a, 1 + 6 * a, iter, N);
}

static void project2d(float * velocX, float * velocY, float * p, float * div, const int iter, const int N)
{
	for (int j = 1; j < N - 1; ++j)
	{
		for (int i = 1; i < N - 1; ++i)
		{
			div[IX_2D(i, j)] =
				-0.25f *
					(
						+ (+ velocX[IX_2D(i+1, j  )] - velocX[IX_2D(i-1, j  )])
						+ (+ velocY[IX_2D(i  , j+1)] - velocY[IX_2D(i  , j-1)])
					);
		}
	}
	
    set_bnd2d(0, div, N);
	
    memset(p, 0, N * N * sizeof(float));
	lin_solve2d(0, p, div, 1, 4, iter, N);
	
	for (int j = 1; j < N - 1; ++j)
	{
		for (int i = 1; i < N - 1; ++i)
		{
			velocX[IX_2D(i, j)] -= ( p[IX_2D(i+1, j)] - p[IX_2D(i-1, j)] );
			velocY[IX_2D(i, j)] -= ( p[IX_2D(i, j+1)] - p[IX_2D(i, j-1)] );
		}
	}

    set_bnd2d(1, velocX, N);
    set_bnd2d(2, velocY, N);
}

static void project3d(float * velocX, float * velocY, float * velocZ, float * p, float * div, const int iter, const int N)
{
    for (int k = 1; k < N - 1; k++)
    {
        for (int j = 1; j < N - 1; j++)
        {
            for (int i = 1; i < N - 1; i++)
            {
                div[IX_3D(i, j, k)] =
                	-0.5f *
						(
							+ velocX[IX_3D(i+1, j  , k  )] - velocX[IX_3D(i-1, j  , k  )]
							+ velocY[IX_3D(i  , j+1, k  )] - velocY[IX_3D(i  , j-1, k  )]
							+ velocZ[IX_3D(i  , j  , k+1)] - velocZ[IX_3D(i  , j  , k-1)]
                    	) / N;
            }
        }
    }
    set_bnd3d(0, div, N);
	
    memset(p, 0, N * N * N * sizeof(float));
    lin_solve3d(0, p, div, 1, 6, iter, N);
	
    for (int k = 1; k < N - 1; k++)
    {
        for (int j = 1; j < N - 1; j++)
        {
            for (int i = 1; i < N - 1; i++)
            {
                velocX[IX_3D(i, j, k)] -= 0.5f * ( p[IX_3D(i+1, j,   k  )] - p[IX_3D(i-1, j,   k  )] ) * N;
                velocY[IX_3D(i, j, k)] -= 0.5f * ( p[IX_3D(i,   j+1, k  )] - p[IX_3D(i,   j-1, k  )] ) * N;
                velocZ[IX_3D(i, j, k)] -= 0.5f * ( p[IX_3D(i,   j,   k+1)] - p[IX_3D(i,   j,   k-1)] ) * N;
            }
        }
    }
	
    set_bnd3d(1, velocX, N);
    set_bnd3d(2, velocY, N);
    set_bnd3d(3, velocZ, N);
}

static void advect2d(const int b, float * d, const float * d0, const float * velocX, const float * velocY, const float dt, const int N)
{
    const float dtx = dt * (N - 2);
    const float dty = dt * (N - 2);
	
	const float XY_max = N - 1.5f;
	
    float ifloat, jfloat;
    int i, j;
	
	for (j = 1, jfloat = 1; j < N - 1; ++j, ++jfloat)
	{
		for (i = 1, ifloat = 1; i < N - 1; ++i, ++ifloat)
		{
			const float tmp1 = dtx * velocX[IX_2D(i, j)];
			const float tmp2 = dty * velocY[IX_2D(i, j)];
			
			float x = ifloat - tmp1;
			float y = jfloat - tmp2;
			
			if(x < 0.5f) x = 0.5f;
			if(x > XY_max) x = XY_max;
			float i0, i1;
			i0 = floorf(x);
			i1 = i0 + 1.0f;
			
			if(y < 0.5f) y = 0.5f;
			if(y > XY_max) y = XY_max;
			float j0, j1;
			j0 = floorf(y);
			j1 = j0 + 1.0f;
			
			float s0, s1, t0, t1;
			s1 = x - i0;
			s0 = 1.0f - s1;
			t1 = y - j0;
			t0 = 1.0f - t1;
			
			const int i0i = i0;
			const int i1i = i1;
			const int j0i = j0;
			const int j1i = j1;
			
			//Assert(i0i >= 0 && i0i < N);
			//Assert(i1i >= 0 && i1i < N);
			//Assert(j0i >= 0 && j0i < N);
			//Assert(j1i >= 0 && j1i < N);
			
			d[IX_2D(i, j)] =
				s0 * (t0 * d0[IX_2D(i0i, j0i)] + t1 * d0[IX_2D(i0i, j1i)]) +
				s1 * (t0 * d0[IX_2D(i1i, j0i)] + t1 * d0[IX_2D(i1i, j1i)]);
		}
	}
	
    set_bnd2d(b, d, N);
}

static void advect3d(const int b, float * d, const float * d0, const float * velocX, const float * velocY, const float * velocZ, const float dt, const int N)
{
    float i0, i1, j0, j1, k0, k1;
    
    float dtx = dt * (N - 2);
    float dty = dt * (N - 2);
    float dtz = dt * (N - 2);
    
    float s0, s1, t0, t1, u0, u1;
    
    float Nfloat = N;
    float ifloat, jfloat, kfloat;
    int i, j, k;
	
    for (k = 1, kfloat = 1; k < N - 1; k++, kfloat++)
    {
        for (j = 1, jfloat = 1; j < N - 1; j++, jfloat++)
        {
            for (i = 1, ifloat = 1; i < N - 1; i++, ifloat++)
            {
                const float tmp1 = dtx * velocX[IX_3D(i, j, k)];
                const float tmp2 = dty * velocY[IX_3D(i, j, k)];
                const float tmp3 = dtz * velocZ[IX_3D(i, j, k)];
				
                float x = ifloat - tmp1;
                float y = jfloat - tmp2;
                float z = kfloat - tmp3;
				
                if(x < 0.5f) x = 0.5f; 
                if(x > Nfloat - 1.5f) x = Nfloat - 1.5f;
                i0 = floorf(x); 
                i1 = i0 + 1.0f;
				
                if(y < 0.5f) y = 0.5f; 
                if(y > Nfloat - 1.5f) y = Nfloat - 1.5f;
                j0 = floorf(y);
                j1 = j0 + 1.0f;
				
                if(z < 0.5f) z = 0.5f;
                if(z > Nfloat - 1.5f) z = Nfloat - 1.5f;
                k0 = floorf(z);
                k1 = k0 + 1.0f;
				
                s1 = x - i0; 
                s0 = 1.0f - s1; 
                t1 = y - j0; 
                t0 = 1.0f - t1;
                u1 = z - k0;
                u0 = 1.0f - u1;
				
                int i0i = i0;
                int i1i = i1;
                int j0i = j0;
                int j1i = j1;
                int k0i = k0;
                int k1i = k1;
				
                d[IX_3D(i, j, k)] =
					+ s0 *
						+ ( t0 * (u0 * d0[IX_3D(i0i, j0i, k0i)] + u1 * d0[IX_3D(i0i, j0i, k1i)])
                        + ( t1 * (u0 * d0[IX_3D(i0i, j1i, k0i)] + u1 * d0[IX_3D(i0i, j1i, k1i)])))
					+ s1 *
						+ ( t0 * (u0 * d0[IX_3D(i1i, j0i, k0i)] + u1 * d0[IX_3D(i1i, j0i, k1i)])
                        + ( t1 * (u0 * d0[IX_3D(i1i, j1i, k0i)] + u1 * d0[IX_3D(i1i, j1i, k1i)])));
            }
        }
    }
	
    set_bnd3d(b, d, N);
}

struct FluidCube2d
{
	int size;

	float dt;
	float diff; // diffusion amount
	float visc; // viscosity

	std::vector<float> s;
	std::vector<float> density;

	std::vector<float> Vx;
	std::vector<float> Vy;

	std::vector<float> Vx0;
	std::vector<float> Vy0;

	void addDensity(const int x, const int y, const float amount)
	{
		if (x < 0 || x >= size ||
			y < 0 || y >= size)
		{
			return;
		}
		
		const int N = size;
		const int index = IX_2D(x, y);
		
		density[index] += amount;
	}
	
	void addVelocity(const int x, const int y, const float amountX, const float amountY)
	{
		if (x < 0 || x >= size ||
			y < 0 || y >= size)
		{
			return;
		}
		
		const int N = size;
		const int index = IX_2D(x, y);

		Vx[index] += amountX;
		Vy[index] += amountY;
	}

	void step()
	{
	    const int N = size;
		
	    const int iter = 4;
	
		diffuse2d_xy(Vx0.data(), Vx.data(), Vy0.data(), Vy.data(), visc, dt, iter, N);
		
		project2d(Vx0.data(), Vy0.data(), Vx.data(), Vy.data(), iter, N);
		
		advect2d(1, Vx.data(), Vx0.data(), Vx0.data(), Vy0.data(), dt, N);
	    advect2d(2, Vy.data(), Vy0.data(), Vx0.data(), Vy0.data(), dt, N);
		
		project2d(Vx.data(), Vy.data(), Vx0.data(), Vy0.data(), iter, N);
		
		diffuse2d(0, s.data(), density.data(), diff, dt, iter, N);
		
		advect2d(0, density.data(), s.data(), Vx.data(), Vy.data(), dt, N);
	}
};

struct FluidCube3d
{
	int size;

	float dt;
	float diff; // diffusion amount
	float visc; // viscosity

	std::vector<float> s;
	std::vector<float> density;

	std::vector<float> Vx;
	std::vector<float> Vy;
	std::vector<float> Vz;

	std::vector<float> Vx0;
	std::vector<float> Vy0;
	std::vector<float> Vz0;

	void addDensity(const int x, const int y, const int z, const float amount)
	{
		if (x < 0 || x >= size ||
			y < 0 || y >= size ||
			z < 0 || z >= size)
		{
			return;
		}
		
		const int N = size;
		const int index = IX_3D(x, y, z);
		
		density[index] += amount;
	}
	
	void addVelocity(const int x, const int y, const int z, const float amountX, const float amountY, const float amountZ)
	{
		if (x < 0 || x >= size ||
			y < 0 || y >= size ||
			z < 0 || z >= size)
		{
			return;
		}
		
		const int N = size;
		const int index = IX_3D(x, y, z);

		Vx[index] += amountX;
		Vy[index] += amountY;
		Vz[index] += amountZ;
	}

	void step()
	{
	    const int N = size;
		
	    const int iter = 4;
	
		diffuse3d(1, Vx0.data(), Vx.data(), visc, dt, iter, N);
	    diffuse3d(2, Vy0.data(), Vy.data(), visc, dt, iter, N);
	    diffuse3d(3, Vz0.data(), Vz.data(), visc, dt, iter, N);
		
		project3d(Vx0.data(), Vy0.data(), Vz0.data(), Vx.data(), Vy.data(), iter, N);
		
		advect3d(1, Vx.data(), Vx0.data(), Vx0.data(), Vy0.data(), Vz0.data(), dt, N);
	    advect3d(2, Vy.data(), Vy0.data(), Vx0.data(), Vy0.data(), Vz0.data(), dt, N);
	    advect3d(3, Vz.data(), Vz0.data(), Vx0.data(), Vy0.data(), Vz0.data(), dt, N);
		
		project3d(Vx.data(), Vy.data(), Vz.data(), Vx0.data(), Vy0.data(), iter, N);
		
		diffuse3d(0, s.data(), density.data(), diff, dt, iter, N);
		
	    advect3d(0, density.data(), s.data(), Vx.data(), Vy.data(), Vz.data(), dt, N);
	}
};

FluidCube2d * createFluidCube2d(const int size, const float diffusion, const float viscosity, const float dt)
{
	FluidCube2d * cube = new FluidCube2d();

	cube->size = size;
	cube->dt = dt;
	cube->diff = diffusion;
	cube->visc = viscosity;

	//const int N = size * size * size;
	const int N = size * size;

	cube->s.resize(N, 0.f);
	cube->density.resize(N, 0.f);

	cube->Vx.resize(N, 0.f);
	cube->Vy.resize(N, 0.f);

	cube->Vx0.resize(N, 0.f);
	cube->Vy0.resize(N, 0.f);

	return cube;
}

FluidCube3d * createFluidCube3d(const int size, const float diffusion, const float viscosity, const float dt)
{
	FluidCube3d * cube = new FluidCube3d();

	cube->size = size;
	cube->dt = dt;
	cube->diff = diffusion;
	cube->visc = viscosity;

	const int N = size * size * size;

	cube->s.resize(N, 0.f);
	cube->density.resize(N, 0.f);

	cube->Vx.resize(N, 0.f);
	cube->Vy.resize(N, 0.f);
	cube->Vz.resize(N, 0.f);

	cube->Vx0.resize(N, 0.f);
	cube->Vy0.resize(N, 0.f);
	cube->Vz0.resize(N, 0.f);

	return cube;
}

int main(int argc, const char * argv[])
{
	if (!framework.init(600, 600))
		return -1;

#if THREE_DIMENSIONAL
	FluidCube3d * cube = createFluidCube3d(48, 0.001f, 0.0001f, 1.f / 30.f);
#else
	FluidCube2d * cube = createFluidCube2d(200, 0.001f, 0.0001f, 1.f / 30.f);
#endif

	GxTexture texture;
	texture.allocate(cube->size, cube->size, GX_R32_FLOAT, false, true);
	texture.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		for (auto & d : cube->density)
			d *= .99f;
		
	#if THREE_DIMENSIONAL
		cube->addDensity(mouse.x / SCALE, mouse.y / SCALE, cube->size/2, 100.f);
		cube->addVelocity(mouse.x / SCALE, mouse.y / SCALE, cube->size/2, mouse.dx, mouse.dy, cosf(framework.time) * 20.f);
	#else
		for (int x = -4; x <= +4; ++x)
		{
			for (int y = -4; y <= +4; ++y)
			{
				cube->addDensity(mouse.x / SCALE + x, mouse.y / SCALE + y, 1.f);
				cube->addVelocity(mouse.x / SCALE, mouse.y / SCALE, mouse.dx / 100.f, mouse.dy / 100.f);
			}
		}
	#endif
		
		cube->step();
		
	#if THREE_DIMENSIONAL
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(60.f, .01f, 100.f);
			gxTranslatef(0, 0, 2);
			gxRotatef(framework.time * 10.f, 0, 1, 0);
			
			setBlend(BLEND_ADD);
			setColor(colorWhite);
			setAlphaf(.3f);
			
			for (int z = 0; z < cube->size; ++z)
			{
				gxPushMatrix();
				gxTranslatef(0, 0, lerp<float>(-.5f, +.5f, z / float(cube->size - 1)));
				const int N = cube->size;
				texture.upload(cube->density.data() + IX_3D(0, 0, z), 4, 0);
				
				gxSetTexture(texture.id);
				drawRect(-.5f, -.5f, .5f, .5f);
				gxSetTexture(0);
				gxPopMatrix();
			}
			
			lineCube(Vec3(), Vec3(.5f, .5f, .5f));
		}
		framework.endDraw();
	#else
		framework.beginDraw(255, 255, 255, 0);
		{
			texture.upload(cube->density.data(), 4, 0);
			gxSetTexture(texture.id);
			drawRect(0, 0, cube->size * SCALE, cube->size * SCALE);
			gxSetTexture(0);
		}
		framework.endDraw();
	#endif
	}
	
	texture.free();

	delete cube;
	cube = nullptr;

	framework.shutdown();
	
	return 0;
}
