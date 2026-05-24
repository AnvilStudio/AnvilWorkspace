#type vert
#version 450

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
	gl_Position = camera.ViewProjection * vec4(inPosition, 0.0, 1.0);

	fragColor = inColor;
}
 
#type frag
#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec4 fragColor;

void main() {
    outColor = fragColor;
}