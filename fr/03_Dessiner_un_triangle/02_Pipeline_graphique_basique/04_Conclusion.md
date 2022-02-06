Nous pouvons maintenant combiner toutes les structures et tous les objets des chapitres précédentes pour créer la 
pipeline graphique! Voici un petit récapitulatif des objets que nous avons :

* Étapes shader : les modules shader définissent le fonctionnement des étapes programmables de la pipeline graphique
* Étapes à fonction fixée : plusieurs structures paramètrent les étapes à fonction fixée comme l'assemblage des 
entrées, le rasterizer, le viewport et le mélange des couleurs
* Organisation de la pipeline : les uniformes et push constants utilisées par les shaders, auxquelles on attribue une
valeur pendant l'exécution de la pipeline
* Render pass : les attachements référencés par la pipeline et leurs utilisations

Tout cela combiné définit le fonctionnement de la pipeline graphique. Nous pouvons maintenant remplir la structure 
`VkGraphicsPipelineCreateInfo` à la fin de la fonction `createGraphicsPipeline`, mais avant les appels à la fonction 
`vkDestroyShaderModule` pour ne pas invalider les shaders que la pipeline utilisera.

Commençons par référencer le tableau de `VkPipelineShaderStageCreateInfo`.

```c++
VkGraphicsPipelineCreateInfo pipelineInfo{};
pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
pipelineInfo.stageCount = 2;
pipelineInfo.pStages = shaderStages;
```

Puis donnons toutes les structure décrivant les étapes à fonction fixée.

```c++
pipelineInfo.pVertexInputState = &vertexInputInfo;
pipelineInfo.pInputAssemblyState = &inputAssembly;
pipelineInfo.pViewportState = &viewportState;
pipelineInfo.pRasterizationState = &rasterizer;
pipelineInfo.pMultisampleState = &multisampling;
pipelineInfo.pDepthStencilState = nullptr; // Optionnel
pipelineInfo.pColorBlendState = &colorBlending;
pipelineInfo.pDynamicState = nullptr; // Optionnel
```

Après cela vient l'organisation de la pipeline, qui est une référence à un objet Vulkan plutôt qu'une structure.

```c++
pipelineInfo.layout = pipelineLayout;
```

Finalement nous devons fournir les références à la render pass et aux indices des subpasses. Il est aussi possible
d'utiliser d'autres render passes avec cette pipeline mais elles doivent être compatibles avec `renderPass`. La 
signification de compatible est donnée
[ici](https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap8.html#renderpass-compatibility), mais nous 
n'utiliserons pas cette possibilité dans ce tutoriel.

```c++
pipelineInfo.renderPass = renderPass;
pipelineInfo.subpass = 0;
```

Il nous reste en fait deux paramètres : `basePipelineHandle` et `basePipelineIndex`. Vulkan vous permet de créer une 
nouvelle pipeline en "héritant" d'une pipeline déjà existante. L'idée derrière cette fonctionnalité est qu'il
est moins coûteux de créer une pipeline à partir d'une qui existe déjà, mais surtout que passer d'une pipeline à une
autre est plus rapide si elles ont un même parent. Vous pouvez spécifier une pipeline de deux manières : soit en 
fournissant une référence soit en donnant l'indice de la pipeline à hériter. Nous n'utilisons pas cela donc 
nous indiquerons une référence nulle et un indice invalide. Ces valeurs ne sont de toute façon utilisées que si le champ 
`flags` de la structure `VkGraphicsPipelineCreateInfo` comporte `VK_PIPELINE_CREATE_DERIVATIVE_BIT`.

```c++
pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optionnel
pipelineInfo.basePipelineIndex = -1; // Optionnel
```

Préparons-nous pour l'étape finale en créant un membre donnée où stocker la référence à la `VkPipeline` :

```c++
VkPipeline graphicsPipeline;
```

Et créons enfin la pipeline graphique :

```c++
if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("échec de la création de la pipeline graphique!");
}
```

La fonction `vkCreateGraphicsPipelines` possède en fait plus de paramètres que les fonctions de création d'objet que 
nous avons pu voir jusqu'à présent. Elle peut en effet accepter plusieurs structures `VkGraphicsPipelineCreateInfo` 
et créer plusieurs `VkPipeline` en un seul appel.

Le second paramètre que nous n'utilisons pas ici (mais que nous reverrons dans un chapitre qui lui sera dédié) sert à
fournir un objet `VkPipelineCache` optionnel. Un tel objet peut être stocké et réutilisé entre plusieurs appels de la
fonction et même entre plusieurs exécutions du programme si son contenu est correctement stocké dans un fichier. Cela
permet de grandement accélérer la création des pipelines.

La pipeline graphique est nécessaire à toutes les opérations d'affichage, nous ne devrons donc la supprimer qu'à la fin
du programme dans la fonction `cleanup` :

```c++
void cleanup() {
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    ...
}
```

Exécutez votre programme pour vérifier que tout ce travail a enfin résulté dans la création d'une pipeline graphique.
Nous sommes de plus en plus proches d'avoir un dessin à l'écran! Dans les prochains chapitres nous générerons les 
framebuffers à partir des images de la swap chain et préparerons les commandes d'affichage.

[Code C++](/code/12_graphics_pipeline_complete.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
