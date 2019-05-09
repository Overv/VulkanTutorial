Dans ce chapitre nous allons paramétrer votre environnement de développement pour Vulkan et installer des librairies
utiles. Tous les outils que nous allons utiliser, excepté le compilateur, seront compatibles Windows, Linux et MacOS.
Cependant les étapes pour les installer diffèrent un peu, d'où les sections suivantes.

## Windows

Si vous développez pour Windows, je partirai du principe que vous utilisez Visual Studio 2017 pour ce projet. Vous
pouvez aussi utiliser VS 2013 ou 2015, mais les étapes sont un peu différentes.

### SDK Vulkan

Le composant central du développement d'applications Vulkan est le SDK. Il inclut les headers, les validation layers
standards, des outils de débogage et un loader pour les fonctions Vulkan. Ce loader récupère les fonctions dans le
driver à l'exécution, comme GLEW pour OpenGL - si vous connaissez.

Le SDK peut être téléchargé sur [le site de LunarG](https://vulkan.lunarg.com/) en utilisant les boutons en bas de page.
Vous n'avez pas besoin de compte, mais celui-ci vous donne accès à une documentation supplémentaire qui pourra vous être
utile.

![](/images/vulkan_sdk_download_buttons.png)

Réalisez l'installation et notez l'emplacement du SDK. La première chose que nous allons faire est vérifier que votre
carte graphique supporte Vulkan. Allez dans le dossier d'installation du SDK, ouvrez le dossier "Bin" et lancez
"cube.exe". Vous devriez voire la fenêtre suivante :

![](/images/cube_demo.png)

Si vous recevez un message d'erreur assurez-vous que votre driver est à jour, inclut Vulkan et que votre carte graphique
est supportée. Référez-vous au [chapitre introductif](!Introduction) pour les liens vers les principaux constructeurs.

Il y a un autre programme dans ce dossier qui vous sera utile : "glslangValidator.exe". Il nous sera utile lors de la
compilation des shaders. Il transforme un code lisible par l'homme écrit en [GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language) en byte code.
Nous couvrirons cela dans le chapitre des [shader modules](!Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules).
Le dossier "Bin" contient aussi les fichiers binaires du loader Vulkan et des validation layers. Le dossier "Lib" en
contient les librairies.

Le dossier "Doc" contient des informations pouvant vous être utiles sur le SDK et une version offline de la
spécification de Vulkan. Enfin, le dossier "Include" contient les headers Vulkan. Vous pouvez parourir les autres
fichiers, mais nous ne les utiliserons pas dans ce tutoriel.

### GLFW

Comme dit précédemment, Vulkan ignore la plateforme sur laquelle il opère, et n'inclut pas d'outil de création
de fenêtre pour y afficher les résultats de notre travail. Pour bien exploiter les possibilités cross-platform de Vulkan
et éviter les horreurs de Win32, nous utiliserons la [librairie GLFW](http://www.glfw.org/) pour créer une fenêtre qui
supportera Windows, Linux et MacOS. Il existe d'autres librairies telles que [SDL](https://www.libsdl.org/), mais GLFW a
l'avantage d'abstraire d'autres aspects spécifiques à la plateforme requis par Vulkan.

Vous pouvez trouver la dernière version de GLFW sur leur site officiel. Nous utiliserons la version 64 bits, mais vous
pouvez également utiliser la version 32 bits. Dans ce cas assurez-vous de bien lier le dossier "Lib32" dans le SDK et
non "Lib". Après avoir téléchargé GLFW, extrayez l'archive à l'emlacement qui vous convient. J'ai choisi de créer un
dossier "Librairies" dans le dossier de Visual Studio. Il est normal qu'il n'y ait pas de dossier "libvc-2017", le
dossier "libvc-2015" est toujours compatible.

![](/images/glfw_directory.png)

### GLM

Contrairement à DirectX 12, Vulkan n'intègre pas de librairie pour l'algèbre linéaire. Nous devons donc en télécharger
une. [GLM](http://glm.g-truc.net/) est une bonne librairie conçue pour être utilisée avec les APIs graphiques, et est
souvent utilisée avec OpenGL.

GLM est une librairie écrite exclusivement dans les headers, il suffit donc de télécharger la
[dernière version](https://github.com/g-truc/glm/releases), la stocker où vous le souhaitez et l'inclure là où vous en
aurez besoin. Vous devrez vous trouver avec quelque chose de semblable :

![](/images/library_directory.png)

### Préparer Visual Studio

Maintenant que vous avez installé toutes les dépendances, nous pouvons paramétrer un projet Visual Studio pour Vulkan,
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

    std::cout << extensionCount << " extensions supported" << std::endl;

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

Configurons maintenant le projet afin de se débarasser des erreurs. Ouvrez le dialogue des propriétés du projet et
assurez-vous que "All Configurations" est sélectionné, car la plupart des paramètres s'appliquent autant à "Debug"
qu'à "Release".

![](/images/vs_open_project_properties.png)

![](/images/vs_all_configs.png)

Allez à "C++" -> "General" -> "Additional Include Directories" et appuyer sur "<Edit...>" dans la boite déroulante.

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

Vous pouvez enfin fermer le dialogue de propriétés. Si vous avez tout fait correctement vous ne devriez plus voir
d'erreur dans votre code.

Assurez-vous finalement que vous compilez effectivement en 64 bits :

![](/images/vs_build_mode.png)

Appuyez sur F5 pour compiler et lancer le projet. Vous devriez voir une fenêtre s'afficher comme cela :

![](/images/vs_test_window.png)

Le nombre d'extensions ne devrait pas être nul. Bravo, vous êtes fin prêts à
[jouer avec Vulkan!](!Drawing_a_triangle/Setup/Base_code)

## Linux

Ces instructions sont conçues pour les utilisateurs d'Ubuntu, mais vous devriez pouvoir suivre ces instructions depuis
une autre distribution si vous recompilez vous-même le SDK et adaptez les commandes "apt" à votre propre gestionnaire de
packages. Installez si ne n'est pas déjà fait gcc 4.8 ou plus récent. Vous aurez égamelent besoin de make et de CMake.

### Le SDK Vulkan

Le composant central au développement d'application Vulkan est le SDK. Il inclut les headers, les validation layers
standards, des outils de débogage et un loader pour les fonctions Vulkan. Ce loader récupère les fonctions dans le
driver à l'exécution, comme GLEW pour OpenGL - si vous connaissez.

Le SDK peut être téléchargé sur [le site LunarG](https://vulkan.lunarg.com/) en utilisant les boutons en bas de page.
Vous n'avez pas besoin de compte, mais celui-ci vous donne accès à une documentation supplémentaire qui pourra vous être
utile.

![](/images/vulkan_sdk_download_buttons.png)

Ouvrez un terminal dans le dossier où vous avez téléchargé l'archive en ".tar.gz" et extrayez-là :

```bash
tar -xzf vulkansdk-linux-x86_64-xxx.tar.gz
```

Cela extraira tous les fichiers dans un sous-dossier dont le nom est celui de la version du SDK. Placez le dossier où
vous le désirez. Ouvrez-y un terminal. Vous devriez être là où sont présents les fichier comme "build_examples.sh".

Les exemples dans le SDK et l'une des libraries dont nous aurons besoin dépendent de la librairie XCB. C'est une
librairie écrite en C utilisée pour communiquer avec le gestionnaire de fenêtres X. Elle s'installe à l'aide du package
"libxcb1-dev" sous Ubuntu. Vous aurez également besoin de fichiers de développement X présents dans le package
"xorg-dev".

```bash
sudo apt install libxcb1-dev xorg-dev
```

Compilez les exemples Vulkan ainsi :

```bash
./build_examples.sh
```

Si la compilation a été un succès, vous avez normalement un exécutable "./examples/build/cube". Lancez-le depuis le
dossier "examples/build" avec la commande "./cube" et assurez-vous que vous obtenez la fenêtre suivante :

![](/images/cube_demo_nowindow.png)

Si vous recevez un message d'erreur, assurez-vous que vos drivers sont à jour, incluent Vulkan et que votre carte
graphique est supportée. Référez-vous au [chapitre introductif](!Introduction) pour les liens aux vendeurs principaux.

### GLFW

Comme dit précédemment, Vulkan ignore la plateforme sur laquelle il opère, et n'inclut pas d'outil de création
de fenêtre pour y afficher les résultats de notre travail. Pour bien exploiter les possibilités cross-platform de
Vulkan, nous utiliserons la [librairie GLFW](http://www.glfw.org/) pour créer une fenêtre qui supportera Windows, Linux
et MacOS. Il existe d'autres librairies telles que [SDL](https://www.libsdl.org/), mais GLFW à l'avantage d'abstraire
d'autres aspects spécifiques à la plateforme requis par Vulkan.

Nous allons installer GLFW à partir des sources, car Vulkan nécessite une version récente. Vous pouvez trouvez ces
sources sur le [site officiel](http://www.glfw.org/). Extrayez les sources où vous voulez et ouvrez un terminal dans le
dossier extrait, où se trouve le fichier "CmakeLists.txt".

Exécutez les commandes suivantes afin de générer un makefile et de compiler GLFW :

```bash
cmake .
make
```

Si vous rencontrez le message "Could NOT find Vulkan", ignorez-le. Si la compilation a fonctionné, vous pouvez installer
GLFW sur votre système en exécutant :

```bash
sudo make install
```

### GLM

Contrairement à DirectX 12, Vulkan n'intègre pas de librairie pour l'algèbre linéaire. Nous devons donc en télécharger
une. [GLM](http://glm.g-truc.net/) est une bonne librairie conçue pour être utilisée avec les APIs graphiques, et est
souvent utilisée avec OpenGL.

Cette librairie contenue intégralement dans les headers peut être installée depuis le package "libglm-dev" :

```bash
sudo apt install libglm-dev
```

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

    std::cout << extensionCount << " extensions supported" << std::endl;

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

Nous allons maintenant créer un makefile pour compiler et lancer ce code. Créez un fichier "makefile". Je pars du
principe que vous connaissez déjà les bases de makefile, dont les variables et les règles. Sinon vous pouvez trouver des
introductions claires sur internet, par exemple [ici](http://mrbook.org/blog/tutorials/make/).

Nous allons d'abord définir quelques variables pour simplifier le reste du fichier. Définissez `VULKAN_SDK_PATH` se
référant à l'emplacement du dossier "x86_64" dans le SDK, par exemple :

```make
VULKAN_SDK_PATH = /home/user/VulkanSDK/x.x.x.x/x86_64
```

Remplacez bien "user" par votre nom d'utilisateur et "x.x.x.x" par la bonne version. Définissez ensuite `CFLAGS`, qui
spécifiera les arguments pour la compilation :

```make
CFLAGS = -std=c++17 -I$(VULKAN_SDK_PATH)/include
```

Nous utiliserons du C++ moderne (`-std=c++17`). Nous devons également localiser "vulkan.h" dans le SDK de
LunarG.

Définissez de manière analogue `LDFLAGS` :

```make
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan
```

Le premier paramètre permet de localiser les librairies comme "libvulkan.so" dans le dossier "x86_64/lib" du SDK. La
seconde partie utilise `pkg-config` pour trouver les pramètres nécessaires aux linker pour compiler avec GLFW. Enfin,
`-lvulkan` correspond au loader dynamique des fonctions Vulkan présent dans le SDK.

Spécifier les commandes pour la compilation de "VulkanTest" est désormais un jeu d'enfant. Assurez-vous que vous
utilisez des tabulations et non des espaces pour l'indentation.

```make
VulkanTest: main.cpp
    g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS)
```

Vérifiez que le fichier fonctionne en le sauveguardant et en exécutant make depuis un terminal ouvert dans dans le
dossier le contenant. Vous devriez avoir un exécutable appelé "VulkanTest".

Nous allons ensuite définir deux règles, `test` et `clean`. Le premier exécutera le programme et le second supprimera
l'exécutable :

```make
.PHONY: test clean

test: VulkanTest
    ./VulkanTest

clean:
    rm -f VulkanTest
```

Vous devriez constater que `make clean` fonctionne mais que `make test` s'interrompera avec le message suivant :

```text
./VulkanTest: error while loading shared libraries: libvulkan.so.1: cannot open shared object file: No such file or directory
```

En effet "libvulkan.so" n'est pas installé comme une librairie système. Pour que le linker la trouve, nous devons lui
spécifier explicitement la localisation de la librairie avec une variable :

```make
test: VulkanTest
    LD_LIBRARY_PATH=$(VULKAN_SDK_PATH)/lib ./VulkanTest
```

Le programme devrait se lancer correctement maintenant, et afficher le nombre d'extensions disponibles. L'application
devrait quitter avec le code 0 lorsque vous fermez la fenêtre. Il y a pourtant encore une variable que nous devons
créer. Nous allons utiliser les validation layers et devons donc lui indiquer leur localisation avec la variable
`VK_LAYER_PATH` :

```make
test: VulkanTest
    LD_LIBRARY_PATH=$(VULKAN_SDK_PATH)/lib VK_LAYER_PATH=$(VULKAN_SDK_PATH)/etc/explicit_layer.d ./VulkanTest
```

Vous devriez désormais avoir un makefile complet tel que celui-ci :

```make
VULKAN_SDK_PATH = /home/user/VulkanSDK/x.x.x.x/x86_64

CFLAGS = -std=c++17 -I$(VULKAN_SDK_PATH)/include
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan

VulkanTest: main.cpp
    g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
    LD_LIBRARY_PATH=$(VULKAN_SDK_PATH)/lib VK_LAYER_PATH=$(VULKAN_SDK_PATH)/etc/explicit_layer.d ./VulkanTest

clean:
    rm -f VulkanTest
```

Vous pouvez désormais utiliser ce dossier comme exemple pour vos projets Vulkan futurs. Faites en une copie, changer le
nom du projet et tout est prêt!

Avant d'avancer, explorons le SDK un peu plus. Il y a un autre programme dans ce dossier qui vous sera utile :
"glslangValidator". Il nous sera utile lors de compilation des shaders. Il transforme un code lisible par l'homme
écrit en [GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language) en byte code. Nous couvrirons cela dans le
chapitre [modeules shader](!Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules).


Le dossier "Doc" contient des informations pouvant vous être utiles sur le SDK et une version offline de la
spécification de Vulkan. Enfin, le dossier "Include" contient les headers Vulkan. Vous pouvez parourir les autres
fichiers, mais nous ne les utiliserons pas dans ce tutoriel.

Bravo, vous êtes fin prêts à [jouer avec Vulkan!](!Drawing_a_triangle/Setup/Base_code)

## MacOS

Ces instructions partent du principe que vous utilisez Xcode et le [gestionnaire de packages Homebrew](https://brew.sh/)
. Vous aurez besoin de MacOS 10.11 minimum, et votre ordinateur doit supporter
l'[API Metal](https://en.wikipedia.org/wiki/Metal_(API)#Supported_GPUs).

### Le SDK Vulkan

Le SDK est le composant le plus important pour programmer une application avec Vulkan. Il inclue headers, validations
layers, outils de débogage et un loader dynamique pour les fonctions Vulkan. Le loader cherche les fonctions dans le
driver pendant l'exécution, comme GLEW pour OpenGL - si vous connaissez.

Le SDK se télécharge sur le [site de LunarG](https://vulkan.lunarg.com/) en utilisant les boutons en bas de page. Vous
n'avez pas besoin de dréer de compte, mais il permet d'accéder à une documentation supplémentaire qui pourra vous être
utile.

![](/images/vulkan_sdk_download_buttons.png)

La version MacOS du SDK utilise [MoltenVK](https://moltengl.com/). Il n'y a pas de support natif pour Vulkan sur MacOS,
donc nous avons besoin de MoltenVK pour transcrire les appels à l'API Vulkan en appels au framework Metal d'Apple.
Vous pouvez ainsi exploiter pleinement les possibilités de cet API automatiquement.


Une fois téléchargé, extrayez le contenu où vous le souhaitez. Dans le dossier extrait, il devrait y avoir un
sous-dossier "Applications" comportant des exécutables lançant des démos du SDK. Lancez "cube" pour vérifier que vous
obtenez ceci :

![](/images/cube_demo_mac.png)

### GLFW

Comme dit précédemment, Vulkan ignore la plateforme sur laquelle il opère, et n'inclut pas d'outil de création
de fenêtre pour y afficher les résultats de notre travail. Pour bien exploiter les possibilités cross-platform de
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
[GLM](http://glm.g-truc.net/) est une bonne librairie souvent utilisée avec les APIs graphiques et particulièrement
OpenGL.

Cette librairie est intégralement codée dans les headers et se télécharge avec le package "glm" :

```bash
brew install glm
```

### Préparation de Xcode

Maintenant que nous avons toutes les dépendances nous pouvons créer un projet basique pour Vulkan dans Xcode. La plupart
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

    std::cout << extensionCount << " extensions supported" << std::endl;

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
>
Gardez à l'esprit que vous n'avez pas à comprendre tout ce que le code fait, dans la mesure où il se content de réaliser
quelques appels à l'API pour s'assurer que tout fonctionne.

Xcode devrait déjà vous afficher des erreurs comme le fait que des librairies soient introuvables. Nous allons
maintenant faire en sorte que ces erreurs disparaissent. Séletionnez votre projet sur le menu *Project Navigator*. Ouvrez
 *Build Settings* puis :

* Trouvez le champ **Header Search Paths** et ajoutez "/usr/local/include" (c'est ici que Homebrew installe les headers)
 et "vulkansdk/macOS/include" pour le SDK.
* Trouvez le champ **Library Search Paths** et ajoutez "/usr/local/lib" (même raison pour les librairies) et
"vulkansdk/macOS/lib".

Vous avez normalement (avec des différences évidantes selon l'endroit où vous avez placé votre SDK) :

![](/images/xcode_paths.png)

Maintenant, dans le menu *Build Phases*, ajoutez les frameworks "glfw3" et "vulkan" dans **Link Binary With Librairies**
. Pour nous simplifier les choses, nous allons ajouter les librairies dynamiques directement dans le projet
(référez-vous à la documentation de ces librairies si vous voulez les lier de manière statique).

* Pour glfw ouvrez le dossier "/usr/local/lib" où vous trouverez un fichier avec un nom comme "libglfw.3.x.dylib" où x
est le numéro de la version. Glissez ce fichier jusqu'à la barre des "Linked Frameworks and librairies" dans Xcode.
* Pour Vulkan, rendez-vous dans "vulkansdk/macOS/lib" et répétez l'opération pour "libvulkan.1.dylib" et "libvulkan.1.x.xx
.dylib" avec les x correspondant à la version du SDK que vous avez téléchargé.

Maintenant que vous avez ajouté ces librairies, remplissez le champ `Destination` avec "Frameworks" dans **Copy Files**,
 supprimez le sous-chemin et décochez "Copy only when installing". Cliquez sur le "+" et ajoutez-y les trois mêmes
 frameworks.

Votre configuration Xcode devrait ressembler à cela :

![](/images/xcode_frameworks.png)

Il ne reste plus qu'à définir quelques variables d'environnement. Sur la barre d'outils de Xcode allez à `Product` >
`Scheme` > `Edit Scheme...`, et dans la liste `Arguments` ajoutez les deux variables suivantes :

* VK_ICD_FILENAMES = `vulkansdk/macOS/etc/vulkan/icd.d/MoltenVK_icd.json`
* VK_LAYER_PATH = `vulkansdk/macOS/etc/vulkan/explicit_layer.d`

Vous avez normalement ceci :

![](/images/xcode_variables.png)

Vous êtes maintenant prêts! Si vous lancez le projet (en pensant à bien choisir entre Debug et Release) vous devrez
avoir ceci :

![](/images/xcode_output.png)

Le nombre d'extensions ne doit pas être nul. Les autres données proviennet de librairies, et dépendent de votre
configuration.

Vous êtes maintenant prêts pour [la partie intéressante!](!Drawing_a_triangle/Setup/Base_code).
