## Introduction

Notre programme peut maintenant charger et afficher des modèles 3D. Dans ce chapitre nous allons ajouter une nouvelle
fonctionnalité : celle de générer et d'utiliser des mipmaps. Elles sont utilisées dans tous les applications 3D. Vulkan
laisse au programmeur un control quasiment total sur leur génération.

Les mipmaps sont des versions de qualité réduite précalculées d'une texture. Chacune de ces versions est deux fois
moins haute et large que l'originale. Les objets plus distants de la caméra peuvent utiliser ces versions pour le
sampling de la texture. Le rendu est alors plus rapide et plus lisse. Voici un exemple de mipmaps :

![](/images/mipmaps_example.jpg)

## Création des images

Avec Vulkan, chaque niveau de mipmap est stocké dans les différents *niveaux de mipmap* de l'image originale. Le niveau
0 correspond à l'image originale. Les images suivantes sont souvent appelées *mip chain*.

Le nombre de niveaux de mipmap doit être fourni lors de la création de l'image. Jusqu'à présent nous avons indiqué la
valeur `1`. Nous devons ainsi calculer le nombre de mipmaps à générer à partir de la taille de l'image. Créez un membre
donnée pour contenir cette valeur :

```c++
...
uint32_t mipLevels;
VkImage textureImage;
...
```

La valeur pour `mipLevels` peut être déterminée une fois que nous avons chargé la texture dans `createTextureImage` :

```c++
int texWidth, texHeight, texChannels;
stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
...
mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

```

La troisième ligne ci-dessus calcule le nombre de niveaux de mipmaps. La fonction `max` chosit la plus grande des
dimensions, bien que dans la pratique les textures seront toujours carrées. Ensuite, `log2` donne le nombre de fois que
les dimensions peuvent être divisées par deux. La fonction `floor` gère le cas où la dimension n'est pas un multiple
de deux (ce qui est déconseillé). `1` est finalement rajouté pour que l'image originale soit aussi comptée.

Pour utiliser cette valeur nous devons changer les fonctions `createImage`, `createImageView` et 
`transitionImageLayout`. Nous devrons y indiquer le nombre de mipmaps. Ajoutez donc cette donnée en paramètre à toutes
ces fonctions :

```c++
void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    ...
    imageInfo.mipLevels = mipLevels;
    ...
}
```

```c++
VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
    ...
    viewInfo.subresourceRange.levelCount = mipLevels;
    ...
```

```c++
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
    ...
    barrier.subresourceRange.levelCount = mipLevels;
    ...
```

Il nous faut aussi mettre à jour les appels.

```c++
createImage(swapChainExtent.width, swapChainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
...
createImage(texWidth, texHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
```

```c++
swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
...
depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
...
textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
```

```c++
transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
...
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
```

## Génération des mipmaps

Notre texture a plusieurs niveaux de mipmaps, mais le buffer intermédiaire ne peut pas gérer cela. Les niveaux
autres que 0 sont indéfinis. Pour les remplir nous devons générer les mipmaps à partir du seul niveau que nous avons.
Nous allons faire cela du côté de la carte graphique. Nous allons pour cela utiliser la commande `vkCmdBlitImage`.
Elle effectue une copie, une mise à l'échelle et un filtrage. Nous allons l'appeler une fois par niveau.

Cette commande est considérée comme une opération de transfert. Nous devons donc indiquer que la mémoire de l'image sera
utilisée à la fois comme source et comme destination de la commande. Ajoutez `VK_IMAGE_USAGE_TRANSFER_SRC_BIT` à la
création de l'image.

```c++
...
createImage(texWidth, texHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
...
```

Comme pour les autres opérations sur les images, la commande `vkCmdBlitImage` dépend de l'organisation de l'image sur
laquelle elle opère. Nous pourrions transitionner l'image vers `VK_IMAGE_LAYOUT_GENERAL`, mais les opérations 
prendraient beaucoup de temps. En fait il est possible de transitionner les niveaux de mipmaps indépendemment les uns
des autres. Nous pouvons donc mettre l'image initiale à `VK_IMAGE_LAYOUT_TRANSFER_SCR_OPTIMAL` et la chaîne de mipmaps
à `VK_IMAGE_LAYOUT_DST_OPTIMAL`. Nous pourrons réaliser les transitions à la fin de chaque opération.

La fonction `transitionImageLayout` ne peut réaliser une transition d'organisation que sur l'image entière. Nous allons
donc devoir écrire quelque commandes liées aux barrières de pipeline. Supprimez la transition vers 
`VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` dans `createTextureImage` :

```c++
...
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
//transitionné vers VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL lors de la generation des mipmaps
...
```

Tous les niveaux de l'image seront ainsi en `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`. Chaque niveau sera ensuite
transitionné vers `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` après l'exécution de la commande.

Nous allons maintenant écrire la fonction qui génèrera les mipmaps.

```c++
void generateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    
    endSingleTimeCommands(commandBuffer);
}
```

Nous allons réaliser plusieurs transitions, et pour cela nous réutiliserons cette structure `VkImageMemoryBarrier`. Les
champs remplis ci-dessus seront valides pour tous les niveaux, et nous allons changer les champs manquant au fur et à
mesure de la génération des mipmaps.

```c++
int32_t mipWidth = texWidth;
int32_t mipHeight = texHeight;

for (uint32_t i = 1; i < mipLevels; i++) {

}
```

Cette boucle va enregistrer toutes les commandes `VkCmdBlitImage`. Remarquez que la boucle commence à 1, et pas à 0.

```c++
barrier.subresourceRange.baseMipLevel = i - 1;
barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

vkCmdPipelineBarrier(commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
    0, nullptr,
    0, nullptr,
    1, &barrier);
```

Tout d'abord nous transitionnons le `i-1`ième niveau vers `VK_IMAGE_LAYOUT_TRANSFER_SCR_OPTIMAL`. Cette transition
attendra que le niveau de mipmap soit prêt, que ce soit par copie depuis le buffer pour l'image originale, ou bien par 
`vkCmdBlitImage`. La commande de génération de la mipmap suivante attendra donc la fin de la précédente.

```c++
VkImageBlit blit{};
blit.srcOffsets[0] = { 0, 0, 0 };
blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
blit.srcSubresource.mipLevel = i - 1;
blit.srcSubresource.baseArrayLayer = 0;
blit.srcSubresource.layerCount = 1;
blit.dstOffsets[0] = { 0, 0, 0 };
blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
blit.dstSubresource.mipLevel = i;
blit.dstSubresource.baseArrayLayer = 0;
blit.dstSubresource.layerCount = 1;
```

Nous devons maintenant indiquer les régions concernées par la commande. Le niveau de mipmap source est `i-1` et le
niveau destination est `i`. Les deux éléments du tableau `scrOffsets` déterminent en 3D la région source, et 
`dstOffsets` la région cible. Les coordonnées X et Y sont à chaque fois divisées par deux pour réduire la taille des
mipmaps. La coordonnée Z doit être mise à la profondeur de l'image, c'est à dire 1.

```c++
vkCmdBlitImage(commandBuffer,
    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1, &blit,
    VK_FILTER_LINEAR);
```

Nous enregistrons maintenant les commandes. Remarquez que `textureImage` est utilisé à la fois comme source et comme
cible, car la commande s'applique à plusieurs niveaux de l'image. Le niveau de mipmap source vient d'être transitionné
vers `VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL`, et le niveau cible est resté en destination depuis sa création.

Le dernier paramètre permet de fournir un `VkFilter`. Nous voulons le même filtre que pour le sampler, nous pouvons donc
mettre `VK_FILTER_LINEAR`.

```c++
barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

vkCmdPipelineBarrier(commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
    0, nullptr,
    0, nullptr,
    1, &barrier);
```

Ensuite, la boucle transtionne le `i-1`ième niveau de mipmap vers l'organisation optimale pour la lecture par shader.
La transition attendra la fin de la commande, de même que les opérations de sampling.

```c++
    ...
    if (mipWidth > 1) mipWidth /= 2;
    if (mipHeight > 1) mipHeight /= 2;
}
```

Les tailles de la mipmap sont ensuite divisées par deux. Nous vérifions quand même que ces dimensions sont bien
supérieures à 1, ce qui peut arriver dans le cas d'une image qui n'est pas carrée.

```c++
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    endSingleTimeCommands(commandBuffer);
}
```

Avant de terminer avec le command buffer, nous devons ajouter une dernière barrière. Elle transitionne le dernier
niveau de mipmap vers `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`. Ce cas n'avait pas été géré par la boucle, car elle
n'a jamais servie de source à une copie.

Appelez finalement cette fonction depuis `createTextureImage` :

```c++
transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
//transions vers VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL pendant la génération des mipmaps
...
generateMipmaps(textureImage, texWidth, texHeight, mipLevels);
```

Les mipmaps de notre image sont maintenant complètement remplies.

## Support pour le filtrage linéaire

La fonction `vkCmdBlitImage` est extrêmement pratique. Malheureusement il n'est pas garanti qu'elle soit disponible. Elle
nécessite que le format de l'image texture supporte ce type de filtrage, ce que nous pouvons vérifier avec la fonction 
`vkGetPhysicalDeviceFormatProperties`. Nous allons vérifier sa disponibilité dans `generateMipmaps`.

Ajoutez d'abord un paramètre qui indique le format de l'image :

```c++
void createTextureImage() {
    ...

    generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
}

void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {

    ...
}
```

Utilisez `vkGetPhysicalDeviceFormatProperties` dans `generateMipmaps` pour récupérer les propriétés liés au format :

```c++
void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {

    // Vérifions si l'image supporte le filtrage linéaire
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    ...
```

La structure `VkFormatProperties` possède les trois champs `linearTilingFeatures`, `optimalTilingFeature` et 
`bufferFeaetures`. Ils décrivent chacun l'utilisation possible d'images de ce format dans certains contextes. Nous avons
créé l'image avec le format optimal, les informations qui nous concernent sont donc dans `optimalTilingFeatures`. Le
support pour le filtrage linéaire est ensuite indiqué par `VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT`.

```c++
if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    throw std::runtime_error("le format de l'image texture ne supporte pas le filtrage lineaire!");
}
```

Il y a deux alternatives si le format ne permet pas l'utilisation de `vkCmdBlitImage`. Vous pouvez créer une fonction
pour essayer de trouver un format supportant la commande, ou vous pouvez utiliser une librairie pour générer les
mipmaps comme [stb_image_resize](https://github.com/nothings/stb/blob/master/stb_image_resize.h). Chaque niveau de
mipmap peut ensuite être chargé de la même manière que vous avez chargé l'image.

Souvenez-vous qu'il est rare de générer les mipmaps pendant l'exécution. Elles sont généralement prégénérées et stockées
dans le fichier avec l'image de base. Le chargement de mipmaps prégénérées est laissé comme exercice au lecteur.

## Sampler

Un objet `VkImage` contient les données de l'image et un objet `VkSampler` contrôle la lecture des données pendant le
rendu. Vulkan nous permet de spécifier les valeurs `minLod`, `maxLod`, `mipLodBias` et `mipmapMode`, où "Lod" signifie 
*level of detail* (*niveau de détail*). Pendant l’échantillonnage d'une texture, le sampler sélectionne le niveau de
mipmap à utiliser suivant ce pseudo-code :

```c++
lod = getLodLevelFromScreenSize(); //plus petit quand l'objet est proche, peut être negatif
lod = clamp(lod + mipLodBias, minLod, maxLod);

level = clamp(floor(lod), 0, texture.mipLevels - 1);  //limité par le nombre de niveaux de mipmaps dans le texture

if (mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST) {
    color = sample(level);
} else {
    color = blend(sample(level), sample(level + 1));
}
```

Si `samplerInfo.mipmapMode` est `VK_SAMPLER_MIPMAP_MODE_NEAREST`, la variable `lod` correspond au niveau de mipmap à
échantillonner. Sinon, si il vaut `VK_SAMPLER_MIPMAP_MODE_LINEAR`, deux niveaux de mipmaps sont samplés, puis interpolés
linéairement.

L'opération d'échantillonnage est aussi affectée par `lod` :

```c++
if (lod <= 0) {
    color = readTexture(uv, magFilter);
} else {
    color = readTexture(uv, minFilter);
}
```

Si l'objet est proche de la caméra, `magFilter` est utilisé comme filtre. Si l'objet est plus distant, `minFilter` sera
utilisé. Normalement `lod` est positif, est devient nul au niveau de la caméra. `mipLodBias` permet de forcer Vulkan à
utiliser un `lod` plus petit et donc un noveau de mipmap plus élevé.

Pour voir les résultats de ce chapitre, nous devons choisir les valeurs pour `textureSampler`. Nous avons déjà fourni 
`minFilter` et `magFilter`. Il nous reste les valeurs `minLod`, `maxLod`, `mipLodBias` et `mipmapMode`.

```c++
void createTextureSampler() {
    ...
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0;
    samplerInfo.maxLod = static_cast<float>(mipLevels);
    samplerInfo.mipLodBias = 0; // Optionnel
    ...
}
```

Pour utiliser la totalité des niveaux de mipmaps, nous mettons `minLod` à `0` et `maxLod` au nombre de niveaux de
mipmaps. Nous n'avons aucune raison d'altérer `lod` avec `mipLodBias`, alors nous pouvons le mettre à `0`.

Lancez votre programme et vous devriez voir ceci :

![](/images/mipmaps.png)

Notre scène est si simple qu'il n'y a pas de différence majeure. En comparant précisement on peut voir quelques
différences.

![](/images/mipmaps_comparison.png)

La différence la plus évidente est le texte sur le paneau, plus lisse avec les mipmaps.

Vous pouvez modifier les paramètres du sampler pour voir l'impact sur le rendu. Par exemple vous pouvez empêcher le
sampler d'utiliser le plus haut nivau de mipmap en ne lui indiquant pas le niveau le plus bas :

```c++
samplerInfo.minLod = static_cast<float>(mipLevels / 2);
```

Ce paramètre produira ce rendu :

![](/images/highmipmaps.png)

[Code C++](/code/28_mipmapping.cpp) /
[Vertex shader](/code/26_shader_depth.vert) /
[Fragment shader](/code/26_shader_depth.frag)