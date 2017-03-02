uniform sampler2D colormap;
uniform vec4 params1;
uniform vec4 params2;

shader_in vec2 texcoord;

float strength = params1.x;

float param1 = params2.x;
float param2 = params2.y;
float param3 = params2.z;
float param4 = params2.w;
