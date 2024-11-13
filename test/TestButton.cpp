#include "TestButton.h"

TestButton::TestButton(SDL_Renderer* renderer, float dpiScale, const char* basePath, TestButtonId id, const std::string& label, bool enabled)
{
    this->basePath = basePath;
    this->dpiScale = dpiScale;
    this->enabled  = enabled;
    this->id       = id;
    this->label    = label;
    this->renderer = renderer;

    this->create();
}

TestButton::~TestButton()
{
    this->destroy();
}

void TestButton::create()
{
    //auto surface = this->getSurface();
	auto surface = SDL_CreateRGBSurfaceWithFormat(0, 64, 64, 32, SDL_PIXELFORMAT_RGBA32);

	printf("SDL_SURFACE_CREATE: %s\n", SDL_GetError());

    //this->size    = { surface->w, surface->h };
    //this->texture = SDL_CreateTextureFromSurface(this->renderer, surface);

	if (surface)
		SDL_FreeSurface(surface);

	printf("SDL_SURFACE_FREE: %s\n", SDL_GetError());
}

void TestButton::destroy()
{
    if (this->texture) {
        SDL_DestroyTexture(this->texture);
        this->texture = nullptr;
    }
}

void TestButton::enable(bool enabled)
{
    this->enabled = enabled;

    this->destroy();
    this->create();
}

SDL_Surface* TestButton::getSurface()
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

	const auto fontSize = (int)(14.0f * this->dpiScale);
	
	LibFT::FT_Set_Char_Size(font, 0, (fontSize << 6), 0, 0);

    const SDL_Color WHITE = { 0xFF, 0xFF, 0xFF, 0xFF };
    const SDL_Color GRAY  = { 0xAA, 0xAA, 0xAA, 0xFF };

	auto color      = (this->enabled ? WHITE : GRAY);
	auto colors     = 4;
	auto size       = this->getSurfaceSize(font);
	auto pitch      = (size.x * colors);
	auto pixelsSize = (size_t)(size.y * pitch);
	auto pixels     = (uint8_t*)std::calloc(pixelsSize, sizeof(uint8_t));
	auto position   = SDL_Point();

	if (font->glyph->bitmap.pixel_mode != 2)
		throw std::runtime_error(TextFormat("Font pixel mode: %d", (int)font->glyph->bitmap.pixel_mode));

	for (auto charCode : this->label)
	{
		LibFT::FT_Load_Char(font, charCode, FT_LOAD_RENDER);

		auto offsetY = (position.y + (size.y - font->glyph->bitmap_top));

		for (int y1 = 0, y2 = offsetY; (y1 < (int)font->glyph->bitmap.rows) && (y2 < size.y); y1++, y2++)
		{
			if (offsetY < 0)
				continue;

			auto offsetX = ((position.x + font->glyph->bitmap_left) * colors);
			auto rowSrc  = (y1 * font->glyph->bitmap.pitch);
			auto rowDest = (y2 * pitch);

			for (int x1 = 0, x2 = offsetX; (x1 < font->glyph->bitmap.pitch) && (x2 < pitch); x1++, x2 += colors)
			{
				if (offsetX < 0)
					continue;

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

	auto surface = SDL_CreateRGBSurfaceWithFormat(0, size.x, size.y, 32, SDL_PIXELFORMAT_RGBA32);

	if (pixels)
		std::memcpy(surface->pixels, pixels, pixelsSize);

	std::free(pixels);

	return surface;
}

SDL_Point TestButton::getSurfaceSize(LibFT::FT_Face font)
{
	SDL_Point size = {};

	for (auto charCode : this->label)
	{
		LibFT::FT_Load_Char(font, charCode, FT_LOAD_BITMAP_METRICS_ONLY);

		size.x += (font->glyph->advance.x >> 6);
	}

	size.y = (int)(font->glyph->metrics.vertAdvance >> 6);

	return size;
}

void TestButton::update(const std::string& label)
{
    this->label = label;

    this->destroy();
    this->create();
}
