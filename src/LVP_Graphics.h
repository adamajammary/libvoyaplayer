#ifndef LVP_GLOBAL_H
	#include "LVP_Global.h"
#endif

#ifndef LVP_GRAPHICS_H
#define LVP_GRAPHICS_H

namespace LibVoyaPlayer
{
	namespace Graphics
	{
		class LVP_Graphics
		{
		private:
			LVP_Graphics()  {}
			~LVP_Graphics() {}

		public:
			static void      FillArea(const LVP_Color &color, const SDL_Rect &area, SDL_Renderer* renderer);
			static void      FillBorder(const LVP_Color &color, const SDL_Rect &area, const LVP_Border &borderThickness, SDL_Renderer* renderer);
			static LVP_Color ToLVPColor(const std::string &colorHex);

		};
	}
}

#endif
