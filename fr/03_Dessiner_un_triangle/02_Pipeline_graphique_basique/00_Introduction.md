Dans les chapitres qui viennent nous allons configurer une pipeline graphique pour qu'elle affiche notre premier 
triangle. La pipeline graphique est l'ensemble des opérations qui prennent les vertices et les textures de vos 
éléments et les utilisent pour en faire des pixels sur les cibles d'affichage. Un résumé simplifié ressemble à ceci :

![](/images/vulkan_simplified_pipeline.svg)

L'_input assembler_ collecte les données des sommets à partir des buffers que vous avez mis en place, et peut aussi
utiliser un _index buffer_ pour répéter certains éléments sans avoir à stocker deux fois les mêmes données dans un
buffer.

Le _vertex shader_ est exécuté pour chaque sommet et leur applique en général des transformations pour que leurs
coordonnées passent de l'espace du modèle (model space) à l'espace de l'écran (screen space). Il fournit ensuite des
données à la suite de la pipeline.

Les _tesselation shaders_ permettent de subdiviser la géométrie selon des règles paramétrables afin d'améliorer la 
qualité du rendu. Ce procédé est notamment utilisé pour que des surface comme les murs de briques ou les escaliers 
aient l'air moins plats lorsque l'on s'en approche.

Le _geometry shader_ est invoqué pour chaque primitive (triangle, ligne, points...) et peut les détruire ou en créer
de nouvelles, du même type ou non. Ce travail est similaire au tesselation shader tout en étant beaucoup plus
flexible. Il n'est cependant pas beaucoup utilisé à cause de performances assez moyennes sur les cartes graphiques
(avec comme exception les GPUs intégrés d'Intel).

La _rasterization_ transforme les primitives en _fragments_. Ce sont les pixels auxquels les primitives correspondent
sur le framebuffer. Tout fragment en dehors de l'écran est abandonné. Les attributs sortant du vertex shader 
sont interpolés lorsqu'ils sont donnés aux étapes suivantes. Les fragments cachés par d'autres fragments sont aussi 
quasiment toujours éliminés grâce au test de profondeur (depth testing).

Le _fragment shader_ est invoqué pour chaque fragment restant et détermine à quel(s) framebuffer(s) le fragment
est envoyé, et quelles données y sont inscrites. Il réalise ce travail à l'aide des données interpolées émises par le
vertex shader, ce qui inclut souvent des coordonnées de texture et des normales pour réaliser des calculs d'éclairage.

Le _color blending_ applique des opérations pour mixer différents fragments correspondant à un même pixel sur le 
framebuffer. Les fragments peuvent remplacer les valeurs des autres, s'additionner ou se mélanger selon les 
paramètres de transparence (ou plus correctement de translucidité, en anglais translucency).

Les étapes écrites en vert sur le diagramme s'appellent _fixed-function stages_ (étapes à fonction fixée). Il est 
possible de modifier des paramètres influençant les calculs, mais pas de modifier les calculs eux-mêmes.

Les étapes colorées en orange sont programmables, ce qui signifie que vous pouvez charger votre propre code dans la 
carte graphique pour y appliquer exactement ce que vous voulez. Cela vous permet par exemple d'utiliser les fragment
shaders pour implémenter n'importe quoi, de l'utilisation de textures et d'éclairage jusqu'au _ray tracing_. Ces 
programmes tournent sur de nombreux coeurs simultanément pour y traiter de nombreuses données en parallèle.

Si vous avez utilisé d'anciens APIs comme OpenGL ou Direct3D, vous êtes habitués à pouvoir changer un quelconque 
paramètre de la pipeline à tout moment, avec des fonctions comme `glBlendFunc` ou `OMSSetBlendState`. Cela n'est plus
possible avec Vulkan. La pipeline graphique y est quasiment fixée, et vous devrez en recréer une complètement si 
vous voulez changer de shader, y attacher différents framebuffers ou changer le color blending. Devoir créer une
pipeline graphique pour chacune des combinaisons dont vous aurez besoin tout au long du programme représente un gros 
travail, mais permet au driver d'optimiser beaucoup mieux l'exécution des tâches car il sait à l'avance ce que la carte
graphique aura à faire.

Certaines étapes programmables sont optionnelles selon ce que vous voulez faire. Par exemple la tesselation et le 
geometry shader peuvent être désactivés. Si vous n'êtes intéressé que par les valeurs de profondeur vous pouvez 
désactiver le fragment shader, ce qui est utile pour les [shadow maps](https://en.wikipedia.org/wiki/Shadow_mapping).

Dans le prochain chapitre nous allons d'abord créer deux étapes nécessaires à l'affichage d'un triangle à l'écran : 
le vertex shader et le fragment shader. Les étapes à fonction fixée seront mises en place dans le chapitre suivant. 
La dernière préparation nécessaire à la mise en place de la pipeline graphique Vulkan sera de fournir les framebuffers 
d'entrée et de sortie.

Créez la fonction `createGraphicsPipeline` et appelez-la depuis `initVulkan` après `createImageViews`. Nous 
travaillerons sur cette fonction dans les chapitres suivants.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
}

...

void createGraphicsPipeline() {

}
```

[Code C++](/code/08_graphics_pipeline.cpp)
