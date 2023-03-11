#include "LVP_Graphics.h"

// https://boldaestheticcreative.com/2018/01/box-blur-with-sdl2/
void Graphics::LVP_Graphics::Blur(SDL_Surface* surface, int radius)
{
	auto block        = ((radius * 2) + 1);
	auto blockSquared = (block * block);
	auto pixels       = static_cast<uint32_t*>(surface->pixels);
	auto pixelsPerRow = (surface->pitch / 4);

	for (int y = 0; y < surface->h; y++)
    {
        for (int x = 0; x < pixelsPerRow; x++)
        {
			uint32_t r2 = 0, g2 = 0, b2 = 0, a2 = 0;

            for (int y2 = -radius; y2 <= radius; y2++)
            {
                for (int x2 = -radius; x2 <= radius; x2++)
                {
					auto y3 = (y + y2);
					auto x3 = (x + x2);

					if ((y3 < 0) || (x3 < 0) || (y3 >= surface->h) || (x3 >= pixelsPerRow))
						continue;

					auto pixel3 = ((y3 * pixelsPerRow) + x3);
					auto color3 = pixels[pixel3];

					uint8_t r3 = 0, g3 = 0, b3 = 0, a3 = 0;
                    SDL_GetRGBA(color3, surface->format, &r3, &g3, &b3, &a3);

                    r2 += r3;
                    g2 += g3;
                    b2 += b3;
                    a2 += a3;
                }
            }

            auto r = (uint8_t)(r2 / blockSquared);
			auto g = (uint8_t)(g2 / blockSquared);
			auto b = (uint8_t)(b2 / blockSquared);
			auto a = (uint8_t)(a2 / blockSquared);

			auto color = (r | (g << 8) | (b << 16) | (a << 24));
			auto pixel = ((y * pixelsPerRow) + x);

			pixels[pixel] = color;
        }
    }
}

void Graphics::LVP_Graphics::FillArea(const LVP_Color &color, const SDL_Rect &button, SDL_Renderer* renderer)
{
	SDL_SetRenderDrawBlendMode(renderer, (color.a < 0xFF ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE));
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	
	SDL_RenderFillRect(renderer, &button);
}

void Graphics::LVP_Graphics::FillBorder(const LVP_Color &color, const SDL_Rect &button, const LVP_Border &borderThickness, SDL_Renderer* renderer)
{
	SDL_Rect borderLine = SDL_Rect(button);

	// TOP
	borderLine.h = borderThickness.top;

	LVP_Graphics::FillArea(color, borderLine, renderer);

	// BOTTOM
	borderLine.y = (button.y + button.h - borderThickness.bottom);
	borderLine.h = borderThickness.bottom;

	LVP_Graphics::FillArea(color, borderLine, renderer);

	// LEFT
	borderLine.y = button.y;
	borderLine.w = borderThickness.left;
	borderLine.h = button.h;

	LVP_Graphics::FillArea(color, borderLine, renderer);

	// RIGHT
	borderLine.x = (button.x + button.w - borderThickness.right);
	borderLine.w = borderThickness.right;
	borderLine.h = button.h;

	LVP_Graphics::FillArea(color, borderLine, renderer);
}

/**
* Valid color string:
* "#00ff00ff" => 0 red, 255 green, 0 blue, 255 alpha
* "&HFF0000FF" (ABGR)
* "\c&H0000FF&" "\1c&H0000FF&" (BGR)
*/
Graphics::LVP_Color Graphics::LVP_Graphics::ToLVPColor(const std::string &colorHex)
{
	LVP_Color color   = LVP_COLOR_BLACK;
	bool      invertA = false;
	off_t     offsetR = -1;
	off_t     offsetG = -1;
	off_t     offsetB = -1;
	off_t     offsetA = -1;

	if (colorHex.size() < 2)
		return color;

	// RED (RGBA)
	if (colorHex[0] == '#')
	{
		// "#FF0000"
		if (colorHex.size() >= 7)
		{
			offsetR = 1;
			offsetG = 3;
			offsetB = 5;
		}

		// "#FF0000FF"
		if (colorHex.size() >= 9)
			offsetA = 7;
	}
	// RED (ABGR)
	else if (colorHex.substr(0, 2) == "&H")
	{
		// "&HFF0000FF"
		if (colorHex.size() >= 10)
		{
			offsetR = 8;
			offsetG = 6;
			offsetB = 4;
			offsetA = 2;
			invertA = true;
		}
		// "&H0000FF"
		else if (colorHex.size() >= 8)
		{
			offsetR = 6;
			offsetG = 4;
			offsetB = 2;
		}
		// "&HFF00" 
		else if (colorHex.size() >= 6)
		{
			offsetR = 4;
			offsetG = 2;
		}
		// "&HFF" 
		else if (colorHex.size() >= 4)
		{
			offsetR = 2;
		}
		// "&H0" 
	}

	if (offsetR != -1)
		color.r = (uint8_t)std::strtoul(std::string("0x" + colorHex.substr(offsetR, 2)).c_str(), NULL, 16);

	if (offsetG != -1)
		color.g = (uint8_t)std::strtoul(std::string("0x" + colorHex.substr(offsetG, 2)).c_str(), NULL, 16);

	if (offsetB != -1)
		color.b = (uint8_t)std::strtoul(std::string("0x" + colorHex.substr(offsetB, 2)).c_str(), NULL, 16);

	if (offsetA != -1)
	{
		color.a = (uint8_t)std::strtoul(std::string("0x" + colorHex.substr(offsetA, 2)).c_str(), NULL, 16);

		if (invertA)
			color.a = (0xFF - color.a);
	}

	return color;
}
