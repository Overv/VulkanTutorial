Les anciens APIs définissaient des configurations par défaut pour toutes les étapes à fonction fixée de la pipeline
graphique. Avec Vulkan vous devez être explicite dans ce domaine également et devrez donc configurer la fonction de
mélange par exemple. Dans ce chapitre nous remplirons toutes les structures nécessaires à la configuration des étapes à
fonction fixée.

## Entrée des sommets

La structure `VkPipelineVertexInputStateCreateInfo` décrit le format des sommets envoyés au vertex shader. Elle
fait cela de deux manières :

* Liens (bindings) : espace entre les données et information sur ces données; sont-elles par sommet ou par instance?
(voyez [l'instanciation](https://en.wikipedia.org/wiki/Geometry_instancing))
* Descriptions d'attributs : types d'attributs passés au vertex shader, de quels bindings les charger et avec quel
décalage entre eux.

Dans la mesure où nous avons écrit les coordonnées directement dans le vertex shader, nous remplirons cette structure
en indiquant qu'il n'y a aucune donnée à charger. Nous y reviendrons dans le chapitre sur les vertex buffers.

```c++
VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
vertexInputInfo.vertexBindingDescriptionCount = 0;
vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optionnel
vertexInputInfo.vertexAttributeDescriptionCount = 0;
vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optionnel
```

Les membres `pVertexBindingDescriptions` et `pVertexAttributeDescriptions` pointent vers un tableau de structures
décrivant les détails du chargement des données des sommets. Ajoutez cette structure à la fonction
`createGraphicsPipeline` juste après le tableau `shaderStages`.

## Input assembly

La structure `VkPipelineInputAssemblyStateCreateInfo` décrit la nature de la géométrie voulue quand les sommets sont
reliés, et permet d'activer ou non la réévaluation des vertices. La première information est décrite dans le membre
`topology` et peut prendre ces valeurs :

* `VK_PRIMITIVE_TOPOLOGY_POINT_LIST` : chaque sommet est un point
* `VK_PRIMITIVE_TOPOLOGY_LINE_LIST` : dessine une ligne liant deux sommet en n'utilisant ces derniers qu'une seule fois
* `VK_PRIMITIVE_TOPOLOGY_LINE_STRIP` : le dernier sommet de chaque ligne est utilisée comme premier sommet
pour la ligne suivante
* `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST` : dessine un triangle en utilisant trois sommets, sans en réutiliser pour le
triangle suivant
* `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP ` : le deuxième et troisième sommets sont utilisées comme les deux premiers
pour le triangle suivant

Les sommets sont normalement chargés séquentiellement depuis le vertex buffer. Avec un _element buffer_ vous pouvez
cependant choisir vous-même les indices à charger. Vous pouvez ainsi réaliser des optimisations, comme n'utiliser
une combinaison de sommet qu'une seule fois au lieu de d'avoir les mêmes données plusieurs fois dans le buffer. Si
vous mettez le membre `primitiveRestartEnable` à la valeur `VK_TRUE`, il devient alors possible d'interrompre les
liaisons des vertices pour les modes `_STRIP` en utilisant l'index spécial `0xFFFF` ou `0xFFFFFFFF`.

Nous n'afficherons que des triangles dans ce tutoriel, nous nous contenterons donc de remplir la structure de
cette manière :

```c++
VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
inputAssembly.primitiveRestartEnable = VK_FALSE;
```

## Viewports et ciseaux

Un viewport décrit simplement la région d'un framebuffer sur laquelle le rendu sera effectué. Il couvrira dans la
pratique quasiment toujours la totalité du framebuffer, et ce sera le cas dans ce tutoriel.

```c++
VkViewport viewport{};
viewport.x = 0.0f;
viewport.y = 0.0f;
viewport.width = (float) swapChainExtent.width;
viewport.height = (float) swapChainExtent.height;
viewport.minDepth = 0.0f;
viewport.maxDepth = 1.0f;
```

N'oubliez pas que la taille des images de la swap chain peut différer des macros `WIDTH` et `HEIGHT`. Les images de
la swap chain seront plus tard les framebuffers sur lesquels la pipeline opérera, ce que nous devons prendre en compte
en donnant les dimensions dynamiquement acquises.

Les valeurs `minDepth` et `maxDepth` indiquent l'étendue des valeurs de profondeur à utiliser pour le frambuffer. Ces
valeurs doivent être dans `[0.0f, 1.0f]` mais `minDepth` peut être supérieure à `maxDepth`. Si vous ne faites rien de
particulier contentez-vous des valeurs `0.0f` et `1.0f`.

Alors que les viewports définissent la transformation de l'image vers le framebuffer, les rectangles de ciseaux
définissent la région de pixels qui sera conservée. Tout pixel en dehors des rectangles de ciseaux seront
éliminés par le rasterizer. Ils fonctionnent plus comme un filtre que comme une transformation. Les différence sont
illustrée ci-dessous. Notez que le rectangle de ciseau dessiné sous l'image de gauche n'est qu'une des possibilités :
tout rectangle plus grand que le viewport aurait fonctionné.

![](/images/viewports_scissors.png)

Dans ce tutoriel nous voulons dessiner sur la totalité du framebuffer, et ce sans transformation. Nous
définirons donc un rectangle de ciseaux couvrant tout le frambuffer :

```c++
VkRect2D scissor{};
scissor.offset = {0, 0};
scissor.extent = swapChainExtent;
```

Le viewport et le rectangle de ciseau se combinent en un *viewport state* à l'aide de la structure
`VkPipelineViewportStateCreateInfo`. Il est possible sur certaines cartes graphiques d'utiliser plusieurs viewports
et rectangles de ciseaux, c'est pourquoi la structure permet d'envoyer des tableaux de ces deux données.
L'utilisation de cette possibilité nécessite de l'activer au préalable lors de la création du logical device.

```c++
VkPipelineViewportStateCreateInfo viewportState{};
viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
viewportState.viewportCount = 1;
viewportState.pViewports = &viewport;
viewportState.scissorCount = 1;
viewportState.pScissors = &scissor;
```

## Rasterizer

Le rasterizer récupère la géométrie définie par des sommets et calcule les fragments qu'elle recouvre. Ils sont ensuite
traités par le fragment shaders. Il réalise également un
[test de profondeur](https://en.wikipedia.org/wiki/Z-buffering), le
[face culling](https://en.wikipedia.org/wiki/Back-face_culling) et le test de ciseau pour vérifier si le fragment doit
effectivement être traité ou non. Il peut être configuré pour émettre des fragments remplissant tous les polygones ou
bien ne remplissant que les cotés (wireframe rendering). Tout cela se configure dans la structure
`VkPipelineRasterizationStateCreateInfo`.

```c++
VkPipelineRasterizationStateCreateInfo rasterizer{};
rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
rasterizer.depthClampEnable = VK_FALSE;
```

Si le membre `depthClampEnable` est mis à `VK_TRUE`, les fragments au-delà des plans near et far ne pas supprimés
mais affichés à cette distance. Cela est utile dans quelques situations telles que les shadow maps. Cela aussi doit
être explicitement activé lors de la mise en place du logical device.

```c++
rasterizer.rasterizerDiscardEnable = VK_FALSE;
```

Si le membre `rasterizerDiscardEnable` est mis à `VK_TRUE`, aucune géométrie ne passe l'étape du rasterizer, ce qui
désactive purement et simplement toute émission de donnée vers le frambuffer.

```c++
rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
```

Le membre `polygonMode` définit la génération des fragments pour la géométrie. Les modes suivants sont disponibles :

* `VK_POLYGON_MODE_FILL` : remplit les polygones de fragments
* `VK_POLYGON_MODE_LINE` : les côtés des polygones sont dessinés comme des lignes
* `VK_POLYGON_MODE_POINT` : les sommets sont dessinées comme des points

Tout autre mode que fill doit être activé lors de la mise en place du logical device.

```c++
rasterizer.lineWidth = 1.0f;
```

Le membre `lineWidth` définit la largeur des lignes en terme de fragments. La taille maximale supportée dépend du GPU
et pour toute valeur autre que `1.0f` l'extension `wideLines` doit être activée.

```c++
rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
```

Le membre `cullMode` détermine quel type de face culling utiliser. Vous pouvez désactiver tout ce filtrage,
n'éliminer que les faces de devant, que celles de derrière ou éliminer toutes les faces. Le membre `frontFace`
indique l'ordre d'évaluation des vertices pour dire que la face est devant ou derrière, qui est le sens des
aiguilles d'une montre ou le contraire.

```c++
rasterizer.depthBiasEnable = VK_FALSE;
rasterizer.depthBiasConstantFactor = 0.0f; // Optionnel
rasterizer.depthBiasClamp = 0.0f; // Optionnel
rasterizer.depthBiasSlopeFactor = 0.0f; // Optionnel
```

Le rasterizer peut altérer la profondeur en y ajoutant une valeur constante ou en la modifiant selon l'inclinaison du
fragment. Ces possibilités sont parfois exploitées pour le shadow mapping mais nous ne les utiliserons pas. Laissez
`depthBiasEnabled` à la valeur `VK_FALSE`.

## Multisampling

La structure `VkPipelineMultisampleCreateInfo` configure le multisampling, l'un des outils permettant de réaliser
[l'anti-aliasing](https://en.wikipedia.org/wiki/Multisample_anti-aliasing). Le multisampling combine les résultats
d'invocations du fragment shader sur des fragments de différents polygones qui résultent au même pixel. Cette
superposition arrive plutôt sur les limites entre les géométries, et c'est aussi là que les problèmes visuels de
hachage arrivent le plus. Dans la mesure où le fragment shader n'a pas besoin d'être invoqué plusieurs fois si seul un
polygone correspond à un pixel, cette approche est beaucoup plus efficace que d'augmenter la résolution de la texture.
Son utilisation nécessite son activation au niveau du GPU.

```c++
VkPipelineMultisampleStateCreateInfo multisampling{};
multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
multisampling.sampleShadingEnable = VK_FALSE;
multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
multisampling.minSampleShading = 1.0f; // Optionnel
multisampling.pSampleMask = nullptr; // Optionnel
multisampling.alphaToCoverageEnable = VK_FALSE; // Optionnel
multisampling.alphaToOneEnable = VK_FALSE; // Optionnel
```

Nous reverrons le multisampling plus tard, pour l'instant laissez-le désactivé.

## Tests de profondeur et de pochoir

Si vous utilisez un buffer de profondeur (depth buffer) et/ou de pochoir (stencil buffer) vous devez configurer les
tests de profondeur et de pochoir avec la structure `VkPipelineDepthStencilStateCreateInfo`. Nous n'avons aucun de
ces buffers donc nous indiquerons `nullptr` à la place d'une structure. Nous y reviendrons au chapitre sur le depth
buffering.

## Color blending

La couleur donnée par un fragment shader doit être combinée avec la couleur déjà présente dans le framebuffer. Cette
opération s'appelle color blending et il y a deux manières de la réaliser :

* Mélanger linéairement l'ancienne et la nouvelle couleur pour créer la couleur finale
* Combiner l'ancienne et la nouvelle couleur à l'aide d'une opération bit à bit

Il y a deux types de structures pour configurer le color blending. La première,
`VkPipelineColorBlendAttachmentState`, contient une configuration pour chaque framebuffer et la seconde,
`VkPipelineColorBlendStateCreateInfo` contient les paramètres globaux pour ce color blending. Dans notre cas nous
n'avons qu'un seul framebuffer :

```c++
VkPipelineColorBlendAttachmentState colorBlendAttachment{};
colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
colorBlendAttachment.blendEnable = VK_FALSE;
colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optionnel
colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optionnel
colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optionnel
colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optionnel
colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optionnel
colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optionnel
```

Cette structure spécifique de chaque framebuffer vous permet de configurer le color blending. L'opération sera
effectuée à peu près comme ce pseudocode le montre :

```c++
if (blendEnable) {
    finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
    finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
} else {
    finalColor = newColor;
}

finalColor = finalColor & colorWriteMask;
```

Si `blendEnable` vaut `VK_FALSE` la nouvelle couleur du fragment shader est inscrite dans le framebuffer sans
modification et sans considération de la valeur déjà présente dans le framebuffer. Sinon les deux opérations de
mélange sont exécutées pour former une nouvelle couleur. Un AND binaire lui est appliquée avec `colorWriteMask` pour
déterminer les canaux devant passer.

L'utilisation la plus commune du mélange de couleurs utilise le canal alpha pour déterminer l'opacité du matériau et
donc le mélange lui-même. La couleur finale devrait alors être calculée ainsi :

```c++
finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
finalColor.a = newAlpha.a;
```

Avec cette méthode la valeur alpha correspond à une pondération pour la nouvelle valeur par rapport à l'ancienne. Les
paramètres suivants permettent de faire exécuter ce calcul :

```c++
colorBlendAttachment.blendEnable = VK_TRUE;
colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
```

Vous pouvez trouver toutes les opérations possibles dans les énumérations `VkBlendFactor` et `VkBlendOp` dans la
spécification.

La seconde structure doit posséder une référence aux structures spécifiques des framebuffers. Vous pouvez également y
indiquer des constantes utilisables lors des opérations de mélange que nous venons de voir.

```c++
VkPipelineColorBlendStateCreateInfo colorBlending{};
colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
colorBlending.logicOpEnable = VK_FALSE;
colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optionnel
colorBlending.attachmentCount = 1;
colorBlending.pAttachments = &colorBlendAttachment;
colorBlending.blendConstants[0] = 0.0f; // Optionnel
colorBlending.blendConstants[1] = 0.0f; // Optionnel
colorBlending.blendConstants[2] = 0.0f; // Optionnel
colorBlending.blendConstants[3] = 0.0f; // Optionnel
```

Si vous voulez utiliser la seconde méthode de mélange (la combinaison bit à bit) vous devez indiquer `VK_TRUE` au
membre `logicOpEnable` et déterminer l'opération dans `logicOp`. Activer ce mode de mélange désactive automatiquement
la première méthode aussi radicalement que si vous aviez indiqué `VK_FALSE` au membre `blendEnable` de la
précédente structure pour chaque framebuffer. Le membre `colorWriteMask` sera également utilisé dans ce second mode pour
déterminer les canaux affectés. Il est aussi possible de désactiver les deux modes comme nous l'avons fait ici. Dans
ce cas les résultats des invocations du fragment shader seront écrits directement dans le framebuffer.

## États dynamiques

Un petit nombre d'états que nous avons spécifiés dans les structures précédentes peuvent en fait être altérés
sans avoir à recréer la pipeline. On y trouve la taille du viewport, la largeur des lignes et les constantes de mélange.
Pour cela vous devrez remplir la structure `VkPipelineDynamicStateCreateInfo` comme suit :

```c++
std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_LINE_WIDTH
};

VkPipelineDynamicStateCreateInfo dynamicState{};
dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
dynamicState.pDynamicStates = dynamicStates.data();
```

Les valeurs données lors de la configuration seront ignorées et vous devrez en fournir au moment du rendu. Nous y
reviendrons plus tard. Cette structure peut être remplacée par `nullptr` si vous ne voulez pas utiliser de dynamisme
sur ces états.

## Pipeline layout

Les variables `uniform` dans les shaders sont des données globales similaires aux états dynamiques. Elles doivent
être déterminées lors du rendu pour altérer les calculs des shaders sans avoir à les recréer. Elles sont très utilisées
pour fournir les matrices de transformation au vertex shader et pour créer des samplers de texture dans les fragment
shaders.

Ces variables doivent être configurées lors de la création de la pipeline en créant une variable
de type `VkPipelineLayout`. Même si nous n'en utilisons pas dans nos shaders actuels nous devons en créer un vide.

Créez un membre donnée pour stocker la structure car nous en aurons besoin plus tard.

```c++
VkPipelineLayout pipelineLayout;
```

Créons maintenant l'objet dans la fonction `createGraphicsPipline` :

```c++
VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 0;            // Optionnel
pipelineLayoutInfo.pSetLayouts = nullptr;         // Optionnel
pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optionnel
pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optionnel

if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("échec de la création du pipeline layout!");
}
```

Cette structure informe également sur les _push constants_, une autre manière de passer des valeurs dynamiques au
shaders que nous verrons dans un futur chapitre. Le pipeline layout sera utilisé pendant toute la durée du
programme, nous devons donc le détruire dans la fonction `cleanup` :

```c++
void cleanup() {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    ...
}
```

## Conclusion

Voila tout ce qu'il y a à savoir sur les étapes à fonction fixée! Leur configuration représente un gros travail
mais nous sommes au courant de tout ce qui se passe dans la pipeline graphique, ce qui réduit les chances de
comportement imprévu à cause d'un paramètre par défaut oublié.

Il reste cependant encore un objet à créer avant du finaliser la pipeline graphique. Cet objet s'appelle
[passe de rendu](!fr/Dessiner_un_triangle/Pipeline_graphique_basique/Render_pass).

[Code C++](/code/10_fixed_functions.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
