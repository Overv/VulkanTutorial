## Introduction

Dans les quatre prochains chapitres nous allons remplacer les sommets inscrits dans le vertex shader par un vertex
buffer stocké dans la mémoire de la carte graphique. Nous commencerons par une manière simple de procéder en créant un
buffer manipulable depuis le CPU et en y copiant des données avec `memcpy`. Puis nous verrons comment avantageusement
utiliser un *staging buffer* pour accéder à de la mémoire de haute performance.

## Vertex shader

Premièrement, changeons le vertex shader en retirant les coordonnées des sommets de son code. Elles seront maintenant
stockés dans une variable. Elle sera liée au contenu du vertex buffer, ce qui est indiqué par le mot-clef `in`. Faisons
de même avec la couleur.

```glsl
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

Les variables `inPosition` et `inColor` sont des *vertex attributes*. Ce sont des propriétés spécifiques du sommet à
l'origine de l'invocation du shader. Ces données peuvent être de différentes natures, des couleurs aux coordonnées en
passant par des coordonnées de texture. Recompilez ensuite le vertex shader.

Tout comme pour `fragColor`, les annotations de type `layout(location=x)` assignent un indice à l'entrée. Cet indice
est utilisé depuis le code C++ pour les reconnaître. Il est important de savoir que certains types - comme les vecteurs
de flottants de double précision (64 bits) - prennent deux emplacements. Voici un exemple d'une telle situation, où il
est nécessaire de prévoir un écart entre deux entrés :

```glsl
layout(location = 0) in dvec3 inPosition;
layout(location = 2) in vec3 inColor;
```

Vous pouvez trouver plus d'information sur les qualificateurs d'organisation sur
[le wiki](https://www.khronos.org/opengl/wiki/Layout_Qualifier_(GLSL)).

## Sommets

Nous déplaçons les données des sommets depuis le code du shader jusqu'au code C++. Commencez par inclure la librairie
GLM, afin d'utiliser des vecteurs et des matrices. Nous allons utiliser ces types pour les vecteurs de position et de
couleur.

```c++
#include <glm/glm.hpp>
```

Créez une nouvelle structure appelée `Vertex`. Elle possède deux attributs que nous utiliserons pour le vertex shader :

```c++
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};
```

GLM nous fournit des types très pratiques simulant les types utilisés par GLSL.

```c++
const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};
```

Nous utiliserons ensuite un tableau de structures pour représenter un ensemble de sommets. Nous utiliserons les mêmes
couleurs et les mêmes positions qu'avant, mais elles seront combinées en un seul tableau d'objets.

## Lier les descriptions

La prochaine étape consiste à indiquer à Vulkan comment passer ces données au shader une fois qu'elles sont
stockées dans le GPU. Nous verrons plus tard comment les y stocker. Il y a deux types de structures que nous allons
devoir utiliser.

Pour la première, appelée `VkVertexInputBindingDescription`, nous allons ajouter une fonction à `Vertex` qui renverra
une instance de cette structure.

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

Un *vertex binding* décrit la lecture des données stockées en mémoire. Elle fournit le nombre d'octets entre les jeux de
données et la manière de passer d'un ensemble de données (par exemple une coordonnée) au suivant. Elle permet à Vulkan
de savoir comment extraire chaque jeu de données correspondant à une invocation du vertex shader du vertex buffer.

```c++
VkVertexInputBindingDescription bindingDescription{};
bindingDescription.binding = 0;
bindingDescription.stride = sizeof(Vertex);
bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
```

Nos données sont compactées en un seul tableau, nous n'aurons besoin que d'un seul vertex binding. Le membre `binding`
indique l'indice du vertex binding dans le tableau des bindings. Le paramètre `stride` fournit le nombre d'octets
séparant les débuts de deux ensembles de données, c'est à dire l'écart entre les données devant ếtre fournies à une
invocation de vertex shader et celles devant être fournies à la suivante. Enfin `inputRate` peut prendre les valeurs
suivantes :

* `VK_VERTEX_INPUT_RATE_VERTEX` : Passer au jeu de données suivante après chaque sommet
* `VK_VERTEX_INPUT_RATE_INSTANCE` : Passer au jeu de données suivantes après chaque instance

Nous n'utilisons pas d'*instanced rendering* donc nous utiliserons `VK_VERTEX_INPUT_RATE_VERTEX`.

## Description des attributs

La seconde structure dont nous avons besoin est `VkVertexInputAttributeDescription`. Nous allons également en créer deux
instances depuis une fonction membre de `Vertex` :

```c++
#include <array>

...

static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    return attributeDescriptions;
}
```

Comme le prototype le laisse entendre, nous allons avoir besoin de deux de ces structures. Elles décrivent chacunes
l'origine et la nature des données stockées dans une variable shader annotée du `location=x`, et la manière d'en
déterminer les valeurs depuis les données extraites par le binding. Comme nous avons deux de
ces variables, nous avons besoin de deux de ces structures. Voici ce qu'il faut remplir pour la position.

```c++
attributeDescriptions[0].binding = 0;
attributeDescriptions[0].location = 0;
attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
attributeDescriptions[0].offset = offsetof(Vertex, pos);
```

Le paramètre `binding` informe Vulkan de la provenance des données du sommet qui mené à l'invocation du vertex shader,
en lui fournissant le vertex binding qui les a extraites. Le paramètre `location` correspond à la valeur donnée à la
directive `location` dans le code du vertex shader. Dans notre cas l'entrée `0` correspond à la position du sommet
stockée dans un vecteur de floats de 32 bits.

Le paramètre `format` permet donc de décrire le type de donnée de l'attribut. Étonnement les formats doivent être
indiqués avec des valeurs énumérées dont les noms semblent correspondre à des gradients de couleur :

* `float` : `VK_FORMAT_R32_SFLOAT`
* `vec2` : `VK_FORMAT_R32G32_SFLOAT`
* `vec3` : `VK_FORMAT_R32G32B32_SFLOAT`
* `vec4` : `VK_FORMAT_R32G32B32A32_SFLOAT`

Comme vous pouvez vous en douter il faudra utiliser le format dont le nombre de composants de couleurs correspond au
nombre de données à transmettre. Il est autorisé d'utiliser plus de données que ce qui est prévu dans le shader, et ces
données surnuméraires seront silencieusement ignorées. Si par contre il n'y a pas assez de valeurs les valeurs suivantes
seront utilisées par défaut pour les valeurs manquantes : 0, 0 et 1 pour les deuxième, troisième et quatrième
composantes. Il n'y a pas de valeur par défaut pour le premier membre car ce cas n'est pas autorisé. Les types
(`SFLOAT`, `UINT` et `SINT`) et le nombre de bits doivent par contre correspondre parfaitement à ce qui est indiqué dans
le shader. Voici quelques exemples :

* `ivec2` correspond à `VK_FORMAT_R32G32_SINT` et est un vecteur à deux composantes d'entiers signés de 32 bits
* `uvec4` correspond à `VK_FORMAT_R32G32B32A32_UINT` et est un vecteur à quatre composantes d'entiers non signés de 32
bits
* `double` correspond à `VK_FORMAT_R64_SFLOAT` et est un float à précision double (donc de 64 bits)

Le paramètre `format` définit implicitement la taille en octets des données. Mais le binding extrait dans notre cas deux
données pour chaque sommet : la position et la couleur. Pour savoir quels octets doivent être mis dans la variable à
laquelle la structure correspond, le paramètre `offset` permet d'indiquer de combien d'octets il faut se décaler dans
les données extraites pour se trouver au début de la variable. Ce décalage est calculé automatiquement par la macro
`offsetof`.

L'attribut de couleur est décrit de la même façon. Essayez de le remplir avant de regarder la solution ci-dessous.

```c++
attributeDescriptions[1].binding = 0;
attributeDescriptions[1].location = 1;
attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
attributeDescriptions[1].offset = offsetof(Vertex, color);
```

## Entrée des sommets dans la pipeline

Nous devons maintenant mettre en place la réception par la pipeline graphique des données des sommets. Nous allons
modifier une structure dans `createGraphicsPipeline`. Trouvez `vertexInputInfo` et ajoutez-y les références aux deux
structures de description que nous venons de créer :

```c++
auto bindingDescription = Vertex::getBindingDescription();
auto attributeDescriptions = Vertex::getAttributeDescriptions();

vertexInputInfo.vertexBindingDescriptionCount = 1;
vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
```

La pipeline peut maintenant accepter les données des vertices dans le format que nous utilisons et les fournir au vertex
shader. Si vous lancez le programme vous verrez que les validation layers rapportent qu'aucun vertex buffer n'est mis
en place. Nous allons donc créer un vertex buffer et y placer les données pour que le GPU puisse les utiliser.

[Code C++](/code/17_vertex_input.cpp) /
[Vertex shader](/code/17_shader_vertexbuffer.vert) /
[Fragment shader](/code/17_shader_vertexbuffer.frag)
