## Introduction

Vulkan ignore la plateforme sur laquelle il opère et ne peut donc pas directement établir d'interface avec le
gestionnaire de fenêtres. Pour créer une interface permettant de présenter les rendus à l'écran, nous devons utiliser
l'extension WSI (Window System Integration). Nous verrons dans ce chapitre l'extension `VK_KHR_surface`, l'une des
extensions du WSI. Nous pourrons ainsi obtenir l'objet `VkSurfaceKHR`, qui est un type abstrait de surface sur
lequel nous pourrons effectuer des rendus. Cette surface sera en lien avec la fenêtre que nous avons créée grâce à GLFW.

L'extension `VK_KHR_surface`, qui se charge au niveau de l'instance, a déjà été ajoutée, car elle fait partie des
extensions retournées par la fonction `glfwGetRequiredInstanceExtensions`. Les autres fonctions WSI que nous verrons
dans les prochains chapitres feront aussi partie des extensions retournées par cette fonction.

La surface de fenêtre doit être créée juste après l'instance car elle peut influencer le choix du physical device.
Nous ne nous intéressons à ce sujet que maintenant car il fait partie du grand ensemble que nous abordons et qu'en
parler plus tôt aurait été confus. Il est important de noter que cette surface est complètement optionnelle, et vous
pouvez l'ignorer si vous voulez effectuer du rendu off-screen ou du calculus. Vulkan vous offre ces possibilités sans
vous demander de recourir à des astuces comme créer une fenêtre invisible, là où d'autres APIs le demandaient (cf
OpenGL).

## Création de la window surface

Commencez par ajouter un membre donnée `surface` sous le messager.

```c++
VkSurfaceKHR surface;
```

Bien que l'utilisation d'un objet `VkSurfaceKHR` soit indépendant de la plateforme, sa création ne l'est pas.
Celle-ci requiert par exemple des références à `HWND` et à `HMODULE` sous Windows. C'est pourquoi il existe des
extensions spécifiques à la plateforme, dont par exemple `VK_KHR_win32_surface` sous Windows, mais celles-ci sont
aussi évaluées par GLFW et intégrées dans les extensions retournées par la fonction `glfwGetRequiredInstanceExtensions`.

Nous allons voir l'exemple de la création de la surface sous Windows, même si nous n'utiliserons pas cette méthode.
Il est en effet contre-productif d'utiliser une librairie comme GLFW et un API comme Vulkan pour se retrouver à écrire
du code spécifique à la plateforme. La fonction de GLFW `glfwCreateWindowSurface` permet de gérer les différences de
plateforme. Cet exemple ne servira ainsi qu'à présenter le travail de bas niveau, dont la connaissance est toujours
utile à une bonne utilisation de Vulkan.

Une window surface est un objet Vulkan comme un autre et nécessite donc de remplir une structure, ici 
`VkWin32SurfaceCreateInfoKHR`. Elle possède deux paramètres importants : `hwnd` et `hinstance`. Ce sont les références
à la fenêtre et au processus courant.

```c++
VkWin32SurfaceCreateInfoKHR createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
createInfo.hwnd = glfwGetWin32Window(window);
createInfo.hinstance = GetModuleHandle(nullptr);
```

Nous pouvons extraire `HWND` de la fenêtre à l'aide de la fonction `glfwGetWin32Window`. La fonction 
`GetModuleHandle` fournit une référence au `HINSTANCE` du thread courant.

La surface peut maintenant être crée avec `vkCreateWin32SurfaceKHR`. Cette fonction prend en paramètre une instance, des
détails sur la création de la surface, l'allocateur optionnel et la variable dans laquelle placer la référence. Bien que
cette fonction fasse partie d'une extension, elle est si communément utilisée qu'elle est chargée par défaut par Vulkan.
Nous n'avons ainsi pas à la charger à la main :

```c++
if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
    throw std::runtime_error("échec de la creation d'une window surface!");
}
```

Ce processus est similaire pour Linux, où la fonction `vkCreateXcbSurfaceKHR` requiert la fenêtre et une connexion à
XCB comme paramètres pour X11.

La fonction `glfwCreateWindowSurface` implémente donc tout cela pour nous et utilise le code correspondant à la bonne
plateforme. Nous devons maintenant l'intégrer à notre programme. Ajoutez la fonction `createSurface` et appelez-la
dans `initVulkan` après la création de l'instance et du messager :

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
}

void createSurface() {

}
```

L'appel à la fonction fournie par GLFW ne prend que quelques paramètres au lieu d'une structure, ce qui rend le tout
très simple :

```c++
void createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("échec de la création de la window surface!");
    }
}
```

Les paramètres sont l'instance, le pointeur sur la fenêtre, l'allocateur optionnel et un pointeur sur une variable de
type `VkSurfaceKHR`. GLFW ne fournit aucune fonction pour détruire cette surface mais nous pouvons le faire
nous-mêmes avec une simple fonction Vulkan :

```c++
void cleanup() {
        ...
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        ...
    }
```

Détruisez bien la surface avant l'instance.

## Demander le support pour la présentation

Bien que l'implémentation de Vulkan supporte le WSI, il est possible que d'autres éléments du système ne le supportent
pas. Nous devons donc allonger `isDeviceSuitable` pour s'assurer que le logical device puisse présenter les
rendus à la surface que nous avons créée. La présentation est spécifique aux queues families, ce qui signifie que
nous devons en fait trouver une queue family supportant cette présentation.

Il est possible que les queue families supportant les commandes d'affichage et celles supportant les commandes de
présentation ne soient pas les mêmes, nous devons donc considérer que ces deux queues sont différentes. En fait, les
spécificités des queues families diffèrent majoritairement entre les vendeurs, et assez peu entre les modèles d'une même
série. Nous devons donc étendre la structure `QueueFamilyIndices` :

```c++
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};
```

Nous devons ensuite modifier la fonction `findQueueFamilies` pour qu'elle cherche une queue family pouvant supporter
les commandes de présentation. La fonction qui nous sera utile pour cela est `vkGetPhysicalDeviceSurfaceSupportKHR`.
Elle possède quatre paramètres, le physical device, un indice de queue family, la surface et un booléen. Appelez-la
depuis la même boucle que pour `VK_QUEUE_GRAPHICS_BIT` :

```c++
VkBool32 presentSupport = false;
vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
```

Vérifiez simplement la valeur du booléen et stockez la queue dans la structure si elle est intéressante :

```c++
if (presentSupport) {
    indices.presentFamily = i;
}
```

Il est très probable que ces deux queue families soient en fait les mêmes, mais nous les traiterons comme si elles
étaient différentes pour une meilleure compatibilité. Vous pouvez cependant ajouter un alorithme préférant des
queues combinées pour améliorer légèrement les performances.

## Création de la queue de présentation (presentation queue)

Il nous reste à modifier la création du logical device pour extraire de celui-ci la référence à une presentation queue
`VkQueue`. Ajoutez un membre donnée pour cette référence :

```c++
VkQueue presentQueue;
```

Nous avons besoin de plusieurs structures `VkDeviceQueueCreateInfo`, une pour chaque queue family. Une manière de
gérer ces structures est d'utiliser un `set` contenant tous les indices des queues et un `vector` pour les structures :

```c++
#include <set>

...

QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

float queuePriority = 1.0f;
for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
}
```

Puis modifiez `VkDeviceCreateInfo` pour qu'il pointe sur le contenu du vector :

```c++
createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
createInfo.pQueueCreateInfos = queueCreateInfos.data();
```

Si les queues sont les mêmes, nous n'avons besoin de les indiquer qu'une seule fois, ce dont le set s'assure. Ajoutez
enfin un appel pour récupérer les queue families :

```c++
vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
```

Si les queues sont les mêmes, les variables contenant les références contiennent la même valeur. Dans le prochain
chapitre nous nous intéresserons aux swap chain, et verrons comment elle permet de présenter les rendus à l'écran.

[Code C++](/code/05_window_surface.cpp)
