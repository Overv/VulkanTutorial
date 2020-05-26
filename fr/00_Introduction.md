## À propos

Ce tutoriel vous enseignera les bases de l'utilisation de l'API [Vulkan](https://www.khronos.org/vulkan/) qui expose 
les graphismes et le calcul sur cartes graphiques. Vulkan est une nouvelle API créée par le
[groupe Khronos](https://www.khronos.org/) (connu pour OpenGL). Elle fournit une bien meilleure abstraction des cartes
graphiques modernes. Cette nouvelle interface vous permet de mieux décrire ce que votre application souhaite faire,
ce qui peut mener à de meilleures performances et à des comportements moins variables comparés à des APIs
existantes comme [OpenGL](https://en.wikipedia.org/wiki/OpenGL) et
[Direct3D](https://en.wikipedia.org/wiki/Direct3D). Les concepts introduits par Vulkan sont similaires à ceux de
[Direct3D 12](https://en.wikipedia.org/wiki/Direct3D#Direct3D_12) et [Metal](https://en.wikipedia.org/wiki/Metal_(API)).
Cependant Vulkan a l'avantage d'être complètement cross-platform, et vous permet ainsi de développer pour Windows,
Linux, Mac et Android en même temps.

Il y a cependant un contre-coup à ces avantages. L'API vous impose d'être explicite sur chaque détail. Vous ne pourrez
rien laisser au hasard, et il n'y a aucune structure, aucun environnement créé pour vous par défaut. Il faudra le
recréer à partir de rien. Le travail du driver graphique sera ainsi considérablement réduit, ce qui implique un plus 
grand travail de votre part pour assurer un comportement correct.

Le message véhiculé ici est que Vulkan n'est pas fait pour tout le monde. Cette API est conçue pour les programmeurs 
concernés par la programmation avec GPU de haute performance, et qui sont prêts à y travailler sérieusement. Si vous
êtes intéressées dans le développement de jeux vidéo, et moins dans les graphismes eux-mêmes, vous devriez plutôt
continuer d'utiliser OpenGL et DirectX, qui ne seront pas dépréciés en faveur de Vulkan avant un certain temps. Une
autre alternative serait d'utiliser un moteur de jeu comme
[Unreal Engine](https://en.wikipedia.org/wiki/Unreal_Engine#Unreal_Engine_4) ou
[Unity](https://en.wikipedia.org/wiki/Unity_(game_engine)), qui pourront être capables d'utiliser Vulkan tout en
exposant une API de bien plus haut niveau.

Cela étant dit, présentons quelques prérequis pour ce tutoriel:

* Une carte graphique et un driver compatibles avec Vulkan ([NVIDIA](https://developer.nvidia.com/vulkan-driver),
[AMD](https://www.amd.com/en/technologies/vulkan),
[Intel](https://software.intel.com/en-us/blogs/2017/02/10/intel-announces-that-we-are-moving-from-beta-support-to-full-official-support-for))
* De l'expérience avec le C++ (familiarité avec RAII, listes d'initialisation, et autres fonctionnalités modernes)
* Un compilateur avec un support décent des fonctionnalités du C++17 (Visual Studio 2017+, GCC 7+ ou Clang 5+)
* Un minimum d'expérience dans le domaine de la programmation graphique

Ce tutoriel ne considérera pas comme acquis les concepts d'OpenGL et de Direct3D, mais il requiert que vous connaissiez
les bases du rendu 3D. Il n'expliquera pas non plus les mathématiques derrière la projection de perspective, par
exemple. Lisez [ce livre](http://opengl.datenwolf.net/gltut/html/index.html) pour une bonne introduction des concepts
de rendu 3D. D'autres ressources pour le développement d'application graphiques sont :
* [Ray tracing en un week-end](https://github.com/petershirley/raytracinginoneweekend)
* [Livre sur le Physical Based Rendering](http://www.pbr-book.org/)
* Une application de Vulkan dans les moteurs graphiques open source [Quake](https://github.com/Novum/vkQuake) et de
[DOOM 3](https://github.com/DustinHLand/vkDOOM3)

Vous pouvez utiliser le C plutôt que le C++ si vous le souhaitez, mais vous devrez utiliser une autre bibliothèque
d'algèbre linéaire et vous structurerez vous-même votre code. Nous utiliserons des possibilités du C++ (RAII,
classes) pour organiser la logique et la durée de vie des ressources. Il existe aussi une
[version alternative](https://github.com/bwasty/vulkan-tutorial-rs) de ce tutoriel pour les développeurs rust.

Pour faciliter la tâche des développeurs utilisant d'autres langages de programmation, et pour acquérir de l'expérience
avec l'API de base, nous allons utiliser l'API C originelle pour travailler avec Vulkan. Cependant, si vous utilisez le
C++, vous pourrez préférer utiliser le binding [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp) plus récent,
qui permet de s'éloigner de certains détails ennuyeux et d'éviter certains types d'erreurs.

## E-book

Si vous préférez lire ce tutoriel en E-book, vous pouvez en télécharger une version EPUB ou PDF ici:

* [EPUB](https://raw.githubusercontent.com/Overv/VulkanTutorial/master/ebook/Vulkan%20Tutorial%20fr.epub)
* [PDF](https://raw.githubusercontent.com/Overv/VulkanTutorial/master/ebook/Vulkan%20Tutorial%20fr.pdf)

## Structure du tutoriel

Nous allons commencer par une approche générale du fonctionnement de Vulkan, et verrons d'abord rapidement le travail à
effectuer pour afficher un premier triangle à l'écran. Le but de chaque petite étape aura ainsi plus de sens quand
vous aurez compris leur rôle dans le fonctionnement global. Ensuite, nous préparerons l'environnement de développement,
avec le [SDK Vulkan](https://lunarg.com/vulkan-sdk/), la [bibliothèque GLM](http://glm.g-truc.net/) pour les opérations
d'algèbre linéaire, et [GLFW](http://www.glfw.org/) pour la création d'une fenêtre. Ce tutoriel couvrira leur mise en
place sur Windows avec Visual Studio, sur Linux Ubuntu avec GCC et sur MacOS.

Après cela, nous implémenterons tous les éléments nécessaires à un programme Vulkan pour afficher votre premier
triangle. Chaque chapitre suivra approximativement la structure suivante :

* Introduction d'un nouveau concept et de son utilité
* Utilisation de tous les appels correspondants à l'API pour leur mise en place dans votre programme
* Placement d'une partie de ces appels dans des fonctions pour une réutilisation future

Bien que chaque chapitre soit écrit comme suite du précédent, il est également possible de lire chacun d'entre eux
comme un article introduisant une certaine fonctionnalité de Vulkan. Ainsi le site peut vous être utile comme référence.
Toutes les fonctions et les types Vulkan sont liés à leur spécification, vous pouvez donc cliquer dessus pour en
apprendre plus. La spécification est par contre en Anglais. Vulkan est une API récente, il peut donc y avoir des 
lacunes dans la spécification elle-même. Vous êtes encouragés à transmettre vos retours dans
[ce repo Khronos](https://github.com/KhronosGroup/Vulkan-Docs).

Comme indiqué plus haut, Vulkan est une API assez prolixe, avec de nombreux paramètres, pensés pour vous fournir un
maximum de contrôle sur le hardware graphique. Ainsi des opérations comme créer une texture prennent de nombreuses
étapes qui doivent être répétées chaque fois. Nous créerons notre propre collection de fonctions d'aide tout le long
du tutoriel.

Chaque chapitre se conclura avec un lien menant à la totalité du code écrit jusqu'à ce point. Vous pourrez vous y
référer si vous avez un quelconque doute quant à la structure du code, ou si vous rencontrez un bug et que voulez
comparer. Tous les fichiers de code ont été testés sur des cartes graphiques de différents vendeurs pour pouvoir
affirmer qu'ils fonctionnent. Chaque chapitre possède également une section pour écrire vos commentaires en relation
avec le sujet discuté. Veuillez y indiquer votre plateforme, la version de votre driver, votre code source, le
comportement attendu et celui obtenu pour nous simplifier la tâche de vous aider.

Ce tutoriel est destiné à être un effort de communauté. Vulkan est encore une API très récente et les meilleures
manières d'arriver à un résultat n'ont pas encore été déterminées. Si vous avez un quelconque retour sur le tutoriel
et le site lui-même, n'hésitez alors pas à créer une issue ou une pull request sur le
[repo GitHub](https://github.com/Overv/VulkanTutorial). Vous pouvez *watch* le dépôt afin d'être notifié des
dernières mises à jour du site.

Après avoir accompli le rituel de l'affichage de votre premier triangle avec Vulkan, nous étendrons le programme pour y
inclure les transformations linéaires, les textures et les modèles 3D.

Si vous avez déjà utilisé une API graphique auparavant, vous devez savoir qu'il y a nombre d'étapes avant d'afficher la
première géométrie sur l'écran. Il y aura beaucoup plus de ces étapes préliminaires avec Vulkan, mais vous verrez que
chacune d'entre elle est simple à comprendre et n'est pas redondante. Gardez aussi à l'esprit qu'une fois que vous savez
afficher un triangle - certes peu intéressant -, afficher un modèle 3D parfaitement texturé ne nécessite pas tant de
travail supplémentaire, et que chaque étape à partir de ce point est bien mieux récompensée visuellement.

Si vous rencontrez un problème en suivant ce tutoriel, vérifiez d'abord dans la FAQ que votre problème et sa solution
n'y sont pas déjà listés. Si vous êtes toujours coincé après cela, demandez de l'aide dans la section des commentaires
du chapitre le plus en lien avec votre problème.

Prêt à vous lancer dans le futur des API graphiques de haute performance? [Allons-y!](!fr/Introduction)
