
#type vert
#version 450

layout(push_constant)
uniform CameraData
{
    mat4 viewProjection;
} camera;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
    gl_Position =
        camera.viewProjection *
        vec4(inPosition, 0.0, 1.0);

    fragColor = inColor;
    fragTexCoord = inTexCoord;
}

#type frag
#version 450

layout(set = 1, binding = 0)
uniform sampler2D spriteTexture;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 texColor = texture(spriteTexture, fragTexCoord);

    if (texColor.a == 0.0)
        discard;

    outColor = texColor * fragColor;
}
