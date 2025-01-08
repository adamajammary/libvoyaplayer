#include "LVP_AudioSpecs.h"

MediaPlayer::LVP_AudioSpecs::LVP_AudioSpecs(LibFFmpeg::AVFrame* frame, double playbackSpeed)
{
	this->playbackSpeed = playbackSpeed;
	this->channelLayout = frame->ch_layout;
	this->format        = frame->format;
	this->sampleRate    = frame->sample_rate;
}

MediaPlayer::LVP_AudioSpecs::LVP_AudioSpecs(LibFFmpeg::AVFrame* frame)
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

bool MediaPlayer::LVP_AudioSpecs::equals(LibFFmpeg::AVFrame* frame, double playbackSpeed) const
{
	if (!ARE_EQUAL_DOUBLES(playbackSpeed, this->playbackSpeed))
		return false;

	if (frame->ch_layout.nb_channels != this->channelLayout.nb_channels)
		return false;

	if (frame->format != this->format)
		return false;

	if (frame->sample_rate != this->sampleRate)
		return false;

	return true;
}

LibFFmpeg::AVChannelLayout MediaPlayer::LVP_AudioSpecs::getChannelLayout(int channels)
{
	LibFFmpeg::AVChannelLayout layout;

	LibFFmpeg::av_channel_layout_default(&layout, channels);

	return layout;
}

std::string MediaPlayer::LVP_AudioSpecs::getChannelLayoutName(const LibFFmpeg::AVChannelLayout& layout)
{
	char buffer[DEFAULT_CHAR_BUFFER_SIZE];

	LibFFmpeg::av_channel_layout_describe(&layout, buffer, DEFAULT_CHAR_BUFFER_SIZE);

	auto layoutName = std::string(buffer);

	if (layoutName == "2 channels")
		return "stereo";
	else if (layoutName == "1 channel")
		return "mono";

	return layoutName;
}

LibFFmpeg::AVSampleFormat MediaPlayer::LVP_AudioSpecs::getSampleFormat(SDL_AudioFormat sdlFormat)
{
	switch (sdlFormat) {
		case AUDIO_U8:     return LibFFmpeg::AV_SAMPLE_FMT_U8;
		case AUDIO_S16SYS: return LibFFmpeg::AV_SAMPLE_FMT_S16;
		case AUDIO_S32SYS: return LibFFmpeg::AV_SAMPLE_FMT_S32;
		case AUDIO_F32SYS: return LibFFmpeg::AV_SAMPLE_FMT_FLT;
		default: break;
	}

	return LibFFmpeg::AV_SAMPLE_FMT_NONE;
}

SDL_AudioFormat MediaPlayer::LVP_AudioSpecs::getSampleFormat(LibFFmpeg::AVSampleFormat avFormat)
{
	switch (avFormat) {
		case LibFFmpeg::AV_SAMPLE_FMT_U8:
		case LibFFmpeg::AV_SAMPLE_FMT_U8P:
			return AUDIO_U8;
		case LibFFmpeg::AV_SAMPLE_FMT_S16:
		case LibFFmpeg::AV_SAMPLE_FMT_S16P:
			return AUDIO_S16SYS;
		case LibFFmpeg::AV_SAMPLE_FMT_S32:
		case LibFFmpeg::AV_SAMPLE_FMT_S32P:
			return AUDIO_S32SYS;
		case LibFFmpeg::AV_SAMPLE_FMT_FLT:
		case LibFFmpeg::AV_SAMPLE_FMT_FLTP:
			return AUDIO_F32SYS;
		default:
			break;
	}

	return 0;
}

int MediaPlayer::LVP_AudioSpecs::getSampleRate(int sampleRate, double playbackSpeed)
{
	if (ARE_EQUAL_DOUBLES(playbackSpeed, 1.0))
		return sampleRate;

	return (int)std::ceil((double)sampleRate / playbackSpeed);
}
