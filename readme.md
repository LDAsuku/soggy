# Soggy

Experimental server emulator for a game I forgot its name 💀

![soggy cat](static/soggy_cat.png "soggy cat")

[Setup & Documentation](https://nitter.pussthecat.org/sillysoggycat/)

## Building on GNU/Linux

```sh
# install dependencies (ubuntu)
apt install build-essential cmake libprotobuf-dev protobuf-compiler liblua5.3-dev
# install dependencies (arch linux)
pacman -S cmake protobuf lua
# setup for build
cmake -B build
# build
cmake --build build -j8
```

## Building with Visual Studio on Microsoft Windows

Make sure you've selected "C++ CMake tools for Windows" in the Visual Studio installer.

Install vcpkg [according to their documentation](https://vcpkg.io/en/getting-started.html).

```powershell
# install dependencies (vcpkg)
vcpkg install protobuf lua[cpp]:x64-windows
```

Open the folder in Visual Studio and build it.

## Building with MSYS2/MinGW on Microsoft Windows

Use the MINGW64 terminal.

```sh
# install dependencies
pacman -S ${MINGW_PACKAGE_PREFIX}-{toolchain,cmake,protobuf,lua}
# prepare for build
cmake -B build -G "Unix Makefiles"
# build
cmake --build build -j8
```

## Running

Put the `resources` directory and `soggy.cfg` in the current working directory and run soggy. Enter `help` in the interactive prompt to see a list of commands.

`dispatch.py` is obsolete. The game server has a built-in dispatch server now.

Resources can be found on the repo wiki.

## Client patches

```
=== GS.exe
# disable AC
(VA=0x140ef5080, fileoffset=0xef4480) = c3

=== GS_Data/Native/UserAssembly.dll
# sprint nocd
(VA=0x1802d1ef0, fileoffset=0x2d12f0) = b0 01 c3
# skill nocd
(VA=0x181ac998c, fileoffset=0x1ac8d8c) = 66 0f ef ff
# skip update checks for slightly faster load time
(VA=0x18213221b, fileoffset=0x213161b) = 90 90 90 90 90 90
# kill paimon
(VA=0x182083430, fileoffset=0x2082830) = c3
```
