Language: [EN](readme.md) | 中文

如果你打算通过发布“爆料”来在某些平台上恰饭或蹭热度，请贡献这个项目。

# Soggy

某款游戏的服务端重实现，不幸的是我忘记了这款游戏的名字 💀

![soggy cat](soggy_cat.png "soggy cat")

[安装 & 文档](https://nitter.pussthecat.org/sillysoggycat/)

## 在GNU/Linux上构建

```sh
# 安装依赖项 (适用于ubuntu)
apt install build-essential cmake libprotobuf-dev protobuf-compiler liblua5.3-dev
# 安装依赖项 (适用于arch linux)
pacman -S cmake protobuf lua
# 安装依赖项 (适用于opensuse)
zypper in libprotobuf-mutator-devel libprotobuf-c-devel protobuf-devel liblua5_3-5 lua53-devel
# 准备构建
cmake -B build
# 构建
cmake --build build -j8
```

## 在Microsoft Windows上使用Visual Studio构建

确保您已在Visual Studio Installer中选择了 "适用于 Windows 的 C++ CMake 工具"

安装vkpkg [根据他们的文档](https://vcpkg.io/en/getting-started.html).

```powershell
# 安装依赖项 (vcpkg)
vcpkg install protobuf lua[cpp]:x64-windows
```

在Visual Studio中打开文件夹并构建它

## 在 Microsoft Windows 上使用 MSYS2/MinGW 构建

使用MINGW64的终端

```sh
# 安装依赖项
pacman -S ${MINGW_PACKAGE_PREFIX}-{toolchain,cmake,protobuf,lua}
# 准备构建
cmake -B build -G "Unix Makefiles"
# 构建
cmake --build build -j8
```

## Running

将 `resources` 目录放入当前工作目录并运行，在命令行中输入 `help` 以查看命令列表