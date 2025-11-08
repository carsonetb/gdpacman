# gdpacman

This is a tool for Godot addons.

## Contents

- [Usage](#usage)
    - [For addon developers](#for-addon-developers)
    - [Folder for this addon already exists message](#folder-for-this-addon-already-exists-message)
- [Installation](#installation)
- [Development](#development)

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
without other commands. If you want to install
a package you can use the `-u` or `--url` 
flags, along with all the Git urls to add.
Any of these is valid:

```
gdpacman -u https://github.com/coppolaemilio/dialogic
# OR
gdpacman -u https://github.com/viniciusgerevini/godot-aseprite-wizard https://github.com/coppolaemilio/dialogic
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

All of the dependencies of this library are 
contained in submodules, so no installation 
of any third party libraries are required.

First you will need to clone the repo:

```
git clone https://github.com/carsonetb/gdpacman.git
cd gdpacman
```

Then add all the submodules:

```
git submodule init
git submodule update --init --recursive
```

Now you can build with cmake.

```
mkdir build
cd build
cmake ..
cmake --build .
```

Optionally you can install:

```
cmake --install .
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