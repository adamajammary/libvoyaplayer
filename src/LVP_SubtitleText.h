#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_SUBTITLETEXT_H
#define LVP_SUBTITLETEXT_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		class LVP_SubtitleText
		{
		private:
			LVP_SubtitleText()  {}
			~LVP_SubtitleText() {}

		private:
			static LibASS::ASS_Library*  library;
			static LibASS::ASS_Renderer* renderer;
			static LibASS::ASS_Track*    track;
			static std::mutex            trackLock;

		public:
			static void Init(LVP_SubtitleContext* subContext);
			static void ProcessEvent(LVP_Subtitle* subtitle);
			static void Quit();
			static void Remove();
			static void Render(SDL_Surface* surface, double progress);

		private:
			static LibASS::ASS_Image* create(double progress);
			static void               render(LibASS::ASS_Image* image, SDL_Surface* surface);
		};
	}
}

#endif
