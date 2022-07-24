imtui
=====
[![Actions Status](https://github.com/ggerganov/imtui/workflows/CI/badge.svg)](https://github.com/ggerganov/imtui/actions)
[![ImTui v1.0.4 badge][changelog-badge]][changelog]
[![Dear ImGui version badge][imgui-version-badge]](https://github.com/ocornut/imgui)

ImTui is an immediate mode text-based user interface library. Supports 256 ANSI colors and mouse/keyboard input.

<p align="center">
  <a href="https://asciinema.org/a/JsUQsJyCchqlsQzm1P0CN4OJU">
    <img alt="ImTui basic" src="https://media.giphy.com/media/AcKDr9ZyW3RWyNZRg1/giphy.gif"></img>  
  </a>
  <br>
  <i>A very basic ImTui example</i>
</p>

---

<p align="center">
  <a href="https://slack.ggerganov.com">
    <img alt="Slack client" src="https://user-images.githubusercontent.com/1991296/180660513-e9471200-11b1-4e79-bec0-e2d313dfd6a6.gif"></img>  
  </a>
  <br>
  <i>Text-based client for <a href="https://slack.com">Slack</a></i>
</p>

---

<p align="center">
  <a href="https://imtui.ggerganov.com">
    <img alt="Tables" src="https://user-images.githubusercontent.com/1991296/140774086-285cb34f-0851-47b0-82e5-2e8a5bf174ac.gif"></img>  
  </a>
  <br>
  <i>Tables example</i>
</p>

---

<p align="center">
  <a href="https://github.com/ggerganov/hnterm">
    <img alt="HNTerm" src="https://user-images.githubusercontent.com/1991296/131371951-3af42be8-657e-4542-a46a-0370cfc431d8.gif"></img>  
  </a>
  <br>
  <i>Text-based client for <a href="https://news.ycombinator.com/news">Hacker News</a></i>
</p>

---

<p align="center">
  <a href="https://asciinema.org/a/VUKWZM70PxRCHueyPFXy9smU8">
    <img alt="WTF util" src="https://asciinema.org/a/VUKWZM70PxRCHueyPFXy9smU8.svg"></img>  
  </a>
  <br>
  <i>Text-based configuration editor for the <a href="https://wtfutil.com/">WTF Dashboard</a></i>
</p>

## Live demo in the browser

Even though this library is supposed to be used in the terminal, for convenience here is an [Emscripten](https://emscripten.org) build to demonstrate what it looks like, by simulating a console in the browser:

- Demo 0: [imtui.ggerganov.com](https://imtui.ggerganov.com/)
- Demo 1: [hnterm.ggerganov.com](https://hnterm.ggerganov.com/)
- Demo 2: [wtf-tui.ggerganov.com](https://wtf-tui.ggerganov.com/)
- Demo 3: [slack.ggerganov.com](https://slack.ggerganov.com/)

Note: the demos work best with **Chrome**

## Details

This library is 99.9% based on the popular [Dear ImGui](https://github.com/ocornut/imgui) library. ImTui simply provides an [ncurses](https://en.wikipedia.org/wiki/Ncurses) interface in order to draw and interact with widgets in the terminal. The entire Dear ImGui interface is available out-of-the-box.

For basic usage of ImTui, check one of the available samples:

- [example-ncurses0](https://github.com/ggerganov/imtui/blob/master/examples/ncurses0/main.cpp)
- [example-emscripten0](https://github.com/ggerganov/imtui/blob/master/examples/emscripten0/main.cpp)
- [hnterm](https://github.com/ggerganov/hnterm) - a simple tool to browse Hacker News in the terminal
- [wtf-tui](https://github.com/ggerganov/wtf-tui) - text-based UI for configuring the WTF terminal dashboard
- [slack](https://github.com/ggerganov/imtui/blob/master/examples/slack) - text-based mock UI for Slack

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
