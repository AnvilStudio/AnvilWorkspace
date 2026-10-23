#type vert
#version 450

layout(location = 0) out vec2 vNdc;

void main()
{
    const vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vNdc = positions[gl_VertexIndex];
    gl_Position = vec4(vNdc, 0.0, 1.0);
}

#type frag
#version 450

layout(location = 0) in vec2 vNdc;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform GridParameters
{
    mat4 inverseViewProjection;
    vec4 settings;    // cell size, major interval, pixel width, reserved
    vec4 minorColor;
    vec4 majorColor;
} grid;

float lineCoverage(vec2 world, float spacing, float widthPixels)
{
    vec2 coordinates = world / spacing;
    vec2 derivatives = max(fwidth(coordinates), vec2(1e-6));
    vec2 distancePx = abs(coordinates - round(coordinates)) / derivatives;
    float distance = min(distancePx.x, distancePx.y);
    return 1.0 - smoothstep(widthPixels * 0.5,
                            widthPixels * 0.5 + 1.0, distance);
}

void main()
{
    // Same NDC convention as the viewport projection and its Vulkan viewport.
    vec4 worldHomogeneous = grid.inverseViewProjection * vec4(vNdc, 0.0, 1.0);
    vec2 world = worldHomogeneous.xy / worldHomogeneous.w;

    float cell = max(grid.settings.x, 0.001);
    float majorCell = cell * max(grid.settings.y, 1.0);
    float width = max(grid.settings.z, 0.1);

    // Reduce dense-grid shimmer on zoom out.
    float pixelsPerCell = 1.0 / max(length(fwidth(world / cell)), 1e-6);
    float minorFade = smoothstep(3.0, 12.0, pixelsPerCell);

    float minor = lineCoverage(world, cell, width) * minorFade;
    float major = lineCoverage(world, majorCell, width * 1.4);

    // Priority-composite line colors, preserving their alpha.
    vec4 color = grid.minorColor;
    color.a *= minor;
    vec4 stronger = grid.majorColor;
    stronger.a *= major;
    color.rgb = mix(color.rgb, stronger.rgb, major);
    color.a = max(color.a, stronger.a);

    // X axis = red, Y axis = green.
    vec2 derivative = max(fwidth(world), vec2(1e-6));
    float xAxis = 1.0 - smoothstep(0.5, 1.5, abs(world.y) / derivative.y);
    float yAxis = 1.0 - smoothstep(0.5, 1.5, abs(world.x) / derivative.x);
    color.rgb = mix(color.rgb, vec3(0.89, 0.37, 0.37), xAxis);
    color.a = max(color.a, xAxis * 0.75);
    color.rgb = mix(color.rgb, vec3(0.40, 0.80, 0.48), yAxis);
    color.a = max(color.a, yAxis * 0.75);
    outColor = color;
}
