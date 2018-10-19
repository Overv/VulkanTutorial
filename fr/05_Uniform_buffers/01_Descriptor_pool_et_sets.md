## Introduction

L'objet `VkDescriptorSetLayout` que nous avons créé au chapitre précédent décrit les descripteurs que nous devons lier
pour les opérations de rendu. Dans ce chapitre nous allons créer les véritables sets de descripteurs, un pour chaque
`VkBuffer`, afin que nous puissions chacun les lier au descripteur UBO.

## Pool de descripteurs

Les sets de descipteurs ne peuvent pas être crées directement. Il faut les allouer depuis une pool, comme les command
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
VkDescriptorPoolSize poolSize = {};
poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
poolSize.descriptorCount = static_cast<uint32_t>(swapChainImages.size());
```

Nous allons allouer un descripteur par frame. Cette structure doit maintenant être référencée dans la structure
principale `VkDescriptorPoolCreateInfo`.

```c++
VkDescriptorPoolCreateInfo poolInfo = {};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.poolSizeCount = 1;
poolInfo.pPoolSizes = &poolSize;
```

Nous devons aussi spécifier le nombre maximum de sets de descriptors que nous sommes susceptibles d'allouer.

```c++
poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());;
```

La stucture possède un membre optionnel également présent pour les command pools. Il permet d'indiquer que les
sets peuvent être libérés indépendemment les uns des autres avec la valeur
`VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT`. Comme nous ne voulons pas toucher aux descripteurs pendant que le
programme s'exécute, nous n'avons pas besoin de l'utiliser. Indiquez `0` pour ce champ.

```c++
VkDescriptorPool descriptorPool;

...

if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("echec lors de la creation de la pool de descripteurs!");
}
```

Créez un nouveau membre donnée pour référencer la pool, puis appelez `vkCreateDescriptorPool`. La pool doit alors être
détruite à la fin du programme, comme la plupart des ressources liées au rendu.

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    ...
}
```

## Set de descriptors

Nous pouvons maintenant allouer les sets de descipteurs. Créez pour cela la fonction `createDescriptorSets` :

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
VkDescriptorSetAllocateInfo allocInfo = {};
allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
allocInfo.descriptorPool = descriptorPool;
allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
allocInfo.pSetLayouts = layouts.data();
```

Dans notre cas nous allons créer autant de sets qu'il y a d'images dans la swap chain. Ils auront tous la même
organisation. Malheuresement nous devons copier la structure plusieurs fois car la fonction que nous allons utiliser
prend en argument un tableau, dont le contenu doit correspondre indice à indice aux objets à créer.

Ajoutez un membre donnée pour garder une référence aux sets, et allouez-les avec `vkAllocateDescriptorSets` :

```c++
VkDescriptorPool descriptorPool;
std::vector<VkDescriptorSet> descriptorSets;

...

descriptorSets.resize(swapChainImages.size());
if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[0]) != VK_SUCCESS) {
    throw std::runtime_error("echec lors de l'allocation d'un set de descripteurs!");
}
```

Il n'est pas nécessaire de détruire les sets de descripteurs explicitement, car leur libération est induite par la
destruction de la pool. L'appel à `vkAllocateDescriptorSets` alloue donc tous les sets, chacun possédant un descripteur
de buffer uniform.

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
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);
}
```

Nous allons utiliser tout le buffer, donc nous pourrions indiquer `VK_WHOLE_SIZE`. La configuration des
descripteurs est maintenant mise à jour avec la fonction `vkUpdateDescriptorSets`. Elle prend un tableau de
`VkWriteDescriptorSet` en paramètre.

```c++
VkWriteDescriptorSet descriptorWrite = {};
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
descripteurs liés aux vues sur un buffer.

```c++
vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
```

Les mises à jour sont appliquées quand nous appellons `vkUpdateDescriptorSets`. La fonction accepte deux tableaux, un de
`VkWriteDesciptorSets` et un de `VkCopyDescriptorSet`. Le second permet de copier des descripteurs.

## Utiliser des sets de descripteurs

Nous devons maintenant étendre `createCommandBuffers` pour qu'elle lie les sets de descripteurs aux descripteurs des
shaders avec la commande `cmdBindDescriptorSets`. Il faut invoquer cette commande dans la configuration des buffers de
commande avant l'appel à `vkCmdDrawIndexed`.

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
la matrice induit l'évaluation des vertices dans le sens des aiguilles d'une montre, alors que nous voudrions le
contraire. En effet, les systèmes actuels utilisent ce sens de rotation pour détermnier la face de devant. Le face de
derrière est ensuite simplement ignorée. C'est pourquoi notre géométrie n'est pas rendue. C'est le *backface culling*.
Changez le champ `frontface` de la structure `VkPipelineRasterizationStateCreateInfo` dans la fonction
`createGraphicsPipeline` de la manière suivante :

```c++
rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
```

Maintenant vous devriez voir ceci en lançant votre programme :

![](/images/spinning_quad.png)

Le rectangle est maintenant un carré car la matrice de projection corrige son aspect. La fonction `updateUniformBuffer`
traite les redimensionnements d'écran, il n'est donc pas nécessaire de recréer les descripteurs dans
`recreateSwapChain`.

## Plusieurs sets de descripteurs

Comme on a pu le voir dans les en-têtes de certaines fonctions, il est possible de lier plusieurs sets de descripteurs
en même temps. Vous devez fournir une organisation pour chacun des sets pendant la mise en place de l'organisation de la
pipeline. Les shaders peuvent alors accéder aux descripteurs de la manière suivante :

```c++
layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
```

Vous pouvez utiliser cette possibilité pour placer dans différents sets les descripteurs dépendant d'objets et les
descripteurs partagés. De cette manière vous éviter de relier une partie des descripteurs, ce qui peut être plus
performant.

[Code C++](/code/22_descriptor_sets.cpp) /
[Vertex shader](/code/21_shader_ubo.vert) /
[Fragment shader](/code/21_shader_ubo.frag)
