# gdpacman

This is a tool for Godot addons.

## Contents

- [Usage](#usage)
    - [For addon developers](#for-addon-developers)
- [Installation](#installation)

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