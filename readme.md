Language: EN | [中文](readme_cn.md)

If you're going to chase clout on twitter by posting "leaks" with this, PLEASE credit the project.

# Soggy

Server software implementation for a game I forgot its name 💀

![soggy cat](soggy_cat.png "soggy cat")

[Setup & Documentation](https://nitter.pussthecat.org/sillysoggycat/)

## Building on GNU/Linux

```sh
# install dependencies (ubuntu)
apt install build-essential cmake libprotobuf-dev protobuf-compiler liblua5.3-dev
# install dependencies (arch linux)
pacman -S cmake protobuf lua
# install dependencies (opensuse)
zypper in libprotobuf-mutator-devel libprotobuf-c-devel protobuf-devel liblua5_3-5 lua53-devel
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

Put the `resources` directory in the current working directory and run. Enter `help` in the interactive prompt to see a list of commands.
