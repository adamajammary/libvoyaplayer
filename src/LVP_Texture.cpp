#include "LVP_Texture.h"

Graphics::LVP_Texture::LVP_Texture(SDL_Surface* surface, SDL_Renderer* renderer)
{
	this->init();

	if (surface != NULL)
	{
		if (renderer != NULL)
			this->data = SDL_CreateTextureFromSurface(renderer, surface);

		this->setProperties();
	}
}

Graphics::LVP_Texture::LVP_Texture(uint32_t format, int access, int width, int height, SDL_Renderer* renderer)
{
	this->init();

	if (renderer != NULL)
		this->data = SDL_CreateTexture(renderer, format, access, width, height);

	this->setProperties();
}

Graphics::LVP_Texture::~LVP_Texture()
{
	FREE_TEXTURE(this->data);
}

void Graphics::LVP_Texture::init()
{
	this->access = 0;
	this->width  = 0;
	this->height = 0;
	this->format = 0;
	this->data   = NULL;
	this->scroll = {};
}

void Graphics::LVP_Texture::setProperties()
{
	if (this->data != NULL)
		SDL_QueryTexture(this->data, &this->format, &this->access, &this->width, &this->height);
}
