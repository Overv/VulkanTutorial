---
title: 中文翻译说明
---

本书源代码托管于 GitHub（[地址](https://github.com/Overv/VulkanTutorial)），使用经过修改的文档生成器 daux.io
（[官方网站](https://daux.io)，[修改版仓库](https://github.com/Overv/daux.io)）来渲染网页，使用 Markdwon 格式。贡献翻译者需要了解
daux.io 的基本使用规则和其支持的 Markdwon 语法。

本书的中文翻译基于 [daux.io 的多语言支持](https://daux.io/Features/Multilanguage.html)，翻译文本放置在 `zh` 文件夹下，文件名保持与英
文版本一致，在 [Front Matter](https://daux.io/Features/Front_Matter.html) 中使用 `title` 属性设置中文标题。

## Markdown 写法要求

中文翻译文本应当符合[中文排版指北](https://github.com/sparanoid/chinese-copywriting-guidelines/blob/master/README.zh-Hans.md)，
有几项额外要求：

- 使用弯引号；
- 中文文本的行内链接的两侧不用额外添加空格，但若链接文本开头或结尾是英文数字等，需在相应位置添加空格；
- 每行不应超过 120 个半角字符长度，除非是代码，URL，标点禁则等必要情形。

行长度限制可用文本编辑器的辅助标尺来提示，如 VSCode 可在其 settings.json 中设置 `"editor.rulers": [120]` 在 120 半角字符行宽的位置显
示一条竖线。

注意，文本以空行分段，没有首行缩进。

## 术语翻译

参考 [OpenGL 3.3 教程的翻译术语对照](https://github.com/cybercser/OpenGL_3_3_Tutorial_Translation/blob/master/%E7%BF%BB%E8%AF%91%E6%9C%AF%E8%AF%AD%E5%AF%B9%E7%85%A7.md)：

- 以[《游戏引擎架构》中英词汇索引表](https://www.cnblogs.com/miloyip/p/GameEngineArchitectureIndex.html)为参考标准；
- 某些术语可以保留英文原文，如使用“uniform buffer”或“uniform 缓冲”而非“统一缓冲区”，“统一的缓冲区”。

<!-- TODO: 列出需要保留英文原文的情形，如 uniform buffer -->
<!-- TODO: 致谢 -->
