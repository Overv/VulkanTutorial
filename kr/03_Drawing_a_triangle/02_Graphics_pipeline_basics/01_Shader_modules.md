기존 API와는 다르게, Vulkan의 셰이더 코드는 [GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language)이나 [HLSL](https://en.wikipedia.org/wiki/High-Level_Shading_Language)과 같은 사람이 읽을 수 있는(human-readable) 문법이 아닌 바이트코드(bytecode) 포맷으로 명시되어야 합니다. 이 바이트코드 포맷은 [SPIR-V](https://www.khronos.org/spir)라 불리며 Vulkan과 OpenCL에서의 사용을 위해 설계되었습니다(둘 다 크로노스(Khronos)의 API). 이를 사용해 그래픽스 및 계산 셰이더 작성이 가능하지만 이 튜토리얼에서는 Vulkan의 그래픽스 파이프라인에 사용되는 셰이더에 포커스를 맞추도록 하겠습니다.

바이트코드를 사용함으로써 얻을 수 있는 장점은 GPU 벤더가 작성하는, 셰이더 코드를 네이티브 코드로 변환하는 컴파일러가 훨씬 간단해진다는 것입니다. 과거의 사례를 봤을 때 사람이 읽을 수 있는 GLSL과 같은 문법에서, 어떤 GPU 벤더들은 표준을 유연하게 해석하는 경우가 있었습니다. 이러한 벤더의 GPU에서 여러분이 일반적이지 않은(non-trivial) 셰이더를 작성하는 경우에, 다른 벤더의 드라이버에서는 여러분의 코드가 문법 오류로 판단된다던지, 더 안좋은 상황에서는 다른 방식으로 동작한다던지 하는 문제가 있을 수 있습니다. SPIR-V와 같은 직관적인 바이트코드 포맷을 사용하면 이러한 문제가 해결될 것으로 바라고 있습니다.

그렇다고 우리가 손으로 바이트코드를 작성해야 한다는 뜻은 아닙니다. 크로노스 자체적으로 GLSL을 SPIR-V로 변환하는 벤더 독립적인 컴파일러를 릴리즈하였습니다. 이 컴파일러는 여러분의 셰이더 코드가 표준에 맞는지를 검증하고 프로그램에 사용할 수 있는 SPIR-V 바이너리를 생성합니다. 또한 이 컴파일러를 라이브러리의 형태로 추가하여 런타임에 SPIR-V를 생성하도록 할 수도 있지만, 이 튜토리얼에서 이 기능을 사용하지는 않을 것입니다. 컴파일러는 `glslangValidator.exe`를 통해 직접 사용할수도 있지만 우리는 구글에서 만든 `glslc.exe`를 사용할 것입니다. `glslc`의 장점은 GCC와 Clang과 같은 유명한 컴파일러와 같은 매개변수 포맷을 사용한다는 점, 그리고 *include*와 같은 부가 기능을 제공하는 점입니다. 둘 다 Vulkan SDK에 포함되어 있으므로 추가적으로 다운로드 할 필요는 없습니다.

GLSL은 C 스타일 문법을 가진 셰이더 언어입니다. GLSL로 작성된 프로그램은 `main`함수가 있어서 모든 객체에 대해 실행됩니다. 입력에 매개변수를 사용하고 출력에 반환값을 사용하는 대신, GLSL은 입력과 출력을 관리하는 전역 변수를 가지고 있습니다. 이 언어는 그래픽스 프로그램을 위한 다양한 기능을 포함하고 있는데 내장 벡터(vector)와 행렬(matrix) 타입이 그 예시입니다. 외적(cross product)이나 행렬-벡터 곱, 벡터를 기준으로 한 반사(reflection) 연산을 위한 함수 또한 포함되어 있습니다. 벡터 타입은 `vec`이라고 물리며 요소의 개수를 명시하는 숫자가 뒤에 붙습니다. 예를 들어 3차원 위치는 `vec3`에 저장됩니다. 개별 요소에 대한 접근은 멤버 접근 연산자처러 `.x`로 접근 가능하지만 여러 요소를 갖는 벡터를 새로 만들수도 있습니다. 예를 들어 `vec3(1.0, 2.0, 3.0).xy`는 결과적으로 `vec2` 입니다. 벡터의 생성자(constructor)는 벡터 객체와 스칼라(scalar)값의 조합을 받을 수 있습니다. 예를 들어 `vec3`가 `vec3(vec2(1.0, 2.0), 3.0)`를 통해 만들어질 수 있습니다.

이전 챕터에서 이야기한 것처럼 삼각형을 화면에 그리기 위해 우리는 정점 셰이더와 프래그먼트 셰이더를 작성해야 합니다. 다음 두 섹션에서 각각의 GLSL 코드를 설명할 것이고 그 이후에는 SPIR-V 바이너리를 만드는 방법과 이를 프로그램에 로드(load)하는 법을 보여드리겠습니다.

## 정점 셰이더

정점 셰이더는 각 입력 정점을 처리합니다. 정점 셰이더는 모델 공간 좌표, 색상, 법선과 텍스처 좌표같은 입력 데이터를 어트리뷰트로 받습니다. 출력은 클립 좌표(clip coordinate) 위치와 프래그먼트 셰이더로 전달할 색상과 텍스처 좌표와 같은 어트리뷰트 들입니다. 이 값들은 래스터화 단계에서 여러 프래그먼트에 걸쳐 부드럽게 변하도록(smooth gradient) 보간됩니다.

*클립 좌표*는 정점 셰이더에서 도출된 4차원 벡터로 벡터를 마지막 구성요소의 값으로 나눔으로써 *정규화된 장치 좌표(normalized device coordinate)*로 변환됩니다. 정규화된 장치 좌표계는 [동차 좌표(homogeneous coordinates)](https://en.wikipedia.org/wiki/Homogeneous_coordinates)로, 아래 그림과 같이 프레임버퍼와 맵핑되는 [-1, 1]x[-1, 1] 좌표계입니다:

![](/images/normalized_device_coordinates.svg)

컴퓨터 그래픽스를 좀 공부하셨다면 이런 것들이 익숙하실 겁니다. OpenGL을 사용해보셨다면 Y좌표가 뒤집혀 있는 것을 눈치채실 겁니다. Z좌표도 이제 Direct3D와 동일하게 0에서 1 사이의 값을 사용합니다.

첫 번째 삼각형 그릴 때, 우리는 아무 변환도 적용하지 않을 것입니다. 그냥 3개 정점의 위치를 정규화된 장치 좌표에서 직접 명시하여 아래와 같은 모양을 만들 것입니다:
![](/images/triangle_coordinates.svg)

정점 셰이더의 클립 좌표에서 마지막 요소를 `1`로 설정하여 정규화된 장치 좌표를 바로 출력되도록 할 수 있습니다. 이렇게 하면 클립 좌표를 정규화된 장치 좌표로 변환해도 아무런 값의 변화가 없을것입니다.

일반적으로 이러한 좌표는 정점 버퍼에서 저장되겠지만 Vulkan에서 정점 버퍼를 생성하고 값을 집어넣는 것은 쉽지 않습니다. 그래서 이러한 작업은 화면에 삼각형을 띄우는 만족스러운 결과 이후로 미루도록 하겠습니다. 정석적인 방법은 아니지만 정점 셰이더에 좌표값을 직접 추가하겠습니다. 코드는 아래와 같습니다:

```glsl
#version 450

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
```

`main` 함수는 모든 정점에 대해 호출됩니다. `gl_VertexIndex` 내장 변수가 현재 정점의 인덱스를 가지고 있습니다. 이는 보통 정점 버퍼의 인덱스이지만 우리의 경우 하드코딩된 정점 데이터의 인덱스를 의미합니다. 각 정점의 위치는 셰이더에 있는 상수 배열로부터 얻어지고, `z`와 `w`값이 합쳐져 클립 좌표값이 됩니다. `gl_Position` 내장 변수가 출력처럼 활용됩니다.

## 프래그먼트 셰이더

정점 셰이더의 위치들로 구성된 삼각형은 화면상의 일정 영역을 프래그먼트로 채우게 됩니다. 프래그먼트 셰이더는 이 프래그먼트들에 대해 실행되어 프레임버퍼(들)의 색상과 깊이 값을 생성합니다. 전체 삼각형에 대해 빨간색을 출력하는 간단한 프래그먼트 셰이더는 아래와 같습니다:

```glsl
#version 450

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
```

정점 셰이더가 모든 정점에 대해 `main` 함수를 호출하는 것처럼, 모든 프래그먼트에 대해 `main`함수가 호출됩니다. GLSL에서 색상은 [0, 1] 범위의 R,G,B와 알파 채널의 4차원 벡터로 표현됩니다. 정점 셰이더의 `gl_Position`과는 다르게, 현재 프래그먼트 출력을 위한 내장 변수는 없습니다. 각 프레임버퍼를 위한 출력 변수는 스스로 명시해야 하며 `layout(location = 0)` 수식어가 프레임버퍼의 인덱스를 명시합니다. 빨간색이 이러한 `outColor` 변수에 쓰여졌고, 이는 첫 번째인(그리고 유일한) `0`번 인덱스 프레임버퍼와 연결되어 있습니다.

## 정점별 색상

삼각형 전체를 빨간색으로 만드는것 재미가 없네요. 아래와 같이 그린다면 훨씬 재미있지 않을까요?

![](/images/triangle_coordinates_colors.png)

이를 위해 두 셰이더 모두에 약간의 수정을 하겠습니다. 먼저 세 개의 정점에 각각 다른 색상을 명시해 주어야 합니다. 이제 정점 셰이더는 위치와 함께 색상을 위한 배열도 가집니다:

```glsl
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);
```

이제 프래그먼트 셰이더에 정점별 색상을 전달해줘서 프레임버퍼에 보간된 색상을 출력하게 하면 됩니다. 정점 셰이더에 색상 출력을 위한 변수를 추가하고 `main`함수에서 값을 쓰면 됩니다:

```glsl
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

다음으로, 프래그먼트 셰이더에는 매칭되는 입력을 추가해야 합니다:

```glsl
layout(location = 0) in vec3 fragColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

입력 변수의 이름이 (정점 셰이더의 출력과) 같은 이름일 필요는 없습니다. 이들은 `location` 지시어에 의해 명시된 인덱스를 기반으로 연결됩니다. `main`함수는 알파값과 함께 색상을 출력하도록 수정되었습니다. 위쪽 이미지에서 본 것처럼, `fragColor`의 값은 자동으로 세 정점 사이에서 보간되어 연속적인 값을 보여줍니다.

## 셰이더 컴파일

프로젝트의 루트(root) 디렉토리에 `shaders`라는 이름의 디렉토리를 만들고 정점 셰이더는 `shader.vert` 파일에, 프래그먼트 셰이더는 `shader.frag`파일에 작성하고 해당 디렉토리에 넣으세요. GLSL 셰이더를 위한 공식적인 확장자는 없지만, 이러한 방식이 그 둘을 구분하기 위해 일반적으로 사용하는 방법입니다.

`shader.vert`의 내용은 아래와 같습니다:

```glsl
#version 450

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

`shader.frag`의 내용은 아래와 같습니다:

```glsl
#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

이제 `glslc` 프로그램을 사용해 이들을 SPIR-V 바이트코드로 만들겁니다.

**윈도우즈**

아래와 같은 내용을 담은 `compile.bat` 파일을 만듭니다:

```bash
C:/VulkanSDK/x.x.x.x/Bin/glslc.exe shader.vert -o vert.spv
C:/VulkanSDK/x.x.x.x/Bin/glslc.exe shader.frag -o frag.spv
pause
```

`glslc.exe`의 경로는 여러분이 Vulkan SDK를 설치한 경로로 설정해 주세요. 그리고 더블클릭하여 실행합니다.

**리눅스**

아래와 같은 내용을 담은 `compile.sh` 파일을 만듭니다:

```bash
/home/user/VulkanSDK/x.x.x.x/x86_64/bin/glslc shader.vert -o vert.spv
/home/user/VulkanSDK/x.x.x.x/x86_64/bin/glslc shader.frag -o frag.spv
```

`glslc.exe`의 경로는 여러분이 Vulkan SDK를 설치한 경로로 설정해 주세요. 그리고 `chmod +x compile.sh`로 실행 파일을 만든 뒤 실행합니다.

**플랫폼별 안내는 여기까지**

위 두 명령어는 `-o` (output) 플래그로 컴파일러에게 GLSL 소스 파일을 읽어서 SPIR-V 바이트코드 파일을 출력하도록 합니다.

여러분의 셰이더에 문법적 오류가 있다면 컴파일러가 해당하는 라인과 문제가 뭔지를 알려줍니다. 예를 들어 세미콜론을 지우고 다시 컴파일 해 보세요. 또한 아무런 인자 없이 컴파일러를 실행해 어떤 플래그들이 지원되는지 살펴 보세요. 예를 들어 바이트코드를 사람이 읽을 수 있는 포맷으로 출력하여 셰이더가 정확히 어떤 일을 하는지도 볼 수 있고, 이 단계에서 어떤 최적화가 적용되는지도 볼 수 있습니다.

셰이더를 명령줄(commandline)로 컴파일하는 것은 가장 직관적인 방법이고 이 튜토리얼에서는 이 방식을 사용할 것이지만, 코드 내에서 셰이더를 컴파일하도록 할 수도 있습니다. Vulkan SDK에는 [libshaderc](https://github.com/google/shaderc)가 포함되어 있는데, 프로그램 내에서 GLSL 코드를 SPIR-V로 컴파일하기 위한 라이브러리입니다.

## 셰이더 로딩

SPIR-V 셰이더를 생성할 방법을 알아봤으니 이제는 그 결과물을 프로그램에 로드하고 이를 그래픽스 파이프라인 어딘가에 꽂아넣을 시간입니다. 먼저 간단한 헬퍼 함수를 만들어 바이너리 데이터를 파일로부터 로드할 수 있도록 합니다.

```c++
#include <fstream>

...

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
}
```

`readFile` 함수는 명시한 파일에서 모든 바이트를 읽어와서 `std::vector`로 저장된 바이트 배열을 반환하도록 할 것입니다. 먼저 두 개의 플래그로 파일을 엽니다.

- `ate`: 파일의 끝에서부터 읽습니다.
- `binary`: 파일을 바이너리로 읽습니다 (텍스트로 변환 방지)

끝에서부터 읽는 경우의 장점은 읽기 위치를 사용해 파일의 크기를 파악하여 버퍼를 할당할 수 있다는 점입니다.

```c++
size_t fileSize = (size_t) file.tellg();
std::vector<char> buffer(fileSize);
```

그러고 나서 파일의 맨 앞까지 탐색하여 모든 바이트를 한 번에 읽어옵니다:

```c++
file.seekg(0);
file.read(buffer.data(), fileSize);
```

마지막으로 파일을 닫고 바이트를 반환합니다:

```c++
file.close();

return buffer;
```

이제 이 함수를 `createGraphicsPipeline`에서 호출하여 두 셰이더의 바이트코드를 로드합니다:

```c++
void createGraphicsPipeline() {
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");
}
```

셰이더가 제대로 로드 되었는지를 버퍼의 크기를 출력하여 파일의 실제 바이트 사이와 일치하는를 통해 확인하세요. 바이너리 코드이기 때문에 널 종료(null terminate)여야 할 필요가 없고 나중에는 이러한 크기를 명시적으로 확인할 것입니다.

## 셰이더 모듈 생성

코드를 파이프라인에 넘기기 전에, `VkShaderModule` 객체로 이들을 감싸야 합니다. 이를 위한 `createShaderModule` 헬퍼 함수를 만듭시다.

```c++
VkShaderModule createShaderModule(const std::vector<char>& code) {

}
```

이 함수는 바이트코드 버퍼를 매개변수로 받아서 `VkShaderModule`를 만들 것입니다.

셰이더 모듈을 만드는 것은 간단합니다. 바이트코드가 있는 버퍼에 대한 포인터와 그 길이를 명시해 주기만 하면 됩니다. 이 정보들은 `VkShaderModuleCreateInfo` 구조체에 명시할 것입니다. 하나 주의할 점은 바이트코드의 크기는 바이트 단위이지만, 바이트코드의 포인터는 `char` 포인터가 아닌 `uint32_t` 포인터라는 것입니다. 따라서 아래 보이는 것처럼 `reinterpret_cast`를 활용해 캐스팅(cast)을 해 주어야 합니다. 이런 식으로 캐스팅을 하게 되면 데이터가 `uint32_t`의 정렬(alignment) 요구사항에 맞는지 확인해 주어야 합니다. 다행히 데이터는 기본 할당자가 정렬 요구사항을 만족하도록 보장되어 있는 `std::vector`에 저장되어 있으니 문제 없습니다.

```c++
VkShaderModuleCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
createInfo.codeSize = code.size();
createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
```

`VkShaderModule`은 `vkCreateShaderModule`를 호출하여 생성됩니다:

```c++
VkShaderModule shaderModule;
if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
}
```

매개변수는 이전 객체 생성 함수와 동일합니다. 논리적 장치, 생성 정보를 담은 구조체에 대한 포인터, 사용자 정의 할당자를 위한 선택적 포인터, 그리고 출력 변수에 대한 핸들입니다. 셰이더 모듈을 생성하고 나면 코드가 담긴 버퍼는 해제되어도 됩니다. 만들어진 셰이더 모듈을 반환하는 것도 잊지 마시고요:

```c++
return shaderModule;
```

셰이더 모듈은 단지 파일로부터 로드한 셰이더 바이트코드를 감싸는 작은 래퍼입니다. GPU에서 실행을 위해 수행하는 SPIR-V 바이트코드의 컴파일과 링킹을 통한 기계 코드로의 변환은 그래픽스 파이프라인이 생성되기 전에는 수행되지 않습니다. 즉, 파이프라인 생성이 완료되면 셰이더 모듈은 소멸되어도 문제가 없고, 그러한 이유로 우리는 이들을 클래스 멤버가 아닌 `createGraphicsPipeline`함수의 지역 변수로 선언할 것입니다:

```c++
void createGraphicsPipeline() {
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
```

정리 과정은 함수의 마지막 부분에 `vkDestroyShaderModule` 함수를 두 번 호출함으로써 이루어집니다. 이 챕터의 나머지 모든 코드는 이 둘 사이에 작성될 것입니다.

```c++
    ...
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}
```

## 셰이더 단계(stage) 생성

셰이더를 실제로 사용하기 위해서는 이들을 `VkPipelineShaderStageCreateInfo` 구조체를 활용하여 파이프라인의 특정 단계에 할당해야 하고, 이 역시 파이프라인 생성 과정의 한 부분입니다.

먼저 정점 셰이더를 위한 구조체를 채우는 것부터 시작할 것인데, 마찬가지로 `createGraphicsPipeline` 함수 안에서 이루어집니다.

```c++
VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
```

첫 단계로 당연히 필요한 `sType` 외에, Vulkan에게 셰이더가 사용될 파이프라인 단계를 알려줍니다. 이전 챕터에서 설명한 각 프로그램가능한 단계를 위한 열거형 값들이 있습니다.

```c++
vertShaderStageInfo.module = vertShaderModule;
vertShaderStageInfo.pName = "main";
```

다음 두 멤버는 코드를 담은 셰이더 모듈과 *진입점(entrypoint)*인 호출할 함수를 명시합니다. 즉 여러 프래그먼트 셰이더들을 하나의 셰이더 모듈로 만들고 서로 다른 진입점을 사용해 다른 동작을 하도록 만들 수도 있습니다. 지금은 그냥 표준적인 `main`을 사용할 것입니다.

마지막 하나의 (선택적인) 멤버는 `pSpecializationInfo`이고, 여기서 사용할 것은 아니지만 언급할 필요는 있습니다. 이 멤버는 셰이더 상수(constant)의 값을 명시할 수 있도록 합니다. 하나의 셰이더 모듈을 만들고 파이프라인 생성 단계에서 사용되는 상수의 값을 다르게 명시하여 다르게 동작하도록 할 수 있습니다. 이렇게 하는 것이 변수를 사용하여 렌더링 시점에 셰이더의 동작을 바꾸는 것보다 효율적인데, 이렇게 하면 컴파일러가 이 값에 의존하는 `if` 분기를 제거하는 등의 최적화를 할 수 있습니다. 이에 해당하는 상수가 없다면 이 값은 `nullptr`로 두면 되고, 지금 우리 코드에서는 자동으로 이렇게 됩니다.

프래그먼트 셰이더를 위해 구조체를 수정하는 것은 쉽습니다:

```c++
VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
fragShaderStageInfo.module = fragShaderModule;
fragShaderStageInfo.pName = "main";
```

이 두 구조체를 포함하는 배열을 정의하는 것이 마지막 단계이고, 실제 파이프라인 생성 단계에서는 이 배열을 사용해 셰이더 모듈들을 참조하도록 할 것입니다.

```c++
VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
```

파이프라인의 프로그램 가능한 단계에 대한 설정은 여기까지입니다. 다음 챕터에서는 고정 함수 단계를 살펴볼 것입니다.

[C++ code](/code/09_shader_modules.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
