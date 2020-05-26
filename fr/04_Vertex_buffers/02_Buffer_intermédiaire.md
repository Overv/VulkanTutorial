## Introduction

Nous avons maintenant un vertex buffer fonctionnel. Par contre il n'est pas dans la mémoire la plus optimale posible
pour la carte graphique. Il serait préférable d'utiliser une mémoire `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`,
mais de telles mémoires ne sont pas accessibles depuis le CPU. Dans ce chapitre nous allons créer deux vertex buffers.
Le premier, un buffer intermédiaire (*staging buffer*), sera stocké dans de la mémoire accessible depuis le CPU, et
nous y mettrons nos données. Le second sera directement dans la carte graphique, et nous y copierons les données des
vertices depuis le buffer intermédiaire.

## Queue de transfert

La commande de copie des buffers provient d'une queue family qui supporte les opérations de transfert, ce qui est
indiqué par `VK_QUEUE_TRANFER_BIT`. Une bonne nouvelle : toute queue qui supporte les graphismes ou le calcul doit
supporter les transferts. Par contre il n'est pas obligatoire pour ces queues de l'indiquer dans le champ de bit qui les
décrit.

Si vous aimez la difficulté, vous pouvez préférer l'utilisation d'une queue spécifique aux opérations de transfert. Vous
aurez alors ceci à changer :

* Modifier la structure `QueueFamilyIndices` et la fonction `findQueueFamilies` pour obtenir une queue family dont la
description comprend `VK_QUEUE_TRANSFER_BIT` mais pas `VK_QUEUE_GRAPHICS_BIT`
* Modifier `createLogicalDevice` pour y récupérer une référence à une queue de transfert
* Créer une command pool pour les command buffers envoyés à la queue de transfert
* Changer la valeur de `sharingMode` pour les ressources qui le demandent à `VK_SHARING_MODE_CONCURRENT`, et indiquer à
la fois la queue des graphismes et la queue ds transferts
* Émettre toutes les commandes de transfert telles `vkCmdCopyBuffer` - nous allons l'utiliser dans ce chapitre - à la
queue de transfert au lieu de la queue des graphismes

Cela représente pas mal de travail, mais vous en apprendrez beaucoup sur la gestion des resources entre les queue
families.

## Abstraction de la création des buffers

Comme nous allons créer plusieurs buffers, il serait judicieux de placer la logique dans une fonction. Appelez-la 
`createBuffer` et déplacez-y le code suivant :

```c++
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("echec de la creation d'un buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("echec de l'allocation de memoire!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}
```

Cette fonction nécessite plusieurs paramètres, tels que la taille du buffer, les propriétés dont nous avons besoin et
l'utilisation type du buffer. La fonction a deux résultats, elle fonctionne donc en modifiant la valeur des deux
derniers paramètres, dans lesquels elle place les référernces aux objets créés.

Vous pouvez maintenant supprimer la création du buffer et l'allocation de la mémoire de `createVertexBuffer` et
remplacer tout ça par un appel à votre nouvelle fonction :

```c++
void createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);

    void* data;
    vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, vertexBufferMemory);
}
```

Lancez votre programme et assurez-vous que tout fonctionne toujours aussi bien.

## Utiliser un buffer intermédiaire

Nous allons maintenant faire en sorte que `createVertexBuffer` utilise d'abord un buffer visible pour copier les
données sur la carte graphique, puis qu'il utilise de la mémoire locale à la carte graphique pour le véritable buffer.

```c++
void createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
}
```

Nous utilisons ainsi un nouveau `stagingBuffer` lié à la `stagingBufferMemory` pour transmettre les données à la carte
graphique. Dans ce chapitre nous allons utiliser deux nouvelles valeurs pour les utilisations des buffers :

* `VK_BUFFER_USAGE_TRANSFER_SCR_BIT` : le buffer peut être utilisé comme source pour un transfert de mémoire
* `VK_BUFFER_USAGE_TRANSFER_DST_BIT` : le buffer peut être utilisé comme destination pour un transfert de mémoire

Le `vertexBuffer` est maintenant alloué à partir d'un type de mémoire local au device, ce qui implique en général que
nous ne pouvons pas utiliser `vkMapMemory`. Nous pouvons cependant bien sûr y copier les données depuis le buffer
intermédiaire. Nous pouvons indiquer que nous voulons transmettre des données entre ces buffers à l'aide des valeurs
que nous avons vues juste au-dessus. Nous pouvons combiner ces informations avec par exemple 
`VK_BUFFER_USAGE_VERTEX_BUFFER_BIT`.

Nous allons maintenant écrire la fonction `copyBuffer`, qui servira à recopier le contenu du buffer intermédiaire dans
le véritable buffer.

```c++
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

}
```

Les opérations de transfert de mémoire sont réalisées à travers un command buffer, comme pour l'affichage. Nous devons
commencer par allouer des command buffers temporaires. Vous devriez d'ailleurs utiliser une autre command pool pour
tous ces command buffer temporaires, afin de fournir à l'implémentation une occasion d'optimiser la gestion de la
mémoire séparément des graphismes. Si vous le faites, utilisez `VK_COMMAND_POOL_CREATE_TRANSIENT_BIT` pendant la 
création de la command pool, car les commands buffers ne seront utilisés qu'une seule fois.

```c++
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
}
```

Enregistrez ensuite le command buffer :

```c++
VkCommandBufferBeginInfo beginInfo{};
beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

vkBeginCommandBuffer(commandBuffer, &beginInfo);
```

Nous allons utiliser le command buffer une fois seulement, et attendre que la copie soit
terminée avant de sortir de la fonction. Il est alors préférable d'informer le driver de cela à l'aide de 
`VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT`.

```c++
VkBufferCopy copyRegion{};
copyRegion.srcOffset = 0; // Optionnel
copyRegion.dstOffset = 0; // Optionnel
copyRegion.size = size;
vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
```

La copie est réalisée à l'aide de la commande `vkCmdCopyBuffer`. Elle prend les buffers de source et d'arrivée comme
arguments, et un tableau des régions à copier. Ces régions sont décrites dans des structures de type `VkBufferCopy`, qui
consistent en un décalage dans le buffer source, le nombre d'octets à copier et le décalage dans le buffer d'arrivée. Il
n'est ici pas possible d'indiquer `VK_WHOLE_SIZE`.

```c++
vkEndCommandBuffer(commandBuffer);
```

Ce command buffer ne sert qu'à réaliser les copies des buffers, nous pouvons donc arrêter l'enregistrement dès
maintenant. Exécutez le command buffer pour compléter le transfert :

```c++
VkSubmitInfo submitInfo{};
submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers = &commandBuffer;

vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
vkQueueWaitIdle(graphicsQueue);
```

Au contraire des commandes d'affichage très complexes, il n'y a pas de synchronisation particulière à mettre en place.
Nous voulons simplement nous assurer que le transfert se réalise immédiatement. Deux possibilités s'offrent alors à
nous : utiliser une fence et l'attendre avec `vkWaitForFences`, ou simplement attendre avec `vkQueueWaitIdle` que la
queue des transfert soit au repos. Les fences permettent de préparer de nombreux transferts pour qu'ils s'exécutent
concurentiellement, et offrent au driver encore une manière d'optimiser le travail. L'autre méthode a l'avantage de la
simplicité. Implémentez le système de fence si vous le désirez, mais cela vous obligera à modifier l'organisation de ce
module.

```c++
vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
```

N'oubliez pas de libérer le command buffer utilisé pour l'opération de transfert.

Nous pouvons maintenant appeler `copyBuffer` depuis la fonction `createVertexBuffer` pour que les sommets soient enfin
stockées dans la mémoire locale.

```c++
createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
```

Maintenant que les données sont dans la carte graphique, nous n'avons plus besoin du buffer intermédiaire, et devons
donc le détruire.

```c++
    ...

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
```

Lancez votre programme pour vérifier que vous voyez toujours le même triangle. L'amélioration n'est peut-être pas
flagrante, mais il est clair que la mémoire permet d'améliorer les performances, préparant ainsi le terrain
pour le chargement de géométrie plus complexe.

## Conclusion

Notez que dans une application réelle, vous ne devez pas allouer de la mémoire avec `vkAllocateMemory` pour chaque
buffer. De toute façon le nombre d'appel à cette fonction est limité, par exemple à 4096, et ce même sur des cartes
graphiques comme les GTX 1080. La bonne pratique consiste à allouer une grande zone de mémoire et d'utiliser un
gestionnaire pour créer des décalages pour chacun des buffers. Il est même préférable d'utiliser un buffer pour
plusieurs types de données (sommets et uniformes par exemple) et de séparer ces types grâce à des indices dans le
buffer (voyez encore [ce même article](https://developer.nvidia.com/vulkan-memory-management)).

Vous pouvez implémenter votre propre solution, ou bien utiliser la librairie
[VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) crée par GPUOpen. Pour ce
tutoriel, ne vous inquiétez pas pour cela car nous n'atteindrons pas cette limite.

[Code C++](/code/19_staging_buffer.cpp) /
[Vertex shader](/code/17_shader_vertexbuffer.vert) /
[Fragment shader](/code/17_shader_vertexbuffer.frag)
