#include "LVP_SubtitleContext.h"

MediaPlayer::LVP_SubtitleContext::LVP_SubtitleContext()
{
	this->external      = {};
	this->formatContext = NULL;
	this->frameEncoded  = NULL;
	this->scaleContext  = NULL;
	this->videoSize     = {};
}

MediaPlayer::LVP_SubtitleContext::~LVP_SubtitleContext()
{
	FREE_AVFRAME(this->frameEncoded);
	FREE_SWS(this->scaleContext);
}
