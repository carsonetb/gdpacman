# gdpacman

This is a prototype tool for Godot addons.

## Contents

- [Contents](#contents)
- [Features](#features)
- [Usage](#usage)
    - [For addon developers](#for-addon-developers)
    - [Folder for this addon already exists message](#folder-for-this-addon-already-exists-message)
    - [.deps format](#deps-format)
- [Installation](#installation)
    - [Building on Linux](#building-on-linux)
    - [Building for Windows using MinGW](#building-for-windows-using-mingw)
- [Development](#development)

## Features

What is the point of this tool? Mainly 
dependencies. Sometimes you want to make 
an addon that uses another one, so you don't
have to reinvent the wheel. And while you *could*
include that addon inside of your own project, 
the user could end up installing multiple of 
the same addon, which wouldn't be good.

Here are the features:

- Dependency management
- Simple plugin installation
- Remove addons easily
- (Soon) automatically update all your addons

## Usage

This section assumes you have gdpacman globally
installed, so you can just run it with the 
`gdpacman` command. Otherwise you can replace
that with the path to the executable (e.g.
`../build/gdpacman`)

If you ever need it you can get help with:

```
gdpacman -h
# OR
gdpacman --help
```

By default the working directory will be used
for the project path. If you need to use a 
different path you can use `p` or `--project`,
for example:

```
gdpacman -p ../project
```

Of course the above command won't do anything
without other commands. First you should init
your project:

```
gdpacman --init
```

This will create the `.deps` file in your 
project's root.

If you want to install
a package you can use the `-u` or `--url` 
flags, along with all the Git urls to add.
Any of these is valid:

```
gdpacman -u https://github.com/coppolaemilio/dialogic
# OR
gdpacman -u https://github.com/viniciusgerevini/godot-aseprite-wizard https://github.com/coppolaemilio/dialogic
```

You can also specify a specific branch. After 
the URL add `::{branch}`. For example:

```
gdpacman -u https://codeberg.org/godotsteam/godotsteam::gdextension-plugin
```

Once packages are installed they can be removed
using the `-r` or `--remove` flag, and the name 
of the package:

```
gdpacman -r dialogic
# OR
gdpacman -r dialogic godot-aseprite-wizard
```

### Folder for this addon already exists message

If you get this message, it means either you 
requested this addon to be installed or it was in
a dependency, but the addon already exists. This
could be caused simply by the addon already being
installed, or maybe two addons use the same 
dependency. Either way, it's usually safe to choose
either option, but if you choose yes and the module
was outdated, it will be updated.

### For addon developers

You may add dependencies as an addon developer 
just like normal, and they will be added when the 
user adds your addon (recursively). The only thing
you have to do is register your addon folder, so
the installer knows which folder is your addon
and which is just a dependency (which will not be
moved over). You can do this by running:

```
gdpacman --register {addon_folder_name}
```

`{addon_folder_name}` is the name of the folder
that is in the addons directory 
(`addons/{addon_folder_name}`).

## Installation

This project uses the [conan](https://conan.io/)
package manager, because it makes it very easy 
to export to both Windows and Linux.

### Building on Linux

If you have just installed conan, you can 
initialize the build profile. Some of these things
(like the release flag), we will be overriding.

```
conan profile detect --force
```

Now you can install dependencies (specified in
`conanfile.txt`):

```
conan install . --build=missing -s build_type=Debug
```

Instead of the `Debug` build type you can also
use `Release`. Conan will generate a preset you 
can run:

```
cmake --preset conan-debug
```

You can then build:

```
cd build/Debug
cmake --build .
```

And optionally install:

```
sudo cmake --install .
```

### Building for Windows using MinGW

We will use MinGW to build for Windows. So
you should install the `mingw-w64` package
on whatever distribution you are using.

Create a MinGW profile (this will create
a text file in /home/user/.conan2/profiles
called mingw):

```
conan profile detect --name mingw
```

Open the file it creates and add this:

```
[settings]
arch=x86_64
build_type=Release
compiler=gcc
compiler.cppstd=gnu17
compiler.libcxx=libstdc++11
compiler.version=15
os=Windows

[options]
boost/*:without_stacktrace=True

[buildenv]
CC=x86_64-w64-mingw32-gcc
CXX=x86_64-w64-mingw32-g++
AR=x86_64-w64-mingw32-ar
RANLIB=x86_64-w64-mingw32-ranlib
RC=x86_64-w64-mingw32-windres
```

Now go back to the project working directory 
and install dependencies:

```
conan install . --profile mingw --build=missing
# OR
conan install . --profile mingw --build=missing -s build_type=Debug
```

This will generate a CMake Preset that you 
can run:

```
cmake --preset conan-release
```

Now build:

```
cd build/Release
cmake --build .
```

## Development

If you want to contribute, you should do so 
by submitting a Pull Request on GitHub with 
your changes. If you don't want to edit my crappy
code, you can submit an Issue and I'll probably
fix it.

Development can be done in probably any software
that supports CMake integration, but I personally
use VSCode with the clangd, clang-tidy, clang++,
Cmake, and Cmake Tools extensions, and everything
works very nicely.