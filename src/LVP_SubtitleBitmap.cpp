#include "LVP_SubtitleBitmap.h"

std::queue<MediaPlayer::LVP_Subtitle*>  MediaPlayer::LVP_SubtitleBitmap::queue;
std::mutex                              MediaPlayer::LVP_SubtitleBitmap::queueLock;
std::vector<MediaPlayer::LVP_Subtitle*> MediaPlayer::LVP_SubtitleBitmap::subs;
std::mutex                              MediaPlayer::LVP_SubtitleBitmap::subsLock;

void MediaPlayer::LVP_SubtitleBitmap::create(LVP_SubtitleContext* subContext)
{
	LVP_SubtitleBitmap::queueLock.lock();

	while (!LVP_SubtitleBitmap::queue.empty())
	{
		auto subtitle = LVP_SubtitleBitmap::queue.front();

		LVP_SubtitleBitmap::queue.pop();

		if (!IS_SUB_BITMAP(subtitle->type) || (subtitle->bitmap.w <= 0) || (subtitle->bitmap.h <= 0))
			continue;

		subtitle->surface = SDL_CreateSurface(subtitle->bitmap.w, subtitle->bitmap.h, SDL_PIXELFORMAT_RGBA32);

		sws_convertPalette8ToPacked32(
			subtitle->bitmap.data[0],
			(uint8_t*)subtitle->surface->pixels,
			(subtitle->bitmap.w * subtitle->bitmap.h),
			subtitle->bitmap.data[1]
		);

		if (!subContext->isEqualToVideoSize())
		{
			auto scaledSurface = LVP_SubtitleBitmap::scale(subContext, subtitle);

			SDL_DestroySurface(subtitle->surface);

			subtitle->surface = scaledSurface;
		}

		LVP_SubtitleBitmap::subsLock.lock();

		LVP_SubtitleBitmap::subs.push_back(subtitle);

		LVP_SubtitleBitmap::subsLock.unlock();
	}

	LVP_SubtitleBitmap::queueLock.unlock();
}

void MediaPlayer::LVP_SubtitleBitmap::ProcessEvent(LVP_Subtitle* subtitle, const AVSubtitleRect& frameRect)
{
	if (subtitle == NULL)
		return;

	subtitle->bitmap.data[0] = (uint8_t*)av_memdup(frameRect.data[0], (size_t)(frameRect.linesize[0] * frameRect.h));
	subtitle->bitmap.data[1] = (uint8_t*)av_memdup(frameRect.data[1], AVPALETTE_SIZE);
	subtitle->bitmap.data[2] = NULL;
	subtitle->bitmap.data[3] = NULL;
}

void MediaPlayer::LVP_SubtitleBitmap::Remove()
{
	LVP_SubtitleBitmap::subsLock.lock();

	for (auto subtitle : LVP_SubtitleBitmap::subs)
		DELETE_POINTER(subtitle);

	LVP_SubtitleBitmap::subs.clear();

	LVP_SubtitleBitmap::subsLock.unlock();

	LVP_SubtitleBitmap::queueLock.lock();

	while (!LVP_SubtitleBitmap::queue.empty()) {
		DELETE_POINTER(LVP_SubtitleBitmap::queue.front());
		LVP_SubtitleBitmap::queue.pop();
	}

	LVP_SubtitleBitmap::queueLock.unlock();
}

void MediaPlayer::LVP_SubtitleBitmap::RemoveExpired(double progress)
{
	LVP_SubtitleBitmap::subsLock.lock();

	auto iter = LVP_SubtitleBitmap::subs.begin();

	while (iter != LVP_SubtitleBitmap::subs.end())
	{
		if ((*iter)->isExpiredPTS(progress)) {
			DELETE_POINTER(*iter);
			iter = LVP_SubtitleBitmap::subs.erase(iter);
		} else {
			iter++;
		}
	}

	LVP_SubtitleBitmap::subsLock.unlock();
}

void MediaPlayer::LVP_SubtitleBitmap::Render(SDL_Surface* videoSurface, LVP_SubtitleContext* subContext, double progress)
{
	LVP_SubtitleBitmap::create(subContext);

	LVP_SubtitleBitmap::render(videoSurface, progress);
}

void MediaPlayer::LVP_SubtitleBitmap::render(SDL_Surface* videoSurface, double progress)
{
	LVP_SubtitleBitmap::subsLock.lock();

	for (auto subtitle : LVP_SubtitleBitmap::subs) {
		if (subtitle->isReadyPTS(progress))
			LVP_SubtitleBitmap::render(subtitle, videoSurface);
	}

	LVP_SubtitleBitmap::subsLock.unlock();
}

void MediaPlayer::LVP_SubtitleBitmap::render(LVP_Subtitle* subtitle, SDL_Surface* videoSurface)
{
	auto srcColors = SDL_BYTESPERPIXEL(subtitle->surface->format);
	auto srcPitch  = subtitle->surface->pitch;
	auto srcPixels = (uint8_t*)subtitle->surface->pixels;

	auto destColors = SDL_BYTESPERPIXEL(videoSurface->format);
	auto destPitch  = videoSurface->pitch;
	auto destPixels = (uint8_t*)videoSurface->pixels;

	SDL_Point destPosition = {
		(subtitle->bitmap.x * destColors),
		subtitle->bitmap.y
	};

	for (int y1 = 0, y2 = destPosition.y; (y1 < subtitle->surface->h) && (y2 < videoSurface->h); y1++, y2++)
	{
		auto srcRow  = (y1 * srcPitch);
		auto destRow = (y2 * destPitch);

		for (int x1 = 0, x2 = destPosition.x; (x1 < srcPitch) && (x2 < destPitch); x1 += srcColors, x2 += destColors)
		{
			auto srcColorAlpha = (srcPixels[srcRow + x1 + LVP_RGBA_A] * 0xFF);;

			if (srcColorAlpha == 0)
				continue;

			auto srcColorRed   = (srcPixels[srcRow + x1 + LVP_RGBA_R] * srcColorAlpha);
			auto srcColorGreen = (srcPixels[srcRow + x1 + LVP_RGBA_G] * srcColorAlpha);
			auto srcColorBlue  = (srcPixels[srcRow + x1 + LVP_RGBA_B] * srcColorAlpha);

			auto destColorAlpha = (MAX_255x255 - srcColorAlpha);
			auto destColorRed   = (destPixels[destRow + x2 + LVP_RGBA_R] * destColorAlpha);
			auto destColorGreen = (destPixels[destRow + x2 + LVP_RGBA_G] * destColorAlpha);
			auto destColorBlue  = (destPixels[destRow + x2 + LVP_RGBA_B] * destColorAlpha);

			destPixels[destRow + x2 + LVP_RGBA_R] = ((srcColorRed   + destColorRed)   / MAX_255x255);
			destPixels[destRow + x2 + LVP_RGBA_G] = ((srcColorGreen + destColorGreen) / MAX_255x255);
			destPixels[destRow + x2 + LVP_RGBA_B] = ((srcColorBlue  + destColorBlue)  / MAX_255x255);
		}
	}
}

SDL_Surface* MediaPlayer::LVP_SubtitleBitmap::scale(LVP_SubtitleContext* subContext, LVP_Subtitle* subtitle)
{
	if ((subContext == NULL) || (subtitle == NULL) || (subtitle->surface == NULL))
		return NULL;

	auto scaleX = ((float)subContext->videoSize.width  / (float)subContext->codec->width);
	auto scaleY = ((float)subContext->videoSize.height / (float)subContext->codec->height);

	subtitle->bitmap.x = (int)((float)subtitle->bitmap.x * scaleX);
	subtitle->bitmap.y = (int)((float)subtitle->bitmap.y * scaleY);

	subtitle->bitmap.w = (int)((float)subtitle->bitmap.w * scaleX);
	subtitle->bitmap.h = (int)((float)subtitle->bitmap.h * scaleY);

	auto surface = SDL_CreateSurface(subtitle->bitmap.w, subtitle->bitmap.h, SDL_PIXELFORMAT_RGBA32);

	SDL_BlitSurfaceScaled(subtitle->surface, NULL, surface, NULL, SDL_SCALEMODE_LINEAR);

	return surface;
}

void MediaPlayer::LVP_SubtitleBitmap::UpdateDVDColorPalette(void* context)
{
	// Replace yellow DVD subs with white subs (if it's missing the color palette).

	auto dvdSubContext = static_cast<LVP_DVDSubContext*>(context);

	if ((dvdSubContext == NULL) || dvdSubContext->has_palette)
		return;

	dvdSubContext->has_palette = 1;

	dvdSubContext->palette[dvdSubContext->colormap[1]] = 0xFFFFFFFF; // TEXT
	dvdSubContext->palette[dvdSubContext->colormap[3]] = 0xFF000000; // BORDER
}

void MediaPlayer::LVP_SubtitleBitmap::UpdatePGSEndPTS(double pts)
{
	// Some sub types, like PGS, come in pairs:
	// 1  - The first one with data but without duration or end PTS.
	// 2a - The second one has no data, but contains the end PTS.
	// 2b - Or the second one has data, and previous subs get second start PTS as their end PTS.

	LVP_SubtitleBitmap::subsLock.lock();

	for (auto sub : LVP_SubtitleBitmap::subs)
	{
		if (sub->pts.end > 0.0)
			continue;

		sub->pts.end = pts;

		#if defined _DEBUG
			printf("[%.3f,%.3f] %d,%d %dx%d\n", sub->pts.start, sub->pts.end, sub->bitmap.x, sub->bitmap.y, sub->bitmap.w, sub->bitmap.h);
		#endif
	}

	LVP_SubtitleBitmap::subsLock.unlock();
}
