#ifndef TEST_MAIN_H
    #include "main.h"
#endif

#ifndef TEST_TEXT_H
#define TEST_TEXT_H

class TestText
{
private:
    TestText()  {}
    ~TestText() {}

private:
    static LibFT::FT_Face    font;
    static LibFT::FT_Library library;

public:
    static SDL_Surface* GetSurface(TestButton* button);
    static void         Init(const char* basePath);
    static void         Quit();

private:
    static SDL_Point getSurfaceSize(TestButton* button, LibFT::FT_Face font);
};

#endif
