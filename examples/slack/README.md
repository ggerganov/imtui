# slack

Text-based UI (TUI) for a [Slack](https://slack.com) client

<p align="center">
  <a href="https://slack.ggerganov.com">
    <img alt="Slack client" src="https://user-images.githubusercontent.com/1991296/180660513-e9471200-11b1-4e79-bec0-e2d313dfd6a6.gif"></img>  
  </a>
  <br>
  <i>Running the example in a terminal</i>
</p>

![image](https://user-images.githubusercontent.com/1991296/181059726-0df8657a-d0c7-48c4-97d2-583d9f1aa530.png)

High quality video on Youtube: https://youtu.be/RJRGfvKEgEw

Coding timelapse: https://youtu.be/cEqNYesDCXg

## Live demo in the browser

The following page runs an Emscripten port of the application for demonstration purposes:

[slack.ggerganov.com](https://slack.ggerganov.com/) *(not suitable for mobile devices)*

## Details

This is mock UI for a Slack client that runs in your terminal. The business logic of the client is completely missing. The purpose of this example is
to demonstrate the capabilities for the ImTui library.

## Building

###  Linux and Mac:

```bash
git clone https://github.com/ggerganov/imtui --recursive
cd imtui
mkdir build && cd build
cmake ..
make

./bin/slack
```
