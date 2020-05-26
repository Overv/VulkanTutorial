Ce chapitre commencera par introduire Vulkan et les problèmes auxquels l'API s’adresse. Nous nous intéresserons ensuite aux
éléments requis pour l'affichage d'un premier triangle. Cela vous donnera une vue d'ensemble pour mieux replacer les
futurs chapitres dans leur contexte. Nous conclurons sur la structure de Vulkan et la manière dont l'API est communément
utilisée.

## Origine de Vulkan

Comme les APIs précédentes, Vulkan est conçue comme une abstraction des
[GPUs](https://en.wikipedia.org/wiki/Graphics_processing_unit). Le problème avec la plupart de ces APIs est qu'elles
furent créées à une époque où le hardware graphique était limité à des fonctionnalités prédéfinies tout juste
configurables. Les développeurs devaient fournir les sommets dans un format standardisé, et étaient ainsi à la merci
des constructeurs pour les options d'éclairage et les jeux d'ombre.

Au fur et à mesure que les cartes graphiques progressèrent, elles offrirent de plus en plus de fonctionnalités
programmables. Il fallait alors intégrer toutes ces nouvelles fonctionnalités aux APIs existantes. Ceci résulta
en une abstraction peu pratique et le driver devait deviner l'intention du développeur pour relier le programme aux
architectures modernes. C'est pour cela que les drivers étaient mis à jour si souvent, et que certaines augmentaient
soudainement les performances. À cause de la complexité de ces drivers, les développeurs devaient gérer les
différences de comportement entre les fabricants, dont par exemple des tolérances plus ou moins importantes pour les
[shaders](https://en.wikipedia.org/wiki/Shader). Un exemple de fonctionnalité est le
[tiled rendering](https://en.wikipedia.org/wiki/Tiled_rendering), pour laquelle une plus grande flexibilité mènerait à
de meilleures performance. Ces APIs anciennes souffrent également d’une autre limitation : le support limité du
multithreading, menant à des goulot d'étranglement du coté du CPU. Au-delà des nouveautés techniques, la dernière
décennie a aussi été témoin de l’arrivée de matériel mobile. Ces GPUs portables ont des architectures différentes qui
prennent en compte des contraintes spatiales ou énergétiques.

Vulkan résout ces problèmes en ayant été repensée à partir de rien pour des architectures modernes. Elle réduit le
travail du driver en permettant (en fait en demandant) au développeur d’expliciter ses objectifs en passant par une
API plus prolixe. Elle permet à plusieurs threads d’invoquer des commandes de manière asynchrone. Elle supprime les
différences lors de la compilation des shaders en imposant un format en bytecode compilé par un compilateur officiel.
Enfin, elle reconnaît les capacités des cartes graphiques modernes en unifiant le computing et les graphismes dans 
une seule et unique API.

## Le nécessaire pour afficher un triangle

Nous allons maintenant nous intéresser aux étapes nécessaires à l’affichage d’un triangle dans un programme Vulkan
correctement conçu. Tous les concepts ici évoqués seront développés dans les prochains chapitres. Le but ici est
simplement de vous donner une vue d’ensemble afin d’y replacer tous les éléments.

### Étape 1 - Instance et sélection d’un physical device

Une application commence par paramétrer l’API à l’aide d’une «`VkInstance`». Une instance est créée en décrivant votre
application et les extensions que vous comptez utiliser. Après avoir créé votre `VkInstance`, vous pouvez demander l’accès
à du hardware compatible avec Vulkan, et ainsi sélectionner un ou plusieurs «`VkPhysicalDevice`» pour y réaliser vos
opérations. Vous pouvez traiter des informations telles que la taille de la VRAM ou des capacités de la carte graphique,
et ainsi préférer par exemple du matériel dédié.

### Étape 2 – Logical device et familles de queues (queue families)

Après avoir sélectionné le hardware qui vous convient, vous devez créer un `VkDevice` (logical device). Vous décrivez
pour cela quelles `VkPhysicalDeviceFeatures` vous utiliserez, comme l’affichage multi-fenêtre ou des floats de 64 bits.
Vous devrez également spécifier quelles `vkQueueFamilies` vous utiliserez. La plupart des opérations, comme les
commandes d’affichage et les allocations de mémoire, sont exécutés de manière asynchrone en les envoyant à une
`VkQueue`. Ces queues sont crées à partir d’une famille de queues, chacune de ces dernières supportant uniquement une
certaine collection d’opérations. Il pourrait par exemple y avoir des familles différentes pour les graphismes, le
calcul et les opérations mémoire. L’existence d’une famille peut aussi être un critère pour la sélection d’un physical
device. En effet une queue capable de traiter les commandes graphiques et opérations mémoire permet d'augmenter
encore un peu les performances. Il sera possible qu’un périphérique supportant Vulkan ne fournisse aucun graphisme,
mais à ce jour toutes les opérations que nous allons utiliser devraient être disponibles.

### Étape 3 – Surface d’affichage (window surface) et swap chain

À moins que vous ne soyez intéressé que par le rendu off-screen, vous devrez créer une fenêtre dans laquelle afficher
les éléments. Les fenêtres peuvent être crées avec les APIs spécifiques aux différentes plateformes ou avec des
librairies telles que [GLFW](http://www.glfw.org/) et [SDL](https://www.libsdl.org/). Nous utiliserons GLFW dans ce
tutoriel, mais nous verrons tout cela dans le prochain chapitre.

Nous avons cependant encore deux composants à évoquer pour afficher quelque chose : une Surface (`VkSurfaceKHR`) et une
Swap Chain (`VkSwapchainKHR`). Remarquez le suffixe «KHR», qui indique que ces fonctionnalités font partie d’une
extension. L'API est elle-même totalement agnostique de la plateforme sur laquelle elle travaille, nous devons donc
utiliser l’extension standard WSI (Window System Interface) pour interagir avec le gestionnaire de fenêtre. La
Surface est une abstraction cross-platform de la fenêtre, et est généralement créée en fournissant une référence à
une fenêtre spécifique à la plateforme, par exemple «HWND» sur Windows. Heureusement pour nous, la librairie GLFW
possède une fonction permettant de gérer tous les détails spécifiques à la plateforme pour nous.

La swap chain est une collection de cibles sur lesquelles nous pouvons effectuer un rendu. Son but principal est
d’assurer que l’image sur laquelle nous travaillons n’est pas celle utilisée par l’écran. Nous sommes ainsi sûrs que
l’image affichée est complète. Chaque fois que nous voudrons afficher une image nous devrons demander à la swap chain de
nous fournir une cible disponible. Une fois le traitement de la cible terminé, nous la rendrons à la swap chain qui
l’utilisera en temps voulu pour l’affichage à l’écran. Le nombre de cibles et les conditions de leur affichage dépend
du mode utilisé lors du paramétrage de la Swap Chain. Ceux-ci peuvent être le double buffering (synchronisation
verticale) ou le triple buffering. Nous détaillerons tout cela dans le chapitre dédié à la Swap Chain.

Certaines plateformes permettent d'effectuer un rendu directement à l'écran sans passer par un gestionnaire de fenêtre,
et ce en vous donnant la possibilité de créer une surface qui fait la taille de l'écran. Vous pouvez alors par exemple
créer votre propre gestionnaire de fenêtre.

### Étape 4 - Image views et framebuffers

Pour dessiner sur une image originaire de la swap chain, nous devons l'encapsuler dans une `VkImageView` et un
`VkFramebuffer`. Une vue sur une image correspond à une certaine partie de l’image utilisée, et un framebuffer
référence plusieurs vues pour les traiter comme des cible de couleur, de profondeur ou de stencil. Dans la mesure où
il peut y avoir de nombreuses images dans la swap chain, nous créerons en amont les vues et les framebuffers pour
chacune d’entre elles, puis sélectionnerons celle qui nous convient au moment de l’affichage.

### Étape 5 - Render passes

Avec Vulkan, une render pass décrit les types d’images utilisées lors du rendu, comment elles sont utilisées et
comment leur contenu doit être traité. Pour notre affichage d’un triangle, nous dirons à Vulkan que nous utilisons une
seule image pour la couleur et que nous voulons qu’elle soit préparée avant l’affichage en la remplissant d’une couleur
opaque. Là où la passe décrit le type d’images utilisées, un framebuffer sert à lier les emplacements utilisés par la
passe à une image complète.

### Étape 6 - Le pipeline graphique

Le pipeline graphique est configuré lors de la création d’un `VkPipeline`. Il décrit les éléments paramétrables de la
carte graphique, comme les opérations réalisées par le depth buffer (gestion de la profondeur), et les étapes
programmables à l’aide de `VkShaderModules`. Ces derniers sont créés à partir de byte code. Le driver doit également
être informé des cibles du rendu utilisées dans le pipeline, ce que nous lui disons en référençant la render pass.

L’une des particularités les plus importantes de Vulkan est que la quasi totalité de la configuration des étapes doit
être réalisée à l’avance. Cela implique que si vous voulez changer un shader ou la conformation des sommets, la
totalité du pipeline doit être recréée. Vous aurez donc probablement de nombreux `VkPipeline` correspondant à toutes
les combinaisons dont votre programme aura besoin. Seules quelques configurations basiques peuvent être changées de
manière dynamique, comme la couleur de fond. Les états doivent aussi être anticipés : il n’y a par exemple pas de
fonction de blending par défaut.

La bonne nouvelle est que grâce à cette anticipation, ce qui équivaut à peu près à une compilation versus une
interprétation, il y a beaucoup plus d’optimisations possibles pour le driver et le temps d’exécution est plus
prévisible, car les grandes étapes telles le changement de pipeline sont faites très explicites.

### Étape 7 - Command pools et command buffers

Comme dit plus haut, nombre d’opérations comme le rendu doivent être transmise à une queue. Ces opérations doivent
d’abord être enregistrées dans un `VkCommandBuffer` avant d’être envoyées. Ces command buffers sont alloués à partir
d’une «`VkCommandPool`» spécifique à une queue family. Pour afficher notre simple triangle nous devrons enregistrer les
opérations suivantes :

* Lancer la render pass
* Lier le pipeline graphique
* Afficher 3 sommets
* Terminer la passe

Du fait que l’image que nous avons extraite du framebuffer pour nous en servir comme cible dépend de l’image que la swap
chain nous fournira, nous devons préparer un command buffer pour chaque image possible et choisir le bon au moment de
l’affichage. Nous pourrions en créer un à chaque frame mais ce ne serait pas aussi efficace.

### Étape 8 - Boucle principale

Maintenant que nous avons inscrit les commandes graphiques dans des command buffers, la boucle principale n’est plus
qu'une question d’appels. Nous acquérons d’abord une image de la swap chain en utilisant `vkAcquireNextImageKHR`. Nous
sélectionnons ensuite le command buffer approprié pour cette image et le postons à la queue avec `vkQueueSubmit`. Enfin,
nous retournons l’image à la swap chain pour sa présentation à l’écran à l’aide de `vkQueuePresentKHR`.

Les opérations envoyées à la queue sont exécutées de manière asynchrone. Nous devons donc utiliser des objets de
synchronisation tels que des sémaphores pour nous assurer que les opérations sont exécutées dans l’ordre voulu.
L’exécution du command buffer d’affichage doit de plus attendre que l’acquisition de l’image soit terminée, sinon nous
pourrions dessiner sur une image utilisée pour l’affichage. L’appel à `vkQueuePresentKHR` doit aussi attendre que
l’affichage soit terminé.

### Résumé

Ce tour devrait vous donner une compréhension basique du travail que nous aurons à fournir pour afficher notre premier
triangle. Un véritable programme contient plus d’étapes comme allouer des vertex Buffers, créer des Uniform Buffers et
envoyer des textures, mais nous verrons cela dans des chapitres suivants. Nous allons commencer par les bases car Vulkan
a suffisamment d’étapes ainsi. Notez que nous allons "tricher" en écrivant les coordonnées du triangle directement dans
un shader, afin d’éviter l’utilisation d’un vertex buffer qui nécessite une certaine familiarité avec les Command
Buffers.

En résumé nous devrons, pour afficher un triangle :

* Créer une `VkInstance`
* Sélectionner une carte graphique compatible (`VkPhysicalDevice`)
* Créer un `VkDevice` et une `VkQueue` pour l’affichage et la présentation
* Créer une fenêtre, une surface dans cette fenêtre et une swap chain
* Considérer les images de la swap chain comme des `VkImageViews` puis des `VkFramebuffers`
* Créer la render pass spécifiant les cibles d’affichage et leurs usages
* Créer des framebuffers pour ces passes
* Générer le pipeline graphique
* Allouer et enregistrer des Command Buffers contenant toutes les commandes pour toutes les images de la swap chain
* Dessiner sur les frames en acquérant une image, en soumettant la commande d’affichage correspondante et en retournant
l’image à la swap chain

Cela fait beaucoup d’étapes, cependant le but de chacune d’entre elles sera explicitée clairement et simplement dans les
chapitres suivants. Si vous êtes confus quant à l’intérêt d’une étape dans le programme entier, référez-vous à ce
premier chapitre.

## Concepts de l’API

Ce chapitre va conclure en survolant la structure de l’API à un plus bas niveau.

### Conventions

Toute les fonctions, les énumérations et les structures de Vulkan sont définies dans le header `vulkan.h`, inclus dans
le [SDK Vulkan](https://lunarg.com/vulkan-sdk/) développé par LunarG. Nous verrons comment l’installer dans le prochain
chapitre.

Les fonctions sont préfixées par ‘vk’, les types comme les énumération et les structures par ‘Vk’ et les macros par
‘VK_’. L’API utilise massivement les structures pour la création d’objet plutôt que de passer des arguments à des
fonctions. Par exemple la création d’objet suit généralement le schéma suivant :

```c++
VkXXXCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_XXX_CREATE_INFO;
createInfo.pNext = nullptr;
createInfo.foo = ...;
createInfo.bar = ...;

VkXXX object;
if (vkCreateXXX(&createInfo, nullptr, &object) != VK_SUCCESS) {
    std::cerr << "failed to create object" << std::endl;
    return false;
}
```

De nombreuses structure imposent que l’on spécifie explicitement leur type dans le membre donnée «sType». Le membre
donnée «pNext» peut pointer vers une extension et sera toujours `nullptr` dans ce tutoriel. Les fonctions qui créent ou
détruisent les objets ont un paramètre appelé `VkAllocationCallbacks`, qui vous permettent de spécifier un allocateur.
Nous le mettrons également à `nullptr`.

La plupart des fonctions retournent un `VkResult`, qui peut être soit `VK_SUCCESS` soit un code d’erreur. La
spécification décrit lesquels chaque fonction renvoie et ce qu’ils signifient.

### Validation layers

Vulkan est pensé pour la performance et pour un travail minimal pour le driver. Il inclue donc très peu de gestion
d’erreur et de système de débogage. Le driver crashera beaucoup plus souvent qu’il ne retournera de code d’erreur si
vous faites quelque chose d’inattendu. Pire, il peut fonctionner sur votre carte graphique mais pas sur une autre.

Cependant, Vulkan vous permet d’effectuer des vérifications précises de chaque élément à l’aide d’une fonctionnalité
nommée «validation layers». Ces layers consistent en du code s’insérant entre l’API et le driver, et permettent de
lancer des analyses de mémoire et de relever les défauts. Vous pouvez les activer pendant le développement et les
désactiver sans conséquence sur la performance. N’importe qui peut écrire ses validation layers, mais celui du SDK de
LunarG est largement suffisant pour ce tutoriel. Vous aurez cependant à écrire vos propres fonctions de callback pour
le traitement des erreurs émises par les layers.

Du fait que Vulkan soit si explicite pour chaque opération et grâce à l’extensivité des validations layers, trouver les
causes de l’écran noir peut en fait être plus simple qu’avec OpenGL ou Direct3D!

Il reste une dernière étape avant de commencer à coder : mettre en place
[l’environnement de développement](!fr/Environnement_de_développement).
