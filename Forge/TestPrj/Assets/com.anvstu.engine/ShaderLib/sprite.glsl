#type vert
#version 450

layout(push_constant)
uniform PushData
{
    mat4 model;
    vec4 color;
} push_data;

layout(set = 0, binding = 0) uniform Camera
{
    mat4 View;
    mat4 Projection;
    mat4 ViewProjection;
} camera;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main()
{
	gl_Position = camera.ViewProjection * 
    push_data.model * 
    vec4(inPosition, 0.0, 1.0);

	fragColor = push_data.color;
}
 
#type frag
#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec4 fragColor;

void main() {
    outColor = fragColor;
}