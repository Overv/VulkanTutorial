## Préparation

Avant de finaliser la création de la pipeline nous devons informer Vulkan des attachements des framebuffers utilisés 
lors du rendu. Nous devons indiquer combien chaque framebuffer aura de buffers de couleur et de profondeur, combien de 
samples il faudra utiliser avec chaque framebuffer et comment les utiliser tout au long des opérations de rendu. Toutes
ces informations sont contenues dans un objet appelé *render pass*. Pour le configurer, créons la fonction 
`createRenderPass`. Appelez cette fonction depuis `initVulkan` après `createGraphicsPipeline`.

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
}

...

void createRenderPass() {

}
```

## Description de l'attachement

Dans notre cas nous aurons un seul attachement de couleur, et c'est une image de la swap chain.

```c++
void createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
}
```

Le `format` de l'attachement de couleur est le même que le format de l'image de la swap chain. Nous n'utilisons pas 
de multisampling pour le moment donc nous devons indiquer que nous n'utilisons qu'un seul sample.

```c++
colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
```

Les membres `loadOp` et `storeOp` définissent ce qui doit être fait avec les données de l'attachement respectivement
avant et après le rendu. Pour `loadOp` nous avons les choix suivants :

* `VK_ATTACHMENT_LOAD_OP_LOAD` : conserve les données présentes dans l'attachement
* `VK_ATTACHMENT_LOAD_OP_CLEAR` : remplace le contenu par une constante
* `VK_ATTACHMENT_LOAD_OP_DONT_CARE` : ce qui existe n'est pas défini et ne nous intéresse pas

Dans notre cas nous utiliserons l'opération de remplacement pour obtenir un framebuffer noir avant d'afficher une 
nouvelle image. Il n'y a que deux possibilités pour le membre `storeOp` :

* `VK_ATTACHMENT_STORE_OP_STORE` : le rendu est gardé en mémoire et accessible plus tard
* `VK_ATTACHMENT_STORE_OP_DONT_CARE` : le contenu du framebuffer est indéfini dès la fin du rendu

Nous voulons voir le triangle à l'écran donc nous voulons l'opération de stockage.

```c++
colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
```

Les membres `loadOp` et `storeOp` s'appliquent aux données de couleur et de profondeur, et `stencilLoadOp` et 
`stencilStoreOp` s'appliquent aux données de stencil. Notre application n'utilisant pas de stencil buffer, nous 
pouvons indiquer que les données ne nous intéressent pas.

```c++
colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
```

Les textures et les framebuffers dans Vulkan sont représentés par des objets de type `VkImage` possédant un certain 
format de pixels. Cependant l'organisation des pixels dans la mémoire peut changer selon ce que vous faites de cette 
image.

Les organisations les plus communes sont :

* `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL` : images utilisées comme attachements de couleur
* `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` : images présentées à une swap chain
* `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` : image utilisées comme destination d'opérations de copie de mémoire

Nous discuterons plus précisément de ce sujet dans le chapitre sur les textures. Ce qui compte pour le moment est que
les images doivent changer d'organisation mémoire selon les opérations qui leur sont appliquées au long de l'exécution
de la pipeline.

Le membre `initialLayout` spécifie l'organisation de l'image avant le début du rendu. Le membre `finalLayout` fournit
l'organisation vers laquelle l'image doit transitionner à la fin du rendu. La valeur `VK_IMAGE_LAYOUT_UNDEFINED` 
indique que le format précédent de l'image ne nous intéresse pas, ce qui peut faire perdre les données précédentes. 
Mais ce n'est pas un problème puisque nous effaçons de toute façon toutes les données avant le rendu. Puis, afin de 
rendre l'image compatible avec la swap chain, nous fournissons `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` pour `finalLayout`.

## Subpasses et références aux attachements

Une unique passe de rendu est composée de plusieurs subpasses. Les subpasses sont des opérations de rendu
dépendant du contenu présent dans le framebuffer quand elles commencent. Elles peuvent consister en des opérations de
post-processing exécutées l'une après l'autre. En regroupant toutes ces opérations en une seule passe, Vulkan peut
alors réaliser des optimisations et conserver de la bande passante pour de potentiellement meilleures performances.
Pour notre triangle nous nous contenterons d'une seule subpasse.

Chacune d'entre elle référence un ou plusieurs attachements décrits par les structures que nous avons vues 
précédemment. Ces références sont elles-mêmes des structures du type `VkAttachmentReference` et ressemblent à cela :

```c++
VkAttachmentReference colorAttachmentRef{};
colorAttachmentRef.attachment = 0;
colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
```

Le paramètre `attachment` spécifie l'attachement à référencer à l'aide d'un indice correspondant à la position de la 
structure dans le tableau de descriptions d'attachements. Notre tableau ne consistera qu'en une seule référence donc 
son indice est nécessairement `0`. Le membre `layout` donne l'organisation que l'attachement devrait avoir au début d'une
subpasse utilsant cette référence. Vulkan changera automatiquement l'organisation de l'attachement quand la subpasse 
commence. Nous voulons que l'attachement soit un color buffer, et pour cela la meilleure performance sera obtenue avec
`VK_IMAGE_LAYOUT_COLOR_OPTIMAL`, comme son nom le suggère.

La subpasse est décrite dans la structure `VkSubpassDescription` :

```c++
VkSubpassDescription subpass{};
subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
```

Vulkan supportera également des *compute subpasses* donc nous devons indiquer que celle que nous créons est destinée 
aux graphismes. Nous spécifions ensuite la référence à l'attachement de couleurs :

```c++
subpass.colorAttachmentCount = 1;
subpass.pColorAttachments = &colorAttachmentRef;
```

L'indice de cet attachement est indiqué dans le fragment shader avec le `location = 0` dans la directive 
`layout(location = 0) out vec4 outColor`.

Les types d'attachements suivants peuvent être indiqués dans une subpasse :

* `pInputAttachments` : attachements lus depuis un shader
* `pResolveAttachments` : attachements utilisés pour le multisampling d'attachements de couleurs
* `pDepthStencilAttachment` : attachements pour la profondeur et le stencil
* `pPreserveAttachments` : attachements qui ne sont pas utilisés par cette subpasse mais dont les données doivent 
être conservées

## Passe de rendu

Maintenant que les attachements et une subpasse simple ont été décrits nous pouvons enfin créer la render pass. 
Créez une nouvelle variable du type `VkRenderPass` au-dessus de la variable `pipelineLayout` :

```c++
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
```

L'objet représentant la render pass peut alors être créé en remplissant la structure `VkRenderPassCreateInfo` dans
laquelle nous devons remplir un tableau d'attachements et de subpasses. Les objets `VkAttachmentReference` référencent
les attachements en utilisant les indices de ce tableau.

```c++
VkRenderPassCreateInfo renderPassInfo{};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
renderPassInfo.attachmentCount = 1;
renderPassInfo.pAttachments = &colorAttachment;
renderPassInfo.subpassCount = 1;
renderPassInfo.pSubpasses = &subpass;

if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("échec de la création de la render pass!");
}
```

Comme l'organisation de la pipeline, nous aurons à utiliser la référence à la passe de rendu tout au long du 
programme. Nous devons donc la détruire dans la fonction `cleanup` :

```c++
void cleanup() {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    ...
}
```

Nous avons eu beaucoup de travail, mais nous allons enfin créer la pipeline graphique et l'utiliser dès le prochain 
chapitre!

[Code C++](/code/11_render_passes.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
