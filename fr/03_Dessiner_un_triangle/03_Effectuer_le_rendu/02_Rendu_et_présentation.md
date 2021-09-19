## Mise en place

Nous en sommes au chapitre où tout s'assemble. Nous allons écrire une fonction `drawFrame` qui sera appelée depuis la
boucle principale et affichera les triangles à l'écran. Créez la fonction et appelez-la depuis `mainLoop` :

```c++
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
}

...

void drawFrame() {

}
```

## Synchronisation

Le fonction `drawFrame` réalisera les opérations suivantes :

* Acquérir une image depuis la swap chain
* Exécuter le command buffer correspondant au framebuffer dont l'attachement est l'image obtenue
* Retourner l'image à la swap chain pour présentation

Chacune de ces actions n'est réalisée qu'avec un appel de fonction. Cependant ce n'est pas aussi simple : les
opérations sont par défaut exécutées de manière asynchrones. La fonction retourne aussitôt que les opérations sont
lancées, et par conséquent l'ordre d'exécution est indéfini. Cela nous pose problème car chacune des opérations que nous
voulons lancer dépendent des résultats de l'opération la précédant.

Il y a deux manières de synchroniser les évènements de la swap chain : les *fences* et les *sémaphores*. Ces deux objets
permettent d'attendre qu'une opération se termine en relayant un signal émis par un processus généré par la fonction à
l'origine du lancement de l'opération.

Ils ont cependant une différence : l'état d'une fence peut être accédé depuis le programme à l'aide de fonctions telles
que `vkWaitForFences` alors que les sémaphores ne le permettent pas. Les fences sont généralement utilisées pour
synchroniser votre programme avec les opérations alors que les sémaphores synchronisent les opérations entre elles. Nous
voulons synchroniser les queues, les commandes d'affichage et la présentation, donc les sémaphores nous conviennent le
mieux.

## Sémaphores

Nous aurons besoin d'un premier sémaphore pour indiquer que l'acquisition de l'image s'est bien réalisée, puis d'un
second pour prévenir de la fin du rendu et permettre à l'image d'être retournée dans la swap chain. Créez deux membres
données pour stocker ces sémaphores :

```c++
VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
```

Pour leur création nous allons avoir besoin d'une dernière fonction `create...` pour cette partie du tutoriel.
Appelez-la `createSemaphores` :

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
    createCommandBuffers();
    createSemaphores();
}

...

void createSemaphores() {

}
```

La création d'un sémaphore passe par le remplissage d'une structure de type `VkSemaphoreCreateInfo`. Cependant cette
structure ne requiert pour l'instant rien d'autre que le membre `sType` :

```c++
void createSemaphores() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
}
```

De futures version de Vulkan ou des extensions pourront à terme donner un intérêt aux membre `flags` et `pNext`, comme
pour d'autres structures. Créez les sémaphores comme suit :

```c++
if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
    vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

    throw std::runtime_error("échec de la création des sémaphores!");
}
```

Les sémaphores doivent être détruits à la fin du programme depuis la fonction `cleanup` :

```c++
void cleanup() {
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
```

## Acquérir une image de la swap chain

La première opération à réaliser dans `drawFrame` est d'acquérir une image depuis la swap chain. La swap chain étant une
extension nous allons encore devoir utiliser des fonction suffixées de `KHR` :

```c++
void drawFrame() {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
}
```

Les deux premiers paramètres de `vkAcquireNextImageKHR` sont le logical device et la swap chain depuis laquelle
récupérer les images. Le troisième paramètre spécifie une durée maximale en nanosecondes avant d'abandonner l'attente
si aucune image n'est disponible. Utiliser la plus grande valeur possible pour un `uint32_t` le désactive.

Les deux paramètres suivants sont les objets de synchronisation qui doivent être informés de la complétion de
l'opération de récupération. Ce sera à partir du moment où le sémaphore que nous lui fournissons reçoit un signal que
nous pouvons commencer à dessiner.

Le dernier paramètre permet de fournir à la fonction une variable dans laquelle elle stockera l'indice de l'image
récupérée dans la liste des images de la swap chain. Cet indice correspond à la `VkImage` dans notre `vector`
`swapChainImages`. Nous utiliserons cet indice pour invoquer le bon command buffer.

## Envoi du command buffer

L'envoi à la queue et la synchronisation de celle-ci sont configurés à l'aide de paramètres dans la structure
`VkSubmitInfo` que nous allons remplir.

```c++
VkSubmitInfo submitInfo{};
submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
submitInfo.waitSemaphoreCount = 1;
submitInfo.pWaitSemaphores = waitSemaphores;
submitInfo.pWaitDstStageMask = waitStages;
```

Les trois premiers paramètres (sans compter `sType`) fournissent le sémaphore indiquant si l'opération doit attendre et
l'étape du rendu à laquelle s'arrêter. Nous voulons attendre juste avant l'écriture des couleurs sur l'image. Par contre
nous laissons à l'implémentation la possibilité d'exécuter toutes les étapes précédentes d'ici là. Notez que chaque
étape indiquée dans `waitStages` correspond au sémaphore de même indice fourni dans `waitSemaphores`.

```c++
submitInfo.commandBufferCount = 1;
submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
```

Les deux paramètres qui suivent indiquent les command buffers à exécuter. Nous devons ici fournir le command buffer
qui utilise l'image de la swap chain que nous venons de récupérer comme attachement de couleur.

```c++
VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
submitInfo.signalSemaphoreCount = 1;
submitInfo.pSignalSemaphores = signalSemaphores;
```

Les paramètres `signalSemaphoreCount` et `pSignalSemaphores` indiquent les sémaphores auxquels indiquer que les command
buffers ont terminé leur exécution. Dans notre cas nous utiliserons notre `renderFinishedSemaphore`.

```c++
if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
    throw std::runtime_error("échec de l'envoi d'un command buffer!");
}
```

Nous pouvons maintenant envoyer notre command buffer à la queue des graphismes en utilisant `vkQueueSubmit`. Cette
fonction prend en argument un tableau de structures de type `VkSubmitInfo` pour une question d'efficacité. Le dernier
paramètre permet de fournir une fence optionnelle. Celle-ci sera prévenue de la fin de l'exécution des command
buffers. Nous n'en utilisons pas donc passerons `VK_NULL_HANDLE`.

## Subpass dependencies

Les subpasses s'occupent automatiquement de la transition de l'organisation des images. Ces transitions sont contrôlées
par des *subpass dependencies*. Elles indiquent la mémoire et l'exécution entre les subpasses. Nous n'avons certes
qu'une seule subpasse pour le moment, mais les opérations avant et après cette subpasse comptent aussi comme des
subpasses implicites.

Il existe deux dépendances préexistantes capables de gérer les transitions au début et à la fin de la render pass. Le
problème est que cette première dépendance ne s'exécute pas au bon moment. Elle part du principe que la transition de
l'organisation de l'image doit être réalisée au début de la pipeline, mais dans notre programme l'image n'est pas encore
acquise à ce moment! Il existe deux manières de régler ce problème. Nous pourrions changer `waitStages` pour
`imageAvailableSemaphore` à `VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT` pour être sûrs que la pipeline ne commence pas avant
que l'image ne soit acquise, mais nous perdrions en performance car les shaders travaillant sur les vertices n'ont pas
besoin de l'image. Il faudrait faire quelque chose de plus subtil. Nous allons donc plutôt faire attendre la render
pass à l'étape `VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT` et faire la transition à ce moment. Cela nous donne de
plus une bonne excuse pour s'intéresser au fonctionnement des subpass dependencies.

Celles-ci sont décrites dans une structure de type `VkSubpassDependency`. Créez en une dans la fonction
`createRenderPass` :

```c++
VkSubpassDependency dependency{};
dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
dependency.dstSubpass = 0;
```

Les deux premiers champs permettent de fournir l'indice de la subpasse d'origine et de la subpasse d'arrivée. La valeur
particulière `VK_SUBPASS_EXTERNAL` réfère à la subpass implicite soit avant soit après la render pass, selon que
cette valeur est indiquée dans respectivement `srcSubpass` ou `dstSubpass`. L'indice `0` correspond à notre
seule et unique subpasse. La valeur fournie à `dstSubpass` doit toujours être supérieure à `srcSubpass` car sinon une
boucle infinie peut apparaître (sauf si une des subpasse est `VK_SUBPASS_EXTERNAL`).

```c++
dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.srcAccessMask = 0;
```

Les deux paramètres suivants indiquent les opérations à attendre et les étapes durant lesquelles les opérations à
attendre doivent être considérées. Nous voulons attendre la fin de l'extraction de l'image avant d'y accéder, hors
ceci est déjà configuré pour être synchronisé avec l'étape d'écriture sur l'attachement. C'est pourquoi nous n'avons
qu'à attendre à cette étape.

```c++
dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
```

Nous indiquons ici que les opérations qui doivent attendre pendant l'étape liée à l'attachement de couleur sont celles
ayant trait à l'écriture. Ces paramètres permettent de faire attendre la transition jusqu'à ce qu'elle
soit possible, ce qui correspond au moment où la passe accède à cet attachement puisqu'elle est elle-même configurée
pour attendre ce moment.

```c++
renderPassInfo.dependencyCount = 1;
renderPassInfo.pDependencies = &dependency;
```

Nous fournissons enfin à la structure ayant trait à la render pass un tableau de configurations pour les subpass
dependencies.

## Présentation

La dernière étape pour l'affichage consiste à envoyer le résultat à la swap chain. La présentation est configurée avec
une structure de type `VkPresentInfoKHR`, et nous ferons cela à la fin de la fonction `drawFrame`.

```c++
VkPresentInfoKHR presentInfo{};
presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

presentInfo.waitSemaphoreCount = 1;
presentInfo.pWaitSemaphores = signalSemaphores;
```

Les deux premiers paramètres permettent d'indiquer les sémaphores devant signaler que la présentation peut se dérouler.

```c++
VkSwapchainKHR swapChains[] = {swapChain};
presentInfo.swapchainCount = 1;
presentInfo.pSwapchains = swapChains;
presentInfo.pImageIndices = &imageIndex;
```

Les deux paramètres suivants fournissent un tableau contenant notre unique swap chain qui présentera les images et
l'indice de l'image pour celle-ci.

```c++
presentInfo.pResults = nullptr; // Optionnel
```

Ce dernier paramètre est optionnel. Il vous permet de fournir un tableau de `VkResult` que vous pourrez consulter pour
vérifier que toutes les swap chain ont bien présenté leur image sans problème. Cela n'est pas nécessaire dans notre
cas, car n'utilisant qu'une seule swap chain nous pouvons simplement regarder la valeur de retour de la fonction de
présentation.

```c++
vkQueuePresentKHR(presentQueue, &presentInfo);
```

La fonction `vkQueuePresentKHR` émet la requête de présentation d'une image par la swap chain. Nous ajouterons la
gestion des erreurs pour `vkAcquireNextImageKHR` et `vkQueuePresentKHR` dans le prochain chapitre car une erreur à ces
étapes n'implique pas forcément que le programme doit se terminer, mais plutôt qu'il doit s'adapter à des changements.

Si vous avez fait tout ça correctement vous devriez avoir quelque chose comme cela à l'écran quand vous lancez votre
programme :

![](/images/triangle.png)

Enfin! Malheureusement si vous essayez de quitter proprement le programme vous obtiendrez un crash et un message
semblable à ceci :

![](/images/semaphore_in_use.png)

N'oubliez pas que puisque les opérations dans `drawFrame` sont asynchrones il est quasiment certain que lorsque vous
quittez le programme, celui-ci exécute encore des instructions et cela implique que vous essayez de libérer des
ressources en train d'être utilisées. Ce qui est rarement une bonne idée, surtout avec du bas niveau comme Vulkan.

Pour régler ce problème nous devons attendre que le logical device finisse l'opération qu'il est en train de
réaliser avant de quitter `mainLoop` et de détruire la fenêtre :

```c++
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}
```

Vous pouvez également attendre la fin d'une opération quelconque depuis une queue spécifique à l'aide de la fonction
`vkQueueWaitIdle`. Ces fonction peuvent par ailleurs être utilisées pour réaliser une synchronisation très basique,
mais très inefficace. Le programme devrait maintenant se terminer sans problème quand vous fermez la fenêtre.

## Frames en vol

Si vous lancez l'application avec les validation layers maintenant, vous pouvez soit avoir des erreurs soit vous remarquerez
que l'utilisation de la mémoire augmente, lentement mais sûrement. La raison est que l'application soumet rapidement du
travail dans la fonction `drawframe`, mais que l'on ne vérifie pas si ces rendus sont effectivement terminés.
Si le CPU envoie plus de commandes que le GPU ne peut en exécuter, ce qui est le cas car nous envoyons nos command buffers
de manière totalement débridée, la queue de graphismes va progressivement se remplir de travail à effectuer.
Pire encore, nous utilisons `imageAvailableSemaphore` et `renderFinishedSemaphore`  ainsi que nos command buffers pour
plusieurs frames en même temps.

Le plus simple est d'attendre que le logical device n'aie plus de travail à effectuer avant de lui en envoyer de
nouveau, par exemple à l'aide de `vkQueueIdle` :

```c++
void drawFrame() {
    ...

    vkQueuePresentKHR(presentQueue, &presentInfo);

    vkQueueWaitIdle(presentQueue);
}
```

Cependant cette méthode n'est clairement pas optimale pour le GPU car la pipeline peut en général gérer plusieurs images
à la fois grâce aux architectures massivement parallèles. Les étapes que l'image a déjà passées (par exemple le vertex
shader quand elle en est au fragment shader) peuvent tout à fait être utilisées pour l'image suivante. Nous
allons améliorer notre programme pour qu'il puisse supporter plusieurs images *en vol* (ou *in flight*) tout en
limitant la quantité de commandes dans la queue.

Commencez par ajouter une constante en haut du programme qui définit le nombre de frames à traiter concurentiellement :

```c++
const int MAX_FRAMES_IN_FLIGHT = 2;
```

Chaque frame aura ses propres sémaphores :

```c++
std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
```

La fonction `createSemaphores` doit être améliorée pour gérer la création de tout ceux-là :

```c++
void createSemaphores() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {

            throw std::runtime_error("échec de la création des sémaphores d'une frame!");
        }
}
```

Ils doivent également être libérés à la fin du programme :

```c++
void cleanup() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    }

    ...
}
```

Pour utiliser la bonne paire de sémaphores à chaque fois nous devons garder à portée de main l'indice de la frame en
cours.

```c++
size_t currentFrame = 0;
```

La fonction `drawFrame` peut maintenant être modifiée pour utiliser les bons objets :

```c++
void drawFrame() {
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    ...

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};

    ...

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

    ...
}
```

Nous ne devons bien sûr pas oublier d'avancer à la frame suivante à chaque fois :

```c++
void drawFrame() {
    ...

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
```

En utilisant l'opérateur de modulo `%` nous pouvons nous assurer que l'indice boucle à chaque fois que
`MAX_FRAMES_IN_FLIGHT` est atteint.

Bien que nous ayons pas en place les objets facilitant le traitement de plusieurs frames simultanément, encore
maintenant le GPU traite plus de `MAX_FRAMES_IN_FLIGHT` à la fois. Nous n'avons en effet qu'une synchronisation GPU-GPU
mais pas de synchronisation CPU-GPU. Nous n'avons pas de moyen de savoir que le travail sur telle ou telle frame est
fini, ce qui a pour conséquence que nous pouvons nous retrouver à afficher une frame alors qu'elle est encore en
traitement.

Pour la synchronisation CPU-GPU nous allons utiliser l'autre moyen fourni par Vulkan que nous avons déjà évoqué : les
*fences*. Au lieu d'informer une certaine opération que tel signal devra être attendu avant de continuer, ce que les
sémaphores permettent, les fences permettent au programme d'attendre l'exécution complète d'une opération. Nous allons
créer une fence pour chaque frame :

```c++
std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence> inFlightFences;
size_t currentFrame = 0;
```

J'ai choisi de créer les fences avec les sémaphores et de renommer la fonction `createSemaphores` en
`createSyncObjects` :

```c++
void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("échec de la création des objets de synchronisation pour une frame!");
        }
    }
}
```

La création d'une `VkFence` est très similaire à la création d'un sémaphore. N'oubliez pas de libérer les fences :

```c++
void cleanup() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    ...
}
```

Nous voulons maintenant que `drawFrame` utilise les fences pour la synchronisation. L'appel à `vkQueueSubmit` inclut un
paramètre optionnel qui permet de passer une fence. Celle-ci sera informée de la fin de l'exécution du command buffer.
Nous pouvons interpréter ce signal comme la fin du rendu sur la frame.

```c++
void drawFrame() {
    ...

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("échec de l'envoi d'un command buffer!");
    }
    ...
}
```

La dernière chose qui nous reste à faire est de changer le début de `drawFrame` pour que la fonction attende le rendu de
la frame précédente :

```c++
void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    ...
}
```

La fonction `vkWaitForFences` prend en argument un tableau de fences. Elle attend soit qu'une seule fence soit que
toutes les fences déclarent être signalées avant de retourner. Le choix du mode d'attente se fait selon la valeur du
quatrième paramètre. Avec `VK_TRUE` nous demandons d'attendre toutes les fences, même si cela ne fait bien sûr pas de
différence vu que nous n'avons qu'une seule fence. Comme la fonction `vkAcquireNextImageKHR` cette fonction prend une
durée en argument, que nous ignorons. Nous devons ensuite réinitialiser les fences manuellement à l'aide d'un appel à
la fonction `vkResetFences`.

Si vous lancez le programme maintenant vous allez constater un comportement étrange. Plus rien ne se passe. Nous attendons qu'une fence soit signalée alors qu'elle n'a
jamais été envoyée à aucune fonction. En effet les fences sont par défaut crées dans le
mode non signalé. Comme nous appelons `vkWaitForFences` avant `vkQueueSubmit` notre
première fence va créer une pause infinie. Pour empêcher cela nous devons initialiser
les fences dans le mode signalé, et ce dès leur création :

```c++
void createSyncObjects() {
    ...

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    ...
}
```

La fuite de mémoire n'est plus, mais le programme ne fonctionne pas encore correctement. Si `MAX_FRAMES_IN_FLIGHT` est
plus grand que le nombre d'images de la swapchain ou que `vkAcquireNextImageKHR` ne retourne pas les images dans l'ordre,
alors il est possible que nous lancions le rendu dans une image qui est déjà *en vol*. Pour éviter ça, nous devons pour
chaque image de la swapchain si une frame en vol est en train d'utiliser celle-ci. Cette correspondance permettra de suivre
les images en vol par leur fences respective, de cette façon nous aurons immédiatement un objet de synchronisation à attendre
avant qu'une nouvelle frame puisse utiliser cette image.

Tout d'abord, ajoutez une nouvelle liste nommée `imagesInFlight`:

```c++
std::vector<VkFence> inFlightFences;
std::vector<VkFence> imagesInFlight;
size_t currentFrame = 0;
```

Préparez-la dans `createSyncObjects`:

```c++
void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

    ...
}
```

Initialement aucune frame n'utilise d'image, donc on peut explicitement l'initialiser à *pas de fence*. Maintenant, nous allons modifier
`drawFrame` pour attendre la fin de n'importe quelle frame qui serait en train d'utiliser l'image qu'on nous assigné pour la nouvelle frame.

```c++
void drawFrame() {
    ...

    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // Vérifier si une frame précédente est en train d'utiliser cette image (il y a une fence à attendre)
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Marque l'image comme étant à nouveau utilisée par cette frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    ...
}
```

Parce que nous avons maintenant plus d'appels à `vkWaitForFences`, les appels à `vkResetFences` doivent être **déplacés**. Le mieux reste
de simplement l'appeler juste avant d'utiliser la fence:

```c++
void drawFrame() {
    ...

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("échec de l'envoi d'un command buffer!");
    }

    ...
}
```

Nous avons implémenté tout ce qui est nécessaire à la synchronisation pour certifier qu'il n'y a pas plus de deux frames de travail
dans la queue et que ces frames n'utilise pas accidentellement la même image. Notez qu'il est tout à fait normal pour d'autre parties du code,
comme le nettoyage final, de se reposer sur des mécanismes de synchronisation plus durs comme `vkDeviceWaitIdle`. Vous devriez décider
de la bonne approche à utiliser en vous basant sur vos besoins de performances.

Pour en apprendre plus sur la synchronisation rendez vous sur
[ces exemples complets](https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#swapchain-image-acquire-and-present)
par Khronos.

## Conclusion

Un peu plus de 900 lignes plus tard nous avons enfin atteint le niveau où nous voyons des résultats à l'écran!!
Créer un programme avec Vulkan est clairement un énorme travail, mais grâce au contrôle que cet API vous offre vous
pouvez obtenir des performances énormes. Je ne peux que vous recommander de relire tout ce code et de vous assurer que
vous visualisez bien tout les éléments mis en jeu. Nous allons maintenant construire sur ces acquis pour étendre les
fonctionnalités de ce programme.

Dans le prochain chapitre nous allons voir une autre petite chose nécessaire à tout bon programme Vulkan.

[Code C++](/code/15_hello_triangle.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
