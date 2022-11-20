# Soggy

server for that one game

![soggy cat](soggy_cat.png "soggy cat")

[Setup & Documentation](https://nitter.pussthecat.org/sillysoggycat/)

## Building

```sh
# install dependencies (ubuntu)
apt install build-essential cmake pkgconf libenet-dev libprotobuf-dev protobuf-compiler liblua5.3-dev nlohmann-json3-dev libreadline-dev
# install dependencies (arch linux)
pacman -S cmake pkgconf enet protobuf lua53 nlohmann-json readline
# setup for build
cmake -B build
# build
cmake --build build -j8
```
