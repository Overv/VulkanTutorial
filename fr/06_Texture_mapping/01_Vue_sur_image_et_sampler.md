Dans ce chapitre nous allons créer deux nouvelles ressources dont nous aurons besoin pour pouvoir échantillonner une
image depuis la pipeline graphique. Nous avons déjà vu la première en travaillant avec la swap chain, mais la seconde
est nouvelle, et est liée à la manière dont le shader accédera aux texels de l'image.

## Vue sur une image texture

Nous avons vu précédemment que les images ne peuvent être accédées qu'à travers une vue. Nous aurons donc besoin de
créer une vue sur notre nouvelle image texture.

Ajoutez un membre donnée pour stocker la référence à la vue de type `VkImageView`. Ajoutez ensuite la fonction 
`createTextureImageView` qui créera cette vue.

```c++
VkImageView textureImageView;

...

void initVulkan() {
    ...
    createTextureImage();
    createTextureImageView();
    createVertexBuffer();
    ...
}

...

void createTextureImageView() {

}
```

Le code de cette fonction peut être basé sur `createImageViews`. Les deux seuls changements sont dans `format` et 
`image` :

```c++
VkImageViewCreateInfo viewInfo{};
viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
viewInfo.image = textureImage;
viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
viewInfo.components = VK_COMPONENT_SWIZZLE_IDENTITY;
viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
viewInfo.subresourceRange.baseMipLevel = 0;
viewInfo.subresourceRange.levelCount = 1;
viewInfo.subresourceRange.baseArrayLayer = 0;
viewInfo.subresourceRange.layerCount = 1;
```

Appellons `vkCreateImageView` pour finaliser la création de la vue :

```c++
if (vkCreateImageView(device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
    throw std::runtime_error("échec de la création d'une vue sur l'image texture!");
}
```

Comme la logique est similaire à celle de `createImageViews`, nous ferions bien de la déplacer dans une fonction. Créez
donc `createImageView` :

```c++
VkImageView createImageView(VkImage image, VkFormat format) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("échec de la creation de la vue sur une image!");
    }

    return imageView;
}
```

Et ainsi `createTextureImageView` peut être réduite à :

```c++
void createTextureImageView() {
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
}
```

Et de même `createImageView` se résume à :

```c++
void createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat);
    }
}
```

Préparons dès maintenant la libération de la vue sur l'image à la fin du programme, juste avant la destruction de
l'image elle-même.

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyImageView(device, textureImageView, nullptr);

    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);
```

## Samplers

Il est possible pour les shaders de directement lire les texels de l'image. Ce n'est cependant pas la technique
communément utilisée. Les textures sont généralement accédées à travers un sampler (ou échantillonneur) qui filtrera
et/ou transformera les données afin de calculer la couleur la plus désirable pour le pixel.

Ces filtres sont utiles pour résoudre des problèmes tels que l'oversampling. Imaginez une texture que l'on veut mettre
sur de la géométrie possédant plus de fragments que la texture n'a de texels. Si le sampler se contentait de prendre
le pixel le plus proche, une pixellisation apparaît :

![](/images/texture_filtering.png)

En combinant les 4 texels les plus proches il est possible d'obtenir un rendu lisse comme présenté sur l'image de
droite. Bien sûr il est possible que votre application cherche plutôt à obtenir le premier résultat (Minecraft), mais
la seconde option est en général préférée. Un sampler applique alors automatiquement ce type d'opérations.

L'undersampling est le problème inverse. Cela crée des artefacts particulièrement visibles dans le cas de textures
répétées vues à un angle aigu :

![](/images/anisotropic_filtering.png)

Comme vous pouvez le voir sur l'image de droite, la texture devient d'autant plus floue que l'angle de vision se réduit.
La solution à ce problème peut aussi être réalisée par le sampler et s'appelle
[anisotropic filtering](https://en.wikipedia.org/wiki/Anisotropic_filtering). Elle est par contre plus gourmande en
ressources.

Au delà de ces filtres le sampler peut aussi s'occuper de transformations. Il évalue ce qui doit se passer quand le
fragment shader essaie d'accéder à une partie de l'image qui dépasse sa propre taille. Il se base sur le *addressing 
mode* fourni lors de sa configuration. L'image suivante présente les différentes possiblités :

![](/images/texture_addressing.png)

Nous allons maintenant créer la fonction `createTextureSampler` pour mettre en place un sampler simple. Nous
l'utiliserons pour lire les couleurs de la texture.

```c++
void initVulkan() {
    ...
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    ...
}

...

void createTextureSampler() {

}
```

Les samplers se configurent avec une structure de type `VkSamplerCreateInfo`. Elle permet d'indiquer les filtres et les
transformations à appliquer.

```c++
VkSamplerCreateInfo samplerInfo{};
samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
samplerInfo.magFilter = VK_FILTER_LINEAR;
samplerInfo.minFilter = VK_FILTER_LINEAR;
```

Les membres `magFilter` et `minFilter` indiquent comment interpoler les texels respectivement magnifiés et minifiés, ce
qui correspond respectivement aux problèmes évoqués plus haut. Nous avons choisi `VK_FILTER_LINEAR`, qui indiquent
l'utilisation des méthodes pour régler les problèmes vus plus haut.

```c++
samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
```

Le addressing mode peut être configuré pour chaque axe. Les axes disponibles sont indiqués ci-dessus ; notez
l'utilisation de U, V et W au lieu de X, Y et Z. C'est une convention dans le contexte des textures. Voilà les
différents modes possibles :

* `VK_SAMPLER_ADDRESS_MODE_REPEAT` : répète le texture
* `VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT` : répète en inversant les coordonnées pour réaliser un effet miroir
* `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE` : prend la couleur du pixel de bordure le plus proche
* `VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE` : prend la couleur de l'opposé du plus proche côté de l'image
* `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER` : utilise une couleur fixée

Le mode que nous utilisons n'est pas très important car nous ne dépasserons pas les coordonnées dans ce tutoriel.
Cependant le mode de répétition est le plus commun car il est infiniment plus efficace que d'envoyer plusieurs fois le
même carré à la pipeline, pour dessiner un pavage au sol par exemple.

```c++
samplerInfo.anisotropyEnable = VK_TRUE;
samplerInfo.maxAnisotropy = 16;
```

Ces deux membres paramètrent l'utilisation de l'anistropic filtering. Il n'y a pas vraiment de raison de ne pas
l'utiliser, sauf si vous manquez de performances. Le champ `maxAnistropy` est le nombre maximal de texels utilisés pour
calculer la couleur finale. Une plus petite valeur permet d'augmenter les performances, mais résulte évidemment en une
qualité réduite. Il n'existe à ce jour aucune carte graphique pouvant utiliser plus de 16 texels car la qualité ne
change quasiment plus.

```c++
samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
```

Le paramètre `borderColor` indique la couleur utilisée pour le sampling qui dépasse les coordonnées, si tel est le mode
choisi. Il est possible d'indiquer du noir, du blanc ou du transparent, mais vous ne pouvez pas indiquer une couleur
quelconque.

```c++
samplerInfo.unnormalizedCoordinates = VK_FALSE;
```

Le champ `unnomalizedCoordinates` indique le système de coordonnées que vous voulez utiliser pour accéder aux texels de
l'image. Avec `VK_TRUE`, vous pouvez utiliser des coordonnées dans `[0, texWidth)` et `[0, texHeight)`. Sinon, les
valeurs sont accédées avec des coordonnées dans `[0, 1)`. Dans la plupart des cas les coordonnées sont utilisées
normalisées car cela permet d'utiliser un même shader pour des textures de résolution différentes.

```c++
samplerInfo.compareEnable = VK_FALSE;
samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
```

Si une fonction de comparaison est activée, les texels seront comparés à une valeur. Le résultat de la comparaison est
ensuite utilisé pour une opération de filtrage. Cette fonctionnalité est principalement utilisée pour réaliser
[un percentage-closer filtering](https://developer.nvidia.com/gpugems/GPUGems/gpugems_chll.html) sur les shadow maps.
Nous verrons cela dans un futur chapitre.

```c++
samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
samplerInfo.mipLodBias = 0.0f;
samplerInfo.minLod = 0.0f;
samplerInfo.maxLod = 0.0f;
```

Tous ces champs sont liés au mipmapping. Nous y reviendrons dans un [prochain chapitre](/Generating_Mipmaps), mais pour
faire simple, c'est encore un autre type de filtre.

Nous avons maintenant paramétré toutes les fonctionnalités du sampler. Ajoutez un membre donnée pour stocker la
référence à ce sampler, puis créez-le avec `vkCreateSampler` :

```c++
VkImageView textureImageView;
VkSampler textureSampler;

...

void createTextureSampler() {
    ...

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("échec de la creation d'un sampler!");
    }
}
```

Remarquez que le sampler n'est pas lié à une quelconque `VkImage`. Il ne constitue qu'un objet distinct qui représente
une interface avec les images. Il peut être appliqué à n'importe quelle image 1D, 2D ou 3D. Cela diffère d'anciens APIs,
qui combinaient la texture et son filtrage.

Préparons la destruction du sampler à la fin du programme :

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);

    ...
}
```

## Capacité du device à supporter l'anistropie

Si vous lancez le programme, vous verrez que les validation layers vous envoient un message comme celui-ci :

![](/images/validation_layer_anisotropy.png)

En effet, l'anistropic filtering est une fonctionnalité du device qui doit être activée. Nous devons donc mettre à jour
la fonction `createLogicalDevice` :

```c++
VkPhysicalDeviceFeatures deviceFeatures{};
deviceFeatures.samplerAnisotropy = VK_TRUE;
```

Et bien qu'il soit très peu probable qu'une carte graphique moderne ne supporte pas cette fonctionnalité, nous devrions
aussi adapter `isDeviceSuitable` pour en être sûr.

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    ...

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}
```

La structure `VkPhysicalDeviceFeatures` permet d'indiquer les capacités supportées quand elle est utilisée avec la
fonction `VkPhysicalDeviceFeatures`, plutôt que de fournir ce dont nous avons besoin.

Au lieu de simplement obliger le client à posséder une carte graphique supportant l'anistropic filtering, nous pourrions
conditionnellement activer ou pas l'anistropic filtering :

```c++
samplerInfo.anisotropyEnable = VK_FALSE;
samplerInfo.maxAnisotropy = 1;
```

Dans le prochain chapitre nous exposerons l'image et le sampler au fragment shader pour qu'il puisse utiliser la
texture sur le carré.

[C++ code](/code/24_sampler.cpp) /
[Vertex shader](/code/21_shader_ubo.vert) /
[Fragment shader](/code/21_shader_ubo.frag)
