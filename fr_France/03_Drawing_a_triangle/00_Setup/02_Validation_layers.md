## Que sont les validation layers?

L'API Vulkan est concu pour limiter au maximum le travail du driver. Par conséquent il n'y a aucun traitement d'erreur
par défaut. Une erreur aussi simple que se tromper dans la valeur d'une énumération ou passer un pointeur nul comme
argument non optionnel résultent en un crash. Dans la mesure où Vulkan nous demande d'être complètement explicite, il
est facile d'utiliser une fonctionnalité optionnelle et d'oublier de paramétrer l'utilisation de l'extension à laquelle
elle appartient, par exemple.

Cependant de telles vérifications peuvent être ajoutées à l'API. Vulkan possède un système élégant appelé validation
layers. Ce sont des composants optionnels s'insérant dans les appels des fonctions Vulkan pour y insérer des opérations.
Voici un exemple d'opérations qu'elles réalisent :

* Comparer les valeurs des paramètres à celles de la spécification pour détecter une mauvaise utilisation
* Suivre la création et la destruction des objets pour repérer les pertes de mémoire
* Vérifier la sécurité des threads en suivant l'origine des appels
* Afficher toutes les informations sur les appels à l'aide de la sortie standard
* Suivre les appels Vulkan pour permettre l'analyse dynamique

Voici ce à quoi une fonction de diagnostic pourrait ressembler :

```c++
VkResult vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkInstance* instance) {

    if (pCreateInfo == nullptr || instance == nullptr) {
        log("Pointeur nul passé à un paramètre obligatoire!");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    return real_vkCreateInstance(pCreateInfo, pAllocator, instance);
}
```

Les validation layers peuvent être combinées à loisir pour fournir toutes les fonctionnalités de débuggage nécessaires.
Vous pouvez même activer les validations layers lors du développement et les désactiver lors du déploiement sans
aucun problème!

Vulkan ne possède aucune validation layer, mais nous en avons dans le SDK de LunarG. Elles sont complètement [open
source](https://github.com/KhronosGroup/Vulkan-ValidationLayers), vous pouvez donc voir quelles erreurs elles suivent et
contribuer à leur développement. Les utiliser est la meilleure manière d'éviter que votre application fonctionne grâce
à un comportement spécifique à un driver.

Les validations layers ne sont utilisables que si elles sont installées sur la machine. Il faut le SDK installé et
paramétré pour qu'elles fonctionnent.

Il a existé deux formes de validation layers : les layers spécifiques de l'instance et celles spécifiques du
physical device. Elles ne vérifiaient ainsi respectivement que les appels aux fonctions d'ordre global et les appels aux
fonctions spécifiques au GPU. Les layers spécifiques du GPU sont désormais dépréciées. Les autres portent désormais sur
tous les appels. Cependant la spécification recommande encore que nous activions les validations layers au niveau du 
logical device, car cela est requis par certaines implémentations. Nous nous contenterons de spécifier les mêmes 
layers pour le logical device que pour le physical device, que nous verrons
[plus tard](!Drawing_a_triangle/Setup/Logical_device_and_queues).

## Utiliser les validation layers

Nous allons maintenant activer les validations layers fournies par le SDK de LunarG. Comme les extensions, nous devons
indiquer leurs nom. Au lieu de devoir spécifier les noms de chacune d'entre elles, nous pouvons les activer à l'aide
d'un nom générique : "VK_LAYER_LUNARG_standard_validation".

Mais ajoutons d'abord deux variables spécifiant les layers à activer et si le programme doit en effet les activer. J'ai
choisi d'effectuer ce choix selon si le programme est compilé en mode debug ou non. La macro `NDEBUG` fait partie
du standard et correspond au deuxième cas.

```c++
const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

#ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
#else
    constexpr bool enableValidationLayers = true;
#endif
```

Ajoutons une nouvelle fonction `chackValidationLayerSupport`, qui devra vérifier si les layers que nous voulons utiliser
 sont disponibles. Listez d'abord les validation layers disponibles à l'aide de la fonction
 `vkEnumerateInstanceLayerProperties`. Elle s'utilise de la même façon que `vkEnumerateInstanceExtensionProperties`.

```c++
bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    return false;
}
```

Verifiez que toutes les layers de `validationLayers` sont présentes dans la liste des layers disponibles. Vous aurez
besoin de `<cstring>` pour la fonction `strcmp`.

```c++
for (const char* layerName : validationLayers) {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) {
        if (strcmp(layerName, layerProperties.layerName) == 0) {
            layerFound = true;
            break;
        }
    }

    if (!layerFound) {
        return false;
    }
}

return true;
```

Nous pouvons maintenant utiliser cette fonction dans `createInstance` :

```c++
void createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("les validations layers sont activées mais ne sont pas disponibles!");
    }

    ...
}
```

Lancez maintenant le programme en mode debug et assurez-vous qu'il fonctionne. Si vous obtenez une erreur, vérifiez
votre installation du SDK. Si aucune layer n'est disponible, ou seulement très peu, vous vous trouvez peut-être dans le
cas dû à [ce bug](https://vulkan.lunarg.com/app/issues/578e8c8d5698c020d71580fc) (vous aurez besoin d'un compte pour
voir cette page).

Modifions enfin la structure `VkCreateInstanceInfo` pour inclure les noms des validation layers à utiliser lorsqu'elles
sont activées :

```c++
if (enableValidationLayers) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
} else {
    createInfo.enabledLayerCount = 0;
}
```

Si l'appel à la fonction `checkValidationLayerSupport` est un succès, `vkCreateInstance` ne devrait jamais retourner
`VK_ERROR_LAYER_NOT_PRESENT`, mais exécutez tout de même le programme pour être sûr que d'autres erreurs n'apparaissent 
pas.

## Fonction de rappel des erreurs

Malheureusement, activer les validation layers ne nous aide pas beaucoup car elles n'ont pour l'instant aucun moyen de
nous envoyer les messages. Pour les recevoir nous aurons besoin d'une fonction de rappel qui nécessite l'extension
`VK_EXT_debug_utils`.

Créons d'abord une fonction `getRequiredExtensions`. Elle nous fournira les extensions nécessaires selon que nous
activons les validation layers ou non :

```c++
std::vector<const char*> getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}
```

Les extensions spécifiées par GLFW seront toujours nécessaires, mais celle pour le débugage n'est ajoutée que
conditionnellement. Remarquez l'utilisation de la macro `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` au lieu du nom de
l'extension pour éviter les erreurs de frappe.

Nous pouvons maintenant utiliser cette fonction dans `createInstance` :

```c++
auto extensions = getRequiredExtensions();
createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
createInfo.ppEnabledExtensionNames = extensions.data();
```

Exécutez le programme et assurez-vous que vous ne recevez pas l'erreur `VK_ERROR_EXTENSION_NOT_PRESENT`. Nous ne devrions
 pas avoir besoin de vérifier sa présence dans la mesure où elle devrait être disponible avec les validation layers,
mais sait-on jamais.

Intéressons-nous maintenant à la fonction de rappel. Ajoutez la fonction statique `debugCallback` à votre classe avec le
 prototype `PFN_vkDebugUtilsMessengerCallbackEXT`. `VKAPI_ATTR` et `VKAPI_CALL` assurent une comptaibilité avec tous les
 compilateurs.

```c++
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}
```

Le premier paramètre indique la sévérité du message, et peut prendre les valeurs suivantes :

* `VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT`: Message de diagnostic
* `VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT`: Message d'information (allocation d'une ressource...)
* `VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT`: Message relevant un comportment qui n'est pas un bug mais plutôt
une imperfection involontaire
* `VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT`: Message relevant un comportant invalide pouvant mener à un crash

Les valeurs de cette énumération on été conçues de telle sorte qu'il est possible de les comparer pour vérifier la
sévérité d'un message, par exemple :

```c++
if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    // Le message est suffisemment important pour être affiché
}
```

Le paramètre `messageType` peut prendre les valeurs suivantes :

* `VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT` : Un évenement quelconque est survenu, sans lien avec les
performances ou la spécification
* `VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT` : Une violation de la spécification ou une potentielle erreur est
survenue
* `VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT` : Utilisation potentiellement mal optiomisée de Vulkan

Le paramètre `pCallbackData` est une structure du type `VkDebugUtilsMessengerCallbackDataEXT` contenant les détails du
message. Ses membres les plus importants sont :

* `pMessage`: Le message sous la forme d'une chaîne de type C terminée par le caractère nul
* `pObjects`: Un tableau d'objets Vulkan liés au message
* `objectCount`: Le nombre d'objets dans le tableau précédent

Finallement, le paramètre `pUserData` est un pointeur sur une donnée quelconque que vous pouvez spécifier à la création
de la fonction de rappel.

La fonction de rappel que nous programmons retourne un booléen déterminant si la fonction à l'origine de son appel doit
être interrompue. Si elle retourne `VK_TRUE`, l'exécution de la fonction est interrompue et cette dernière retourne
`VK_ERROR_VALIDATION_FAILED_EXT`. Cette fonctionnalité n'est globalement utilisée que pour tester les validation layers
elles-mêmes, nous retournerons donc invariablement `VK_FALSE`.

Il ne nous reste plus qu'à fournir notre fonction à Vulkan. D'une manière suprenante, même le messager de débugage se
gère à travers une référence, du type `VkDebugUtilsMessengerEXT`, que nous devrons explicitement créer et détruire. Une
telle fonction de rappel est appelée *messager*, et vous pouvez en posséder autant que vous le désirez. Ajoutez un
membre donnée pour le messager sous l'instance :

```c++
VkDebugUtilsMessengerEXT callback;
```

Ajoutez ensuite une fonction `setupDebugCallback` et appelez la dans `initVulkan` après `createInstance` :

```c++
void initVulkan() {
    createInstance();
    setupDebugCallback();
}

void setupDebugCallback() {
    if (!enableValidationLayers) return;

}
```

Nous devons maintenant remplir une structure avec des informations sur le messager :

```c++
VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
createInfo.pfnUserCallback = debugCallback;
createInfo.pUserData = nullptr; // Optionnel
```

Le champ `messageSeverity` vous permet de filtrer les niveaux de sévérité pour lesquels la fonction de rappel sera
appelée. J'ai laissé tous les types sauf `VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT`, ce qui permet de recevoir
toutes les informations à propos de possibles bugs tout en éliminant la verbose.

De manière similaire, le champ `messageType` vous permet de filtrer les types de message pour lesquels la fonction de
rappel sera appelée. J'y ai mis tous les types possibles. Vous pouvez très bien en désactiver si ils ne vous servent à
rien.

Le champ `pfnUserCallback` indique le pointeur sur la fonction de rappel.

Vous pouvez optionnellement ajouter un pointeur sur une donnée de votre choix grâce au champ `pUserData`. Le pointeur
fait partie des paramètres de la fonction de rappel.

Notez qu'il existe de nombreuses autres manières de configurer des messagers auprès des validation layers, mais nous
avons ici une bonne base pour ce tutoriel. Référez-vous à la 
[spécification de l'extension](www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VK_EXT_debug_utils)
pour plus d'informations sur ces possibilités.

Cette structure doit maintenant être passée à la fonction `vkCreateDebugUtilsMessengerEXT` afin de créer l'objet
`VkDebugUtilsMessengerEXT`. Malheureusement cette fonction fait partie d'une extension non incluse par GLFW. Nous devons
 donc gérer la procédure de son chargement nous-mêmes. Nous utiliserons la fonction `vkGetInstancePorcAddr` pous en
récupérer un pointeur. Nous allons créer notre propre fonction - servant de proxy - pour abstraire cela. Je l'ai ajoutée
 au-dessus de la définition de la classe `HelloTriangleApplication`.

```c++
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
```

La fonction `vkGetInstanceProcAddr` retourne `nullptr` si la fonction n'a pas pu être chargée. Nous pouvons maintenant
utiliser cette fonction pour créer le messager s'il est disponible :

```c++
if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
    throw std::runtime_error("le messager n'a pas pu être créé!");
}
```

Le troisième paramètre est encore le même allocateur optionnel que nous laissons `nullptr`. Les autres paramètres sont
assez logiques. La fonction de rappel est spécifique de l'instance et des validation layers, nous devons donc passer
l'instance en premier argument. Lancez le programme et vérifiez qu'il fonction... Vous devrez avoir le résultat 
suivant...
:

![](/images/validation_layer_test.png)

...indiquant déjà un bug dans notre application! En effet l'objet `VkDebugUtilsMessengerEXT` doit être libéré 
explicitement à l'aide de la fonction `vkDestroyDebugUtilsMessagerEXT`. De même qu'avec 
`vkCreateDebugUtilsMessangerEXT` nous devons charger dynamiquement cette fonction. Notez qu'il est normal que le 
message s'affiche plusieurs fois; il y a plusieurs validation layers, et dans certains cas leurs domaines de travail 
se recoupent.

Créez une autre fonction proxy en-dessous de `CreateDebugUtilsMessengerEXT` :

```c++
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}
```

Nous pouvons maintenant l'appeler dans notre fonction `cleanup` :

```c++
void cleanup() {
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, callback, nullptr);
    }

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}
```

Si vous exécutez le programme maintenant, vous devriez constater que le message n'apparait plus. Si vous voulez voir
quel appel a lancé un appel, vous pouvez insérer un point d'arrêt dans la fonction de rappel.

## Configuration

Les validation layers peuvent être paramétrées de nombreuses autres manières que juste avec les informations que nous
avons fournies dans la structure `VkDebugUtilsMessangerCreateInfoEXT`. Ouvrez le SDK Vulkan et rendez-vous dans le
dossier `Config`. Vous y trouverez le fichier `vk_layer_settings.txt` qui vous expliquera comment configurer les
validation layers.

Pour configurer les layers pour votre propre application, copiez le fichier dans les dossiers `Debug` et/ou `Release`,
puis suivez les instructions pour obtenir le comportement que vous souhaitez. Cependant, pour le reste du tutoriel, je
partirai du principe que vous les avez laissées avec leur comportement par défaut.

Tout au long du tutoriel je laisserai de petites erreurs intentionnelles pour vous montrer à quel point les validation
layers sont pratiques, et à quel point vous devez comprendre tout ce que vous faites avec Vulkan. Il est maintenant
temps de s'intéresser aux [devices Vulkan dans le système](!Drawing_a_triangle/Setup/Physical_devices_and_queue_families).

[Code C++](/code/02_validation_layers.cpp)
