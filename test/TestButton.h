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

struct TestButton
{
    SDL_Rect      background = {};
    const char*   basePath   = nullptr;
    float         dpiScale   = 1.0f;
    bool          enabled    = true;
    TestButtonId  id         = TEST_BUTTON_ID_UNKNOWN;
    std::string   label      = "";
    SDL_Renderer* renderer   = nullptr;
    SDL_Point     size       = {};
    SDL_Texture*  texture    = nullptr;

    TestButton(SDL_Renderer* renderer, float dpiScale, const char* basePath, TestButtonId id, const std::string& label, bool enabled = true);
    ~TestButton();

    void create();
    void destroy();
    void enable(bool enabled = true);
    void update(const std::string& label);
};

using TestButtonIds = std::unordered_map<TestButtonId, TestButton*>;
using TestButtons   = std::vector<TestButton*>;

#endif
