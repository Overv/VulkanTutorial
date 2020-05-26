## Sélection d'un physical device

La librairie étant initialisée à travers `VkInstance`, nous pouvons dès à présent chercher et sélectionner une carte
graphique (physical device) dans le système qui supporte les fonctionnalitées dont nous aurons besoin. Nous pouvons en
fait en sélectionner autant que nous voulons et travailler avec chacune d'entre elles, mais nous n'en utiliserons qu'une
dans ce tutoriel pour des raisons de simplicité.

Ajoutez la fonction `pickPhysicalDevice` et appelez la depuis `initVulkan` :

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
}

void pickPhysicalDevice() {

}
```

Nous stockerons le physical device que nous aurons sélectionnée dans un nouveau membre donnée de la classe, et celui-ci
sera du type `VkPhysicalDevice`. Cette référence sera implicitement détruit avec l'instance, nous n'avons donc rien à
ajouter à la fonction `cleanup`.

```c++
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
```

Lister les physical devices est un procédé très similaire à lister les extensions. Comme d'habitude, on commence par en
lister le nombre.

```c++
uint32_t deviceCount = 0;
vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
```

Si aucun physical device ne supporte Vulkan, il est inutile de continuer l'exécution.

```c++
if (deviceCount == 0) {
    throw std::runtime_error("aucune carte graphique ne supporte Vulkan!");
}
```

Nous pouvons ensuite allouer un tableau contenant toutes les références aux `VkPhysicalDevice`.

```c++
std::vector<VkPhysicalDevice> devices(deviceCount);
vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
```

Nous devons maintenant évaluer chacun des gpus et vérifier qu'ils conviennent pour ce que nous voudrons en faire, car
toutes les cartes graphiques n'ont pas été crées égales. Voici une nouvelle fonction qui fera le travail de
sélection :

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    return true;
}
```

Nous allons dans cette fonction vérifier que le physical device respecte nos conditions.

```c++
for (const auto& device : devices) {
    if (isDeviceSuitable(device)) {
        physicalDevice = device;
        break;
    }
}

if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("aucun GPU ne peut exécuter ce programme!");
}
```

La section suivante introduira les premières contraintes que devront remplir les physical devices. Au fur et à mesure
que nous utiliserons de nouvelles fonctionnalités, nous les ajouterons dans cette fonction.

## Vérification des fonctionnalités de base

Pour évaluer la compatibilité d'un physical device nous devons d'abord nous informer sur ses capacités. Des propriétés
basiques comme le nom, le type et les versions de Vulkan supportées peuvent être obtenues en appelant 
`vkGetPhysicalDeviceProperties`.

```c++
VkPhysicalDeviceProperties deviceProperties;
vkGetPhysicalDeviceProperties(device, &deviceProperties);
```

Le support des fonctionnalités optionnelles telles que les textures compressées, les floats de 64 bits et le multi
viewport rendering (pour la VR) s'obtiennent avec `vkGetPhysicalDeviceFeatures` :

```c++
VkPhysicalDeviceFeatures deviceFeatures;
vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
```

De nombreux autres détails intéressants peuvent être requis, mais nous en remparlerons dans les prochains chapitres.

Voyons un premier exemple. Considérons que notre application a besoin d'une carte graphique dédiée supportant les
geometry shaders. Notre fonction `isDeviceSuitable` ressemblerait alors à cela :

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
           deviceFeatures.geometryShader;
}
```

Au lieu de choisir le premier physical device nous convenant, nous pourrions attribuer un score à chacun d'entre eux et
utiliser celui dont le score est le plus élevé. Vous pourriez ainsi préférer une carte graphique dédiée, mais utiliser
un GPU intégré au CPU si le système n'en détecte aucune. Vous pourriez implémenter ce concept comme cela :

```c++
#include <map>

...

void pickPhysicalDevice() {
    ...

    // L'utilisation d'une map permet de les trier automatiquement de manière ascendante
    std::multimap<int, VkPhysicalDevice> candidates;

    for (const auto& device : devices) {
        int score = rateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    // Voyons si la meilleure possède les fonctionnalités dont nous ne pouvons nous passer
    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error("aucun GPU ne peut executer ce programme!");
    }
}

int rateDeviceSuitability(VkPhysicalDevice device) {
    ...

    int score = 0;

    // Les carte graphiques dédiées ont un énorme avantage en terme de performances
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // La taille maximale des textures affecte leur qualité
    score += deviceProperties.limits.maxImageDimension2D;

    // L'application (fictive) ne peut fonctionner sans les geometry shaders
    if (!deviceFeatures.geometryShader) {
        return 0;
    }

    return score;
}
```

Vous n'avez pas besoin d'implémenter tout ça pour ce tutoriel, mais faites-le si vous voulez, à titre d'entrainement.
Vous pourriez également vous contenter d'afficher les noms des cartes graphiques et laisser l'utilisateur choisir.

Nous ne faisons que commencer donc nous prendrons la première carte supportant Vulkan :

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    return true;
}
```

Nous discuterons de la première fonctionnalité qui nous sera nécessaire dans la section suivante.

## Familles de queues (queue families)

Il a été évoqué que chaque opération avec Vulkan, de l'affichage jusqu'au chargement d'une texture, s'effectue en
ajoutant une commande à une queue. Il existe différentes queues appartenant à différents types de
*queue families*. De plus chaque queue family ne permet que certaines commandes. Il se peut par exemple qu'une queue ne
traite que les commandes de calcul et qu'une autre ne supporte que les commandes d'allocation de mémoire.

Nous devons analyser quelles queue families existent sur le système et lesquelles correspondent aux commandes que nous
souhaitons utiliser. Nous allons donc créer la fonction `findQueueFamilies` dans laquelle nous chercherons les
commandes nous intéressant.

Nous allons chercher une queue qui supporte les commandes graphiques, la fonction pourrait ressembler à ça:

```c++
uint32_t findQueueFamilies(VkPhysicalDevice device) {
    // Code servant à trouver la famille de queue "graphique"
}
```

Mais dans un des prochains chapitres, nous allons avoir besoin d'une autre famille de queues, il est donc plus intéressant
de s'y préparer dès maintenant en empactant plusieurs indices dans une structure:

```c++
struct QueueFamilyIndices {
    uint32_t graphicsFamily;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    // Code pour trouver les indices de familles à ajouter à la structure
    return indices
}
```

Que se passe-t-il si une famille n'est pas disponible ? On pourrait lancer une exception dans `findQueueFamilies`,
mais cette fonction n'est pas vraiment le bon endroit pour prendre des decisions concernant le choix du bon Device.
Par exemple, on pourrait *préférer* des Devices avec une queue de transfert dédiée, sans toutefois le requérir.
Par conséquent nous avons besoin d'indiquer si une certaine famille de queues à été trouvé.

Ce n'est pas très pratique d'utiliser une valeur magique pour indiquer la non-existence d'une famille, comme n'importe
quelle valeur de `uint32_t` peut théoriquement être une valeur valide d'index de famille, incluant `0`.
Heureusement, le C++17 introduit un type qui permet la distinction entre le cas où la valeur existe et celui
où elle n'existe pas:

```c++
#include <optional>

...

std::optional<uint32_t> graphicsFamily;

std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl; // faux

graphicsFamily = 0;

std::cout << std::boolalpha << graphicsFamily.has_value() << std::endl; // vrai
```

`std::optional` est un wrapper qui ne contient aucune valeur tant que vous ne lui en assignez pas une.
Vous pouvez, quelque soit le moment, lui demander si il contient une valeur ou non en appelant sa fonction membre 
`has_value()`. On peut donc changer le code comme suit:

```c++
#include <optional>

...

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    // Assigne l'index aux familles qui ont pu être trouvées

    return indices;
}
```

On peut maintenant commencer à implémenter `findQueueFamilies`:

```c++
QueueFamilyIndices findQueueFamily(VkPhysicalDevice) {
    QueueFamilyIndices indices;

    ...

    return indices;
}
```

Récupérer la liste des queue families disponibles se fait de la même manière que d'habitude, avec la fonction 
`vkGetPhysicalDeviceQueueFamilyProperties` :

```c++
uint32_t queueFamilyCount = 0;
vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
```

La structure `VkQueueFamilyProperties` contient des informations sur la queue family, et en particulier le type
d'opérations qu'elle supporte et le nombre de queues que l'on peut instancier à partir de cette famille. Nous devons
trouver au moins une queue supportant `VK_QUEUE_GRAPHICS_BIT` :

```c++
int i = 0;
for (const auto& queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphicsFamily = i;
    }

    i++;
}
```

Nous pouvons maintenant utiliser cette fonction dans `isDeviceSuitable` pour s'assurer que le physical device peut
recevoir les commandes que nous voulons lui envoyer :

```c++
bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    return indices.graphicsFamily.has_value();
}
```

Pour que ce soit plus pratique, nous allons aussi ajouter une fonction générique à la structure:

```c++
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() {
        return graphicsFamily.has_value();
    }
};

...

bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    return indices.isComplete();
}
```

On peut également utiliser ceci pour sortir plus tôt de `findQueueFamilies`:

```c++
for (const auto& queueFamily : queueFamilies) {
    ...

    if (indices.isComplete()) {
        break;
    }

    i++;
}
```

Bien, c'est tout ce dont nous aurons besoin pour choisir le bon physical device! La prochaine étape est de [créer un
logical device](!fr/Dessiner_un_triangle/Mise_en_place/Logical_device_et_queues) pour créer une interface avec la carte.

[Code C++](/code/03_physical_device_selection.cpp)
