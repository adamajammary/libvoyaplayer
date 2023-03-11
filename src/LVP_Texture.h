#ifndef LVP_GLOBAL_H
	#include "LVP_Global.h"
#endif

#ifndef LVP_TEXTURE_H
#define LVP_TEXTURE_H

namespace LibVoyaPlayer
{
	namespace Graphics
	{
		class LVP_Texture
		{
		public:
			LVP_Texture(SDL_Surface* surface, SDL_Renderer* renderer);
			LVP_Texture(uint32_t format, int access, int width, int height, SDL_Renderer* renderer);
			~LVP_Texture();

		public:
			int          access;
			SDL_Texture* data;
			uint32_t     format;
			int          height;
			SDL_Point    scroll;
			int          width;

		private:
			void init();
			void setProperties();

		};
	}
}

#endif
