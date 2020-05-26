## Introduction

Les buffers sont pour Vulkan des emplacements mémoire qui peuvent permettre de stocker des données quelconques sur la
carte graphique. Nous pouvons en particulier y placer les données représentant les sommets, et c'est ce que nous allons
faire dans ce chapitre. Nous verrons plus tard d'autres utilisations répandues. Au contraire des autres objets que nous
avons rencontré les buffers n'allouent pas eux-mêmes de mémoire. Il nous faudra gérer la mémoire à la main.

## Création d'un buffer

Créez la fonction `createVertexBuffer` et appelez-la depuis `initVulkan` juste avant `createCommandBuffers`.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffer();
    createCommandBuffers();
    createSyncObjects();
}

...

void createVertexBuffer() {

}
```

Pour créer un buffer nous allons devoir remplir une structure de type `VkBufferCreateInfo`.

```c++
VkBufferCreateInfo bufferInfo{};
bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
bufferInfo.size = sizeof(vertices[0]) * vertices.size();
```

Le premier champ de cette structure s'appelle `size`. Il spécifie la taille du buffer en octets. Nous pouvons utiliser 
`sizeof` pour déterminer la taille de notre tableau de valeur.

```c++
bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
```

Le deuxième champ, appelé `usage`, correspond à l'utilisation type du buffer. Nous pouvons indiquer plusieurs valeurs
représentant les utilisations possibles. Dans notre cas nous ne mettons que la valeur qui correspond à un vertex buffer.

```c++
bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
```

De la même manière que les images de la swap chain, les buffers peuvent soit être gérés par une queue family, ou bien
être partagés entre plusieurs queue families. Notre buffer ne sera utilisé que par la queue des graphismes, nous
pouvons donc rester en mode exclusif.

Le paramètre `flags` permet de configurer le buffer tel qu'il puisse être constitué de plusieurs emplacements distincts
dans la mémoire. Nous n'utiliserons pas cette fonctionnalité, laissez `flags` à `0`.

Nous pouvons maintenant créer le buffer en appelant `vkCreateBuffer`. Définissez un membre donnée pour stocker ce
buffer :

```c++
VkBuffer vertexBuffer;

...

void createVertexBuffer() {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(vertices[0]) * vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("echec de la creation d'un vertex buffer!");
    }
}
```

Le buffer doit être disponible pour toutes les opérations de rendu, nous ne pouvons donc le détruire qu'à la fin du
programme, et ce dans `cleanup` car il ne dépend pas de la swap chain.

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyBuffer(device, vertexBuffer, nullptr);

    ...
}
```

## Fonctionnalités nécessaires de la mémoire

Le buffer a été créé mais il n'est lié à aucune forme de mémoire. La première étape de l'allocation de mémoire consiste
à récupérer les fonctionnalités dont le buffer a besoin à l'aide de la fonction `vkGetBufferMemoryRequirements`.

```c++
VkMemoryRequirements memRequirements;
vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);
```

La structure que la fonction nous remplit possède trois membres :

* `size` : le nombre d'octets dont le buffer a besoin, ce qui peut différer de ce que nous avons écrit en préparant le
buffer
* `alignment` : le décalage en octets entre le début de la mémoire allouée pour lui et le début des données du buffer,
ce que le driver détermine avec les valeurs que nous avons fournies dans `usage` et `flags`
* `memoryTypeBits` : champs de bits combinant les types de mémoire qui conviennent au buffer

Les cartes graphiques offrent plusieurs types de mémoire. Ils diffèrent en performance et en opérations disponibles.
Nous devons considérer ce dont le buffer a besoin en même temps que ce dont nous avons besoin pour sélectionner le
meilleur type de mémoire possible. Créons une fonction `findMemoryType` pour y isoler cette logique.

```c++
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {

}
```

Nous allons commencer cette fonction en récupérant les différents types de mémoire que la carte graphique peut nous
offrir.

```c++
VkPhysicalDeviceMemoryProperties memProperties;
vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
```

La structure `VkPhysicalDeviceMemoryProperties` comprend deux tableaux appelés `memoryHeaps` et `memoryTypes`. Une pile
de mémoire (memory heap en anglais) correspond aux types physiques de mémoire. Par exemple la VRAM est une pile, de même
que la RAM utilisée comme zone de swap si la VRAM est pleine en est une autre. Tous les autres types de mémoire stockés
dans `memoryTypes` sont répartis dans ces piles. Nous n'allons pas utiliser la pile comme facteur de choix, mais vous
pouvez imaginer l'impact sur la performance que cette distinction peut avoir.

Trouvons d'abord un type de mémoire correspondant au buffer :

```c++
for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if (typeFilter & (1 << i)) {
        return i;
    }
}

throw std::runtime_error("aucun type de memoire ne satisfait le buffer!");
```

Le paramètre `typeFilter` nous permettra d'indiquer les types de mémoire nécessaires au buffer lors de l'appel à la
fonction. Ce champ de bit voit son n-ième bit mis à `1` si le n-ième type de mémoire disponible lui convient. Ainsi
nous pouvons itérer sur les bits de `typeFilter` pour trouver les types de mémoire qui lui correspondent.

Cependant cette vérification ne nous est pas suffisante. Nous devons vérifier que la mémoire est accesible depuis le CPU
afin de pouvoir y écrire les données des vertices. Nous devons pour cela vérifier que le champ de bits `properyFlags`
comprend au moins `VK_MEMORY_PROPERTY_HOSY_VISIBLE_BIT`, de même que `VK_MEMORY_PROPERTY_HOSY_COHERENT_BIT`. Nous
verrons pourquoi cette deuxième valeur est nécessaire quand nous lierons de la mémoire au buffer.

Nous placerons ces deux valeurs dans le paramètre `properties`. Nous pouvons changer la boucle pour qu'elle prenne en
compte le champ de bits :

```c++
for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
    }
}
```

Le ET bit à bit fournit une valeur non nulle si et seulement si au moins l'une des propriétés est supportée. Nous ne
pouvons nous satisfaire de cela, c'est pourquoi il est nécessaire de comparer le résultat au champ de bits complet. Si
ce résultat nous convient, nous pouvons retourner l'indice de la mémoire et utiliser cet emplacement. Si aucune mémoire
ne convient nous levons une exception.

## Allocation de mémoire

Maintenant que nous pouvons déterminer un type de mémoire nous convenant, nous pouvons y allouer de la mémoire. Nous
devons pour cela remplir la structure `VkMemoryAllocateInfo`.

```c++
VkMemoryAllocateInfo allocInfo{};
allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
allocInfo.allocationSize = memRequirements.size;
allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
```

Pour allouer de la mémoire il nous suffit d'indiquer une taille et un type, ce que nous avons déjà déterminé. Créez un
membre donnée pour contenir la référence à l'espace mémoire et allouez-le à l'aide de `vkAllocateMemory`.

```c++
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;

...
if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("echec d'une allocation de memoire!");
}
```

Si l'allocation a réussi, nous pouvons associer cette mémoire au buffer avec la fonction `vkBindBufferMemory` :

```c++
vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
```

Les trois premiers paramètres sont évidents. Le quatrième indique le décalage entre le début de la mémoire et le début
du buffer. Nous avons alloué cette mémoire spécialement pour ce buffer, nous pouvons donc mettre `0`. Si vous décidez
d'allouer un grand espace mémoire pour y mettre plusieurs buffers, sachez qu'il faut que ce nombre soit divisible par 
`memRequirements.alignement`. Notez que cette stratégie est la manière recommandée de gérer la mémoire des GPUs (voyez
[cet article](https://developer.nvidia.com/vulkan-memory-management)).

Il est évident que cette allocation dynamique de mémoire nécessite que nous libérions l'emplacement nous-mêmes. Comme la
mémoire est liée au buffer, et que le buffer sera nécessaire à toutes les opérations de rendu, nous ne devons la libérer
qu'à la fin du programme.


```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
```

## Remplissage du vertex buffer

Il est maintenant temps de placer les données des vertices dans le buffer. Nous allons
[mapper la mémoire](https://en.wikipedia.org/wiki/Memory-mapped_I/O) dans un emplacement accessible par le CPU à l'aide
de la fonction `vkMapMemory`.

```c++
void* data;
vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
```

Cette fonction nous permet d'accéder à une région spécifique d'une ressource. Nous devons pour cela indiquer un décalage
et une taille. Nous mettons ici respectivement `0` et `bufferInfo.size`. Il est également possible de fournir la valeur 
`VK_WHOLE_SIZE` pour mapper d'un coup toute la ressource. L'avant-dernier paramètre est un champ de bits pour l'instant
non implémenté par Vulkan. Il est impératif de la laisser à `0`. Enfin, le dernier paramètre permet de fournir un
pointeur vers la mémoire ainsi mappée.

```c++
void* data;
vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferInfo.size);
vkUnmapMemory(device, vertexBufferMemory);
```

Vous pouvez maintenant utiliser `memcpy` pour copier les vertices dans la mémoire, puis démapper le buffer à l'aide de 
`vkUnmapMemory`. Malheureusement le driver peut décider de cacher les données avant de les copier dans le buffer. Il est
aussi possible que les données soient copiées mais que ce changement ne soit pas visible immédiatement. Il y a deux
manières de régler ce problème :

* Utiliser une pile de mémoire cohérente avec la RAM, ce qui est indiqué par `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`
* Appeler `vkFlushMappedMemoryRanges` après avoir copié les données, puis appeler `vkInvalidateMappedMemory` avant
d'accéder à la mémoire

Nous utiliserons la première approche qui nous assure une cohérence permanente. Cette méthode est moins performante que
le flushing explicite, mais nous verrons dès le prochain chapitre que cela n'a aucune importance car nous changerons
complètement de stratégie.

Par ailleurs, notez que l'utilisation d'une mémoire cohérente ou le flushing de la mémoire ne garantissent que le fait
que le driver soit au courant des modifications de la mémoire. La seule garantie est que le déplacement se finisse d'ici
le prochain appel à `vkQueueSubmit`.

Remarquez également l'utilisation de `memcpy` qui indique la compatibilité bit-à-bit des structures avec la
représentation sur la carte graphique.

## Lier le vertex buffer

Il ne nous reste qu'à lier le vertex buffer pour les opérations de rendu. Nous allons pour cela compléter la fonction 
`createCommandBuffers`.

```c++
vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

VkBuffer vertexBuffers[] = {vertexBuffer};
VkDeviceSize offsets[] = {0};
vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
```

La fonction `vkCmdBindVertexBuffers` lie des vertex buffers aux bindings. Les deuxième et troisième paramètres indiquent
l'indice du premier binding auquel le buffer correspond et le nombre de bindings qu'il contiendra. L'avant-dernier 
paramètre est le tableau de vertex buffers à lier, et le dernier est un tableau de décalages en octets entre le début
d'un buffer et le début des données. Il est d'ailleurs préférable d'appeler `vkCmdDraw` avec la taille du tableau de
vertices plutôt qu'avec un nombre écrit à la main.

Lancez maintenant le programme; vous devriez voir le triangle habituel apparaître à l'écran.

![](/images/triangle.png)

Essayez de colorer le vertex du haut en blanc et relancez le programme :

```c++
const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};
```

![](/images/triangle_white.png)

Dans le prochain chapitre nous verrons une autre manière de copier les données vers un buffer. Elle est plus performante
mais nécessite plus de travail.

[Code C++](/code/18_vertex_buffer.cpp) /
[Vertex shader](/code/17_shader_vertexbuffer.vert) /
[Fragment shader](/code/17_shader_vertexbuffer.frag)
