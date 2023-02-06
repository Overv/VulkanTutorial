#version 450

struct Particle {
	vec2 position;
	vec2 velocity;
    vec4 color;
};

layout (binding = 0) uniform ParameterUBO {
    float deltaTime;
} ubo;

layout(std140, binding = 1) readonly buffer ParticleSSBOIn {
   Particle particlesIn[ ];
};

layout(std140, binding = 2) buffer ParticleSSBOOut {
   Particle particlesOut[ ];
};

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main() 
{
    uint index = gl_GlobalInvocationID.x;  

    Particle particleIn = particlesIn[index];

    particlesOut[index].position = particleIn.position + particleIn.velocity.xy * ubo.deltaTime;
    particlesOut[index].velocity = particleIn.velocity;

    // Flip movement at window border
    if ((particlesOut[index].position.x <= -1.0) || (particlesOut[index].position.x >= 1.0)) {
        particlesOut[index].velocity.x = -particlesOut[index].velocity.x;
    }
    if ((particlesOut[index].position.y <= -1.0) || (particlesOut[index].position.y >= 1.0)) {
        particlesOut[index].velocity.y = -particlesOut[index].velocity.y;
    }

}