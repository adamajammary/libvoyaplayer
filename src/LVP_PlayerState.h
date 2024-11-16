#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_PLAYERSTATE_H
#define LVP_PLAYERSTATE_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		class LVP_PlayerState
		{
		public:
			LVP_PlayerState();

		public:
			bool                   completed;
			int64_t                duration;
			std::string            filePath;
			size_t                 fileSize;
			bool                   isMuted;
			bool                   isPaused;
			bool                   isPlaying;
			bool                   isStopped;
			LibFFmpeg::AVMediaType mediaType;
			std::string            openFilePath;
			double                 playbackSpeed;
			double                 progress;
			bool                   quit;
			bool                   threads[LVP_NR_OF_THREADS];
			unsigned int           trackCount;
			int                    volume;

		public:
			void reset();
		};
	}
}

#endif
