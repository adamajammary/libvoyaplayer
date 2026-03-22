#ifndef TEST_MAIN_H
#define TEST_MAIN_H

#include <format>

#if defined _android
    #include <android/asset_manager_jni.h> // AAsset*, JNI*, j*
#elif defined _ios
    #include <UIKit/UIKit.h> // UIApplication, UIWindow*
#elif defined _windows
    #include <windows.h> // WinMain(x)
#endif

#include <libvoyaplayer.h>

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

static std::string TimeFormat(int64_t milliSeconds)
{
    auto totSecs = (milliSeconds / 1000);

    auto remSecs = (totSecs % 3600);
    auto minutes = (remSecs / 60);
    auto seconds = (remSecs % 60);

    auto time = std::format("{}:{:02}", minutes, seconds);

    return time;
}

#include "TestButton.h"
#include "TestPlayer.h"
#include "TestText.h"
#include "TestWindow.h"

#endif
