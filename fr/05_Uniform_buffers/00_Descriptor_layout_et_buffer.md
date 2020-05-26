## Introduction

Nous pouvons maintenant passer des données à chaque groupe d'invocation de vertex shaders. Mais qu'en est-il des
variables globales? Nous allons enfin passer à la 3D, et nous avons besoin d'une matrice model-view-projection. Nous
pourrions la transmettre avec les vertices, mais cela serait un gachis de mémoire et, de plus, nous devrions mettre à
jour le vertex buffer à chaque frame, alors qu'il est très bien rangé dans se mémoire à hautes performances.

La solution fournie par Vulkan consiste à utiliser des *descripteurs de ressource* (ou *resource descriptors*), qui
font correspondre des données en mémoire à une variable shader. Un descripteur permet à des shaders d'accéder
librement à des ressources telles que les buffers ou les *images*. Attention, Vulkan donne un sens particulier au
terme image. Nous verrons cela bientôt. Nous allons pour l'instant créer un buffer qui contiendra les matrices de
transformation. Nous ferons en sorte que le vertex shader puisse y accéder. Il y a trois parties à l'utilisation d'un
descripteur de ressources :

* Spécifier l'organisation des descripteurs durant la création de la pipeline
* Allouer un set de descripteurs depuis une pool de descripteurs (encore un objet de gestion de mémoire)
* Lier le descripteur pour les opérations de rendu

L'*organisation du descripteur* (descriptor layout) indique le type de ressources qui seront accédées par la
pipeline. Cela ressemble sur le principe à indiquer les attachements accédés. Un *set de descripteurs* (descriptor
set) spécifie le buffer ou l'image qui sera lié à ce descripteur, de la même manière qu'un framebuffer doit indiquer
les ressources qui le composent.

Il existe plusieurs types de descripteurs, mais dans ce chapitre nous ne verrons que les *uniform buffer objects* (UBO).
Nous en verrons d'autres plus tard, et leur utilisation sera très similaire. Rentrons dans le vif du sujet et supposons
maintenant que nous voulons que toutes les invocations du vertex shader que nous avons codé accèdent à la structure C
suivante :

```c++
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```

Nous devons la copier dans un `VkBuffer` pour pouvoir y accéder à l'aide d'un descripteur UBO depuis le vertex shader.
De son côté le vertex shader y fait référence ainsi :

```glsl
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

Nous allons mettre à jour les matrices model, view et projection à chaque frame pour que le rectangle tourne sur
lui-même et donne un effet 3D à la scène.

## Vertex shader

Modifiez le vertex shader pour qu'il inclue l'UBO comme dans l'exemple ci-dessous. Je pars du principe que vous
connaissez les transformations MVP. Si ce n'est pourtant pas le cas, vous pouvez vous rendre sur
[ce site](http://opengl.datenwolf.net/gltut/html/index.html) déjà mentionné dans le premier chapitre.

```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

L'ordre des variables `in`, `out` et `uniform` n'a aucune importance. La directive `binding` est assez semblable à 
`location` ; elle permet de fournir l'indice du binding. Nous allons l'indiquer dans l'organisation du descripteur.
Notez le changement dans la ligne calculant `gl_Position`, qui prend maintenant en compte la matrice MVP. La dernière
composante du vecteur ne sera plus à `0`, car elle sert à diviser les autres coordonnées en fonction de leur distance à
la caméra pour créer un effet de profondeur.

## Organisation du set de descripteurs

La prochaine étape consiste à définir l'UBO côté C++. Nous devons aussi informer Vulkan que nous voulons l'utiliser
dans le vertex shader.

```c++
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```

Nous pouvons faire correspondre parfaitement la déclaration en C++ avec celle dans le shader grâce à GLM. De plus les
matrices sont stockées d'une manière compatible bit à bit avec l'interprétation de ces données par les shaders. Nous
pouvons ainsi utiliser `memcpy` sur une structure `UniformBufferObject` vers un `VkBuffer`.

Nous devons fournir des informations sur chacun des descripteurs utilisés par les shaders lors de la création de la
pipeline, similairement aux entrées du vertex shader. Nous allons créer une fonction pour gérer toute cette information,
et ainsi pour créer le set de descripteurs. Elle s'appelera `createDescriptorSetLayout` et sera appelée juste avant la
finalisation de la création de la pipeline.

```c++
void initVulkan() {
    ...
    createDescriptorSetLayout();
    createGraphicsPipeline();
    ...
}

...

void createDescriptorSetLayout() {

}
```

Chaque `binding` doit être décrit à l'aide d'une structure de type `VkDescriptorSetLayoutBinding`.

```c++
void createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
}
```

Les deux premiers champs permettent de fournir la valeur indiquée dans le shader avec `binding` et le type de
descripteur auquel il correspond. Il est possible que la variable côté shader soit un tableau d'UBO, et dans ce cas
il faut indiquer le nombre d'éléments qu'il contient dans le membre `descriptorCount`. Cette possibilité pourrait être
utilisée pour transmettre d'un coup toutes les transformations spécifiques aux différents éléments d'une structure
hiérarchique. Nous n'utilisons pas cette possiblité et indiquons donc `1`.

```c++
uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
```

Nous devons aussi informer Vulkan des étapes shaders qui accèderont à cette ressource. Le champ de bits `stageFlags`
permet de combiner toutes les étapes shader concernées. Vous pouvez aussi fournir la valeur 
`VK_SHADER_STAGE_ALL_GRAPHICS`. Nous mettons uniquement `VK_SHADER_STAGE_VERTEX_BIT`.

```c++
uboLayoutBinding.pImmutableSamplers = nullptr; // Optionnel
```

Le champ `pImmutableSamplers` n'a de sens que pour les descripteurs liés aux samplers d'images. Nous nous attaquerons à
ce sujet plus tard. Vous pouvez le mettre à `nullptr`.

Tous les liens des descripteurs sont ensuite combinés en un seul objet `VkDescriptorSetLayout`. Créez pour cela un
nouveau membre donnée :

```c++
VkDescriptorSetLayout descriptorSetLayout;
VkPipelineLayout pipelineLayout;
```

Nous pouvons créer cet objet à l'aide de la fonction `vkCreateDescriptorSetLayout`. Cette fonction prend en argument une
structure de type `VkDescriptorSetLayoutCreateInfo`. Elle contient un tableau contenant les structures qui décrivent les
bindings :

```c++
VkDescriptorSetLayoutCreateInfo layoutInfo{};
layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
layoutInfo.bindingCount = 1;
layoutInfo.pBindings = &uboLayoutBinding;

if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("echec de la creation d'un set de descripteurs!");
}
```

Nous devons fournir cette structure à Vulkan durant la création de la pipeline graphique. Ils sont transmis par la
structure `VkPipelineLayoutCreateInfo`. Modifiez ainsi la création de cette structure :

```c++
VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 1;
pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
```

Vous vous demandez peut-être pourquoi il est possible de spécifier plusieurs set de descripteurs dans cette structure,
dans la mesure où un seul inclut tous les `bindings` d'une pipeline. Nous y reviendrons dans le chapitre suivant, quand
nous nous intéresserons aux pools de descripteurs.

L'objet que nous avons créé ne doit être détruit que lorsque le programme se termine.

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    ...
}
```

## Uniform buffer

Dans le prochain chapitre nous référencerons le buffer qui contient les données de l'UBO. Mais nous devons bien sûr
d'abord créer ce buffer. Comme nous allons accéder et modifier les données du buffer à chaque frame, il est assez
inutile d'utiliser un buffer intermédiaire. Ce serait même en fait contre-productif en terme de performances.

Comme des frames peuvent être "in flight" pendant que nous essayons de modifier le contenu du buffer, nous allons avoir
besoin de plusieurs buffers. Nous pouvons soit en avoir un par frame, soit un par image de la swap chain. Comme nous
avons un command buffer par image nous allons utiliser cette seconde méthode.

Pour cela créez les membres données `uniformBuffers` et `uniformBuffersMemory` :

```c++
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

std::vector<VkBuffer> uniformBuffers;
std::vector<VkDeviceMemory> uniformBuffersMemory;
```

Créez ensuite une nouvelle fonction appelée `createUniformBuffers` et appelez-la après `createIndexBuffers`. Elle
allouera les buffers :

```c++
void initVulkan() {
    ...
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    ...
}

...

void createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(swapChainImages.size());
    uniformBuffersMemory.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
    }
}
```

Nous allons créer une autre fonction qui mettra à jour le buffer en appliquant à son contenu une transformation à chaque
frame. Nous n'utiliserons donc pas `vkMapMemory` ici. Le buffer doit être détruit à la fin du programme. Mais comme il
dépend du nombre d'images de la swap chain, et que ce nombre peut évoluer lors d'une reécration, nous devons le
supprimer depuis `cleanupSwapChain` :

```c++
void cleanupSwapChain() {
    ...
    
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    ...
}
```

Nous devons également le recréer depuis `recreateSwapChain` :

```c++
void recreateSwapChain() {
    ...
    createFramebuffers();
    createUniformBuffers();
    createCommandBuffers();
}
```

## Mise à jour des données uniformes

Créez la fonction `updateUniformBuffer` et appelez-la dans `drawFrame`, juste après que nous avons déterminé l'image de
la swap chain que nous devons acquérir :

```c++
void drawFrame() {
    ...

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    ...

    updateUniformBuffer(imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    ...
}

...

void updateUniformBuffer(uint32_t currentImage) {

}
```

Cette fonction générera une rotation à chaque frame pour que la géométrie tourne sur elle-même. Pour ces fonctionnalités
mathématiques nous devons inclure deux en-têtes :

```c++
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
```

Le header `<glm/gtc/matrix_transform.hpp>` expose des fonctions comme `glm::rotate`, `glm:lookAt` ou `glm::perspective`,
dont nous avons besoin pour implémenter la 3D. La macro `GLM_FORCE_RADIANS` permet d'éviter toute confusion sur la
représentation des angles.

Pour que la rotation s'exécute à une vitesse indépendante du FPS, nous allons utiliser les fonctionnalités de mesure
précise de la librairie standrarde C++. Incluez donc `<chrono>` :

```c++
void updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
}
```

Nous commençons donc par écrire la logique de calcul du temps écoulé, mesuré en secondes et stocké dans un `float`.

Nous allons ensuite définir les matrices model, view et projection stockées dans l'UBO. La rotation sera implémentée
comme une simple rotation autour de l'axe Z en fonction de la variable `time` :

```c++
UniformBufferObject ubo{};
ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
```

La fonction `glm::rotate` accepte en argument une matrice déjà existante, un angle de rotation et un axe de rotation. Le
constructeur `glm::mat4(1.0)` crée une matrice identité. Avec la multiplication `time * glm::radians(90.0f)` la
géométrie tournera de 90 degrés par seconde.

```c++
ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
```

Pour la matrice view, j'ai décidé de la générer de telle sorte que nous regardions le rectangle par dessus avec une
inclinaison de 45 degrés. La fonction `glm::lookAt` prend en arguments la position de l'oeil, la cible du regard et
l'axe servant de référence pour le haut.

```c++
ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f);
```

J'ai opté pour un champ de vision de 45 degrés. Les autres paramètres de `glm::perspective` sont le ratio et les plans
near et far. Il est important d'utiliser l'étendue actuelle de la swap chain pour calculer le ratio, afin d'utiliser les
valeurs qui prennent en compte les redimensionnements de la fenêtre.

```c++
ubo.proj[1][1] *= -1;
```

GLM a été conçue pour OpenGL, qui utilise les coordonnées de clip et de l'axe Y à l'envers. La manière la plus simple de
compenser cela consiste à changer le signe de l'axe Y dans la matrice de projection.

Maintenant que toutes les transformations sont définies nous pouvons copier les données dans le buffer uniform actuel.
Nous utilisons la première technique que nous avons vue pour la copie de données dans un buffer.

```c++
void* data;
vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
```

Utiliser un UBO de cette manière n'est pas le plus efficace pour transmettre des données fréquemment mises à jour. Une
meilleure pratique consiste à utiliser les *push constants*, que nous aborderons peut-être dans un futur chapitre.

Dans un avenir plus proche nous allons lier les sets de descripteurs au `VkBuffer` contenant les données des matrices,
afin que le vertex shader puisse y avoir accès.

[Code C++](/code/21_descriptor_layout.cpp) /
[Vertex shader](/code/21_shader_ubo.vert) /
[Fragment shader](/code/21_shader_ubo.frag)
