## Introduction

Notre application nous permet maintenant d'afficher correctement un triangle, mais certains cas de figures ne sont pas
encore correctement gérés. Il est possible que la surface d'affichage soit redimensionnée  par l'utilisateur et que la
swap chain ne soit plus parfaitement compatible. Nous devons faire en sorte d'être informés de tels changements pour 
pouvoir recréer la swap chain.

## Recréer la swap chain

Créez la fonction `recreateSwapChain` qui appelle `createSwapChain` et toutes les fonctions de création d'objets
dépendants de la swap chain ou de la taille de la fenêtre.

```c++
void recreateSwapChain() {
    vkDeviceWaitIdle(device);

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers();
}
```

Nous appelons d'abord `vkDeviceIdle` car nous ne devons surtout pas toucher à des ressources en cours d'utilisation. La
première chose à faire est bien sûr de recréer la swap chain. Les image views doivent être recrées également car 
elles dépendent des images de la swap chain. La render pass doit être recrée car elle dépend du format des images de
la swap chain. Il est rare que le format des images de la swap chain soit altéré mais il n'est pas officiellement
garanti qu'il reste le même, donc nous gérerons ce cas là. La pipeline dépend de la taille des images pour la
configuration des rectangles de viewport et de ciseau, donc nous devons recréer la pipeline graphique. Il est possible
d'éviter cela en faisant de la taille de ces rectangles des états dynamiques. Finalement, les framebuffers et les 
command buffers dépendent des images de la swap chain.

Pour être certains que les anciens objets sont bien détruits avant d'en créer de nouveaux, nous devrions créer une
fonction dédiée à cela et que nous appellerons depuis `recreateSwapChain`. Créez donc `cleanupSwapChain` :

```c++
void cleanupSwapChain() {

}

void recreateSwapChain() {
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers();
}
```

Nous allons déplacer le code de suppression depuis `cleanup` jusqu'à `cleanupSwapChain` :

```c++
void cleanupSwapChain() {
    for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
    }

    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}
```

Nous pouvons ensuite appeler cette nouvelle fonction depuis `cleanup` pour éviter la redondance de code :

```c++
void cleanup() {
    cleanupSwapChain();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugReportCallbackEXT(instance, callback, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}
```

Nous pourrions recréer la command pool à partir de rien mais ce serait du gâchis. J'ai préféré libérer les command
buffers existants à l'aide de la fonction `vkFreeCommandBuffers`. Nous pouvons de cette manière réutiliser la même
command pool mais changer les command buffers.

Pour bien gérer le redimensionnement de la fenêtre nous devons récupérer la taille actuelle du framebuffer qui lui est
associé pour s'assurer que les images de la swap chain ont bien la nouvelle taille. Pour cela changez 
`chooseSwapExtent` afin que cette fonction prenne en compte la nouvelle taille réelle :

```c++
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        ...
    }
}
```

C'est tout ce que nous avons à faire pour recréer la swap chain! Le problème cependant est que nous devons arrêter
complètement l'affichage pendant la recréation alors que nous pourrions éviter que les frames en vol soient perdues. 
Pour cela vous devez passer l'ancienne swap chain en paramètre à `oldSwapChain` dans la structure 
`VkSwapchainCreateInfoKHR` et détruire cette ancienne swap chain dès que vous ne l'utilisez plus.

## Swap chain non-optimales ou dépassées

Nous devons maintenant déterminer quand recréer la swap chain et donc quand appeler `recreateSwapChain`. Heureusement
pour nous Vulkan nous indiquera quand la swap chain n'est plus adéquate au moment de la présentation. Les fonctions 
`vkAcquireNextImageKHR` et `vkQueuePresentKHR` peuvent pour cela retourner les valeurs suivantes :

* `VK_ERROR_OUT_OF_DATE_KHR` : la swap chain n'est plus compatible avec la surface de fenêtre et ne peut plus être
utilisée pour l'affichage, ce qui arrive en général avec un redimensionnement de la fenêtre
* `VK_SUBOPTIMAL_KHR` : la swap chain peut toujours être utilisée pour présenter des images avec succès, mais les
caractéristiques de la surface de fenêtre ne correspondent plus à celles de la swap chain

```c++
VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("échec de la présentation d'une image à la swap chain!");
}
```

Si la swap chain se trouve être dépassée quand nous essayons d'acquérir une nouvelle image il ne nous est plus possible
de présenter un quelconque résultat. Nous devons de ce fait aussitôt recréer la swap chain et tenter la présentation
avec la frame suivante.

Vous pouvez aussi décider de recréer la swap chain si sa configuration n'est plus optimale, mais j'ai choisi de ne pas
le faire ici car nous avons de toute façon déjà acquis l'image. Ainsi `VK_SUCCES` et `VK_SUBOPTIMAL_KHR` sont considérés
comme des indicateurs de succès.

```c++
result = vkQueuePresentKHR(presentQueue, &presentInfo);

if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    recreateSwapChain();
} else if (result != VK_SUCCESS) {
    throw std::runtime_error("échec de la présentation d'une image!");
}

currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
```

La fonction `vkQueuePresentKHR` retourne les mêmes valeurs avec la même signification. Dans ce cas nous recréons la
swap chain si elle n'est plus optimale car nous voulons les meilleurs résultats possibles.

## Explicitement gérer les redimensionnements

Bien que la plupart des drivers émettent automatiquement le code `VK_ERROR_OUT_OF_DATE_KHR` après qu'une fenêtre est
redimensionnée, cela n'est pas garanti par le standard. Par conséquent nous devons explictement gérer ces cas de
figure. Ajoutez une nouvelle variable qui indiquera que la fenêtre a été redimensionnée :

```c++
std::vector<VkFence> inFlightFences;
size_t currentFrame = 0;

bool framebufferResized = false;
```

La fonction `drawFrame` doit ensuite être modifiée pour prendre en compte cette nouvelle variable :

```c++
if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
    framebufferResized = false;
    recreateSwapChain();
} else if (result != VK_SUCCESS) {
    ...
}
```

Il est important de faire cela après `vkQueuePresentKHR` pour que les sémaphores soient dans un état correct. Pour
détecter les redimensionnements de la fenêtre nous n'avons qu'à mettre en place `glfwSetFrameBufferSizeCallback`
qui nous informera d'un changement de la taille associée à la fenêtre :

```c++
void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {

}
```

Nous devons utiliser une fonction statique car GLFW ne sait pas correctement appeler une fonction membre d'une classe
avec `this`.

Nous récupérons une référence à la `GLFWwindow` dans la fonction de rappel que nous fournissons. De plus nous pouvons
paramétrer un pointeur de notre choix qui sera accessible à toutes nos fonctions de rappel. Nous pouvons y mettre la 
classe elle-même.

```c++
window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
glfwSetWindowUserPointer(window, this);
glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
```

De cette manière nous pouvons changer la valeur de la variable servant d'indicateur des redimensionnements :

```c++
static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}
```

Lancez maintenant le programme et changez la taille de la fenêtre pour voir si tout se passe comme prévu.

## Gestion de la minimisation de la fenêtre

Il existe un autre cas important où la swap chain peut devenir invalide : si la fenêtre est minimisée. Ce cas est
particulier car il résulte en un framebuffer de taille `0`. Dans ce tutoriel nous mettrons en pause le programme
jusqu'à ce que la fenêtre soit remise en avant-plan. À ce moment-là nous recréerons la swap chain.

```c++
void recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    ...
}
```

L'appel initial à `glfwGetFramebufferSize` prend en charge le cas où la taille est déjà correcte et `glfwWaitEvents` n'aurait rien à attendre.

Félicitations, vous avez codé un programme fonctionnel avec Vulkan! Dans le prochain chapitre nous allons supprimer les 
sommets du vertex shader et mettre en place un vertex buffer.

[Code C++](/code/16_swap_chain_recreation.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
