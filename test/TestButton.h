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
    SDL_Rect     background;
    float        dpiScale;
    bool         enabled;
    float        fontSize;
    TestButtonId id;
    std::string  label;
    SDL_Point    size;
    SDL_Texture* texture;

private:
    const char*   basePath;
    SDL_Renderer* renderer;

public:
    void create();
    void destroy();
    void enable(bool enabled = true);
    void update(const std::string& label);
};

using TestButtonIds = std::unordered_map<TestButtonId, TestButton*>;
using TestButtons   = std::vector<TestButton*>;

#endif
