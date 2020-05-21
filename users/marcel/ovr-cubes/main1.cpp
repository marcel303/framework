#include "opengl-ovr.h"
#include "ovr-framebuffer.h"

#include "Log.h"
#include "StringEx.h"

#include <VrApi.h>
#include <VrApi_Helpers.h>
#include <VrApi_Input.h>
#include <VrApi_SystemUtils.h>

#include <android/input.h>
#include <android/log.h>
#include <android/native_window_jni.h> // for native window JNI
#include <android/window.h> // for AWINDOW_FLAG_KEEP_SCREEN_ON

#include <assert.h>
#include <math.h>
#include <sys/prctl.h> // for prctl( PR_SET_NAME )
#include <time.h>
#include <unistd.h>

static const int CPU_LEVEL = 2;
static const int GPU_LEVEL = 3;
static const int NUM_MULTI_SAMPLES = 4;

/*
================================================================================

System Clock Time

================================================================================
*/

static double GetTimeInSeconds()
{
    // note : this time must match the time used internally by the VrApi. this is due to us having
    //        to simulate the time step since the last update until the predicted (by the vr system) display time

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec * 1e9 + now.tv_nsec) * 0.000000001;
}

/*
================================================================================

OpenGL-ES Utility Functions

================================================================================
*/

// todo : move OpenGL extensions into a separate file

static void checkErrorGL()
{
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR)
        LOG_ERR("GL error: %s", GlErrorString(error));
}

static void EglInitExtensions()
{
    const char * allExtensions = (const char*)glGetString(GL_EXTENSIONS);

    if (allExtensions != nullptr)
    {
        glExtensions.multi_view = strstr(allExtensions, "GL_OVR_multiview2") &&
            strstr(allExtensions, "GL_OVR_multiview_multisampled_render_to_texture");

        glExtensions.EXT_texture_border_clamp =
            strstr(allExtensions, "GL_EXT_texture_border_clamp") ||
            strstr(allExtensions, "GL_OES_texture_border_clamp");
    }
}

/*
================================================================================

ovrEgl

================================================================================
*/

struct ovrEgl
{
    EGLint MajorVersion = 0; // note : major version number returned by eglInitialize
    EGLint MinorVersion = 0; // note : minor version number returned by eglInitialize
    EGLDisplay Display = EGL_NO_DISPLAY;
    EGLConfig Config = 0;
    EGLSurface TinySurface = EGL_NO_SURFACE;
    EGLSurface MainSurface = EGL_NO_SURFACE;
    EGLContext Context = EGL_NO_CONTEXT;
};

static void ovrEgl_CreateContext(ovrEgl * egl, const ovrEgl * shareEgl)
{
    assert(egl->Display == EGL_NO_DISPLAY);

    egl->Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(egl->Display, &egl->MajorVersion, &egl->MinorVersion);

    // Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
    // flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
    // settings, and that is completely wasted for our warp target.

    const int MAX_CONFIGS = 1024;
    EGLConfig configs[MAX_CONFIGS];
    EGLint numConfigs = 0;

    if (eglGetConfigs(egl->Display, configs, MAX_CONFIGS, &numConfigs) == EGL_FALSE)
    {
        LOG_ERR("eglGetConfigs() failed: %s", EglErrorString(eglGetError()));
        return;
    }

    const EGLint configAttribs[] =
        {
            EGL_RED_SIZE,       8,
            EGL_GREEN_SIZE,     8,
            EGL_BLUE_SIZE,      8,
            EGL_ALPHA_SIZE,     8, // need alpha for the multi-pass timewarp compositor
            EGL_DEPTH_SIZE,     0,
            EGL_STENCIL_SIZE,   0,
            EGL_SAMPLES,        0,
            EGL_NONE
        };

    egl->Config = 0;

    for (int i = 0; i < numConfigs; i++)
    {
        EGLint value = 0;

        eglGetConfigAttrib(egl->Display, configs[i], EGL_RENDERABLE_TYPE, &value);
        if ((value & EGL_OPENGL_ES3_BIT_KHR) != EGL_OPENGL_ES3_BIT_KHR)
            continue;

        // The pbuffer config also needs to be compatible with normal window rendering
        // so it can share textures with the window context.
        eglGetConfigAttrib(egl->Display, configs[i], EGL_SURFACE_TYPE, &value);
        if ((value & (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) != (EGL_WINDOW_BIT | EGL_PBUFFER_BIT))
            continue;

        int j = 0;
        while (configAttribs[j] != EGL_NONE)
        {
            eglGetConfigAttrib(egl->Display, configs[i], configAttribs[j], &value);
            if (value != configAttribs[j + 1])
                break;
            j += 2;
        }

        if (configAttribs[j] == EGL_NONE)
        {
            egl->Config = configs[i];
            break;
        }
    }

    if (egl->Config == 0)
    {
        LOG_ERR("eglChooseConfig() failed: %s", EglErrorString(eglGetError()));
        return;
    }

    LOG_DBG("Context = eglCreateContext( Display, Config, EGL_NO_CONTEXT, contextAttribs )", 0);
    const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    egl->Context = eglCreateContext(
        egl->Display,
        egl->Config,
        (shareEgl != NULL) ? shareEgl->Context : EGL_NO_CONTEXT,
        contextAttribs);

    if (egl->Context == EGL_NO_CONTEXT)
    {
        LOG_ERR("eglCreateContext() failed: %s", EglErrorString(eglGetError()));
        return;
    }

    LOG_DBG("TinySurface = eglCreatePbufferSurface( Display, Config, surfaceAttribs )", 0);
    const EGLint surfaceAttribs[] = { EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE };
    egl->TinySurface = eglCreatePbufferSurface(egl->Display, egl->Config, surfaceAttribs);
    if (egl->TinySurface == EGL_NO_SURFACE)
    {
        LOG_ERR("eglCreatePbufferSurface() failed: %s", EglErrorString(eglGetError()));
        eglDestroyContext(egl->Display, egl->Context);
        egl->Context = EGL_NO_CONTEXT;
        return;
    }

    LOG_DBG("eglMakeCurrent( Display, TinySurface, TinySurface, Context )", 0);
    if (eglMakeCurrent(egl->Display, egl->TinySurface, egl->TinySurface, egl->Context) == EGL_FALSE)
    {
        LOG_ERR("eglMakeCurrent() failed: %s", EglErrorString(eglGetError()));
        eglDestroySurface(egl->Display, egl->TinySurface);
        eglDestroyContext(egl->Display, egl->Context);
        egl->Context = EGL_NO_CONTEXT;
        return;
    }
}

static void ovrEgl_DestroyContext(ovrEgl * egl)
{
    if (egl->Display != EGL_NO_DISPLAY)
    {
        LOG_ERR("- eglMakeCurrent( Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT )", 0);
        if (eglMakeCurrent(egl->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE)
            LOG_ERR("- eglMakeCurrent() failed: %s", EglErrorString(eglGetError()));
    }

    if (egl->Context != EGL_NO_CONTEXT)
    {
        LOG_ERR("- eglDestroyContext( Display, Context )", 0);
        if (eglDestroyContext(egl->Display, egl->Context) == EGL_FALSE)
            LOG_ERR("- eglDestroyContext() failed: %s", EglErrorString(eglGetError()));
        egl->Context = EGL_NO_CONTEXT;
    }

    if (egl->TinySurface != EGL_NO_SURFACE)
    {
        LOG_ERR("- eglDestroySurface( Display, TinySurface )", 0);
        if (eglDestroySurface(egl->Display, egl->TinySurface) == EGL_FALSE)
            LOG_ERR("- eglDestroySurface() failed: %s", EglErrorString(eglGetError()));
        egl->TinySurface = EGL_NO_SURFACE;
    }

    if (egl->Display != EGL_NO_DISPLAY)
    {
        LOG_ERR("- eglTerminate( Display )", 0);
        if (eglTerminate(egl->Display) == EGL_FALSE)
            LOG_ERR("- eglTerminate() failed: %s", EglErrorString(eglGetError()));
        egl->Display = EGL_NO_DISPLAY;
    }
}

/*
================================================================================

ovrGeometry

================================================================================
*/

struct ovrVertexAttribPointer
{
    GLint Index;
    GLint Size;
    GLenum Type;
    GLboolean Normalized;
    GLsizei Stride;
    const GLvoid * Pointer;
};

#define MAX_VERTEX_ATTRIB_POINTERS 3

struct ovrGeometry
{
    GLuint VertexBuffer = 0;
    GLuint IndexBuffer = 0;
    GLuint VertexArrayObject = 0;
    int VertexCount = 0;
    int IndexCount = 0;
    ovrVertexAttribPointer VertexAttribs[MAX_VERTEX_ATTRIB_POINTERS];

    ovrGeometry()
    {
        memset(VertexAttribs, 0, sizeof(VertexAttribs));
        for (int i = 0; i < MAX_VERTEX_ATTRIB_POINTERS; ++i)
            VertexAttribs[i].Index = -1;
    }
};

enum VertexAttributeLocation
{
    VERTEX_ATTRIBUTE_LOCATION_POSITION,
    VERTEX_ATTRIBUTE_LOCATION_COLOR,
    VERTEX_ATTRIBUTE_LOCATION_UV,
    VERTEX_ATTRIBUTE_LOCATION_TRANSFORM
};

struct ovrVertexAttribute
{
    enum VertexAttributeLocation location;
    const char* name;
};

static const ovrVertexAttribute ProgramVertexAttributes[] =
{
    { VERTEX_ATTRIBUTE_LOCATION_POSITION,   "vertexPosition"    },
    { VERTEX_ATTRIBUTE_LOCATION_COLOR,      "vertexColor"       },
    { VERTEX_ATTRIBUTE_LOCATION_UV,         "vertexUv"          },
    { VERTEX_ATTRIBUTE_LOCATION_TRANSFORM,  "vertexTransform"   }
};

static void ovrGeometry_CreateCube(ovrGeometry * geometry)
{
    struct ovrCubeVertices
    {
        signed char positions[8][4];
        unsigned char colors[8][4];
    };

    static const ovrCubeVertices cubeVertices =
    {
        // positions
        {
            {-127, +127, -127, +127},
            {+127, +127, -127, +127},
            {+127, +127, +127, +127},
            {-127, +127, +127, +127}, // top
            {-127, -127, -127, +127},
            {-127, -127, +127, +127},
            {+127, -127, +127, +127},
            {+127, -127, -127, +127} // bottom
        },
        // colors
        {
            {255, 0, 255, 255},
            {0, 255, 0, 255},
            {0, 0, 255, 255},
            {255, 0, 0, 255},
            {0, 0, 255, 255},
            {0, 255, 0, 255},
            {255, 0, 255, 255},
            {255, 0, 0, 255}
        },
    };

    static const unsigned short cubeIndices[36] =
    {
        0, 2, 1, 2, 0, 3, // top
        4, 6, 5, 6, 4, 7, // bottom
        2, 6, 7, 7, 1, 2, // right
        0, 4, 5, 5, 3, 0, // left
        3, 5, 6, 6, 2, 3, // front
        0, 1, 7, 7, 4, 0 // back
    };

    geometry->VertexCount = 8;
    geometry->IndexCount = 36;

    geometry->VertexAttribs[0].Index = VERTEX_ATTRIBUTE_LOCATION_POSITION;
    geometry->VertexAttribs[0].Size = 4;
    geometry->VertexAttribs[0].Type = GL_BYTE;
    geometry->VertexAttribs[0].Normalized = true;
    geometry->VertexAttribs[0].Stride = sizeof(cubeVertices.positions[0]);
    geometry->VertexAttribs[0].Pointer = (const GLvoid*)offsetof(ovrCubeVertices, positions);

    geometry->VertexAttribs[1].Index = VERTEX_ATTRIBUTE_LOCATION_COLOR;
    geometry->VertexAttribs[1].Size = 4;
    geometry->VertexAttribs[1].Type = GL_UNSIGNED_BYTE;
    geometry->VertexAttribs[1].Normalized = true;
    geometry->VertexAttribs[1].Stride = sizeof(cubeVertices.colors[0]);
    geometry->VertexAttribs[1].Pointer = (const GLvoid*)offsetof(ovrCubeVertices, colors);

    glGenBuffers(1, &geometry->VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, geometry->VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    checkErrorGL();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &geometry->IndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
    checkErrorGL();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

static void ovrGeometry_Destroy(ovrGeometry * geometry)
{
    glDeleteBuffers(1, &geometry->IndexBuffer);
    glDeleteBuffers(1, &geometry->VertexBuffer);
    checkErrorGL();

    *geometry = ovrGeometry();
}

static void ovrGeometry_CreateVAO(ovrGeometry * geometry)
{
    glGenVertexArrays(1, &geometry->VertexArrayObject);
    glBindVertexArray(geometry->VertexArrayObject);
    checkErrorGL();

    glBindBuffer(GL_ARRAY_BUFFER, geometry->VertexBuffer);
    checkErrorGL();

    for (int i = 0; i < MAX_VERTEX_ATTRIB_POINTERS; i++)
    {
        if (geometry->VertexAttribs[i].Index != -1)
        {
            glEnableVertexAttribArray(geometry->VertexAttribs[i].Index);
            glVertexAttribPointer(
                geometry->VertexAttribs[i].Index,
                geometry->VertexAttribs[i].Size,
                geometry->VertexAttribs[i].Type,
                geometry->VertexAttribs[i].Normalized,
                geometry->VertexAttribs[i].Stride,
                geometry->VertexAttribs[i].Pointer);
            checkErrorGL();
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->IndexBuffer);
    checkErrorGL();

    glBindVertexArray(0);
    checkErrorGL();
}

static void ovrGeometry_DestroyVAO(ovrGeometry * geometry)
{
    glDeleteVertexArrays(1, &geometry->VertexArrayObject);
    checkErrorGL();
}

/*
================================================================================

ovrProgram

================================================================================
*/

#define MAX_PROGRAM_UNIFORMS 8
#define MAX_PROGRAM_TEXTURES 8

struct ovrProgram
{
    GLuint Program = 0;
    GLuint VertexShader = 0;
    GLuint FragmentShader = 0;

    // These will be -1 if not used by the program.
    GLint UniformLocation[MAX_PROGRAM_UNIFORMS] = { }; // ProgramUniforms[].name
    GLint UniformBinding[MAX_PROGRAM_UNIFORMS] = { }; // ProgramUniforms[].name
    GLint Textures[MAX_PROGRAM_TEXTURES] = { }; // Texture%i
};

enum UNIFORM_INDEX
{
    UNIFORM_MODEL_MATRIX,
    UNIFORM_VIEW_ID,
    UNIFORM_SCENE_MATRICES,
};

enum UNIFORM_TYPE
{
    UNIFORM_TYPE_VECTOR4,
    UNIFORM_TYPE_MATRIX4X4,
    UNIFORM_TYPE_INT,
    UNIFORM_TYPE_BUFFER,
};

struct ovrUniform
{
    UNIFORM_INDEX index;
    UNIFORM_TYPE type;
    const char * name;
};

static const ovrUniform ProgramUniforms[] =
{
    { UNIFORM_MODEL_MATRIX,     UNIFORM_TYPE_MATRIX4X4, "ModelMatrix"   },
    { UNIFORM_VIEW_ID,          UNIFORM_TYPE_INT,       "ViewID"        },
    { UNIFORM_SCENE_MATRICES,   UNIFORM_TYPE_BUFFER,    "SceneMatrices" },
};

static const char * programVersion = "#version 300 es\n";

static bool ovrProgram_Create(
    ovrProgram * program,
    const char * vertexSource,
    const char * fragmentSource,
    const bool useMultiview)
{
    GLint r;

    program->VertexShader = glCreateShader(GL_VERTEX_SHADER);

    const char * vertexSources[3] =
        {
            programVersion,
            (useMultiview) ? "#define DISABLE_MULTIVIEW 0\n" : "#define DISABLE_MULTIVIEW 1\n",
            vertexSource
        };

    glShaderSource(program->VertexShader, 3, vertexSources, 0);
    glCompileShader(program->VertexShader);
    checkErrorGL();

    glGetShaderiv(program->VertexShader, GL_COMPILE_STATUS, &r);
    if (r == GL_FALSE)
    {
        GLchar msg[4096];
        glGetShaderInfoLog(program->VertexShader, sizeof(msg), 0, msg);
        LOG_ERR("%s\n%s\n", vertexSource, msg);
        return false;
    }

    const char * fragmentSources[2] = { programVersion, fragmentSource };
    program->FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(program->FragmentShader, 2, fragmentSources, 0);
    glCompileShader(program->FragmentShader);
    checkErrorGL();

    glGetShaderiv(program->FragmentShader, GL_COMPILE_STATUS, &r);
    if (r == GL_FALSE)
    {
        GLchar msg[4096];
        glGetShaderInfoLog(program->FragmentShader, sizeof(msg), 0, msg);
        LOG_ERR("%s\n%s\n", fragmentSource, msg);
        return false;
    }

    program->Program = glCreateProgram();

    glAttachShader(program->Program, program->VertexShader);
    checkErrorGL();
    glAttachShader(program->Program, program->FragmentShader);
    checkErrorGL();

    // Bind the vertex attribute locations.
    for (int i = 0; i < (int)(sizeof(ProgramVertexAttributes) / sizeof(ProgramVertexAttributes[0])); i++)
    {
        glBindAttribLocation(
            program->Program,
            ProgramVertexAttributes[i].location,
            ProgramVertexAttributes[i].name);
        checkErrorGL();
    }

    glLinkProgram(program->Program);
    checkErrorGL();

    glGetProgramiv(program->Program, GL_LINK_STATUS, &r);
    if (r == GL_FALSE)
    {
        GLchar msg[4096];
        glGetProgramInfoLog(program->Program, sizeof(msg), 0, msg);
        LOG_ERR("Linking program failed: %s\n", msg);
        return false;
    }

    int numBufferBindings = 0;

    // Get the uniform locations.
    memset(program->UniformLocation, -1, sizeof(program->UniformLocation));
    for (int i = 0; i < (int)(sizeof(ProgramUniforms) / sizeof(ProgramUniforms[0])); i++)
    {
        const int uniformIndex = ProgramUniforms[i].index;
        if (ProgramUniforms[i].type == UNIFORM_TYPE_BUFFER)
        {
            program->UniformLocation[uniformIndex] = glGetUniformBlockIndex(program->Program, ProgramUniforms[i].name);
            program->UniformBinding[uniformIndex] = numBufferBindings++;
            glUniformBlockBinding(
                program->Program,
                program->UniformLocation[uniformIndex],
                program->UniformBinding[uniformIndex]);
            checkErrorGL();
        }
        else
        {
            program->UniformLocation[uniformIndex] = glGetUniformLocation(program->Program, ProgramUniforms[i].name);
            program->UniformBinding[uniformIndex] = program->UniformLocation[uniformIndex];
            checkErrorGL();
        }
    }

    glUseProgram(program->Program);
    checkErrorGL();

    // Get the texture locations.
    for (int i = 0; i < MAX_PROGRAM_TEXTURES; i++)
    {
        char name[32];
        sprintf_s(name, sizeof(name), "Texture%i", i);
        program->Textures[i] = glGetUniformLocation(program->Program, name);
        if (program->Textures[i] != -1)
        {
            glUniform1i(program->Textures[i], i);
            checkErrorGL();
        }
    }

    glUseProgram(0);
    checkErrorGL();

    return true;
}

static void ovrProgram_Destroy(ovrProgram* program)
{
    if (program->Program != 0)
    {
        glDeleteProgram(program->Program);
        program->Program = 0;
    }

    if (program->VertexShader != 0)
    {
        glDeleteShader(program->VertexShader);
        program->VertexShader = 0;
    }

    if (program->FragmentShader != 0)
    {
        glDeleteShader(program->FragmentShader);
        program->FragmentShader = 0;
    }

    checkErrorGL();
}

static const char VERTEX_SHADER[] =
    "#ifndef DISABLE_MULTIVIEW\n"
    "	#define DISABLE_MULTIVIEW 0\n"
    "#endif\n"
    "#define NUM_VIEWS 2\n"
    "#if defined( GL_OVR_multiview2 ) && ! DISABLE_MULTIVIEW\n"
    "	#extension GL_OVR_multiview2 : enable\n"
    "	layout(num_views=NUM_VIEWS) in;\n"
    "	#define VIEW_ID gl_ViewID_OVR\n"
    "#else\n"
    "	uniform lowp int ViewID;\n"
    "	#define VIEW_ID ViewID\n"
    "#endif\n"
    "in vec3 vertexPosition;\n"
    "in vec4 vertexColor;\n"
    "in mat4 vertexTransform;\n"
    "uniform SceneMatrices\n"
    "{\n"
    "	uniform mat4 ViewMatrix[NUM_VIEWS];\n"
    "	uniform mat4 ProjectionMatrix[NUM_VIEWS];\n"
    "} sm;\n"
    "out vec4 fragmentColor;\n"
    "void main()\n"
    "{\n"
    "	gl_Position = sm.ProjectionMatrix[VIEW_ID] * ( sm.ViewMatrix[VIEW_ID] * ( vertexTransform * vec4( vertexPosition * 0.1, 1.0 ) ) );\n"
    "	fragmentColor = vertexColor;\n"
    "}\n";

static const char FRAGMENT_SHADER[] =
    "in lowp vec4 fragmentColor;\n"
    "out lowp vec4 outColor;\n"
    "void main()\n"
    "{\n"
    "	outColor = fragmentColor;\n"
    "}\n";

/*
================================================================================

ovrScene

================================================================================
*/

#define NUM_INSTANCES 2500
#define NUM_ROTATIONS 16

struct ovrScene
{
    bool CreatedScene = false;
    bool CreatedVAOs = false;
    unsigned int Random = 2;
    ovrProgram Program;
    ovrGeometry Cube;
    GLuint SceneMatrices = 0;
    GLuint InstanceTransformBuffer = 0;
    ovrVector3f Rotations[NUM_ROTATIONS];
    ovrVector3f CubePositions[NUM_INSTANCES];
    int CubeRotations[NUM_INSTANCES];
};

static bool ovrScene_IsCreated(ovrScene * scene)
{
    return scene->CreatedScene;
}

static void ovrScene_CreateVAOs(ovrScene * scene)
{
    if (!scene->CreatedVAOs)
    {
        ovrGeometry_CreateVAO(&scene->Cube);

        // Modify the VAO to use the instance transform attributes.
        glBindVertexArray(scene->Cube.VertexArrayObject);
        glBindBuffer(GL_ARRAY_BUFFER, scene->InstanceTransformBuffer);
        checkErrorGL();

        for (int i = 0; i < 4; i++)
        {
            glEnableVertexAttribArray(VERTEX_ATTRIBUTE_LOCATION_TRANSFORM + i);
            glVertexAttribPointer(
                VERTEX_ATTRIBUTE_LOCATION_TRANSFORM + i,
                4,
                GL_FLOAT,
                false,
                4 * 4 * sizeof(float),
                (void*)(i * 4 * sizeof(float)));
            glVertexAttribDivisor(VERTEX_ATTRIBUTE_LOCATION_TRANSFORM + i, 1);
            checkErrorGL();
        }

        glBindVertexArray(0);
        checkErrorGL();

        scene->CreatedVAOs = true;
    }
}

static void ovrScene_DestroyVAOs(ovrScene * scene)
{
    if (scene->CreatedVAOs)
    {
        ovrGeometry_DestroyVAO(&scene->Cube);

        scene->CreatedVAOs = false;
    }
}

// Returns a random float in the range [0, 1].
static float ovrScene_RandomFloat(ovrScene * scene)
{
    scene->Random = 1664525L * scene->Random + 1013904223L;
    union
    {
        float f;
        uint32_t i;
    } r;
    r.i = 0x3F800000 | (scene->Random & 0x007FFFFF);
    return r.f - 1.0f;
}

static void ovrScene_Create(ovrScene * scene, bool useMultiview)
{
    ovrProgram_Create(&scene->Program, VERTEX_SHADER, FRAGMENT_SHADER, useMultiview);
    ovrGeometry_CreateCube(&scene->Cube);

    // Create the instance transform attribute buffer.
    glGenBuffers(1, &scene->InstanceTransformBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, scene->InstanceTransformBuffer);
    glBufferData(GL_ARRAY_BUFFER, NUM_INSTANCES * 4 * 4 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    checkErrorGL();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Setup the scene matrices.
    glGenBuffers(1, &scene->SceneMatrices);
    glBindBuffer(GL_UNIFORM_BUFFER, scene->SceneMatrices);
    glBufferData(
        GL_UNIFORM_BUFFER,
        + 2 * sizeof(ovrMatrix4f) /* 2 view matrices */
        + 2 * sizeof(ovrMatrix4f) /* 2 projection matrices */,
        NULL,
        GL_STATIC_DRAW);
    checkErrorGL();
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Setup random rotations.
    for (int i = 0; i < NUM_ROTATIONS; i++)
    {
        scene->Rotations[i].x = ovrScene_RandomFloat(scene);
        scene->Rotations[i].y = ovrScene_RandomFloat(scene);
        scene->Rotations[i].z = ovrScene_RandomFloat(scene);
    }

    // Setup random cube positions and rotations.
    for (int i = 0; i < NUM_INSTANCES; i++)
    {
        float rx, ry, rz;
        for (;;)
        {

            rx = (ovrScene_RandomFloat(scene) - 0.5f) * (50.0f + sqrt(NUM_INSTANCES));
            ry = (ovrScene_RandomFloat(scene) - 0.5f) * (50.0f + sqrt(NUM_INSTANCES));
            rz = (ovrScene_RandomFloat(scene) - 0.5f) * (50.0f + sqrt(NUM_INSTANCES));
            // If too close to 0,0,0
            if (fabsf(rx) < 1.0f && fabsf(ry) < 1.0f && fabsf(rz) < 1.0f)
                continue;
            // Test for overlap with any of the existing cubes.
            bool overlap = false;
            for (int j = 0; j < i; j++)
            {
                if (fabsf(rx - scene->CubePositions[j].x) < 1.0f &&
                    fabsf(ry - scene->CubePositions[j].y) < 1.0f &&
                    fabsf(rz - scene->CubePositions[j].z) < 1.0f)
                    overlap = true;
            }
            if (!overlap)
                break;
        }

        rx *= 0.1f;
        ry *= 0.1f;
        rz *= 0.1f;

        // Insert into list sorted based on distance.
        int insert = 0;
        const float distSqr = rx * rx + ry * ry + rz * rz;
        for (int j = i; j > 0; j--)
        {
            const ovrVector3f* otherPos = &scene->CubePositions[j - 1];
            const float otherDistSqr =
                otherPos->x * otherPos->x + otherPos->y * otherPos->y + otherPos->z * otherPos->z;
            if (distSqr > otherDistSqr)
            {
                insert = j;
                break;
            }
            scene->CubePositions[j] = scene->CubePositions[j - 1];
            scene->CubeRotations[j] = scene->CubeRotations[j - 1];
        }

        scene->CubePositions[insert].x = rx;
        scene->CubePositions[insert].y = ry;
        scene->CubePositions[insert].z = rz;

        scene->CubeRotations[insert] = (int)(ovrScene_RandomFloat(scene) * (NUM_ROTATIONS - 0.1f));
    }

    scene->CreatedScene = true;

    ovrScene_CreateVAOs(scene);
}

static void ovrScene_Destroy(ovrScene * scene)
{
    ovrScene_DestroyVAOs(scene);

    ovrProgram_Destroy(&scene->Program);
    ovrGeometry_Destroy(&scene->Cube);

    glDeleteBuffers(1, &scene->InstanceTransformBuffer);
    glDeleteBuffers(1, &scene->SceneMatrices);
    checkErrorGL();

    scene->CreatedScene = false;
}

/*
================================================================================

ovrSimulation

================================================================================
*/

struct ovrSimulation
{
    ovrVector3f CurrentRotation = { 0.0f, 0.0f, 0.0f };
};

static void ovrSimulation_Advance(ovrSimulation * simulation, const double elapsedDisplayTime)
{
    // Update rotation.
    simulation->CurrentRotation.x = (float)(elapsedDisplayTime);
    simulation->CurrentRotation.y = (float)(elapsedDisplayTime);
    simulation->CurrentRotation.z = (float)(elapsedDisplayTime);
}

/*
================================================================================

ovrRenderer

================================================================================
*/

struct ovrRenderer
{
    ovrFramebuffer FrameBuffer[VRAPI_FRAME_LAYER_EYE_MAX];
    int NumBuffers = VRAPI_FRAME_LAYER_EYE_MAX;
};

static void ovrRenderer_Create(ovrRenderer * renderer, const ovrJava * java, const bool useMultiview)
{
    renderer->NumBuffers = useMultiview ? 1 : VRAPI_FRAME_LAYER_EYE_MAX;

    // Create the frame buffers.
    for (int eye = 0; eye < renderer->NumBuffers; eye++)
    {
        renderer->FrameBuffer[eye].init(
            useMultiview,
            GL_RGBA8,
            vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH),
            vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT),
            NUM_MULTI_SAMPLES);
    }
}

static void ovrRenderer_Destroy(ovrRenderer * renderer)
{
    for (int eye = 0; eye < renderer->NumBuffers; eye++)
        renderer->FrameBuffer[eye].shut();
}

static ovrLayerProjection2 ovrRenderer_RenderFrame(
    ovrRenderer * renderer,
    const ovrJava * java,
    const ovrScene * scene,
    const ovrSimulation * simulation,
    const ovrTracking2 * tracking,
    ovrMobile * ovr)
{
    ovrMatrix4f rotationMatrices[NUM_ROTATIONS];
    for (int i = 0; i < NUM_ROTATIONS; i++)
    {
        rotationMatrices[i] = ovrMatrix4f_CreateRotation(
            scene->Rotations[i].x * simulation->CurrentRotation.x,
            scene->Rotations[i].y * simulation->CurrentRotation.y,
            scene->Rotations[i].z * simulation->CurrentRotation.z);
    }

    // Update the instance transform attributes.
    glBindBuffer(GL_ARRAY_BUFFER, scene->InstanceTransformBuffer);
    ovrMatrix4f* cubeTransforms = (ovrMatrix4f*)glMapBufferRange(
           GL_ARRAY_BUFFER,
           0,
           NUM_INSTANCES * sizeof(ovrMatrix4f),
           GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    for (int i = 0; i < NUM_INSTANCES; i++)
    {
        const int index = scene->CubeRotations[i];

        // Write in order in case the mapped buffer lives on write-combined memory.
        cubeTransforms[i].M[0][0] = rotationMatrices[index].M[0][0];
        cubeTransforms[i].M[0][1] = rotationMatrices[index].M[0][1];
        cubeTransforms[i].M[0][2] = rotationMatrices[index].M[0][2];
        cubeTransforms[i].M[0][3] = rotationMatrices[index].M[0][3];

        cubeTransforms[i].M[1][0] = rotationMatrices[index].M[1][0];
        cubeTransforms[i].M[1][1] = rotationMatrices[index].M[1][1];
        cubeTransforms[i].M[1][2] = rotationMatrices[index].M[1][2];
        cubeTransforms[i].M[1][3] = rotationMatrices[index].M[1][3];

        cubeTransforms[i].M[2][0] = rotationMatrices[index].M[2][0];
        cubeTransforms[i].M[2][1] = rotationMatrices[index].M[2][1];
        cubeTransforms[i].M[2][2] = rotationMatrices[index].M[2][2];
        cubeTransforms[i].M[2][3] = rotationMatrices[index].M[2][3];

        cubeTransforms[i].M[3][0] = scene->CubePositions[i].x;
        cubeTransforms[i].M[3][1] = scene->CubePositions[i].y;
        cubeTransforms[i].M[3][2] = scene->CubePositions[i].z;
        cubeTransforms[i].M[3][3] = 1.0f;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    checkErrorGL();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    ovrTracking2 updatedTracking = *tracking;

    ovrMatrix4f eyeViewMatrixTransposed[2];
    eyeViewMatrixTransposed[0] = ovrMatrix4f_Transpose(&updatedTracking.Eye[0].ViewMatrix);
    eyeViewMatrixTransposed[1] = ovrMatrix4f_Transpose(&updatedTracking.Eye[1].ViewMatrix);

    ovrMatrix4f projectionMatrixTransposed[2];
    projectionMatrixTransposed[0] = ovrMatrix4f_Transpose(&updatedTracking.Eye[0].ProjectionMatrix);
    projectionMatrixTransposed[1] = ovrMatrix4f_Transpose(&updatedTracking.Eye[1].ProjectionMatrix);

    // Update the scene matrices.
    glBindBuffer(GL_UNIFORM_BUFFER, scene->SceneMatrices);
    ovrMatrix4f* sceneMatrices = (ovrMatrix4f*)glMapBufferRange(
           GL_UNIFORM_BUFFER,
           0,
           + 2 * sizeof(ovrMatrix4f) /* 2 view matrices */
           + 2 * sizeof(ovrMatrix4f) /* 2 projection matrices */,
           GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    if (sceneMatrices != NULL)
    {
        memcpy((char*)sceneMatrices, &eyeViewMatrixTransposed, 2 * sizeof(ovrMatrix4f));
        memcpy(
            (char*)sceneMatrices + 2 * sizeof(ovrMatrix4f),
            &projectionMatrixTransposed,
            2 * sizeof(ovrMatrix4f));
    }
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    checkErrorGL();
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
    layer.HeadPose = updatedTracking.HeadPose;
    for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++)
    {
        ovrFramebuffer * frameBuffer = &renderer->FrameBuffer[renderer->NumBuffers == 1 ? 0 : eye];
        layer.Textures[eye].ColorSwapChain = frameBuffer->ColorTextureSwapChain;
        layer.Textures[eye].SwapChainIndex = frameBuffer->TextureSwapChainIndex;
        layer.Textures[eye].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection(&updatedTracking.Eye[eye].ProjectionMatrix);
    }
    layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

    // Render the eye images.
    for (int eye = 0; eye < renderer->NumBuffers; eye++)
    {
        // NOTE: In the non-mv case, latency can be further reduced by updating the sensor
        // prediction for each eye (updates orientation, not position)
        ovrFramebuffer * frameBuffer = &renderer->FrameBuffer[eye];
        ovrFramebuffer_SetCurrent(frameBuffer);

        glEnable(GL_SCISSOR_TEST);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        checkErrorGL();

        glViewport(0, 0, frameBuffer->Width, frameBuffer->Height);
        glScissor(0, 0, frameBuffer->Width, frameBuffer->Height);
        checkErrorGL();

        glClearColor(0.125f, 0.0f, 0.125f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkErrorGL();

        glUseProgram(scene->Program.Program);
        {
            glBindBufferBase(
                    GL_UNIFORM_BUFFER,
                    scene->Program.UniformBinding[UNIFORM_SCENE_MATRICES],
                    scene->SceneMatrices);

            if (scene->Program.UniformLocation[UNIFORM_VIEW_ID] >= 0) // NOTE: will not be present when multiview path is enabled.
            {
                glUniform1i(scene->Program.UniformLocation[UNIFORM_VIEW_ID], eye);
            }

            glBindVertexArray(scene->Cube.VertexArrayObject);
            glDrawElementsInstanced(GL_TRIANGLES, scene->Cube.IndexCount, GL_UNSIGNED_SHORT, NULL, NUM_INSTANCES);
            checkErrorGL();
            glBindVertexArray(0);
        }
        glUseProgram(0);

		ovrFramebuffer_ClearBorder(frameBuffer);

        ovrFramebuffer_Resolve(frameBuffer);
        ovrFramebuffer_Advance(frameBuffer);
    }

    ovrFramebuffer_SetNone();

    return layer;
}

/*
================================================================================

ovrApp

================================================================================
*/

struct ovrApp
{
    ovrJava Java;
    ovrEgl Egl;
    ANativeWindow* NativeWindow = nullptr;
    bool Resumed = false;
    ovrMobile* Ovr = nullptr;
    ovrScene Scene;
    ovrSimulation Simulation;
    long long FrameIndex = 1;
    double DisplayTime = 0.0;
    int SwapInterval = 1;
    int CpuLevel = 2;
    int GpuLevel = 2;
    int MainThreadTid = 0;
    int RenderThreadTid = 0;
    bool BackButtonDownLastFrame = false;
    bool GamePadBackButtonDown = false;
    ovrRenderer Renderer;
    bool UseMultiview = true;

    ovrApp()
    {
        Java.Vm = nullptr;
        Java.Env = nullptr;
        Java.ActivityObject = nullptr;
    }
};

static void ovrApp_PushBlackFinal(ovrApp * app)
{
    const int frameFlags = VRAPI_FRAME_FLAG_FLUSH | VRAPI_FRAME_FLAG_FINAL;

    ovrLayerProjection2 layer = vrapi_DefaultLayerBlackProjection2();
    layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;

    const ovrLayerHeader2 * layers[] = { &layer.Header };

    ovrSubmitFrameDescription2 frameDesc = { };
    frameDesc.Flags = frameFlags;
    frameDesc.SwapInterval = 1;
    frameDesc.FrameIndex = app->FrameIndex;
    frameDesc.DisplayTime = app->DisplayTime;
    frameDesc.LayerCount = 1;
    frameDesc.Layers = layers;

    vrapi_SubmitFrame2(app->Ovr, &frameDesc);
}

static void ovrApp_HandleVrModeChanges(ovrApp * app)
{
    if (app->Resumed != false && app->NativeWindow != nullptr)
    {
        if (app->Ovr == nullptr)
        {
            ovrModeParms parms = vrapi_DefaultModeParms(&app->Java);

            // Must reset the FLAG_FULLSCREEN window flag when using a SurfaceView
            parms.Flags |= VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;

            parms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
            parms.Display = (size_t)app->Egl.Display;
            parms.WindowSurface = (size_t)app->NativeWindow;
            parms.ShareContext = (size_t)app->Egl.Context;

            LOG_DBG("- eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));

            LOG_DBG("- vrapi_EnterVrMode()", 0);
            app->Ovr = vrapi_EnterVrMode(&parms);

            LOG_DBG("- eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));

            // If entering VR mode failed then the ANativeWindow was not valid.
            if (app->Ovr == nullptr)
            {
                LOG_ERR("Invalid ANativeWindow!", 0);
                app->NativeWindow = nullptr;
            }

            // Set performance parameters once we have entered VR mode and have a valid ovrMobile.
            if (app->Ovr != nullptr)
            {
                vrapi_SetClockLevels(app->Ovr, app->CpuLevel, app->GpuLevel);
                LOG_DBG("- vrapi_SetClockLevels( %d, %d )", app->CpuLevel, app->GpuLevel);

                vrapi_SetPerfThread(app->Ovr, VRAPI_PERF_THREAD_TYPE_MAIN, app->MainThreadTid);
                LOG_DBG("- vrapi_SetPerfThread( MAIN, %d )", app->MainThreadTid);

                vrapi_SetPerfThread(app->Ovr, VRAPI_PERF_THREAD_TYPE_RENDERER, app->RenderThreadTid);
                LOG_DBG("- vrapi_SetPerfThread( RENDERER, %d )", app->RenderThreadTid);
            }
        }
    }
    else
    {
        if (app->Ovr != nullptr)
        {
            LOG_DBG("- eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));

            LOG_DBG("- vrapi_LeaveVrMode()", 0);
            vrapi_LeaveVrMode(app->Ovr);
            app->Ovr = nullptr;

            LOG_DBG("- eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));
        }
    }
}

static void ovrApp_HandleInput(ovrApp * app)
{
    bool backButtonDownThisFrame = false;

    for (int i = 0; true; i++)
    {
        ovrInputCapabilityHeader cap;
        const ovrResult result = vrapi_EnumerateInputDevices(app->Ovr, i, &cap);
        if (result < 0)
            break;

        if (cap.Type == ovrControllerType_Headset)
        {
            ovrInputStateHeadset headsetInputState;
            headsetInputState.Header.ControllerType = ovrControllerType_Headset;
            const ovrResult result = vrapi_GetCurrentInputState(app->Ovr, cap.DeviceID, &headsetInputState.Header);
            if (result == ovrSuccess)
                backButtonDownThisFrame |= headsetInputState.Buttons & ovrButton_Back;
        }
        else if (cap.Type == ovrControllerType_TrackedRemote)
        {
            ovrInputStateTrackedRemote trackedRemoteState;
            trackedRemoteState.Header.ControllerType = ovrControllerType_TrackedRemote;
            const ovrResult result = vrapi_GetCurrentInputState(app->Ovr, cap.DeviceID, &trackedRemoteState.Header);
            if (result == ovrSuccess)
            {
                backButtonDownThisFrame |= trackedRemoteState.Buttons & ovrButton_Back;
                backButtonDownThisFrame |= trackedRemoteState.Buttons & ovrButton_B;
                backButtonDownThisFrame |= trackedRemoteState.Buttons & ovrButton_Y;
            }
        }
        else if (cap.Type == ovrControllerType_Gamepad)
        {
            ovrInputStateGamepad gamepadState;
            gamepadState.Header.ControllerType = ovrControllerType_Gamepad;
            const ovrResult result = vrapi_GetCurrentInputState(app->Ovr, cap.DeviceID, &gamepadState.Header);
            if (result == ovrSuccess)
                backButtonDownThisFrame |= gamepadState.Buttons & ovrButton_Back;
        }
    }

    backButtonDownThisFrame |= app->GamePadBackButtonDown;

    bool backButtonDownLastFrame = app->BackButtonDownLastFrame;
    app->BackButtonDownLastFrame = backButtonDownThisFrame;

    if (backButtonDownLastFrame && !backButtonDownThisFrame)
    {
        LOG_DBG("back button short press", 0);
        LOG_DBG("- ovrApp_PushBlackFinal()", 0);
        ovrApp_PushBlackFinal(app);
        LOG_DBG("- vrapi_ShowSystemUI( confirmQuit )", 0);
        vrapi_ShowSystemUI(&app->Java, VRAPI_SYS_UI_CONFIRM_QUIT_MENU);
    }
}

static int ovrApp_HandleKeyEvent(ovrApp * app, const int keyCode, const int action)
{
    // Handle back button.
    if (keyCode == AKEYCODE_BACK || keyCode == AKEYCODE_BUTTON_B)
    {
        if (action == AKEY_EVENT_ACTION_DOWN)
            app->GamePadBackButtonDown = true;
        else if (action == AKEY_EVENT_ACTION_UP)
            app->GamePadBackButtonDown = false;
        return 1;
    }

    return 0;
}

static void ovrApp_HandleVrApiEvents(ovrApp * app)
{
    ovrEventDataBuffer eventDataBuffer;

    // Poll for VrApi events
    for (;;)
    {
        ovrEventHeader * eventHeader = (ovrEventHeader*)&eventDataBuffer;
        const ovrResult res = vrapi_PollEvent(eventHeader);
        if (res != ovrSuccess)
            break;

        switch (eventHeader->EventType)
        {
            case VRAPI_EVENT_DATA_LOST:
                LOG_DBG("vrapi_PollEvent: Received VRAPI_EVENT_DATA_LOST", 0);
                break;

            case VRAPI_EVENT_VISIBILITY_GAINED:
                LOG_DBG("vrapi_PollEvent: Received VRAPI_EVENT_VISIBILITY_GAINED", 0);
                break;

            case VRAPI_EVENT_VISIBILITY_LOST:
                LOG_DBG("vrapi_PollEvent: Received VRAPI_EVENT_VISIBILITY_LOST", 0);
                break;

            case VRAPI_EVENT_FOCUS_GAINED:
                // FOCUS_GAINED is sent when the application is in the foreground and has
                // input focus. This may be due to a system overlay relinquishing focus
                // back to the application.
                LOG_DBG("vrapi_PollEvent: Received VRAPI_EVENT_FOCUS_GAINED", 0);
                break;

            case VRAPI_EVENT_FOCUS_LOST:
                // FOCUS_LOST is sent when the application is no longer in the foreground and
                // therefore does not have input focus. This may be due to a system overlay taking
                // focus from the application. The application should take appropriate action when
                // this occurs.
                LOG_DBG("vrapi_PollEvent: Received VRAPI_EVENT_FOCUS_LOST", 0);
                break;

            default:
                LOG_DBG("vrapi_PollEvent: Unknown event", 0);
                break;
        }
    }
}

/*
================================================================================

Android Main (android_native_app_glue)

================================================================================
*/

extern "C"
{
    #include <android_native_app_glue.h>
    #include <android/native_activity.h>

    void android_main(android_app* app)
    {
        ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

        ovrJava java;
        java.Vm = app->activity->vm;
        java.Vm->AttachCurrentThread(&java.Env, nullptr);
        java.ActivityObject = app->activity->clazz;

    // todo : set thread name
        // Note that AttachCurrentThread will reset the thread name.
        //prctl(PR_SET_NAME, (long)"OVR::Main", 0, 0, 0);

        const ovrInitParms initParms = vrapi_DefaultInitParms(&java);
        const int32_t initResult = vrapi_Initialize(&initParms);

        if (initResult != VRAPI_INITIALIZE_SUCCESS)
        {
            // If intialization failed, vrapi_* function calls will not be available.
            exit(0);
        }

        ovrApp appState;
        appState.Java = java;

    // todo : the native activity example doesn't do this. why? and what does it do anyway?
        // This app will handle android gamepad events itself.
        vrapi_SetPropertyInt(&appState.Java, VRAPI_EAT_NATIVE_GAMEPAD_EVENTS, 0);

        ovrEgl_CreateContext(&appState.Egl, nullptr);

        EglInitExtensions();

        appState.UseMultiview &= (glExtensions.multi_view && vrapi_GetSystemPropertyInt(&appState.Java, VRAPI_SYS_PROP_MULTIVIEW_AVAILABLE));

        LOG_DBG("AppState UseMultiview : %d", appState.UseMultiview ? 1 : 0);

        appState.CpuLevel = CPU_LEVEL;
        appState.GpuLevel = GPU_LEVEL;
        appState.MainThreadTid = gettid();

        ovrRenderer_Create(&appState.Renderer, &java, appState.UseMultiview);

        const double startTime = GetTimeInSeconds();

        while (!app->destroyRequested)
        {
            int ident;
            int events;
            struct android_poll_source *source;
            //const int timeoutMilliseconds = (appState.Ovr == NULL && app->destroyRequested == 0) ? -1 : 0;
            const int timeoutMilliseconds = 0;

            while ((ident = ALooper_pollAll(timeoutMilliseconds, NULL, &events, (void **) &source)) >= 0)
            {
                if (ident == LOOPER_ID_MAIN)
                {
                    auto cmd = android_app_read_cmd(app);

                    android_app_pre_exec_cmd(app, cmd);

					switch (cmd)
					{
					case APP_CMD_START:
                        LOG_DBG("APP_CMD_START", 0);
                        break;

					case APP_CMD_STOP:
                        LOG_DBG("APP_CMD_STOP", 0);
                        break;

					case APP_CMD_RESUME:
                        LOG_DBG("APP_CMD_RESUME", 0);
                        appState.Resumed = true;
                        break;

					case APP_CMD_PAUSE:
                        LOG_DBG("APP_CMD_PAUSE", 0);
                        appState.Resumed = false;
                        break;

					case APP_CMD_INIT_WINDOW:
                        LOG_DBG("APP_CMD_INIT_WINDOW", 0);
                        appState.NativeWindow = app->window;
                        break;

					case APP_CMD_TERM_WINDOW:
                        LOG_DBG("APP_CMD_TERM_WINDOW", 0);
                        appState.NativeWindow = nullptr;
                        break;

					case APP_CMD_CONTENT_RECT_CHANGED:
                        if (ANativeWindow_getWidth(app->window) < ANativeWindow_getHeight(app->window))
                        {
                            // An app that is relaunched after pressing the home button gets an initial surface with
                            // the wrong orientation even though android:screenOrientation="landscape" is set in the
                            // manifest. The choreographer callback will also never be called for this surface because
                            // the surface is immediately replaced with a new surface with the correct orientation.
                            LOG_ERR("- Surface not in landscape mode!", 0);
                        }

                        if (app->window != appState.NativeWindow)
                        {
                            if (appState.NativeWindow != nullptr)
                            {
                                // todo : perform actions due to window being destroyed
                                appState.NativeWindow = nullptr;
                            }

                            if (app->window != nullptr)
                            {
                                // todo : perform actions due to window being created
                                appState.NativeWindow = app->window;
                            }
                        }
                        break;

					default:
                        break;
					}

                    android_app_post_exec_cmd(app, cmd);
                }
                else if (ident == LOOPER_ID_INPUT)
                {
                    AInputEvent * event = nullptr;
                    while (AInputQueue_getEvent(app->inputQueue, &event) >= 0)
                    {
                        //if (AInputQueue_preDispatchEvent(app->inputQueue, event)) {
                        //    continue;
                        //}

                        int32_t handled = 0;

                        if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY)
                        {
                            int keyCode = AKeyEvent_getKeyCode(event);
                            int action = AKeyEvent_getAction(event);

                            if (action != AKEY_EVENT_ACTION_DOWN &&
                                action != AKEY_EVENT_ACTION_UP)
                            {
                                // dispatch
                            }
                            else if (keyCode == AKEYCODE_VOLUME_UP)
                            {
                                // todo : we get here twice due to up/down ?
                                //adjustVolume(1);
                                handled = 1;
                            }
                            else if (keyCode == AKEYCODE_VOLUME_DOWN)
                            {
                                // todo : we get here twice due to up/down ?
                                //adjustVolume(-1);
                                handled = 1;
                            }
                            else
                            {
                                if (action == AKEY_EVENT_ACTION_UP)
                                {
                                    //Log.v( TAG, "GLES3JNIActivity::dispatchKeyEvent( " + keyCode + ", " + action + " )" );
                                }

                                ovrApp_HandleKeyEvent(
                                    &appState,
                                    keyCode,
                                    action);

                                handled = 1;
                            }
                        }

                        AInputQueue_finishEvent(app->inputQueue, event, handled);
                    }
                }

                ovrApp_HandleVrModeChanges(&appState);
            }

            // We must read from the event queue with regular frequency.
            ovrApp_HandleVrApiEvents(&appState);

            ovrApp_HandleInput(&appState);

            if (appState.Ovr == nullptr)
                continue;

            // Create the scene if not yet created.
            // The scene is created here to be able to show a loading icon.
            if (!ovrScene_IsCreated(&appState.Scene))
            {
                // Show a loading icon.
                const int frameFlags = VRAPI_FRAME_FLAG_FLUSH;

                ovrLayerProjection2 blackLayer = vrapi_DefaultLayerBlackProjection2();
                blackLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;

                ovrLayerLoadingIcon2 iconLayer = vrapi_DefaultLayerLoadingIcon2();
                iconLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;

                const ovrLayerHeader2 * layers[] =
                    {
                        &blackLayer.Header,
                        &iconLayer.Header,
                    };

                ovrSubmitFrameDescription2 frameDesc = { };
                frameDesc.Flags = frameFlags;
                frameDesc.SwapInterval = 1;
                frameDesc.FrameIndex = appState.FrameIndex;
                frameDesc.DisplayTime = appState.DisplayTime;
                frameDesc.LayerCount = 2;
                frameDesc.Layers = layers;

                vrapi_SubmitFrame2(appState.Ovr, &frameDesc);

                // Create the scene.
                ovrScene_Create(&appState.Scene, appState.UseMultiview);
            }

            // This is the only place the frame index is incremented, right before
            // calling vrapi_GetPredictedDisplayTime().
            appState.FrameIndex++;

            // Get the HMD pose, predicted for the middle of the time period during which
            // the new eye images will be displayed. The number of frames predicted ahead
            // depends on the pipeline depth of the engine and the synthesis rate.
            // The better the prediction, the less black will be pulled in at the edges.
            const double predictedDisplayTime = vrapi_GetPredictedDisplayTime(appState.Ovr, appState.FrameIndex);
            const ovrTracking2 tracking = vrapi_GetPredictedTracking2(appState.Ovr, predictedDisplayTime);

            appState.DisplayTime = predictedDisplayTime;

            // Advance the simulation based on the elapsed time since start of loop till predicted
            // display time.
            ovrSimulation_Advance(&appState.Simulation, predictedDisplayTime - startTime);

            // Render eye images and setup the primary layer using ovrTracking2.
            const ovrLayerProjection2 worldLayer = ovrRenderer_RenderFrame(
                &appState.Renderer,
                &appState.Java,
                &appState.Scene,
                &appState.Simulation,
                &tracking,
                appState.Ovr);

            const ovrLayerHeader2 * layers[] = { &worldLayer.Header };

            ovrSubmitFrameDescription2 frameDesc = { };
            frameDesc.Flags = 0;
            frameDesc.SwapInterval = appState.SwapInterval;
            frameDesc.FrameIndex = appState.FrameIndex;
            frameDesc.DisplayTime = appState.DisplayTime;
            frameDesc.LayerCount = 1;
            frameDesc.Layers = layers;

            // Hand over the eye images to the time warp.
            vrapi_SubmitFrame2(appState.Ovr, &frameDesc);
        }

        ovrRenderer_Destroy(&appState.Renderer);

        ovrScene_Destroy(&appState.Scene);
        ovrEgl_DestroyContext(&appState.Egl);

        vrapi_Shutdown();

        java.Vm->DetachCurrentThread();
    }
}
