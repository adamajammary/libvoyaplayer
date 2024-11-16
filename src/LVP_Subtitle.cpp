#include "LVP_Subtitle.h"

MediaPlayer::LVP_Subtitle::LVP_Subtitle(const LibFFmpeg::AVSubtitleRect& frameRect, const LVP_PTS& pts)
{
	this->bitmap      = {};
	this->dialogue    = "";
	this->pts         = pts;
	this->surface     = NULL;
	this->type        = frameRect.type;

	switch (this->type) {
		case LibFFmpeg::SUBTITLE_BITMAP: this->bitmap   = frameRect; break;
		case LibFFmpeg::SUBTITLE_ASS:    this->dialogue = frameRect.ass; break;
		case LibFFmpeg::SUBTITLE_TEXT:   this->dialogue = frameRect.text; break;
		default: break;
	}
}

MediaPlayer::LVP_Subtitle::~LVP_Subtitle()
{
	FREE_SUB_DATA(this->bitmap.data[0]);
	FREE_SUB_DATA(this->bitmap.data[1]);

	FREE_SURFACE(this->surface);
}

bool MediaPlayer::LVP_Subtitle::isExpiredPTS(double progress) const
{
	return ((this->pts.end > 0.0) && ((progress + MIN_SUB_TIME_TO_PTS) >= this->pts.end));
}

bool MediaPlayer::LVP_Subtitle::isReadyPTS(double progress) const
{
	return ((progress + MIN_SUB_TIME_TO_PTS) >= this->pts.start);
}
