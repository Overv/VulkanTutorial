## 개요

다음 몇 챕터동안 정점 셰이더에 하드코딩된 정점 데이터를 메모리의 정점 버퍼(vertex buffer)로 바꾸어 보겠습니다.
먼저 가장 손쉬운 방법인 CPU에서 보이는(visible) 버퍼를 만든 뒤 `memcpy`를 통해 정점 데이터를 직접 복사하는 방법을 알아볼 것이고, 이후에 스테이징 버퍼(staging buffer)를 사용해 정점 데이터를 고성능 메모리에 복사하는 방법을 알아볼 것입니다.

## 정점 셰이더

먼저 정점 셰이더가 정점 데이터를 코드로 포함하지 않도록 수정할 것입니다.
정점 셰이더는 `in` 키워드로 정점 버퍼에서 입력을 받을 것입니다.

```glsl
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

`inPosition`와 `inColor` 변수는 *정점 어트리뷰트(vertex attribute)*입니다.
이는 정점 버퍼에 명시된 정점별 속성이며, 기존처럼 위치와 속성 데이터 입니다.
정점 셰이더를 수정한 뒤 다시 컴파일하는 것을 잊지 마세요!

`fragColor`처럼, `layout(location = x)`는 입력에 대해 나중에 참조하기 위한 인덱스를 할당하는 것입니다.
예를들어 `dvec3`와 같은 64비트 벡터는 여러 *슬롯(slot)*을 사용한다는 사실을 중요하게 알아두셔야 합니다.
이러한 경우 그 다음으로 오는 인덱스는 2 이상 큰 인덱스여야 합니다:

```glsl
layout(location = 0) in dvec3 inPosition;
layout(location = 2) in vec3 inColor;
```

레이아웃 한정자(qualifier)에 대해서는 [OpenGL wiki](https://www.khronos.org/opengl/wiki/Layout_Qualifier_(GLSL))에서 자세한 정보를 찾아 볼 수 있습니다.

## 정점 데이터

정점 데이터를 셰이더 코드에서 우리 프로그램의 배열로 옮길 예정입니다.
먼저 벡터와 행렬 같은 선형대수 관련 자료형을 제공해 주는 GLM 라이브러를 include 하는 것 부터 시작합니다.
이 자료형들을 사용해 위치와 색상 벡터를 명시할 것입니다.

```c++
#include <glm/glm.hpp>
```

우리가 정점 셰이더에서 사용할 두 어트리뷰트를 포함하는 `Vertex` 구조체를 만듭니다:

```c++
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};
```

GLM은 셰이더 언어에서 사용되는 벡터 자료형과 정확히 매치되는 C++ 자료형을 제공해 줍니다:

```c++
const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};
```

이제 `Vertex` 구조체를 사용해 정점 데이터를 명시합니다.
이전과 완전히 동일한 위치와 색상값을 사용하지만 이제는 정점에 대한 배열 하나에 모두 포함해 두었습니다.
이러한 방식을 정점 어트리뷰트의 *interleving*이라고 합니다.

## 바인딩 명세(Binding descriptions)

다음 단계는 GPU 메모리에 업로드된 데이터를 정점 셰이더로 어떻게 전달할지를 Vulkan에 알려주는 것입니다.
이러한 정보를 전달하기 위한 두 종류의 구조체가 필요합니다.

첫 구조체는 `VkVertexInputBindingDescription`이고 `Vertex` 구조체에 멤버 함수를 추가하여 적절한 데이터를 생성할 수 있도록 합니다.

```c++
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        return bindingDescription;
    }
};
```

정점 바인딩은 정점에 대해 얼만큼의 데이터를 메모리로부터 로드할 것인지를 명시합니다.
각 데이터별 바이트의 크기, 그리고 각 정점에 대해 다음 데이터로 넘어갈지, 아니면 다음 인스턴스에서 널어갈지를 포함합니다.

```c++
VkVertexInputBindingDescription bindingDescription{};
bindingDescription.binding = 0;
bindingDescription.stride = sizeof(Vertex);
bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
```

우리의 정점별 데이터는 하나의 배열에 포장되어(packed) 있으니 바인딩은 하나만 있으면 됩니다.
`binding` 매개변수는 바인딩 배열의 바인딩할 인덱스를 명시합니다.
`stride` 매개변수는 한 요소와 다음 요소 사이의 바이트 크기입니다.
`inputRate` 매개변수는 아래와 같은 값 중 하나를 가집니다:

* `VK_VERTEX_INPUT_RATE_VERTEX`: 각 정점에 대해 다음 데이터 요소로 이동함
* `VK_VERTEX_INPUT_RATE_INSTANCE`: 각 인스턴스에 대해 다음 데이터 요소로 넘어감

인스턴스 렌더링을 한 것은 아니므로 정점별 데이터로 해 두겠습니다.

## 어트리뷰트 명세

정점 입력을 처리하는 방법을 설명하기 위한 두 번째 구조체는 `VkVertexInputAttributeDescription`입니다. 
이 구조체를 채우기 위해 또 다른 헬퍼 함수를 `Vertex`에 추가하겠습니다.

```c++
#include <array>

...

static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    return attributeDescriptions;
}
```

함수 프로토타입에서 알 수 있듯, 두 개의 구조체를 사용할 것입니다.
어트리뷰트 명세를 위한 구조체는 바인딩 명세를 활용해 얻어진 정점 데이터 덩어리로부터 정점 어트리뷰트를 어떻게 추출할지를 알려줍니다.
우리는 위치와 색상 두 개의 어트리뷰트가 있으니 두 개의 어트리뷰트 명세 구조체가 필요합니다.

```c++
attributeDescriptions[0].binding = 0;
attributeDescriptions[0].location = 0;
attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
attributeDescriptions[0].offset = offsetof(Vertex, pos);
```

`binding` 매개변수는 어떤 바인딩에서 정점별 데이터를 얻어올 것인지를 Vulkan에 알려줍니다.
`location` 매개변수는 정점 셰이더의 `location` 지시자에 대한 참조입니다.
정점 셰이더의 location `0`에 대한 입력이 위치값에 해당하고, 이는 두 개의 32비트 부동소수점으로 이루어져 있습니다.

`format` 매개변수는 어트리뷰트의 데이터 자료형을 알려줍니다.
약간 헷갈리는 점은 이러한 포맷이 색상 포맷과 동일한 열거자로 명시된다는 점입니다.
아래와 같은 셰이더 자료형에 따르는 포맷이 사용됩니다:

* `float`: `VK_FORMAT_R32_SFLOAT`
* `vec2`: `VK_FORMAT_R32G32_SFLOAT`
* `vec3`: `VK_FORMAT_R32G32B32_SFLOAT`
* `vec4`: `VK_FORMAT_R32G32B32A32_SFLOAT`

보다시피 색상 채널의 수와 일치하는 요소 숫자를 갖는 셰이더 자료형의 포맷을 사용해야 합니다.
셰이더의 요소 숫자보다 더 많은 채널을 사용하는 것도 허용되지만 남는 값은 무시됩니다.
요소 숫자보다 채널 수가 적으면 BGA 요소의 기본값인 `(0, 0, 1)`가 사용됩니다. 
색상 타입인 (`SFLOAT`, `UINT`, `SINT`)와 비트 너비 또한 셰이더 입력의 자료형과 일치해야 합니다.
예시는 다음과 같습니다:

* `ivec2`: `VK_FORMAT_R32G32_SINT`, 32비트 부호 있는 정수 2개 요소를 갖는 벡터
* `uvec4`: `VK_FORMAT_R32G32B32A32_UINT`, 32비트 부호 없는 정수 4개의 요소를 갖는 벡터
* `double`: `VK_FORMAT_R64_SFLOAT`, 64비트 double 부동소수점

`format` 매개변수는 어트리뷰트 데이터의 바이트 크기를 암시적으로 정의하며 `offset` 매개변수는 정점별 데이터를 읽어올 시작 바이트를 명시합니다.
바인딩은 한 번에 하나의 `Vertex`를 읽어오며 위치 어트리뷰트(`pos`)는 `0` 바이트, 즉 처음부터 읽어옵니다. 
`offsetof` 매크로를 사용하면 자동으로 계산됩니다.

```c++
attributeDescriptions[1].binding = 0;
attributeDescriptions[1].location = 1;
attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
attributeDescriptions[1].offset = offsetof(Vertex, color);
```

색상 어트리뷰트도 동일한 방식으로 기술됩니다.

## 파이프라인 정점 입력

이제 `createGraphicsPipeline` 안의 구조체를 참조하여 정점 데이터를 위와 같은 포맷으로 받아들이도록 그래픽스 파이프라인을 설정해야 합니다.
`vertexInputInfo` 구조체를 찾아 두 명세를 참조하도록 수정합니다:

```c++
auto bindingDescription = Vertex::getBindingDescription();
auto attributeDescriptions = Vertex::getAttributeDescriptions();

vertexInputInfo.vertexBindingDescriptionCount = 1;
vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
```

이제 이 파이프라인은 `vertices` 컨테이너의 정점 데이터를 받아들이고 정점 셰이더로 넘길 준비가 되었습니다.
검증 레이어를 활성화 한 상태에서 프로그램을 실행하면 바인딩된 정점 버퍼가 없다는 오류 메시지를 보시게 될겁니다.
다음 단계는 정점 버퍼를 만들고 정점 데이터를 버퍼에 넘겨 GPU가 접근할 수 있도록 하는 것입니다.

[C++ code](/code/18_vertex_input.cpp) /
[Vertex shader](/code/18_shader_vertexbuffer.vert) /
[Fragment shader](/code/18_shader_vertexbuffer.frag)
