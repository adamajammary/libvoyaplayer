#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_SUBBITMAP_H
#define LVP_SUBBITMAP_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		class LVP_SubtitleBitmap
		{
		private:
			LVP_SubtitleBitmap()  {}
			~LVP_SubtitleBitmap() {}

		public:
			static std::queue<LVP_Subtitle*> queue;
			static std::mutex                queueLock;

		private:
			static std::vector<LVP_Subtitle*> subs;
			static std::mutex                 subsLock;

		public:
			static void ProcessEvent(LVP_Subtitle* subtitle, const LibFFmpeg::AVSubtitleRect& frameRect);
			static void Remove();
			static void RemoveExpired(double progress);
			static void Render(SDL_Surface* surface, LVP_SubtitleContext* subContext, double progress);
			static void UpdateDVDColorPalette(void* context);
			static void UpdatePGSEndPTS(LibFFmpeg::AVPacket* packet, const LibFFmpeg::AVRational& timeBase);

		private:
			static void create(LVP_SubtitleContext* subContext);
			static void render(SDL_Surface* surface, double progress);
		};
	}
}

#endif
