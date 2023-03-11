#ifndef LVP_GLOBAL_H
	#include "LVP_Global.h"
#endif

#ifndef LVP_SUBRECT_H
#define LVP_SUBRECT_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		const int NR_OF_DATA_PLANES = 4;

		class LVP_SubRect
		{
		public:
			LVP_SubRect(const LibFFmpeg::AVSubtitleRect &subRect);
			LVP_SubRect();
			~LVP_SubRect();

		public:
			int                       x, y, w, h;
			int                       nb_colors;
			uint8_t*                  data[NR_OF_DATA_PLANES];
			int                       linesize[NR_OF_DATA_PLANES];
			LibFFmpeg::AVSubtitleType type;
			std::string               text;
			std::string               ass;
			int                       flags;

		public:
			SDL_Rect getSdlRect();

		};
	}
}

#endif
