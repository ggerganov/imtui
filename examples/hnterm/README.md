*Note: this example has been moved in a standalone repository - https://github.com/ggerganov/hnterm*

*Future development will continue there*

# hnterm

Interactive browsing of [Hacker News](https://news.ycombinator.com/news) in the terminal

[![hnterm-demo](https://asciinema.org/a/gJakwNnEcgmGzZYiYIzFA1n8R.svg)](https://asciinema.org/a/gJakwNnEcgmGzZYiYIzFA1n8R)

<a href="https://i.imgur.com/JMuM62I.png" target="_blank">![hnterm-dark](https://i.imgur.com/JMuM62I.png)</a>

## Live demo in the browser

The Emscripten port of HNTerm uses Emscripten's Fetch API instead of `libcurl` to perform requests to the [HN API](https://github.com/HackerNews/API). 

Demo: [hnterm.ggerganov.com](https://hnterm.ggerganov.com/) *(not suitable for mobile devices)*

## Details

HNTerm is a small console application written in C++ for browsing [Hacker News](https://news.ycombinator.com/news). It queries the official [HN API](https://github.com/HackerNews/API) and interactively displays the current stories and comments. It uses `libcurl` to perform the GET requests to the API. The UI is rendered with [ImTui](https://github.com/ggerganov/imtui). HNTerm fetches only the content that is currently visible on the screen. The window splits allow browsing multiple stories/comment sections at the same time.

## Building

###  Linux and Mac:

```bash
git clone https://github.com/ggerganov/imtui --recursive
cd imtui
mkdir build && cd build
cmake ..
make

./bin/hnterm
```
