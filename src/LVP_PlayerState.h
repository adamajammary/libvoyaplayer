#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_PLAYERSTATE_H
#define LVP_PLAYERSTATE_H

#include <libvoyaplayer_events.h>

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		struct LVP_AudioDevice
		{
			SDL_AudioDeviceID id   = 0;
			std::string       name = "Default";
		};

		struct LVP_SeekState
		{
			bool isRequested       = false;
			bool isRequestedBack   = false;
			bool isRequestedPaused = false;

			int    by  = 0;
			double pts = 0.0;
			double to  = 0.0;
		};

		struct LVP_TrackState
		{
			uint32_t                   count     = 0;
			bool                       requested = false;
			std::queue<LVP_MediaTrack> requests  = {};
		};

		class LVP_PlayerState
		{
		public:
			LVP_PlayerState();

		public:
			bool isCompleted;
			bool isMuted;
			bool isOpening;
			bool isPaused;
			bool isPlaying;
			bool isStopped;
			bool isStopping;

			LVP_AudioDevice  audioDevice;
			LVP_AudioDevices audioDevices;

			std::string filePath;
			size_t      fileSize;

			int64_t        duration;
			AVMediaType    mediaType;
			std::string    openFilePath;
			double         playbackSpeed;
			double         progress;
			bool           quit;
			LVP_SeekState  seek;
			bool           threads[LVP_NR_OF_THREADS];
			LVP_TrackState track;
			float          volume;

		public:
			void reset();
		};
	}
}

#endif
