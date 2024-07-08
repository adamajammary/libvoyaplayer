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
[SDL2](https://www.libsdl.org/) | [2.30.1](https://www.libsdl.org/release/SDL2-2.30.1.tar.gz) | [zlib license](https://www.libsdl.org/license.php)
[SDL2_ttf](https://www.libsdl.org/projects/SDL_ttf/) | [2.22.0](https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.22.0.tar.gz) | [zlib license](https://www.libsdl.org/license.php)
[FFmpeg](https://ffmpeg.org/) | [7.0.1](https://ffmpeg.org/releases/ffmpeg-7.0.1.tar.bz2) | [LGPL v.2.1 (GNU Lesser General Public License)](https://ffmpeg.org/legal.html)
[dav1d](https://www.videolan.org/projects/dav1d.html) | [1.4.3](https://code.videolan.org/videolan/dav1d/-/archive/1.4.3/dav1d-1.4.3.tar.gz) | [BSD 2-Clause "Simplified" License](https://code.videolan.org/videolan/dav1d/-/blob/master/COPYING)
[zLib](http://www.zlib.net/) | [1.3.1](https://www.zlib.net/zlib-1.3.1.tar.gz) | [zlib license](http://www.zlib.net/zlib_license.html)

## Platform-dependent Include Headers

Platform | Header | Package
-------- | ------ | -------
Android | android/asset_manager_jni.h | Android NDK
Android | sys/stat.h | Android NDK
Windows | dirent.h | [dirent.h](https://github.com/tronkko/dirent/raw/master/include/dirent.h)
Windows | windows.h | WinMain

## Compilers and C++20

libvoyaplayer uses modern [C++20](https://en.cppreference.com/w/cpp/compiler_support#cpp20) features and requires the following minimum compiler versions.

Compiler | Version
-------- | -------
CLANG | 14
GCC | 13
MSVC | 2019

## How to build

1. Build the [third-party libraries](#3rd-party-libraries) and place the them in a common directory.
   - You will also need the [dirent.h](https://github.com/tronkko/dirent/raw/master/include/dirent.h) header if you are building on **Windows**.
1. Make sure you have [cmake](https://cmake.org/download/) installed.
1. Open a command prompt or terminal.
1. Create a **build** directory and enter it.
1. Run `cmake` to create a **Makefile**, **Xcode** project or **Visual Studio** solution based on your target platform.
1. After building, the **dist** directory will contain all the output resources in the **include**, **lib** and **bin** directories.

```bash
mkdir build
cd build
```

### Android

Make sure you have [Android SDK](https://developer.android.com/studio) and [Android NDK](https://developer.android.com/ndk/downloads) installed.

Make sure the correct Android SDK path is set as either

- an environment variable `ANDROID_HOME=/path/to/ANDROID_SDK` or
- a local property `sdk.dir=/path/to/ANDROID_SDK` in the **android/local.properties** file

> See [Android SDK Command-Line Tools](https://developer.android.com/tools) and [SDL2 Android README](https://wiki.libsdl.org/SDL2/README/android) for more details.

```bash
cmake .. -G "Unix Makefiles" \
-D CMAKE_SYSTEM_NAME="Android" \
-D CMAKE_TOOLCHAIN_FILE="/path/to/ANDROID_NDK/build/cmake/android.toolchain.cmake" \
-D ANDROID_NDK="/path/to/ANDROID_NDK" \
-D ANDROID_ABI="arm64-v8a" \
-D ANDROID_PLATFORM="android-29" \
-D EXT_LIB_DIR="/path/to/libs"

make
```

#### ADB (Android Debug Bridge)

> See [ADB (Android Debug Bridge)](https://developer.android.com/tools/adb) for more details.

##### Install APK to device

```bash
/path/to/ANDROID_SDK/platform-tools/adb install dist/bin/testvoyaplayer-arm64-v8a-debug.apk
```

##### Re-install (update) APK to device

```bash
/path/to/ANDROID_SDK/platform-tools/adb install -r dist/bin/testvoyaplayer-arm64-v8a-debug.apk
```

##### Uninstall (remove) APK from device

```bash
/path/to/ANDROID_SDK/platform-tools/adb uninstall com.libvoyaplayer.test
```

### iOS

You can get the iOS SDK path with the following command: `xcrun --sdk iphoneos --show-sdk-path`

> See [SDL2 iOS README](https://wiki.libsdl.org/SDL2/README/ios) for more details.

```bash
/Applications/CMake.app/Contents/bin/cmake .. -G "Xcode" \
-D CMAKE_SYSTEM_NAME="iOS" \
-D CMAKE_OSX_ARCHITECTURES="arm64" \
-D CMAKE_OSX_DEPLOYMENT_TARGET="12.5" \
-D CMAKE_OSX_SYSROOT="/path/to/IOS_SDK" \
-D CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="YOUR_DEVELOPMENT_TEAM_ID" \
-D EXT_LIB_DIR="/path/to/libs" \
-D IOS_SDK="iphoneos"

xcodebuild IPHONEOS_DEPLOYMENT_TARGET="12.5" -project voyaplayer.xcodeproj -configuration Release -destination "generic/platform=iOS" -allowProvisioningUpdates
```

#### Xcode - Devices

> See [Xcode - Running your app on a device](https://developer.apple.com/documentation/xcode/running-your-app-in-simulator-or-on-a-device) for more details.

#### Install APP on a device

1. Connect the device to your Mac.
1. Open **Xcode**.
1. Select `Window > Devices and Simulators` from the main menu.
1. Select the device from the list on the left.
1. Click the `+` icon under **Installed Apps**.
1. Locate and select `dist/bin/testvoyaplayer-arm64.app`.

The app should now be installed on the device with the name **testvoyaplayer**.

> If the installation fails, most likely it means the app package was not signed correctly. Try opening `voyaplayer.xcodeproj` in Xcode to make sure all signing options have been set correctly.

### macOS

You can get the macOS SDK path with the following command: `xcrun --sdk macosx --show-sdk-path`

```bash
/Applications/CMake.app/Contents/bin/cmake .. -G "Xcode" \
-D CMAKE_OSX_ARCHITECTURES="x86_64" \
-D CMAKE_OSX_DEPLOYMENT_TARGET="12.6" \
-D CMAKE_OSX_SYSROOT="/path/to/MACOSX_SDK" \
-D EXT_LIB_DIR="/path/to/libs"

xcodebuild MACOSX_DEPLOYMENT_TARGET="12.6" -project voyaplayer.xcodeproj -configuration Release
```

### Linux

```bash
cmake .. -G "Unix Makefiles" -D EXT_LIB_DIR="/path/to/libs"

make
```

### Windows

```bash
cmake .. -G "Visual Studio 17 2022" -D EXT_LIB_DIR="/path/to/libs" -D DIRENT_DIR="/path/to/dirent"

devenv.com voyaplayer.sln -build "Release|x64"
```

## Test project

You must call [LVP_Initialize](#lvp_initialize) with your callback handlers before using other *LVP_\** methods, see the *test* project for examples.

```cpp
bool QUIT = false;

try {
  init();

  while (!QUIT) {
    render();
    SDL_Delay(16);
  }

  quit();
} catch (const std::exception &e) {
  handleError(e.what());
  quit();
}
```

## Initialize

The first step is to call [LVP_Initialize](#lvp_initialize) with a [LVP_CallbackContext](#lvp_callbackcontext).

To use hardware rendering set the `.hardwareRenderer` parameter to an instance of an [SDL_Renderer](https://wiki.libsdl.org/SDL2/SDL_CreateRenderer).

Otherwise software rendering will be used (much slower).

```cpp
void init(SDL_Renderer* renderer, const void* data)
{
  LVP_CallbackContext callbackContext = {
    .errorCB  = handleError,
    .eventsCB = handleEvent,
    .videoCB  = handleVideoIsAvailable,
    .data     = data,
    .hardwareRenderer = renderer
  };

  LVP_Initialize(callbackContext);
}
```

## Handle Errors

The library will send error messages to your error handler callback, which must follow the function signature defined by [LVP_ErrorCallback](#lvp_errorcallback).

```cpp
void handleError(const std::string &errorMessage, const void* data)
{
  fprintf(stderr, "%s\n", errorMessage.c_str());
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "My Player", errorMessage.c_str(), nullptr);
}
```

## Handle Events

The library will send player/media events as [LVP_EventType](#lvp_eventtype) to your event handler callback, which must follow the function signature defined by [LVP_EventsCallback](#lvp_eventscallback).

```cpp
void handleEvent(LVP_EventType type, const void* data)
{
  switch (type) {
    case LVP_EVENT_AUDIO_MUTED:
      handleAudioMuted();
      break;
    case LVP_EVENT_MEDIA_PAUSED:
      handleMediaPaused();
      break;
    default:
      break;
  }
}
```

## Handle Video Frames - Hardware Rendering (GPU)

For optimal performance, you should render video frames using hardware rendering.

To use hardware rendering, just call [LVP_Render](#lvp_render) which will use the `.hardwareRenderer` you passed to [LVP_CallbackContext](#lvp_callbackcontext).

```cpp
void render(const SDL_Rect &destination)
{
  LVP_Render(&destination);
}
```

## Handle Video Frames - Software Rendering (CPU)

If you use software rendering, the library will send the available video frames as an [SDL_Surface](https://wiki.libsdl.org/SDL2/SDL_Surface) to your video handler callback, which must follow the function signature defined by [LVP_VideoCallback](#lvp_videocallback).

If you use your own custom [SDL_Renderer](https://wiki.libsdl.org/SDL2/SDL_CreateRenderer), you can copy the pixels from the [SDL_Surface](https://wiki.libsdl.org/SDL2/SDL_Surface) (CPU) to an [SDL_Texture](https://wiki.libsdl.org/SDL2/SDL_CreateTexture) (GPU) using [SDL_UpdateTexture](https://wiki.libsdl.org/SDL2/SDL_UpdateTexture) which you can render using your own [SDL_Renderer](https://wiki.libsdl.org/SDL2/SDL_CreateRenderer) with [SDL_RenderCopy](https://wiki.libsdl.org/SDL2/SDL_RenderCopy).

If you don't use SDL2 for rendering, you need to copy the pixels to a bitmap/image format used by your library/framework.

[SDL_Surface](https://wiki.libsdl.org/SDL2/SDL_Surface)

- The `.pixels` property will contain the pixel buffer as an RGB byte-array
- The `.w` and `.h` properties will contain the dimensions of the video frame
- The `.pitch` property will contain the byte size of a row of RGB pixels

```cpp
SDL_Texture* texture          = nullptr;
SDL_Surface* videoFrame       = nullptr;
bool         videoIsAvailable = false;
std::mutex   videoLock;

void handleVideoIsAvailable(SDL_Surface* videoSurface, const void* data)
{
  videoLock.lock();

  if (videoFrame)
    SDL_FreeSurface(videoFrame);

  videoFrame       = videoSurface;
  videoIsAvailable = true;

  videoLock.unlock();
}

void render(SDL_Renderer* renderer, const SDL_Rect &destination)
{
  videoLock.lock();

  if (!texture && videoFrame) {
    texture = SDL_CreateTexture(
      renderer,
      videoFrame->format->format,
      SDL_TEXTUREACCESS_STREAMING,
      videoFrame->w,
      videoFrame->h
    );
  }

  if (videoIsAvailable && texture && videoFrame) {
    videoIsAvailable = false;
    SDL_UpdateTexture(texture, nullptr, videoFrame->pixels, videoFrame->pitch);
  }

  if (texture)
    SDL_RenderCopy(renderer, texture, nullptr, &destination);

  videoLock.unlock();
}
```

## Quit

Make sure to call [LVP_Quit](#lvp_quit) to cleanup all resources and close the library.

```cpp
void quit() {
  LVP_Quit();

  videoLock.lock();

  SDL_DestroyTexture(texture);
  SDL_FreeSurface(videoFrame);

  videoLock.unlock();
}
```

## API

### LVP_EventType

```cpp
enum LVP_EventType {
  LVP_EVENT_AUDIO_MUTED,
  LVP_EVENT_AUDIO_UNMUTED,
  LVP_EVENT_AUDIO_VOLUME_CHANGED,
  LVP_EVENT_MEDIA_COMPLETED,
  LVP_EVENT_MEDIA_COMPLETED_WITH_ERRORS,
  LVP_EVENT_MEDIA_OPENED,
  LVP_EVENT_MEDIA_PAUSED,
  LVP_EVENT_MEDIA_PLAYING,
  LVP_EVENT_MEDIA_STOPPED,
  LVP_EVENT_MEDIA_STOPPING,
  LVP_EVENT_MEDIA_TRACKS_UPDATED,
  LVP_EVENT_METADATA_UPDATED,
  LVP_EVENT_PLAYBACK_SPEED_CHANGED
};
```

### LVP_MediaType

```cpp
enum LVP_MediaType {
  LVP_MEDIA_TYPE_UNKNOWN  = -1,
  LVP_MEDIA_TYPE_VIDEO    = 0,
  LVP_MEDIA_TYPE_AUDIO    = 1,
  LVP_MEDIA_TYPE_SUBTITLE = 3
};
```

### LVP_CallbackContext

```cpp
struct LVP_CallbackContext {
  LVP_ErrorCallback  errorCB  = nullptr; // Called every time an error occurs.
  LVP_EventsCallback eventsCB = nullptr; // Called every time an event of type LVP_EventType occurs.
  LVP_VideoCallback  videoCB  = nullptr; // Called every time a new video frame is available.

  const void* data = nullptr; // Custom data context, will be available in all callbacks.

  SDL_Renderer* hardwareRenderer = nullptr; // Use an existing SDL hardware renderer to process the video frames, otherwise software rendering will be used.
};
```

### LVP_MediaChapter

```cpp
struct LVP_MediaChapter {
  std::string title = "";

  int64_t startTime = 0; // Chapter start time in milliseconds (one thousandth of a second).
  int64_t endTime   = 0; // Chapter end time in milliseconds (one thousandth of a second).
};
```

### LVP_MediaTrack

```cpp
struct LVP_MediaTrack {
  LVP_MediaType mediaType = LVP_MEDIA_TYPE_UNKNOWN; // Media type of the track, like video (0), audio (1) or subtitle (3).

  int track = -1; // Track index number (position of the track in the media file).

  std::map<std::string, std::string> meta;  // Track metadata, like title, language etc.
  std::map<std::string, std::string> codec; // Codec specs, like codec_name, bit_rate etc.
};
```

### LVP_MediaDetails

```cpp
struct LVP_MediaDetails {
  int64_t duration  = 0; // Media duration in seconds.
  size_t  fileSize  = 0; // File size in bytes.

  LVP_MediaType mediaType = LVP_MEDIA_TYPE_UNKNOWN; // Media type, like video (0), audio (1) or subtitle (3).

  std::map<std::string, std::string> meta; // Media metadata like title, artist, album, genre etc.

  std::vector<LVP_MediaChapter> chapters;

  std::vector<LVP_MediaTrack> audioTracks;
  std::vector<LVP_MediaTrack> subtitleTracks;
  std::vector<LVP_MediaTrack> videoTracks;
};
```

### LVP_ErrorCallback

```cpp
typedef std::function<void(const std::string &errorMessage, const void* data)> LVP_ErrorCallback;
```

### LVP_EventsCallback

```cpp
typedef std::function<void(LVP_EventType type, const void* data)> LVP_EventsCallback;
```

### LVP_VideoCallback

```cpp
typedef std::function<void(SDL_Surface* videoFrame, const void* data)> LVP_VideoCallback;
```

### LVP_Initialize

Tries to initialize the library and other dependencies.

```cpp
void LVP_Initialize(const LVP_CallbackContext &callbackContext);
```

Exceptions

- runtime_error

### LVP_GetAudioDevices

Returns a list of available audio devices.

```cpp
std::vector<std::string> LVP_GetAudioDevices();
```

### LVP_GetChapters

Returns a list of chapters in the currently loaded media.

```cpp
std::vector<LVP_MediaChapter> LVP_GetChapters();
```

Exceptions

- runtime_error

### LVP_GetAudioTrack

Returns the current audio track index number.

```cpp
int LVP_GetAudioTrack();
```

Exceptions

- runtime_error

### LVP_GetAudioTracks

Returns a list of audio tracks in the currently loaded media.

```cpp
std::vector<LVP_MediaTrack> LVP_GetAudioTracks();
```

Exceptions

- runtime_error

### LVP_GetDuration

Returns the media duration in milliseconds (one thousandth of a second).

```cpp
int64_t LVP_GetDuration();
```

Exceptions

- runtime_error

### LVP_GetFilePath

Returns the current media file path.

```cpp
std::string LVP_GetFilePath();
```

Exceptions

- runtime_error

### LVP_GetMediaDetails

Returns media details of the currently loaded media.

```cpp
LVP_MediaDetails LVP_GetMediaDetails();
```

Exceptions

- runtime_error

### LVP_GetMediaDetails (string)

Returns media details of the the provided media file.

```cpp
LVP_MediaDetails LVP_GetMediaDetails(const std::string& filePath);
```

Parameters

- **filePath** Full path to the media file

Exceptions

- runtime_error

### LVP_GetMediaDetails (wstring)

Returns media details of the the provided media file.

```cpp
LVP_MediaDetails LVP_GetMediaDetails(const std::wstring& filePath);
```

Parameters

- **filePath** Full path to the media file

Exceptions

- runtime_error

### LVP_GetMediaType

Returns the media type of the currently loaded media.

```cpp
LVP_MediaType LVP_GetMediaType();
```

Exceptions

- runtime_error

### LVP_GetMediaType (string)

Returns the media type of the the provided media file.

```cpp
LVP_MediaType LVP_GetMediaType(const std::string& filePath);
```

Parameters

- **filePath** Full path to the media file

Exceptions

- runtime_error

### LVP_GetMediaType (wstring)

Returns the media type of the the provided media file.

```cpp
LVP_MediaType LVP_GetMediaType(const std::wstring& filePath);
```

Parameters

- **filePath** Full path to the media file

Exceptions

- runtime_error

### LVP_GetPlaybackSpeed

Returns the current playback speed as a percent between 0.5 and 2.0.

```cpp
double LVP_GetPlaybackSpeed();
```

Exceptions

- runtime_error

### LVP_GetProgress

Returns the media playback progress in milliseconds (one thousandth of a second).

```cpp
int64_t LVP_GetProgress();
```

Exceptions

- runtime_error

### LVP_GetSubtitleTrack

Returns the current subtitle track index number.

```cpp
int LVP_GetSubtitleTrack();
```

Exceptions

- runtime_error

### LVP_GetSubtitleTracks

Returns a list of subtitle tracks in the currently loaded media.

```cpp
std::vector<LVP_MediaTrack> LVP_GetSubtitleTracks();
```

Exceptions

- runtime_error

### LVP_GetVideoTracks

Returns a list of video tracks in the currently loaded media.

```cpp
std::vector<LVP_MediaTrack> LVP_GetVideoTracks();
```

Exceptions

- runtime_error

### LVP_GetVolume

Returns the current audio volume as a percent between 0 and 1.

```cpp
double LVP_GetVolume();
```

Exceptions

- runtime_error

### LVP_IsMuted

Returns true if audio volume is muted.

```cpp
bool LVP_IsMuted();
```

Exceptions

- runtime_error

### LVP_IsPaused

Returns true if playback is paused.

```cpp
bool LVP_IsPaused();
```

Exceptions

- runtime_error

### LVP_IsPlaying

Returns true if playback is playing (not paused and not stopped).

```cpp
bool LVP_IsPlaying();
```

Exceptions

- runtime_error

### LVP_IsStopped

Returns true if playback is stopped.

```cpp
bool LVP_IsStopped();
```

Exceptions

- runtime_error

### LVP_Open

Tries to open and play the given media file.

```cpp
void LVP_Open(const std::string &filePath);
```

Parameters

- **filePath** Full path to the media file

Exceptions

- runtime_error

### LVP_Open (wstring)

Tries to open and play the given media file.

```cpp
void LVP_Open(const std::wstring &filePath);
```

Parameters

- **filePath** Full path to the media file

Exceptions

- runtime_error

### LVP_Quit

Cleans up allocated resources.

```cpp
void LVP_Quit();
```

### LVP_Render

Generates and renders a video frame.

- If hardware rendering is used, it will copy the texture to the renderer.
- If software rendering is used, it will generate a [LVP_VideoCallback](#lvp_videocallback) with an [SDL_Surface](https://wiki.libsdl.org/SDL2/SDL_Surface).

```cpp
void LVP_Render(const SDL_Rect* destination = nullptr);
```

Parameters

- **destination** Optional clipping/scaling region used by the hardware renderer

### LVP_Resize

Should be called whenever the window resizes to tell the player to recreate the video frame context.

```cpp
void LVP_Resize();
```

### LVP_SeekTo

Seeks to the given position as a percent between 0 and 1.

```cpp
void LVP_SeekTo(double percent);
```

Parameters

- **percent** [0.0-1.0]

Exceptions

- runtime_error

### LVP_SetAudioDevice

Tries to set the given audio device as the current device if valid.

Returns true if the audio device is successfully set.

```cpp
bool LVP_SetAudioDevice(const std::string &device);
```

Parameters

- **device** Name of the audio device

### LVP_SetMuted

Mutes/unmutes the audio volume.

```cpp
void LVP_SetMuted(bool muted);
```

Parameters

- **muted** true to mute or false to unmute

Exceptions

- runtime_error

### LVP_SetPlaybackSpeed

Sets the given playback speed as a relative percent between 0.5 and 2.0, where 1.0 is normal/default.

```cpp
void LVP_SetPlaybackSpeed(double speed);
```

Parameters

- **speed** [0.5-2.0]

Exceptions

- runtime_error

### LVP_SetTrack

Tries to set the given stream as the current stream if valid.

```cpp
void LVP_SetTrack(const LVP_MediaTrack &track);
```

Parameters

- **track** -1 to disable subtitles or >= 0 for a valid media track

Exceptions

- runtime_error

### LVP_SetVolume

Sets the given audio volume as a percent between 0 and 1.

```cpp
void LVP_SetVolume(double percent);
```

Parameters

- **percent** [0.0-1.0]

Exceptions

- runtime_error

### LVP_Stop

Stops playback of the currently loaded media.

```cpp
void LVP_Stop();
```

Exceptions

- runtime_error

### LVP_ToggleMute

Toggles muting audio volume on/off.

```cpp
void LVP_ToggleMute();
```

Exceptions

- runtime_error

### LVP_TogglePause

Toggles between pausing and playing.

```cpp
void LVP_TogglePause();
```

Exceptions

- runtime_error
