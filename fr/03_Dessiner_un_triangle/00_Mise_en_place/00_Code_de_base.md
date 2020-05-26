## Structure générale

Dans le chapitre précédent nous avons créé un projet Vulkan avec une configuration solide et nous l'avons testé. Nous
recommençons ici à partir du code suivant :

```c++
#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>

class HelloTriangleApplication {
public:
    void run() {
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    void initVulkan() {

    }

    void mainLoop() {

    }

    void cleanup() {

    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;`
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

Nous incluons d'abord le header Vulkan du SDK, qui fournit les fonctions, les structures et les énumérations.
`<stdexcept>` et `<iostream>` nous permettront de reporter et de traiter les erreurs. Le header `<functional>` nous 
servira pour l'écriture d'une lambda dans la section sur la gestion des ressources. `<cstdlib>` nous fournit les macros
`EXIT_FAILURE` et `EXIT_SUCCESS` (optionnelles).

Le programme est écrit à l'intérieur d'une classe, dans laquelle seront stockés les objets Vulkan. Nous avons également
une fonction pour la création de chacun de ces objets. Une fois toute l'initialisation réalisée, nous entrons dans la
boucle principale, qui attend que nous fermions la fenêtre pour quitter le programme, après avoir libéré grâce à la
fonction cleanup toutes les ressources que nous avons allouées .

Si nous rencontrons une quelconque erreur lors de l'exécution nous lèverons une `std::runtime_error` comportant un
message descriptif, qui sera affiché sur le terminal depuis la fonction `main`. Afin de s'assurer que nous récupérons
bien toutes les erreurs, nous utilisons `std::exception` dans le `catch`. Nous verrons bientôt que la requête de
certaines extensions peut mener à lever des exceptions.

À peu près tous les chapitres à partir de celui-ci introduiront une nouvelle fonction appelée dans `initVulkan` et un
nouvel objet Vulkan qui sera justement créé par cette fonction. Il sera soit détruit dans `cleanup`, soit libéré 
automatiquement.

## Gestion des ressources

De la même façon qu'une quelconque ressource explicitement allouée par `new` doit être explicitement libérée par `delete`, nous
devrons explicitement détruire quasiment toutes les ressources Vulkan que nous allouerons. Il est possible d'exploiter
des fonctionnalités du C++ pour s’acquitter automatiquement de cela. Ces possibilités sont localisées dans `<memory>` si
vous désirez les utiliser. Cependant nous resterons explicites pour toutes les opérations dans ce tutoriel, car la
puissance de Vulkan réside en particulier dans la clareté de l'expression de la volonté du programmeur. De plus, cela
nous permettra de bien comprendre la durée de vie de chacun des objets.

Après avoir suivi ce tutoriel vous pourrez parfaitement implémenter une gestion automatique des ressources en
spécialisant `std::shared_ptr` par exemple. L'utilisation du [RAII](https://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization)
à votre avantage est toujours recommandé en C++ pour de gros programmes Vulkan, mais il est quand même bon de
commencer par connaître les détails de l'implémentation.

Les objets Vulkan peuvent être créés de deux manières. Soit ils sont directement créés avec une fonction du type 
`vkCreateXXX`, soit ils sont alloués à l'aide d'un autre objet avec une fonction `vkAllocateXXX`. Après vous
être assuré qu'il n'est plus utilisé où que ce soit, il faut le détruire en utilisant les fonctions 
`vkDestroyXXX` ou `vkFreeXXX`, respectivement. Les paramètres de ces fonctions varient sauf pour l'un d'entre eux :
`pAllocator`. Ce paramètre optionnel vous permet de spécifier un callback sur un allocateur de mémoire. Nous
n'utiliserons jamais ce paramètre et indiquerons donc toujours `nullptr`.

## Intégrer GLFW

Vulkan marche très bien sans fenêtre si vous voulez l'utiliser pour du rendu sans écran (offscreen rendering en
Anglais), mais c'est tout de même plus intéressant d'afficher quelque chose! Remplacez d'abord la ligne 
`#include <vulkan/vulkan.h>` par :

```c++
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
```

GLFW va alors automatiquement inclure ses propres définitions des fonctions Vulkan et vous fournir le header Vulkan.
Ajoutez une fonction `initWindow` et appelez-la depuis `run` avant les autres appels. Nous utiliserons cette fonction
pour initialiser GLFW et créer une fenêtre.

```c++
void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

private:
    void initWindow() {

    }
```

Le premier appel dans `initWindow` doit être `glfwInit()`, ce qui initialise la librairie. Dans la mesure où GLFW a été
créée pour fonctionner avec OpenGL, nous devons lui demander de ne pas créer de contexte OpenGL avec l'appel suivant :

```c++
glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
```

Dans la mesure où redimensionner une fenêtre n'est pas chose aisée avec Vulkan, nous verrons cela plus tard et
l'interdisons pour l'instant.

```c++
glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
```

Il ne nous reste plus qu'à créer la fenêtre. Ajoutez un membre privé `GLFWWindow* m_window` pour en stocker une
référence, et initialisez la ainsi :

```c++
window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
```

Les trois premiers paramètres indiquent respectivement la largeur, la hauteur et le titre de la fenêtre. Le quatrième 
vous permet optionnellement de spécifier un moniteur sur lequel ouvrir la fenêtre, et le cinquième est spécifique à 
OpenGL.

Nous devrions plutôt utiliser des constantes pour la hauteur et la largeur dans la mesure où nous aurons besoin de ces
valeurs dans le futur. J'ai donc ajouté ceci au-dessus de la définition de la classe `HelloTriangleApplication` :

```c++
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
```

et remplacé la création de la fenêtre par :

```c++
window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
```

Vous avez maintenant une fonction `initWindow` ressemblant à ceci :

```c++
void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}
```

Pour s'assurer que l'application tourne jusqu'à ce qu'une erreur ou un clic sur la croix ne l'interrompe, nous
devons écrire une petite boucle de gestion d'évènements :

```c++
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}
```

Ce code est relativement simple. GLFW récupère tous les évènements disponibles, puis vérifie qu'aucun d'entre eux ne
correspond à une demande de fermeture de fenêtre. Ce sera aussi ici que nous appellerons la fonction qui affichera un
triangle.

Une fois la requête pour la fermeture de la fenêtre récupérée, nous devons détruire toutes les ressources allouées et
quitter GLFW. Voici notre première version de la fonction `cleanup` :

```c++
void cleanup() {
    glfwDestroyWindow(window);

    glfwTerminate();
}
```

Si vous lancez l'application, vous devriez voir une fenêtre appelée "Vulkan" qui se ferme en cliquant sur la croix.
Maintenant que nous avons une base pour notre application Vulkan, [créons notre premier objet Vulkan!](!fr/Dessiner_un_triangle/Mise_en_place/Instance)!

[Code C++](/code/00_base_code.cpp)
