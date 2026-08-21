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
			AVFrame*      frame;
			AVFrame*      frameEncoded;
			AVFrame*      frameHardware;
			AVFrame*      frameSoftware;
			AVBufferRef*  hwDeviceContext;
			AVPixelFormat hwPixelFormat;
			bool          isReadyForRender;
			bool          isReadyForPresent;
			bool          isSoftwareRenderer;
			double        pts;
			SDL_Renderer* renderer;
			SwsContext*   scaleContext;
			SDL_Surface*  surface;
			SDL_Texture*  texture;

		public:
			int getTimeUntilPTS(double progress) const;
		};
	}
}

#endif
