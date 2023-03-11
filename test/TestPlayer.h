#ifndef TEST_PLAYER_H
#define TEST_PLAYER_H

#include <libvoyaplayer.h>

const int64_t SEEK_DIFF   = 20000;
const double  SPEED_DIFF  = 0.25;
const double  VOLUME_DIFF = 0.05;

class TestPlayer
{
private:
    TestPlayer()  {}
    ~TestPlayer() {}

private:
    static SDL_Texture* texture;
    static SDL_Surface* videoFrame;
    static std::mutex   videoLock;

public:
    static void Init();
    static bool IsActive();
    static void Quit();
    static void Render(SDL_Renderer* renderer, const SDL_Rect &destination);
    static void SeekBack();
    static void SeekForward();
    static void SeekToChapter(const LVP_MediaChapter &chapter);
    static void SetPlaybackSpeedDown();
    static void SetPlaybackSpeedUp();
    static void SetVolumeDown();
    static void SetVolumeUp();

private:
    static void freeResources();
    static void handleError(const std::string &errorMessage, const void* data);
    static void handleEvent(LVP_EventType eventType, const void* data);
    static void handleVideoIsAvailable(SDL_Surface* videoFrame, const void* data);

};

#endif
