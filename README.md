Vulkan tutorial
===============

This repository hosts the contents of [vulkan-tutorial.com](https://vulkan-tutorial.com).
The website itself is based on [daux.io](https://github.com/justinwalsh/daux.io),
which supports [GitHub flavored Markdown](https://help.github.com/articles/basic-writing-and-formatting-syntax/).
A few changes were made to daux.io and its themes, which are included in
`daux.patch` and are licensed as [MIT](https://opensource.org/licenses/MIT). The
patch is based on commit `d45ccff`.

Use issues and pull requests to provide feedback related to the website. If you
have a problem with your code, then use the comments section in the related
chapter to ask a question. Please provide your operating system, graphics card,
driver version, source code, expected behaviour and actual behaviour

Changing code across chapters
-----------------------------

It is sometimes necessary to change code that is reused across many chapters,
for example the `VDeleter` class or a function like `createBuffer`. If you make
such a change, then you should update the code files using the following steps:

* Update any chapters that reference the modified code.
* Make a copy of the first file that uses it and modify the code there, e.g.
`base_code_fixed.cpp`.
* Create a patch using
`diff -Naur base_code.cpp base_code_fixed.cpp > patch.txt`.
* Apply the patch to the specified code file and all files in later chapters
using the `incremental_patch.sh` script. Run it like this:
`./incremental_patch.sh base_code.cpp patch.txt`.
* Clean up the `base_code_fixed.cpp` and `patch.txt` files.
* Commit.

Generating the tutorial
-----------------------------

To generate the tutorial, run `daux.phar` against your copy of the
documentation. Doing this requires installing daux and patching it with some
tweaks needed for this tutorial.

### Prerequisites

1. Make sure [PHP](http://php.net/downloads.php) is installed (Daux is written
   in PHP)
    1. Both the `php_mbstring` and `php_openssl` extensions need to be enabled
    2. The `phar.readonly` setting needs to be set to `Off` (to be able to
	   rebuild Daux)
2. Make sure [Composer](https://getcomposer.org/) is installed, a php dependency
   manager that Daux uses

### Clone, patch, and rebuild daux

1. Clone [daux](https://github.com/justinwalsh/daux.io)
    * `git clone https://github.com/justinwalsh/daux.io.git`
2. Make a new branch at the older revision that the VulkanTutorial patch is
   against:
    * `git checkout d45ccff -b vtpatch`
    * Making a new branch isn't strictly necessary, as you could reset `master`,
      but this keeps master intact.
3. Copy over the `daux.patch` file into the daux.io directory, make sure line
   endings are UNIX style (in case you're using Windows), and apply the patch.
   It should apply cleanly.
    * `git am daux.patch`
4. Run composer in the daux.io directory so that it downloads the dependencies
   Daux needs in order to be built
    * `composer install`
5. Rebuild Daux
    * `php bin/compile`
    * And then copy the newly made `bin/daux.phar` to the base directory so you
      don't accidently use the old one

### Using Daux to generate the tutorial

Assuming the daux.io and VulkanTutorial directories are next to each other, go
into the VulkanTutorial directory and run a command similar to:
`php ../daux.io/generate -s . -d out -f html`.

It should genenerate all of the documentation into the `out` directory. The `-s`
option tells Daux the source material is in the current directory. `-f` controls
the output format.

License
-------

The contents of this repository are licensed as [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/),
unless stated otherwise. By contributing to this repository, you agree to license
your contributions to the public under that same license.

The code listings in the `code` directory are licensed as [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/).
By contributing to that directory, you agree to license your contributions to
the public under that same public domain-like license.
