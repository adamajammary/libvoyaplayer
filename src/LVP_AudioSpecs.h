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
			LVP_AudioSpecs(AVFrame* frame, double playbackSpeed);
			LVP_AudioSpecs(AVFrame* frame);
			LVP_AudioSpecs();

		public:
			AVChannelLayout channelLayout;
			int             format;
			double          playbackSpeed;
			int             sampleRate;

		public:
			bool equals(AVFrame* frame, double playbackSpeed) const;

		public:
			static AVChannelLayout getChannelLayout(int channels);
			static std::string     getChannelLayoutName(const AVChannelLayout& layout);
			static AVSampleFormat  getSampleFormat(SDL_AudioFormat sdlFormat);
			static SDL_AudioFormat getSampleFormat(AVSampleFormat avFormat);
			static int             getSampleRate(int sampleRate, double playbackSpeed);
		};
	}
}

#endif
