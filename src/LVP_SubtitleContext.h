#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_SUBTITLECONTEXT_H
#define LVP_SUBTITLECONTEXT_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		class LVP_SubtitleContext : public LVP_MediaContext
		{
		public:
			LVP_SubtitleContext();
			~LVP_SubtitleContext();

		public:
			LVP_Strings                 external;
			LibFFmpeg::AVFormatContext* formatContext;
			LibFFmpeg::AVFrame*         frameEncoded;
			LibFFmpeg::SwsContext*      scaleContext;
			LVP_Size                    videoSize;
		};
	}
}

#endif
