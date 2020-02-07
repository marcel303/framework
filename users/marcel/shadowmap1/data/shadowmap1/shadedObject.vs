include engine/ShaderVS.txt

uniform mat4x4 lightMVP;

shader_out vec4 color;
shader_out vec3 normal;
shader_out vec4 position_lightSpace;

void main()
{
    gl_Position = ModelViewProjectionMatrix * in_position4;
    
    normal = unpackNormal().xyz;
    normal = objectToView3(normal);
    normal = normalize(normal);
    
    position_lightSpace = lightMVP * in_position4;
    
    color = unpackColor();
}
