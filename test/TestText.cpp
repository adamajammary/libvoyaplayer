#include "TestText.h"

FT_Face    TestText::font    = nullptr;
FT_Library TestText::library = nullptr;

SDL_Surface* TestText::GetSurface(TestButton* button)
{
   
    FT_Set_Char_Size(TestText::font, 0, (button->fontSize << 6), 0, 0);

    const SDL_Color WHITE = { 0xFF, 0xFF, 0xFF, 0xFF };
    const SDL_Color GRAY  = { 0xAA, 0xAA, 0xAA, 0xFF };

    auto color    = (button->enabled ? WHITE : GRAY);
    auto size     = TestText::getSurfaceSize(button, TestText::font);
    auto surface  = SDL_CreateSurface(size.x, size.y, SDL_PIXELFORMAT_RGBA32);
    auto colors   = SDL_BYTESPERPIXEL(surface->format);
    auto pixels   = (uint8_t*)surface->pixels;
    auto position = SDL_Point();

    for (auto charCode : button->label)
    {
        FT_Load_Char(TestText::font, charCode, FT_LOAD_RENDER);

        auto offsetY = (position.y + (surface->h - TestText::font->glyph->bitmap_top));

        for (int y1 = 0, y2 = offsetY; (y1 < (int)TestText::font->glyph->bitmap.rows) && (y2 < surface->h); y1++, y2++)
        {
            if (offsetY < 0)
                continue;

            auto offsetX = ((position.x + TestText::font->glyph->bitmap_left) * colors);
            auto rowSrc  = (y1 * (int)TestText::font->glyph->bitmap.pitch);
            auto rowDest = (y2 * surface->pitch);

            for (int x1 = 0, x2 = offsetX; (x1 < (int)TestText::font->glyph->bitmap.pitch) && (x2 < surface->pitch); x1++, x2 += colors)
            {
                if (offsetX < 0)
                    continue;

                pixels[rowDest + x2]     = color.r;
                pixels[rowDest + x2 + 1] = color.g;
                pixels[rowDest + x2 + 2] = color.b;
                pixels[rowDest + x2 + 3] = TestText::font->glyph->bitmap.buffer[rowSrc + x1];
            }
        }

        position.x += (TestText::font->glyph->advance.x >> 6);
    }

    return surface;
}

SDL_Point TestText::getSurfaceSize(TestButton* button, FT_Face font)
{
    SDL_Point size = {};

    for (auto charCode : button->label)
    {
        FT_Load_Char(font, charCode, FT_LOAD_BITMAP_METRICS_ONLY);

        size.x += (font->glyph->advance.x >> 6);
    }

    size.y = (int)(font->glyph->metrics.vertAdvance >> 6);

    return size;
}

void TestText::Init(const std::string& basePath)
{
    auto ftError = FT_Init_FreeType(&TestText::library);

    if (!TestText::library || (ftError != FT_Err_Ok))
        throw std::runtime_error(std::format("Failed to initialize FreeType2: {}", ftError));

    #if defined _android
        const auto fontPath = "/system/fonts/Roboto-Regular.ttf";
    #elif defined _ios
        auto arial    = std::format("{}{}", basePath, "Arial.ttf");
        auto fontPath = arial.c_str();
    #elif defined _linux
        auto fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    #elif defined  _macosx
        auto fontPath = "/System/Library/Fonts/Supplemental/Arial.ttf";
    #elif defined _windows
        auto fontPath = "C:\\Windows\\Fonts\\arial.ttf";
    #endif

    ftError = FT_New_Face(TestText::library, fontPath, 0, &TestText::font);

    if (!TestText::font || (ftError != FT_Err_Ok))
        throw std::runtime_error(std::format("Failed to open font '{}': {}", fontPath, ftError));

}

void TestText::Quit()
{
    FT_Done_Face(TestText::font);
    FT_Done_FreeType(TestText::library);
}
