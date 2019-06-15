## À propos

Ce tutoriel vous enseignera les bases de l'utilisation de l'API [Vulkan](https://www.khronos.org/vulkan/) pour les graphismes
et le calcul. Vulkan est une nouvelle API créée par le [groupe Khronos](https://www.khronos.org/) (connu pour OpenGL) qui
fournit une bien meilleure abstraction des cartes graphiques modernes. Cette nouvelle interface vous permet de mieux
décrire ce que votre application souhaite faire, ce qui peut mener à de meilleures performances et à des comportements moins
surprenants comparés à des APIs existantes comme [OpenGL](https://en.wikipedia.org/wiki/OpenGL) et
[Direct3D](https://en.wikipedia.org/wiki/Direct3D). Les concepts derrière Vulkan sont similaires à ceux de
[Direct3D 12](https://en.wikipedia.org/wiki/Direct3D#Direct3D_12) et [Metal](https://en.wikipedia.org/wiki/Metal_(API)),
mais Vulkan a l'avantage d'être complètement cross-platform et vous permet ainsi de développer pour Windows, Linux, Mac
et Android en même temps.

Cependant, le prix à payer pour ces avantages est que vous devrez travailler avec une API beaucoup plus redondante. 
Chaque détail lié à l'API graphique doit être créé à partir de rien par votre application, dont la mise en place d'un
framebuffer initial et la gestion de la mémoire pour les objets tels que les buffers et les textures. Le travail du
driver graphique sera ainsi grandement réduit, ce qui implique un plus grand travail de votre part pour 
assurer un comportement correct.

Le message à retenir ici est que Vulkan n'est pas fait pour tout le monde. Il cible les programmeurs concernés par la
programmation graphique de haute performance, et qui sont prêts à y travailler sérieusement. Si vous êtes plus
intéressées dans le développement de jeux vidéos, plutôt que dans les graphismes eux-mêmes, vous devriez plutôt
continuer d'utiliser OpenGL et DirectX, qui ne seront pas dépréciés en faveur de Vulkan avant un certain temps. Une autre
alternative serait d'utiliser un moteur de jeu comme
[Unreal Engine](https://en.wikipedia.org/wiki/Unreal_Engine#Unreal_Engine_4) ou
[Unity](https://en.wikipedia.org/wiki/Unity_(game_engine)), qui pourront être capables d'utiliser Vulkan tout en
exposant une API de bien plus haut niveau.

Cela étant dit, présentons quelques prérequis pour ce tutoriel:

* Une carte graphique et un driver compatibles avec Vulkan ([NVIDIA](https://developer.nvidia.com/vulkan-driver),
[AMD](https://www.amd.com/en/technologies/vulkan),
[Intel](https://software.intel.com/en-us/blogs/2017/02/10/intel-announces-that-we-are-moving-from-beta-support-to-full-official-support-for))
* De l'expérience avec le C++ (familiarité avec RAII, listes d'initialisation, et autres fonctionnalités modernes)
* Un compilateur compatible C++11 (Visual Studio 2013+, GCC 4.8+)
* Un minimum d'expérience dans le domaine de la programmation graphique

Ce tutoriel ne considérera pas comme acquis les concepts d'OpenGL et de Direct3D, mais il requiert que vous connaissiez
les bases du graphisme 3D. Il n'expliquera pas non plus les mathématiques derrière la projection de perspective, par
exemple. Lisez [ce livre](http://opengl.datenwolf.net/gltut/html/index.html) pour une bonne introduction des concepts
des graphismes 3D. D'autres ressources pour le développement d'application graphiques sont :
* [Ray tracing en un week-end](https://github.com/petershirley/raytracinginoneweekend)
* [Livre sur le Physical Based Rendering](http://www.pbr-book.org/)
* Une application de Vulkan dans les moteurs graphiques open source [Quake](https://github.com/Novum/vkQuake) et de
[DOOM 3](https://github.com/DustinHLand/vkDOOM3)

Vous pouvez utiliser le C plutôt que le C++ si vous le souhaitez, mais vous devrez utiliser une autre bibliothèque d'algèbre
linéaire et vous structurerez vous-même votre code. Nous utiliserons des possibilités du C++ (RAII, classes) pour
organiser la logique et la durée de vie des ressources. Il existe aussi une
[version alternative](https://github.com/bwasty/vulkan-tutorial-rs) de ce tutoriel pour les développeurs rust.

Pour faciliter la tâche des développeurs utilisant d'autres langages de programmation, et pour acquérir de l'expérience
avec l'API de base, nous allons utiliser l'API C originelle pour travailler avec Vulkan. Cependant, si vous utilisez le
C++, vous pourrez préférer utiliser le binding [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp) plus récent,
qui permet de s'éloigner de certains détails ennuyeux et d'éviter certains types d'erreurs.

## E-book

Si vous préférez lire ce tutoriel en E-book, vous pouvez en télécharger une version EPUB ou PDF ici:

* [EPUB](https://raw.githubusercontent.com/Overv/VulkanTutorial/master/ebook/Vulkan%20Tutorial.epub)
* [PDF](https://raw.githubusercontent.com/Overv/VulkanTutorial/master/ebook/Vulkan%20Tutorial.pdf)

## Structure du tutoriel

Nous allons commencer par une approche globale de la manière dont Vulkan fonctionne, et le travail que nous aurons à faire pour afficher un
premier triangle à l'écran. Le but de chaque petite étape aura plus de sens quand vous aurez compris leur rôle dans le
fonctionnement global. Ensuite, nous préparerons l'environnement de développement, avec le [SDK Vulkan](https://lunarg.com/vulkan-sdk/), la
[bibliothèque GLM](http://glm.g-truc.net/) pour les opérations d'algèbre linéaire, et [GLFW](http://www.glfw.org/) pour la
création de fenêtre. Ce tutoriel couvrira leur mise en place sur Windows avec Visual Studio, sur Linux Ubuntu avec
GCC et sur MacOS.

Après cela, nous implémenterons tous les éléments basiques d'un programme Vulkan nécessaires à l'affichage de votre
premier triangle. Chaque chapitre suivra approximativement la structure suivante:

* Introduction d'un nouveau concept et son but
* Utilisation de tous les appels correspondants à l'API pour leur mise en place dans votre programme
* Placement d'une partie de ces appels dans des fonctions pour une réutilisation future

Bien que chaque chapitre soit écrit comme suite du précédent, il est également possible de lire chacun d'entre eux
comme un article introduisant une certaine fonctionnalité de Vulkan. Ainsi le site peut vous être utile comme référence.
Toutes les fonctions et les types Vulkan sont liés à leur spécification, vous pouvez donc cliquer dessus pour en
apprendre plus. Vulkan est une API récente, il peut donc y avoir des lacunes dans la spécification elle-même. Vous
êtes encouragés à transmettre vos retours dans [ce repo Khronos](https://github.com/KhronosGroup/Vulkan-Docs).

Comme indiqué plus haut, Vulkan est une API assez prolixe, avec de nombreux paramètres, afin de vous fournir un
maximum de contrôle sur le hardware graphique. Ansi, des opérations comme créer une texture prennent de nombreuses étapes
qui doivent être répétées chaque fois. Nous créerons par conséquent notre propre collection de fonctions d'aide tout le
long du tutoriel.

Chaque chapitre se conclura avec un lien menant à la totalité du code écrit jusqu'à ce point. Vous pourrez vous y référer
si vous avez un quelconque doute quant à la structure du code, ou si vous rencontrez un bug et que voulez comparer. Tous
les fichiers de code ont été testés sur des cartes graphiques de différents vendeurs afin d'assurer qu'ils fonctionnent.
Chaque chapitre possède également une section pour écrire vos commentaires en relation avec le sujet discuté. Veuillez y
indiquer votre plateforme, la version de votre driver, votre code source, le comportement attendu et celui obtenu pour
nous aider à vous aider.

Ce tutoriel est destiné à être un effort de communauté. Vulkan est encore une API très récente et les meilleures manières
d'arriver à un résultat n'ont pas encore été déterminées. Si vous avez un quelconque retour sur le tutoriel et le site
lui-même, n'hésitez alors pas à créer une issue ou une pull request sur le
[repo GitHub](https://github.com/Overv/VulkanTutorial). Vous pouvez *watch* le dépôt afin d'être notifié des
dernières mises à jour du tutoriel.

Après que vous avez accompli le rituel de l'affichage de votre premier triangle en Vulkan, nous étendrons le programme pour y
inclure les transformations linéaires, les textures et les modèles 3D.

Si vous avez déjà utilisé une API graphique auparavant, vous devez savoir qu'il y a nombre d'étapes avant d'afficher la
première géométrie sur l'écran. Il y a beaucoup de ces étapes préliminaires avec Vulkan, mais vous verrez que chacune
d'entre elle est simple à comprendre et n'est pas redondante. Gardez aussi à l'esprit qu'une fois que vous savez
afficher un triangle - certes peu intéressant -, afficher un modèle 3D parfaitement texturé ne nécessite pas tant de
travail supplémentaire, et que chaque étape à partir de ce point est bien plus récompensée visuellement.

Si vous rencontrez un problème en suivant ce tutoriel, vérifiez d'abord dans la FAQ que votre problème et sa solution
n'y sont pas déjà listés. Si vous êtes toujours coincé après cela, demandez de l'aide dans la section des commentaires
du chapitre le plus en lien avec votre problème.

Prêt à vous lancer dans le futur des API graphiques de haute performance? [Allons-y!](!Overview)
