#include "LVP_Color.h"

Graphics::LVP_Color::LVP_Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

Graphics::LVP_Color::LVP_Color(const LVP_Color &color)
{
	this->r = color.r;
	this->g = color.g;
	this->b = color.b;
	this->a = color.a;
}

Graphics::LVP_Color::LVP_Color(const SDL_Color &color)
{
	this->r = color.r;
	this->g = color.g;
	this->b = color.b;
	this->a = color.a;
}

Graphics::LVP_Color::LVP_Color()
{
	this->r = this->g = this->b = this->a = 0;
}

bool Graphics::LVP_Color::operator ==(const LVP_Color &color)
{
	return ((color.r == this->r) && (color.g == this->g) && (color.b == this->b) && (color.a == this->a));
}

bool Graphics::LVP_Color::operator !=(const LVP_Color &color)
{
	return !((LVP_Color)color == *this);
}

Graphics::LVP_Color::operator SDL_Color()
{
	return { this->r, this->g, this->b, this->a };
}

bool Graphics::LVP_Color::IsEmpty()
{
	return ((this->r == 0) && (this->g == 0) && (this->b == 0) && (this->a == 0));
}
