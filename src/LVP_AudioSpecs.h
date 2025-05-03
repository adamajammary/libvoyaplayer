#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_AUDIOSPECS_H
#define LVP_AUDIOSPECS_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		class LVP_AudioSpecs
		{
		public:
			LVP_AudioSpecs(LibFFmpeg::AVFrame* frame, double playbackSpeed);
			LVP_AudioSpecs(LibFFmpeg::AVFrame* frame);
			LVP_AudioSpecs();

		public:
			LibFFmpeg::AVChannelLayout channelLayout;
			int                        format;
			double                     playbackSpeed;
			int                        sampleRate;

		public:
			bool equals(LibFFmpeg::AVFrame* frame, double playbackSpeed) const;

		public:
			static LibFFmpeg::AVChannelLayout getChannelLayout(int channels);
			static std::string                getChannelLayoutName(const LibFFmpeg::AVChannelLayout& layout);
			static LibFFmpeg::AVSampleFormat  getSampleFormat(SDL_AudioFormat sdlFormat);
			static SDL_AudioFormat            getSampleFormat(LibFFmpeg::AVSampleFormat avFormat);
			static int                        getSampleRate(int sampleRate, double playbackSpeed);
		};
	}
}

#endif
