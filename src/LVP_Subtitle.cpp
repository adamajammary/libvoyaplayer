#include "LVP_Subtitle.h"

MediaPlayer::LVP_Subtitle::LVP_Subtitle()
{
	this->customPos      = false;
	this->customRotation = false;
	this->drawRect       = {};
	this->id             = 0;
	this->layer          = 0;
	this->position       = {};
	this->pts.end        = 0;
	this->pts.start      = 0;
	this->rotation       = 0.0;
	this->rotationPoint  = {};
	this->skip           = false;
	this->style          = NULL;
	this->subRect        = NULL;
	this->text           = "";
	this->text2          = "";
	this->text3          = "";
	this->textUTF16      = NULL;
}

MediaPlayer::LVP_Subtitle::~LVP_Subtitle()
{
	DELETE_POINTER(this->style);
	DELETE_POINTER(this->subRect);

	SDL_free(this->textUTF16);
}

void MediaPlayer::LVP_Subtitle::copy(const LVP_Subtitle &subtitle)
{
	this->customPos      = subtitle.customPos;
	this->customRotation = subtitle.customRotation;
	this->drawRect       = subtitle.drawRect;
	this->position       = subtitle.position;
	this->pts.start      = subtitle.pts.start;
	this->pts.end        = subtitle.pts.end;
	this->rotation       = subtitle.rotation;
	this->rotationPoint  = subtitle.rotationPoint;
	this->subRect        = subtitle.subRect;

	this->style->copy(*subtitle.style);
}

MediaPlayer::LVP_SubAlignment MediaPlayer::LVP_Subtitle::getAlignment() const
{
	return this->style->alignment;
}

int MediaPlayer::LVP_Subtitle::getBlur() const
{
	return this->style->blur;
}

Graphics::LVP_Color MediaPlayer::LVP_Subtitle::getColor() const
{
	return this->style->colorPrimary;
}

Graphics::LVP_Color MediaPlayer::LVP_Subtitle::getColorOutline() const
{
	return this->style->colorOutline;
}

Graphics::LVP_Color MediaPlayer::LVP_Subtitle::getColorShadow() const
{
	return this->style->colorShadow;
}

TTF_Font* MediaPlayer::LVP_Subtitle::getFont(LVP_SubtitleContext &subContext)
{
	return this->style->getFont(subContext);
}

SDL_Rect MediaPlayer::LVP_Subtitle::getMargins(const SDL_FPoint &scale)
{
	return {
		(int)ceilf((float)this->style->marginL * scale.x),
		(int)ceilf((float)this->style->marginR * scale.x),
		(int)ceilf((float)this->style->marginV * scale.y),
		(int)ceilf((float)this->style->marginV * scale.y)
	};
}

int MediaPlayer::LVP_Subtitle::getOutline(const SDL_FPoint &scale) const
{
	return (int)ceilf((float)this->style->outline * scale.y);
}

SDL_Point MediaPlayer::LVP_Subtitle::getShadow(const SDL_FPoint &scale)
{
	return {
		(int)ceilf((float)this->style->shadow.x * scale.x),
		(int)ceilf((float)this->style->shadow.y * scale.y),
	};
}

bool MediaPlayer::LVP_Subtitle::isAlignedBottom()
{
	auto a = this->getAlignment();

	return ((a == SUB_ALIGN_BOTTOM_LEFT) || (a == SUB_ALIGN_BOTTOM_RIGHT) || (a == SUB_ALIGN_BOTTOM_CENTER));
}

bool MediaPlayer::LVP_Subtitle::isAlignedCenter()
{
	auto a = this->getAlignment();

	return ((a == SUB_ALIGN_BOTTOM_CENTER) || (a == SUB_ALIGN_TOP_CENTER) || (a == SUB_ALIGN_MIDDLE_CENTER));
}

bool MediaPlayer::LVP_Subtitle::isAlignedLeft()
{
	auto a = this->getAlignment();

	return ((a == SUB_ALIGN_BOTTOM_LEFT) || (a == SUB_ALIGN_TOP_LEFT) || (a == SUB_ALIGN_MIDDLE_LEFT));
}

bool MediaPlayer::LVP_Subtitle::isAlignedMiddle()
{
	auto a = this->getAlignment();

	return ((a == SUB_ALIGN_MIDDLE_LEFT) || (a == SUB_ALIGN_MIDDLE_RIGHT) || (a == SUB_ALIGN_MIDDLE_CENTER));
}

bool MediaPlayer::LVP_Subtitle::isAlignedRight()
{
	auto a = this->getAlignment();

	return ((a == SUB_ALIGN_BOTTOM_RIGHT) || (a == SUB_ALIGN_TOP_RIGHT) || (a == SUB_ALIGN_MIDDLE_RIGHT));
}

bool MediaPlayer::LVP_Subtitle::isAlignedTop()
{
	auto a = this->getAlignment();

	return ((a == SUB_ALIGN_TOP_LEFT) || (a == SUB_ALIGN_TOP_RIGHT) || (a == SUB_ALIGN_TOP_CENTER));
}

bool MediaPlayer::LVP_Subtitle::isDuplicate(const LVP_Subtitles& subs) const
{
	if (!this->customPos)
		return false;

	for (auto sub : subs)
	{
		if (!sub->customPos)
			continue;

		if ((sub->position.x == this->position.x) &&
			(sub->position.y == this->position.y) &&
			(sub->pts.start  == this->pts.start)  &&
			(sub->pts.end    == this->pts.end))
		{
			return true;
		}
	}

	return false;
}

bool MediaPlayer::LVP_Subtitle::isExpiredPTS(const LVP_SubtitleContext &subContext, double progress) const
{
	auto ptsEnd    = (this->pts.end - 0.001);
	bool isExpired = ((ptsEnd <= subContext.pts.start) || (ptsEnd <= progress));

	return isExpired;
}

bool MediaPlayer::LVP_Subtitle::isSeekedPTS(const LVP_SubtitleContext& subContext) const
{
	bool isFuturePTS = (this->pts.start > subContext.nextPTS.end);

	return isFuturePTS;
}

bool MediaPlayer::LVP_Subtitle::overlaps(LVP_Subtitle* subtitle) const
{
	bool overlaps = !(
		(this->pts.end < subtitle->pts.start) ||
		!ARE_DIFFERENT_DOUBLES(this->pts.end, subtitle->pts.start) ||
		(this->pts.start > subtitle->pts.end) ||
		!ARE_DIFFERENT_DOUBLES(this->pts.start, subtitle->pts.end)
	);

	return overlaps;
}

bool MediaPlayer::LVP_Subtitle::skipRender(bool layered) const
{
	return (this->skip || (!layered && (this->layer > 0)) || (layered && (this->layer < 1)));
}
