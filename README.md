# ffmpeg-gui

A limited GUI for simple media conversions that sits on top of the [FFmpeg command-line tool](https://www.ffmpeg.org/download.html).

Supports basic conversions using video, audio, and subtitle streams.
Features image preview and tab selection.

(Note: requires [FFmpeg](https://www.ffmpeg.org/download.html) to be installed)

Download the [latest release](https://github.com/Varulli/ffmpeg-gui/releases/latest) or [build it yourself](https://github.com/Varulli/ffmpeg-gui#build-instructions).

![ffmpeg-gui-demo](https://github.com/user-attachments/assets/332d0f82-0879-4c5d-9c65-f492d8801178)

## Build Instructions

### Requirements
- C compiler
- CMake 3.5+

### Steps
```
git clone https://github.com/Varulli/ffmpeg-gui.git
cd ffmpeg-gui
mkdir build
cd build
cmake ..
cmake --build .
```
