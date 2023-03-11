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
[SDL2](https://www.libsdl.org/) | [2.26.2](https://www.libsdl.org/release/SDL2-2.26.2.tar.gz) | [zlib license](https://www.libsdl.org/license.php)
[SDL2_ttf](https://www.libsdl.org/projects/SDL_ttf/) | [2.20.1](https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.20.1.tar.gz) | [zlib license](https://www.libsdl.org/license.php)
[FFmpeg](https://ffmpeg.org/) | [5.1.2](https://ffmpeg.org/releases/ffmpeg-5.1.2.tar.bz2) | [LGPL v.2.1 (GNU Lesser General Public License)](https://ffmpeg.org/legal.html)
[libaom](https://aomedia.googlesource.com/aom/) | [3.6.0](https://storage.googleapis.com/aom-releases/libaom-3.6.0.tar.gz) | [Alliance for Open Media Patent License](https://aomedia.org/license/software-license/)
[zLib](http://www.zlib.net/) | [1.2.13](https://www.zlib.net/zlib-1.2.13.tar.gz) | [zlib license](http://www.zlib.net/zlib_license.html)

## How to build

1. Build the third-party libraries and place the them in a common directory
1. Open *CMakeLists.txt* in a text-editor
1. Update the *EXT_LIB_DIR* variable with the path to where you placed the the third-party libraries
1. You will also need the [dirent](https://github.com/tronkko/dirent/raw/master/include/dirent.h) header if you are building on Windows
    - Update the *DIRENT_DIR* variable with the path to where you placed *dirent.h*
1. Make sure you have [cmake](https://cmake.org/download/) installed
1. Open a command prompt or terminal
1. Create a *build* directory `mkdir build` and enter it `cd build`
1. Run the command `cmake ..`
1. The *build* directory will now contain a *makefile* or a *Visual Studio* solution

> The *test* project currently only has UI components for Windows using [Win32 Controls](https://learn.microsoft.com/en-us/windows/win32/controls/individual-control-info)
