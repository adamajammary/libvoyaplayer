#include "LVP_Graphics.h"

void Graphics::LVP_Graphics::FillArea(const LVP_Color &color, const SDL_Rect &area, SDL_Renderer* renderer)
{
	SDL_SetRenderDrawBlendMode(renderer, (color.a < 0xFF ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE));
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	
	SDL_RenderFillRect(renderer, &area);
}

void Graphics::LVP_Graphics::FillBorder(const LVP_Color &color, const SDL_Rect &area, const LVP_Border &borderThickness, SDL_Renderer* renderer)
{
	SDL_Rect borderLine = SDL_Rect(area);

	// TOP
	borderLine.h = borderThickness.top;

	LVP_Graphics::FillArea(color, borderLine, renderer);

	// BOTTOM
	borderLine.y = (area.y + area.h - borderThickness.bottom);
	borderLine.h = borderThickness.bottom;

	LVP_Graphics::FillArea(color, borderLine, renderer);

	// LEFT
	borderLine.y = area.y;
	borderLine.w = borderThickness.left;
	borderLine.h = area.h;

	LVP_Graphics::FillArea(color, borderLine, renderer);

	// RIGHT
	borderLine.x = (area.x + area.w - borderThickness.right);
	borderLine.w = borderThickness.right;
	borderLine.h = area.h;

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
