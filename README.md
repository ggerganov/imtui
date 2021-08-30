imtui
=====
[![Actions Status](https://github.com/ggerganov/imtui/workflows/CI/badge.svg)](https://github.com/ggerganov/imtui/actions)
[![ImTui v1.0.4 badge][changelog-badge]][changelog]
[![Dear ImGui version badge][imgui-version-badge]](https://github.com/ocornut/imgui)

ImTui is an immediate mode text-based user interface library. Supports 256 ANSI colors and mouse/keyboard input.

[![imtui-sample](https://media.giphy.com/media/AcKDr9ZyW3RWyNZRg1/giphy.gif)](https://asciinema.org/a/JsUQsJyCchqlsQzm1P0CN4OJU)

![hnterm2](https://user-images.githubusercontent.com/1991296/131371951-3af42be8-657e-4542-a46a-0370cfc431d8.gif)

[![wtf-tui-demo](https://asciinema.org/a/VUKWZM70PxRCHueyPFXy9smU8.svg)](https://asciinema.org/a/VUKWZM70PxRCHueyPFXy9smU8)

![image](https://user-images.githubusercontent.com/1991296/131372067-65393d76-5c59-499a-b8d4-0f9c7ea1365a.png)

![image](https://user-images.githubusercontent.com/1991296/131369757-f6411f2b-b629-4d9d-a5b1-e20b6d5c484d.png)

## Live demo in the browser

Even though this library is supposed to be used in the terminal, for convenience here is an [Emscripten](https://emscripten.org) build to demonstrate what it looks like, by simulating a console in the browser:

- Demo 0: [imtui.ggerganov.com](https://imtui.ggerganov.com/)
- Demo 1: [hnterm.ggerganov.com](https://hnterm.ggerganov.com/)
- Demo 2: [wtf-tui.ggerganov.com](https://wtf-tui.ggerganov.com/)

Note: the demos work best with **Chrome**

## Details

This library is 99.9% based on the popular [Dear ImGui](https://github.com/ocornut/imgui) library. ImTui simply provides an [ncurses](https://en.wikipedia.org/wiki/Ncurses) interface in order to draw and interact with widgets in the terminal. The entire Dear ImGui interface is available out-of-the-box.

For basic usage of ImTui, check one of the available samples:

- [example-ncurses0](https://github.com/ggerganov/imtui/blob/master/examples/ncurses0/main.cpp)
- [example-emscripten0](https://github.com/ggerganov/imtui/blob/master/examples/emscripten0/main.cpp)
- [hnterm](https://github.com/ggerganov/hnterm) - a simple tool to browse Hacker News in the terminal
- [wtf-tui](https://github.com/ggerganov/wtf-tui) - text-based UI for configuring the WTF terminal dashboard

## Building

ImTui depends only on `libncurses`

###  Linux and Mac:

```bash
git clone https://github.com/ggerganov/imtui --recursive
cd imtui
mkdir build && cd build
cmake ..
make

./bin/imtui-example-ncurses0
```

### Windows:

Partial Windows support is currently available using MSYS2 + MinGW + PDCurses:

```
# install required packages in an MSYS2 terminal:
pacman -S git cmake make mingw-w64-x86_64-dlfcn mingw-w64-x86_64-gcc mingw-w64-x86_64-pdcurses mingw-w64-x86_64-curl

# build
git clone https://github.com/ggerganov/imtui --recursive
cd imtui
mkdir build && cd build
cmake ..
make

./bin/hnterm.exe
```
![](https://user-images.githubusercontent.com/1991296/103576542-fa5aef80-4edb-11eb-8340-4bd60a1f9fba.gif)
For more information, checkout the following discussion: [#19](https://github.com/ggerganov/imtui/discussions/19)

### Emscripten:

```bash
git clone https://github.com/ggerganov/imtui --recursive
cd imtui
mkdir build && cd build
emconfigure cmake ..
make
```

[changelog]: ./CHANGELOG.md
[changelog-badge]: https://img.shields.io/badge/changelog-ImTui%20v1.0.4-dummy
[imgui-version-badge]: https://img.shields.io/badge/Powered%20by%20Dear%20ImGui-v1.81-blue.svg
