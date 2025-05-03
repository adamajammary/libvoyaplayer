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
			uint8_t*                        buffer;
			int                             bufferOffset;
			int                             bufferSize;
			int                             dataSize;
			SDL_AudioSpec                   deviceSpecs;
			SDL_AudioSpec                   deviceSpecsWanted;
			LVP_AudioFilter                 filter;
			LVP_AudioSpecs                  filterSpecs;
			std::queue<LibFFmpeg::AVFrame*> frames;
			std::mutex                      framesLock;
			double                          lastPogress;
			double                          packetDuration;

		public:
			void free();

		private:
			void clearFrames();
		};
	}
}

#endif
