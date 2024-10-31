#ifndef TEST_MAIN_H
#define TEST_MAIN_H

#if defined _android
    #include <android/asset_manager_jni.h> // AAsset*, JNI*, j*
    #include <sys/stat.h>                  // mkdir(x)
#elif defined _windows
    #include <windows.h> // WinMain(x)
#endif

#include <libvoyaplayer.h>

#include <unordered_map>

namespace LibFT
{
	extern "C"
	{
		#include <ft2build.h>
		#include FT_FREETYPE_H
	}
}

struct TestAppIcon {
    static const int      Depth  = 24;
    static const uint32_t Format = SDL_PIXELFORMAT_RGB24;
    static const int      Height = 64;
    static const int      Pitch  = 192;
    static const size_t   Size   = 12288;
    static const int      Width  = 64;
};

template<typename... Args>
static std::string TextFormat(const char* formatString, const Args&... args)
{
    if (!formatString)
        return "";

    char buffer[1024] = {};
    std::snprintf(buffer, 1024, formatString, args...);

    return std::string(buffer);
}

static std::string TimeFormat(int64_t milliSeconds)
{
    auto totSecs = (milliSeconds / 1000);
    auto hours   = (totSecs / 3600);
    auto remSecs = (totSecs % 3600);
    auto minutes = (remSecs / 60);
    auto seconds = (remSecs % 60);

    return TextFormat("%02d:%02d:%02d", hours, minutes, seconds);
}

#include "TestButton.h"
#include "TestPlayer.h"
#include "TestText.h"
#include "TestWindow.h"

#endif
