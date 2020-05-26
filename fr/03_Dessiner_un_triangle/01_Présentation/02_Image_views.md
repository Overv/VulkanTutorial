Quelque soit la `VkImage` que nous voulons utiliser, dont celles de la swap chain, nous devons en créer une 
`VkImageView` pour la manipuler. Cette image view correspond assez litéralement à une vue dans l'image. Elle décrit 
l'accès à l'image et les parties de l'image à accéder. Par exemple elle indique si elle doit être traitée comme une 
texture 2D pour la profondeur sans aucun niveau de mipmapping.

Dans ce chapitre nous écrirons une fonction `createImageViews` pour créer une image view basique pour chacune des 
images dans la swap chain, pour que nous puissions les utiliser comme cibles de couleur.

Ajoutez d'abord un membre donnée pour y stocker une image view :

```c++
std::vector<VkImageView> swapChainImageViews;
```

Créez la fonction `createImageViews` et appelez-la juste après la création de la swap chain.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
}

void createImageViews() {

}
```

Nous devons d'abord redimensionner la liste pour pouvoir y mettre toutes les image views que nous créerons :

```c++
void createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

}
```

Créez ensuite la boucle qui parcourra toutes les images de la swap chain.

```c++
for (size_t i = 0; i < swapChainImages.size(); i++) {

}
```

Les paramètres pour la création d'image views se spécifient dans la structure `VkImageViewCreateInfo`. Les deux 
premiers paramètres sont assez simples :

```c++
VkImageViewCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
createInfo.image = swapChainImages[i];
```

Les champs `viewType` et `format` indiquent la manière dont les images doivent être interprétées. Le paramètre 
`viewType` permet de traiter les images comme des textures 1D, 2D, 3D ou cube map.

```c++
createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
createInfo.format = swapChainImageFormat;
```

Le champ `components` vous permet d'altérer les canaux de couleur. Par exemple, vous pouvez envoyer tous les 
canaux au canal rouge pour obtenir une texture monochrome. Vous pouvez aussi donner les valeurs constantes `0` ou `1`
à un canal. Dans notre cas nous garderons les paramètres par défaut.

```c++
createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
```

Le champ `subresourceRange` décrit l'utilisation de l'image et indique quelles parties de l'image devraient être 
accédées. Notre image sera utilisée comme cible de couleur et n'aura ni mipmapping ni plusieurs couches.

```c++
createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
createInfo.subresourceRange.baseMipLevel = 0;
createInfo.subresourceRange.levelCount = 1;
createInfo.subresourceRange.baseArrayLayer = 0;
createInfo.subresourceRange.layerCount = 1;
```

Si vous travailliez sur une application 3D stéréoscopique, vous devrez alors créer une swap chain avec plusieurs 
couches. Vous pourriez alors créer plusieurs image views pour chaque image. Elles représenteront ce qui sera affiché
pour l'œil gauche et pour l'œil droit.

Créer l'image view ne se résume plus qu'à appeler `vkCreateImageView` :

```c++
if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
    throw std::runtime_error("échec de la création d'une image view!");
}
```

À la différence des images, nous avons créé les image views explicitement et devons donc les détruire de la même 
manière, ce que nous faisons à l'aide d'une boucle :

```c++
void cleanup() {
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    ...
}
```

Une image view est suffisante pour commencer à utiliser une image comme une texture, mais pas pour que l'image soit 
utilisée comme cible d'affichage. Pour cela nous avons encore une étape, appelée framebuffer. Mais nous devons 
d'abord mettre en place le pipeline graphique.

[Code C++](/code/07_image_views.cpp)
