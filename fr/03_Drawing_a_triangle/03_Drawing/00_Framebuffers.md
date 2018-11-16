Nous avons beaucoup parlé de framebuffers dans les chapitres précédents, et nous avons paramétré la render pass 
pour qu'elle en accepte un unique framebuffer du même format que les images de la swap chain, mais nous n'en avons pour
l'instant créé aucun.

Les attachements spécifiés durant la render pass sont liés en les considérant dans des objets de type
`VkFramebuffer`. Un tel objet référence toutes les `VkImageView` utilisées comme attachements par une passe. Dans notre
cas nous n'en aurons qu'un : un attachement de couleur. Cependant l'image utilisée dépendra de l'image fournie par la
swap chain lors de la requète pour l'affichage. Cela signifie que nous devons créer un framebuffer pour chacune des
images de la swap chain et utiliser celui qui correspond au moment de l'affichage.

Pour cela créez un autre `std::vector` qui contiendra un framebuffer :

```c++
std::vector<VkFramebuffer> swapChainFramebuffers;
```

Nous allons remplir cette liste depuis une nouvelle fonction `createFramebuffers` que nous appellerons depuis
`initVulkan` juste après la création de la pipeline graphique :

```c++
void initVulkan() {
    createInstance();
    setupDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
}

...

void createFramebuffers() {

}
```

Commencez par redimensionner le conteneur afin qu'il stocke tous les framebuffers :

```c++
void createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());
}
```

Nous allons maintenant itérer à travers toutes les images et créer un framebuffer à partir de chacune d'entre elles :

```c++
for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    VkImageView attachments[] = {
        swapChainImageViews[i]
    };

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
        throw std::runtime_error("échec lors de la création d'un framebuffer!");
    }
}
```

Comme vous le pouvez le voir la création d'un framebuffer est assez simple. Nous devons d'abord indiquer avec quelle
`renderPass` le framebuffer doit être compatible. Vous ne pouvez utiliser un framebuffer qu'avec des render passes
compatibles, où compatible signifie approximativement que les render passes utilisent le même nombre et les mêmes
types d'attachements.

Les paramètres `attachementCount` et `pAttachments` doivent donner la taille du tableau contenant les `VkImageViews`
qui servent d'attachements.

Les paramètres `width` et `height` sont évidents. Le membre `layers` réfère au nombres de couches dans les images
fournies comme attachements. Les images de la swap chain n'ont qu'une seule couche donc nous indiquons `1`.

Nous devons détruire les framebuffers avant les image views et la render pass dans la fonction `cleanup` :

```c++
void cleanup() {
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    ...
}
```

Nous avons atteint le moment où tous les objets sont prêts pour l'affichage. Dans le prochain chapitre nous allons
écrire les commandes d'affichage.

[Code C++](/code/13_framebuffers.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
