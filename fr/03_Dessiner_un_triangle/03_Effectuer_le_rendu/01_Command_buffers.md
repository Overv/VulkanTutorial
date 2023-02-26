Les commandes Vulkan, comme les opérations d'affichage et de transfert mémoire, ne sont pas réalisées avec des appels de fonctions.
Il faut pré-enregistrer toutes les opérations dans des _command buffers_.
L'avantage de cette approche est que quand nous sommes prêt à dire à Vulkan ce nous voulons faire, toutes les commandes sont
soumises ensemble et Vulkan peut traiter les commandes plus efficacement puisqu'elles sont toutes disponibles en même temps.
Cela permet également de faire l'enregistrement de commandes dans mulptiple threads si besoin.

## Command pools

Nous devons créer une *command pool* avant de pouvoir créer les command buffers. Les command pools gèrent la mémoire
utilisée par les buffers, et c'est de fait les command pools qui nous instancient les command buffers. Ajoutez un 
nouveau membre donnée à la classe de type `VkCommandPool` :

```c++
VkCommandPool commandPool;
```

Créez ensuite la fonction `createCommandPool` et appelez-la depuis `initVulkan` après la création du framebuffer.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
}

...

void createCommandPool() {

}
```

La création d'une command pool ne nécessite que deux paramètres :

```c++
QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

VkCommandPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
```

Il existe deux valeurs acceptées par `flags` pour les command pools :

* `VK_COMMAND_POOL_CREATE_TRANSIENT_BIT` : informe que les command buffers sont ré-enregistrés très souvent, ce qui
peut inciter Vulkan (et donc le driver) à ne pas utiliser le même type d'allocation
* `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT` : permet aux command buffers d'être ré-enregistrés individuellement,
ce que les autres configurations ne permettent pas

Nous enregistrerons les command buffers à chaque frame, donc nous voulons pouvoir les réinitialiser et les réenregistrer.
Ainsi nous utilisons le flag `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT` pour notre command pool.

Les command buffers sont exécutés en les soumettant sur une des queues, comme la queue des graphismes et de présentation
que l'on a déjà récupéré. Chaque command pool ne peut allouer que des command buffers qui sont soumis sur un seul type de queue.
Nous allons enregistrer les commands pour dessiner, c'est pour cette raison que nous avons choisi la famille de queue des graphismes.

```c++
if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
    throw std::runtime_error("échec de la création d'une command pool!");
}
```

Terminez la création de la command pool à l'aide de la fonction `vkCreateComandPool`. Elle ne comprend pas de
paramètre particulier. Les commandes seront utilisées tout au long du programme pour tout affichage, nous ne devons
donc la détruire que dans la fonction `cleanup` :

```c++
void cleanup() {
    vkDestroyCommandPool(device, commandPool, nullptr);

    ...
}
```

## Allocation des command buffers

Nous pouvons maintenant allouer des command buffers.

Créez un objet `VkCommandBuffer` comme membre de classe. Les command buffers seront automatiquement
libérés quand leur command pool sera détruite, donc nous n'avons pas besoin de les détruire explicitement.

```c++
VkCommandBuffer commandBuffer;
```

Commençons maintenant à travailler sur notre fonction `createCommandBuffer` qui allouera et enregistrera un unique command
buffer dans la command pool.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
}

...

void createCommandBuffer() {

}
```

Les command buffers sont alloués par la fonction `vkAllocateCommandBuffers` qui prend en paramètre une structure du
type `VkCommandBufferAllocateInfo`. Cette structure spécifie la command pool et le nombre de buffers à allouer depuis
celle-ci :

```c++
VkCommandBufferAllocateInfo allocInfo{};
allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
allocInfo.commandPool = commandPool;
allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
allocInfo.commandBufferCount = 1;

if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("échec de l'allocation du command buffer!");
}
```

Les command buffers peuvent être *primaires* ou *secondaires*, ce que l'on indique avec le paramètre `level`. Il peut
prendre les valeurs suivantes :

* `VK_COMMAND_BUFFER_LEVEL_PRIMARY` : peut être envoyé à une queue pour y être exécuté mais ne peut être appelé par
d'autres command buffers
* `VK_COMMAND_BUFFER_LEVEL_SECONDARY` : ne peut pas être directement émis à une queue mais peut être appelé par un autre
command buffer

Nous n'utiliserons pas la fonctionnalité de command buffer secondaire ici. Sachez que le mécanisme de command buffer
secondaire est à la base de la génération rapie de commandes d'affichage depuis plusieurs threads.

Comme nous n'allouons qu'un seul command buffer, le paramètre `commandBufferCount` est à 1.

## Enregistrement des commandes

Nous allons commencer à travailler sur la fonction `recordCommandBuffer` qui permet d'écrire les commandes que l'on voudra
exécuter dans un command buffer. L'objet `VkCommandBuffer` sera passé en paramètre, tout comme l'indice de l'image de la swapchain
sur laquelle nous voudrons écrire.

```c++
void recordCommandBuffer(VkCommandBuffer commandBuffer, uin32_t imageIndex) {

}
```

Comme toujours, nous commençons par remplir une structure `VkCommandBufferBeginInfo` que nous passerons en argument à la fonction
`vkBeginCommandBuffer` pour décrire les détails de l'utilisation du command buffer.

```c++
VkCommandBufferBeginInfo beginInfo{};
beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
beginInfo.flags = 0; // Optionnel
beginInfo.pInheritanceInfo = nullptr; // Optionnel

if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
}
}
```

L'utilisation du command buffer s'indique avec le paramètre `flags`, qui peut prendre les valeurs suivantes :

* `VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT` : le command buffer sera ré-enregistré après son utilisation, donc
invalidé une fois son exécution terminée
* `VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT` : ce command buffer secondaire sera intégralement exécuté dans une
unique render pass
* `VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT` : le command buffer peut être ré-envoyé à la queue alors qu'il y est
déjà et/ou est en cours d'exécution

Nous n'avons pas besoin de ces flags ici.

Le paramètre `pInheritanceInfo` n'a de sens que pour les command buffers secondaires.
Il indique l'état à hériter de l'appel par le command buffer primaire.

Si un command buffer est déjà prêt un appel à `vkBeginCommandBuffer` le regénèrera implicitement. Il n'est pas possible
d'enregistrer un command buffer en plusieurs fois.

## Commencer une render pass

L'affichage commence par le lancement de la render pass réalisé par `vkCmdBeginRenderPass`. La passe est configurée
à l'aide des paramètres remplis dans une structure de type `VkRenderPassBeginInfo`.

```c++
VkRenderPassBeginInfo renderPassInfo{};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
renderPassInfo.renderPass = renderPass;
renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
```

Ces premiers paramètres sont la render pass elle-même et les attachements à lui fournir. Nous avons créé un
framebuffer pour chacune des images de la swap chain, ces images sont spécifiées comme attachements de couleur.
De plus nous devons lier le frambuffer à l'image sur laquelle nous voulons écrire. En utilisant le paramètre
`imageIndex`, nous pouvons déterminer le bon framebuffer pour l'image actuelle de la swap chain.

```c++
renderPassInfo.renderArea.offset = {0, 0};
renderPassInfo.renderArea.extent = swapChainExtent;
```

Les deux paramètres qui suivent définissent la taille de la zone de rendu. Cette zone de rendu définit où les
chargements et stockages shaders se produiront. Les pixels hors de cette région auront une valeur non définie. Elle
doit correspondre à la taille des attachements pour avoir une performance optimale.

```c++
VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
renderPassInfo.clearValueCount = 1;
renderPassInfo.pClearValues = &clearColor;
```

Les deux derniers paramètres définissent les valeurs à utiliser pour remplacer le contenu (fonctionnalité que nous 
avions activée avec `VK_ATTACHMENT_LOAD_CLEAR`). J'ai utilisé un noir complètement opaque.

```c++
vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
```

La render pass peut maintenant commencer. Toutes les fonctions enregistrables se reconnaisent à leur préfixe `vkCmd`.
Comme elles retournent toutes `void` nous n'avons aucun moyen de gérer d'éventuelles erreurs avant d'avoir fini
l'enregistrement.

Le premier paramètre de chaque commande est toujours le command buffer qui stockera l'appel. Le second paramètre donne
des détails sur la render pass à l'aide de la structure que nous avons préparée. Le dernier paramètre informe sur la
provenance des commandes pendant l'exécution de la passe. Il peut prendre ces valeurs :

* `VK_SUBPASS_CONTENTS_INLINE` : les commandes de la render pass seront inclues directement dans le command buffer
(qui est donc primaire)
* `VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFER` : les commandes de la render pass seront fournies par un ou
plusieurs command buffers secondaires

Nous n'utiliserons pas de command buffer secondaire, nous devons donc fournir la première valeur à la fonction.

## Commandes d'affichage basiques

Nous pouvons maintenant activer la pipeline graphique :

```c++
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
```

Le second paramètre indique que la pipeline est bien une pipeline graphique et non de calcul. Nous avons fourni à Vulkan
les opérations à exécuter avec la pipeline graphique et les attachements que le fragment shader devra utiliser. Il ne
nous reste donc plus qu'à lui dire d'afficher un triangle :

```c++
vkCmdDraw(commandBuffer, 3, 1, 0, 0);
```

Le fonction `vkCmdDraw` est assez ridicule quand on sait tout ce qu'elle implique, mais sa simplicité est due
à ce que tout a déjà été préparé en vue de ce moment tant attendu. Elle possède les paramètres suivants en plus du
command buffer concerné :

* `vertexCount` : même si nous n'avons pas de vertex buffer, nous avons techniquement trois vertices à dessiner
* `instanceCount` : sert au rendu instancié (instanced rendering); indiquez `1` si vous ne l'utilisez pas
* `firstVertex` : utilisé comme décalage dans le vertex buffer et définit ainsi la valeur la plus basse pour `glVertexIndex`
* `firstInstance` : utilisé comme décalage pour l'instanced rendering et définit ainsi la valeur la plus basse pour `gl_InstanceIndex`

## Finitions

La render pass peut ensuite être terminée :

```c++
vkCmdEndRenderPass(commandBuffer);
```

Et nous avons fini l'enregistrement du command buffer :

```c++
if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("échec de l'enregistrement du command buffer!");
}
```

Dans le prochain chapitre nous écrirons le code pour la boucle principale. Elle récupérera une image de la swap chain,
enregistrera et exécutera un command buffer et retournera l'image complète à la swap chain.

[Code C++](/code/14_command_buffers.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
