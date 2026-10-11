#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_SUBBITMAP_H
#define LVP_SUBBITMAP_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		// libavcodec/dvdsubdec.c
		struct LVP_DVDSubContext
		{
			AVClass* av_class;
			uint32_t palette[16];
			char*    palette_str;
			char*    ifo_str;
			int      has_palette;
			uint8_t  colormap[4];
			uint8_t  alpha[256];
			uint8_t  buf[0x10000];
			int      buf_size;
			int      forced_subs_only;
		};

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
			static void ProcessEvent(LVP_Subtitle* subtitle, const AVSubtitleRect& frameRect);
			static void Remove();
			static void RemoveExpired(double progress);
			static void Render(SDL_Surface* videoSurface, LVP_SubtitleContext* subContext, double progress);
			static void UpdateDVDColorPalette(void* context);
			static void UpdatePGSEndPTS(double pts);

		private:
			static void         create(LVP_SubtitleContext* subContext);
			static void         render(SDL_Surface* videoSurface, double progress);
			static void         render(LVP_Subtitle* subtitle, SDL_Surface* videoSurface);
			static SDL_Surface* scale(LVP_SubtitleContext* subContext, LVP_Subtitle* subtitle);
		};
	}
}

#endif
