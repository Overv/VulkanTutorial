이 챕터에서는 Vulkan 응용 프로그램 개발을 위한 환경을 설정하고 몇 가지 유용한 라이브러리를 설치할 것입니다. 우리가 사용할 툴들은 윈도우즈, 리눅스와 MacOS에서 호환되지만 설치 방법은 약간씩 다르기 때문에 개별적으로 설명합니다.

## 윈도우즈

윈도우에서 개발하시는 경우엔 코드 컴파일에는 비주얼 스튜디오를 사용한다고 가정하겠습니다. C++17 지원을 위해서는 비주얼 스튜디오 2017이나 2019가 필요합니다. 아래 설명하는 단계들은 2017을 기준으로 작성되었습니다.

### Vulkan SDK

Vulkan 응용 프로그램 개발을 위해 가장 중요한 구성요소는 SDK입니다. SDK는 헤더, 표준 검증 레이어, 디버깅 도구와 Vulkan 함수의 로더(loader)가 포함되어 있습니다. 로더는 런타임에 드라이버의 함수를 탐색하는 OpenGL에서의 GLEW와 유사한 도구입니다.

SDK는 [LunarG 웹사이트](https://vulkan.lunarg.com/) 페이지 하단의 버튼을 통해 다운로드 할 수 있습니다. 계정을 만드실 필요는 없지만 계정을 만들면 유용하게 활용할 수 있는 추가적인 문서에 접근할 수 있습니다.

![](/images/vulkan_sdk_download_buttons.png)

설치 과정을 거치시고, SDK의 설치 경로를 주의깊게 확인하십시오. 처음으로 할 것은 여러분의 그래픽 카드와 드라이버가 Vulkan을 제대로 지원하는지 확인하는 것입니다. SDK를 설치한 폴더로 가서 `Bin` 디렉터리 안의 `vkcube.exe` 데모를 실행해 보세요. 아래와 같은 화면이 나타나야 합니다:

![](/images/cube_demo.png)

오류 메시지가 나타난다면 드라이버가 최신 버전인지 확인하고, 그래픽 카드가 Vulkan을 지원하고 Vulkan 런타임이 드라이브에 포함되어 있는지 확인하세요. 주요 벤더들의 드라이버 링크는 [introduction](!en/Introduction) 챕터를 확인하세요.

이 폴더에는 개발에 유용한 다른 프로그램들도 있습니다. `glslangValidator.exe`와 `glslc.exe`는 사람이 읽을 수 있는 [GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language) 코드를 바이트 코드로 변환하기 위해 사용됩니다. [shader modules](!en/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules) 챕터에서 이 내용을 자세히 살펴볼 것입니다. `Bin` 디렉터리에는 또한 Vulkan 로더와 검증 레이어의 바이너리들을 포함하고 있으며, `Lib` 디렉터리에는 라이브러리들이 들어 있습니다.

마지막으로 `Include` 디렉터리에는 Vulkan 헤더들이 있습니다. 다른 파일들도 자유롭게 살펴보시길 바라지만 이 튜토리얼에서는 필요하지 않습니다.

### GLFW

앞서 언급한 것처럼 Vulkan은 플랫폼 독립적인 API여서 렌더링 결과를 표시할 윈도우 생성을 위한 도구 같은것은 포함되어 있지 않습니다. Vulkan의 크로스 플랫폼 이점을 살리면서도 Win32의 어려움을 회피하는 방법으로 우리는 [GLFW library](http://www.glfw.org/)를 사용하여 윈도우를 만들 것입니다. GLFW는 윈도우, 리눅스와 MacOS를 모두 지원합니다. 비슷한 목적으로 사용 가능한 [SDL](https://www.libsdl.org/)과 같은 라이브러리도 있지만, GLFW는 윈도우 생성뿐만 아니라 Vulkan의 다른 추가적인 플랫폼 의존적인 작업들에 대한 추상화도 제공해 준다는 것입니다.

GLFW의 최신 버전을 [공식 웹사이트](http://www.glfw.org/download.html)에서 찾을 수 있습니다. 이 튜토리얼에서는 64비트 바이너리를 사용할 것인데 32비트 모드로 빌드하셔도 됩니다. 그런 경우 Vulkan SDK의 `Lib` 디렉터리 대신 `Lib32` 디렉터리의 라이브러리들을 링크하셔야 합니다. 다운로드 하시고 나서 편한 곳에 압축을 푸십시오. 저는 내 문서 아래의 비주얼 스튜디오 디렉터리 아래 `Libraries` 폴더를 만들어 그 곳에 넣었습니다.

![](/images/glfw_directory.png)

### GLM

DirectX 12와는 다르게, Vulkan은 선형대수 연산을 위한 라이브러리가 포함되어 있지 않아서 다운로드 해야 합니다. [GLM](http://glm.g-truc.net/)은 그래픽스 API를 위해 설계된 좋은 라이브러리로 OpenGL에서도 자주 사용됩니다.

GLM은 헤더만으로 구성된 라이브러리로, [최신 버전](https://github.com/g-truc/glm/releases)을 다운로드하고 적절한 위치에 가져다 놓으세요. 그러면 아래와 같은 디렉터리 구조가 될 겁니다:

![](/images/library_directory.png)

### 비주얼 스튜디오 설정

필요한 의존성(dependencies)를 설치했으므로 Vulkan 개발을 위한 비주얼 스튜디오 프로젝트를 설정하고 모든 것들이 올바로 동작하는지 확인하기 위한 짧은 코드를 작성해 보겠습니다.

비주얼 스튜디오를 실행하고 `Windows Desktop Wizard` 프로젝트를 선택한 뒤 이름을 설정하고 `OK`를 누르세요.

![](/images/vs_new_cpp_project.png)

`Console Application (.exe)`를 선택해서 우리의 응용 프로그램이 디버깅 메시지를 표시할 수 있도록 하고 `Empty Project`로 비주얼 스튜디오가 보일러플레이트(boilerplate) 코드를 생성하지 않도록 하세요.

![](/images/vs_application_settings.png)

`OK`를 눌러 프로젝트를 만들고 C++ 소스 파일을 추가 하세요. 어떻게 하는지 알고 계실 테지만, 하는 법을 알려 드리겠습니다.

![](/images/vs_new_item.png)

![](/images/vs_new_source_file.png)

이제 아래 코드를 파일에 추가 하세요. 지금은 이해하려 하실 필요 없습니다. 그냥 Vulkan 응용 프로그램을 컴파일하고 실행할 수 있는지 확인하세요. 다음 챕터에서 다시 처음부터 시작할 것입니다.

```c++
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
```

이제 프로젝트 설정을 통해 오류를 해결해 봅시다. 프로젝트 설정 창을 열고 `All Configurations`이 선택되어 있는지 확인하세요. 대부분의 세팅이 `Debug`와 `Release` 모드에 공통적으로 해당됩니다:

![](/images/vs_open_project_properties.png)

![](/images/vs_all_configs.png)

`C++ -> General -> Additional Include Directories`로 가셔서 드롭다운 메뉴에서 `<Edit...>`을 누르세요:

![](/images/vs_cpp_general.png)

Vulkan, GLFW, GLM을 위한 헤더 디렉터리를 추가하세요:

![](/images/vs_include_dirs.png)

다음으로 `Linker -> General`에서 추가 라이브러리 디렉터리 설정창을 여세요:

![](/images/vs_link_settings.png)

그리고 Vulkan과 GLFW를 위한 오브젝트 파일의 위치를 추가하세요:

![](/images/vs_link_dirs.png)

`Linker -> Input`에서 `Additional Dependencies`의 `<Edit...>`의 드롭다운 메뉴를 누르세요:

![](/images/vs_link_input.png)

Vulkan과 GLFW의 오브젝트 파일 이름을 추가하세요:

![](/images/vs_dependencies.png)

마지막으로 컴파일러가 C++17 기능을 지원하도록 설정하세요:

![](/images/vs_cpp17.png)

이제 프로젝트 설정 창을 닫아도 됩니다. 모두 제대로 설정되었으면 더이상 에러 메시지가 나타나지 않을 겁니다.

마지막으로 64비트 모드에서 컴파일을 하는지 확인하시고:

![](/images/vs_build_mode.png)

`F5`를 눌러 컴파일 후 실행을 해 보면 명령 창(command prompt)과 윈도우가 아래처럼 나타나는 것을 볼 수 있을 겁니다:

![](/images/vs_test_window.png)

extention의 숫자는 0이 아니어야 합니다. 축하합니다. [Vulkan을 즐기기 위한](!en/Drawing_a_triangle/Setup/Base_code)! 모든 준비가 완료되었습니다.

## 리눅스

이 가이드는 우분투, 페도라와 Arch 리눅스 유저를 대상으로 하지만, 패키지 매니저별로 다른 명령어만 사용하시면 그대로 따라하시면 됩니다. C++17을 지원하는 컴파일러 (GCC 7+ 또는 Clang 5+)를 사용하셔야 합니다. `make`도 필요합니다.

### Vulkan 패키지

리눅스에서의 Vulkan 응용 프로그램 개발을 위해 가장 중요한 구성요소는 Vulkan 로더, 검증 레이어와 여러분의 기기가 Vulkan을 지원하는지 테스트하기 위한 몇 개의 명령줄 유틸리티들입니다.

- `sudo apt install vulkan-tools` 또는 `sudo dnf install vulkan-tools`: 명령줄 유틸리티로, 가장 중요한 것은 `vulkaninfo`와 `vkcube`입니다. 기기가 Vulkan을 지원하는지 확인하기 위해 실행해 보십시오.
- `sudo apt install libvulkan-dev` 또는 `sudo dnf install vulkan-loader-devel` : Vulkan 로더를 설치합니다. 로더는 OpenGL에서의 GLEW처럼, 런타임에 드라이버의 함수들을 탐색합니다.
- `sudo apt install vulkan-validationlayers-dev spirv-tools` 또는 `sudo dnf install mesa-vulkan-devel vulkan-validation-layers-devel`: 표준 검증 레이어와 필요한 SPIR-V 도구들을 설치합니다. Vulkan 응용 프로그램을 디버깅하기 위해 필수적이고, 이어지는 챕터에서 자세히 다룰 것입니다.

Arch 리눅스에서는 위 도구들을 설치하기 위해서는 `sudo pacman -S vulkan-devel`를 실행하면 됩니다.

성공적으로 설치가 되었다면, Vulkan 관련 부분은 완료된 것입니다. `vkcube`를 실행해서 아래와 같은 윈도우가 나타나는 것을 확인하십시오.

![](/images/cube_demo_nowindow.png)

오류 메시지가 나타난다면 드라이버가 최신 버전인지 확인하고, 그래픽 카드가 Vulkan을 지원하고 Vulkan 런타임이 드라이브에 포함되어 있는지 확인하세요. 주요 벤더들의 드라이버 링크는 [introduction](!en/Introduction) 챕터를 확인하세요.

### X Window System and XFree86-VidModeExtension

이 라이브러리들이 시스템에 없을 수도 있습니다. 그런 경우엔 다음 명령어를 사용해 설치할 수 있습니다.

- `sudo apt install libxxf86vm-dev` 또는 `dnf install libXxf86vm-devel`: XFree86-VidModeExtension에 대한 인터페이스를 제공합니다.
- `sudo apt install libxi-dev` or `dnf install libXi-devel`: X Window System에서 XINPUT 확장에 대한 클라이언트 인터페이스를 제공합니다.

### GLFW

앞서 언급한 것처럼 Vulkan은 플랫폼 독립적인 API여서 렌더링 결과를 표시할 윈도우 생성을 위한 도구 같은것은 포함되어 있지 않습니다. Vulkan의 크로스 플랫폼 이점을 살리면서도 Win32의 어려움을 회피하는 방법으로 우리는 [GLFW library](http://www.glfw.org/)를 사용하여 윈도우를 만들 것입니다. GLFW는 윈도우, 리눅스와 MacOS를 모두 지원합니다. 비슷한 목적으로 사용 가능한 [SDL](https://www.libsdl.org/)과 같은 라이브러리도 있지만, GLFW는 윈도우 생성뿐만 아니라 Vulkan의 다른 추가적인 플랫폼 의존적인 작업들에 대한 추상화도 제공해 준다는 것입니다.

다음 명령문을 통해 GLFW를 설치할 것입니다.:

```bash
sudo apt install libglfw3-dev
```

or

```bash
sudo dnf install glfw-devel
```

or

```bash
sudo pacman -S glfw-wayland # glfw-x11 for X11 users
```

### GLM

DirectX 12와는 다르게, Vulkan은 선형대수 연산을 위한 라이브러리가 포함되어 있지 않아서 다운로드 해야 합니다. [GLM](http://glm.g-truc.net/)은 그래픽스 API를 위해 설계된 좋은 라이브러리로 OpenGL에서도 자주 사용됩니다.

GLM은 헤더만으로 구성된 라이브러리로, `libglm-dev` 또는 `glm-devel` 패키지를 통해 설치 가능합니다:

```bash
sudo apt install libglm-dev
```

또는

```bash
sudo dnf install glm-devel
```

또는

```bash
sudo pacman -S glm
```

### 셰이더 컴파일러

필요한 것들이 거의 다 준비되었는데, 사람이 읽을 수 있는 [GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language)를 바이트 코드로 컴파일해 주는 프로그램은 아직 설치되지 않았습니다.

두 가지 유명한 셰이더 컴파일러는 크로노스 그룹의 `glslangValidator`와 구글의 `glslc`입니다. 후자는 우리에게 익숙한 GCC와 Clang과 유사한 사용법을 제공하기 때문에 그것을 사용할 것입니다. 우분투에서, 구글의 [공식 바이너리](https://github.com/google/shaderc/blob/main/downloads.md)를 다운로드 하고 `glslc`를 `/usr/local/bin`에 복사하십시오. 권한에 따라 `sudo`를 사용해야 할 수 있습니다. 페도라에서는 `sudo dnf install glslc`, Arch 리눅스에서는 `sudo pacman -S shaderc`를 사용하십시오. 테스트를 위해 `glslc`를 실행하면 컴파일할 셰이더를 올바로 전달하지 않았다는 메시지가 나타날 겁니다.

`glslc: error: no input files`

[shader modules](!en/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules) 챕터에서 `glslc`를 자세히 살펴볼 것입니다.

### makefile 프로젝트 구성

필요한 의존성(dependencies)들을 모두 설치하였으니 Vulkan을 위한 기본 makefile 프로젝트를 만들고 약간의 코드 작성을 통해 모든 것들이 올바로 동작하는지 확인해 봅시다.

`VulkanTest`와 같은 새 디렉터리를 편한 위치에 만들고 `main.cpp` 소스 파일을 생성한 뒤, 다음 코드를 삽입하세요. 지금은 이해하려 하지 마시고, Vulkan 응용 프로그램을 컴파일하고 실행할 수 있는지만 확인하시면 됩니다. 다음 챕터에서 처음부터 다시 시작할 것입니다.

```c++
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
```

다음으로 기본 Vulkan 코드를 컴파일하고 실행하기 위한 makefile을 작성할 것입니다. `Makefile`이라는 이름으로 새 파일을 만드십시오. 저는 여러분들이 기본적인 makefile 사용 경험이 있다고 가정할 것입니다. 예를 들어 변수(variable)와 규칙(rule)이 어떻게 동작하는지 등을 이야기하는 것입니다. 그렇지 않으면, [이 튜토리얼](https://makefiletutorial.com/)을 통해 빠르게 살펴 보십시오.

우선 나머지 부분을 간략하 하기 위해 몇 가지 변수를 정의할 것입니다. 기본 컴파일러 플래그를 명시하기 위해 `CFLAGS` 변수를 정의합니다.

```make
CFLAGS = -std=c++17 -O2
```

모던(modern) C++ (`-std=c++17`)를 사용할 것이고, 최적화 수준(optimization level)을 O2로 설정할 것입니다. 빠른 컴파일을 위해 -O2를 제거할 수도 있지만, 릴리즈(release) 빌드에서는 포함해야 한다는 것을 잊으면 안됩니다.

비슷하게 `LDFLAGS` 변수로 링커 플래그를 정의합니다.

```make
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
```

GLFW를 위해 `-lglfw` 플래그를, Vulkan 함수 로더를 위해 `-lvulkan`를 사용하고, 나머지는 GLFW가 필요로 하는 저수준(low-level) 시스템 라이브러리들입니다. 나머지 플래그들은 GLFW의 의존성들은 쓰레딩과 윈도우 관리와 관련한 플래그들입니다.

`Xxf68vm`와 `Xi` 라이브러리가 여러분의 시스템에 아직 설치되어 있지 않을 수 있습니다. 다음 패키지로부터 찾을 수 있습니다:

```bash
sudo apt install libxxf86vm-dev libxi-dev
```

또는

```bash
sudo dnf install libXi-devel libXxf86vm-devel
```

또는

```bash
sudo pacman -S libxi libxxf86vm
```

이제 `VulkanTest`를 위한 컴파일 규칙을 명시하는 것은 쉽습니다. 들여쓰기(indentation)에 스페이스 대신 탭을 사용하는 것을 잊지 마세요.

```make
VulkanTest: main.cpp
	g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS)
```

위와 같은 규칙이 제대로 동작하는 것을, makefile을 저장한 뒤 `make`를 `main.cpp`와 `Makefile`이 있는 디렉터리에서 실행하여 확인하십시오. 그 결과 `VulkanTest` 실행 파일이 생성될 것입니다.

`test`와 `clean` 두 가지 규칙을 더 정의할 것인데, 앞의 것은 실행 파일을 실행하는 것이고 뒤의 것은 생성된 실행 파일을 삭제하는 것입니다.

```make
.PHONY: test clean

test: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest
```

`make test`를 실행하면 프로그램이 성공적으로 실행될 것이고 Vulkan 확장 숫자를 보여줄 것입니다. 창을 닫으면 성공(`0`) 반환 코드를 반환하면서 응용 프로그램이 종료될 것입니다. 결과적으로 아래와 같은 makefile이 존재하게 됩니다:

```make
CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: main.cpp
	g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest
```

이제 이 디렉터리를 여러분의 Vulkan 프로젝트를 위한 템플릿으로 사용하시면 됩니다. 복사하고 이름을 `HelloTriangle`과 같은 것으로 바꾸고, `main.cpp`의 모든 내용을 지우면 됩니다.

이제 [진정한 탐험](!en/Drawing_a_triangle/Setup/Base_code)을 위한 준비가 끝났습니다.

## MacOS

These instructions will assume you are using Xcode and the [Homebrew package manager](https://brew.sh/). Also, keep in mind that you will need at least MacOS version 10.11, and your device needs to support the [Metal API](<https://en.wikipedia.org/wiki/Metal_(API)#Supported_GPUs>).

### Vulkan SDK

The most important component you'll need for developing Vulkan applications is the SDK. It includes the headers, standard validation layers, debugging tools and a loader for the Vulkan functions. The loader looks up the functions in the driver at runtime, similarly to GLEW for OpenGL - if you're familiar with that.

The SDK can be downloaded from [the LunarG website](https://vulkan.lunarg.com/) using the buttons at the bottom of the page. You don't have to create an account, but it will give you access to some additional documentation that may be useful to you.

![](/images/vulkan_sdk_download_buttons.png)

The SDK version for MacOS internally uses [MoltenVK](https://moltengl.com/). There is no native support for Vulkan on MacOS, so what MoltenVK does is actually act as a layer that translates Vulkan API calls to Apple's Metal graphics framework. With this you can take advantage of debugging and performance benefits of Apple's Metal framework.

After downloading it, simply extract the contents to a folder of your choice (keep in mind you will need to reference it when creating your projects on Xcode). Inside the extracted folder, in the `Applications` folder you should have some executable files that will run a few demos using the SDK. Run the `vkcube` executable and you will see the following:

![](/images/cube_demo_mac.png)

### GLFW

As mentioned before, Vulkan by itself is a platform agnostic API and does not include tools for creation a window to display the rendered results. We'll use the [GLFW library](http://www.glfw.org/) to create a window, which supports Windows, Linux and MacOS. There are other libraries available for this purpose, like [SDL](https://www.libsdl.org/), but the advantage of GLFW is that it also abstracts away some of the other platform-specific things in Vulkan besides just window creation.

To install GLFW on MacOS we will use the Homebrew package manager to get the `glfw` package:

```bash
brew install glfw
```

### GLM

Vulkan does not include a library for linear algebra operations, so we'll have to download one. [GLM](http://glm.g-truc.net/) is a nice library that is designed for use with graphics APIs and is also commonly used with OpenGL.

It is a header-only library that can be installed from the `glm` package:

```bash
brew install glm
```

### Setting up Xcode

Now that all the dependencies are installed we can set up a basic Xcode project for Vulkan. Most of the instructions here are essentially a lot of "plumbing" so we can get all the dependencies linked to the project. Also, keep in mind that during the following instructions whenever we mention the folder `vulkansdk` we are refering to the folder where you extracted the Vulkan SDK.

Start Xcode and create a new Xcode project. On the window that will open select Application > Command Line Tool.

![](/images/xcode_new_project.png)

Select `Next`, write a name for the project and for `Language` select `C++`.

![](/images/xcode_new_project_2.png)

Press `Next` and the project should have been created. Now, let's change the code in the generated `main.cpp` file to the following code:

```c++
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
```

Keep in mind you are not required to understand all this code is doing yet, we are just setting up some API calls to make sure everything is working.

Xcode should already be showing some errors such as libraries it cannot find. We will now start configuring the project to get rid of those errors. On the _Project Navigator_ panel select your project. Open the _Build Settings_ tab and then:

- Find the **Header Search Paths** field and add a link to `/usr/local/include` (this is where Homebrew installs headers, so the glm and glfw3 header files should be there) and a link to `vulkansdk/macOS/include` for the Vulkan headers.
- Find the **Library Search Paths** field and add a link to `/usr/local/lib` (again, this is where Homebrew installs libraries, so the glm and glfw3 lib files should be there) and a link to `vulkansdk/macOS/lib`.

It should look like so (obviously, paths will be different depending on where you placed on your files):

![](/images/xcode_paths.png)

Now, in the _Build Phases_ tab, on **Link Binary With Libraries** we will add both the `glfw3` and the `vulkan` frameworks. To make things easier we will be adding the dynamic libraries in the project (you can check the documentation of these libraries if you want to use the static frameworks).

- For glfw open the folder `/usr/local/lib` and there you will find a file name like `libglfw.3.x.dylib` ("x" is the library's version number, it might be different depending on when you downloaded the package from Homebrew). Simply drag that file to the Linked Frameworks and Libraries tab on Xcode.
- For vulkan, go to `vulkansdk/macOS/lib`. Do the same for the both files `libvulkan.1.dylib` and `libvulkan.1.x.xx.dylib` (where "x" will be the version number of the the SDK you downloaded).

After adding those libraries, in the same tab on **Copy Files** change `Destination` to "Frameworks", clear the subpath and deselect "Copy only when installing". Click on the "+" sign and add all those three frameworks here aswell.

Your Xcode configuration should look like:

![](/images/xcode_frameworks.png)

The last thing you need to setup are a couple of environment variables. On Xcode toolbar go to `Product` > `Scheme` > `Edit Scheme...`, and in the `Arguments` tab add the two following environment variables:

- VK_ICD_FILENAMES = `vulkansdk/macOS/share/vulkan/icd.d/MoltenVK_icd.json`
- VK_LAYER_PATH = `vulkansdk/macOS/share/vulkan/explicit_layer.d`

It should look like so:

![](/images/xcode_variables.png)

Finally, you should be all set! Now if you run the project (remembering to setting the build configuration to Debug or Release depending on the configuration you chose) you should see the following:

![](/images/xcode_output.png)

The number of extensions should be non-zero. The other logs are from the libraries, you might get different messages from those depending on your configuration.

You are now all set for [the real thing](!kr/Drawing_a_triangle/Setup/Base_code).
