#include "LVP_SubStyle.h"

MediaPlayer::LVP_SubStyle::LVP_SubStyle()
{
	this->alignment    = SUB_ALIGN_BOTTOM_CENTER;
	this->borderStyle  = SUB_BORDER_STYLE_OUTLINE;
	this->blur         = 0;
	this->colorPrimary = LVP_COLOR_WHITE;
	this->colorOutline = {};
	this->colorShadow  = {};
	this->fontScale    = {};
	this->fontSize     = 0;
	this->fontStyle    = 0;
	this->marginL      = 0;
	this->marginR      = 0;
	this->marginV      = 0;
	this->name         = "";
	this->outline      = 0;
	this->rotation     = 0;
	this->shadow       = {};

	#if defined _windows
		this->fontName = L"";
	#else
		this->fontName = "";
	#endif
}

MediaPlayer::LVP_SubStyle::LVP_SubStyle(Strings data, LVP_SubStyleVersion version)
{
	this->alignment = SUB_ALIGN_BOTTOM_CENTER;
	this->blur      = 0;
	this->fontStyle = 0;
	this->marginL   = 0;
	this->marginR   = 0;
	this->marginV   = 0;
	this->outline   = 0;
	this->rotation  = 0;
	this->shadow    = {};
	this->version   = version;

	// STYLE NAME

	this->name = data[SUB_STYLE_NAME];

	// FONT NAME

	#if defined _windows
		auto fontName16 = (wchar_t*)System::LVP_Text::ToUTF16(data[SUB_STYLE_FONT_NAME].c_str());
		this->fontName  = std::wstring(fontName16);

		SDL_free(fontName16);
	#else
		this->fontName = data[SUB_STYLE_FONT_NAME];
	#endif

	// FONT SIZE

	this->fontSize = std::atoi(data[SUB_STYLE_FONT_SIZE].c_str());

	// FONT COLORS

	this->colorPrimary = Graphics::LVP_Graphics::ToLVPColor(data[SUB_STYLE_COLOR_PRIMARY]);
	this->colorOutline = Graphics::LVP_Graphics::ToLVPColor(data[SUB_STYLE_COLOR_BORDER]);
	this->colorShadow  = Graphics::LVP_Graphics::ToLVPColor(data[SUB_STYLE_COLOR_SHADOW]);

	if (data[SUB_STYLE_COLOR_SHADOW].size() < 10)
		this->colorShadow.a = 128;

	// FONT STYLE

	if (std::atoi(data[SUB_STYLE_BOLD].c_str()) != 0)
		this->fontStyle |= TTF_STYLE_BOLD;

	if (std::atoi(data[SUB_STYLE_ITALIC].c_str()) != 0)
		this->fontStyle |= TTF_STYLE_ITALIC;

	if (std::atoi(data[SUB_STYLE_UNDERLINE].c_str()) != 0)
		this->fontStyle |= TTF_STYLE_UNDERLINE;

	int shadow = 0;

	switch (this->version) {
	case SUB_STYLE_VERSION_4PLUS_ASS:
		this->alignment = (LVP_SubAlignment)std::atoi(data[SUB_STYLE_V4PLUS_ALIGNMENT].c_str());

		this->fontScale = {
			(float)(std::atof(data[SUB_STYLE_V4PLUS_SCALE_X].c_str()) * 0.01),
			(float)(std::atof(data[SUB_STYLE_V4PLUS_SCALE_Y].c_str()) * 0.01)
		};

		if (std::atoi(data[SUB_STYLE_V4PLUS_STRIKEOUT].c_str()) != 0)
			this->fontStyle |= TTF_STYLE_STRIKETHROUGH;

		this->borderStyle = (LVP_SubBorderStyle)std::atoi(data[SUB_STYLE_V4PLUS_BORDER_STYLE].c_str());

		this->outline = (int)std::round(std::atof(data[SUB_STYLE_V4PLUS_OUTLINE].c_str()));

		shadow = (int)std::round(std::atof(data[SUB_STYLE_V4PLUS_SHADOW].c_str()));

		this->marginL = (int)std::round(std::atof(data[SUB_STYLE_V4PLUS_MARGINL].c_str()));
		this->marginR = (int)std::round(std::atof(data[SUB_STYLE_V4PLUS_MARGINR].c_str()));
		this->marginV = (int)std::round(std::atof(data[SUB_STYLE_V4PLUS_MARGINV].c_str()));

		this->rotation = (std::atof(data[SUB_STYLE_V4PLUS_ROTATION_ANGLE].c_str()) * -1.0f);

		break;
	case SUB_STYLE_VERSION_4_SSA:
		this->alignment = LVP_SubStyle::ToSubAlignment(std::atoi(data[SUB_STYLE_V4_ALIGNMENT].c_str()));

		this->borderStyle = (LVP_SubBorderStyle)std::atoi(data[SUB_STYLE_V4_BORDER_STYLE].c_str());

		this->outline = (int)std::round(std::atof(data[SUB_STYLE_V4_OUTLINE].c_str()));

		shadow = (int)std::round(std::atof(data[SUB_STYLE_V4_SHADOW].c_str()));

		this->marginL = (int)std::round(std::atof(data[SUB_STYLE_V4_MARGINL].c_str()));
		this->marginR = (int)std::round(std::atof(data[SUB_STYLE_V4_MARGINR].c_str()));
		this->marginV = (int)std::round(std::atof(data[SUB_STYLE_V4_MARGINV].c_str()));

		break;
	default:
		break;
	}

	if (this->borderStyle == SUB_BORDER_STYLE_OUTLINE) {
		this->outline = std::max(1, this->outline);
		shadow        = std::max(1, shadow);
	}

	this->shadow = { shadow, shadow };
}

void MediaPlayer::LVP_SubStyle::copy(const LVP_SubStyle &subStyle)
{
	this->alignment    = subStyle.alignment;
	this->blur         = subStyle.blur;
	this->borderStyle  = subStyle.borderStyle;
	this->colorOutline = subStyle.colorOutline;
	this->colorPrimary = subStyle.colorPrimary;
	this->colorShadow  = subStyle.colorShadow;
	this->fontName     = subStyle.fontName;
	this->fontScale    = subStyle.fontScale;
	this->fontSize     = subStyle.fontSize;
	this->fontStyle    = subStyle.fontStyle;
	this->marginL      = subStyle.marginL;
	this->marginR      = subStyle.marginR;
	this->marginV      = subStyle.marginV;
	this->name         = subStyle.name;
	this->outline      = subStyle.outline;
	this->rotation     = subStyle.rotation;
	this->shadow       = subStyle.shadow;
}

TTF_Font* MediaPlayer::LVP_SubStyle::getFont(LVP_SubtitleContext &subContext)
{
	if (this->fontName.empty() || (this->fontSize < 1))
		return NULL;

	auto fontSize = this->getFontSizeScaled(subContext.scale.y);

	#if defined _windows
		auto fontName = System::LVP_Text::FormatW(L"%s_%d", this->fontName.c_str(), fontSize);
	#else
		auto fontName = System::LVP_Text::Format("%s_%d", this->fontName.c_str(), fontSize);
	#endif

	if (!subContext.styleFonts.contains(fontName))
		subContext.styleFonts[fontName] = this->openFontInternal(subContext, fontSize);

	auto font = subContext.styleFonts[fontName];

	if (font != NULL)
		TTF_SetFontStyle(font, this->fontStyle);

	return font;
}

int MediaPlayer::LVP_SubStyle::getFontSizeScaled(double scale) const
{
	if ((this->name == "Default") && (this->fontName == FONT_ARIAL) && (this->fontSize == DEFAULT_FONT_SIZE))
		return (int)(18.0 * scale);

	return (int)((double)this->fontSize * scale * FONT_DPI_SCALE);
}

int MediaPlayer::LVP_SubStyle::GetOffsetX(Graphics::LVP_SubTexture* subTexture, LVP_SubtitleContext &subContext)
{
	if ((subTexture == NULL) ||
		(subTexture->subtitle->style == NULL) ||
		!(subTexture->subtitle->style->fontStyle & TTF_STYLE_ITALIC))
	{
		return 0;
	}

	auto font = subTexture->subtitle->getFont(subContext);

	if (font == NULL)
		return 0;

	TTF_SetFontStyle(font, subTexture->subtitle->style->fontStyle);

	for (int i = 0; i < DEFAULT_CHAR_BUFFER_SIZE; i++)
	{
		if ((i < DEFAULT_CHAR_BUFFER_SIZE - 1) && (subTexture->textUTF16[i + 1] != 0))
			continue;

		// Return the x-offset of the last italic character
		int minx, maxx, miny, maxy, advance;
		TTF_GlyphMetrics(font, subTexture->textUTF16[i], &minx, &maxx, &miny, &maxy, &advance);

		return (advance - maxx);
	}

	return 0;
}

MediaPlayer::LVP_SubStyle* MediaPlayer::LVP_SubStyle::GetStyle(const std::string &name, const LVP_SubStyles &subStyles)
{
	for (auto style : subStyles) {
		if (System::LVP_Text::ToLower(style->name) == System::LVP_Text::ToLower(name))
			return style;
	}

	return NULL;
}

bool MediaPlayer::LVP_SubStyle::IsAlignedBottom(LVP_SubAlignment a)
{
	return ((a == SUB_ALIGN_BOTTOM_LEFT) || (a == SUB_ALIGN_BOTTOM_RIGHT) || (a == SUB_ALIGN_BOTTOM_CENTER));
}

bool MediaPlayer::LVP_SubStyle::IsAlignedCenter(LVP_SubAlignment a)
{
	return ((a == SUB_ALIGN_BOTTOM_CENTER) || (a == SUB_ALIGN_TOP_CENTER) || (a == SUB_ALIGN_MIDDLE_CENTER));
}

bool MediaPlayer::LVP_SubStyle::IsAlignedLeft(LVP_SubAlignment a)
{
	return ((a == SUB_ALIGN_BOTTOM_LEFT) || (a == SUB_ALIGN_TOP_LEFT) || (a == SUB_ALIGN_MIDDLE_LEFT));
}

bool MediaPlayer::LVP_SubStyle::IsAlignedMiddle(LVP_SubAlignment a)
{
	return ((a == SUB_ALIGN_MIDDLE_LEFT) || (a == SUB_ALIGN_MIDDLE_RIGHT) || (a == SUB_ALIGN_MIDDLE_CENTER));
}

bool MediaPlayer::LVP_SubStyle::IsAlignedRight(LVP_SubAlignment a)
{
	return ((a == SUB_ALIGN_BOTTOM_RIGHT) || (a == SUB_ALIGN_TOP_RIGHT) || (a == SUB_ALIGN_MIDDLE_RIGHT));
}

bool MediaPlayer::LVP_SubStyle::IsAlignedTop(LVP_SubAlignment a)
{
	return ((a == SUB_ALIGN_TOP_LEFT) || (a == SUB_ALIGN_TOP_RIGHT) || (a == SUB_ALIGN_TOP_CENTER));
}

TTF_Font* MediaPlayer::LVP_SubStyle::openFontArial(int fontSize)
{
	if (fontSize < 1)
		return NULL;

	TTF_Font* font = NULL;

	#if defined _android
		font = OPEN_FONT("/system/fonts/DroidSans.ttf", fontSize);
	#elif defined _ios
		font = OPEN_FONT("/System/Library/Fonts/Cache/arialuni.ttf", fontSize);
	#elif defined _linux
		font = OPEN_FONT("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", fontSize);
	#elif defined  _macosx
		font = OPEN_FONT("/System/Library/Fonts/Supplemental/Arial Unicode.ttf", fontSize);
	#elif defined _windows
		font = OPEN_FONT(L"C:\\Windows\\Fonts\\ARIALUNI.TTF", fontSize);
	#endif

	return font;
}

TTF_Font* MediaPlayer::LVP_SubStyle::openFontInternal(const LVP_SubtitleContext &subContext, int fontSize)
{
	if (this->fontName.empty() || (fontSize < 1) || (subContext.formatContext == NULL))
		return NULL;

	LibFFmpeg::AVStream* stream = NULL;

	for (const auto &fontFace : subContext.fontFaces)
	{
		auto fontNameLower = System::LVP_Text::ToLower(this->fontName);
		auto fontFaceLower = System::LVP_Text::ToLower(fontFace.first);

		// "arial" => "arial", "arial std", "arial bold" etc.
		if ((fontNameLower != fontFaceLower) && (fontNameLower.find(fontFaceLower) == std::string::npos))
			continue;

		// Match font style
		for (const auto &style : fontFace.second)
		{
			bool hasBold    = (this->fontStyle & TTF_STYLE_BOLD);
			bool hasItalic  = (this->fontStyle & TTF_STYLE_ITALIC);
			bool hasRegular = (this->fontStyle == TTF_STYLE_NORMAL);

			bool isBoldItalic = (hasBold && hasItalic && style.style == "bold italic");
			bool isBold       = (hasBold    && style.style == "bold");
			bool isItalic     = (hasItalic  && style.style == "italic");
			bool isRegular    = (hasRegular && style.style == "regular");

			if (isBoldItalic || isBold || isItalic || isRegular) {
				stream = subContext.formatContext->streams[style.streamIndex];
				break;
			}
		}

		// Select the first style font as default
		if ((stream == NULL) && !fontFace.second.empty()) {
			stream = subContext.formatContext->streams[fontFace.second[0].streamIndex];
			break;
		}
	}
	
	if (stream != NULL)
	{
		auto fontRW = SDL_RWFromConstMem(stream->codecpar->extradata, stream->codecpar->extradata_size);

		if (fontRW != NULL)
			return TTF_OpenFontRW(fontRW, 0, fontSize);
	}

	return this->openFontArial(fontSize);
}

MediaPlayer::LVP_SubAlignment MediaPlayer::LVP_SubStyle::ToSubAlignment(int alignment)
{
	switch (alignment) {
		case 1:  return SUB_ALIGN_BOTTOM_LEFT;
		case 2:  return SUB_ALIGN_BOTTOM_CENTER;
		case 3:  return SUB_ALIGN_BOTTOM_RIGHT;
		case 5:  return SUB_ALIGN_TOP_LEFT;
		case 6:  return SUB_ALIGN_TOP_CENTER;
		case 7:  return SUB_ALIGN_TOP_RIGHT;
		case 9:  return SUB_ALIGN_MIDDLE_LEFT;
		case 10: return SUB_ALIGN_MIDDLE_CENTER;
		case 11: return SUB_ALIGN_MIDDLE_RIGHT;
		default: return SUB_ALIGN_UNKNOWN;
	}
}
