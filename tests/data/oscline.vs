include engine/ShaderVS.txt

#define EPS 1E-6

uniform float uInvert;
uniform float uSize;

#define aStart in_position4.xy
#define aEnd in_position4.zw

shader_out vec4 uvl;

void main ()
{
    // All points in quad contain the same data:
    // segment start point and segment end point.
    // We determine point position from it's index.

    int idx = gl_VertexID % 4;
    
    if (idx == 0)
        idx = 0;
    else if (idx == 1)
        idx = 2;
    else if (idx == 2)
        idx = 3;
    else
        idx = 1;

    int idx2 = (gl_VertexID / 4 * 4) + idx;

    vec2 current = idx <= 1 ? aStart : aEnd;
    int tang = idx <= 1 ? -1 : +1;

    float side = ((idx % 2) - 0.5) * 2.0;

    uvl.xy = vec2(tang, side);
    uvl.w = floor(idx2 / 4.0 + 0.5);

    vec2 dir = aEnd - aStart;

    uvl.z = length(dir);

    if (uvl.z > EPS)
    {
        dir = dir / uvl.z;
    }
    else
    {
        // If the segment is too short draw a square;
        dir = vec2(1.0, 0.0);
    }

    vec2 norm = vec2(-dir.y, +dir.x);

    vec2 pos = (current+(tang*dir+norm*side)*uSize)*uInvert;

    gl_Position = ModelViewProjectionMatrix * vec4(pos, 0.0, 1.0);
}
