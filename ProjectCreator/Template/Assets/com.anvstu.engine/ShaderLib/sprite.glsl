#type vert
#version 450

layout(push_constant)
uniform PushData
{
    mat4 transform;
    vec4 color;
} push_data;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
    gl_Position = push_data.transform *
        vec4(inPosition, 0.0, 1.0);

    fragColor = push_data.color;
    fragTexCoord = inTexCoord;
}

#type frag
#version 450

layout(set = 0, binding = 0) uniform sampler2D spriteTexture;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(spriteTexture, fragTexCoord) * fragColor;
}
