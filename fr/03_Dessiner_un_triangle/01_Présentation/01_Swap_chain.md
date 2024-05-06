Vulkan ne possède pas de concept comme le framebuffer par défaut, et nous devons donc créer une infrastructure qui
contiendra les buffers sur lesquels nous effectuerons les rendus avant de les présenter à l'écran. Cette
infrastructure s'appelle _swap chain_ sur Vulkan et doit être créée explicitement. La swap chain est essentiellement
une file d'attente d'images attendant d'être affichées. Notre application devra récupérer une des images de la file,
dessiner dessus puis la retourner à la file d'attente. Le fonctionnement de la file d'attente et les conditions de la
présentation dépendent du paramétrage de la swap chain. Cependant, l'intérêt principal de la swap chain est de
synchroniser la présentation avec le rafraîchissement de l'écran.

## Vérification du support de la swap chain

Toutes les cartes graphiques ne sont pas capables de présenter directement les images à l'écran, et ce pour
différentes raisons. Ce pourrait être car elles sont destinées à être utilisées dans un serveur et n'ont aucune
sortie vidéo. De plus, dans la mesure où la présentation est très dépendante du gestionnaire de fenêtres et de la
surface, la swap chain ne fait pas partie de Vulkan "core". Il faudra donc utiliser des extensions, dont
`VK_KHR_swapchain`.

Pour cela nous allons modifier `isDeviceSuitable` pour qu'elle vérifie si cette extension est supportée. Nous avons
déjà vu comment lister les extensions supportées par un `VkPhysicalDevice` donc cette modification devrait être assez
simple. Notez que le header Vulkan intègre la macro `VK_KHR_SWAPCHAIN_EXTENSION_NAME` qui permet d'éviter une faute
de frappe. Toutes les extensions ont leur macro.

Déclarez d'abord une liste d'extensions nécessaires au physical device, comme nous l'avons fait pour les validation
layers :

```c++
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
```

Créez ensuite une nouvelle fonction appelée `checkDeviceExtensionSupport` et appelez-la depuis `isDeviceSuitable`
comme vérification supplémentaire :

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    return indices.isComplete() && extensionsSupported;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    return true;
}
```

Énumérez les extensions et vérifiez si toutes les extensions requises en font partie.

```c++
bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}
```

J'ai décidé d'utiliser une collection de strings pour représenter les extensions requises en attente de confirmation.
Nous pouvons ainsi facilement les éliminer en énumérant la séquence. Vous pouvez également utiliser des boucles
imbriquées comme dans `checkValidationLayerSupport`, car la perte en performance n'est pas capitale dans cette phase de
chargement. Lancez le code et vérifiez que votre carte graphique est capable de gérer une swap chain. Normalement
la disponibilité de la queue de présentation implique que l'extension de la swap chain est supportée. Mais soyons
tout de mêmes explicites pour cela aussi.

## Activation des extensions du device

L'utilisation de la swap chain nécessite l'extension `VK_KHR_swapchain`. Son activation ne requiert qu'un léger
changement à la structure de création du logical device :

```c++
createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
createInfo.ppEnabledExtensionNames = deviceExtensions.data();
```

Supprimez bien l'ancienne ligne `createInfo.enabledExtensionCount = 0;`.

## Récupérer des détails à propos du support de la swap chain

Vérifier que la swap chain est disponible n'est pas suffisant. Nous devons vérifier qu'elle est compatible avec notre
surface de fenêtre. La création de la swap chain nécessite un nombre important de paramètres, et nous devons récupérer
encore d'autres détails pour pouvoir continuer.

Il y a trois types de propriétés que nous devrons vérifier :

* Possibilités basiques de la surface (nombre min/max d'images dans la swap chain, hauteur/largeur min/max des images)
* Format de la surface (format des pixels, palette de couleur)
* Mode de présentation disponibles

Nous utiliserons une structure comme celle dans `findQueueFamilies` pour contenir ces détails une fois qu'ils auront
été récupérés. Les trois catégories mentionnées plus haut se présentent sous la forme de la structure et des listes de
structures suivantes :

```c++
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
```

Créons maintenant une nouvelle fonction `querySwapChainSupport` qui remplira cette structure :

```c++
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    return details;
}
```

Cette section couvre la récupération des structures. Ce qu'elles signifient sera expliqué dans la section suivante.

Commençons par les capacités basiques de la texture. Il suffit de demander ces informations et elles nous seront
fournies sous la forme d'une structure du type `VkSurfaceCapabilitiesKHR`.

```c++
vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
```

Cette fonction requiert que le physical device et la surface de fenêtre soient passées en paramètres, car elle en
extrait ces capacités. Toutes les fonctions récupérant des capacités de la swap chain demanderont ces paramètres,
car ils en sont les composants centraux.

La prochaine étape est de récupérer les formats de texture supportés. Comme c'est une liste de structure, cette
acquisition suit le rituel des deux étapes :

```c++
uint32_t formatCount;
vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
}
```

Finalement, récupérer les modes de présentation supportés suit le même principe et utilise
`vkGetPhysicalDeviceSurfacePresentModesKHR` :

```c++
uint32_t presentModeCount;
vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
}
```

Tous les détails sont dans des structures, donc étendons `isDeviceSuitable` une fois de plus et utilisons cette
fonction pour vérifier que le support de la swap chain nous correspond. Nous ne demanderons que des choses très
simples dans ce tutoriel.

```c++
bool swapChainAdequate = false;
if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
}
```

Il est important de ne vérifier le support de la swap chain qu'après s'être assuré que l'extension est disponible. La
dernière ligne de la fonction devient donc :

```c++
return indices.isComplete() && extensionsSupported && swapChainAdequate;
```

## Choisir les bons paramètres pour la swap chain

Si la fonction `swapChainAdequate` retourne `true` le support de la swap chain est assuré. Il existe cependant encore
plusieurs modes ayant chacun leur intérêt. Nous allons maintenant écrire quelques fonctions qui détermineront les bons
paramètres pour obtenir la swap chain la plus efficace possible. Il y a trois types de paramètres à déterminer :

* Format de la surface (profondeur de la couleur)
* Modes de présentation (conditions de "l'échange" des images avec l'écran)
* Swap extent (résolution des images dans la swap chain)

Pour chacun de ces paramètres nous aurons une valeur idéale que nous choisirons si elle est disponible, sinon nous
nous rabattrons sur ce qui nous restera de mieux.

### Format de la surface

La fonction utilisée pour déterminer ce paramètre commence ainsi. Nous lui passerons en argument le membre donnée
`formats` de la structure `SwapChainSupportDetails`.

```c++
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

}
```

Chaque `VkSurfaceFormatKHR` contient les données `format` et `colorSpace`. Le `format` indique les canaux de couleur
disponibles et les types qui contiennent les valeurs des gradients. Par exemple `VK_FORMAT_B8G8R8A8_SRGB` signifie que
nous stockons les canaux de couleur R, G, B et A dans cet ordre et en entiers non signés de 8 bits. `colorSpace` permet
de vérifier que le sRGB est supporté en utilisant le champ de bits `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`.

Pour l'espace de couleur nous utiliserons sRGB si possible, car il en résulte
[un rendu plus réaliste](http://stackoverflow.com/questions/12524623/). Le format le plus commun est
`VK_FORMAT_B8G8R8A8_SRGB`.

Itérons dans la liste et voyons si le meilleur est disponible :

```c++
for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return availableFormat;
    }
}
```

Si cette approche échoue aussi nous pourrions trier les combinaisons disponibles, mais pour rester simple nous
prendrons le premier format disponible.

```c++
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}
```

### Mode de présentation

Le mode de présentation est clairement le paramètre le plus important pour la swap chain, car il touche aux
conditions d'affichage des images à l'écran. Il existe quatre modes avec Vulkan :

* `VK_PRESENT_MODE_IMMEDIATE_KHR` : les images émises par votre application sont directement envoyées à l'écran, ce
qui peut produire des déchirures (tearing).
* `VK_PRESENT_MODE_FIFO_KHR` : la swap chain est une file d'attente, et l'écran récupère l'image en haut de la pile
quand il est rafraîchi, alors que le programme insère ses nouvelles images à l'arrière. Si la queue est pleine le
programme doit attendre. Ce mode est très similaire à la synchronisation verticale utilisée par la plupart des jeux
vidéo modernes. L'instant durant lequel l'écran est rafraichi s'appelle l'*intervalle de rafraîchissement vertical* (vertical blank).
* `VK_PRESENT_MODE_FIFO_RELAXED_KHR` : ce mode ne diffère du précédent que si l'application est en retard et que la
queue est vide pendant le vertical blank. Au lieu d'attendre le prochain vertical blank, une image arrivant dans la
file d'attente sera immédiatement transmise à l'écran.
* `VK_PRESENT_MODE_MAILBOX_KHR` : ce mode est une autre variation du second mode. Au lieu de bloquer l'application
quand le file d'attente est pleine, les images présentes dans la queue sont simplement remplacées par de nouvelles.
Ce mode peut être utilisé pour implémenter le triple buffering, qui vous permet d'éliminer le tearing tout en réduisant
le temps de latence entre le rendu et l'affichage qu'une file d'attente implique.

Seul `VK_PRESENT_MODE_FIFO_KHR` est toujours disponible. Nous aurons donc encore à écrire une fonction pour réaliser
un choix, car le mode que nous choisirons préférentiellement est `VK_PRESENT_MODE_MAILBOX_KHR` :

```c++
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    return VK_PRESENT_MODE_FIFO_KHR;
}
```

Je pense que le triple buffering est un très bon compromis. Vérifions si ce mode est disponible :

```c++
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
```

### Le swap extent

Il ne nous reste plus qu'une propriété, pour laquelle nous allons créer encore une autre fonction :

```c++
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {

}
```

Le swap extent donne la résolution des images dans la swap chain et correspond quasiment toujours à la résolution de
la fenêtre que nous utilisons. L'étendue des résolutions disponibles est définie dans la
structure `VkSurfaceCapabilitiesKHR`. Vulkan nous demande de faire correspondre notre résolution à celle de la fenêtre
fournie par le membre `currentExtent`. Cependant certains gestionnaires de fenêtres nous permettent de choisir une
résolution différente, ce que nous pouvons détecter grâce aux membres `width` et `height` qui sont alors égaux à la plus
grande valeur d'un `uint32_t`. Dans ce cas nous choisirons la résolution correspondant le mieux à la taille de la
fenêtre, dans les bornes de `minImageExtent` et `maxImageExtent`.

```c++
#include <cstdint> // uint32_t
#include <limits> // std::numeric_limits
#include <algorithm> // std::clamp

...

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {WIDTH, HEIGHT};

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}
```

La fonction `clamp` est utilisée ici pour limiter les valeurs `WIDTH` et `HEIGHT` entre le minimum et le
maximum supportés par l'implémentation.

## Création de la swap chain

Maintenant que nous avons toutes ces fonctions nous pouvons enfin acquérir toutes les informations nécessaires à la
création d'une swap chain.

Créez une fonction `createSwapChain`. Elle commence par récupérer les résultats des fonctions précédentes. Appelez-la
depuis `initVulkan` après la création du logical device.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
}

void createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
}
```

Il nous reste une dernière chose à faire : déterminer le nombre d'images dans la swap chain. L'implémentation décide
d'un minimum nécessaire pour fonctionner :

```c++
uint32_t imageCount = swapChainSupport.capabilities.minImageCount;
```

Se contenter du minimum pose cependant un problème. Il est possible que le driver fasse attendre notre programme car il
n'a pas fini certaines opérations, ce que nous ne voulons pas. Il est recommandé d'utiliser au moins une image de plus
que ce minimum :

```c++
uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
```

Il nous faut également prendre en compte le maximum d'images supportées par l'implémentation. La valeur `0` signifie
qu'il n'y a pas de maximum autre que la mémoire.

```c++
if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
}
```

Comme la tradition le veut avec Vulkan, la création d'une swap chain nécessite de remplir une grande structure. Elle
commence de manière familière :

```c++
VkSwapchainCreateInfoKHR createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
createInfo.surface = surface;
```

Après avoir indiqué la surface à laquelle la swap chain doit être liée, les détails sur les images de la swap chain
doivent être fournis :

```c++
createInfo.minImageCount = imageCount;
createInfo.imageFormat = surfaceFormat.format;
createInfo.imageColorSpace = surfaceFormat.colorSpace;
createInfo.imageExtent = extent;
createInfo.imageArrayLayers = 1;
createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
```

Le membre `imageArrayLayers` indique le nombre de couches que chaque image possède. Ce sera toujours `1` sauf si vous
développez une application stéréoscopique 3D. Le champ de bits `imageUsage` spécifie le type d'opérations que nous
appliquerons aux images de la swap chain. Dans ce tutoriel nous effectuerons un rendu directement sur les images,
nous les utiliserons donc comme *color attachement*. Vous voudrez peut-être travailler sur une image séparée pour
pouvoir appliquer des effets en post-processing. Dans ce cas vous devrez utiliser une valeur comme
`VK_IMAGE_USAGE_TRANSFER_DST_BIT` à la place et utiliser une opération de transfert de mémoire pour placer le
résultat final dans une image de la swap chain.

```c++
QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
} else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Optionnel
    createInfo.pQueueFamilyIndices = nullptr; // Optionnel
}
```

Nous devons ensuite indiquer comment les images de la swap chain seront utilisées dans le cas où plusieurs queues
seront à l'origine d'opérations. Cela sera le cas si la queue des graphismes n'est pas la même que la queue de
présentation. Nous devrons alors dessiner avec la graphics queue puis fournir l'image à la presentation queue. Il
existe deux manières de gérer les images accédées par plusieurs queues :

* `VK_SHARING_MODE_EXCLUSIVE` : une image n'est accesible que par une queue à la fois et sa gestion doit être
explicitement transférée à une autre queue pour pouvoir être utilisée. Cette option offre le maximum de performances.
* `VK_SHARING_MODE_CONCURRENT` : les images peuvent être simplement utilisées par différentes queue families.

Si nous avons deux queues différentes, nous utiliserons le mode concurrent pour éviter d'ajouter un chapitre sur la
possession des ressources, car cela nécessite des concepts que nous ne pourrons comprendre correctement que plus tard.
Le mode concurrent vous demande de spécifier à l'avance les queues qui partageront les images en utilisant les
paramètres `queueFamilyIndexCount` et `pQueueFamilyIndices`. Si les graphics queue et presentation queue sont les
mêmes, ce qui est le cas sur la plupart des cartes graphiques, nous devons rester sur le mode exclusif car le mode
concurrent requiert au moins deux queues différentes.

```c++
createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
```

Nous pouvons spécifier une transformation à appliquer aux images quand elles entrent dans la swap chain si cela est
supporté (à vérifier avec `supportedTransforms` dans `capabilities`), comme par exemple une rotation de 90 degrés ou
une symétrie verticale. Si vous ne voulez pas de transformation, spécifiez la transformation actuelle.

```c++
createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
```

Le champ `compositeAlpha` indique si le canal alpha doit être utilisé pour mélanger les couleurs avec celles des autres
fenêtres. Vous voudrez quasiment tout le temps ignorer cela, et indiquer `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR` :

```c++
createInfo.presentMode = presentMode;
createInfo.clipped = VK_TRUE;
```

Le membre `presentMode` est assez simple. Si le membre `clipped` est activé avec `VK_TRUE` alors les couleurs des
pixels masqués par d'autres fenêtres seront ignorées. Si vous n'avez pas un besoin particulier de lire ces
informations, vous obtiendrez de meilleures performances en activant ce mode.

```c++
createInfo.oldSwapchain = VK_NULL_HANDLE;
```

Il nous reste un dernier champ, `oldSwapchain`. Il est possible avec Vulkan que la swap chain devienne
invalide ou mal adaptée pendant que votre application tourne, par exemple parce que la fenêtre a été redimensionnée.
Dans ce cas la swap chain doit être intégralement recréée et une référence à l'ancienne swap chain doit être fournie.
C'est un sujet compliqué que nous aborderons [dans un chapitre futur](!fr/Dessiner_un_triangle/Recréation_de_la_swap_chain).
Pour le moment, considérons que nous ne devrons jamais créer qu'une swap chain.

Ajoutez un membre donnée pour stocker l'objet `VkSwapchainKHR` :

```c++
VkSwapchainKHR swapChain;
```

Créer la swap chain ne se résume plus qu'à appeler `vkCreateSwapchainKHR` :

```c++
if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
    throw std::runtime_error("échec de la création de la swap chain!");
}
```

Les paramètres sont le logical device, la structure contenant les informations, l'allocateur optionnel et la variable
contenant la référence à la swap chain. Cet objet devra être explicitement détruit à l'aide de la fonction
`vkDestroySwapchainKHR` avant de détruire le logical device :

```c++
void cleanup() {
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    ...
}
```

Lancez maintenant l'application et contemplez la création de la swap chain! Si vous obtenez une erreur de violation
d'accès dans `vkCreateSwapchainKHR` ou voyez un message comme `Failed to find 'vkGetInstanceProcAddress' in layer
SteamOverlayVulkanLayer.ddl`, allez voir [la FAQ à propos de la layer Steam](!fr/FAQ).

Essayez de retirer la ligne `createInfo.imageExtent = extent;` avec les validation layers actives. Vous verrez que
l'une d'entre elles verra l'erreur et un message vous sera envoyé :

![](/images/swap_chain_validation_layer.png)

## Récupérer les images de la swap chain

La swap chain est enfin créée. Il nous faut maintenant récupérer les références aux `VkImage` dans la swap
chain. Nous les utiliserons pour l'affichage et dans les chapitres suivants. Ajoutez un membre donnée pour les
stocker :

```c++
std::vector<VkImage> swapChainImages;
```

Ces images ont été créées par l'implémentation avec la swap chain et elles seront automatiquement supprimées avec la
destruction de la swap chain, nous n'aurons donc rien à rajouter dans la fonction `cleanup`.

Ajoutons le code nécessaire à la récupération des références à la fin de `createSwapChain`, juste après l'appel à
`vkCreateSwapchainKHR`. Comme notre logique n'a au final informé Vulkan que d'un minimum pour le nombre d'images dans la
swap chain, nous devons nous enquérir du nombre d'images avant de redimensionner le conteneur.

```c++
vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
swapChainImages.resize(imageCount);
vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
```

Une dernière chose : gardez dans des variables le format et le nombre d'images de la swap chain, nous en aurons
besoin dans de futurs chapitres.

```c++
VkSwapchainKHR swapChain;
std::vector<VkImage> swapChainImages;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;

...

swapChainImageFormat = surfaceFormat.format;
swapChainExtent = extent;
```

Nous avons maintenant un ensemble d'images sur lesquelles nous pouvons travailler et qui peuvent être présentées pour
être affichées. Dans le prochain chapitre nous verrons comment utiliser ces images comme des cibles de rendu,
puis nous verrons le pipeline graphique et les commandes d'affichage!

[Code C++](/code/06_swap_chain_creation.cpp)
