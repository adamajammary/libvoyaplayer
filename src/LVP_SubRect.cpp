#include "LVP_SubRect.h"

// https://code.soundsoftware.ac.uk/projects/pmhd/embedded/pgssubdec_8c_source.html
MediaPlayer::LVP_SubRect::LVP_SubRect(const LibFFmpeg::AVSubtitleRect &subRect)
{
	this->x = subRect.x;
	this->y = subRect.y;
	this->w = subRect.w;
	this->h = subRect.h;

	this->nb_colors = subRect.nb_colors;
	this->flags     = subRect.flags;
	this->type      = subRect.type;

	this->ass  = (subRect.ass  != NULL ? std::string(subRect.ass)  : "");
	this->text = (subRect.text != NULL ? std::string(subRect.text) : "");

	for (int i = 0; i < NR_OF_DATA_PLANES; i++)
		this->linesize[i] = subRect.linesize[i];

	auto DATA_SIZE = (size_t)(subRect.linesize[0] * subRect.h);

	this->data[0] = (uint8_t*)LibFFmpeg::av_memdup(subRect.data[0], DATA_SIZE);
	this->data[1] = (uint8_t*)LibFFmpeg::av_memdup(subRect.data[1], AVPALETTE_SIZE);
	this->data[2] = NULL;
	this->data[3] = NULL;
}

MediaPlayer::LVP_SubRect::LVP_SubRect()
{
	this->x = this->y = this->w = this->h = this->nb_colors = this->flags = 0;

	this->type = LibFFmpeg::SUBTITLE_NONE;
	this->text = this->ass = "";

	for (int i = 0; i < NR_OF_DATA_PLANES; i++)
		this->linesize[i] = 0;

	for (int i = 0; i < NR_OF_DATA_PLANES; i++)
		this->data[i] = NULL;
}

MediaPlayer::LVP_SubRect::~LVP_SubRect()
{
	this->x = this->y = this->w = this->h = this->nb_colors = this->flags = 0;

	this->type = LibFFmpeg::SUBTITLE_NONE;
	this->text = this->ass = "";

	for (int i = 0; i < NR_OF_DATA_PLANES; i++)
		this->linesize[i] = 0;

	for (int i = 0; i < NR_OF_DATA_PLANES; i++)
		FREE_AVPOINTER(this->data[i]);
}

SDL_Rect MediaPlayer::LVP_SubRect::getSdlRect()
{
	return { this->x, this->y, this->w, this->h };
}
