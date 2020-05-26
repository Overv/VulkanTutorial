Nous avons beaucoup parlé de framebuffers dans les chapitres précédents, et nous avons mis en place la render pass
pour qu'elle en accepte un du même format que les images de la swap chain. Pourtant nous n'en avons encore créé aucun.

Les attachements de différents types spécifiés durant la render pass sont liés en les considérant dans des objets de
type `VkFramebuffer`. Un tel objet référence toutes les `VkImageView` utilisées comme attachements par une passe.
Dans notre cas nous n'en aurons qu'un : un attachement de couleur, qui servira de cible d'affichage uniquement.
Cependant l'image utilisée dépendra de l'image fournie par la swap chain lors de la requête pour l'affichage. Nous
devons donc créer un framebuffer pour chacune des images de la swap chain et utiliser le bon au moment de l'affichage.

Pour cela créez un autre `std::vector` qui contiendra des framebuffers :

```c++
std::vector<VkFramebuffer> swapChainFramebuffers;
```

Nous allons remplir ce `vector` depuis une nouvelle fonction `createFramebuffers` que nous appellerons depuis 
`initVulkan` juste après la création de la pipeline graphique :

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
}

...

void createFramebuffers() {

}
```

Commencez par redimensionner le conteneur afin qu'il puisse stocker tous les framebuffers :

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

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
        throw std::runtime_error("échec de la création d'un framebuffer!");
    }
}
```

Comme vous le pouvez le voir la création d'un framebuffer est assez simple. Nous devons d'abord indiquer avec quelle
`renderPass` le framebuffer doit être compatible. Sachez que si vous voulez utiliser un framebuffer avec plusieurs
render passes, les render passes spécifiées doivent être compatibles entre elles. La compatibilité signifie ici
approximativement qu'elles utilisent le même nombre d'attachements du même type. Ceci implique qu'il ne faut pas
s'attendre à ce qu'une render pass puisse ignorer certains attachements d'un framebuffer qui en aurait trop.

Les paramètres `attachementCount` et `pAttachments` doivent donner la taille du tableau contenant les `VkImageViews`
qui servent d'attachements.

Les paramètres `width` et `height` sont évidents. Le membre `layers` correspond au nombres de couches dans les images
fournies comme attachements. Les images de la swap chain n'ont toujours qu'une seule couche donc nous indiquons `1`.

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
