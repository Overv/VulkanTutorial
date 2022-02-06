## Introduction

Notre programme peut maintenant générer plusieurs niveaux de détails pour les textures qu'il utilise. Ces images sont
plus lisses quand vues de loin. Cependant on peut voir des motifs en dent de scie si on regarde les textures de plus
près. Ceci est particulièrement visible sur le rendu de carrés :

![](/images/texcoord_visualization.png)

Cet effet indésirable s'appelle "aliasing". Il est dû au manque de pixels pour afficher tous les détails de la
géométrie. Il sera toujours visible, par contre nous pouvons utiliser des techniques pour le réduire considérablement.
Nous allons ici implémenter le [multisample anti-aliasing](https://en.wikipedia.org/wiki/Multisample_anti-aliasing),
terme condensé en MSAA.

Dans un rendu standard, la couleur d'un pixel est déterminée à partir d'un unique sample, en général le centre du pixel.
Si une ligne passe partiellement par un pixel sans en toucher le centre, sa contribution à la couleur sera nulle. Nous
voudrions plutôt qu'il y contribue partiellement.

![](/images/aliasing.png)

Le MSAA consiste à utiliser plusieurs points dans un pixel pour déterminer la couleur d'un pixel. Comme on peut s'y
attendre, plus de points offrent un meilleur résultat, mais consomment plus de ressources.

![](/images/antialiasing.png)

Nous allons utiliser le maximum de points possible. Si votre application nécessite plus de performances, il vous suffira
de réduire ce nombre.

## Récupération du nombre maximal de samples

Commençons par déterminer le nombre maximal de samples que la carte graphique supporte. Les GPUs modernes supportent au
moins 8 points, mais il peut tout de même différer entre modèles. Nous allons stocker ce nombre dans un membre donnée :

```c++
...
VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
...
```

Par défaut nous n'utilisons qu'un point, ce qui correspond à ne pas utiliser de multisampling. Le nombre maximal est
inscrit dans la structure de type `VkPhysicalDeviceProperties` associée au GPU. Comme nous utilisons un buffer de
profondeur, nous devons prendre en compte le nombre de samples pour la couleur et pour la profondeur. Le plus haut taux
de samples supporté par les deux (&) sera celui que nous utiliserons. Créez une fonction dans laquelle les informations
seront récupérées :

```c++
VkSampleCountFlagBits getMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}
```

Nous allons maintenant utiliser cette fonction pour donner une valeur à `msaaSamples` pendant la sélection du GPU. Nous
devons modifier la fonction `pickPhysicalDevice` :

```c++
void pickPhysicalDevice() {
    ...
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            msaaSamples = getMaxUsableSampleCount();
            break;
        }
    }
    ...
}
```

## Mettre en place une cible de rendu

Le MSAA consiste à écrire chaque pixel dans un buffer indépendant de l'affichage, dont le contenu est ensuite rendu en
le résolvant à un framebuffer standard. Cette étape est nécessaire car le premier buffer est une image particulière :
elle doit supporter plus d'un échantillon par pixel. Il ne peut pas être utilisé comme framebuffer dans la swap chain.
Nous allons donc devoir changer notre rendu. Nous n'aurons besoin que d'une cible de rendu, car seule une opération
de rendu n'est autorisée à s'exécuter à un instant donné. Créez les membres données suivants :

```c++
...
VkImage colorImage;
VkDeviceMemory colorImageMemory;
VkImageView colorImageView;
...
```

Cette image doit supporter le nombre de samples déterminé auparavant, nous devons donc le lui fournir durant sa
création. Ajoutez un paramètre `numSamples` à la fonction `createImage` :

```c++
void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    ...
    imageInfo.samples = numSamples;
    ...
```

Mettez à jour tous les appels avec `VK_SAMPLE_COUNT_1_BIT`. Nous changerons cette valeur pour la nouvelle image.

```c++
createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
...
createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
```

Nous allons maintenant créer un buffer de couleur à plusieurs samples. Créez la fonction `createColorResources`, et
passez `msaaSamples` à `createImage` depuis cette fonction. Nous n'utilisons également qu'un niveau de mipmap, ce qui
est nécessaire pour conformer à la spécification de Vulkan. Mais de toute façon cette image n'a pas besoin de mipmaps.

```c++
void createColorResources() {
    VkFormat colorFormat = swapChainImageFormat;

    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
    colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}
```

Pour une question de cohérence mettons cette fonction juste avant `createDepthResource`.

```c++
void initVulkan() {
    ...
    createColorResources();
    createDepthResources();
    ...
}
```

Nous avons maintenant un buffer de couleurs qui utilise le multisampling. Occupons-nous maintenant de la profondeur.
Modifiez `createDepthResources` et changez le nombre de samples utilisé :

```c++
void createDepthResources() {
    ...
    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    ...
}
```

Comme nous avons créé quelques ressources, nous devons les libérer :

```c++
void cleanupSwapChain() {
    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);
    ...
}
```

Mettez également à jour `recreateSwapChain` pour prendre en charge les recréations de l'image couleur.

```c++
void recreateSwapChain() {
    ...
    createGraphicsPipeline();
    createColorResources();
    createDepthResources();
    ...
}
```

Nous avons fini le paramétrage initial du MSAA. Nous devons maintenant utiliser ces ressources dans la pipeline, le
framebuffer et la render pass!

## Ajouter de nouveaux attachements

Gérons d'abord la render pass. Modifiez `createRenderPass` et changez-y la création des attachements de couleur et de
profondeur.

```c++
void createRenderPass() {
    ...
    colorAttachment.samples = msaaSamples;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ...
    depthAttachment.samples = msaaSamples;
    ...
```

Nous avons changé l'organisation finale à `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`, car les images qui utilisent le
multisampling ne peuvent être présentées directement. Nous devons la convertir en une image plus classique. Nous
n'aurons pas à convertir le buffer de profondeur, dans la mesure où il ne sera jamais présenté. Nous avons donc besoin
d'un nouvel attachement pour la couleur, dans lequel les pixels seront résolus.

```c++
    ...
    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    ...
```

La render pass doit maintenant être configurée pour résoudre l'attachement multisamplé en un attachement simple.
Créez une nouvelle référence au futur attachement qui contiendra le buffer de pixels résolus :

```c++
    ...
    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ...
```

Ajoutez la référence à l'attachement dans le membre `pResolveAttachments` de la structure de création de la subpasse.
La subpasse n'a besoin que de cela pour déterminer l'opération de résolution du multisampling :

```
    ...
    subpass.pResolveAttachments = &colorAttachmentResolveRef;
    ...
```

Fournissez ensuite l'attachement de couleur à la structure de création de la render pass.

```c++
    ...
    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
    ...
```

Modifiez ensuite `createFramebuffer` afin de d'ajouter une image view de couleur à la liste :

```c++
void createFrameBuffers() {
        ...
        std::array<VkImageView, 3> attachments = {
            colorImageView,
            depthImageView,
            swapChainImageViews[i]
        };
        ...
}
```

Il ne reste plus qu'à informer la pipeline du nombre de samples à utiliser pour les opérations de rendu.

```c++
void createGraphicsPipeline() {
    ...
    multisampling.rasterizationSamples = msaaSamples;
    ...
}
```

Lancez votre programme et vous devriez voir ceci :

![](/images/multisampling.png)

Comme pour le mipmapping, la différence n'est pas forcément visible immédiatement. En y regardant de plus près, vous
pouvez normalement voir que, par exemple, les bords sont beaucoup plus lisses qu'avant.

![](/images/multisampling_comparison.png)

La différence est encore plus visible en zoomant sur un bord :

![](/images/multisampling_comparison2.png)

## Amélioration de la qualité

Notre implémentation du MSAA est limitée, et ces limitations impactent la qualité. Il existe un autre problème
d'aliasing dû aux shaders qui n'est pas résolu par le MSAA. En effet cette technique ne permet que de lisser les bords
de la géométrie, mais pas les lignes contenus dans les textures. Ces bords internes sont particulièrement visibles dans
le cas de couleurs qui contrastent beaucoup. Pour résoudre ce problème nous pouvons activer le
[sample shading](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap27.html#primsrast-sampleshading), qui
améliore encore la qualité de l'image au prix de performances encore réduites.

```c++

void createLogicalDevice() {
    ...
    deviceFeatures.sampleRateShading = VK_TRUE; // Activation du sample shading pour le device
    ...
}

void createGraphicsPipeline() {
    ...
    multisampling.sampleShadingEnable = VK_TRUE; // Activation du sample shading dans la pipeline
    multisampling.minSampleShading = .2f; // Fraction minimale pour le sample shading; plus proche de 1 lisse d'autant plus
    ...
}
```

Dans notre tutoriel nous désactiverons le sample shading, mais dans certain cas son activation permet une nette
amélioration de la qualité du rendu :

![](/images/sample_shading.png)

## Conclusion

Il nous a fallu beaucoup de travail pour en arriver là, mais vous avez maintenant une bonne connaissances des bases de
Vulkan. Ces connaissances vous permettent maintenant d'explorer d'autres fonctionnalités, comme :

* Push constants
* Instanced rendering
* Uniforms dynamiques
* Descripteurs d'images et de samplers séparés
* Pipeline caching
* Génération des command buffers depuis plusieurs threads
* Multiples subpasses
* Compute shaders

Le programme actuel peut être grandement étendu, par exemple en ajoutant l'éclairage Blinn-Phong, des effets en
post-processing et du shadow mapping. Vous devriez pouvoir apprendre ces techniques depuis des tutoriels conçus pour
d'autres APIs, car la plupart des concepts sont applicables à Vulkan.

[Code C++](/code/29_multisampling.cpp) /
[Vertex shader](/code/26_shader_depth.vert) /
[Fragment shader](/code/26_shader_depth.frag)
