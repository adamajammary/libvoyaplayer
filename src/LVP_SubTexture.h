#ifndef LVP_GLOBAL_H
	#include "LVP_Global.h"
#endif

#ifndef LVP_SUBTEXTURE_H
#define LVP_SUBTEXTURE_H

namespace LibVoyaPlayer
{
	namespace Graphics
	{
		class LVP_SubTexture
		{
		public:
			LVP_SubTexture();
			LVP_SubTexture(const LVP_SubTexture &textureR);
			LVP_SubTexture(MediaPlayer::LVP_Subtitle* subtitle);
			~LVP_SubTexture();

		public:
			SDL_Rect                   locationRender;
			bool                       offsetY;
			LVP_SubTexture*            outline;
			LVP_SubTexture*            shadow;
			MediaPlayer::LVP_Subtitle* subtitle;
			LVP_Texture*               textureData;
			uint16_t*                  textUTF16;
			SDL_Rect                   total;

		};
	}
}

#endif
