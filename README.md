# imtui [![Actions Status](https://github.com/ggerganov/imtui/workflows/CI/badge.svg)](https://github.com/ggerganov/imtui/actions)

ImTui is an immediate mode text-based user interface library. Supports 256 ANSI colors and mouse/keyboard input. 

[![imtui-sample](https://asciinema.org/a/JsUQsJyCchqlsQzm1P0CN4OJU.svg)](https://asciinema.org/a/JsUQsJyCchqlsQzm1P0CN4OJU)

<a href="https://i.imgur.com/4370FJt.png" target="_blank">![imtui-screenshot-0](https://i.imgur.com/4370FJt.png)</a>

<a href="https://i.imgur.com/IQNIIbB.png" target="_blank">![imtui-screenshot-1](https://i.imgur.com/IQNIIbB.png)</a>

## Live demo in the browser

Eventhough this library is supposed to be used in the terminal, for convenience here is an [Emscripten](https://emscripten.org) build to demonstrate what it would look like, by simulating a console in the browser:

- Demo 0: [imtui.ggerganov.com](https://imtui.ggerganov.com/) 
- Demo 1: [hnterm.ggerganov.com](https://hnterm.ggerganov.com/)

Note: the demos work best with **Chrome**

## Details

This library is 99.9% based on the popular [Dear ImGui](https://github.com/ocornut/imgui) library. ImTui simply provides an [ncurses](https://en.wikipedia.org/wiki/Ncurses) interface in order to draw and interact with widgets in the terminal. The entire Dear ImGui interface is available out-of-the-box.

For basic usage of ImTui, check one of the available samples:

- [example-ncurses0](https://github.com/ggerganov/imtui/blob/master/examples/ncurses0/main.cpp)
- [example-emscripten0](https://github.com/ggerganov/imtui/blob/master/examples/emscripten0/main.cpp)
- [hnterm](https://github.com/ggerganov/hnterm) - a simple tool to browse Hacker News in the terminal

*Note: the current implementation is experimental, so don't expect all things to work.*

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

### Emscripten:

```bash
git clone https://github.com/ggerganov/imtui --recursive
cd imtui
mkdir build && cd build
emconfigure cmake ..
make
```
