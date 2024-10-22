#ifndef TEST_MAIN_H
    #include "main.h"
#endif

#ifndef TEST_WINDOW_H
#define TEST_WINDOW_H

class TestWindow
{
private:
    TestWindow()  {}
    ~TestWindow() {}

private:
    static TestButtonIds buttonIds;
    static TestButtons   buttons;
    static SDL_Renderer* renderer;
    static std::string   title;
    static SDL_Window*   window;

public:
    static void          EnableButton(TestButtonId id, bool enabled = true);
    static TestButton*   GetClickedButton(const SDL_MouseButtonEvent& event);
    static SDL_Rect      GetDimensions();
    static float         GetDPIScale();
    static SDL_Renderer* GetRenderer();
    static void          Init(int width, int height, const char* basePath);
    static void          Quit();
    static void          RenderControls(const SDL_Rect& destination, float dpiScale);
    static void          UpdateButton(TestButtonId id, const std::string& label);
    static void          UpdateProgress();
    static void          UpdateTitle(const std::string& title = "");

private:
    static void initButtons(const char* basePath);

    #if defined _linux || defined _macosx || defined _windows
    static void initIcon(const char* basePath);
    #endif

};

#endif
