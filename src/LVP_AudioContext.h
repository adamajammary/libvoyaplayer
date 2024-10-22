#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_AUDIOCONTEXT_H
#define LVP_AUDIOCONTEXT_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		class LVP_AudioContext : public LVP_MediaContext
		{
		public:
			LVP_AudioContext();
			~LVP_AudioContext();

		public:
			int                 bufferOffset;
			int                 bufferRemaining;
			int                 bufferSize;
			bool                decodeFrame;
			SDL_AudioSpec       deviceSpecs;
			SDL_AudioSpec       deviceSpecsWanted;
			LVP_AudioFilter     filter;
			LibFFmpeg::AVFrame* frame;
			double              frameDuration;
			uint8_t*            frameEncoded;
			bool                isDriverReady;
			double              lastPogress;
			LVP_AudioSpecs      specs;
			int                 writtenToStream;
		};
	}
}

#endif
