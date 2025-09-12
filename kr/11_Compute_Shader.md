## 서론

이 추가 챕터에서는 컴퓨트 셰이더에 대해 알아보겠습니다. 이전 챕터에서는 Vulkan 파이프라인의 전통적인 그래픽 부분에 대해 다뤘습니다. 그러나 OpenGL과 같은 이전 API와 달리 Vulkan에서는 컴퓨트 셰이더 지원이 필수 사항이 되었습니다. 이는 고성능 데스크톱 GPU이든 저전력 임베디드 장치이든 상관없이 모든 Vulkan 구현에서 컴퓨트 셰이더를 사용할 수 있다는 것을 의미합니다.

이로써 응용 프로그램이 실행되는 장치의 종류에 관계없이 일반 목적으로의 GPU 컴퓨팅(GPGPU)을 수행할 수 있게 되었습니다. GPU에서 일반 계산을 수행할 수 있다는 것은 과거에는 CPU의 영역이었던 많은 작업을 GPU에서 실시간으로 수행할 수 있다는 것을 의미합니다. GPU가 점점 더 강력하고 유연해지면서, CPU의 일반 목적 기능이 필요한 많은 작업이 GPU에서 실시간으로 수행될 수 있게 되었습니다.

GPU의 컴퓨트 기능을 사용할 수 있는 몇 가지 예는 이미지 조작, 가시성 테스트, 후처리, 고급 조명 계산, 애니메이션, 물리(예를 들어 파티클 시스템) 등이 있습니다. 게다가 컴퓨트를 사용하여 그래픽 출력이 필요하지 않은 비시각적인 계산 작업, 예를 들어 숫자 계산이나 AI 관련 작업도 가능합니다. 이를 "헤드리스 컴퓨트"라고 합니다.

## 장점

GPU에서 컴퓨팅 비용이 높은 연산을 하는 것에는 몇 가지 장점이 있습니다. 가장 명확한 것은 CPU에서의 연산 비용을 줄일 수 있다는 것입니다. 또 다른 장점은 CPU의 메인 메모리에서 GPU의 메모리로 데이터를 옮길 필요가 없다는 것입니다. 모든 데이터는 GPU 내에 상주할 수 있으므로 메인 메모리로부터의 느린 전송을 기다릴 필요가 없습니다.

이외에도 GPU는 몇 만 개의 작은 연산 유닛을 가진 병렬화된 연산 장치입니다. 이로 인해 몇 개의 큰 연산 유닛을 가진 CPU보다 병렬화된 연산에 더 적합합니다.

## Vulkan 파이프라인

파이프라인의 그래픽스 부분과 컴퓨트 부분이 완전히 분리되어 있다는 사실을 이해하고 있는 것이 중요합니다. 이는 Vulkan 공식 명세에서 가져온 아래 Vulkan 파이프라인에 대한 블록 다이어그램을 통해서도 확인할 수 있습니다:

![](/images/vulkan_pipeline_block_diagram.png)

이 다이어그램에서 파이프라인의 일반적인 그래픽스 부분은 왼쪽에 표시되어 있고, 이러한 그래픽스 파트가 아닌 부분은 오른쪽에 표시되어 있는데 컴퓨트 셰이더(스테이지)도 오른쪽에 표시되어 있습니다. 컴퓨트 셰이더 스테이지가 그래픽스 파이프라인과 분리되어 있으므로 언제든 필요할 때 사용할 수 있습니다. 이는 예를 들어 프래그먼트 셰이더가 항상 정점 셰이더의 변환된 출력값을 활용해야 하는 것과는 아주 다르다고 볼 수 있습니다.

다이어그램의 중간에는 컴퓨트에도 사용되는 기술자 집합 등이 표시되어 있습니다. 따라서 우리가 배웠던 기술자 레이아웃, 기술자 집합 등이 여기에서도 활용될 것입니다.

## 예시

이해하기 쉬운 예시로 이 챕터에서는 GPU 기반의 파티클(particle) 시스템을 구현해 볼 것입니다. 이러한 시스템은 여러 게임에서 활용되며 몇 천개의 파티클들을 실시간에 갱신해야 합니다. 이러한 시스템을 렌더링 하기 위해서는 두 가지 주요 구성요소가 필요합니다. 정점 버퍼에서 전달된 정점들과, 이들을 수식(equation)에 기반하여 갱신하는 방법입니다.

"전통적인" CPU 기반의 파티클 시스템은 파티클 데이터를 시스템의 메인 메모리에 저장해 두고 CPU를 사용해 갱신하였습니다. 갱신이 끝나면 GPU의 메모리로 정점들이 다시 전달되어 다음 프레임에 갱신된 파티클의 위치가 표시될 수 있도록 해야만 했습니다. 가장 간단한 방법으로는 각 프레임마다 새로운 데이터로 정점 버퍼를 다시 만드는 방법이 있습니다. 물론 아주 높은 비용이 발생하죠. 구현에 따라 GPU 메모리를 맵핑하여 CPU로부터 값을 쓸 수 있게 한다거나 (데스크톱 시스템에서는 "resizable BAR", 내장 GPU에서는 통합 메모리라고 불립니다) 아니면 그냥 호스트의 지역 버퍼 (PCI-E 대역폭 문제로 아주 느립니다)를 사용하는 방법이 있습니다. 어떤 방법을 선택하든 CPU에서 갱신된 파티클이 "왕복(round-trip)"해야 한다는 요구사항이 생깁니다.

GPU 기반의 파티클 시스템에서는 이러한 왕복이 필요하지 않습니다. 정점은 처음에 GPU로 업로드되기만 하고, 이후의 모든 갱신은 컴퓨트 셰이더를 사용해 GPU의 메모리에서 이루어집니다. 이 방법이 빠른 가장 주요한 이유는 GPU와 GPU의 지역 메모리 사이의 대역폭이 훨씬 크기 때문입니다. CPU 기반의 시나리오에서는 메인 메모리와 PCI-express 대역폭으로 인해 속도가 제한되는데 이는 GPU의 메모리 대역폭에 비해 훨씬 작습니다.

이러한 작업이 GPU의 컴퓨트 큐에서 이루어진다면 그래픽스 파이프라인의 렌더링 부분과 파티클의 갱신을 병렬적으로 수행할 수 있습니다. 이를 "비동기 컴퓨트"라 하고, 이 튜토리얼에서는 다루지 않는 고급 주제입니다.

아래는 이 챕터 코드의 실행 예시입니다. 이 파티클들은 GPU의 컴퓨트 셰이더에서 직접 갱신되며, CPU와의 상호작용은 없습니다:

![](/images/compute_shader_particles.png)

## 데이터 조작(manipulation)

이 튜토리얼을 통해 이미 정점 버퍼, 인덱스 버퍼를 통해 프리미티브 데이터를 전달하는 방법과 유니폼 버퍼를 통해 셰이더에 데이터를 전달하는 법 등을 배웠습니다. 또 이미지를 사용해 텍스처 맵핑을 하는 법도요. 하지만 지금까지 우리는 항상 CPU에서 데이터를 쓰고, GPU에서 그 데이터를 읽기만 했습니다.

컴퓨트 셰이더에서 소개하는 중요한 개념은 버터의 데이터를 읽고 **쓰는** 기능입니다. 이를 위해 Vulkan은 두 종류의 스토리지를 제공합니다.

### 셰이더 스토리지 버퍼 객체(Shader storage buffer objects, SSBO)

셰이더 스토리지 버퍼(SSBO)를 통해 셰이더가 버퍼의 데이터를 읽고 쓸 수 있습니다. 사용 방법은 유니폼 버퍼 객체와 비슷합니다. 가장 큰 차이는 다른 버퍼 타입을 SSBO로 사용할 수 있어서 임의의 크기로 사용할 수 있다는 점입니다.

GPU 기반의 파티클 시스템으로 돌아가서, 정점의 갱신(쓰기)를 어떻게 컴퓨트 셰이더로 수행하고 읽기(그리기)는 정점 셰이더로 수행하는지 의아하실겁니다. 왜냐하면 두 사용법이 서로 다른 버퍼 타입을 요구하는 것 같아 보이기 때문입니다.

하지만 그렇지 않습니다. Vulkan에서는 버퍼와 이미지에 여러 사용법을 명시할 수 있습니다. 따라서 파티클 정점 버퍼를 (그래픽스 패스에서) 정점 버퍼로 활용하고 (컴퓨트 패스에서) 스토리지 버퍼로도 사용할 수 있습니다. 단지 버퍼를 만들 때 두 개의 사용법 플래그를 명시해주면 됩니다:

```c++
VkBufferCreateInfo bufferInfo{};
...
bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
...

if (vkCreateBuffer(device, &bufferInfo, nullptr, &shaderStorageBuffers[i]) != VK_SUCCESS) {
    throw std::runtime_error("failed to create vertex buffer!");
}
```

`bufferInfo.usage`에 명시한 `VK_BUFFER_USAGE_VERTEX_BUFFER_BIT`와 `VK_BUFFER_USAGE_STORAGE_BUFFER_BIT` 두 개의 플래그는 이 버퍼를 두 개의 시나리오에서 사용할 것이라는 뜻입니다. 정점 셰이더에서는 정점 버퍼로, 그리고 그리고 스토리지 버퍼로 말이죠. `VK_BUFFER_USAGE_TRANSFER_DST_BIT` 플래그 또한 추가해서 호스트에서 GPU로 데이터를 전송할 수 있도록도 한 것에 주의하세요. 셰이더 스토리지 버퍼가 GPU 메모리에만 상주해 있길 원하므로(`VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`), 호스트에서 이 버퍼로 데이터를 전송해야만 합니다.

`createBuffer` 헬퍼 함수를 통한 구현은 아래와 같습니다:

```c++
createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorageBuffers[i], shaderStorageBuffersMemory[i]);
```

이러한 버퍼에 접근하기 위한 GLSL 셰이더에서의 선언은 아래와 같습니다:

```glsl
struct Particle {
  vec2 position;
  vec2 velocity;
  vec4 color;
};

layout(std140, binding = 1) readonly buffer ParticleSSBOIn {
   Particle particlesIn[ ];
};

layout(std140, binding = 2) buffer ParticleSSBOOut {
   Particle particlesOut[ ];
};
```

이 예제에서는 타입이 명시된 SSBO를 정의했는데 각 파티클은 위치와 속도(`Particle` 구조체 참고)를 가지고 있습니다. SSBO는 `[]`를 통해 명시되지 않은 개수의 파티클을 가지도록 했습니다. SSBO에 원소의 개수를 명시하지 않아도 되는 것도 예를 들자면 유니폼 버퍼와 비교했을 때의 장점입니다. `std140`는 메모리 레이아웃 한정자로 셰이더 스토리지 버퍼의 원소들이 어떻게 메모리에 정렬되어있는지를 결정합니다. 이를 통해 호스트와 GPU 사이의 버퍼를 맵핑하는 데 필요한 요구사항이 만족되었다고 보장합니다.

스토리지 버퍼 객체에 컴퓨트 셰이더를 통해 쓰기를 수행하는 것은 어렵지 않은데, C++ 쪽에서 버퍼에 쓰기를 수행하는 것과 비슷합니다:

```glsl
particlesOut[index].position = particlesIn[index].position + particlesIn[index].velocity.xy * ubo.deltaTime;
```

### 스토리지 이미지

*이 챕터에서 이미지 조작을 수행하지는 않을 것입니다. 이 문단은 컴퓨트 셰이더를 통해 이미지 조작도 가능하다는 것을 독자들에게 알려주기 위함입니다.*

스토리지 이미지는 이미지에 읽고 쓰기를 가능하게 해줍니다. 일반적인 사용법으로는 텍스처에 이미지 이펙트를 적용한다거나, 후처리를 수행한다거나 (둘 다 비슷합니다), 밉맵을 생성하는 것입니다.

이미지에 대해서도 비슷합니다:

```c++
VkImageCreateInfo imageInfo {};
...
imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
...

if (vkCreateImage(device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
}
```

`imageInfo.usage`에 설정된 `VK_IMAGE_USAGE_SAMPLED_BIT`과 `VK_IMAGE_USAGE_STORAGE_BIT`는 이 이미지를 서로 다른 두 시나리오에 사용할 것을 명시합니다. 프래그먼트에서 샘플링될 이미지와 컴퓨트 셰이더에서의 스토리지 이미지 입니다.

GLSL 셰이더에서의 스토리지 이미지 선언은 프래그먼트 셰이더에서의 샘플링된 이미지의 사용과 비슷합니다:

```glsl
layout (binding = 0, rgba8) uniform readonly image2D inputImage;
layout (binding = 1, rgba8) uniform writeonly image2D outputImage;
```

몇 가지 차이점은 이미지 포맷을 명시하기 위한 `rgba8`과 같은 어트리뷰트와 `readonly`와  `writeonly` 한정자를 통해서 입력 이미지는 읽기만, 출력 이미지는 쓰기만 수행할 것이라는 것을 명시한 점입니다. 또한 스토리지 이미지 선언을 위해 `image2D` 타입을 명시하였습니다.

컴퓨트 셰이더에서 스토리지 이미지를 읽고 쓰는 것은 `imageLoad`와 `imageStore`를 통해 수행됩니다:

```glsl
vec3 pixel = imageLoad(inputImage, ivec2(gl_GlobalInvocationID.xy)).rgb;
imageStore(outputImage, ivec2(gl_GlobalInvocationID.xy), pixel);
```

## 컴퓨트 큐 패밀리

[물리적 장치와 큐 패밀리 챕터](03_Drawing_a_triangle/00_Setup/03_Physical_devices_and_queue_families.md#page_Queue-families)에서 큐 패밀리가 무엇인지와 그래픽스 큐 패밀리는 선택하는 방법을 배웠습니다. 컴퓨트의 경우 `VK_QUEUE_COMPUTE_BIT` 플래그의 큐 패밀리 속성을 사용합니다. 따라서 컴퓨트 작업을 하려면 컴퓨트를 지원하는 큐 패밀리로부터 큐를 얻어와야 합니다.

Vulkan은 그래픽스와 컴퓨트 연산을 모두 지원하는 큐 패밀리를 적어도 하나 갖는 그래픽스 연산을 지원하는 구현이 필요합니다. 하지만 구현이 전용 컴퓨트 큐를 제공해도 됩니다. 이러한 전용 컴퓨트 큐(그래픽스를 포함하지 않는)는 비동기 컴퓨트 큐임을 암시합니다. 이 튜토리얼에서는 좀 더 쉬운 안내를 위해 그래픽스와 컴퓨트 연산을 모두 지원하는 큐를 사용할 것입니다. 이렇게 하면 추가적인 비동기 메커니즘 또한 필요하지 않습니다.

우리 예제에서는 장치 생성 코드를 일부 수정해야 합니다:

```c++
uint32_t queueFamilyCount = 0;
vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

int i = 0;
for (const auto& queueFamily : queueFamilies) {
    if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
        indices.graphicsAndComputeFamily = i;
    }

    i++;
}
```

수정된 큐 패밀리 인덱스 선택 코드는 이제 그래픽스와 컴퓨트를 모두 지원하는 큐 패밀리를 찾게 됩니다.

`createLogicalDevice`에서는 이 큐 패밀리로부터 컴퓨트 큐를 얻습니다:

```c++
vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0, &computeQueue);
```

## 컴퓨트 셰이더 스테이지

그래픽스 예제에서 셰이더를 로드하는 부분과 기술자에 접근하는 별도의 파이프라인 스테이지가 있었습니다. 컴퓨트 셰이더 또한 비슷한 방법으로 `VK_SHADER_STAGE_COMPUTE_BIT` 파이프라인을 통해 접근됩니다. 따라서 컴퓨트 셰이더를 로드하는 것 또한 정점 셰이더를 로드하는 것과 동일하지만, 셰이더 스테이지가 다를 뿐입니다. 다음 부분에서 이 내용에 대해 자세히 알아볼 것입니다. 컴퓨트는 또한 기술자와 파이프라인에 `VK_PIPELINE_BIND_POINT_COMPUTE`라는 새로운 바인딩 포인트를 필요로 하고, 이를 사용할 예정입니다.

## 컴퓨트 셰이더 로딩

우리 프로그램에서 컴퓨트 셰이더를 로딩하는 것은 다른 셰이더 로딩과 다를 바 없습니다. 차이점은 위에서 이야기한대로 `VK_SHADER_STAGE_COMPUTE_BIT`를 사용해야 한다는 것 뿐입니다.

```c++
auto computeShaderCode = readFile("shaders/compute.spv");

VkShaderModule computeShaderModule = createShaderModule(computeShaderCode);

VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
computeShaderStageInfo.module = computeShaderModule;
computeShaderStageInfo.pName = "main";
...
```

## 셰이더 스토리지 버퍼 준비

이전에 임의의 데이터를 컴퓨트 셰이더에 넘기기 위해 셰이더 스토리지 버퍼를 사용해야 한다는 것을 배웠습니다. 이 예제에서 파티클의 배열을 GPU로 넘겨서 GPU의 메모리에서 직접 이 데이터들을 조작할 수 있도록 할 것입니다.

[여러 프레임의 사용](03_Drawing_a_triangle/03_Drawing/03_Frames_in_flight.md) 챕터에서 프레임별 리소스를 복제하는 방법을 통해 CPU와 GPU 연산을 동시에 수행할 수 있도록 하였습니다. 먼저 버퍼 객체를 위한 벡터와 이를 지원하는 장치 메모리를 선언합니다:

```c++
std::vector<VkBuffer> shaderStorageBuffers;
std::vector<VkDeviceMemory> shaderStorageBuffersMemory;
```

`createShaderStorageBuffers`에서 이 벡터의 크기를 최대값에 맞게 설정합니다. 사용하는 프레임의 개수입니다:

```c++
shaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
shaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
```

이제 초기 파티클 정보를 GPU로 넘겨줄 수 있습니다. 먼저 호스트 쪽에서 파티클의 벡터를 초기화 합니다:

```c++
    // Initialize particles
    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initial particle positions on a circle
    std::vector<Particle> particles(PARTICLE_COUNT);
    for (auto& particle : particles) {
        float r = 0.25f * sqrt(rndDist(rndEngine));
        float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
        float x = r * cos(theta) * HEIGHT / WIDTH;
        float y = r * sin(theta);
        particle.position = glm::vec2(x, y);
        particle.velocity = glm::normalize(glm::vec2(x,y)) * 0.00025f;
        particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
    }

```

그리고 [스테이징 버퍼](04_Vertex_buffers/02_Staging_buffer.md)를 호스트의 메모리에 만들어 초기 파티클 속성을 저장합니다:

```c++
    VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, particles.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
```    

이 스테이징 버퍼를 소스로 해서 프레임별 셰이더 스토리지 버퍼를 만들고 파티클 속성을 각각의 스테이징 버퍼에 복사합니다:

```c++
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorageBuffers[i], shaderStorageBuffersMemory[i]);
        // Copy data from the staging buffer (host) to the shader storage buffer (GPU)
        copyBuffer(stagingBuffer, shaderStorageBuffers[i], bufferSize);
    }
}
```

## 기술자

컴퓨트를 위한 기술자를 설정하는 것은 그래픽스에서와 거의 동일합니다. 유일한 차이점은 기술자가 `VK_SHADER_STAGE_COMPUTE_BIT`가 설정되어 있어서 컴퓨트 스테이지에서 접근 가능해야 한다는 것입니다:

```c++
std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
layoutBindings[0].binding = 0;
layoutBindings[0].descriptorCount = 1;
layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
layoutBindings[0].pImmutableSamplers = nullptr;
layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
...
```

셰이더 스테이지를 여기에서 결합할 수 있기 때문에 정점과 컴퓨트 스테이지에서 기술자를 접근 가능하게 하고 싶다면 (예를 들자면 유니폼 버퍼가 양 셰이더에서 매개변수를 공유하게 하고 싶다면) 두 스테이지를 모두 설정하면 됩니다:

```c++
layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
```

우리의 예제에 대한 기술자 설정은 이와 같습니다. 레이아웃은 아래와 같습니다:

```c++
std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
layoutBindings[0].binding = 0;
layoutBindings[0].descriptorCount = 1;
layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
layoutBindings[0].pImmutableSamplers = nullptr;
layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

layoutBindings[1].binding = 1;
layoutBindings[1].descriptorCount = 1;
layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
layoutBindings[1].pImmutableSamplers = nullptr;
layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

layoutBindings[2].binding = 2;
layoutBindings[2].descriptorCount = 1;
layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
layoutBindings[2].pImmutableSamplers = nullptr;
layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

VkDescriptorSetLayoutCreateInfo layoutInfo{};
layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
layoutInfo.bindingCount = 3;
layoutInfo.pBindings = layoutBindings.data();

if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &computeDescriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute descriptor set layout!");
}
```

이 설정을 보면 셰이더 스토리지 버퍼 객체에 대해 왜 두 레이아웃 바인딩이 있는지 의문이 드실겁니다. 파티클 시스템은 하나만 렌더링 하는데도요. 그 이유는 파티클의 위치가 프레임별 시간에 따라 갱신될 것이기 때문입니다. 즉 각 프레임에서 이전 프레임에서의 파티클 위치를 알아야 하고, 그 위치들을 프레임간 소요 시간(delta time)을 기반으로 갱신한 뒤 자신 SSBO에 쓰게 됩니다:

![](/images/compute_ssbo_read_write.svg)

이렇게 하려면 컴퓨트 셰이더가 이전과 현재 프레임의 SSBO에 접근 가능해야 합니다. 기술자 설정 과정에서 셰이더에 둘 다 넘겨주면 됩니다. `storageBufferInfoLastFrame`과 `storageBufferInfoCurrentFrame`를 살펴 봅시다:

```c++
for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = uniformBuffers[i];
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = sizeof(UniformBufferObject);

    std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
    ...

    VkDescriptorBufferInfo storageBufferInfoLastFrame{};
    storageBufferInfoLastFrame.buffer = shaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT];
    storageBufferInfoLastFrame.offset = 0;
    storageBufferInfoLastFrame.range = sizeof(Particle) * PARTICLE_COUNT;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = computeDescriptorSets[i];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &storageBufferInfoLastFrame;

    VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
    storageBufferInfoCurrentFrame.buffer = shaderStorageBuffers[i];
    storageBufferInfoCurrentFrame.offset = 0;
    storageBufferInfoCurrentFrame.range = sizeof(Particle) * PARTICLE_COUNT;

    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = computeDescriptorSets[i];
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;

    vkUpdateDescriptorSets(device, 3, descriptorWrites.data(), 0, nullptr);
}
```

기술자 풀에서 SSBO의 기술자 타입을 요청해야 하는 것을 잊지 마세요:

```c++
std::array<VkDescriptorPoolSize, 2> poolSizes{};
...

poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;
```

풀에서 요청한 `VK_DESCRIPTOR_TYPE_STORAGE_BUFFER` 타입의 숫자를 두 배 해서 이전과 현재 프레임의 SSBO의 참조할 수 있도록 합니다.

## 컴퓨트 파이프라인

컴퓨트는 그래픽스 파이프라인에 속하지 않으므로 `vkCreateGraphicsPipelines`를 사용할 수 없습니다. 대신 `vkCreateComputePipelines`를 사용해 적절한 파이프라인을 만들어 컴퓨트 명령을 수행해야 합니다. 컴퓨트 파이프라인은 래스터화 상태와는 상관이 없으므로 그래픽스 파이프라인보다는 상태가 훨씬 적습니다:

```c++
VkComputePipelineCreateInfo pipelineInfo{};
pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
pipelineInfo.layout = computePipelineLayout;
pipelineInfo.stage = computeShaderStageInfo;

if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline!");
}
```

설정은 훨씬 간단한데, 하나의 셰이더 스테이지와 파이프라인 레이아웃만 있으면 되기 때문입니다. 파이프라인 레이아웃 작업은 그래픽스 파이프라인과 동일합니다:

```c++
VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 1;
pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;

if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline layout!");
}
```

## 컴퓨트 공간(space)

컴퓨트 셰이더가 어떻게 동작하고, 어떻게 GPU에 컴퓨트 작업을 제출하는지 알아보기 전에, 두 가지 중요한 개념에 대해 먼저 이야기 해보겠습니다. 이는 **작업 그룹(work groups)**과 **호출(invocations)** 입니다. 이들은 컴퓨트 작업이 GPU의 컴퓨트 하드웨어에서 어떻게 처리되는지를 3차원(x,y,z)으로 추상화한 실행 모델을 정의합니다.

**작업 그룹**은 컴퓨트 작업이 어떻게 구성되고 GPU의 컴퓨트 하드웨어에서 처리되는지를 정의합니다. 이를 GPU가 작업해야 하는 작업 아이템이라고 생각할 수 있습니다. 작업 그룹의 차원은 응용 프로그램의 명령 버퍼 시점에서 디스패치(dispatch) 명령에 의해 설정됩니다.

각 작업 그룹은 동일한 컴퓨트 셰이더를 실행하는 **호출**들의 집합입니다. 호출은 잠재적으로 병렬 실행되며 그 차원은 컴퓨트 셰이더에 설정합니다. 동일한 작업 그룹에 속한 호출들은 공유 메모리에 접근할 수 있습니다.

아래 그림은 3차원으로 이 두 가지 개념의 관계를 보여줍니다:

![](/images/compute_space.svg)

(`vkCmdDispatch`에 의해 정의되는) 작업 그룹의 차원과 (컴퓨트 셰이더에서 로컬 크기로 정의되는) 호출의 차원은 입력 데이터가 어떻게 구성되어 있는지에 의존적입니다. 예를 들어 여러분이 1차원 배열에 대해 작업을 수행한다면 (이 챕터의 예제처럼), 두 가지 모두 x 차원만 명시하면 됩니다. 

예를 들어: 작업 그룹 [64, 1, 1]개를 컴퓨트 셰이더의 로컬 크기 [32, 32, 1]로 디스패치 하면 컴퓨트 셰이더는 총 64 x 32 x 32 = 65,536 번 호출됩니다.

작업 그룹과 로컬 크기는 구현마다 다르므로 `VkPhysicalDeviceLimits`에 정의된 `maxComputeWorkGroupCount`, `maxComputeWorkGroupInvocations`, `maxComputeWorkGroupSize`를 항상 확인해야 합니다.

## 컴퓨트 셰이더

이제 컴퓨트 셰이더 파이프라인과 관련한 설정에 대해 모두 배웠으니 이제 컴퓨트 셰이더 자체를 살펴 봅시다. 지금까지 GLSL 셰이더에 대해서 배웠던, 예를 들면 정점 셰이더와 프래그먼트 셰이더에 관한 것들이 컴퓨트 셰이더에도 적용됩니다. 문법 또한 동일하고 응용 프로그램에서 세이더로 데이터를 넘기는 방법도 같습니다. 하지만 중요한 차이도 몇 가지 있습니다.

일차원 배열로 저장된 파티클을 갱신하는 간단한 컴퓨트 셰이더는 아래와 같습니다:

```glsl
#version 450

layout (binding = 0) uniform ParameterUBO {
    float deltaTime;
} ubo;

struct Particle {
    vec2 position;
    vec2 velocity;
    vec4 color;
};

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
    ...
}
```

최상단에는 셰이더의 입력을 정의하는 부분이 있습니다. 첫 번째로 0에 바인딩된 유니폼 버퍼 객체가 있는데, 이미 배웠던 내용입니다. 그 아래는 파티클 구조체에 대한 선언이 있고 이는 C++쪽의 선언과 매칭됩니다. 아래 1에 바인딩한 것은 셰이더 스토리지 버퍼 객체로 이전 프레임의 파티클 데이터이고(기술자 설정 부분 참고), 2는 현재 프레임의 SSBO의 바인딩 포인트이며 셰이더에서 갱신할 부분입니다.

흥미로운 부분은 컴퓨트 공간과 관련된, 컴퓨트에서만 활용되는 선언입니다:

```glsl
layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;
```

위 코드가 이 컴퓨트 셰이더가 작업 그룹 안에서 호출될 개수를 정의합니다. 전에 이야기한 것처럼 이 부분은 컴퓨트 공간의 로컬 부분입니다. 따라서 `local_` 접두어가 붙습니다. 우리는 파티클에 대한 1차원 배열을 사용하기 때문에 x 차원인 `local_size_x`에만 숫자를 명시해 줍니다.

`main` 함수에서는 이전 프레임의 SSBO를 읽어와 현재 프레임의 SSBO에 파티클의 위치를 갱신해 줍니다. 다른 셰이더 타입과 비슷하게 컴퓨트 셰이더도 내장 입력 변수를 가지고 있습니다. 내장 변수는 항상 `gl_` 접두어를 가지고 있습니다. 내장 변수 중 하나로 `gl_GlobalInvocationID`가 있는데 현재 디스패치에 대한 현재 컴퓨트 셰이더 호출의 ID를 가지고 있는 변수입니다. 우리는 이 값을 파티클 배열의 인덱스로 사용합니다.

## 컴퓨트 명령 실행

### 디스패치

이제 실제로 GPU가 컴퓨트 작업을 하도록 할 차례입니다. 이는 명령 버퍼에서 `vkCmdDispatch`를 호출하여 수행합니다. 완전히 같진 않지만, 그래픽스에서 드로우를 수행하기 위해 `vkCmdDraw`를 호출하는 것이 컴퓨트에서는 디스패치와 대응됩니다. 이를 통해 (최대) 3차원으로 주어진 개수만큼 컴퓨트 작업 아이템을 디스패치합니다.

```c++
VkCommandBufferBeginInfo beginInfo{};
beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
}

...

vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSets[i], 0, 0);

vkCmdDispatch(computeCommandBuffer, PARTICLE_COUNT / 256, 1, 1);

...

if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
}
```

`vkCmdDispatch`는 x 차원에 대해 `PARTICLE_COUNT / 256`개의 로컬 작업 그룹을 디스패치합니다. 우리의 파티클 배열은 1차원이므로 나머지 두 차원은 1로 두었고, 이는 1차원 디스패치를 의미합니다. 그런데 왜 (배열 내의) 파티클 개수를 256으로 나누는 것일까요? 이는 이전 장에서 우리가 작업 그룹 내의 각 컴퓨트 셰이더가 256번씩 호출되도록 정의했기 때문입니다. 따라서 4096개의 파티클이 있다면, 16개의 작업 그룹을 디스패치 하는데 각 작업 그룹 내에서는 256번의 컴퓨트 셰이더 호출이 수행되는 것입니다. 이러한 숫자들을 올바로 정의하는 것은 필요한 작업량과 실행되는 하드웨어에 따라 때때로 수정과 확인이 필요합니다. 만일 파티클 개수가 동적으로 변해서 항상 256으로 나누어 떨어지지 않는다면 컴퓨트 셰이더 시작 지점에서 `gl_GlobalInvocationID`를 사용해 파티클 개수보다 큰 글로벌 호출 인덱스를 갖는 경우 바로 반환하도록 할 수 있습니다.

컴퓨트 파이프라인에서와 마찬가지고 컴퓨트 명령 버퍼도 그래픽스 명령 버퍼보다 훨씬 적인 상태만 가지고 있습니다. 렌더 패스를 시작하거나, 뷰포트를 설정하는 등의 작업은 필요 없습니다.

### 작업 제출

우리 예제는 컴퓨트와 그래픽스 연산을 모두 가지고 있으므로 매 프레임마다 그래픽스와 컴퓨트 큐에 모두 제출을 수행해야 합니다(`drawFrame` 함수 참고):

```c++
...
if (vkQueueSubmit(computeQueue, 1, &submitInfo, nullptr) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit compute command buffer!");
};
...
if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
}
```

첫 번째의 컴퓨트 큐로의 제출은 컴퓨트 셰이더를 통해 파티클의 위치를 갱신하고, 두 번째의 제출은 갱신된 데이터로 파티클 시스템을 그립니다.

### 그래픽스와 컴퓨트의 동기화

동기화는 Vulkan에서 중요한 부분 중 하나로, 컴퓨트와 그래픽스를 동시에 사용할 때는 더 중요해집니다. 동기화가 없거나 잘못된 경우 컴퓨트 셰이더가 갱신을 끝내기(=쓰기) 전에 정점 스테이지가 그리기(=읽기)가 시작되거나 정점에서 사용되고 있는 부분의 파티클을 컴퓨트 셰이더가 갱신하기 시작한다거나 하는 문제가 발생할 수 있습니다.

따라서 이러한 경우가 발생하지 않도록 적절히 그래픽스와 컴퓨트 간에 동기화를 수행해야만 합니다. 수행하는 방법은 컴퓨트 작업을 어떻게 제출했느냐에 따라 여러 방법이 있을 수 있는데 우리의 경우 두 개의 제출이 분리되어 있으므로 [세마포어](03_Drawing_a_triangle/03_Drawing/02_Rendering_and_presentation.md#page_Semaphores)와 [펜스](03_Drawing_a_triangle/03_Drawing/02_Rendering_and_presentation.md#page_Fences) 를 사용해 컴퓨트 셰이더가 갱신을 끝내기 전에는 정점 셰이더가 데이터 읽기를 수행하지 않도록 할 것입니다.

두 제출 과정에 선후 관계가 있어도 이러한 동기화가 필요한데, GPU에서 이렇게 제출된 순서대로 실행된다는 보장이 없기 때문입니다. 대기 및 시그널 상태 세마포어를 사용하면 실행 순서를 보장할 수 있습니다.

먼저 `createSyncObjects`에서 컴퓨트 작업을 위한 동기화 요소들을 추가할 것입니다. 그래픽스 펜스와 마찬가지로 컴퓨트 펜스는 시그널 상태로 생성될 것인데, 그렇지 않으면 첫 번째의 그리기 호출이 펜스가 시그널 상태가 될때까지 계속 기다리다 타임아웃이 되기 때문입니다. 자세한 내용은 [여기](03_Drawing_a_triangle/03_Drawing/02_Rendering_and_presentation.md#page_Waiting-for-the-previous-frame)를 참고하세요:

```c++
std::vector<VkFence> computeInFlightFences;
std::vector<VkSemaphore> computeFinishedSemaphores;
...
computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

VkSemaphoreCreateInfo semaphoreInfo{};
semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

VkFenceCreateInfo fenceInfo{};
fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    ...
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &computeFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &computeInFlightFences[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute synchronization objects for a frame!");
    }
}
```

그러고 나서 컴퓨트 버퍼의 제출과 그래픽스 제출을 동기화 하는데 이들을 사용합니다:

```c++
// Compute submission
vkWaitForFences(device, 1, &computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

updateUniformBuffer(currentFrame);

vkResetFences(device, 1, &computeInFlightFences[currentFrame]);

vkResetCommandBuffer(computeCommandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
recordComputeCommandBuffer(computeCommandBuffers[currentFrame]);

submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers = &computeCommandBuffers[currentFrame];
submitInfo.signalSemaphoreCount = 1;
submitInfo.pSignalSemaphores = &computeFinishedSemaphores[currentFrame];

if (vkQueueSubmit(computeQueue, 1, &submitInfo, computeInFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit compute command buffer!");
};

// Graphics submission
vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

...

vkResetFences(device, 1, &inFlightFences[currentFrame]);

vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

VkSemaphore waitSemaphores[] = { computeFinishedSemaphores[currentFrame], imageAvailableSemaphores[currentFrame] };
VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
submitInfo = {};
submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

submitInfo.waitSemaphoreCount = 2;
submitInfo.pWaitSemaphores = waitSemaphores;
submitInfo.pWaitDstStageMask = waitStages;
submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
submitInfo.signalSemaphoreCount = 1;
submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];

if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
}
```

[세마포어 챕터](03_Drawing_a_triangle/03_Drawing/02_Rendering_and_presentation.md#page_Semaphores)에서의 예제와 비슷하게 이렇게 설정하면 대기 세마포어가 없기 때문에 컴퓨트 셰이더가 곧바로 실행됩니다. 문제는 없는것이 `vkWaitForFences` 커맨드를 통한 컴퓨트의 제출 이전에 현재 프레임의 컴퓨트 커맨드 버퍼의 실행이 끝나는 것을 기다리고 있기 때문입니다.

그래픽스 관련 제출은 컴퓨트 작업이 끝나기를 기다려야 하므로 컴퓨트 버퍼가 갱신하는 도중에는 정점을 가져오지 말아야 합니다. 따라서 그래픽스의 제출은 현재 프레임의 `computeFinishedSemaphores`를 기다리면서 정점이 사용되는 `VK_PIPELINE_STAGE_VERTEX_INPUT_BIT` 스테이지에서 대기하도록 해야 합니다.

또한 표시도 기다려야 하는데 이미지가 표시되기 전에 프래그먼트 셰이더가 생상 출력을 내지 않도록 하기 위함입니다. 따라서 현재 프레임의 `imageAvailableSemaphores`를 `VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT` 스테이지에서 기다려야 합니다.

## 파티클 시스템 그리기

이전 내용에서 Vulkan에서 버퍼는 여러 사용법을 가질 수 있다는 것을 배웠고, 셰이더 스토리지 버퍼를 만들고 여기에 저장된 파티클들이 셰이더 스토리지 버퍼와 정점 버퍼로 활용될 수 있도록 설정했습니다. 다시말해 셰이더 스토리지 버퍼를 전에 사용했던 "순수한" 정점 버퍼처럼 그리기를 위해 사용할 수 있다는 뜻입니다.

먼저 정점 입력 상태를 파티클 구조체와 매칭기켜줍니다:

```c++
struct Particle {
    ...

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Particle, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Particle, color);

        return attributeDescriptions;
    }
};
```

정점 입력 어트리뷰트에 `velocity`는 추가하지 않은 것에 유의하세요. 이 값은 컴퓨트 셰이더에서만 사용하기 때문입니다.

그리고 일반적인 정점 버퍼처럼 바딩인들 하고 그리기를 수행합니다:

```c++
vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shaderStorageBuffer[currentFrame], offsets);

vkCmdDraw(commandBuffer, PARTICLE_COUNT, 1, 0, 0);
```

## 결론

이 장에서, 우리는 컴퓨트 셰이더를 사용해 CPU의 작업을 GPU로 이전하는 방법을 배웠습니다. 컴퓨트 셰이더가 없었다면 현대 게임 및 응용 프로그램들의 몇몇 효과는 불가능했거나 훨씬 느렸을 것입니다. 그래픽스 용도 이외에도 컴퓨트는 다양한 사용 방법이 존재합니다. 이 챕터는 그 가능성 중 아주 일부분을 보여드렸을 뿐입니다. 이제 컴퓨트 셰이더를 사용하는 방법을 알게 되셨으니 컴퓨트 관련한 고급 토픽들을 알아보고 싶으실겁니다:

- Shared memory
- [Asynchronous compute](https://github.com/KhronosGroup/Vulkan-Samples/tree/master/samples/performance/async_compute)
- Atomic operations
- [Subgroups](https://www.khronos.org/blog/vulkan-subgroup-tutorial)

고급 컴퓨트 기능에 대한 예제는 [공식 Khronos Vulkan Samples 레포지토리](https://github.com/KhronosGroup/Vulkan-Samples/tree/master/samples/api)에서 찾아보실 수 있습니다.

[C++ code](/code/31_compute_shader.cpp) /
[Vertex shader](/code/31_shader_compute.vert) /
[Fragment shader](/code/31_shader_compute.frag) /
[Compute shader](/code/31_shader_compute.comp)
