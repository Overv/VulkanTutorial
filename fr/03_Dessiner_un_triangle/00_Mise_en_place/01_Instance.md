## Création d'une instance

La première chose à faire avec Vulkan est son initialisation au travers d'une *instance*. Cette instance relie
l'application à l'API. Pour la créer vous devrez donner quelques informations au driver.

Créez une fonction `createInstance` et appelez-la depuis la fonction `initVulkan` :

```c++
void initVulkan() {
    createInstance();
}
```

Ajoutez ensuite un membre donnée représentant cette instance :

```c++
private:
VkInstance instance;
```

Pour créer l'instance, nous allons d'abord remplir une première structure avec des informations sur notre application.
Ces données sont optionnelles, mais elles peuvent fournir des informations utiles au driver pour optimiser ou
diagnostiquer les erreurs lors de l'exécution, par exemple en reconnaissant le nom d'un moteur graphique. Cette structure
s'appelle `VkApplicationInfo` :

```c++
void createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
}
```

Comme mentionné précédemment, la plupart des structures Vulkan vous demandent d'expliciter leur propre type dans le
membre `sType`. Cela permet d'indiquer la version exacte de la structure que nous voulons utiliser : il y aura dans
le futur des extensions à celles-ci. Pour simplifier leur implémentation, les utiliser ne nécessitera que de changer
le type `VK_STRUCTURE_TYPE_XXX` en `VK_STRUCTURE_TYPE_XXX_2` (ou plus de 2) et de fournir une structure complémentaire
à l'aide du pointeur `pNext`. Nous n'utiliserons aucune extension, et donnerons donc toujours `nullptr` à `pNext`.

Avec Vulkan, nous rencontrerons souvent (TRÈS souvent) des structures à remplir pour passer les informations à Vulkan.
Nous allons maintenant remplir le reste de la structure permettant la création de l'instance. Celle-ci n'est pas
optionnelle. Elle permet d'informer le driver des extensions et des validation layers que nous utiliserons, et ceci
de manière globale. Globale siginifie ici que ces données ne serons pas spécifiques à un périphérique. Nous verrons
la signification de cela dans les chapitres suivants.

```c++
VkInstanceCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
createInfo.pApplicationInfo = &appInfo;
```

Les deux premiers paramètres sont simples. Les deux suivants spécifient les extensions dont nous aurons besoin. Comme
nous l'avons vu dans l'introduction, Vulkan ne connaît pas la plateforme sur laquelle il travaille, et nous aurons donc
besoin d'extensions pour utiliser des interfaces avec le gestionnaire de fenêtre. GLFW possède une fonction très
pratique qui nous donne la liste des extensions dont nous aurons besoin pour afficher nos résultats. Remplissez donc la
structure de ces données :

```c++
uint32_t glfwExtensionCount = 0;
const char** glfwExtensions;

glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

createInfo.enabledExtensionCount = glfwExtensionCount;
createInfo.ppEnabledExtensionNames = glfwExtensions;
```

Les deux derniers membres de la structure indiquent les validations layers à activer. Nous verrons cela dans le prochain
chapitre, laissez ces champs vides pour le moment :

```c++
createInfo.enabledLayerCount = 0;
```

Nous avons maintenant indiqué tout ce dont Vulkan a besoin pour créer notre première instance. Nous pouvons enfin
appeler `vkCreateInstance` :

```c++
VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
```

Comme vous le reverrez, l'appel à une fonction pour la création d'un objet Vulkan a le prototype suivant :

* Pointeur sur une structure contenant l'information pour la création
* Pointeur sur une fonction d'allocation que nous laisserons toujours `nullptr`
* Pointeur sur une variable stockant une référence au nouvel objet

Si tout s'est bien passé, la référence à l'instance devrait être contenue dans le membre `VkInstance`. Quasiment toutes
les fonctions Vulkan retournent une valeur de type VkResult, pouvant être soit `VK_SUCCESS` soit un code d'erreur. Afin
de vérifier si la création de l'instance s'est bien déroulée nous pouvons placer l'appel dans un `if` :

```c++
if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("Echec de la création de l'instance!");
}
```

Lancez votre programme pour voir si l'instance s'est créée correctement.

## Vérification du support des extensions

Si vous regardez la documentation pour `vkCreateInstance` vous pourrez voir que l'un des messages d'erreur possible est 
`VK_ERROR_EXTENSION_NOT_PRESENT`. Nous pourrions juste interrompre le programme et afficher une erreur si une extension
manque. Ce serait logique pour des fonctionnalités cruciales comme l'affichage, mais pas dans le cas d'extensions
optionnelles.

La fonction `vkEnumerateInstanceExtensionProperties` permet de récupérer la totalité des extensions supportées par le
système avant la création de l'instance. Elle demande un pointeur vers une variable stockant le nombre d'extensions
supportées et un tableau où stocker des informations sur chacune des extensions. Elle possède également un paramètre
optionnel permettant de filtrer les résultats pour une validation layer spécifique. Nous l'ignorerons pour le moment.

Pour allouer un tableau contenant les détails des extensions nous devons déjà connaître le nombre de ces extensions.
Vous pouvez ne demander que cette information en laissant le premier paramètre `nullptr` :

```c++
uint32_t extensionCount = 0;
vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
```

Nous utiliserons souvent cette méthode. Allouez maintenant un tableau pour stocker les détails des extensions (incluez
<vector>) :

```c++
std::vector<VkExtensionProperties> extensions(extensionCount);
```

Nous pouvons désormais accéder aux détails des extensions :

```c++
vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
```

Chacune des structure `VkExtensionProperties` contient le nom et la version maximale supportée de l'extension. Nous
pouvons les afficher à l'aide d'une boucle `for` toute simple (`\t` représente une tabulation) :

```c++
std::cout << "Extensions disponibles :\n";

for (const auto& extension : extensions) {
    std::cout << '\t' << extension.extensionName << '\n';
}
```

Vous pouvez ajouter ce code dans la fonction `createInstance` si vous voulez indiquer des informations à propos du
support Vulkan sur la machine. Petit challenge : programmez une fonction vérifiant si les extensions dont vous avez
besoin (en particulier celles indiquées par GLFW) sont disponibles.

## Libération des ressources

L'instance contenue dans `VkInstance` ne doit être détruite qu'à la fin du programme. Nous la détruirons dans la
fonction `cleanup` grâce à la fonction `vkDestroyInstance` :

```c++
void cleanup() {
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}
```

Les paramètres de cette fonction sont évidents. Nous y retrouvons le paramètre pour un désallocateur que nous laissons
`nullptr`. Toutes les ressources que nous allouerons à partir du prochain chapitre devront être libérées avant la
libération de l'instance.

Avant d'avancer dans les notions plus complexes, créons un moyen de déboger notre programme avec
[les validations layers.](!fr/Dessiner_un_triangle/Mise_en_place/Validation_layers).

[Code C++](/code/01_instance_creation.cpp)
