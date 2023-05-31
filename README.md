# libvoyaplayer

## A free cross-platform media player library

Copyright (C) 2021 Adam A. Jammary (Jammary Studio)

libvoyaplayer is a free cross-platform media player library that easily plays your music and video files.

## Music

Supports most popular audio codecs like AAC, FLAC, MP3, Vorbis, WMA and many more.

## Video

Supports most popular video codecs like H.265/HEVC, AV1, DivX, MPEG, Theora, WMV, XviD and many more.

## 3rd Party Libraries

Library | Version | License
------- | ------- | -------
[SDL2](https://www.libsdl.org/) | [2.26.5](https://www.libsdl.org/release/SDL2-2.26.5.tar.gz) | [zlib license](https://www.libsdl.org/license.php)
[SDL2_ttf](https://www.libsdl.org/projects/SDL_ttf/) | [2.20.2](https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.20.2.tar.gz) | [zlib license](https://www.libsdl.org/license.php)
[FFmpeg](https://ffmpeg.org/) | [6.0](https://ffmpeg.org/releases/ffmpeg-6.0.tar.bz2) | [LGPL v.2.1 (GNU Lesser General Public License)](https://ffmpeg.org/legal.html)
[libaom](https://aomedia.googlesource.com/aom/) | [3.6.1](https://storage.googleapis.com/aom-releases/libaom-3.6.1.tar.gz) | [Alliance for Open Media Patent License](https://aomedia.org/license/software-license/)
[zLib](http://www.zlib.net/) | [1.2.13](https://www.zlib.net/zlib-1.2.13.tar.gz) | [zlib license](http://www.zlib.net/zlib_license.html)

## How to build

1. Build the third-party libraries and place the them in a common directory
1. You will also need the [dirent.h](https://github.com/tronkko/dirent/raw/master/include/dirent.h) header if you are building on Windows
1. Make sure you have [cmake](https://cmake.org/download/) installed
1. Open a command prompt or terminal
1. Create a *build* directory `mkdir build` and enter it `cd build`
1. Run the command `cmake .. -D EXT_LIB_DIR="/path/to/libs" -D DIRENT_DIR="/path/to/dirent"`
1. The *build* directory will now contain a *makefile* or a *Visual Studio* solution

### Test project

> The *test* project currently only has UI components for Windows using [Win32 Controls](https://learn.microsoft.com/en-us/windows/win32/controls/individual-control-info).

### LVP_Initialize

> You must call *LVP_Initialize* with your callback handlers before using other *LVP_\** methods, see the *test* project for examples.

> To use hardware rendering set the *hardwareRenderer* parameter to an instance of *SDL_Renderer\**.
