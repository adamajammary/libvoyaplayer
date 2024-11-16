#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_SUBTITLE_H
#define LVP_SUBTITLE_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		class LVP_Subtitle
		{
		public:
			LVP_Subtitle(const LibFFmpeg::AVSubtitleRect& frameRect, const LVP_PTS& pts);
			~LVP_Subtitle();

		public:
			LibFFmpeg::AVSubtitleRect bitmap;
			std::string               dialogue;
			LVP_PTS                   pts;
			SDL_Surface*              surface;
			LibFFmpeg::AVSubtitleType type;

		public:
			bool isExpiredPTS(double progress) const;
			bool isReadyPTS(double progress) const;
		};
	}
}

#endif
