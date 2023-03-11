#include "LVP_SubTexture.h"

Graphics::LVP_SubTexture::LVP_SubTexture()
{
	this->locationRender = {};
	this->offsetY        = false;
	this->outline        = NULL;
	this->shadow         = NULL;
	this->subtitle       = NULL;
	this->textureData    = NULL;
	this->total          = {};
}

Graphics::LVP_SubTexture::LVP_SubTexture(const LVP_SubTexture &textureR)
{
	this->locationRender = textureR.locationRender;
	this->offsetY        = textureR.offsetY;
	this->outline        = NULL;
	this->shadow         = NULL;
	this->subtitle       = textureR.subtitle;
	this->textureData    = NULL;
	this->textUTF16      = NULL;
	this->total          = textureR.total;
}

Graphics::LVP_SubTexture::LVP_SubTexture(MediaPlayer::LVP_Subtitle* subtitle)
{
	this->locationRender = {};
	this->offsetY        = false;
	this->outline        = NULL;
	this->shadow         = NULL;
	this->subtitle       = subtitle;
	this->textureData    = NULL;
	this->textUTF16      = NULL;
	this->total          = {};
}

Graphics::LVP_SubTexture::~LVP_SubTexture()
{
	DELETE_POINTER(this->outline);
	DELETE_POINTER(this->shadow);
	DELETE_POINTER(this->textureData);
}
