## 서론

이제 텍스처가 입혀진 3D 메쉬를 렌더링할 준비가 되었지만 지금의 `vertices`와 `indices` 배열에 정의된 형상은 좀 재미가 없습니다. 이 챕터에서는 프로그램을 확장해서 실제 모델 파일로부터 정점과 인덱스를 로드하여 그래픽 카드가 좀 더 실질적인 작업을 하도록 만들어 보겠습니다.

많은 그래픽스 API 튜토리얼에서는 이러한 챕터에서 직접 OBJ 로더를 작성합니다. 문제는 실제 3D 응용 프로그램을 만들다면 이 포맷에서 지원하지 않는, 예를 들자면 스켈레톤 애니메이션 같은 기능을 얼마 지나지 않아 필요로 하게 된다는 것입니다. 이 챕터에서 우리도 역시 OBJ 모델로부터 메쉬 데이터를 로딩할 것이지만, 파일에서 이러한 데이터를 어떻게 로딩하는지보다는 실제 프로그램에서 어떻게 메쉬 데이터를 사용하도록 해야 하는지에 대해서 집중하도록 하겠습니다.

## 라이브러리

우리는 [tinyobjloader](https://github.com/syoyo/tinyobjloader) 라이브러리를 사용해 OBJ 파일로부터 정점과 표면(face) 정보를 로드할 것입니다. stb_image처럼 하나의 파일로 된 라이브러리기 때문에 빠르고, 프로그램에 통합하기도 쉽습니다. 위 링크의 레포지토리로 가서 `tiny_obj_loader.h` 파일을 여러분의 라이브러리 리렉토리에 다운로드 하십시오.

**비주얼 스튜디오**

`tiny_obj_loader.h`가 있는 디렉토리를 `추가 포함 디렉토리` 경로에 추가하세요.

![](/images/include_dirs_tinyobjloader.png)

**Makefile**

`tiny_obj_loader.h`가 있는 디렉토리는 GCC의 include 리렉토리로 추가하세요:

```text
VULKAN_SDK_PATH = /home/user/VulkanSDK/x.x.x.x/x86_64
STB_INCLUDE_PATH = /home/user/libraries/stb
TINYOBJ_INCLUDE_PATH = /home/user/libraries/tinyobjloader

...

CFLAGS = -std=c++17 -I$(VULKAN_SDK_PATH)/include -I$(STB_INCLUDE_PATH) -I$(TINYOBJ_INCLUDE_PATH)
```

## 예제 메쉬

이 챕터에서 아직 라이팅(lighting)을 적용하진 않을 것이기 때문에 텍스처에 라이팅이 베이크 되어 있는 예제 모델을 사용하는 것이 좋을 것 같습니다. 쉬운 방법 중 하나는 [Sketchfab](https://sketchfab.com/)에서 3D 스캐닝 모델을 찾는 것입니다. 이 사이트의 많은 모델들이 OBJ 포맷을 허용적 라이센스(permissive license)로 제공합니다.

이 튜토리얼에서는 [nigelgoh](https://sketchfab.com/nigelgoh)이 모델링한 [Viking room](https://sketchfab.com/3d-models/viking-room-a49f1b8e4f5c4ecf9e1fe7d81915ad38)을 사용하기로 했습니다 ([CC BY 4.0](https://web.archive.org/web/20200428202538/https://sketchfab.com/3d-models/viking-room-a49f1b8e4f5c4ecf9e1fe7d81915ad38)). 크기와 자세를 바꾸어서 현재 형상을 바로 대체할 수 있도록 했습니다:

* [viking_room.obj](/resources/viking_room.obj)
* [viking_room.png](/resources/viking_room.png)

다른 모델을 사용해도 되지만 하나의 머티리얼(material)로만 구성되고, 크기가 대략 1.5 x 1.5 x 1.5 여야 합니다. 이보다 큰 경우에는 뷰 행렬을 수정해야 합니다. `shaders`와 `textures` 디렉토리와 같은 위치에 `models` 디렉토리를 만들고 모델 파일을 넣으십시오. 그리고 텍스처 이미지는 `texture` 디렉토리에 넣으십시오.

모델과 텍스처 경로에 대한 변수를 프로그램에 추가합니다:

```c++
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";
```

그리고 `createTextureImage`에서 이 경로를 사용하도록 수정합니다:

```c++
stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
```

## 정점과 인덱스 로딩

이제 모델 파일로부터 정점과 인덱스를 로딩할 것입니다. 따라서 전역 선언된 `vertices`와 `indices` 배열은 제거해야 합니다. 이들을 상수가 아닌 컨테이너로 클래스 멤버에 추가합니다:

```c++
std::vector<Vertex> vertices;
std::vector<uint32_t> indices;
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
```

인덱스의 타입을 `uint16_t`에서 `uint32_t`로 수정해야 하는데 65535보다 훨씬 많은 정점이 존재하기 때문입니다. `vkCmdBindIndexBuffer` 매개변수도 바꾸는 것을 잊지 마세요:

```c++
vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
```

tinyobjloader 라이브러리는 STB 라이브러리와 같은 방식으로 include됩니다. `tiny_obj_loader.h` 파일을 include하고 소스 파일 중 하나에 `TINYOBJLOADER_IMPLEMENTATION`를 선언하여 함수 본문에 대한 링크 오류가 발생하지 않도록 합니다:

```c++
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
```

이제 이 라이브러리를 사용해 메쉬의 정점 데이터를 `vertices`와 `indices` 컨테이너에 담는 `loadModel` 함수를 만들 것입니다. 이 함수는 정점과 인덱스 버퍼가 생성되기 이전에 호출되어야 합니다:

```c++
void initVulkan() {
    ...
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    ...
}

...

void loadModel() {

}
```

`tinyobj::LoadObj` 함수를 호출하면 모델이 라이브러리의 데이터 구조에 담겨 로딩됩니다:

```c++
void loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }
}
```

OBj 파일은 위치, 법선, 텍스처 좌표와 표면(face)으로 구성되어 있습니다 표면은 임의 개의 정점으로 정의되는데 각 정점은 위치, 법선과 텍스처 좌표의 인덱스를 참조합니다. 이러한 방식을 통해 정점 전체를 재사용하는 것 뿐만 아니라 개별 속성을 재사용하는 것도 가능합니다.

`attrib` 컨테이너는 전체 위치, 법선, 텍스처 좌표를 `attrib.vertices`, `attrib.normals`, `attrib.texcoords` 벡터에 저장하고 있습니다. `shapes` 컨테이너는 모든 개별 물체와 물체를 구성하는 표면을 가지고 있습니다. 각 표면은 정점의 배열로 이루어지며 각 정점은 위치, 법선, 텍스처 좌표 속성의 인덱스를 가지고 있습니다. OBJ 모델은 각 표면별 텍스처와 머티리얼을 정의할 수 있게 되어 있는데 지금은 이를 무시할 것입니다.

`err` 문자열은 파일을 로딩하는 과정에서 발생한 오류를, `warn` 문자열은 경고를 가지고 있습니다. 예를 들자면 머티리얼 정의가 없는 경우 등이 있을 것입니다. `LoadObj` 함수가 `false`를 반환하는 경우가 실제 로딩이 실패한 경우입니다. 위에서 이야기한 것처럼 OBJ 파일에서 표면은 임의의 정점을 가질 수 있지만, 우리 프로그램에서는 삼각형만 그릴 것입니다. 다행히 `LoadObj` 함수는 표면을 자동으로 삼각화(triangulate) 해 주는 기능을 사용하기 위한 매개변수가 있고, 이는 사용하는 것이 기본값으로 되어 있습니다.

파일에 들어있는 모든 표면을 하나의 모델로 만들 것이기 때문에 shape을 반복(iterate)합니다:

```c++
for (const auto& shape : shapes) {

}
```

삼각화 기능을 통해 각 표면별로 세 개의 정점을 가지는 것이 보장되어 있기 때문에 정점을 반복하며 `vertices` 벡터에 집어 넣습니다:

```c++
for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
        Vertex vertex{};

        vertices.push_back(vertex);
        indices.push_back(indices.size());
    }
}
```

For simplicity, we will assume that every vertex is unique for now, hence the
simple auto-increment indices.. `index` 변수는 `tinyobj::index_t` 타입인데, 이는 `vertex_index`, `normal_index`, `texcoord_index` 멤버를 가집니다. 이 인덱스를 사용해서 `attrib` 배열로부터 실제 정점 어트리뷰트를 가져옵니다:

```c++
vertex.pos = {
    attrib.vertices[3 * index.vertex_index + 0],
    attrib.vertices[3 * index.vertex_index + 1],
    attrib.vertices[3 * index.vertex_index + 2]
};

vertex.texCoord = {
    attrib.texcoords[2 * index.texcoord_index + 0],
    attrib.texcoords[2 * index.texcoord_index + 1]
};

vertex.color = {1.0f, 1.0f, 1.0f};
```

안타깝게도 `attrib.vertices` 배열은 `glm::vec3`와 같은 것이 아니고 `float`의 배열입니다. 따라서 익덱스에 `3`을 곱해 주어야 합니다. 또한 텍스처 좌표 요소는 두 개씩 들어 있습니다. X, Y, Z 요소 또는 텍스처 좌표의 경우 U, V 요소에 대해서 `0`, `1`, `2`의 오프셋을 가집니다.

최적화를 수행한 상태(예를들어 비주얼 스튜디오에서는 `Release` 모드, GCC에서는 `-O3` 컴파일 플래스를 사용)에서 프로그램을 실행해 보세요. 이렇게 하지 않으면 모델을 로딩하는 것이 매우 느릴 겁니다. 그 결과로 아래와 같은 모습이 보일 겁니다:

![](/images/inverted_texture_coordinates.png)

좋습니다. 형상은 맞는 것 같네요. 하지만 텍스처에 무슨 문제가 생긴 걸까요? OBJ 포맷은 수직 좌표에서 `0`이 이미지의 하단이라고 가정하는데 우리는 Vulkan에 이미지를 업로드 할 때 위쪽이 `0`을 의미하도록 정의하였습니다. 텍스처 좌표의 수직 요소를 뒤집어서 이 문제를 해결합니다:

```c++
vertex.texCoord = {
    attrib.texcoords[2 * index.texcoord_index + 0],
    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
};
```

다시 프로그램을 실행하면, 올바른 결과를 볼 수 있습니다:

![](/images/drawing_model.png)


지금까지의 모든 노력이 이제 이 같은 데모를 통해 결실을 얻게 되었습니다!

>모델이 회전되면서 뒤쪽(벽면의 뒷면)이 이상하게 보이는 것을 눈치 채실 수 있을 겁니다. 이는 모델 자체가 뒤쪽에서 보는 것을 고려하지 많고 만들어졌기 때문이고, 정상적인 현상입니다.

## 중복 정점 제거

아쉽게도 지금은 인덱스 버퍼를 통한 이점을 얻지 못하고 있습니다. `vertices` 벡터는 중복된 정점 데이터가 많은데 많은 정점들이 여러 삼각형에 중복되어 포함되기 때문입니다. 고유한 정점만을 남기고 인덱스 버퍼를 사용해 재사용해야 합니다. 이를 구현하는 간단한 방법은 `map`이나 `unordered_map`을 사용해 고유한 정점과 그에 상응하는 인덱스를 추적하는 것입니다:

```c++
#include <unordered_map>

...

std::unordered_map<Vertex, uint32_t> uniqueVertices{};

for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
        Vertex vertex{};

        ...

        if (uniqueVertices.count(vertex) == 0) {
            uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
            vertices.push_back(vertex);
        }

        indices.push_back(uniqueVertices[vertex]);
    }
}
```

OBJ 파일에서 정점을 읽어올 때마다 동일한 위치와 텍스처 좌표를 갖는 정점이 이미 존재하는지를 살펴봅니다. 없다면, `vertices`에 추가하고 그 인덱스를 `uniqueVertices` 컨테이너에 추가합니다. 그리고 그 정점의 인덱스를 `indices`에 추가합니다. 이미 존재하는 정점이라면 `uniqueVertices`에서 인덱스를 찾아서 그 인덱스를 `indices`에 저장합니다.

지금 컴파일하면 컴파일이 실패하게 되는데 `Vertex`와 같은 유저가 정의한 타입을 해시 테이블의 키로 사용하기 위해서는 두 가지 기능을 구현해야 하기 때문입니다. 동일성 테스트와 해시 계산 기능을 구현해야 합니다. 전자는 `==`연산자를 `Vertex` 구조체에 오버라이딩 하면 됩니다:

```c++
bool operator==(const Vertex& other) const {
    return pos == other.pos && color == other.color && texCoord == other.texCoord;
}
```

`Vertex`의 해시 함수는 `std::hash<T>`에 대한 템플릿 특수화를 명시하여 구현할 수 있습니다. 해시 함수는 복잡한 주제이지만 [cppreference.com의 추천](http://en.cppreference.com/w/cpp/utility/hash)에 따라서 아래와 같이 구조체의 필드를 결합해서 괜찮은 해시 함수를 정의할 수 있습니다:

```c++
namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}
```

이 코드는 `Vertex` 구조체 밖에 정의되어야 합니다. GLM 타입에 대한 해시 함수는 아래와 같은 헤더를 include해야 사용할 수 있습니다:

```c++
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
```

해시 함수는 `gtx` 폴더에 정의되어 있는데 이는 이 기능이 사실은 GLM의 실험적 확장 기능이라는 의미입니다. 따라서 사용하기 위해서는 `GLM_ENABLE_EXPERIMENTAL`가 정의되어 있어야 합니다. 또한 이 기능은 나중에 GLM 버전이 바뀌면 API가 바뀔 수 있다는 뜻이기도 하지만 실제로는 API는 상당히 안정적입니다.(*역주: 바뀔 가능성이 적다는 의미*)

이제 컴파일 오류 없이 프로그램을 실행할 수 있습니다. `vertices`의 크기를 확인해 보면 1,500,000에서 265,645로 줄어든 것을 확인할 수 있습니다! 즉 각 정점이 평균적으로 대략 여섯 개의 삼각형에서 재사용되고 있다는 뜻입니다. 이를 통해 GPU 메모리가 상당히 절약되었을 것입니다.

[C++ code](/code/28_model_loading.cpp) /
[Vertex shader](/code/27_shader_depth.vert) /
[Fragment shader](/code/27_shader_depth.frag)
