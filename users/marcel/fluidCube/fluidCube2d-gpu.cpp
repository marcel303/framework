#include "fluidCube2d-gpu.h"

/*

todo : expperiment with the following boundary modes, and external forces,
from: http://karlsims.com/fluid-flow.html,

Zero: using a value of zero beyond the grid will avoid flow toward or away from the boundary, as if the fluid is contained in a box, because any edge cell's component of flow normal to the boundary would typically create a non-zero divergence and be removed. The divergence-removal examples above used this mode.
Repeat: repeating the flow value from the nearest edge cell will instead allow flow toward or away from the edges as if the fluid can exit or enter the grid.
Scaled repeat: combining zero and repeat edge-modes can also be useful. Using a scaled value of the nearest edge cell can give a soft boundary effect that slows down the flow at the edges but doesn't completely stop it. Alternatively, using zero for the flow component normal to the boundary, and repeat mode for the component parallel to the boundary, can give a more slippery edge effect.
Wrap: copying the flow value from the opposite side of the grid can create a wrap-around behavior where flow exiting on one side of the grid enters on the other.

Obstacles can also be added to the grid with similar boundary conditions by forcing the flow, or its normal component, to zero at their locations.

--

Realistic looking fluid behavior can be generated by alternating between the fluid momentum and divergence-removal steps described above. However some non-zero flow velocity needs to be set somehow, either procedurally or interactively. For example, adding linear flow at specific locations can create a squirting ink effect (see below), or tracking mouse or camera motion can be used to interactively push the fluid.

Other forces can be added to simulate various physical phenomena:

Gravity or buoyency can affect some parts of the fluid differently than others using a tracer image that represents the fluid density.
Damping or friction can reduce the flow velocity over time.
Viscosity can be simulated by diffusing the flow field slightly at each time step so the velocities become more like their neighbors. Note that the repeated resampling of the flow field will also cause a small amount of diffusion.
Cohesive forces can be approximated by using a tracer image to track different fluid substances, such as oil vs water, and then pushing the substances toward areas of the same type, moving convex boundaries inwards and concave boundaries outwards.

*/

// -----

/*
getOrCreateShader is a helper function, which constructs a shader given the pixel shader code in 'code',
and the uniforms (and optionally other things) specified in 'globals'. it will use a basic vertex shader
which outputs the position and texture coordinate, and a pixel shader template which will call the
provided code

getOrCreateShader is used throughout the code when a GPU shader is needed
*/

#include "StringEx.h"
#include <set>
#include <string>

static std::set<std::string> s_createdShaders;

static void getOrCreateShader(const char * name, const char * code, const char * globals)
{
	// don't do anything if the shader already exists
	
	if (s_createdShaders.count(name) == 0)
	{
		// remember we processed this shader
		
		s_createdShaders.insert(name);
		
		// define the vertex shader
		
		const char * vs =
			R"SHADER(
				include engine/ShaderVS.txt

				shader_out vec2 v_texcoord;

				void main()
				{
					vec4 position = unpackPosition();

					gl_Position = objectToProjection(position);
					
					v_texcoord = unpackTexcoord(0);
				}
			)SHADER";
		
		// define the pixel shader, using a template and the code and globals passed into this function
		
		const char * ps_template =
			R"SHADER(
				include engine/ShaderPS.txt

				%s
		
				shader_in vec2 v_texcoord;

				#define samp(in_s, in_x, in_y) textureOffset(in_s, v_texcoord, ivec2(in_x, in_y)).x
		
				float samp_filter(sampler2D s, float x, float y)
				{
					vec2 size = textureSize(s, 0);
					
					return texture(s, v_texcoord + vec2(x, y) / size).x;
				}
		
				float process()
				{
					%s
				}
		
				void main()
				{
					shader_fragColor = vec4(process());
					shader_fragColor.a = 1.0; // has to be 1.0 because BLEND_ADD multiplies the rgb with this value before addition
				}
			)SHADER";
		
		char ps[1024];
		sprintf_s(ps, sizeof(ps), ps_template, globals, code);
		
		// register shader sources with framework
		
		char vs_name[64];
		char ps_name[64];
		sprintf_s(vs_name, sizeof(vs_name), "%s.vs", name);
		sprintf_s(ps_name, sizeof(ps_name), "%s.ps", name);
		shaderSource(vs_name, vs);
		shaderSource(ps_name, ps);
		
		// construct the shader, so we can catch any errors here
		// note this step is optional and just here for convenience
		Shader shader(name, vs_name, ps_name);
	#if ENABLE_OPENGL
		checkErrorGL();
	#endif
	}
}

// -----

static void set_bnd2d(const int b, Surface * x, const int sizeX, const int sizeY)
{
	// just here for future reference. this is where the cpu version would apply its
	// boundary condition. for the gpu version this would be relatively expensive to
	// implement (naively). we don't seem to actually need it, so we leave it empty for now
}

static void lin_solve2d(const int b, Surface * x, const Surface * x0, const float a, const float c, const int iter, const int sizeX, const int sizeY)
{
    float cRecip = 1.f / c;

	getOrCreateShader("lin_solve2d",
		R"SHADER(
			return
				(
					samp(x0, 0, 0)
					+ a *
						(
							+samp(x, +1,  0)
							+samp(x, -1,  0)
							+samp(x,  0, +1)
							+samp(x,  0, -1)
						)
				) * cRecip;
		)SHADER",
		R"SHADER(
			uniform sampler2D x;
			uniform sampler2D x0;
			uniform float a;
			uniform float cRecip;
		)SHADER");
	
    for (int k = 0; k < iter; ++k)
    {
		Shader shader("lin_solve2d");
		setShader(shader);
		shader.setTexture("x", 0, x->getTexture(), false, true);
		shader.setTexture("x0", 1, x0->getTexture(), false, true);
		shader.setImmediate("a", a);
		shader.setImmediate("cRecip", cRecip);
    	x->postprocess(shader);
		
        set_bnd2d(b, x, sizeX, sizeY);
    }
}

static void lin_solve2d_xy(
	Surface * x, const Surface * x0,
	Surface * y, const Surface * y0,
	const float a, const float c, const int iter, const int sizeX, const int sizeY)
{
    float cRecip = 1.f / c;

	getOrCreateShader("lin_solve2d_xy",
		R"SHADER(
			return
				(
					samp(x0, 0, 0)
					+ a *
						(
							+samp(x, +1,  0)
							+samp(x, -1,  0)
							+samp(x,  0, +1)
							+samp(x,  0, -1)
						)
				) * cRecip;
		)SHADER",
		R"SHADER(
			uniform sampler2D x;
			uniform sampler2D x0;
			uniform float a;
			uniform float cRecip;
		)SHADER");
	
	for (int k = 0; k < iter; ++k)
    {
		Shader shader("lin_solve2d_xy");
		setShader(shader);
		shader.setTexture("x", 0, x->getTexture(), false, true);
		shader.setTexture("x0", 1, x0->getTexture(), false, true);
		shader.setImmediate("a", a);
		shader.setImmediate("cRecip", cRecip);
    	x->postprocess(shader);
		
        set_bnd2d(1, x, sizeX, sizeY);
	}
	
	for (int k = 0; k < iter; ++k)
    {
		Shader shader("lin_solve2d_xy");
		setShader(shader);
    	shader.setTexture("x", 0, y->getTexture(), false, true);
		shader.setTexture("x0", 1, y0->getTexture(), false, true);
		shader.setImmediate("a", a);
		shader.setImmediate("cRecip", cRecip);
    	y->postprocess(shader);
		
        set_bnd2d(2, y, sizeX, sizeY);
    }
}

static void diffuse2d(const int b, Surface * x, const Surface * x0, const float diff, const float dt, const int iter, const int sizeX, const int sizeY, const float voxelSize)
{
	const float a = dt * diff / voxelSize;
	lin_solve2d(b, x, x0, a, 1 + 4 * a, iter, sizeX, sizeY);
}

static void diffuse2d_xy(Surface * x, const Surface * x0, Surface * y, const Surface * y0, const float diff, const float dt, const int iter, const int sizeX, const int sizeY, const float voxelSize)
{
	const float a = dt * diff / voxelSize;
	lin_solve2d_xy(x, x0, y, y0, a, 1 + 4 * a, iter, sizeX, sizeY);
}

static void project2d(
	Surface * velocX,
	Surface * velocY,
	Surface * p,
	Surface * div, const int iter, const int sizeX, const int sizeY)
{
	getOrCreateShader("project2d_div",
		R"SHADER(
			return
				-0.25f *
					(
						+ (+ samp(velocX, +1,  0) - samp(velocX, -1,  0))
						+ (+ samp(velocY,  0, +1) - samp(velocY,  0, -1))
					);
		)SHADER",
		R"SHADER(
			uniform sampler2D velocX;
			uniform sampler2D velocY;
		)SHADER");
	
	pushSurface(div);
	{
		Shader shader("project2d_div");
		setShader(shader);
		shader.setTexture("velocX", 0, velocX->getTexture(), false, true);
		shader.setTexture("velocY", 1, velocY->getTexture(), false, true);
		drawRect(0, 0, div->getWidth(), div->getHeight());
		clearShader();
	}
	popSurface();
	
    set_bnd2d(0, div, sizeX, sizeY);
	
	p->clear();
	lin_solve2d(0, p, div, 1, 4, iter, sizeX, sizeY);
	
	getOrCreateShader("project2d_veloc_x",
		R"SHADER(
			return - ( samp(p, +1, 0) - samp(p, -1, 0) );
		)SHADER",
		"uniform sampler2D p;");
	
	getOrCreateShader("project2d_veloc_y",
		R"SHADER(
			return - ( samp(p, 0, +1) - samp(p, 0, -1) );
		)SHADER",
		"uniform sampler2D p;");
	
	pushSurface(velocX);
	pushBlend(BLEND_ADD);
	{
		Shader shader("project2d_veloc_x");
		setShader(shader);
		shader.setTexture("p", 0, p->getTexture(), false, true);
		drawRect(0, 0, velocX->getWidth(), velocX->getHeight());
		clearShader();
	}
	popBlend();
	popSurface();

	pushSurface(velocY);
	pushBlend(BLEND_ADD);
	{
		Shader shader("project2d_veloc_y");
		setShader(shader);
		shader.setTexture("p", 0, p->getTexture(), false, true);
		drawRect(0, 0, velocY->getWidth(), velocY->getHeight());
		clearShader();
	}
	popBlend();
	popSurface();

    set_bnd2d(1, velocX, sizeX, sizeY);
    set_bnd2d(2, velocY, sizeY, sizeY);
}

static void advect2d(const int b, Surface * d, const Surface * d0, const Surface * velocX, const Surface * velocY, const float in_dt, const int sizeX, const int sizeY, const float voxelSize)
{
	const float dt = in_dt / voxelSize;
	
    getOrCreateShader("advect2d",
		R"SHADER(
			float tmp1 = dt * samp(velocX, 0, 0);
			float tmp2 = dt * samp(velocY, 0, 0);
			
			return samp_filter(d0, - tmp1, - tmp2);
		)SHADER",
		R"SHADER(
			uniform sampler2D velocX;
			uniform sampler2D velocY;
			uniform sampler2D d0;
			uniform float dt;
		)SHADER");
	
    pushSurface(d);
    pushBlend(BLEND_OPAQUE);
    {
		Shader shader("advect2d");
		setShader(shader);
		shader.setTexture("velocX", 0, velocX->getTexture(), false, true);
		shader.setTexture("velocY", 1, velocY->getTexture(), false, true);
		shader.setTexture("d0", 2, d0->getTexture(), true, true);
		shader.setImmediate("dt", dt);
		drawRect(0, 0, d->getWidth(), d->getHeight());
		clearShader();
	}
	popBlend();
    popSurface();
	
    set_bnd2d(b, d, sizeX, sizeY);
}

//

void FluidCube2dGpu::addDensity(const int x, const int y, const float amount)
{
	if (x < 0 || x >= sizeX ||
		y < 0 || y >= sizeY)
	{
		return;
	}
	
	pushSurface(&density);
	pushBlend(BLEND_ADD);
	{
		setColorClamp(false);
		gxBegin(GX_POINTS);
		{
			gxColor4f(amount, 0, 0, 1);
			gxVertex2f(x, y);
		}
		gxEnd();
		setColorClamp(true);
	}
	popBlend();
	popSurface();
}

void FluidCube2dGpu::addVelocity(const int x, const int y, const float amountX, const float amountY)
{
	if (x < 0 || x >= sizeX ||
		y < 0 || y >= sizeY)
	{
		return;
	}
	
	pushSurface(&Vx);
	pushBlend(BLEND_ADD);
	{
		setColorClamp(false);
		gxBegin(GX_POINTS);
		{
			gxColor4f(amountX, 0, 0, 1);
			gxVertex2f(x, y);
		}
		gxEnd();
		setColorClamp(true);
	}
	popBlend();
	popSurface();
	
	pushSurface(&Vy);
	pushBlend(BLEND_ADD);
	{
		setColorClamp(false);
		gxBegin(GX_POINTS);
		{
			gxColor4f(amountY, 0, 0, 1);
			gxVertex2f(x, y);
		}
		gxEnd();
		setColorClamp(true);
	}
	popBlend();
	popSurface();
}

void FluidCube2dGpu::step()
{
	pushBlend(BLEND_OPAQUE);
	{
		diffuse2d_xy(&Vx0, &Vx, &Vy0, &Vy, visc, dt, iter, sizeX, sizeY, voxelSize);
		
		project2d(&Vx0, &Vy0, &Vx, &Vy, iter, sizeX, sizeY);
	
		advect2d(1, &Vx, &Vx0, &Vx0, &Vy0, dt, sizeX, sizeY, voxelSize);
		advect2d(2, &Vy, &Vy0, &Vx0, &Vy0, dt, sizeX, sizeY, voxelSize);
		
		project2d(&Vx, &Vy, &Vx0, &Vy0, iter, sizeX, sizeY);
	
		diffuse2d(0, &s, &density, diff, dt, iter, sizeX, sizeY, voxelSize);
		
		advect2d(0, &density, &s, &Vx, &Vy, dt, sizeX, sizeY, voxelSize);
	}
	popBlend();
}

//

FluidCube2dGpu * createFluidCube2dGpu(const int sizeX, const int sizeY, const float diffusion, const float viscosity, const float dt)
{
	FluidCube2dGpu * cube = new FluidCube2dGpu();

	cube->sizeX = sizeX;
	cube->sizeY = sizeY;
	cube->dt = dt;
	cube->diff = diffusion;
	cube->visc = viscosity;

	// initialize surfaces
	
	SurfaceProperties surfaceProperties;
	surfaceProperties.dimensions.init(sizeX, sizeY);
	surfaceProperties.colorTarget.init(SURFACE_R16F, true);
	
	cube->s.init(surfaceProperties);
	cube->density.init(surfaceProperties);

	cube->Vx.init(surfaceProperties);
	cube->Vy.init(surfaceProperties);

	cube->Vx0.init(surfaceProperties);
	cube->Vy0.init(surfaceProperties);
	
	// clear surfaces
	
	for (int i = 0; i < 2; ++i)
	{
		cube->s.clear();
		cube->density.clear();
		
		cube->Vx.clear();
		cube->Vy.clear();
		cube->Vx0.clear();
		cube->Vy0.clear();
		
		//
		
		cube->s.swapBuffers();
		cube->density.swapBuffers();
		
		cube->Vx.swapBuffers();
		cube->Vy.swapBuffers();
		cube->Vx0.swapBuffers();
		cube->Vy0.swapBuffers();
	}

	return cube;
}
