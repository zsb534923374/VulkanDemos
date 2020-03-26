#version 450

layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput inputNormal;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 normal   = subpassLoad(inputNormal);
	outFragColor = normal;
}