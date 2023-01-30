## Introduction

Votre programme peut maintenant réaliser des rendus 3D, mais la géométrie que nous utilisons n'est pas très
intéressante. Nous allons maintenant étendre notre programme pour charger les sommets depuis des fichiers. Votre carte
graphique aura enfin un peu de travail sérieux à faire.

Beaucoup de tutoriels sur les APIs graphiques font implémenter par le lecteur un système pour charger les modèle OBJ. Le
problème est que ce type de fichier est limité. Nous *allons* charger des modèles en OBJ, mais nous nous concentrerons
plus sur l'intégration des sommets dans le programme, plutôt que sur les aspects spécifiques de ce format de fichier.

## Une librairie

Nous utiliserons la librairie [tinyobjloader](https://github.com/syoyo/tinyobjloader) pour charger les vertices et les
faces depuis un fichier OBJ. Elle est facile à utiliser et à intégrer, car elle est contenue dans un seul fichier.
Téléchargez-la depuis le lien GitHub, elle est contenue dans le fichier `tiny_obj_loader.h`.

**Visual Studio**

Ajoutez dans `Additional Include Directories` le dossier dans lequel est contenu `tiny_obj_loader.h`.

![](/images/include_dirs_tinyobjloader.png)

**Makefile**

Ajoutez le dossier contenant `tiny_obj_loader.h` aux dossiers d'inclusions de GCC :

```text
VULKAN_SDK_PATH = /home/user/VulkanSDK/x.x.x.x/x86_64
STB_INCLUDE_PATH = /home/user/libraries/stb
TINYOBJ_INCLUDE_PATH = /home/user/libraries/tinyobjloader

...

CFLAGS = -std=c++17 -I$(VULKAN_SDK_PATH)/include -I$(STB_INCLUDE_PATH) -I$(TINYOBJ_INCLUDE_PATH)
```

## Exemple de modèle

Nous n'allons pas utiliser de lumières pour l'instant. Il est donc préférable de charger un modèle qui comprend les
ombres pour que nous ayons un rendu plus intéressant. Vous pouvez trouver de tels modèles sur
[Sketchfab](https://sketchfab.com/).

Pour ce tutoriel j'ai choisi d'utiliser le [Viking room](https://sketchfab.com/3d-models/viking-room-a49f1b8e4f5c4ecf9e1fe7d81915ad38) créé par [nigelgoh](https://sketchfab.com/nigelgoh) ([CC BY 4.0](https://web.archive.org/web/20200428202538/https://sketchfab.com/3d-models/viking-room-a49f1b8e4f5c4ecf9e1fe7d81915ad38)).
J'en ai changé la taille et l'orientation pour l'utiliser comme remplacement de notre géométrie actuelle :

* [viking_room.obj](/resources/viking_room.obj)
* [viking_room.png](/resources/viking_room.png)

Il possède un demi-million de triangles, ce qui fera un bon test pour notre application. Vous pouvez utiliser un
autre modèle si vous le désirez, mais assurez-vous qu'il ne comprend qu'un seul matériau et que ses dimensions sont
d'approximativement 1.5 x 1.5 x 1.5. Si il est plus grand vous devrez changer la matrice view. Mettez le modèle dans un
dossier appelé `models`, et placez l'image dans le dossier `textures`.

Ajoutez deux variables de configuration pour la localisation du modèle et de la texture :

```c++
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";
```

Changez la fonction `createTextureImage` pour qu'elle utilise cette seconde constante pour charger la texture.

```c++
stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
```

## Charger les vertices et les indices

Nous allons maintenant charger les vertices et les indices depuis le fichier OBJ. Supprimez donc les tableaux
`vertices` et `indices`, et remplacez-les par des vecteurs dynamiques :

```c++
std::vector<Vertex> vertices;
std::vector<uint32_t> indices;
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
```

Il faut aussi que le type des indices soit maintenant un `uint32_t` car nous allons avoir plus que 65535 sommets.
Changez également le paramètre de type dans l'appel à `vkCmdBindIndexBuffer`.

```c++
vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
```

La librairie que nous utilisons s'inclue de la même manière que les librairies STB. Il faut définir la macro
`TINYOBJLOADER_IMLEMENTATION` pour que le fichier comprenne les définitions des fonctions.

```c++
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
```

Nous allons ensuite écrire la fonction `loadModel` pour remplir le tableau de vertices et d'indices depuis le fichier
OBJ. Nous devons l'appeler avant que les buffers de vertices et d'indices soient créés.

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

Un modèle se charge dans la librairie avec la fonction `tinyobj::LoadObj` :

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

Dans un fichier OBJ on trouve des positions, des normales, des coordonnées de textures et des faces. Ces dernières
sont une collection de vertices, avec chaque vertex lié à une position, une normale et/ou un coordonnée de texture à
l'aide d'un indice. Il est ainsi possible de réutiliser les attributs de manière indépendante.

Le conteneur `attrib` contient les positions, les normales et les coordonnées de texture dans les vecteurs
`attrib.vertices`, `attrib.normals` et `attrib.texcoords`. Le conteneur `shapes` contient tous les objets et leurs
faces. Ces dernières se réfèrent donc aux données stockées dans `attrib`. Les modèles peuvent aussi définir un matériau
et une texture par face, mais nous ignorerons ces attributs pour le moment.

La chaîne de caractères `err` contient les erreurs et les messages générés pendant le chargement du fichier. Le
chargement des fichiers ne rate réellement que quand `LoadObj` retourne `false`. Les faces peuvent être constitués d'un
nombre quelconque de vertices, alors que notre application ne peut dessiner que des triangles. Heureusement, la fonction
possède la capacité - activée par défaut - de triangulariser les faces.

Nous allons combiner toutes les faces du fichier en un seul modèle. Commençons par itérer sur ces faces.

```c++
for (const auto& shape : shapes) {

}
```

Grâce à la triangularisation nous sommes sûrs que les faces n'ont que trois vertices. Nous pouvons donc simplement les
copier vers le vecteur des vertices finales :

```c++
for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
        Vertex vertex{};

        vertices.push_back(vertex);
        indices.push_back(indices.size());
    }
}
```

Pour faire simple nous allons partir du principe que les sommets sont uniques. La variable `index` est du type
`tinyobj::index_t`, et contient `vertex_index`, `normal_index` et `texcoord_index`. Nous devons traiter ces données
pour les relier aux données contenues dans les tableaux `attrib` :

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

Le tableau `attrib.vertices` est constitués de floats et non de vecteurs à trois composants comme `glm::vec3`. Il faut
donc multiplier les indices par 3. De même on trouve deux coordonnées de texture par entrée. Les décalages `0`, `1` et
`2` permettent ensuite d'accéder aux composant X, Y et Z, ou aux U et V dans le cas des textures.

Lancez le programme avec les optimisation activées (`Release` avec Visual Studio ou avec l'argument `-03` pour GCC).
Vous pourriez le faire sans mais le chargement du modèle sera très long. Vous devriez voir ceci :

![](/images/inverted_texture_coordinates.png)

La géométrie est correcte! Par contre les textures sont quelque peu... étranges. En effet le format OBJ part d'en bas à
gauche pour les coordonnées de texture, alors que Vulkan part d'en haut à gauche. Il suffit de changer cela pendant le
chargement du modèle :

```c++
vertex.texCoord = {
    attrib.texcoords[2 * index.texcoord_index + 0],
    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
};
```

Vous pouvez lancer à nouveau le programme. Le rendu devrait être correct :

![](/images/drawing_model.png)

## Déduplication des vertices

Pour le moment nous n'utilisons pas l'index buffer, et le vecteur `vertices` contient beaucoup de vertices dupliquées.
Nous ne devrions les inclure qu'une seule fois dans ce conteneur et utiliser leurs indices pour s'y référer. Une
manière simple de procéder consiste à utiliser une `unoredered_map` pour suivre les vertices multiples et leurs indices.

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

Chaque fois que l'on extrait un vertex du fichier, nous devons vérifier si nous avons déjà manipulé un vertex possédant
les mêmes attributs. Si il est nouveau, nous le stockerons dans `vertices` et placerons son indice dans
`uniqueVertices` et dans `indices`. Si nous avons déjà un tel vertex nous regarderons son indice depuis `uniqueVertices`
et copierons cette valeur dans `indices`.

Pour l'instant le programme ne peut pas compiler, car nous devons implémenter une fonction de hachage et l'opérateur
d'égalité pour utiliser la structure `Vertex` comme clé dans une table de hachage. L'opérateur est simple à surcharger :

```c++
bool operator==(const Vertex& other) const {
    return pos == other.pos && color == other.color && texCoord == other.texCoord;
}
```

Nous devons définir une spécialisation du patron de classe `std::hash<T>` pour la fonction de hachage. Le hachage est
un sujet compliqué, mais [cppreference.com recommande](http://en.cppreference.com/w/cpp/utility/hash) l'approche
suivante pour combiner correctement les champs d'une structure :

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

Ce code doit être placé hors de la définition de `Vertex`. Les fonctions de hashage des type GLM sont activés avec
la définition et l'inclusion suivantes :

```c++
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
```

Le dossier `glm/gtx/` contient les extensions expérimentales de GLM. L'API peut changer dans le futur, mais la
librairie a toujours été très stable.

Vous devriez pouvoir compiler et lancer le programme maintenant. Si vous regardez la taille de `vertices` vous verrez
qu'elle est passée d'un million et demi vertices à seulement 265645! Les vertices sont utilisés pour six triangles en
moyenne, ce qui représente une optimisation conséquente.

[Code C++](/code/27_model_loading.cpp) /
[Vertex shader](/code/26_shader_depth.vert) /
[Fragment shader](/code/26_shader_depth.frag)
