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
    static LibFT::FT_Library ftLibrary;

public:
    static SDL_Surface*    GetSurface(TestButton* button);
    static SDL_Point       GetSurfaceSize(TestButton* button, LibFT::FT_Face font);
    static LibFT::FT_Error Init();
    static void            Quit();
};

#endif
