#include "LVP_AudioSpecs.h"

MediaPlayer::LVP_AudioSpecs::LVP_AudioSpecs(AVFrame* frame, double playbackSpeed)
{
	this->playbackSpeed = playbackSpeed;
	this->channelLayout = frame->ch_layout;
	this->format        = frame->format;
	this->sampleRate    = frame->sample_rate;
}

MediaPlayer::LVP_AudioSpecs::LVP_AudioSpecs(AVFrame* frame)
{
	this->playbackSpeed = 1.0;
	this->channelLayout = frame->ch_layout;
	this->format        = frame->format;
	this->sampleRate    = frame->sample_rate;
}

MediaPlayer::LVP_AudioSpecs::LVP_AudioSpecs()
{
	this->playbackSpeed = 1.0;
	this->channelLayout = {};
	this->format        = 0;
	this->sampleRate    = 0;
}

bool MediaPlayer::LVP_AudioSpecs::equals(AVFrame* frame, double playbackSpeed) const
{
	if (!ARE_EQUAL_DOUBLES(playbackSpeed, this->playbackSpeed))
		return false;

	if (frame->format != this->format)
		return false;

	if (frame->sample_rate != this->sampleRate)
		return false;

	if (frame->ch_layout.nb_channels != this->channelLayout.nb_channels)
		return false;

	if (av_channel_layout_compare(&frame->ch_layout, &this->channelLayout) == 1)
		return false;

	return true;
}

AVChannelLayout MediaPlayer::LVP_AudioSpecs::getChannelLayout(int channels)
{
	AVChannelLayout layout;

	av_channel_layout_default(&layout, channels);

	return layout;
}

std::string MediaPlayer::LVP_AudioSpecs::getChannelLayoutName(const AVChannelLayout& layout)
{
	char buffer[DEFAULT_CHAR_BUFFER_SIZE];

	av_channel_layout_describe(&layout, buffer, DEFAULT_CHAR_BUFFER_SIZE);

	auto layoutName = std::string(buffer);

	if (layoutName.starts_with("2 ch"))
		return "stereo";
	else if (layoutName.starts_with("1 ch"))
		return "mono";

	return layoutName;
}

AVSampleFormat MediaPlayer::LVP_AudioSpecs::getSampleFormat(SDL_AudioFormat sdlFormat)
{
	switch (sdlFormat) {
		case SDL_AUDIO_U8:  return AV_SAMPLE_FMT_U8;
		case SDL_AUDIO_S16: return AV_SAMPLE_FMT_S16;
		case SDL_AUDIO_S32: return AV_SAMPLE_FMT_S32;
		case SDL_AUDIO_F32: return AV_SAMPLE_FMT_FLT;
		default: break;
	}

	return AV_SAMPLE_FMT_NONE;
}

SDL_AudioFormat MediaPlayer::LVP_AudioSpecs::getSampleFormat(AVSampleFormat avFormat)
{
	switch (avFormat) {
		case AV_SAMPLE_FMT_U8:
		case AV_SAMPLE_FMT_U8P:
			return SDL_AUDIO_U8;
		case AV_SAMPLE_FMT_S16:
		case AV_SAMPLE_FMT_S16P:
			return SDL_AUDIO_S16;
		case AV_SAMPLE_FMT_S32:
		case AV_SAMPLE_FMT_S32P:
			return SDL_AUDIO_S32;
		case AV_SAMPLE_FMT_FLT:
		case AV_SAMPLE_FMT_FLTP:
			return SDL_AUDIO_F32;
		default:
			break;
	}

	return SDL_AUDIO_UNKNOWN;
}

int MediaPlayer::LVP_AudioSpecs::getSampleRate(int sampleRate, double playbackSpeed)
{
	if (ARE_EQUAL_DOUBLES(playbackSpeed, 1.0))
		return sampleRate;

	return (int)std::ceil((double)sampleRate / playbackSpeed);
}
