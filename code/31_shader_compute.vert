#version 450

layout(binding = 0) uniform UniformBufferObject {
    float deltaTime;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec3 fragColor;

void main() {

    gl_PointSize = 14.0;
    gl_Position = vec4(inPosition.xy, 1.0, 1.0);

//    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor.rgb;
}
