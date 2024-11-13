#ifndef TEST_MAIN_H
    #include "main.h"
#endif

#ifndef TEST_BUTTON_H
#define TEST_BUTTON_H

enum TestButtonId
{
    TEST_BUTTON_ID_UNKNOWN = -1,
    TEST_BUTTON_ID_PLAY_PAUSE,
    TEST_BUTTON_ID_PROGRESS,
    TEST_BUTTON_ID_SEEK_BACK,
    TEST_BUTTON_ID_SEEK_FORWARD,
    TEST_BUTTON_ID_STOP
};

class TestButton
{
public:
    TestButton(SDL_Renderer* renderer, float dpiScale, const char* basePath, TestButtonId id, const std::string& label, bool enabled = true);
    ~TestButton();

public:
    SDL_Rect     background = {};
    bool         enabled    = true;
    TestButtonId id         = TEST_BUTTON_ID_UNKNOWN;
    SDL_Point    size       = {};
    SDL_Texture* texture    = nullptr;

private:
    const char*   basePath = nullptr;
    float         dpiScale = 1.0f;
    std::string   label    = "";
    SDL_Renderer* renderer = nullptr;

public:
    void create();
    void destroy();
    void enable(bool enabled = true);
    void update(const std::string& label);

private:
    SDL_Surface* getSurface();
    SDL_Point    getSurfaceSize(LibFT::FT_Face font);
};

using TestButtonIds = std::unordered_map<TestButtonId, TestButton*>;
using TestButtons   = std::vector<TestButton*>;

#endif
