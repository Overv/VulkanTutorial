## Introduction

Nous avons déjà évoqué les descripteurs dans la partie sur les buffers d'uniformes. Dans ce chapitre nous en verrons un
nouveau type : les *samplers d'image combinés* (*combined image sampler*). Ceux-ci permettent aux shaders d'accéder au
contenu d'images, à travers un sampler.

Nous allons d'abord modifier l'organisation des descripteurs, la pool de descripteurs et le set de descripteurs pour
qu'ils incluent le sampler d'image combiné. Ensuite nous ajouterons des coordonnées de texture à la structure
`Vertex` et modifierons le vertex shader et le fragment shader pour qu'il utilisent les couleurs de la texture.

## Modifier les descripteurs

Trouvez la fonction `createDescriptorSetLayout` et créez une instance de `VkDescriptorSetLayoutBinding`. Cette
structure correspond aux descripteurs d'image combinés. Nous n'avons quasiment que l'indice du binding à y mettre :

```c++
VkDescriptorSetLayoutBinding samplerLayoutBinding{};
samplerLayoutBinding.binding = 1;
samplerLayoutBinding.descriptorCount = 1;
samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
samplerLayoutBinding.pImmutableSamplers = nullptr;
samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
VkDescriptorSetLayoutCreateInfo layoutInfo{};
layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
layoutInfo.pBindings = bindings.data();
```

Assurez-vous également de bien indiquer le fragment shader dans le champ `stageFlags`. Ce sera à cette étape que la
couleur sera extraite de la texture. Il est également possible d'utiliser le sampler pour échantilloner une texture dans
le vertex shader. Cela permet par exemple de déformer dynamiquement une grille de vertices pour réaliser une
[heightmap](https://en.wikipedia.org/wiki/Heightmap) à partir d'une texture de vecteurs.

Si vous lancez l'application, vous verrez que la pool de descripteurs ne peut pas allouer de set avec l'organisation que
nous avons préparée, car elle ne comprend aucun descripteur de sampler d'image combiné. Il nous faut donc modifier la
fonction `createDescriptorPool` pour qu'elle inclue une structure `VkDesciptorPoolSize` qui corresponde à ce type de
descripteur :

```c++
std::array<VkDescriptorPoolSize, 2> poolSizes{};
poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

VkDescriptorPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
poolInfo.pPoolSizes = poolSizes.data();
poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());
```

La dernière étape consiste à lier l'image et le sampler aux descripteurs du set de descripteurs. Allez à la fonction
`createDescriptorSets`.

```c++
for (size_t i = 0; i < swapChainImages.size(); i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureImageView;
    imageInfo.sampler = textureSampler;

    ...
}
```

Les ressources nécessaires à la structure paramétrant un descripteur d'image combiné doivent être fournies dans
une structure de type `VkDescriptorImageInfo`. Cela est similaire à la création d'un descripteur pour buffer. Les objets
que nous avons créés dans les chapitres précédents s'assemblent enfin!

```c++
std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[0].dstSet = descriptorSets[i];
descriptorWrites[0].dstBinding = 0;
descriptorWrites[0].dstArrayElement = 0;
descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
descriptorWrites[0].descriptorCount = 1;
descriptorWrites[0].pBufferInfo = &bufferInfo;

descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrites[1].dstSet = descriptorSets[i];
descriptorWrites[1].dstBinding = 1;
descriptorWrites[1].dstArrayElement = 0;
descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
descriptorWrites[1].descriptorCount = 1;
descriptorWrites[1].pImageInfo = &imageInfo;

vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
```

Les descripteurs doivent être mis à jour avec des informations sur l'image, comme pour les buffers. Cette fois nous
allons utiliser le tableau `pImageInfo` plutôt que `pBufferInfo`. Les descripteurs sont maintenant prêts à l'emploi.

## Coordonnées de texture

Il manque encore un élément au mapping de textures. Ce sont les coordonnées spécifiques aux sommets. Ce sont elles qui
déterminent les coordonnées de la texture à lier à la géométrie.

```c++
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};
```

Modifiez la structure `Vertex` pour qu'elle comprenne un `vec2`, qui servira à contenir les coordonnées de texture.
Ajoutez également un `VkVertexInputAttributeDescription` afin que ces coordonnées puissent être accédées en entrée du
vertex shader. Il est nécessaire de les passer du vertex shader vers le fragment shader afin que l'interpolation les
transforment en un gradient.

```c++
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};
```

Dans ce tutoriel nous nous contenterons de mettre une texture sur le carré en utilisant des coordonnées normalisées.
Nous mettrons le `0, 0` en haut à gauche et le `1, 1` en bas à droite. Essayez de mettre des valeurs sous `0` ou au-delà
de `1` pour voir l'addressing mode en action. Vous pourrez également changer le mode dans la création du sampler pour
voir comment ils se comportent.

## Shaders

La dernière étape consiste à modifier les shaders pour qu'ils utilisent la texture et non les couleurs. Commençons par
le vertex shader :

```glsl
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
```

Comme pour les couleurs spécifiques aux vertices, les valeurs `fragTexCoord` seront interpolées dans le carré par
le rasterizer pour créer un gradient lisse. Le résultat de l'interpolation peut être visualisé en utilisant les
coordonnées comme couleurs :

```glsl
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragTexCoord, 0.0, 1.0);
}
```

Vous devriez avoir un résultat similaire à l'image suivante. N'oubliez pas de recompiler les shader!

![](/images/texcoord_visualization.png)

Le vert représente l'horizontale et le rouge la verticale. Les coins noirs et jaunes confirment la normalisation des
valeurs de `0, 0` à `1, 1`. Utiliser les couleurs pour visualiser les valeurs et déboguer est similaire à utiliser
`printf`. C'est peu pratique mais il n'y a pas vraiment d'autre option.

Un descripteur de sampler d'image combiné est représenté dans les shaders par un objet de type `sampler` placé dans
une variable uniforme. Créez donc une variable `texSampler` :

```glsl
layout(binding = 1) uniform sampler2D texSampler;
```

Il existe des équivalents 1D et 3D pour de telles textures.

```glsl
void main() {
    outColor = texture(texSampler, fragTexCoord);
}
```

Les textures sont échantillonées à l'aide de la fonction `texture`. Elle prend en argument un objet `sampler` et des
coordonnées. Le sampler exécute les transformations et le filtrage en arrière-plan. Vous devriez voir la texture sur le
carré maintenant!

![](/images/texture_on_square.png)

Expérimentez avec l'addressing mode en fournissant des valeurs dépassant `1`, et vous verrez la répétition de texture à
l'oeuvre :

```glsl
void main() {
    outColor = texture(texSampler, fragTexCoord * 2.0);
}
```

![](/images/texture_on_square_repeated.png)

Vous pouvez aussi combiner les couleurs avec celles écrites à la main :

```glsl
void main() {
    outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
}
```

J'ai séparé l'alpha du reste pour ne pas altérer la transparence.

![](/images/texture_on_square_colorized.png)

Nous pouvons désormais utiliser des textures dans notre programme! Cette technique est extrêmement puissante et permet
beaucoup plus que juste afficher des couleurs. Vous pouvez même utiliser les images de la swap chain comme textures et y
appliquer des effets post-processing.

[Code C++](/code/25_texture_mapping.cpp) /
[Vertex shader](/code/25_shader_textures.vert) /
[Fragment shader](/code/25_shader_textures.frag)
