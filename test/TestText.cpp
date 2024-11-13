#include "TestText.h"

SDL_Surface* TestText::GetSurface(TestButton* button)
{
    #if defined _android
        const auto fontPath = "/system/fonts/Roboto-Regular.ttf";
    #elif defined _ios
		auto arial    = TextFormat("%s%s", button->basePath, "Arial.ttf");
		auto fontPath = arial.c_str();
    #elif defined _linux
    	auto fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    #elif defined  _macosx
	    auto fontPath = "/System/Library/Fonts/Supplemental/Arial.ttf";
    #elif defined _windows
	    auto fontPath = "C:\\Windows\\Fonts\\arial.ttf";
    #endif

	LibFT::FT_Library library = nullptr;

	auto ftError = LibFT::FT_Init_FreeType(&library);

	if (!library || (ftError != LibFT::FT_Err_Ok))
		throw std::runtime_error(TextFormat("Failed to initialize FreeType2: %d", ftError));

	LibFT::FT_Face font = nullptr;
	
	ftError = LibFT::FT_New_Face(library, fontPath, 0, &font);

    if (!font || (ftError != LibFT::FT_Err_Ok))
        throw std::runtime_error(TextFormat("Failed to open font '%s': %d", fontPath, ftError));

	const auto fontSize = (int)(14.0f * button->dpiScale);
	
	LibFT::FT_Set_Char_Size(font, 0, (fontSize << 6), 0, 0);

    const SDL_Color WHITE = { 0xFF, 0xFF, 0xFF, 0xFF };
    const SDL_Color GRAY  = { 0xAA, 0xAA, 0xAA, 0xFF };

	auto color    = (button->enabled ? WHITE : GRAY);
	auto size     = TestText::GetSurfaceSize(button, font);
	auto surface  = SDL_CreateRGBSurfaceWithFormat(0, size.x, size.y, 32, SDL_PIXELFORMAT_RGBA32);
	auto colors   = surface->format->BytesPerPixel;
	auto pixels   = (uint8_t*)surface->pixels;
	auto position = SDL_Point();

	for (auto charCode : button->label)
	{
		LibFT::FT_Load_Char(font, charCode, FT_LOAD_RENDER);

		auto offsetY = (position.y + (surface->h - font->glyph->bitmap_top));

		for (int y1 = 0, y2 = offsetY; (y1 < (int)font->glyph->bitmap.rows) && (y2 < surface->h); y1++, y2++)
		{
			auto offsetX = ((position.x + font->glyph->bitmap_left) * colors);
			auto rowSrc  = (y1 * (int)font->glyph->bitmap.width);
			auto rowDest = (y2 * surface->pitch);

			for (int x1 = 0, x2 = offsetX; (x1 < (int)font->glyph->bitmap.width) && (x2 < surface->pitch); x1++, x2 += colors)
			{
				pixels[rowDest + x2]     = color.r;
				pixels[rowDest + x2 + 1] = color.g;
				pixels[rowDest + x2 + 2] = color.b;
				pixels[rowDest + x2 + 3] = font->glyph->bitmap.buffer[rowSrc + x1];
			}
		}

		position.x += (font->glyph->advance.x >> 6);
	}

	LibFT::FT_Done_Face(font);
	LibFT::FT_Done_FreeType(library);

	return surface;
}

SDL_Point TestText::GetSurfaceSize(TestButton* button, LibFT::FT_Face font)
{
	SDL_Point size = {};

	for (auto charCode : button->label)
	{
		LibFT::FT_Load_Char(font, charCode, FT_LOAD_BITMAP_METRICS_ONLY);

		size.x += (font->glyph->advance.x >> 6);
	}

	size.y = (int)(font->glyph->metrics.vertAdvance >> 6);

	return size;
}
