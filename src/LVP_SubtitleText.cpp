#include "LVP_SubtitleText.h"

LibASS::ASS_Library*  MediaPlayer::LVP_SubtitleText::library   = NULL;
LibASS::ASS_Renderer* MediaPlayer::LVP_SubtitleText::renderer  = NULL;
LibASS::ASS_Track*    MediaPlayer::LVP_SubtitleText::track     = NULL;
std::mutex            MediaPlayer::LVP_SubtitleText::trackLock = {};

LibASS::ASS_Image* MediaPlayer::LVP_SubtitleText::create(double progress)
{
	if (LVP_SubtitleText::track == NULL)
		return NULL;

	LVP_SubtitleText::trackLock.lock();

	auto time  = (long long)(progress * ONE_SECOND_MS);
	auto image = LibASS::ass_render_frame(LVP_SubtitleText::renderer, LVP_SubtitleText::track, time, NULL);

	LVP_SubtitleText::trackLock.unlock();

	return image;
}

void MediaPlayer::LVP_SubtitleText::Init(LVP_SubtitleContext* subContext)
{
	LVP_SubtitleText::Quit();

	LVP_SubtitleText::library  = LibASS::ass_library_init();
	LVP_SubtitleText::renderer = LibASS::ass_renderer_init(LVP_SubtitleText::library);
	LVP_SubtitleText::track    = LibASS::ass_new_track(LVP_SubtitleText::library);

	for (uint32_t i = 0; i < subContext->formatContext->nb_streams; i++)
	{
		auto stream = subContext->formatContext->streams[i];

		if (!LVP_Media::IsStreamWithFontAttachments(stream))
			continue;

		auto fontName = LibFFmpeg::av_dict_get(stream->metadata, "filename", NULL, 0);

		if ((fontName != NULL) && (fontName->value != NULL))
			LibASS::ass_add_font(LVP_SubtitleText::library, fontName->value, (const char*)stream->codecpar->extradata, stream->codecpar->extradata_size);
	}

	LibASS::ass_set_fonts(LVP_SubtitleText::renderer, NULL, "sans-serif", LibASS::ASS_FONTPROVIDER_AUTODETECT, NULL, 1);

	LibASS::ass_set_frame_size(LVP_SubtitleText::renderer,   subContext->videoSize.width, subContext->videoSize.height);
	LibASS::ass_set_storage_size(LVP_SubtitleText::renderer, subContext->videoSize.width, subContext->videoSize.height);

	LibASS::ass_process_codec_private(LVP_SubtitleText::track, (const char*)subContext->codec->extradata, subContext->codec->extradata_size);
}

void MediaPlayer::LVP_SubtitleText::ProcessEvent(LVP_Subtitle* subtitle)
{
	if ((subtitle == NULL) || subtitle->dialogue.empty() || (LVP_SubtitleText::track == NULL))
		return;

	auto start    = (long long)(subtitle->pts.start * ONE_SECOND_MS);
	auto duration = (long long)((subtitle->pts.end - subtitle->pts.start) * ONE_SECOND_MS);

	LVP_SubtitleText::trackLock.lock();

	LibASS::ass_process_chunk(LVP_SubtitleText::track, subtitle->dialogue.c_str(), subtitle->dialogue.size(), start, duration);

	LVP_SubtitleText::trackLock.unlock();
}

void MediaPlayer::LVP_SubtitleText::Quit()
{
	LVP_SubtitleText::Remove();

	FREE_ASS_TRACK(LVP_SubtitleText::track);
	FREE_ASS_RENDERER(LVP_SubtitleText::renderer);
	FREE_ASS_LIBRARY(LVP_SubtitleText::library);
}

void MediaPlayer::LVP_SubtitleText::Remove()
{
	if (LVP_SubtitleText::track != NULL)
		LibASS::ass_flush_events(LVP_SubtitleText::track);
}

void MediaPlayer::LVP_SubtitleText::Render(SDL_Surface* surface, double progress)
{
	auto image = LVP_SubtitleText::create(progress);

	LVP_SubtitleText::render(image, surface);
}

void MediaPlayer::LVP_SubtitleText::render(LibASS::ASS_Image* image, SDL_Surface* surface)
{
	while (image != NULL)
	{
		if ((image->w < 1) || (image->h < 1)) {
			image = image->next;
			continue;
		}

		SDL_Color srcColor = {
			((image->color >> 24) & 0xFF),
			((image->color >> 16) & 0xFF),
			((image->color >>  8) & 0xFF),
			(0xFF - (image->color & 0xFF))
		};

		auto destColors = surface->format->BytesPerPixel;
		auto destPitch  = surface->pitch;
		auto destPixels = (uint8_t*)surface->pixels;

		SDL_Point destPosition = {
			(image->dst_x * destColors),
			image->dst_y
		};

		for (int y1 = 0, y2 = destPosition.y; (y1 < image->h) && (y2 < surface->h); y1++, y2++)
		{
			auto srcRow  = (y1 * image->stride);
			auto destRow = (y2 * destPitch);

			for (int x1 = 0, x2 = destPosition.x; (x1 < image->w) && (x2 < destPitch); x1++, x2 += destColors)
			{
				auto srcColorAlpha  = (srcColor.a * image->bitmap[srcRow + x1]);

				if (srcColorAlpha == 0)
					continue;

				auto srcColorRed   = (srcColor.r * srcColorAlpha);
				auto srcColorGreen = (srcColor.g * srcColorAlpha);
				auto srcColorBlue  = (srcColor.b * srcColorAlpha);

				auto destColorAlpha = (MAX_255x255 - srcColorAlpha);
				auto destColorRed   = (destPixels[destRow + x2 + LVP_RGBA_R] * destColorAlpha);
				auto destColorGreen = (destPixels[destRow + x2 + LVP_RGBA_G] * destColorAlpha);
				auto destColorBlue  = (destPixels[destRow + x2 + LVP_RGBA_B] * destColorAlpha);

				destPixels[destRow + x2 + LVP_RGBA_R] = ((srcColorRed   + destColorRed)   / MAX_255x255);
				destPixels[destRow + x2 + LVP_RGBA_G] = ((srcColorGreen + destColorGreen) / MAX_255x255);
				destPixels[destRow + x2 + LVP_RGBA_B] = ((srcColorBlue  + destColorBlue)  / MAX_255x255);
			}
		}

		image = image->next;
	}
}
