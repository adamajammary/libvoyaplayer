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
			LVP_AudioSpecs();

		public:
			LibFFmpeg::AVChannelLayout channelLayout;
			int                        format;
			bool                       initContext;
			double                     playbackSpeed;
			int                        sampleRate;
			LibFFmpeg::SwrContext*     swrContext;

		public:
			bool hasChanged(LibFFmpeg::AVFrame* frame, double playbackSpeed) const;
			void init(LibFFmpeg::AVFrame* frame, double playbackSpeed);
		};
	}
}

#endif
