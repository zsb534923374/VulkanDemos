#version 450

layout (location = 0) in vec2 inUV;
layout (binding = 0) uniform sampler2D inputNormal;

layout (location = 1) out vec4 outFragColor;

void main() 
{
	 vec3 normal = texture(inputNormal, inUV).xyz;
	 vec4 color = vec4(normal, 1.0);
	 outFragColor = color;
}