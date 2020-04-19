## Introduction

La sélection d'un physical device faite, nous devons générer un *logical device* pour servir d'interface. Le 
processus de sa création est similaire à celui de l'instance : nous devons décrire ce dont nous aurons besoin. Nous 
devons également spécifier les queues dont nous aurons besoin. Vous pouvez également créer plusieurs logical devices à
partir d'un physical device si vous en avez besoin.

Commencez par ajouter un nouveau membre donnée pour stocker la référence au logical device.

```c++
VkDevice device;
```

Ajoutez ensuite une fonction `createLogicalDevice` et appelez-la depuis `initVulkan`.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
    createLogicalDevice();
}

void createLogicalDevice() {

}
```

## Spécifier les queues à créer

La création d'un logical device requiert encore que nous remplissions des informations dans des structures. La 
première de ces structures s'appelle `VkDeviceQueueCreateInfo`. Elle indique le nombre de queues que nous désirons pour 
chaque queue family. Pour le moment nous n'avons besoin que d'une queue originaire d'une unique queue family : la 
première avec un support pour les graphismes.

```c++
QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

VkDeviceQueueCreateInfo queueCreateInfo{};
queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
queueCreateInfo.queueCount = 1;
```

Actuellement les drivers ne vous permettent que de créer un petit nombre de queues pour chacune des familles, et vous
n'avez en effet pas besoin de plus. Vous pouvez très bien créer les commandes (command buffers) depuis plusieurs 
threads et les soumettre à la queue d'un coup sur le thread principal, et ce sans perte de performance.

Vulkan permet d'assigner des niveaux de priorité aux queues à l'aide de floats compris entre `0.0` et `1.0`. Vous 
pouvez ainsi influencer l'exécution des command buffers. Il est nécessaire d'indiquer une priorité même lorsqu'une
seule queue est présente :

```c++
float queuePriority = 1.0f;
queueCreateInfo.pQueuePriorities = &queuePriority;
```

## Spécifier les fonctionnalités utilisées

Les prochaines informations à fournir sont les fonctionnalités du physical device que nous souhaitons utiliser. Ce 
sont celles dont nous avons vérifié la présence avec `vkGetPhysicalDeviceFeatures` dans le chapitre précédent. Nous 
n'avons besoin de rien de spécial pour l'instant, nous pouvons donc nous contenter de créer la structure et de tout 
laisser à `VK_FALSE`, valeur par défaut. Nous reviendrons sur cette structure quand nous ferons des choses plus 
intéressantes avec Vulkan.

```c++
VkPhysicalDeviceFeatures deviceFeatures{};
```

## Créer le logical device

Avec ces deux structure prêtes, nous pouvons enfin remplir la structure principale appelée `VkDeviceCreateInfo`.

```c++
VkDeviceCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
```

Référencez d'abord les structures sur la création des queues et sur les fonctionnalités utilisées :

```c++
createInfo.pQueueCreateInfos = &queueCreateInfo;
createInfo.queueCreateInfoCount = 1;

createInfo.pEnabledFeatures = &deviceFeatures;
```

Le reste ressemble à la structure `VkInstanceCreateInfo`. Nous devons spécifier les extensions spécifiques de la 
carte graphique et les validation layers.

Un exemple d'extension spécifique au GPU est `VK_KHR_swapchain`. Celle-ci vous permet de présenter à l'écran les images 
sur lesquels votre programme a effectué un rendu. Il est en effet possible que certains GPU ne possèdent pas cette 
capacité, par exemple parce qu'ils ne supportent que les compute shaders. Nous reviendrons sur cette extension
dans le chapitre dédié à la swap chain.

Comme dit dans le chapitre sur les validation layers, nous activerons les mêmes que celles que nous avons spécifiées 
lors de la création de l'instance. Nous n'avons pour l'instant besoin d'aucune validation layer en particulier. Notez
que le standard ne fait plus la différence entre les extensions de l'instance et celles du device, au point que les
paramètres `enabledLayerCount` et `ppEnabledLayerNames` seront probablement ignorés. Nous les remplissons quand même
pour s'assurer de la bonne compatibilité avec les anciennes implémentations.

```c++
createInfo.enabledExtensionCount = 0;

if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
} else {
    createInfo.enabledLayerCount = 0;
}
```

C'est bon, nous pouvons maintenant instancier le logical device en appelant la fonction `vkCreateDevice`.

```c++
if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
    throw std::runtime_error("échec lors de la création d'un logical device!");
}
```

Les paramètres sont d'abord le physical device dont on souhaite extraire une interface, ensuite la structure contenant
les informations, puis un pointeur optionnel pour l'allocation et enfin un pointeur sur la référence au logical 
device créé. Vérifions également si la création a été un succès ou non, comme lors de la création de l'instance.

Le logical device doit être explicitement détruit dans la fonction `cleanup` avant le physical device :

```c++
void cleanup() {
    vkDestroyDevice(device, nullptr);
    ...
}
```

Les logical devices n'interagissent pas directement avec l'instance mais seulement avec le physical device, c'est 
pourquoi il n'y a pas de paramètre pour l'instance.

## Récupérer des références aux queues

Les queue families sont automatiquement crées avec le logical device. Cependant nous n'avons aucune interface avec 
elles. Ajoutez un membre donnée pour stocker une référence à la queue family supportant les graphismes :

```c++
VkQueue graphicsQueue;
```

Les queues sont implicitement détruites avec le logical device, nous n'avons donc pas à nous en charger dans `cleanup`.

Nous pourrons ensuite récupérer des références à des queues avec la fonction `vkGetDeviceQueue`. Les paramètres en 
sont le logical device, la queue family, l'indice de la queue à récupérer et un pointeur où stocker la référence à la
queue. Nous ne créons qu'une seule queue, nous écrirons donc `0` pour l'indice de la queue.

```c++
vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
```

Avec le logical device et les queues nous allons maintenant pouvoir faire travailler la carte graphique! Dans le 
prochain chapitre nous mettrons en place les ressources nécessaires à la présentation des images à l'écran.

[Code C++](/code/04_logical_device.cpp)
