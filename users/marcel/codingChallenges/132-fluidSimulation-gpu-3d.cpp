#include "framework.h"
#include "Timer.h"
#include <vector>

/*
https://mikeash.com/pyblog/fluid-simulation-for-dummies.html
Coding Challenge #132: Fluid Simulation
https://www.youtube.com/watch?v=alhpH6ECFvQ
*/

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

#include <GL/glew.h>

#if defined(DEBUG)
	#include <SDL2/SDL_opengl.h> // so we can call glFinish to measure GPU time
#endif

#define TODO 0

#define SCALE 1

#define IX_3D(x, y) ((x) + (y) * N)

//

struct Texture3dProperties
{
	struct
	{
		int sx = 0;
		int sy = 0;
		int sz = 0;
		
		void init(const int in_sx, const int in_sy, const int in_sz)
		{
			sx = in_sx;
			sy = in_sy;
			sz = in_sz;
		}
	} dimensions;
};

struct Texture3d
{
	GLuint m_colorTexture = 0;
	
	int m_size[3] = { };
	
	bool init(const int sx, const int sy, const int sz)
	{
		Texture3dProperties properties;
		properties.dimensions.init(sx, sy, sz);
		
		return init(properties);
	}
	
	bool init(const Texture3dProperties & properties)
	{
		bool result = true;
		
		m_size[0] = properties.dimensions.sx;
		m_size[1] = properties.dimensions.sy;
		m_size[2] = properties.dimensions.sz;
		
		// allocate storage
		
		fassert(m_colorTexture == 0);
		glGenTextures(1, &m_colorTexture);
		result &= m_colorTexture != 0;
		checkErrorGL();
		
		glBindTexture(GL_TEXTURE_3D, m_colorTexture);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);
		checkErrorGL();
		
		GLenum glFormat = GL_R16F;
		
		glTexStorage3D(
			GL_TEXTURE_3D, 1, glFormat,
			properties.dimensions.sx,
			properties.dimensions.sy,
			properties.dimensions.sz);
		checkErrorGL();

		// set filtering
		
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		checkErrorGL();
	
		if (!result)
		{
			logError("failed to init 3d texture. calling destruct()");
			
			destruct();
		}
		
		return result;
	}
	
	void destruct()
	{
		if (m_colorTexture != 0)
		{
			glDeleteTextures(1, &m_colorTexture);
			m_colorTexture = 0;
		}
	}
	
	GxTextureId getTexture() const
	{
		return m_colorTexture;
	}
	
	int getWidth() const
	{
		return m_size[0];
	}
	
	int getHeight() const
	{
		return m_size[1];
	}
	
	int getDepth() const
	{
		return m_size[2];
	}
	
	void clear()
	{
		fassert(false);
		// todo : implement clear
	}
};

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

static void getOrCreateShader(const char * name, const char * code, const char * globals, BLEND_MODE blendMode)
{
	// don't do anything if the shader already exists
	
	if (s_createdShaders.count(name) == 0)
	{
		// remember we processed this shader
		
		s_createdShaders.insert(name);
		
		// define the compute shader, using a template and the code and globals passed into this function
		
	// todo : define output texture
	// todo : apply blend mode
	
		const char * cs_template =
			R"SHADER(
				include engine/ShaderCS.txt

				%s
		
				shader_in vec2 v_texcoord;

				#define samp(in_s, in_x, in_y, in_z) textureOffset(in_s, v_texcoord, ivec2(in_x, in_y)).x
		
				float samp_filter(sampler3D s, float x, float y, float z)
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
		
		char cs[1024];
		sprintf_s(cs, sizeof(cs), cs_template, globals, code);
		
		// register shader sources with framework
		
		char cs_name[64];
		sprintf_s(cs_name, sizeof(cs_name), "%s.cs", name);
		shaderSource(cs_name, cs);
		
		// construct the shader, so we can catch any errors here
		// note this step is optional and just here for convenience
		ComputeShader shader(cs_name);
		checkErrorGL();
	}
}

// -----

static void set_bnd3d(const int b, Texture3d * in_x, const int N)
{
#if TODO
	float * x = xfer_begin(in_x, 0);
	{
		// todo
	}
	xfer_end(in_x, 0);
#endif
}

static void lin_solve3d(const int b, Texture3d * x, const Texture3d * x0, const float a, const float c, const int iter, const int N)
{
    float cRecip = 1.f / c;

	getOrCreateShader("lin_solve3d",
		R"SHADER(
			return
				(
					samp(x0, 0, 0, 0)
					+ a *
						(
							+samp(x, +1,  0,  0)
							+samp(x, -1,  0,  0)
							+samp(x,  0, +1,  0)
							+samp(x,  0, -1,  0)
							+samp(x,  0,  0, +1)
							+samp(x,  0,  0, -1)
						)
				) * cRecip;
		)SHADER",
		"uniform sampler3D x; uniform sampler3D x0; uniform float a; uniform float cRecip;",
		BLEND_OPAQUE);
	
    for (int k = 0; k < iter; ++k)
    {
		ComputeShader shader("lin_solve3d");
		shader.setTexture("x", 0, x->getTexture(), false, true);
		shader.setTexture("x0", 1, x0->getTexture(), false, true);
		shader.setTextureRw("output", 2, x->getTexture(), GL_R16F, false);
		shader.setImmediate("a", a);
		shader.setImmediate("cRecip", cRecip);
		shader.dispatch(N, N, N);
		
        set_bnd3d(b, x, N);
    }
}

static void lin_solve3d_xyz(
	Texture3d * x, const Texture3d * x0,
	Texture3d * y, const Texture3d * y0,
	Texture3d * z, const Texture3d * z0,
	const float a, const float c, const int iter, const int N)
{
    float cRecip = 1.f / c;

	getOrCreateShader("lin_solve3d_xyz",
		R"SHADER(
			return
				(
					samp(x0, 0, 0, 0)
					+ a *
						(
							+samp(x, +1,  0,  0)
							+samp(x, -1,  0,  0)
							+samp(x,  0, +1,  0)
							+samp(x,  0, -1,  0)
							+samp(x,  0,  0, +1)
							+samp(x,  0,  0, -1)
						)
				) * cRecip;
		)SHADER",
		"uniform sampler3D x; uniform sampler3D x0; uniform float a; uniform float cRecip;",
		BLEND_OPAQUE);
	
	for (int k = 0; k < iter; ++k)
    {
		ComputeShader shader("lin_solve3d_xyz");
		shader.setTexture("x", 0, x->getTexture(), false, true);
		shader.setTexture("x0", 1, x0->getTexture(), false, true);
		shader.setTexture("output", 2, x->getTexture(), GL_R16F, false);
		shader.setImmediate("a", a);
		shader.setImmediate("cRecip", cRecip);
		shader.dispatch(N, N, N);
		
        set_bnd3d(1, x, N);
	}
	
	for (int k = 0; k < iter; ++k)
    {
		ComputeShader shader("lin_solve3d_xyz");
    	shader.setTexture("x", 0, y->getTexture(), false, true);
		shader.setTexture("x0", 1, y0->getTexture(), false, true);
		shader.setTexture("output", 2, y->getTexture(), GL_R16F, false);
		shader.setImmediate("a", a);
		shader.setImmediate("cRecip", cRecip);
		shader.dispatch(N, N, N);
		
        set_bnd3d(2, y, N);
    }
	
	for (int k = 0; k < iter; ++k)
    {
		ComputeShader shader("lin_solve3d_xyz");
    	shader.setTexture("x", 0, z->getTexture(), false, true);
		shader.setTexture("x0", 1, z0->getTexture(), false, true);
		shader.setTexture("output", 2, z->getTexture(), GL_R16F, false);
		shader.setImmediate("a", a);
		shader.setImmediate("cRecip", cRecip);
		shader.dispatch(N, N, N);
		
        set_bnd3d(3, y, N);
    }
}

static void diffuse3d(const int b, Texture3d * x, const Texture3d * x0, const float diff, const float dt, const int iter, const int N)
{
	const float a = dt * diff * (N - 2) * (N - 2);
	lin_solve3d(b, x, x0, a, 1 + 6 * a, iter, N);
}

static void diffuse3d_xyz(
	Texture3d * x, const Texture3d * x0,
	Texture3d * y, const Texture3d * y0,
	Texture3d * z, const Texture3d * z0,
	const float diff, const float dt, const int iter, const int N)
{
	const float a = dt * diff * (N - 2) * (N - 2);
	lin_solve3d_xyz(x, x0, y, y0, z, z0, a, 1 + 6 * a, iter, N);
}

static void project3d(
	Texture3d * velocX,
	Texture3d * velocY,
	Texture3d * velocZ,
	Texture3d * p,
	Texture3d * div, const int iter, const int N)
{
	getOrCreateShader("project3d_div",
		R"SHADER(
			return
				-0.25f *
					(
						+ (+ samp(velocX, +1,  0,  0) - samp(velocX, -1,  0,  0))
						+ (+ samp(velocY,  0, +1,  0) - samp(velocY,  0, -1,  0))
						+ (+ samp(velocY,  0,  0, +1) - samp(velocY,  0,  0, -1))
					);
		)SHADER",
		"uniform sampler3D velocX; uniform sampler3D velocY; uniform sampler3D velocZ;",
		BLEND_OPAQUE);
	
	{
		ComputeShader shader("project3d_div");
		setShader(shader);
		shader.setTexture("velocX", 0, velocX->getTexture(), false, true);
		shader.setTexture("velocY", 1, velocY->getTexture(), false, true);
		shader.setTexture("velocZ", 2, velocY->getTexture(), false, true);
		shader.setTextureRw("output", 3, div->getTexture(), GL_R16F, false);
		shader.dispatch(div->getWidth(), div->getHeight(), div->getDepth());
		clearShader();
	}
	
    set_bnd3d(0, div, N);
	
	p->clear();
	lin_solve3d(0, p, div, 1, 6, iter, N);
	
	getOrCreateShader("project3d_veloc_x",
		R"SHADER(
			return - ( samp(p, +1, 0, 0) - samp(p, -1, 0, 0) );
		)SHADER",
		"uniform sample3D p;",
		BLEND_ADD);
	
	getOrCreateShader("project3d_veloc_y",
		R"SHADER(
			return - ( samp(p, 0, +1, 0) - samp(p, 0, -1, 0) );
		)SHADER",
		"uniform sampler3D p;",
		BLEND_ADD);
	
	getOrCreateShader("project3d_veloc_z",
		R"SHADER(
			return - ( samp(p, 0, 0, +1) - samp(p, 0, 0, -1) );
		)SHADER",
		"uniform sampler3D p;",
		BLEND_ADD);
	
	{
		ComputeShader shader("project3d_veloc_x");
		setShader(shader);
		shader.setTexture("p", 0, p->getTexture(), false, true);
		shader.setTextureRw("output", 1, velocX->getTexture(), GL_R16F, false);
		shader.dispatch(velocX->getWidth(), velocX->getHeight(), velocX->getDepth());
		clearShader();
	}

	{
		ComputeShader shader("project3d_veloc_y");
		setShader(shader);
		shader.setTexture("p", 0, p->getTexture(), false, true);
		shader.setTextureRw("output", 1, velocY->getTexture(), GL_R16F, false);
		shader.dispatch(velocY->getWidth(), velocY->getHeight(), velocY->getDepth());
		clearShader();
	}
	
	{
		ComputeShader shader("project3d_veloc_z");
		setShader(shader);
		shader.setTexture("p", 0, p->getTexture(), false, true);
		shader.setTextureRw("output", 1, velocZ->getTexture(), GL_R16F, false);
		shader.dispatch(velocZ->getWidth(), velocZ->getHeight(), velocZ->getDepth());
		clearShader();
	}

    set_bnd3d(1, velocX, N);
    set_bnd3d(2, velocY, N);
}

static void advect3d(
	const int b,
	Texture3d * d, const Texture3d * d0,
	const Texture3d * velocX,
	const Texture3d * velocY,
	const Texture3d * velocZ,
	const float dt, const int N)
{
    const float dtx = dt * (N - 2);
    const float dty = dt * (N - 2);
    const float dtz = dt * (N - 2);
	
    getOrCreateShader("advect3d",
	R"SHADER(
		float tmp1 = dtx * samp(velocX, 0, 0, 0);
		float tmp2 = dty * samp(velocY, 0, 0, 0);
		float tmp3 = dtz * samp(velocZ, 0, 0, 0);
		
		return samp_filter(d0, - tmp1, - tmp2, - tmp3);
	)SHADER",
	"uniform sampler3D velocX; uniform sampler3D velocY; uniform sampler3D velocZ; uniform sampler3D d0; uniform float dtx; uniform float dty; uniform float dtz;",
	BLEND_OPAQUE);
	
    {
		ComputeShader shader("advect3d");
		setShader(shader);
		shader.setTexture("velocX", 0, velocX->getTexture(), false, true);
		shader.setTexture("velocY", 1, velocY->getTexture(), false, true);
		shader.setTexture("velocZ", 2, velocZ->getTexture(), false, true);
		shader.setTexture("d0", 3, d0->getTexture(), true, true);
		shader.setTextureRw("output", 4, d->getTexture(), GL_R16F, false);
		shader.setImmediate("dtx", dtx);
		shader.setImmediate("dty", dty);
		shader.setImmediate("dtz", dtz);
		shader.dispatch(d->getWidth(), d->getHeight(), d->getHeight());
		clearShader();
	}
	
    set_bnd3d(b, d, N);
}

static void advect3d_xyz(
	Texture3d * x, const Texture3d * x0,
	Texture3d * y, const Texture3d * y0,
	Texture3d * z, const Texture3d * z0,
	const Texture3d * velocX, const Texture3d * velocY, Texture3d * velocZ,
	const float dt, const int N)
{
#if TODO
    const float dtx = dt * (N - 2);
    const float dty = dt * (N - 2);
    const float dtz = dt * (N - 2);
	
    getOrCreateShader("advect3d_xyz",
	R"SHADER(
		float tmp1 = dtx * samp(velocX, 0, 0, 0);
		float tmp2 = dty * samp(velocY, 0, 0, 0);
		float tmp3 = dtz * samp(velocZ, 0, 0, 0);
		
		return samp_filter(d0, - tmp1, - tmp2, - tmp3);
	)SHADER",
	"uniform sampler3D velocX; uniform sampler3D velocY; uniform sampler3D velocZ; uniform sampler3D d0; uniform float dtx; uniform float dty; uniform float dtz;");
	
    pushSurface(d);
    pushBlend(BLEND_OPAQUE);
    {
		ComputeShader shader("advect3d");
		setShader(shader);
		shader.setTexture("velocX", 0, velocX->getTexture(), false, true);
		shader.setTexture("velocY", 1, velocY->getTexture(), false, true);
		shader.setTexture("velocZ", 2, velocZ->getTexture(), false, true);
		shader.setTexture("d0", 3, d0->getTexture(), true, true);
		shader.setImmediate("dtx", dtx);
		shader.setImmediate("dty", dty);
		shader.setImmediate("dtz", dtz);
		//drawRect(0, 0, d->getWidth(), d->getHeight());
		shader.dispatch(d->getWidth(), d->getHeight(), d->getHeight());
		clearShader();
	}
	popBlend();
    popSurface();
	
    set_bnd3d(b, d, N);
#else
	advect3d(1, x, x0, velocX, velocY, velocZ, dt, N);
	advect3d(2, y, y0, velocX, velocY, velocZ, dt, N);
	advect3d(3, z, z0, velocX, velocY, velocZ, dt, N);
#endif
}

struct FluidCube3d
{
	int size;

	float dt;
	float diff; // diffusion amount
	float visc; // viscosity

	Texture3d s;
	Texture3d density;
	
// todo : some considerable speedup could probably be had when combining Vx and Vy and Vx0 and Vy0
//        this will require changes to most shaders and functions though
	Texture3d Vx;
	Texture3d Vy;
	Texture3d Vz;
	
	Texture3d Vx0;
	Texture3d Vy0;
	Texture3d Vz0;

	void addDensity(const int x, const int y, const int z, const float amount)
	{
		if (x < 0 || x >= size ||
			y < 0 || y >= size ||
			z < 0 || z >= size)
		{
			return;
		}
		
	#if TODO
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
	#endif
	}
	
	void addVelocity(const int x, const int y, const int z, const float amountX, const float amountY, const float amountZ)
	{
		if (x < 0 || x >= size ||
			y < 0 || y >= size ||
			z < 0 || z >= size)
		{
			return;
		}
	
	#if TODO
		pushSurface(&Vx);
		pushBlend(BLEND_ADD);
		{
			setColorClamp(false);
			gxBegin(GX_POINTS);
			{
				gxColor4f(amountX, 0, 0, 1);
				gxVertex3f(x, y, z);
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
				gxVertex3f(x, y, z);
			}
			gxEnd();
			setColorClamp(true);
		}
		popBlend();
		popSurface();
		
		pushSurface(&Vz);
		pushBlend(BLEND_ADD);
		{
			setColorClamp(false);
			gxBegin(GX_POINTS);
			{
				gxColor4f(amountZ, 0, 0, 1);
				gxVertex3f(x, y, z);
			}
			gxEnd();
			setColorClamp(true);
		}
		popBlend();
		popSurface();
	#endif
	}

	void step()
	{
		pushBlend(BLEND_OPAQUE);
		{
			const int N = size;
		
			const int iter = 4;
			
			diffuse3d_xyz(&Vx0, &Vx, &Vy0, &Vy, &Vz0, &Vz, visc, dt, iter, N);
			
			project3d(&Vx0, &Vy0, &Vz0, &Vx, &Vy, iter, N);
		
			advect3d_xyz(
				&Vx, &Vx0,
				&Vy, &Vy0,
				&Vz, &Vz0,
				&Vx0, &Vy0, &Vz0, dt, N);
			
			project3d(&Vx, &Vy, &Vz, &Vx0, &Vy0, iter, N);
		
			diffuse3d(0, &s, &density, diff, dt, iter, N);
			
			advect3d(0, &density, &s, &Vx, &Vy, &Vz, dt, N);
		}
		popBlend();
	}
};

FluidCube3d * createFluidCube3d(const int size, const float diffusion, const float viscosity, const float dt)
{
	FluidCube3d * cube = new FluidCube3d();

	cube->size = size;
	cube->dt = dt;
	cube->diff = diffusion;
	cube->visc = viscosity;

	// initialize textures
	
	Texture3dProperties textureProperties;
	textureProperties.dimensions.init(size, size, size);
// todo : explicit format and swizzle
	//textureProperties.colorTarget.init(SURFACE_R16F, true);
	//textureProperties.colorTarget.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
	
#if 0 // attempt to use 8 bit backing for density field failed. but has a nice aesthetic, so keeping it around!
	SurfaceProperties surfaceProperties_d;
	surfaceProperties_d.dimensions.init(size, size);
	surfaceProperties_d.colorTarget.init(SURFACE_R8, true);
	surfaceProperties_d.colorTarget.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
#else
	Texture3dProperties textureProperties_d = textureProperties;
#endif
	
	cube->s.init(textureProperties_d);
	cube->density.init(textureProperties_d);

	cube->Vx.init(textureProperties);
	cube->Vy.init(textureProperties);
	cube->Vz.init(textureProperties);

	cube->Vx0.init(textureProperties);
	cube->Vy0.init(textureProperties);
	cube->Vz0.init(textureProperties);
	
	// clear textures
	
	cube->s.clear();
	cube->density.clear();
	
	cube->Vx.clear();
	cube->Vy.clear();
	cube->Vz.clear();
	cube->Vx0.clear();
	cube->Vy0.clear();
	cube->Vz0.clear();

	return cube;
}

int main(int argc, char * argv[])
{
	framework.allowHighDpi = false;
	
	if (!framework.init(900, 900))
		return -1;

	Texture3d texture3d;
	texture3d.init(64, 64, 64);
	
	FluidCube3d * cube = createFluidCube3d(64, 0.0001f, 0.0001f, 1.f / 30.f);
	
	mouse.showCursor(false);
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

	#if TODO
		cube->density.mulf(.99f, .99f, .99f);
	#endif
		
	#if TODO
	#if 1
		pushBlend(BLEND_ADD);
		pushSurface(&cube->density);
		{
			hqBegin(HQ_FILLED_CIRCLES);
			setColorf(.008f, 0.f, 0.f, 1.f / 100);
			for (int i = 0; i < 100; ++i)
				hqFillCircle(mouse.x / SCALE, mouse.y / SCALE, i * 60.f / 100);
			hqEnd();
		}
		popSurface();
		pushSurface(&cube->Vx);
		{
			hqBegin(HQ_FILLED_CIRCLES);
			setColorf(mouse.dx / 10.f, 0.f, 0.f, 1.f / 10);
			for (int i = 0; i < 10; ++i)
				hqFillCircle(mouse.x / SCALE, mouse.y / SCALE, i * 8.f / 10);
			hqEnd();
		}
		popSurface();
		pushSurface(&cube->Vy);
		{
			hqBegin(HQ_FILLED_CIRCLES);
			setColorf(mouse.dy / 10.f, 0.f, 0.f, 1.f / 10);
			for (int i = 0; i < 10; ++i)
				hqFillCircle(mouse.x / SCALE, mouse.y / SCALE, i * 8.f / 10);
			hqEnd();
		}
		popSurface();
		popBlend();
	#else
		for (int x = -4; x <= +4; ++x)
		{
			for (int y = -4; y <= +4; ++y)
			{
				cube->addDensity(mouse.x / SCALE + x, mouse.y / SCALE + y, .1f);
				//cube->addVelocity(mouse.x / SCALE + x, mouse.y / SCALE + y, mouse.dx / 100.f, mouse.dy / 100.f);
				cube->addVelocity(mouse.x / SCALE, mouse.y / SCALE, mouse.dx / 100.f, mouse.dy / 100.f);
			}
		}
	#endif
	#endif
		
	#if defined(DEBUG)
		const auto t1 = g_TimerRT.TimeUS_get();
	#endif
		
		for (int i = 0; i < 1; ++i)
		{
			cube->step();
		}
		
	#if defined(DEBUG)
		glFlush();
		glFinish();
		
		const auto t2 = g_TimerRT.TimeUS_get();
		
		printf("step duration: %gms\n", (t2 - t1) / 1000.f);
	#endif
		
		framework.beginDraw(0, 0, 255, 0);
		{
			gxScalef(SCALE, SCALE, 1);
			
			pushBlend(BLEND_OPAQUE);
			{
				if (keyboard.isDown(SDLK_s))
					gxSetTexture(cube->s.getTexture());
				else if (keyboard.isDown(SDLK_v))
				{
					static int n = -1;
					if (keyboard.wentDown(SDLK_v))
						n = (n + 1) % 4;
					if (n == 0)
						gxSetTexture(cube->Vx.getTexture());
					if (n == 1)
						gxSetTexture(cube->Vy.getTexture());
					if (n == 2)
						gxSetTexture(cube->Vx0.getTexture());
					if (n == 3)
						gxSetTexture(cube->Vy0.getTexture());
				}
				else
					gxSetTexture(cube->density.getTexture());
				setColorClamp(false);
				setColor(4000, 3000, 2000);
				drawRect(0, 0, cube->size, cube->size);
				setColorClamp(true);
				gxSetTexture(0);
			}
			popBlend();
		}
		framework.endDraw();
	}

	delete cube;
	cube = nullptr;

	framework.shutdown();
	
	return 0;
}
