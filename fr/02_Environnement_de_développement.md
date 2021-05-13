Dans ce chapitre nous allons paramétrer votre environnement de développement pour Vulkan et installer des librairies
utiles. Tous les outils que nous allons utiliser, excepté le compilateur, seront compatibles Windows, Linux et MacOS.
Cependant les étapes pour les installer diffèrent un peu, d'où les sections suivantes.

## Windows

Si vous développez pour Windows, je partirai du principe que vous utilisez Visual Studio pour ce projet.
Pour un support complet de C++17, il vous faut Visual Studio 2017 or 2019. Les étapes décrites ci-dessous
ont été écrites pour VS 2017.

### SDK Vulkan

Le composant central du développement d'applications Vulkan est le SDK. Il inclut les headers, les validation layers
standards, des outils de débogage et un loader pour les fonctions Vulkan. Ce loader récupère les fonctions dans le
driver à l'exécution, comme GLEW pour OpenGL - si cela vous parle.

Le SDK peut être téléchargé sur [le site de LunarG](https://vulkan.lunarg.com/) en utilisant les boutons en bas de page.
Vous n'avez pas besoin de compte, mais celui-ci vous donne accès à une documentation supplémentaire qui pourra vous être
utile.

![](/images/vulkan_sdk_download_buttons.png)

Réalisez l'installation et notez l'emplacement du SDK. La première chose que nous allons faire est vérifier que votre
carte graphique supporte Vulkan. Allez dans le dossier d'installation du SDK, ouvrez le dossier "Bin" et lancez
"vkcube.exe". Vous devriez voire la fenêtre suivante :

![](/images/cube_demo.png)

Si vous recevez un message d'erreur assurez-vous que votre driver est à jour, inclut Vulkan et que votre carte graphique
est supportée. Référez-vous au [chapitre introductif](!fr/Introduction) pour les liens vers les principaux constructeurs.

Il y a d'autres programmes dans ce dossier qui vous seront utiles : "glslangValidator.exe" et "glslc.exe". Nous en aurons besoin pour la
compilation des shaders. Ils transforment un code compréhensible facilement et semblable au C (le
[GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language)) en bytecode.
Nous couvrirons cela dans le chapitre des [modules shader](!fr/Dessiner_un_triangle/Pipeline_graphique_basique/Modules_shaders).
Le dossier "Bin" contient aussi les fichiers binaires du loader Vulkan et des validation layers. Le dossier "Lib" en
contient les librairies.

Enfin, le dossier "Include" contient les headers Vulkan. Vous pouvez parourir les autres
fichiers, mais nous ne les utiliserons pas dans ce tutoriel.

### GLFW

Comme dit précédemment, Vulkan ignore la plateforme sur laquelle il opère, et n'inclut pas d'outil de création
de fenêtre où afficher les résultats de notre travail. Pour bien exploiter les possibilités cross-platform de Vulkan
et éviter les horreurs de Win32, nous utiliserons la [librairie GLFW](http://www.glfw.org/) pour créer une fenêtre et ce
sur Windows, Linux ou MacOS. Il existe d'autres librairies telles que [SDL](https://www.libsdl.org/), mais GLFW a
l'avantage d'abstraire d'autres aspects spécifiques à la plateforme requis par Vulkan.

Vous pouvez trouver la dernière version de GLFW sur leur site officiel. Nous utiliserons la version 64 bits, mais vous
pouvez également utiliser la version 32 bits. Dans ce cas assurez-vous de bien lier le dossier "Lib32" dans le SDK et
non "Lib". Après avoir téléchargé GLFW, extrayez l'archive à l'emplacement qui vous convient. J'ai choisi de créer un
dossier "Librairies" dans le dossier de Visual Studio.

![](/images/glfw_directory.png)

### GLM

Contrairement à DirectX 12, Vulkan n'intègre pas de librairie pour l'algèbre linéaire. Nous devons donc en télécharger
une. [GLM](http://glm.g-truc.net/) est une bonne librairie conçue pour être utilisée avec les APIs graphiques, et est
souvent utilisée avec OpenGL.

GLM est une librairie écrite exclusivement dans les headers, il suffit donc d'en télécharger la
[dernière version](https://github.com/g-truc/glm/releases), la stocker où vous le souhaitez et l'inclure là où vous en
aurez besoin. Vous devrez vous trouver avec quelque chose de semblable :

![](/images/library_directory.png)

### Préparer Visual Studio

Maintenant que vous avez installé toutes les dépendances, nous pouvons préparer un projet Visual Studio pour Vulkan,
et écrire un peu de code pour vérifier que tout fonctionne.

Lancez Visual Studio et créez un nouveau projet "Windows Desktop Wizard", entrez un nom et appuyez sur OK.

![](/images/vs_new_cpp_project.png)

Assurez-vous que "Console Application (.exe)" est séléctionné pour le type d'application afin que nous ayons un endroit
où afficher nos messages d'erreur, et cochez "Empty Project" afin que Visual Studio ne génère pas un code de base.

![](/images/vs_application_settings.png)

Appuyez sur OK pour créer le projet et ajoutez un fichier source C++. Vous devriez déjà savoir faire ça, mais les étapes
sont tout de même incluses ici.

![](/images/vs_new_item.png)

![](/images/vs_new_source_file.png)

Ajoutez maintenant le code suivant à votre fichier. Ne cherchez pas à en comprendre les tenants et aboutissants, il sert
juste à s'assurer que tout compile correctement et qu'une application Vulkan fonctionne. Nous recommencerons tout depuis
le début dès le chapitre suivant.

```c++
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
```

Configurons maintenant le projet afin de se débarrasser des erreurs. Ouvrez le dialogue des propriétés du projet et
assurez-vous que "All Configurations" est sélectionné, car la plupart des paramètres s'appliquent autant à "Debug"
qu'à "Release".

![](/images/vs_open_project_properties.png)

![](/images/vs_all_configs.png)

Allez à "C++" -> "General" -> "Additional Include Directories" et appuyez sur "<Edit...>" dans le menu déroulant.

![](/images/vs_cpp_general.png)

Ajoutez les dossiers pour les headers Vulkan, GLFW et GLM :

![](/images/vs_include_dirs.png)

Ensuite, ouvrez l'éditeur pour les dossiers des librairies sous "Linker" -> "General" :

![](/images/vs_link_settings.png)

Et ajoutez les emplacements des fichiers objets pour Vulkan et GLFW :

![](/images/vs_link_dirs.png)

Allez à "Linker" -> "Input" et appuyez sur "<Edit...>" dans le menu déroulant "Additional Dependencies" :

![](/images/vs_link_input.png)

Entrez les noms des fichiers objets GLFW et Vulkan :

![](/images/vs_dependencies.png)

Vous pouvez enfin fermer le dialogue des propriétés. Si vous avez tout fait correctement vous ne devriez plus voir
d'erreur dans votre code.

Assurez-vous finalement que vous compilez effectivement en 64 bits :

![](/images/vs_build_mode.png)

Appuyez sur F5 pour compiler et lancer le projet. Vous devriez voir une fenêtre s'afficher comme cela :

![](/images/vs_test_window.png)

Si le nombre d'extensions est nul, il y a un problème avec la configuration de Vulkan sur votre système. Sinon, vous
êtes fin prêts à vous [lancer avec Vulkan!](!fr/Dessiner_un_triangle/Mise_en_place/Code_de_base)

## Linux

Ces instructions sont conçues pour les utilisateurs d'Ubuntu et Fedora, mais vous devriez pouvoir suivre ces instructions depuis
une autre distribution si vous adaptez les commandes "apt" ou "dnf" à votre propre gestionnaire de
packages. Il vous faut un compilateur qui supporte C++17 (GCC 7+ ou Clang 5+). Vous aurez également besoin de make.

### Paquets Vulkan

Les composants les plus importants pour le développement d'applications Vulkan sous Linux sont le loader Vulkan, les validation layers et quelques utilitaires pour tester que votre machine est bien en état de faire fonctionner une application Vulkan:
* `sudo apt install vulkan-tools` ou `sudo dnf install vulkan-tools`: Les utilitaires en ligne de commande, plus précisément `vulkaninfo` et `vkcube`. Lancez ceux-ci pour vérifier le bon fonctionnement de votre machine pour Vulkan.
* `sudo apt install libvulkan-dev` ou `sudo dnf install vulkan-headers vulkan-loader-devel`: Installe le loader Vulkan. Il sert à aller chercher les fonctions auprès du driver de votre GPU au runtime, de la même façon que GLEW le fait pour OpenGL - si vous êtes familier avec ceci.
* `sudo apt install vulkan-validationlayers-dev` ou `sudo dnf install mesa-vulkan-devel vulkan-validation-layers-devel`: Installe les layers de validation standards. Ceux-ci sont cruciaux pour débugger vos applications Vulkan, et nous en reparlerons dans un prochain chapitre.

Si l'installation est un succès, vous devriez être prêt pour la partie Vulkan. N'oubliez pas de lancer `vkcube` et assurez-vous de voir la fenêtre suivante:

![](/images/cube_demo_nowindow.png)

### GLFW

Comme dit précédemment, Vulkan ignore la plateforme sur laquelle il opère, et n'inclut pas d'outil de création
de fenêtre où afficher les résultats de notre travail. Pour bien exploiter les possibilités cross-platform de
Vulkan, nous utiliserons la [librairie GLFW](http://www.glfw.org/) pour créer une fenêtre sur Windows, Linux
ou MacOS indifféremment. Il existe d'autres librairies telles que [SDL](https://www.libsdl.org/), mais GLFW à
l'avantage d'abstraire d'autres aspects spécifiques à la plateforme requis par Vulkan.

Nous allons installer GLFW à l'aide de la commande suivante:
```bash
sudo apt install libglfw3-dev
```
ou
```bash
sudo dnf install glfw-devel
```

### GLM

Contrairement à DirectX 12, Vulkan n'intègre pas de librairie pour l'algèbre linéaire. Nous devons donc en télécharger
une. [GLM](http://glm.g-truc.net/) est une bonne librairie conçue pour être utilisée avec les APIs graphiques, et est
souvent utilisée avec OpenGL.

Cette librairie contenue intégralement dans les headers peut être installée depuis le package "libglm-dev" ou 
"glm-devel" :

```bash
sudo apt install libglm-dev
```
ou
```bash
sudo dnf install glm-devel
```

### Compilateur de shader

Nous avons tout ce qu'il nous faut, excepté un programme qui compile le code [GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language) lisible par un humain en bytecode.

Deux compilateurs de shader populaires sont `glslangValidator` de Khronos et `glslc` de Google. Ce dernier a l'avantage d'être proche de GCC et Clang à l'usage,.
Pour cette raison, nous l'utiliserons: Ubuntu, téléchargez les exécutables [non officiels](https://github.com/google/shaderc/blob/main/downloads.md) et copiez `glslc` dans votre répertoire `/usr/local/bin`. Notez que vous aurez certainement besoin d'utiliser `sudo` en fonctions de vos permissions.  Fedora, utilise `sudo dnf install glslc`.
Pour tester, lancez `glslc` depuis le répertoire de votre choix et il devrait se plaindre qu'il n'a reçu aucun shader à compiler de votre part:

`glslc: error: no input files`

Nous couvrirons l'usage de `glslc` plus en détails dans le chapitre des [modules shaders](!fr/03_Dessiner_un_triangle/02_Pipeline_graphique_basique/01_Modules_shaders.md)

### Préparation d'un fichier makefile

Maintenant que vous avez installé toutes les dépendances, nous pouvons préparer un makefile basique pour Vulkan et
écrire un code très simple pour s'assurer que tout fonctionne correctement.

Ajoutez maintenant le code suivant à votre fichier. Ne cherchez pas à en comprendre les tenants et aboutissants, il sert
juste à s'assurer que tout compile correctement et qu'une application Vulkan fonctionne. Nous recommencerons tout depuis
le début dès le chapitre suivant.

```c++
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
```

Nous allons maintenant créer un makefile pour compiler et lancer ce code. Créez un fichier "Makefile". Je pars du
principe que vous connaissez déjà les bases de makefile, dont les variables et les règles. Sinon vous pouvez trouver des
introductions claires sur internet, par exemple [ici](https://makefiletutorial.com/).

Nous allons d'abord définir quelques variables pour simplifier le reste du fichier.
Définissez `CFLAGS`, qui spécifiera les arguments pour la compilation :

```make
CFLAGS = -std=c++17 -O2
```

Nous utiliserons du C++ moderne (`-std=c++17`) et compilerons avec le paramètre d'optimisation `-O2`. Vous pouvez le retirer pour compiler nos programmes plus rapidement, mais n'oubliez pas de le remettre pour compiler des exécutables prêts à être distribués.

Définissez de manière analogue `LDFLAGS` :

```make
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
```

Le premier flag correspond à GLFW, `-lvulkan` correspond au loader dynamique des fonctions Vulkan. Le reste des options correspondent à des bibliothèques systèmes liés à la gestion des fenêtres et aux threads nécessaire pour le bon fonctionnement de GLFW.

Spécifier les commandes pour la compilation de "VulkanTest" est désormais un jeu d'enfant. Assurez-vous que vous
utilisez des tabulations et non des espaces pour l'indentation.

```make
VulkanTest: main.cpp
    g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS)
```

Vérifiez que le fichier fonctionne en le sauvegardant et en exécutant make depuis un terminal ouvert dans le
dossier le contenant. Vous devriez avoir un exécutable appelé "VulkanTest".

Nous allons ensuite définir deux règles, `test` et `clean`. La première exécutera le programme et le second supprimera
l'exécutable :

```make
.PHONY: test clean

test: VulkanTest
    ./VulkanTest

clean:
    rm -f VulkanTest
```

Lancer `make test` doit vous afficher le programme sans erreur, listant le nombre d'extensions disponible pour Vulkan.
L'application devrait retourner le code de retour 0 (succès) quand vous fermez la fenêtre vide.
Vous devriez désormais avoir un makefile ressemblant à ceci :

```make
CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: main.cpp
    g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
    ./VulkanTest

clean:
    rm -f VulkanTest
```

Vous pouvez désormais utiliser ce dossier comme exemple pour vos futurs projets Vulkan.
Faites-en une copie, changez le nom du projet pour quelque chose comme `HelloTriangle` et retirez tout le code contenu dans `main.cpp`.

Bravo, vous êtes fin prêts à vous [lancer avec Vulkan!](!fr/Dessiner_un_triangle/Mise_en_place/Code_de_base)

## MacOS

Ces instructions partent du principe que vous utilisez Xcode et le
[gestionnaire de packages Homebrew](https://brew.sh/). Vous aurez besoin de MacOS 10.11 minimum, et votre ordinateur
doit supporter l'[API Metal](https://en.wikipedia.org/wiki/Metal_(API)#Supported_GPUs).

### Le SDK Vulkan

Le SDK est le composant le plus important pour programmer une application avec Vulkan. Il inclue headers, validations
layers, outils de débogage et un loader dynamique pour les fonctions Vulkan. Le loader cherche les fonctions dans le
driver pendant l'exécution, comme GLEW pour OpenGL, si cela vous parle.

Le SDK se télécharge sur le [site de LunarG](https://vulkan.lunarg.com/) en utilisant les boutons en bas de page. Vous
n'avez pas besoin de créer de compte, mais il permet d'accéder à une documentation supplémentaire qui pourra vous être
utile.

![](/images/vulkan_sdk_download_buttons.png)

La version MacOS du SDK utilise [MoltenVK](https://moltengl.com/). Il n'y a pas de support natif pour Vulkan sur MacOS,
donc nous avons besoin de MoltenVK pour transcrire les appels à l'API Vulkan en appels au framework Metal d'Apple.
Vous pouvez ainsi exploiter pleinement les possibilités de cet API automatiquement.


Une fois téléchargé, extrayez-en le contenu où vous le souhaitez. Dans le dossier extrait, il devrait y avoir un
sous-dossier "Applications" comportant des exécutables lançant des démos du SDK. Lancez "vkcube" pour vérifier que vous
obtenez ceci :

![](/images/cube_demo_mac.png)

### GLFW

Comme dit précédemment, Vulkan ignore la plateforme sur laquelle il opère, et n'inclut pas d'outil de création
de fenêtre où afficher les résultats de notre travail. Pour bien exploiter les possibilités cross-platform de
Vulkan, nous utiliserons la [librairie GLFW](http://www.glfw.org/) pour créer une fenêtre qui supportera Windows, Linux
et MacOS. Il existe d'autres librairies telles que [SDL](https://www.libsdl.org/), mais GLFW à l'avantage d'abstraire
d'autres aspects spécifiques à la plateforme requis par Vulkan.

Nous utiliserons le gestionnaire de package Homebrew pour installer GLFW. Le support Vulkan sur MacOS n'étant pas
parfaitement disponible (à l'écriture du moins) sur la version 3.2.1, nous installerons le package "glfw3" ainsi :

```bash
brew install glfw3 --HEAD
```

### GLM

Vulkan n'inclut aucune libraire pour l'algèbre linéaire, nous devons donc en télécharger une.
[GLM](http://glm.g-truc.net/) est une bonne librairie souvent utilisée avec les APIs graphiques dont OpenGL.

Cette librairie est intégralement codée dans les headers et se télécharge avec le package "glm" :

```bash
brew install glm
```

### Préparation de Xcode

Maintenant que nous avons toutes les dépendances nous pouvons créer dans Xcode un projet Vulkan basique. La plupart
des opérations seront de la "tuyauterie" pour lier les dépendances au projet. Notez que vous devrez remplacer toutes les
mentions "vulkansdk" par le dossier où vous avez extrait le SDK Vulkan.

Lancez Xcode et créez un nouveau projet. Sur la fenêtre qui s'ouvre sélectionnez Application > Command Line Tool.

![](/images/xcode_new_project.png)

Sélectionnez "Next", inscrivez un nom de projet et choisissez "C++" pour "Language".

![](/images/xcode_new_project_2.png)

Appuyez sur "Next" et le projet devrait être créé. Copiez le code suivant à la place du code généré dans le fichier
"main.cpp" :

```c++
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
```

Gardez à l'esprit que vous n'avez pas à comprendre tout ce que le code fait, dans la mesure où il se contente
d'appeler quelques fonctions de l'API pour s'assurer que tout fonctionne. Nous verrons toutes ces fonctions en détail
plus tard.

Xcode devrait déjà vous afficher des erreurs comme le fait que des librairies soient introuvables. Nous allons
maintenant les faire disparaître. Sélectionnez votre projet sur le menu *Project Navigator*. Ouvrez 
*Build Settings* puis :

* Trouvez le champ **Header Search Paths** et ajoutez "/usr/local/include" (c'est ici que Homebrew installe les headers)
et "vulkansdk/macOS/include" pour le SDK.
* Trouvez le champ **Library Search Paths** et ajoutez "/usr/local/lib" (même raison pour les librairies) et
"vulkansdk/macOS/lib".

Vous avez normalement (avec des différences évidentes selon l'endroit où vous avez placé votre SDK) :

![](/images/xcode_paths.png)

Maintenant, dans le menu *Build Phases*, ajoutez les frameworks "glfw3" et "vulkan" dans **Link Binary With
Librairies**. Pour nous simplifier les choses, nous allons ajouter les librairies dynamiques directement dans le projet
(référez-vous à la documentation de ces librairies si vous voulez les lier de manière statique).

* Pour glfw ouvrez le dossier "/usr/local/lib" où vous trouverez un fichier avec un nom comme "libglfw.3.x.dylib" où x
est le numéro de la version. Glissez ce fichier jusqu'à la barre des "Linked Frameworks and Librairies" dans Xcode.
* Pour Vulkan, rendez-vous dans "vulkansdk/macOS/lib" et répétez l'opération pour "libvulkan.1.dylib" et "libvulkan.1.x.xx
.dylib" avec les x correspondant à la version du SDK que vous avez téléchargé.

Maintenant que vous avez ajouté ces librairies, remplissez le champ `Destination` avec "Frameworks" dans **Copy Files**,
supprimez le sous-chemin et décochez "Copy only when installing". Cliquez sur le "+" et ajoutez-y les trois mêmes
frameworks.

Votre configuration Xcode devrait ressembler à cela :

![](/images/xcode_frameworks.png)

Il ne reste plus qu'à définir quelques variables d'environnement. Sur la barre d'outils de Xcode allez à `Product` >
`Scheme` > `Edit Scheme...`, et dans la liste `Arguments` ajoutez les deux variables suivantes :

* VK_ICD_FILENAMES = `vulkansdk/macOS/share/vulkan/icd.d/MoltenVK_icd.json`
* VK_LAYER_PATH = `vulkansdk/macOS/share/vulkan/explicit_layer.d`

Vous avez normalement ceci :

![](/images/xcode_variables.png)

Vous êtes maintenant prêts! Si vous lancez le projet (en pensant à bien choisir Debug ou Release) vous devrez
avoir ceci :

![](/images/xcode_output.png)

Si vous obtenez `0 extensions supported`, il y a un problème avec la configuration de Vulkan sur votre système. Les
autres données proviennent de librairies, et dépendent de votre configuration.

Vous êtes maintenant prêts à vous [lancer avec Vulkan!](!fr/Dessiner_un_triangle/Mise_en_place/Code_de_base).
