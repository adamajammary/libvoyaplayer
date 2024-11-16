#include "LVP_AudioSpecs.h"

MediaPlayer::LVP_AudioSpecs::LVP_AudioSpecs()
{
	this->channelLayout = {};
	this->format        = 0;
	this->initContext   = true;
	this->playbackSpeed = 1.0;
	this->sampleRate    = 0;
	this->swrContext    = NULL;
}

bool MediaPlayer::LVP_AudioSpecs::hasChanged(LibFFmpeg::AVFrame* frame, double playbackSpeed) const
{
	if (playbackSpeed != this->playbackSpeed)
		return true;

	if (frame->ch_layout.nb_channels != this->channelLayout.nb_channels)
		return true;

	if (frame->format != this->format)
		return true;

	if (frame->sample_rate != this->sampleRate)
		return true;

	return false;
}

void MediaPlayer::LVP_AudioSpecs::init(LibFFmpeg::AVFrame* frame, double playbackSpeed)
{
	this->initContext   = true;
	this->playbackSpeed = playbackSpeed;
	this->channelLayout = frame->ch_layout;
	this->format        = frame->format;
	this->sampleRate    = frame->sample_rate;
}
