## 소개

이 튜토리얼은 [Vulkan](https://www.khronos.org/vulkan/) 그래픽스와 계산 API를 사용하는 기본적인 내용을 알려 드립니다. Vulkan은 (OpenGL로 잘 알려진) [Khronos group](https://www.khronos.org/)에서 만든 새로운 API로 최신 그래픽카드에 대한 훨씬 잘 추상화된 API를 제공합니다. 이 새로운 인터페이스는 여러분의 응용 프로그램이 무엇을 하는 것인지를 더 잘 설명하게 해 주고, 이를 통해 기존 [OpenGL](https://en.wikipedia.org/wiki/OpenGL)
및 [Direct3D](https://en.wikipedia.org/wiki/Direct3D)보다 높은 성능과 더 투명한 드라이버의 동작을 보장합니다. Vulkan의 배경이 되는 아이디어는 [Direct3D 12](https://en.wikipedia.org/wiki/Direct3D#Direct3D_12)
나 [Metal](<https://en.wikipedia.org/wiki/Metal_(API)>)과 비슷하지만, Vulkan은 완전한 크로스 플랫폼을 보장하여 윈도우즈, 리눅스, 안드로이드에서 모두 동작하는 응용 프로그램을 개발할 수 있게 합니다.

하지만, 이러한 이점을 활용하기 위해 여러분이 지불해야 할 비용은 훨씬 장황한 API를 다루어야 한다는 것입니다. 응용 프로그램에서 모든 그래픽스 API와 관련된 상세 사항들을 처음부터 설정해야 하는데, 초기 프레임 버퍼 생성이나 버퍼나 텍스처 이미지 객체들을 위한 메모리 관리 시스템을 만드는 것 등입니다. 그래픽 드라이버가 해 주는 일이 적어서 여러분의 응용 프로그램이 제대로 동작하기 위해서는 직접 더 많은 작업을 해 주어야 합니다.

여기서 말하고자 하는 것은 Vulkan이 모든 사람들을 위해 만들어진 것은 아니라는 점입니다. Vulkan은 고성능 컴퓨터 그래픽스에 관심이 있고, 여기에 시간을 투자할 의지가 있는 프로그래머를 그 대상으로 하고 있습니다. 여러분이 컴퓨터 그래픽스보다는 게임 개발에 더 관심이 있다면, 그냥 OpenGL이나 Direct3D를 계속 사용하는 것이 더 나을 것입니다. Vulkan이 짧은 시간 내에 그 자리를 대체하지는 않을 겁니다. 다른 대안으로는 [Unreal Engine](https://en.wikipedia.org/wiki/Unreal_Engine#Unreal_Engine_4)
이나 [Unity](<https://en.wikipedia.org/wiki/Unity_(game_engine)>) 같은 게임 엔진을 사용하는 것입니다. 게임 엔진을 사용하면 훨씬 고수준의 API를 통해 Vulkan을 사용 가능합니다.

그럼 각설하고, 이 튜토리얼을 위해 준비해야 할 사항들은 다음과 같습니다:

- Vulkan에 호환되는 그래픽 카드와 드라이버 ([NVIDIA](https://developer.nvidia.com/vulkan-driver), [AMD](http://www.amd.com/en-us/innovations/software-technologies/technologies-gaming/vulkan), [Intel](https://software.intel.com/en-us/blogs/2016/03/14/new-intel-vulkan-beta-1540204404-graphics-driver-for-windows-78110-1540), [Apple Silicon (Or the Apple M1)](https://www.phoronix.com/scan.php?page=news_item&px=Apple-Silicon-Vulkan-MoltenVK))
- C++ 경험(RAII, initializer lists에 익숙해야 합니다.)
- C++17 기능을 지원하는 컴파일러 (Visual Studio 2017+, GCC 7+, 또는 Clang 5+)
- 3D 컴퓨터 그래픽스 경험

이 튜토리얼은 OpenGL이나 Direct3D의 개념에 대한 사전지식을 가정하고 있지는 않지만 3D 컴퓨터 그래픽스에 대한 기본 지식은 필요합니다. 예를 들자면 원근 투영(Perspective projection)에 관한 수학적인 배경 등은 설명하지 않습니다. 컴퓨터 그래픽스 개념에 대한 개념서로 [이 책](https://paroj.github.io/gltut/)을 참고 하십시오. 다른 컴퓨터 그래픽스 관련 자료들은 다음과 같습니다:

- [Ray tracing in one weekend](https://github.com/RayTracing/raytracing.github.io)
- [Physically Based Rendering book](http://www.pbr-book.org/)
- Vulkan은 오픈 소스 [Quake](https://github.com/Novum/vkQuake)와 [DOOM 3](https://github.com/DustinHLand/vkDOOM3)의 엔진에서도 사용되었습니다.

원한다면 C++ 대신 C를 사용할 수도 있지만, 그러려면 다른 선형대수 라이브러리를 사용해야 하고 코드 구조를 스스로 설계하셔야 합니다. 우리는 클래스나 RAII 같은 C++ 기능을 로직(logic)과 리소스의 생애주기 관리를 위해 사용할 것입니다. Rust 개발자를 위한 대안으로 이 튜토리얼의 다음과 같은 버전들이 있습니다: [Vulkano based](https://github.com/bwasty/vulkan-tutorial-rs), [Vulkanalia based](https://kylemayes.github.io/vulkanalia).

다른 프로그래밍 언어를 사용하는 개발자들이 따라오시기 쉽고, 기본 API에 대한 이해를 돕기 위해 Vulkan이 동작하도록 하는 데는 원본 C API를 사용할 것입니다. 하지만 C++ 개발자라면, 새로운 [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp) 바인딩을 사용하시면 특정 종류의 오류를 방지할 수 있고, 몇 가지 지저분한 작업들을 하지 않아도 됩니다.

## E-book

이 튜토리얼을 e-book으로 보고 싶으시면 EPUB나 PDF 버전을 받으시면 됩니다:

- [EPUB](https://vulkan-tutorial.com/resources/vulkan_tutorial_en.epub)
- [PDF](https://vulkan-tutorial.com/resources/vulkan_tutorial_en.pdf)

## 튜토리얼 구조

우리는 Vulkan이 어떻게 동작하는지에 대한 개요와 삼각형을 화면에 그리기 위해 해야 하는 일들을 설명하는 것으로 튜토리얼을 시작할 것입니다. 모든 각각의 상세한 단계들의 목적은 전체적인 그림을 이해하고 나면 좀 더 쉽게 이해가 될 것입니다. 다음으로 [Vulkan SDK](https://lunarg.com/vulkan-sdk/), 선형대수 연산을 위한 [GLM library](http://glm.g-truc.net/), 윈도우 생성을 위한 [GLFW](http://www.glfw.org/)를 포함한 개발 환경을 설정할 것입니다. 이 튜토리얼에서는 Visual Studio에 기반한 윈도우즈에서의 개발 환경, GCC를 활용한 우분투 리눅스에서의 개발 환경 설정을 설명할 것입니다.

그 이후에는 삼각형을 화면에 그리기 위해 해야 하는 Vulkan 프로그램 기본 구성요소들을 구현해 볼 것입니다. 각 챕터들은 대략적으로 아래와 같은 구조를 따릅니다:

- 새로운 개념과 그 목적에 대한 소개
- 그러한 내용을 프로그램으로 작성하기 위해 필요한 API 호출 방법들
- 해당 기능들을 헬퍼 함수로 만드는 추상화 작업

각각의 챕터는 이전 챕터에 이어지는 것으로 쓰여졌지만, 각 챕터는 Vulkan의 특정 기능을 소개하는 개별적인 소개글이라 생각하고 읽으셔도 됩니다. 따라서 이 사이트를 유용한 참조 문서로 생각하셔도 됩니다. Vulkan 함수와 타입에 대한 모든 것들이 명세(specification)와 링크되어 있으니 더 알고 싶으시면 클릭하시면 됩니다. Vulkan은 새로운 API라서 명세 자체에 한계점이 있을 수 있습니다. [이 Khronos repository](https://github.com/KhronosGroup/Vulkan-Docs)에 적극적으로 피드백을 남겨 주세요.

앞서 이야기 한 것처럼 Vulkan API는 여러분들에게 그래픽스 하드웨어에 대한 최대한의 제어권을 제공하는 장황한 API입니다. 이로 인해 텍스처를 생성하는 기본적인 연산도 여러 단계를 거쳐야만 하고 이러한 작업들을 여러 번 반복해야만 합니다. 따라서 우리는 자체적으로 헬퍼 함수들을 만들어 볼 것입니다.

각 챕터마다 해당 단계에 해당하는 전체 코드에 대한 링크를 제공할 것입니다. 코드 구조가 잘 이해되지 않거나, 버그가 있거나, 비교를 해 보고 싶다면 참고 하십시오. 모든 코드는 다양한 제조사의 그래픽 카드에서 올바로 동작하는 것을 테스트 한 상태입니다. 각 챕터에는 또한 코멘트 섹션이 있어서 해당 주제에 대한 질문을 남기실 수 있습니다. 여러분의 플랫폼, 드라이버 버전, 소스 코드, 기대하는 동작과 실제 동작을 남겨서 우리가 여러분을 도울 수 있도록 도와 주세요.

이 튜토리얼은 커뮤니티 활성화를 위한 목적도 있습니다. Vulkan은 아직 신생 API이고 모범 사례들이 아직 확립되지 않았습니다. 튜토리얼이나 사이트 자체에 대한 피드백이 있다면 [GitHub repository](https://github.com/Overv/VulkanTutorial)에 이슈나 풀 리퀘스트(pull request)를 남겨 주세요. 레포지토리를 *watch*하시면 튜토리얼이 업데이트 되면 알림을 받을 수 있습니다.

여러분의 첫 삼각형을 Vulkan을 사용해서 화면에 그리는 의식을 치르고 나면, 선형 변환, 텍스처, 3D 모델 등을 포함할 수 있도록 프로그램을 확장할 것입니다.

전에 그래픽스 API를 사용해 본 적이 있다면, 삼각형 하나를 그리기 위해서 여러 단계의 작업이 필요하다는 것을 아실 겁니다. Vulkan에서도 마찬가지인데, 이러한 개별적인 단계들이 이해하기 어렵지 않고 꼭 필요한 작업임을 알게 되실겁니다. 또 명심하여야 할 것은 단순한 삼각형을 한 번 그리기만 하면, 텍스처링된 3D 모델을 그리는 것은 그리 많은 추가 작업이 필요하지는 않다는 것입니다.

이후 튜토리얼을 따라가다 문제가 있다면, 먼저 FAQ에 동일한 문제가 이미 해결된 적이 있는지부터 확인해 보세요. 그러고 나서도 문제를 해결하지 못했다면, 관련된 챕터의 코멘트 섹션에 편하게 질문을 남겨 주세요.

미래의 고성능 그래픽스 API에 뛰어들 준비가 되셨나요? [출발해 봅시다!](!kr/Overview)

## License

Copyright (C) 2015-2023, Alexander Overvoorde

The contents are licensed under [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/),
unless stated otherwise. By contributing, you agree to license
your contributions to the public under that same license.

The code listings in the `code` directory in the source repository are licensed 
under [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/).
By contributing to that directory, you agree to license your contributions to
the public under that same public domain-like license.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.