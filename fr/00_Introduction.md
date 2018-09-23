## À propos

Ce tutoriel enseigne les bases de l'utilisation de l'API [Vulkan](https://www.khronos.org/vulkan/) pour les graphismes
et le calcul. Vulkan est un nouvel API créé par le [groupe Khronos](https://www.khronos.org/) (connu pour OpenGL) qui
fournit une bien meilleure abstraction des cartes graphiques modernes. Cette nouvelle interface vous permet de mieux
décrire le but de votre application, ce qui peut mener à de meilleures performances et à des comportements moins
surprenants comparés à des APIs existants comme [OpenGL] (https://en.wikipedia.org/wiki/OpenGL) et
[Direct3D](https://en.wikipedia.org/wiki/Direct3D). Les concepts derrière Vulkan sont similaires à ceux de
[Direct3D 12](https://en.wikipedia.org/wiki/Direct3D#Direct3D_12) et [Metal](https://en.wikipedia.org/wiki/Metal_(API)),
mais Vulkan a l'avantage d'être parfaitement cross-platform et vous permet ainsi de développer pour Windows, Linux et
Android en même temps.

Cependant, le prix à payer pour ces avantages est que vous devrez donner beaucoup plus d'informations à l'API. Chaque
détail lié à l'API graphique doit être recréé à partir de rien par votre application, dont la mise en place d'un
framebuffer initial et de la gestion de la mémoire pour les buffers et les textures. Le driver graphique ne touchera
qu'à très peu d'éléments, ce qui implique un plus grand travail de votre application pour assurer un comportement
correct.

Le message à retenir ici est que Vulkan n'est pas fait pour tout le monde. Il cible les programmeurs concernés par la
programmation graphique de haute performance, et qui sont prêts à y travailler sérieusement. Si vous êtes plus
intéressées dans le développement de jeux vidéos, plutôt que dans les graphismes eux-mêmes, vous devriez plutôt
continuer d'utiliser OpenGL et DirectX, qui ne seront pas remplacés par Vulkan avant un certain temps. Une autre
alternative serait d'utiliser un moteur graphique comme
[Unreal Engine](https://en.wikipedia.org/wiki/Unreal_Engine#Unreal_Engine_4) ou
[Unity](https://en.wikipedia.org/wiki/Unity_(game_engine)), qui pourront être capables d'utiliser Vulkan tout en
exposant un API de bien plus haut niveau.

Cela étant dit, présentons quelques prérequis pour ce tutoriel:

* Une carte graphique et un driver compatibles avec Vulkan ([NVIDIA](https://developer.nvidia.com/vulkan-driver), [AMD](http://www.amd.com/en-us/innovations/software-technologies/technologies-gaming vulkan), [Intel](https://software.intel.com/en-us/blogs/2016/03/14/new-intel-vulkan-beta-1540204404graphics-driver-for-windows-78110-1540))
* De l'expérience avec le C++ (familiarité avec RAII, listes d'initialisation...)
* Un compilateur compatible C++11 (Visual Studio 2013+, GCC 4.8+)
* De l'expérience dans le domaine de la programmation graphique

Ce tutoriel ne considérera pas comme acquis les concepts d'OpenGL et de Direct3D, mais il requiert que vous connaissiez
les bases du graphisme 3D. Il n'expliquera pas non plus les mathématiques derrière la projection de perspective, par
exemple. Voyez [ce livre](http://opengl.datenwolf.net/gltut/html/index.html) pour une bonne introduction des concepts
des graphismes 3D.

Vous pouvez utiliser le C plutôt que le C++ si vous le désirez, mais vous devrez utiliser une autre librairie d'algèbre
linéaire et vous serez seul à structurer votre code. Nous utiliserons des possibilités du C++ (RAII, classes) pour
organiser la logique et la durée de vie des ressources. Il existe aussi une [version alternative](https://github
.com/bwasty/vulkan-tutorial-rs) de ce tutoriel pour les développeurs en rust.

## E-book

Si vous préférez lire ce tutoriel en E-book, vous pouvez en télécharger une version EPUB ou PDF ici:

* [EPUB](https://raw.githubusercontent.com/Overv/VulkanTutorial/master/ebook/Vulkan%20Tutorial.epub)
* [PDF](https://raw.githubusercontent.com/Overv/VulkanTutorial/master/ebook/Vulkan%20Tutorial.pdf)

## Structure du tutoriel

Commençons par approcher la manière dont Vulkan fonctionne et le travail que nous aurons à faire pour afficher un
premier triangle à l'écran. Le but de chaque petite étape aura plus de sens quand vous saurez leur rôle dans le
fonctionnement global de l'API. Après, nous préparerons l'[SDK Vulkan](https://lunarg.com/vulkan-sdk/), la
[librairie GLM](http://glm.g-truc.net/) pour les opérations d'algèbre linéaire et [GLFW](http://www.glfw.org/) pour la
création d'une fenêtre. Ce tutoriel couvrira leur mise en place sur Windows avec Visual Studio, et sur Linux Ubuntu avec
GCC.

Après cela nous implémenterons tous les éléments d'un basique programme Vulkan, nécessaires pour l'affichage de votre
premier triangle. Chaque chapitre suivra approximativement la structure suivante:

* Introduction d'un nouveau concept et son intérêt
* Utilisation de tous les appels correspondants à l'API pour sa mise en place
* Abstraction d'une partie de ces appels pour une réutilisation future

Bien que chaque chapitre soit conçu comme suite du précédent, il est également possible de lire chacun d'entre eux
comme un article introduisant une certaine fonctionnalité de Vulkan. Ainsi, le site est aussi utile comme référence.
Toutes les fonctions et les types Vulkan sont liés à leur spécification, vous pouvez donc cliquer dessus pour en
apprendre plus. Vulkan est un API très récent, il peut donc y avoir des lacunes dans la spécification elle-même. Vous
êtes ainsi encouragés à indiquer vos retours dans [ce repo Khronos](https://github.com/KhronosGroup/Vulkan-Docs).

Comme indiqué plus haut, Vulkan est un API assez prolixe et requiert de nombreux paramètres, afin de vous fournir un
maximum de contrôle sur le hardware. Des opérations comme créer une texture prennent par conséquent de nombreuses étapes
qui doivent être répétées chaque fois. Nous créerons par conséquent notre propre collection de fonctions d'aide tout le
long du tutoriel.

Chaque chapitre se conclura sur un lien menant à la totalité du code écrit jusqu'à ce point. Vous pourrez vous y référer
si vous avez un quelconque doute quant à la structure du code, ou si vous avez rencontré un bug et voulez comparer. Tous
les fichiers de code ont été testés sur des cartes graphiques de différents vendeurs afin d'assurer qu'ils fonctionnent.
Chaque chapitre possède également une section pour écrire vos commentaires en relation avec le sujet discuté. Veuillez y
indiquer votre plateforme, la version de votre driver, votre code source, le comportement attendu et celui obtenu pour
nous aider à vous aider.

Ce tutoriel est destiné à être un effort de communauté. Vulkan est encore un API très nouveau et les meilleures manières
d'arriver à un résultat n'ont pas encore été déterminées. Si vous avez un quelconque retour sur le tutoriel et le site
eux-mêmes, n'hésitez pas à créer une issue ou une pull request au [repo GitHub](https://github.com/Overv/VulkanTutorial).
Vous pouvez *watch* le repo afin d'être au courant des derniers ajouts.

Après que vous ayez accompli le rituel de l'affichage de votre premier triangle, nous étendrons le programme pour y
inclure les transformations linéaires, les textures et les modèles 3D.

Si vous avez déjà utilisé un API graphique auparavant, vous devez savoir qu'il y a nombre d'étapes avant d'afficher la
première géométrie sur l'écran. Il y a beaucoup de ces étapes préliminaires avec Vulkan, mais vous verrez que chacune
d'entre elle est simple à comprendre et n'est pas redondante. Gardez aussi à l'esprit qu'une fois que vous savez
afficher un triangle - certes peu intéressant - afficher un modèle 3D parfaitement texturé ne nécessite pas tant de
travail supplémentaire, et que chaque étape à partir de ce point est bien mieux récompensée.

Si vous rencontrez un problème en suivant ce tutoriel, vérifiez d'abord dans la FAQ que votre problème et sa solution
n'y sont pas déjà listés. Si vous êtes toujours coincé après cela, demandez de l'aide dans la section des commentaires
du chapitre le plus en lien avec votre problème.

Prêt à vous lancer dans le futur des graphismes haute performance? [Let's go!](!Overview)
