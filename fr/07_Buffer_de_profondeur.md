## Introduction

Jusqu'à présent nous avons projeté notre géométrie en 3D, mais elle n'est toujours définie qu'en 2D. Nous allons ajouter
l'axe Z dans ce chapitre pour permettre l'utilisation de modèles 3D. Nous placerons un carré au-dessus ce celui que nous
avons déjà, et nous verrons ce qui se passe si la géométrie n'est pas organisée par profondeur.

## Géométrie en 3D

Mettez à jour la structure `Vertex` pour que les coordonnées soient des vecteurs à 3 dimensions. Il faut également
changer le champ `format` dans la structure `VkVertexInputAttributeDescription` correspondant aux coordonnées :

```c++
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    ...

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        ...
    }
};
```

Mettez également à jour l'entrée du vertex shader qui correspond aux coordonnées. Recompilez le shader.

```glsl
layout(location = 0) in vec3 inPosition;

...

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
```

Enfin, il nous faut ajouter la profondeur là où nous créons les instances de `Vertex`.

```c++
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};
```

Si vous lancez l'application vous verrez exactement le même résultat. Il est maintenant temps d'ajouter de la géométrie
pour rendre la scène plus intéressante, et pour montrer le problème évoqué plus haut. Dupliquez les vertices afin qu'un
second carré soit rendu au-dessus de celui que nous avons maintenant :

![](/images/extra_square.svg)

Nous allons utiliser `-0.5f` comme coordonnée Z.

```c++
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};
```

Si vous lancez le programme maintenant vous verrez que le carré d'en-dessous est rendu au-dessus de l'autre :

![](/images/depth_issues.png)

Ce problème est simplement dû au fait que le carré d'en-dessous est placé après dans le tableau des vertices. Il y a
deux manières de régler ce problème :

* Trier tous les appels en fonction de la profondeur
* Utiliser un buffer de profondeur

La première approche est communément utilisée pour l'affichage d'objets transparents, car la transparence non ordonnée
est un problème difficile à résoudre. Cependant, pour la géométrie sans transparence, le buffer de profondeur est un
très bonne solution. Il consiste en un attachement supplémentaire au framebuffer, qui stocke les profondeurs. La
profondeur de chaque fragment produit par le rasterizer est comparée à la valeur déjà présente dans le buffer. Si le
fragment est plus distant que celui déjà traité, il est simplement éliminé. Il est possible de manipuler cette valeur de
la même manière que la couleur.

```c++
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
```

La matrice de perspective générée par GLM utilise par défaut la profondeur OpenGL comprise en -1 et 1. Nous pouvons
configurer GLM avec `GLM_FORCE_DEPTH_ZERO_TO_ONE` pour qu'elle utilise des valeurs correspondant à Vulkan.

## Image de pronfondeur et views sur cette image

L'attachement de profondeur est une image. La différence est que celle-ci n'est pas créée par la swap chain. Nous 
n'avons besoin que d'un seul attachement de profondeur, car les opérations sont séquentielles. L'attachement aura
encore besoin des trois mêmes ressources : une image, de la mémoire et une image view.

```c++
VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;
```

Créez une nouvelle fonction `createDepthResources` pour mettre en place ces ressources :

```c++
void initVulkan() {
    ...
    createCommandPool();
    createDepthResources();
    createTextureImage();
    ...
}

...

void createDepthResources() {

}
```

La création d'une image de profondeur est assez simple. Elle doit avoir la même résolution que l'attachement de couleur,
définie par l'étendue de la swap chain. Elle doit aussi être configurée comme image de profondeur, avoir un tiling
optimal et une mémoire placée sur la carte graphique. Une question persiste : quelle est l'organisation optimale pour
une image de profondeur? Le format contient un composant de profondeur, indiqué par `_Dxx_` dans les valeurs de type
`VK_FORMAT`.

Au contraire de l'image de texture, nous n'avons pas besoin de déterminer le format requis car nous n'accéderons pas à
cette texture nous-mêmes. Nous n'avons besoin que d'une précision suffisante, en général un minimum de 24 bits. Il y a
plusieurs formats qui satisfont cette nécéssité :

* `VK_FORMAT_D32_SFLOAT` : float signé de 32 bits pour la profondeur
* `VK_FORMAT_D32_SFLOAT_S8_UINT` : float signé de 32 bits pour la profondeur et int non signé de 8 bits pour le stencil
* `VK_FORMAT_D24_UNORM_S8_UINT` : float signé de 24 bits pour la profondeur et int non signé de 8 bits pour le stencil

Le composant de stencil est utilisé pour le [test de stencil](https://en.wikipedia.org/wiki/Stencil_buffer). C'est un
test additionnel qui peut être combiné avec le test de profondeur. Nous y reviendrons dans un futur chapitre.

Nous pourrions nous contenter d'utiliser `VK_FORMAT_D32_SFLOAT` car son support est pratiquement assuré, mais il est
préférable d'utiliser une fonction pour déterminer le meilleur format localement supporté. Créez pour cela la fonction 
`findSupportedFormat`. Elle vérifiera que les formats en argument sont supportés et choisira le meilleur en se basant
sur leur ordre dans le vecteurs des formats acceptables fourni en argument :

```c++
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {

}
```

Leur support dépend du mode de tiling et de l'usage, nous devons donc les transmettre en argument. Le support des
formats peut ensuite être demandé à l'aide de la fonction `vkGetPhysicalDeviceFormatProperties` :

```c++
for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
}
```

La structure `VkFormatProperties` contient trois champs :

* `linearTilingFeatures` : utilisations supportées avec le tiling linéaire
* `optimalTilingFeatures` : utilisations supportées avec le tiling optimal
* `bufferFeatures` : utilisations supportées avec les buffers

Seuls les deux premiers cas nous intéressent ici, et celui que nous vérifierons dépendra du mode de tiling fourni en
paramètre.

```c++
if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
    return format;
} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
    return format;
}
```

Si aucun des candidats ne supporte l'utilisation désirée, nous pouvons lever une exception.

```c++
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("aucun des formats demandés n'est supporté!");
}
```

Nous allons utiliser cette fonction depuis une autre fonction `findDepthFormat`. Elle sélectionnera un format 
avec un composant de profondeur qui supporte d'être un attachement de profondeur :

```c++
VkFormat findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}
```

Utilisez bien `VK_FORMAT_FEATURE_` au lieu de `VK_IMAGE_USAGE_`. Tous les candidats contiennent la profondeur, mais
certains ont le stencil en plus. Ainsi il est important de voir que dans ce cas, la profondeur n'est qu'une *capacité*
et non un *usage* exclusif. Autre point, nous devons prendre cela en compte pour les transitions d'organisation. Ajoutez
une fonction pour determiner si le format contient un composant de stencil ou non :

```c++
bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
```

Appelez cette fonction depuis `createDepthResources` pour déterminer le format de profondeur :

```c++
VkFormat depthFormat = findDepthFormat();
```

Nous avons maintenant toutes les informations nécessaires pour invoquer `createImage` et `createImageView`.

```c++
createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
depthImageView = createImageView(depthImage, depthFormat);
```

Cependant cette fonction part du principe que la `subresource` est toujours `VK_IMAGE_ASPECT_COLOR_BIT`, il nous faut
donc en faire un paramètre.

```c++
VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    ...
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    ...
}
```

Changez également les appels à cette fonction pour prendre en compte ce changement :

```c++
swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
...
depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
...
textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
```

Voilà tout pour la création de l'image de profondeur. Nous n'avons pas besoin d'y envoyer de données ou quoi que ce soit
de ce genre, car nous allons l'initialiser au début de la render pass tout comme l'attachement de couleur.

### Explicitement transitionner l'image de profondeur

Nous n'avons pas besoin de faire explicitement la transition du layout de l'image vers un attachement de profondeur parce
qu'on s'en occupe directement dans la render pass. En revanche, pour l'exhaustivité je vais quand même vous décrire le processus
dans cette section. Vous pouvez sauter cette étape si vous le souhaitez.

Faites un appel à `transitionImageLayout` à la fin de `createDepthResources` comme ceci:

```c++
transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
```

L'organisation indéfinie peut être utilisée comme organisation intiale, dans la mesure où aucun contenu d'origine n'a
d'importance. Nous devons faire évaluer la logique de `transitionImageLayout` pour qu'elle puisse utiliser la
bonne subresource.

```c++
if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (hasStencilComponent(format)) {
        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
} else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
}
```

Même si nous n'utilisons pas le composant de stencil, nous devons nous en occuper dans les transitions de l'image de
profondeur.

Ajoutez enfin le bon accès et les bonnes étapes pipeline :

```c++
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
} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
} else {
    throw std::invalid_argument("transition d'organisation non supportée!");
}
```

Le buffer de profondeur sera lu avant d'écrire un fragment, et écrit après qu'un fragment valide soit traité. La lecture
se passe en `VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT` et l'écriture en `VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT`.
Vous devriez choisir la première des étapes correspondant à l'opération correspondante, afin que tout soit prêt pour
l'utilisation de l'attachement de profondeur.

## Render pass

Nous allons modifier `createRenderPass` pour inclure l'attachement de profondeur. Spécifiez d'abord un
`VkAttachementDescription` :

```c++
VkAttachmentDescription depthAttachment{};
depthAttachment.format = findDepthFormat();
depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
```

Le `format` doit être celui de l'image de profondeur. Pour cette fois nous ne garderons pas les données de profondeur,
car nous n'en avons plus besoin après le rendu. Encore une fois le hardware pourra réaliser des optimisations. Et
de même nous n'avons pas besoin des valeurs du rendu précédent pour le début du rendu de la frame, nous pouvons donc
mettre `VK_IMAGE_LAYOUT_UNDEFINED` comme valeur pour `initialLayout`.

```c++
VkAttachmentReference depthAttachmentRef{};
depthAttachmentRef.attachment = 1;
depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
```

Ajoutez une référence à l'attachement dans notre seule et unique subpasse :

```c++
VkSubpassDescription subpass{};
subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
subpass.colorAttachmentCount = 1;
subpass.pColorAttachments = &colorAttachmentRef;
subpass.pDepthStencilAttachment = &depthAttachmentRef;
```

Les subpasses ne peuvent utiliser qu'un seul attachement de profondeur (et de stencil). Réaliser le test de profondeur
sur plusieurs buffers n'a de toute façon pas beaucoup de sens.

```c++
std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
VkRenderPassCreateInfo renderPassInfo{};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
renderPassInfo.pAttachments = attachments.data();
renderPassInfo.subpassCount = 1;
renderPassInfo.pSubpasses = &subpass;
renderPassInfo.dependencyCount = 1;
renderPassInfo.pDependencies = &dependency;
```

Changez enfin la structure `VkRenderPassCreateInfo` pour qu'elle se réfère aux deux attachements.

## Framebuffer

L'étape suivante va consister à modifier la création du framebuffer pour lier notre image de profondeur à l'attachement
de profondeur. Trouvez `createFramebuffers` et indiquez la view sur l'image de profondeur comme second attachement :

```c++
std::array<VkImageView, 2> attachments = {
    swapChainImageViews[i],
    depthImageView
};

VkFramebufferCreateInfo framebufferInfo{};
framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
framebufferInfo.renderPass = renderPass;
framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
framebufferInfo.pAttachments = attachments.data();
framebufferInfo.width = swapChainExtent.width;
framebufferInfo.height = swapChainExtent.height;
framebufferInfo.layers = 1;
```

L'attachement de couleur doit différer pour chaque image de la swap chain, mais l'attachement de profondeur peut être le
même pour toutes, car il n'est utilisé que par la subpasse, et la synchronisation que nous avons mise en place ne permet
pas l'exécution de plusieurs subpasses en même temps.

Nous devons également déplacer l'appel à `createFramebuffers` pour que la fonction ne soit appelée qu'après la création
de l'image de profondeur :

```c++
void initVulkan() {
    ...
    createDepthResources();
    createFramebuffers();
    ...
}
```

## Supprimer les valeurs

Comme nous avons plusieurs attachements avec `VK_ATTACHMENT_LOAD_OP_CLEAR`, nous devons spécifier plusieurs valeurs de
suppression. Allez à `createCommandBuffers` et créez un tableau de `VkClearValue` :

```c++
std::array<VkClearValue, 2> clearValues{};
clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
clearValues[1].depthStencil = {1.0f, 0};

renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
renderPassInfo.pClearValues = clearValues.data();
```

Avec Vulkan, `0.0` correspond au plan near et `1.0` au plan far. La valeur initiale doit donc être `1.0`, afin que tout
fragment puisse s'y afficher. Notez que l'ordre des `clearValues` correspond à l'ordre des attachements auquelles les
couleurs correspondent.

## État de profondeur et de stencil

L'attachement de profondeur est prêt à être utilisé, mais le test de profondeur n'a pas encore été activé. Il est
configuré à l'aide d'une structure de type `VkPipelineDepthStencilStateCreateInfo`.

```c++
VkPipelineDepthStencilStateCreateInfo depthStencil{};
depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
depthStencil.depthTestEnable = VK_TRUE;
depthStencil.depthWriteEnable = VK_TRUE;
```

Le champ `depthTestEnable` permet d'activer la comparaison de la profondeur des fragments. Le champ `depthWriteEnable`
autorise les fragments qui passent le test à écrire leur propre profondeur sur l'image de profondeur. Cela est utile
dans le cas des objets transparents. Ils doivent être comparés aux objets opaques, mais ne doivent pas bloquer le rendu
des autres objets derrière eux, car ils participent à leur coloration.

```c++
depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
```

Le champ `depthCompareOp` permet de fournir le test de comparaison utilisé pour conserver ou éliminer les fragments.
Nous gardons le `<` car il correspond le mieux à la convention employée par Vulkan.

```c++
depthStencil.depthBoundsTestEnable = VK_FALSE;
depthStencil.minDepthBounds = 0.0f; // Optionnel
depthStencil.maxDepthBounds = 1.0f; // Optionnel
```

Les champs `depthBoundsTestEnable`, `minDepthBounds` et `maxDepthBounds` sont utilisés pour des tests optionnels
d'encadrement de profondeur. Ils permettent de ne garder que des fragments dont la profondeur est comprise entre deux
valeurs fournies ici. Nous n'utiliserons pas cette fonctionnalité.

```c++
depthStencil.stencilTestEnable = VK_FALSE;
depthStencil.front{}; // Optionnel
depthStencil.back{}; // Optionnel
```

Les trois derniers champs configurent les opérations du buffer de stencil, que nous n'utiliserons pas non plus dans ce
tutoriel. Si vous voulez l'utiliser, vous devrez vous assurer que le format sélectionné pour la profondeur contient
aussi un composant pour le stencil.

```c++
pipelineInfo.pDepthStencilState = &depthStencil;
```

Mettez à jour la création d'une instance de `VkGraphicsPipelineCreateInfo` pour référencer l'état de profondeur et de
stencil que nous venons de créer. Un tel état doit être spécifié si la passe contient au moins l'une de ces
fonctionnalités.

Si vous lancez le programme, vous verrez que la géométrie est maintenant correctement rendue :

![](/images/depth_correct.png)

## Gestion des redimensionnements de la fenêtre

La résolution du buffer de profondeur doit changer avec la fenêtre quand elle redimensionnée, pour pouvoir correspondre
à la taille de l'attachement. Étendez `recreateSwapChain` pour régénérer les ressources :

```c++
void recreateSwapChain() {
    int width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
}
```

La libération des ressources doit avoir lieu dans la fonction de libération de la swap chain.

```c++
void cleanupSwapChain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    ...
}
```

Votre application est maintenant capable de rendre correctement de la géométrie 3D! Nous allons utiliser cette
fonctionnalité pour afficher un modèle dans le prohain chapitre.

[Code C++](/code/26_depth_buffering.cpp) /
[Vertex shader](/code/26_shader_depth.vert) /
[Fragment shader](/code/26_shader_depth.frag)
