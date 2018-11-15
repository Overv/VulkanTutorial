À la différence d'anciens APIs, le code des shaders doit être fourni à Vulkan sous la forme de bytecode et non sous une
forme compréhensible par l'homme, comme [GLSL](https://en.wikipedia.org/wiki/OpenGL_Shading_Language) ou
[HLSL](https://en.wikipedia.org/wiki/High-Level_Shading_Language). Ce format est appelé
[SPIR-V](https://www.khronos.org/spir) et est conçu pour fonctionner avec Vulkan et OpenCL (deux APIs de Khronos). Ce
format peut servir à écrire du code éxécuté sur la carte graphique et pour les graphismes et pour le calcul, mais nous 
nous concentrerons sur la pipeline graphique dans ce tutoriel.

L'avantage d'un tel format est que le compilateur spécifique de la carte graphique a beaucoup moins de travail. 
L'expérience a de plus montré qu'avec les syntaxes compréhensibles par l'homme, certains compilateurs étaient très 
laxistes par rapport à la spécification qui leur était fournie. Si vous écriviez du code complexe, il pouvait être 
accepté par l'une et pas par l'autre, ou pire s'éxécuter différemment. Avec le format de plus bas niveau qu'est 
SPIR-V, ces erreurs seront normalement évitées.

Cela ne veut cependant pas dire que nous devrons écrire ces bytecodes à la main. Khronos fournit lui-même un 
compilateur transformant GLSL en SPIR-V. Ce compilateur standard vérifiera que votre code correspond à la spécification.
Le compilateur est fourni avec le SDK et s'appelle `glslangValidator`, vous n'aurez donc rien de plus à télécharger.

GLSL est un language possédant une syntaxe proche du C. Les programmes y ont une fonction `main` invoquée pour chaque 
objet à traiter. Plutôt que d'utiliser des paramètres et des valeurs de retour, GLSL utilise des variables globales 
pour les entrées et sorties des invocations. Le language possède des fonctionnalités avancées pour aider le travail 
avec les mathématiques nécessaires aux graphismes, avec par exemple des vecteurs, des matrices et des fonctions pour 
les traiter. On y trouve des fonctions pour réaliser des produits vectoriels ou des réflexions d'un vecteurs par 
rapport à un autre. Le type pour le vecteur d'appelle `vec` et est suivi d'un nombre indiquant le nombre d'éléments,
par exemple `vec3`. On peut accéder à ses données comme des membres avec par exemple `.y`, mais aussi créer de nouveaux
vecteurs avec plusieurs indications, par exemple `vec3(1.0, 2.0, 3.0).xz` qui crée un `vec2` égal à `(1.0, 3.0)`. 
Leurs constructeurs peuvent aussi être des combinaisons de vecteurs et de valeurs. Par exemple il est possible de
créer un `vec3` ainsi : `vec3(vec2(1.0, 2.0), 3.0)`.

Comme nous l'avons dit au chapitre précédent, nous devrons écrire un vertex shader et un fragment shader pour pouvoir
afficher un triangle à l'écran. Les deux prochaines sections couvrirons ce travail, puis nous verrons comment créer 
des bytecodes SPIR-V avec ce code.

## Vertex shader

Le vertex shader traite chaque vertex envoyé depuis le programme C++. Il récupère des données telles la position, la
normale, la couleur ou les coordonnées de texture. Ses sorties sont la position du vertex dans l'espace de l'écran et
les autres attributs qui doivent être fournies au reste de la pipeline, comme la couleur ou les coordonnées de texture.
Ces valeurs seront interpolées lors de la rasterization afin de produire un dégradé continu.

Une _clip coordinate_ est un vecteur à quatre éléments émis par le vertex shader. Il est ensuite transformé en une 
_normalized screen coordinate_ en divisant ses trois premiers composants par le quatrième. Ces coordonnées sont des 
[coordonnées homogènes](https://en.wikipedia.org/wiki/Homogeneous_coordinates) qui permettent d'accéder au frambuffer
grâce à un repère de [-1, 1] par [-1, 1]. Il ressemble à cela :

![](/images/normalized_device_coordinates.svg)

Vous devriez déjà être familier de ces notions si vous avez déjà utilisé des graphismes 3D. Si vous avez utilisé 
OpenGL avant vous vous rendrez compte que l'axe Y est maintenenant inversé et que l'axe Z va de 0 à 1, comme Direct3D.

Pour notre premier triangle nous n'appliquerons aucune transformation, nous nous contenterons de spécifier 
directement les coordonnées des trois vertices pour créer la forme suivante :

![](/images/triangle_coordinates.svg)

Nous pouvons directement émettre ces coordonnées en mettant leur quatrième composant à 1 de telle sorte que la 
division ne change pas les valeurs. 

Ces coordonnées devraient normalement être stockées dans un vertex buffer, mais sa création et son remplissage ne 
sont pas des opérations triviales. J'ai donc décidé de retarder ce sujet afin d'obtenir plus rapidement un résultat 
visible à l'écran. Nous ferons ainsi quelque chose de peu orthodoxe en attendant : inclure les coordonnées directement 
dans le vertex shader. Son code ressemble donc à ceci :

```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
```

La fonction `main` est invoquée pour chaque vertex. La variable prédéfinie `gl_VertexIndex` contient l'index du 
vertex à l'origine de l'invocation du `main`. Elle est en général utilisée comme index dans le vertex buffer, mais nous 
l'emploierons pour déterminer la coordonnée à émettre. Cette coordonnée est extraite d'un tableau prédéfini à trois 
entrées, et est combinée avec un `z` à 0.0 et un `w` à 1.0 pour faire de la division une identité. La variable 
prédiéfinie `gl_Position` fonctionne comme sortie pour les coordonnées. L'extension `GL_ARB_separate_shader_objects` 
est requise pour fonctionnner avec Vulkan.

## Fragment shader

Le triangle formé par les positions émises par le vertex shader remplit un certain nombre de fragments. Le fragment 
shader est invoqué pour chacun d'entre eux et produit une couleur et une profondeur, qu'il envoie à un ou plusieurs
framebuffer(s). Un fragment shader colorant tout en rouge est fournit ici :

```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
```

Le `main` est appelé pour chaque fragment de la même manière que le vertex shader est appelé pour chaque vertex. Les 
couleurs sont des vecteurs de quatre composants : R, G, B et le canal alpha. Les valeurs doivent être incluses dans 
[0, 1]. Au contraire de `gl_Position`, il n'y a pas (plus exactement il n'y a plus) de variable prédéfinie dans 
laquelle entrer la valeur de la couleur. Vous devrez spécifier votre propre variable pour contenir la couleur du 
fragment, où `layout(location = 0)` indique l'index du framebuffer où la couleur sera écrite. Ici, la couleur rouge est 
écrite dans `outColor` liée au seul et unique premier framebuffer.

## Une couleur pour chaque vertex

Afficher ce que vous voyez sur cette image ne serait pas plus intéressant qu'un triangle entièrement rouge?

![](/images/triangle_coordinates_colors.png)

Nous devons pour cela faire quelques petits changements aux deux shaders. Spécifions d'abord une couleur distincte 
pour chaque vertex. Ces couleurs seront inscrites dans le vertex shader de la même manière que les positions :

```glsl
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);
```

Nous devons maintenant passer ces couleurs au fragment shader afin qu'il puisse émettre des valeurs interpolées au 
framebuffer. Ajoutez une variable de sortie pour la couleur dans le vertex shader et donnez lui une valeur dans le 
`main`:

```glsl
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

Nous devons ensuite ajouter l'entrée correspondante dans le fragment shader, dont la valeur sera l'interpolation 
correspondant à la position du fragment pour lequel le shader sera invoqué :

```glsl
layout(location = 0) in vec3 fragColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

Les deux variables n'ont pas nécessairement le même nom, elles seront reliées selon l'index fournit par la directive 
`location`. La fonction `main` doit être modifiée pour émettre une couleur possédant un canal alpha. Le résultat 
montré dans l'image précédente est dû à l'interpolation réalisée lors de la rasterization.

## Compilation des shaders

Créez un dossier `shaders` à la racine de votre projet, puis enregistrez le vertex shader dans un fichier appelé
`shader.vert` et le fragment shader dans un fichier appelé `shader.frag`. Les shaders en GLSL n'ont pas d'extension 
officielle mais celles-ci correspondent à l'usage communément accepté.

Le contenu de `shader.vert` devrait être:

```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

Et `shader.frag` devrait contenir :

```glsl
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

Nous allons maintenant compiler ces shaders en bytecode SPIR-V à l'aide du programme `glslangValidator`.

**Windows**

Créez un fichier `compile.bat` et inscrivez ceci à l'intérieur :

```bash
C:/VulkanSDK/1.0.17.0/Bin32/glslangValidator.exe -V shader.vert
C:/VulkanSDK/1.0.17.0/Bin32/glslangValidator.exe -V shader.frag
pause
```

Corrigez le chemin vers `glslangValidator.exe` pour que le .bat pointe effectivement là où le votre se trouve. 
Double-cliquez pour lancer ce script.

**Linux**

Créez un fichier `compile.sh` et inscrivez ceci à l'intérieur :

```bash
/home/user/VulkanSDK/x.x.x.x/x86_64/bin/glslangValidator -V shader.vert
/home/user/VulkanSDK/x.x.x.x/x86_64/bin/glslangValidator -V shader.frag
```

Corrigez le chemin menant au `glslangValidator` pour qu'il pointe là où il faut. Rendez le script exécutable avec la 
commande `chmod +x compile.sh` et lancez-le.

**Fin des instructions spécifiques**

Ces deux commmandes invoquent le programme avec l'argument `-V`. Celui-ci indique au programme de compiler les sources 
GLSL en bytecode SPIR-V, sinon le programme se contenterait de vérifier que les sources correspondent au standard. 
Une fois le script exécuté vous vous retrouverez avec deux nouveaux fichiers : `vert.spv` et `frag.spv`. Les noms 
sont automatiquement dérivés du type de shader, mais vous pouvez les renommer si vous le souhaitez. Vous aurez 
peut-être un message parlant de fonctionnalités manquantes mais vous pouvez l'ignorer sans problème.

Si votre shader contient une erreur de syntaxe le compilateur vous indiquera le problème et la ligne à laquelle il 
apparait. Essayez de retirer un point-virgule et voyez l'efficacité du débugger. Essayez également de voir les 
arguments supportés. Il peut par exemple se forcer à émettre le bytecode sous un format compréhensible permettant de 
voir exactement ce que le shader fait et quelles optimisations le compilateur y a fait.

La compilation des shaders en ligne de commande est l'une des options les plus simples et les plus évidentes. C'est ce
que nous utiliserons dans ce tutoriel, mais il est aussi possible de compiler les shaders depuis votre code. Le SDK
inclue la librairie [libshaderc](https://github.com/google/shaderc) , qui permet de compiler le GLSL en SPIR-V depuis le
programme C++.

## Charger un shader

Maintenant quevous pouvez créer des shaders SPIR-V il est grand temps de les charger dans le programme et de les 
intégrer à la pipeline graphique. Nous allons d'abord écrire une fonction pour assister au chargement des données 
binaires à partir des fichiers.

```c++
#include <fstream>

...

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("échec lors de l'ouverture du fichier!");
    }
}
```

La fonction `readFile` lira tous les octets du fichier qu'on lui indique et les retournera dans un `vertor` de 
charactères servant ici d'octets. Nous ouvrons le fichier deux paramètres particuliers :
* `ate` : permet de commencer la lecture à la fin du fichier
* `binary` : indique que le fichier doit être lu comme des octets et que ceux-ci ne doivent pas être formattés

Commencer la lecture à la fin permet d'utiliser la position du pointeur comme indicateur de la taille totale du 
fichier et nous pouvons ainsi allouer un stockage suffisant :

```c++
size_t fileSize = (size_t) file.tellg();
std::vector<char> buffer(fileSize);
```
Après cela nous revenons au déubut du fichier et lisons tous les octéts d'un coup :

```c++
file.seekg(0);
file.read(buffer.data(), fileSize);
```

Nous pouvons enfin fermer le fichier et retourner les octets :

```c++
file.close();

return buffer;
```

Appelons maintenant cette fonction depuis `createGraphicsPipeline` pour charger les bytecodes des deux shaders :

```c++
void createGraphicsPipeline() {
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");
}
```

Assurez-vous que les shaders soient correctement charger en affichant la taille des fichiers lus depuis votre 
programme et comparez ces valeurs à la taille des fichiers.

## Créer des modeules shader

Avant de passer ce code à la pipeline nous devons en faire un `VkShaderModule`. Créez une fonction 
`createShaderModule` qui fera ce travail.

```c++
VkShaderModule createShaderModule(const std::vector<char>& code) {

}
```

Cette fonction aura comme paramètre le buffer contenant le bytecode et crééra un `VkShaderModule` de ce code.

La création d'un module shader est très simple, nous avons juste à indiquer un pointeur vers le buffer et la taille 
de ce buffer. Ces informations seront inscrites dans la structure `VkShaderModuleCreatInfo`. Le seul problème est que
la taille doit être donnée en octets mais le pointeur sur le code est du type `uint32_t` et non du type `char`. Nous 
devrons donc utiliser `reinterpet_cast` sur notre pointeur. Avec un tel cast vous devez être certain que vos données 
satisfont l'alignement mémoire d'un `uint32_t`. Heuresement pour nous l'objet allocateur de la classe `std::vector` 
s'assure que les données satisfont le pire cas d'alignement.

```c++
VkShaderModuleCreateInfo createInfo = {};
createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
createInfo.codeSize = code.size();
createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
```

Le `VkShaderModule` peut alors être créé en appelant la fonction `vkCreateShaderModule` :


```c++
VkShaderModule shaderModule;
if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("échec lors de la création d'un module shader!");
}
```

Les paramètres sont les mêmes que lors de la création des objets précédents : le logical device, le pointeur sur la 
structure d'informations, le pointeur vers l'allocateur optionnnel et la référence à l'objet créé. Le buffer 
contenant le code peut être libéré immédiatement après l'appel. Retournez enfin le shader module créé :

```c++
return shaderModule;
```

Les modules shader ne sont requis que pendant la création de la pipeline shader, par conséquent nous ne les 
stockerons pas dans des membres données de la classe mais seulement comme des variables dans la fonction 
`createGraphicsPipeline` :

```c++
VkShaderModule vertShaderModule;
VkShaderModule fragShaderModule;
```

Appelez le fonction que nous venons de créer pour charger les modules shader :

```c++
vertShaderModule = createShaderModule(vertShaderCode);
fragShaderModule = createShaderModule(fragShaderCode);
```

Ils doivent être libérés une fois que la pipeline est créée, juste avant que `createGraphicsPipeline` ne retourne. 
Ajoutez ceci à la fin de la fonction :


```c++
    ...
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}
```

## Création des étapes shader

L'objet `VkShaderModule` n'est qu'un simple wrapper autour du bytecode. Les shaders ne sont pas liés l'un à l'autre 
et on ne leur a pas assigné de tâche. Pour indiquer à quelle étape il doivent intervenir nous devons remplir la 
structure `VkPipelineShaderStageCreateInfo`.

Nous allons d'abord remplir cette structure pour le vertex shader, une fois de plus dans la fonction 
`createGraphicsPipeline`.

```c++
VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
```

La première étape, sans compter le membre `sType`, consiste à dire à Vulkan à quelle étape le shader sera utilisé. Il
existe une énumération pour chacune des étapes possibles décrites dans le chapitre précédent.

```c++
vertShaderStageInfo.module = vertShaderModule;
vertShaderStageInfo.pName = "main";
```

Les deux membres suivants indiquent le module contenant le code et la fonction à invoquer. Il est donc possible de 
combiner plusieurs fragment shaders dans un seul module et de les différencier à l'aide de leurs points d'entrée. 
Nous nous contenterons du `main` standard.

Il existe un autre membre, celui-ci optionnel, appelé `pSpecializationInfo`, que nous n'utiliserons pas mais qu'il 
est intéressant d'évoquer. Il vous permet de donner des valeurs à des constantes présentes dans le code du shader. 
Vous pouvez ainsi configurer le comportement d'un shader lors de la création de la pipeline, ce qui est plus efficace
que de le faire pendant l'affichage, car alors le compilateur (sui n'a toujours pas été invoqué!) peut éliminer des 
pants entiers de code sous un `if` vérifiant la valeur d'une constante ainsi configurée. Si vous n'avez aucune 
constante mettez ce paramètre à `nullptr`.

Modifier la structure pour qu'elle corresponde au fragment shader est très simple :

```c++
VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
fragShaderStageInfo.module = fragShaderModule;
fragShaderStageInfo.pName = "main";
```

Intégrez ces deux valeurs dans un tableau que nous utiliserons plus tard et vous aurez fini ce chapitre!

```c++
VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
```

C'est tout ce que nous dirons sur les étapes programmables de la pipeline. Dans le prochain chapitre nous verrons les
étapes à fonction fixée.

[Code C++](/code/09_shader_modules.cpp) /
[Vertex shader](/code/09_shader_base.vert) /
[Fragment shader](/code/09_shader_base.frag)
