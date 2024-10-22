#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_VIDEOCONTEXT_H
#define LVP_VIDEOCONTEXT_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		class LVP_VideoContext : public LVP_MediaContext
		{
		public:
			LVP_VideoContext();
			~LVP_VideoContext();

		public:
			LibFFmpeg::AVFrame*      frame;
			LibFFmpeg::AVFrame*      frameEncoded;
			LibFFmpeg::AVFrame*      frameHardware;
			LibFFmpeg::AVFrame*      frameSoftware;
			double                   frameRate;
			LibFFmpeg::AVPixelFormat hardwareFormat;
			bool                     isReadyForRender;
			bool                     isReadyForPresent;
			bool                     isSoftwareRenderer;
			double                   pts;
			SDL_Renderer*            renderer;
			LibFFmpeg::SwsContext*   scaleContext;
			SDL_Surface*             surface;
			SDL_Texture*             texture;
		};
	}
}

#endif
