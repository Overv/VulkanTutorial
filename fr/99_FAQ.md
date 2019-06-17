Cette page liste quelques problèmes que vous pourriez rencontrer lors du développement d'une application Vulkan.

* **J'obtiens un erreur de violation d'accès dans les validations layers** : assurez-vous que MSI Afterburner /
RivaTuner Statistics Server ne tournent pas, car ils possèdent des problèmes de compatibilité avec Vulkan.

* **Je ne vois aucun message provenant des validation layers** : assurez-vous d'abord que les validation layers peuvent
écrire leurs message en laissant le terminal ouvert après l'exécution. Avec Visual Studio, lancez le programme avec
Ctrl-F5. Sous Linux, lancez le programme depuis un terminal. S'il n'y a toujours pas de message, revoyez l'installation
du SDK [ici](https://vulkan.lunarg.com/doc/sdk/1.0.61.0/windows/getting_started.html).

* **vkCreateSwapchainKHR induit une erreur dans SteamOverlayVulkanLayer64.dll** : Il semble qu'il y ait un problème de
compatibilité avec la version beta du client Steam. Il y a quelques moyens de régler le conflit :
    * Désinstaller Steam
    * Mettre la variable d'environnement `DISABLE_VK_LAYER_VALVE_steam_overlay_1` à `1`
    * Supprimer la layer de Steam dans le répertoire sous `HKEY_LOCAL_MACHINE\SOFTWARE\Khronos\Vulkan\ImplicitLayers`

Exemple pour la variable :

![](/images/steam_layers_env.png)