#include "LVP_VideoContext.h"

MediaPlayer::LVP_VideoContext::LVP_VideoContext()
{
	this->frame              = NULL;
	this->frameEncoded       = NULL;
	this->frameHardware      = NULL;
	this->frameSoftware      = NULL;
	this->hardwareFormat     = LibFFmpeg::AV_PIX_FMT_NONE;
	this->isReadyForRender   = false;
	this->isReadyForPresent  = false;
	this->isSoftwareRenderer = false;
	this->pts                = 0.0;
	this->renderer           = NULL;
	this->scaleContext       = NULL;
	this->surface            = NULL;
	this->texture            = NULL;
}

MediaPlayer::LVP_VideoContext::~LVP_VideoContext()
{
	if (this->isSoftwareRenderer)
		FREE_RENDERER(this->renderer);

	FREE_AVFRAME(this->frameEncoded);
	FREE_AVFRAME(this->frameHardware);
	FREE_AVFRAME(this->frameSoftware);
	FREE_SURFACE(this->surface);
	FREE_SWS(this->scaleContext);
	FREE_TEXTURE(this->texture);
}

int MediaPlayer::LVP_VideoContext::getTimeUntilPTS(double progress) const
{
	return (int)((this->pts - progress) * ONE_SECOND_MS_D);
}
