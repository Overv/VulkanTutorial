## Introduction

L'objet `VkDescriptorSetLayout` que nous avons créé dans le chapitre précédent décrit les descripteurs que nous devons
lier pour les opérations de rendu. Dans ce chapitre nous allons créer les véritables sets de descripteurs, un pour
chaque `VkBuffer`, afin que nous puissions chacun les lier au descripteur de l'UBO côté shader.

## Pool de descripteurs

Les sets de descripteurs ne peuvent pas être crées directement. Il faut les allouer depuis une pool, comme les command
buffers. Nous allons créer la fonction `createDescriptorPool` pour générer une pool de descripteurs.

```c++
void initVulkan() {
    ...
    createUniformBuffer();
    createDescriptorPool();
    ...
}

...

void createDescriptorPool() {

}
```

Nous devons d'abord indiquer les types de descripteurs et combien sont compris dans les sets. Nous utilisons pour cela
une structure du type `VkDescriptorPoolSize` :

```c++
VkDescriptorPoolSize poolSize{};
poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
poolSize.descriptorCount = static_cast<uint32_t>(swapChainImages.size());
```

Nous allons allouer un descripteur par frame. Cette structure doit maintenant être référencée dans la structure
principale `VkDescriptorPoolCreateInfo`.

```c++
VkDescriptorPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.poolSizeCount = 1;
poolInfo.pPoolSizes = &poolSize;
```

Nous devons aussi spécifier le nombre maximum de sets de descripteurs que nous sommes susceptibles d'allouer.

```c++
poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());
```

La structure possède un membre optionnel également présent pour les command pools. Il permet d'indiquer que les
sets peuvent être libérés indépendemment les uns des autres avec la valeur 
`VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT`. Comme nous n'allons pas toucher aux descripteurs pendant que le
programme s'exécute, nous n'avons pas besoin de l'utiliser. Indiquez `0` pour ce champ.

```c++
VkDescriptorPool descriptorPool;

...

if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("echec de la creation de la pool de descripteurs!");
}
```

Créez un nouveau membre donnée pour référencer la pool, puis appelez `vkCreateDescriptorPool`. La pool doit être
recrée avec la swap chain..

```c++
void cleanupSwapChain() {
    ...
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }
    
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    ...
}
```

Et recréée dans `recreateSwapChain` :

```c++
void recreateSwapChain() {
    ...
    createUniformBuffers();
    createDescriptorPool();
    createCommandBuffers();
}
```

## Set de descripteurs

Nous pouvons maintenant allouer les sets de descripteurs. Créez pour cela la fonction `createDescriptorSets` :

```c++
void initVulkan() {
    ...
    createDescriptorPool();
    createDescriptorSets();
    ...
}

...

void createDescriptorSets() {

}
```

L'allocation de cette ressource passe par la création d'une structure de type `VkDescriptorSetAllocateInfo`. Vous devez
bien sûr y indiquer la pool d'où les allouer, de même que le nombre de sets à créer et l'organisation qu'ils doivent
suivre.

```c++
std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
VkDescriptorSetAllocateInfo allocInfo{};
allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
allocInfo.descriptorPool = descriptorPool;
allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
allocInfo.pSetLayouts = layouts.data();
```

Dans notre cas nous allons créer autant de sets qu'il y a d'images dans la swap chain. Ils auront tous la même
organisation. Malheureusement nous devons copier la structure plusieurs fois car la fonction que nous allons utiliser
prend en argument un tableau, dont le contenu doit correspondre indice à indice aux objets à créer.

Ajoutez un membre donnée pour garder une référence aux sets, et allouez-les avec `vkAllocateDescriptorSets` :

```c++
VkDescriptorPool descriptorPool;
std::vector<VkDescriptorSet> descriptorSets;

...

descriptorSets.resize(swapChainImages.size());
if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("echec de l'allocation d'un set de descripteurs!");
}
```

Il n'est pas nécessaire de détruire les sets de descripteurs explicitement, car leur libération est induite par la
destruction de la pool. L'appel à `vkAllocateDescriptorSets` alloue donc tous les sets, chacun possédant un unique
descripteur d'UBO.

Nous avons créé les sets mais nous n'avons pas paramétré les descripteurs. Nous allons maintenant créer une boucle pour
rectifier ce problème :

```c++
for (size_t i = 0; i < swapChainImages.size(); i++) {

}
```

Les descripteurs référant à un buffer doivent être configurés avec une structure de type `VkDescriptorBufferInfo`. Elle
indique le buffer contenant les données, et où les données y sont stockées.

```c++
for (size_t i = 0; i < swapChainImages.size(); i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);
}
```

Nous allons utiliser tout le buffer, il est donc aussi possible d'indiquer `VK_WHOLE_SIZE`. La configuration des
descripteurs est maintenant mise à jour avec la fonction `vkUpdateDescriptorSets`. Elle prend un tableau de
`VkWriteDescriptorSet` en paramètre.

```c++
VkWriteDescriptorSet descriptorWrite{};
descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrite.dstSet = descriptorSets[i];
descriptorWrite.dstBinding = 0;
descriptorWrite.dstArrayElement = 0;
```

Les deux premiers champs spécifient le set à mettre à jour et l'indice du binding auquel il correspond. Nous avons donné
à notre unique descripteur l'indice `0`. Souvenez-vous que les descripteurs peuvent être des tableaux ; nous devons donc
aussi indiquer le premier élément du tableau que nous voulons modifier. Nous n'en n'avons qu'un.

```c++
descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
descriptorWrite.descriptorCount = 1;
```

Nous devons encore indiquer le type du descripteur. Il est possible de mettre à jour plusieurs descripteurs d'un même
type en même temps. La fonction commence à `dstArrayElement` et s'étend sur `descriptorCount` descripteurs.

```c++
descriptorWrite.pBufferInfo = &bufferInfo;
descriptorWrite.pImageInfo = nullptr; // Optionnel
descriptorWrite.pTexelBufferView = nullptr; // Optionnel
```

Le dernier champ que nous allons utiliser est `pBufferInfo`. Il permet de fournir `descriptorCount` structures qui
configureront les descripteurs. Les autres champs correspondent aux structures qui peuvent configurer des descripteurs
d'autres types. Ainsi il y aura `pImageInfo` pour les descripteurs liés aux images, et `pTexelBufferInfo` pour les
descripteurs liés aux buffer views.

```c++
vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
```

Les mises à jour sont appliquées quand nous appelons `vkUpdateDescriptorSets`. La fonction accepte deux tableaux, un de 
`VkWriteDesciptorSets` et un de `VkCopyDescriptorSet`. Le second permet de copier des descripteurs.

## Utiliser des sets de descripteurs

Nous devons maintenant étendre `createCommandBuffers` pour qu'elle lie les sets de descripteurs aux descripteurs des
shaders avec la commande `cmdBindDescriptorSets`. Il faut invoquer cette commande dans l'enregistrement des command
buffers avant l'appel à `vkCmdDrawIndexed`.

```c++
vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
```

Au contraire des buffers de vertices et d'indices, les sets de descripteurs ne sont pas spécifiques aux pipelines
graphiques. Nous devons donc spécifier que nous travaillons sur une pipeline graphique et non pas une pipeline de
calcul. Le troisième paramètre correspond à l'organisation des descripteurs. Viennent ensuite l'indice du premier
descripteur, la quantité à évaluer et bien sûr le set d'où ils proviennent. Nous y reviendrons. Les deux derniers
paramètres sont des décalages utilisés pour les descripteurs dynamiques. Nous y reviendrons aussi dans un futur
chapitre.

Si vous lanciez le programme vous verrez que rien ne s'affiche. Le problème est que l'inversion de la coordonnée Y dans
la matrice induit l'évaluation des vertices dans le sens inverse des aiguilles d'une montre (*counter-clockwise* en anglais),
alors que nous voudrions le contraire. En effet, les systèmes actuels utilisent ce sens de rotation pour détermnier la face de devant.
La face de derrière est ensuite simplement ignorée. C'est pourquoi notre géométrie n'est pas rendue. C'est le *backface culling*.
Changez le champ `frontface` de la structure `VkPipelineRasterizationStateCreateInfo` dans la fonction 
`createGraphicsPipeline` de la manière suivante :

```c++
rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
```

Maintenant vous devriez voir ceci en lançant votre programme :

![](/images/spinning_quad.png)

Le rectangle est maintenant un carré car la matrice de projection corrige son aspect. La fonction `updateUniformBuffer`
inclut d'office les redimensionnements d'écran, il n'est donc pas nécessaire de recréer les descripteurs dans 
`recreateSwapChain`.

## Alignement

Jusqu'à présent nous avons évité la question de la compatibilité des types côté C++ avec la définition des types pour
les variables uniformes. Il semble évident d'utiliser des types au même nom des deux côtés :

```c++
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
```

Pourtant ce n'est pas aussi simple. Essayez la modification suivante :

```c++
struct UniformBufferObject {
    glm::vec2 foo;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
layout(binding = 0) uniform UniformBufferObject {
    vec2 foo;
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
```

Recompilez les shaders et relancez le programme. Le carré coloré a disparu! La raison réside dans cette question de
l'alignement.

Vulkan s'attend à un certain alignement des données en mémoire pour chaque type. Par exemple :

* Les scalaires doivent être alignés sur leur nombre d'octets N (float de 32 bits donne un alognement de 4 octets)
* Un `vec2` doit être aligné sur 2N (8 octets)
* Les `vec3` et `vec4` doivent être alignés sur 4N (16 octets)
* Une structure imbriquée doit être alignée sur la somme des alignements de ses membres arrondie sur le multiple de
16 octets au-dessus
* Une `mat4` doit avoir le même alignement qu'un `vec4`

Les alignemenents imposés peuvent être trouvés dans
[la spécification](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/chap14.html#interfaces-resources-layout)

Notre shader original et ses trois `mat4` était bien aligné. `model` a un décalage de 0, `view` de 64 et `proj` de 128,
ce qui sont des multiples de 16.

La nouvelle structure commence avec un membre de 8 octets, ce qui décale tout ce qui suit. Les décalages sont augmentés
de 8 et ne sont alors plus multiples de 16. Nous pouvons fixer ce problème avec le mot-clef `alignas` :

```c++
struct UniformBufferObject {
    glm::vec2 foo;
    alignas(16) glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```

Si vous recompilez et relancez, le programme devrait fonctionner à nouveau.

Heureusement pour nous, GLM inclue un moyen qui nous permet de plus penser à ce souci d'alignement :

```c++
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
```

La ligne `#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES` force GLM a s'assurer de l'alignement des types qu'elle expose.
La limite de cette méthode s'atteint en utilisant des structures imbriquées. Prenons l'exemple suivant :

```c++
struct Foo {
    glm::vec2 v;
};
struct UniformBufferObject {
    Foo f1;
    Foo f2;
};
```

Et côté shader mettons :

```c++
struct Foo {
    vec2 v;
};
layout(binding = 0) uniform UniformBufferObject {
    Foo f1;
    Foo f2;
} ubo;
```

Nous nous retrouvons avec un décalage de 8 pour `f2` alors qu'il lui faudrait un décalage de 16. Il faut dans ce cas
de figure utiliser `alignas` :

```c++
struct UniformBufferObject {
    Foo f1;
    alignas(16) Foo f2;
};
```

Pour cette raison il est préférable de toujours être explicite à propos de l'alignement de données que l'on envoie aux
shaders. Vous ne serez pas supris par des problèmes d'alignement imprévus.

```c++
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
```

Recompilez le shader avant de continuer la lecture.

## Plusieurs sets de descripteurs

Comme on a pu le voir dans les en-têtes de certaines fonctions, il est possible de lier plusieurs sets de descripteurs
en même temps. Vous devez fournir une organisation pour chacun des sets pendant la mise en place de l'organisation de la
pipeline. Les shaders peuvent alors accéder aux descripteurs de la manière suivante :

```c++
layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
```

Vous pouvez utiliser cette possibilité pour placer dans différents sets les descripteurs dépendant d'objets et les
descripteurs partagés. De cette manière vous éviter de relier constemment une partie des descripteurs, ce qui peut être
plus performant.

[Code C++](/code/22_descriptor_sets.cpp) /
[Vertex shader](/code/21_shader_ubo.vert) /
[Fragment shader](/code/21_shader_ubo.frag)
