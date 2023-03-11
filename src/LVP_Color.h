#ifndef LVP_GLOBAL_H
	#include "LVP_Global.h"
#endif

#ifndef LVP_COLOR_H
#define LVP_COLOR_H

namespace LibVoyaPlayer
{
	namespace Graphics
	{
		class LVP_Color
		{
		public:
			LVP_Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
			LVP_Color(const LVP_Color& color);
			LVP_Color(const SDL_Color& color);
			LVP_Color();

		public:
			bool operator ==(const LVP_Color &color);
			bool operator !=(const LVP_Color &color);
			operator SDL_Color();

		public:
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;

		public:
			bool IsEmpty();

		};
	}
}

#endif
