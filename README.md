# imtui

ImTui is an immediate mode text-based user interface library. Support 256 ANSI colors and mouse/keyboard input. 

[![imtui-sample](https://asciinema.org/a/JsUQsJyCchqlsQzm1P0CN4OJU.svg)](https://asciinema.org/a/JsUQsJyCchqlsQzm1P0CN4OJU)

<a href="https://i.imgur.com/4370FJt.png" target="_blank">![imtui-screenshot](https://i.imgur.com/4370FJt.png)</a>

## Live demo in the browser

Eventhough this library is supposed to be used in the terminal, for convenience here is an [Emscripten](https://emscripten.org) build to demonstrate what it would like, by simulating a console in the browser:

Demo: [imtui.ggerganov.com](https://imtui.ggerganov.com/)

## Details

This library is 99.9% based on the popular [Dear ImGui](https://github.com/ocornut/imgui) library. ImTui simply provides an [ncurses](https://en.wikipedia.org/wiki/Ncurses) interface in order to draw and interact with widgets in the terminal. The entire Dear ImGui interface is available out-of-the-box.

For a basic usage of ImTui, check one of the available samples:

- [example_ncurses0](https://github.com/ggerganov/imtui-wip/blob/master/src/example_ncurses0.cpp)
- [example_ncurses1](https://github.com/ggerganov/imtui-wip/blob/master/src/example_ncurses1.cpp)

The current implementation is experimental, so don't expect all things to work.

## Building

ImTui depends only on `libncurses`

###  Linux and Mac:

```
git clone https://github.com/ggerganov/imtui --recursive
cd imtui
mkdir build
cd build
cmake ..
make

./bin/example_ncurses0
```
