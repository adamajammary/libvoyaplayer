#ifndef TEST_PLAYER_H
#define TEST_PLAYER_H

#include <libvoyaplayer.h>

const int64_t SEEK_DIFF = 20000;

class TestPlayer
{
private:
    TestPlayer()  {}
    ~TestPlayer() {}

private:
    static SDL_Texture* texture;
    static SDL_Surface* videoFrame;
    static bool         videoIsAvailable;
    static std::mutex   videoLock;

public:
    static void Init(SDL_Renderer* renderer = nullptr, const void* data = nullptr);
    static void Quit();
    static void Render(SDL_Renderer* renderer, const SDL_Rect &destination);
    static void SeekBack();
    static void SeekForward();

private:
    static void freeResources();
    static void handleError(const std::string &errorMessage, const void* data);
    static void handleEvent(LVP_EventType type, const void* data);
    static void handleVideoIsAvailable(SDL_Surface* videoFrame, const void* data);

};

#endif
