## Introduction

Les modèles 3D que vous serez susceptibles d'utiliser dans des applications réelles partagerons le plus souvent des
vertices communs à plusieurs triangles. Cela est d'ailleurs le cas avec un simple rectangle :

![](/images/vertex_vs_index.svg)

Un rectangle est composé de triangles, ce qui signifie que nous aurions besoin d'un vertex buffer avec 6 vertices. Mais
nous dupliquerions alors des vertices, aboutissant à un gachis de mémoire. Dans des modèles plus complexes, les vertices
sont en moyenne en contact avec 3 triangles, ce qui serait encore pire. La solution consiste à utiliser un index buffer.

Un index buffer est essentiellement un tableau de références vers le vertex buffer. Il vous permet de réordonner ou de
dupliquer les données de ce buffer. L'image ci-dessus démontre l'utilité de cette méthode.

## Création d'un index buffer

Dans ce chapitre, nous allons ajouter les données nécessaires à l'affichage d'un rectangle. Nous allons ainsi rajouter
une coordonnée dans le vertex buffer et créer un index buffer. Voici les données des sommets au complet :

```c++
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};
```

Le coin en haut à gauche est rouge, celui en haut à droite est vert, celui en bas à droite est bleu et celui en bas à
gauche est blanc. Les couleurs seront dégradées par l'interpolation du rasterizer. Nous allons maintenant créer le
tableau `indices` pour représenter l'index buffer. Son contenu correspond à ce qui est présenté dans l'illustration.

```c++
const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};
```

Il est possible d'utiliser `uint16_t` ou `uint32_t` pour les valeurs de l'index buffer, en fonction du nombre d'éléments
dans `vertices`. Nous pouvons nous contenter de `uint16_t` car nous n'utilisons pas plus de 65535 sommets différents.

Comme les données des sommets, nous devons placer les indices dans un `VkBuffer` pour que le GPU puisse y avoir accès.
Créez deux membres donnée pour référencer les ressources du futur index buffer :

```c++
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;
```

La fonction `createIndexBuffer` est quasiment identique à `createVertexBuffer` :

```c++
void initVulkan() {
    ...
    createVertexBuffer();
    createIndexBuffer();
    ...
}

void createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
```

Il n'y a que deux différences : `bufferSize` correspond à la taille du tableau multiplié par `sizeof(uint16_t)`, et
`VK_BUFFER_USAGE_VERTEX_BUFFER_BIT` est remplacé par `VK_BUFFER_USAGE_INDEX_BUFFER_BIT`. À part ça tout est
identique : nous créons un buffer intermédiaire puis le copions dans le buffer final local au GPU.

L'index buffer doit être libéré à la fin du programme depuis `cleanup`.

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    ...
}
```

## Utilisation d'un index buffer

Pour utiliser l'index buffer lors des opérations de rendu nous devons modifier un petit peu `createCommandBuffers`. Tout
d'abord il nous faut lier l'index buffer. La différence est qu'il n'est pas possible d'avoir plusieurs index buffers. De
plus il n'est pas possible de subdiviser les sommets en leurs coordonnées, ce qui implique que la modification d'une
seule coordonnée nécessite de créer un autre sommet le vertex buffer.

```c++
vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
```

Un index buffer est lié par la fonction `vkCmdBindIndexBuffer`. Elle prend en paramètres le buffer, le décalage dans ce
buffer et le type de donnée. Pour nous ce dernier sera `VK_INDEX_TYPE_UINT16`.

Simplement lier le vertex buffer ne change en fait rien. Il nous faut aussi mettre à jour les commandes d'affichage
pour indiquer à Vulkan comment utiliser le buffer. Supprimez l'appel à `vkCmdDraw`, et remplacez-le par
`vkCmdDrawIndexed` :

```c++
vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
```

Le deuxième paramètre indique le nombre d'indices. Le troisième est le nombre d'instances à invoquer (ici `1` car nous 
n'utilisons par cette technique). Le paramètre suivant est un décalage dans l'index buffer, sachant qu'ici il ne
fonctionne pas en octets mais en indices. L'avant-dernier paramètre permet de fournir une valeur qui sera ajoutée à tous
les indices juste avant de les faire correspondre aux vertices. Enfin, le dernier paramètre est un décalage pour le
rendu instancié.

Lancez le programme et vous devriez avoir ceci :

![](/images/indexed_rectangle.png)

Vous savez maintenant économiser la mémoire en réutilisant les vertices à l'aide d'un index buffer. Cela deviendra
crucial pour les chapitres suivants dans lesquels vous allez apprendre à charger des modèles complexes.

Nous avons déjà évoqué le fait que le plus de buffers possibles devraient être stockés dans un seul emplacement
mémoire. Il faudrait dans l'idéal allez encore plus loin :
[les développeurs des drivers recommandent](https://developer.nvidia.com/vulkan-memory-management) également que vous
placiez plusieurs buffers dans un seul et même `VkBuffer`, et que vous utilisiez des décalages pour les différencier
dans les fonctions comme `vkCmdBindVertexBuffers`. Cela simplifie la mise des données dans des caches car elles sont
regroupées en un bloc. Il devient même possible d'utiliser la même mémoire pour plusieurs ressources si elles ne sont
pas utilisées en même temps et si elles sont proprement mises à jour. Cette pratique s'appelle d'ailleurs *aliasing*, et
certaines fonctions Vulkan possèdent un paramètre qui permet au développeur d'indiquer s'il veut utiliser la technique.

[Code C++](/code/20_index_buffer.cpp) /
[Vertex shader](/code/17_shader_vertexbuffer.vert) /
[Fragment shader](/code/17_shader_vertexbuffer.frag)
