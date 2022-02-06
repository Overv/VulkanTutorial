## Introduction

Jusqu'à présent nous avons écrit les couleurs dans les données de chaque sommet, pratique peu efficace. Nous allons 
maintenant implémenter l'échantillonnage (sampling) des textures, afin que le rendu soit plus intéressant. Nous
pourrons ensuite passer à l'affichage de modèles 3D dans de futurs chapitres.

L'ajout d'une texture comprend les étapes suivantes :

* Créer un objet *image* stocké sur la mémoire de la carte graphique
* La remplir avec les pixels extraits d'un fichier image
* Créer un sampler
* Ajouter un descripteur pour l'échantillonnage de l'image

Nous avons déjà travaillé avec des images, mais nous n'en avons jamais créé. Celles que nous avons manipulées avaient
été automatiquement crées par la swap chain. Créer une image et la remplir de pixels ressemble à la création d'un vertex
buffer. Nous allons donc commencer par créer une ressource intermédiaire pour y faire transiter les données que nous
voulons retrouver dans l'image. Bien qu'il soit possible d'utiliser une image comme intermédiaire, il est aussi autorisé
de créer un `VkBuffer` comme intermédiaire vers l'image, et cette méthode est
[plus rapide sur certaines plateformes](https://developer.nvidia.com/vulkan-memory-management). Nous allons donc
d'abord créer un buffer et y mettre les données relatives aux pixels. Pour l'image nous devrons nous enquérir des
spécificités de la mémoire, allouer la mémoire nécessaire et y copier les pixels. Cette procédure est très
similaire à la création de buffers.

La grande différence - il en fallait une tout de même - réside dans l'organisation des données à l'intérieur même des
pixels. Leur organisation affecte la manière dont les données brutes de la mémoire sont interprétées. De plus, stocker
les pixels ligne par ligne n'est pas forcément ce qui se fait de plus efficace, et cela est dû à la manière dont les
cartes graphiques fonctionnent. Nous devrons donc faire en sorte que les images soient organisées de la meilleure
manière possible. Nous avons déjà croisé certaines organisation lors de la création de la passe de rendu :

* `VK_IMAGE_LAYOUT_PRESENT_SCR_KHR` : optimal pour la présentation
* `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL` : optimal pour être l'attachement cible du fragment shader donc en tant que
cible de rendu
* `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL` : optimal pour être la source d'un transfert comme `vkCmdCopyImageToBuffer`
* `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` : optimal pour être la cible d'un transfert comme `vkCmdCopyBufferToImage`
* `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` : optimal pour être échantillonné depuis un shader

La plus commune des méthode spour réaliser une transition entre différentes organisations est la *barrière pipeline*. 
Celles-ci sont principalement utilisées pour synchroniser l'accès à une ressource, mais peuvent aussi permettre la
transition d'un état à un autre. Dans ce chapitre nous allons utiliser cette seconde possibilité. Les barrières peuvent 
enfin être utilisées pour changer la queue family qui possède une ressource.

## Librairie de chargement d'image

De nombreuses librairies de chargement d'images existent ; vous pouvez même écrire la vôtre pour des formats simples
comme BMP ou PPM. Nous allons utiliser stb_image, de [la collection stb](https://github.com/nothings/stb). Elle
possède l'avantage d'être écrite en un seul fichier. Téléchargez donc `stb_image.h` et placez-la ou vous voulez, par
exemple dans le dossier où sont stockés GLFW et GLM.

**Visual Studio**

Ajoutez le dossier comprenant `stb_image.h` dans `Additional Include Directories`.

![](/images/include_dirs_stb.png)

**Makefile**

Ajoutez le dossier comprenant `stb_image.h` aux chemins parcourus par GCC :

```text
VULKAN_SDK_PATH = /home/user/VulkanSDK/x.x.x.x/x86_64
STB_INCLUDE_PATH = /home/user/libraries/stb

...

CFLAGS = -std=c++17 -I$(VULKAN_SDK_PATH)/include -I$(STB_INCLUDE_PATH)
```

## Charger une image

Incluez la librairie de cette manière :

```c++
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
```

Le header simple ne fournit que les prototypes des fonctions. Nous devons demander les implémentations avec la define 
`STB_IMAGE_IMPLEMENTATION` pour ne pas avoir d'erreurs à l'édition des liens.

```c++
void initVulkan() {
    ...
    createCommandPool();
    createTextureImage();
    createVertexBuffer();
    ...
}

...

void createTextureImage() {

}
```

Créez la fonction `createTextureImage`, depuis laquelle nous chargerons une image et la placerons dans un objet Vulkan
représentant une image. Nous allons avoir besoin de command buffers, il faut donc appeler cette fonction après
`createCommandPool`.

Créez un dossier `textures` au même endroit que `shaders` pour y placer les textures. Nous allons y mettre un fichier
appelé `texture.jpg` pour l'utiliser dans notre programme. J'ai choisi d'utiliser
[cette image de license CC0](https://pixbay.com/en/statue-sculpture-fig-historically-1275469) redimensionnée à 512x512,
mais vous pouvez bien sûr en utiliser une autre. La librairie supporte des formats tels que JPEG, PNG, BMP ou GIF.

![](/images/texture.jpg)

Le chargement d'une image est très facile avec cette librairie :

```c++
void createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("échec du chargement d'une image!");
    }
}
```

La fonction `stbi_load` prend en argument le chemin de l'image et les différentes canaux à charger. L'argument 
`STBI_rgb_alpha` force la fonction à créer un canal alpha même si l'image originale n'en possède pas. Cela simplifie le
travail en homogénéisant les situations. Les trois arguments transmis en addresse servent de résultats pour stocker
des informations sur l'image. Les pixels sont retournés sous forme du pointeur `stbi_uc *pixels`. Ils sont organisés
ligne par ligne et ont chacun 4 octets, ce qui représente `texWidth * texHeight * 4` octets au total pour l'image.

## Buffer intermédiaire

Nous allons maintenant créer un buffer en mémoire accessible pour que nous puissions utiliser `vkMapMemory` et y placer
les pixels. Ajoutez les variables suivantes à la fonction pour contenir ce buffer temporaire :

```c++
VkBuffer stagingBuffer;
VkDeviceMemory stagingBufferMemory;
```

Le buffer doit être en mémoire visible pour que nous puissions le mapper, et il doit être utilisable comme source d'un
transfert vers une image, d'où l'appel suivant :

```c++
createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
```

Nous pouvons placer tel quels les pixels que nous avons récupérés dans le buffer :

```c++
void* data;
vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
vkUnmapMemory(device, stagingBufferMemory);
```

Il ne faut surtout pas oublier de libérer le tableau de pixels après cette opération :

```c++
stbi_image_free(pixels);
```

## Texture d'image

Bien qu'il nous soit possible de paramétrer le shader afin qu'il utilise le buffer comme source de pixels, il est bien
plus efficace d'utiliser un objet image. Ils rendent plus pratique, mais surtout plus rapide, l'accès aux données de
l'image en nous permettant d'utiliser des coordonnées 2D. Les pixels sont appelés texels dans le contexte du shading, et
nous utiliserons ce terme à partir de maintenant. Ajoutez les membres données suivants :

```c++
VkImage textureImage;
VkDeviceMemory textureImageMemory;
```

Les paramètres pour la création d'une image sont indiqués dans une structure de type `VkImageCreateInfo` :

```c++
VkImageCreateInfo imageInfo{};
imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
imageInfo.imageType = VK_IMAGE_TYPE_2D;
imageInfo.extent.width = static_cast<uint32_t>(texWidth);
imageInfo.extent.height = static_cast<uint32_t>(texHeight);
imageInfo.extent.depth = 1;
imageInfo.mipLevels = 1;
imageInfo.arrayLayers = 1;
```

Le type d'image contenu dans `imageType` indique à Vulkan le repère dans lesquels les texels sont placés. Il est
possible de créer des repères 1D, 2D et 3D. Les images 1D peuvent être utilisés comme des tableaux ou des gradients. Les
images 2D sont majoritairement utilisés comme textures. Certaines techniques les utilisent pour stocker autre chose 
que des couleur, par exemple des vecteurs. Les images 3D peuvent être utilisées pour stocker des voxels par
exemple. Le champ `extent` indique la taille de l'image, en terme de texels par axe. Comme notre texture fonctionne
comme un plan dans un espace en 3D, nous devons indiquer `1` au champ `depth`. Finalement, notre texture n'est pas un
tableau, et nous verrons le mipmapping plus tard.

```c++
imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
```

Vulkan supporte de nombreux formats, mais nous devons utiliser le même format que les données présentes dans le buffer.

```c++
imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
```

Le champ `tiling` peut prendre deux valeurs :

* `VK_IMAGE_TILING_LINEAR` : les texels sont organisés ligne par ligne
* `VK_IMAGE_TILING_OPTIMAL` : les texels sont organisés de la manière la plus optimale pour l'implémentation

Le mode mis dans `tiling` ne peut pas être changé, au contraire de l'organisation de l'image. Par conséquent, si vous
voulez pouvoir directement accéder aux texels, comme il faut qu'il soient organisés d'une manière logique, il vous faut
indiquer `VK_IMAGE_TILING_LINEAR`. Comme nous utilisons un buffer intermédiaire et non une image intermédiaire, nous
pouvons utiliser le mode le plus efficace.

```c++
imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
```

Idem, il n'existe que deux valeurs pour `initialLayout` :

* `VK_IMAGE_LAYOUT_UNDEFINED` : inutilisable par le GPU, son contenu sera éliminé à la première transition
* `VK_IMAGE_LAYOUT_PREINITIALIZED` : inutilisable par le GPU, mais la première transition conservera les texels

Il n'existe que quelques situations où il est nécessaire de préserver les texels pendant la première transition. L'une
d'elle consiste à utiliser l'image comme ressource intermédiaire en combinaison avec `VK_IMAGE_TILING_LINEAR`. Il
faudrait dans ce cas la faire transitionner vers un état source de transfert, sans perte de données. Cependant nous
utilisons un buffer comme ressource intermédiaire, et l'image transitionne d'abord vers cible de transfert. À ce
moment-là elle n'a pas de donnée intéressante.

```c++
imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
```

Le champ de bits `usage` fonctionne de la même manière que pour la création des buffers. L'image sera destination
d'un transfert, et sera utilisée par les shaders, d'où les deux indications ci-dessus.

```c++
imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
```

L'image ne sera utilisée que par une famille de queues : celle des graphismes (qui rappelons-le supporte
implicitement les transferts). Si vous avez choisi d'utiliser une queue spécifique vous devrez mettre 
`VK_SHARING_MODE_CONCURENT`.

```c++
imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
imageInfo.flags = 0; // Optionnel
```

Le membre `sample` se réfère au multisampling. Il n'a de sens que pour les images utilisées comme attachements d'un
framebuffer, nous devons donc mettre `1`, traduit par `VK_SAMPLE_COUNT_1_BIT`. Finalement, certaines informations se
réfèrent aux *images étendues*. Ces image étendues sont des images dont seule une partie est stockée dans la mémoire.
Voici une exemple d'utilisation : si vous utilisiez une image 3D pour représenter un terrain à l'aide de voxels, vous
pourriez utiliser cette fonctionnalité pour éviter d'utiliser de la mémoire qui au final ne contiendrait que de l'air.
Nous ne verrons pas cette fonctionnalité dans ce tutoriel, donnez à `flags` la valeur `0`.

```c++
if (vkCreateImage(device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
    throw std::runtime_error("echec de la creation d'une image!");
}
```

L'image est créée par la fonction `vkCreateImage`, qui ne possède pas d'argument particulièrement intéressant. Il est
possible que le format `VK_FORMAT_R8G8B8A8_SRGB` ne soit pas supporté par la carte graphique, mais c'est tellement peu
probable que nous ne verrons pas comment y remédier. En effet utiliser un autre format demanderait de réaliser plusieurs
conversions compliquées. Nous reviendrons sur ces conversions dans le chapitre sur le buffer de profondeur.

```c++
VkMemoryRequirements memRequirements;
vkGetImageMemoryRequirements(device, textureImage, &memRequirements);

VkMemoryAllocateInfo allocInfo{};
allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
allocInfo.allocationSize = memRequirements.size;
allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

if (vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
    throw std::runtime_error("echec de l'allocation de la mémoire pour l'image!");
}

vkBindImageMemory(device, textureImage, textureImageMemory, 0);
```

L'allocation de la mémoire nécessaire à une image fonctionne également de la même façon que pour un buffer. Seuls les
noms de deux fonctions changent : `vkGetBufferMemoryRequirements` devient `vkGetImageMemoryRequirements` et
`vkBindBufferMemory` devient `vkBindImageMemory`.

Cette fonction est déjà assez grande ainsi, et comme nous aurons besoin d'autres images dans de futurs chapitres, il est
judicieux de déplacer la logique de leur création dans une fonction, comme nous l'avons fait pour les buffers. Voici 
donc la fonction `createImage` :

```c++
void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("echec de la creation d'une image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("echec de l'allocation de la memoire d'une image!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}
```

La largeur, la hauteur, le mode de tiling, l'usage et les propriétés de la mémoire sont des paramètres car ils varierons
toujours entre les différentes images que nous créerons dans ce tutoriel.

La fonction `createTextureImage` peut maintenant être réduite à ceci :

```c++
void createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("échec du chargement de l'image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
}
```

## Transitions de l'organisation

La fonction que nous allons écrire inclut l'enregistrement et l'exécution de command buffers. Il est donc également
judicieux de placer cette logique dans une autre fonction :

```c++
VkCommandBuffer beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
```

Le code de ces fonctions est basé sur celui de `copyBuffer`. Vous pouvez maintenant réduire `copyBuffer` à :

```c++
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}
```

Si nous utilisions de simples buffers nous pourrions nous contenter d'écrire une fonction qui enregistre l'appel à 
`vkCmdCopyBufferToImage`. Mais comme cette fonction utilse une image comme cible nous devons changer l'organisation de
l'image avant l'appel. Créez une nouvelle fonction pour gérer de manière générique les transitions :

```c++
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    endSingleTimeCommands(commandBuffer);
}
```

L'une des manières de réaliser une transition consiste à utiliser une *barrière pour mémoire d'image*. Une telle barrière
de pipeline est en général utilisée pour synchroniser l'accès à une ressource, mais nous avons déjà évoqué ce sujet. Il
existe au passage un équivalent pour les buffers : une barrière pour mémoire de buffer.

```c++
VkImageMemoryBarrier barrier{};
barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
barrier.oldLayout = oldLayout;
barrier.newLayout = newLayout;
```

Les deux premiers champs indiquent la transition à réaliser. Il est possible d'utiliser `VK_IMAGE_LAYOUT_UNDEFINED` pour 
`oldLayout` si le contenu de l'image ne vous intéresse pas.

```c++
barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
```

Ces deux paramètres sont utilisés pour transmettre la possession d'une queue à une autre. Il faut leur indiquer les
indices des familles de queues correspondantes. Comme nous ne les utilisons pas, nous devons les mettre à 
`VK_QUEUE_FAMILY_IGNORED`.

```c++
barrier.image = image;
barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
barrier.subresourceRange.baseMipLevel = 0;
barrier.subresourceRange.levelCount = 1;
barrier.subresourceRange.baseArrayLayer = 0;
barrier.subresourceRange.layerCount = 1;
```

Les paramètres `image` et `subresourceRange` servent à indiquer l'image, puis la partie de l'image concernées par les
changements. Comme notre image n'est pas un tableau, et que nous n'avons pas mis en place de mipmapping, les
paramètres sont tous mis au minimum.

```c++
barrier.srcAccessMask = 0; // TODO
barrier.dstAccessMask = 0; // TODO
```

Comme les barrières sont avant tout des objets de synchronisation, nous devons indiquer les opérations utilisant la
ressource avant et après l'exécution de cette barrière. Pour pouvoir remplir les champs ci-dessus nous devons
déterminer ces opérations, ce que nous ferons plus tard.

```c++
vkCmdPipelineBarrier(
    commandBuffer,
    0 /* TODO */, 0 /* TODO */,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
);
```

Tous les types de barrière sont mis en place à l'aide de la même fonction. Le paramètre qui suit le command buffer
indique une étape de la pipeline. Durant celle-ci seront réalisées les opération devant précéder la barrière. Le
paramètre d'après indique également une étape de la pipeline. Cette fois les opérations exécutées durant cette étape
attendront la barrière. Les étapes que vous pouvez fournir comme avant- et après-barrière dépendent de l'utilisation
des ressources qui y sont utilisées. Les valeurs autorisées sont listées
[dans ce tableau](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap7.html#synchronization-access-types-supported).
Par exemple, si vous voulez lire des données présentes dans un UBO après une barrière qui s'applique au buffer, vous 
devrez indiquer `VK_ACCESS_UNIFORM_READ_BIT` comme usage, et si le premier shader à utiliser l'uniform est le fragment
shader il vous faudra indiquer `VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT` comme étape. Dans ce cas de figure, spécifier 
une autre étape qu'une étape shader n'aurait aucun sens, et les validation layers vous le feraient remarquer.

Le paramètre sur la troisième ligne peut être soit `0` soit `VK_DEPENDENCY_BY_REGION_BIT`. Dans ce second cas la
barrière devient une condition spécifique d'une région de la ressource. Cela signifie entre autres que l'implémentation
peut lire une région aussitôt que le transfert y est terminé, sans considération pour les autres régions. Cela permet
d'augmenter encore les performances en permettant d'utiliser les optimisations des architectures actuelles.

Les trois dernières paires de paramètres sont des tableaux de barrières pour chacun des trois types existants : barrière
mémorielle, barrière de buffer et barrière d'image.

## Copier un buffer dans une image

Avant de compléter `vkCreateTextureImage` nous allons écrire une dernière fonction appelée `copyBufferToImage` :

```c++
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    endSingleTimeCommands(commandBuffer);
}
```

Comme avec les recopies de buffers, nous devons indiquer les parties du buffer à copier et les parties de l'image où
écrire. Ces données doivent être placées dans une structure de type `VkBufferImageCopy`.

```c++
VkBufferImageCopy region{};
region.bufferOffset = 0;
region.bufferRowLength = 0;
region.bufferImageHeight = 0;

region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
region.imageSubresource.mipLevel = 0;
region.imageSubresource.baseArrayLayer = 0;
region.imageSubresource.layerCount = 1;

region.imageOffset = {0, 0, 0};
region.imageExtent = {
    width,
    height,
    1
};
```

La plupart de ces champs sont évidents. `bufferOffset` indique l'octet à partir duquel les données des pixels commencent
dans le buffer. L'organisation des pixels doit être indiquée dans les champs `bufferRowLenght` et `bufferImageHeight`.
Il pourrait en effet avoir un espace entre les lignes de l'image. Comme notre image est en un seul bloc, nous devons
mettre ces paramètres à `0`. Enfin, les membres `imageSubResource`, `imageOffset` et `imageExtent` indiquent les parties
de l'image qui receveront les données.

Les copies buffer vers image sont envoyées à la queue avec la fonction `vkCmdCopyBufferToImage`.

```c++
vkCmdCopyBufferToImage(
    commandBuffer,
    buffer,
    image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region
);
```

Le quatrième paramètre indique l'organisation de l'image au moment de la copie. Normalement l'image doit être dans
l'organisation optimale pour la réception de données. Nous avons paramétré la copie pour qu'un seul command buffer
soit à l'origine de la copie successive de tous les pixels. Nous aurions aussi pu créer un tableau de
`VkBufferImageCopy` pour que le command buffer soit à l'origine de plusieurs copies simultanées.

## Préparer la texture d'image

Nous avons maintenant tous les outils nécessaires pour compléter la mise en place de la texture d'image. Nous pouvons
retourner à la fonction `createTextureImage`. La dernière chose que nous y avions fait consistait à créer l'image
texture. Notre prochaine étape est donc d'y placer les pixels en les copiant depuis le buffer intermédiaire. Il y a deux
étapes pour cela :

* Transitionner l'organisation de l'image vers `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`
* Exécuter le buffer de copie

C'est simple à réaliser avec les fonctions que nous venons de créer :

```c++
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
```

Nous avons créé l'image avec une organisation `VK_LAYOUT_UNDEFINED`, car le contenu initial ne nous intéresse pas.

Pour ensuite pouvoir échantillonner la texture depuis le fragment shader nous devons réaliser une dernière transition,
qui la préparera à être accédée depuis un shader :

```c++
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
```

## Derniers champs de la barrière de transition

Si vous lanciez le programme vous verrez que les validation layers vous indiquent que les champs d'accès et d'étapes
shader sont invalides. C'est normal, nous ne les avons pas remplis.

Nous sommes pour le moment interessés par deux transitions :

* Non défini → cible d'un transfert : écritures par transfert qui n'ont pas besoin d'être synchronisées
* Cible d'un transfert → lecture par un shader : la lecture par le shader doit attendre la fin du transfert

Ces règles sont indiquées en utilisant les valeurs suivantes pour l'accès et les étapes shader :

```c++
VkPipelineStageFlags sourceStage;
VkPipelineStageFlags destinationStage;

if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
} else {
    throw std::invalid_argument("transition d'orgisation non supportée!");
}

vkCmdPipelineBarrier(
    commandBuffer,
    sourceStage, destinationStage,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
);
```

Comme vous avez pu le voir dans le tableau mentionné plus haut, l'écriture dans l'image doit se réaliser à l'étape
pipeline de transfert. Mais cette opération d'écriture ne dépend d'aucune autre opération. Nous pouvons donc fournir
une condition d'accès nulle et `VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT` comme opération pré-barrière. Cette valeur correspond
au début de la pipeline, mais ne représente pas vraiment une étape. Elle désigne plutôt le moment où la pipeline se
prépare, et donc sert communément aux transferts. Voyez
[la documentation](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap7.html#VkPipelineStageFlagBits)
pour de plus amples informations sur les pseudo-étapes.

L'image sera écrite puis lue dans la même passe, c'est pourquoi nous devons indiquer que le fragment shader aura accès à
la mémoire de l'image.

Quand nous aurons besoin de plus de transitions, nous compléterons la fonction de transition pour qu'elle les prenne en
compte. L'application devrait maintenant tourner sans problème, bien qu'il n'y aie aucune différence visible.

Un point intéressant est que l'émission du command buffer génère implicitement une synchronisation de type 
`VK_ACCESS_HOST_WRITE_BIT`. Comme la fonction `transitionImageLayout` exécute un command buffer ne comprenant qu'une
seule commande, il est possbile d'utiliser cette synchronisation. Cela signifie que vous pourriez alors mettre
`srcAccessMask` à `0` dans le cas d'une transition vers `VK_ACCESS_HOST_WRITE_BIT`. C'est à vous de voir si vous
voulez être explicites à ce sujet. Personnellement je n'aime pas du tout faire dépendre mon application sur des
opérations cachées, que je trouve dangereusement proche d'OpenGL.

Autre chose intéressante à savoir, il existe une organisation qui supporte toutes les opérations. Elle s'appelle 
`VK_IMAGE_LAYOUT_GENERAL`. Le problème est qu'elle est évidemment moins optimisée. Elle est cependant utile dans
certains cas, comme quand une image doit être utilisée comme cible et comme source, ou pour pouvoir lire l'image juste
après qu'elle aie quittée l'organisation préinitialisée.

Enfin, il important de noter que les fonctions que nous avons mises en place exécutent les commandes de manière
synchronisées et attendent que la queue soit en pause. Pour de véritables applications il est bien sûr recommandé de
combiner toutes ces opérations dans un seul command buffer pour qu'elles soient exécutées de manière asynchrones. Les
commandes de transitions et de copie pourraient grandement bénéficier d'une telle pratique. Essayez par exemple de créer
une fonction `setupCommandBuffer`, puis d'enregistrer les commandes nécessaires depuis les fonctions actuelles.
Appelez ensuite une autre fonction nommée par exemple `flushSetupCommands` qui exécutera le command buffer. Avant
d'implémenter ceci attendez que nous ayons fait fonctionner l'échantillonage.

## Nettoyage

Complétez la fonction `createImageTexture` en libérant le buffer intermédiaire et en libérant la mémoire :

```c++
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}
```

L'image texture est utilisée jusqu'à la fin du programme, nous devons donc la libérer dans `cleanup` :

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);

    ...
}
```

L'image contient maintenant la texture, mais nous n'avons toujours pas mis en place de quoi y accéder depuis la
pipeline. Nous y travaillerons dans le prochain chapitre.

[C++ code](/code/23_texture_image.cpp) /
[Vertex shader](/code/21_shader_ubo.vert) /
[Fragment shader](/code/21_shader_ubo.frag)
