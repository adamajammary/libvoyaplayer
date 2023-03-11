#include "LVP_SubTextRenderer.h"

Graphics::LVP_Color         MediaPlayer::LVP_SubTextRenderer::drawColor = LVP_COLOR_BLACK;
Graphics::LVP_SubTexturesId MediaPlayer::LVP_SubTextRenderer::subsBottom;
Graphics::LVP_SubTexturesId MediaPlayer::LVP_SubTextRenderer::subsMiddle;
Graphics::LVP_SubTexturesId MediaPlayer::LVP_SubTextRenderer::subsPosition;
Graphics::LVP_SubTexturesId MediaPlayer::LVP_SubTextRenderer::subsTop;

Graphics::LVP_SubTexture* MediaPlayer::LVP_SubTextRenderer::createSubFill(uint16_t* subString16, LVP_Subtitle* sub, SDL_Renderer* renderer, LVP_SubtitleContext &subContext)
{
	if (sub == NULL)
		return NULL;

	auto font = sub->getFont(subContext);

	if (font == NULL)
		return NULL;

	// Disable font outline - Primary/Fill sub
	TTF_SetFontOutline(font, 0);

	if (sub->style != NULL)
		TTF_SetFontStyle(font, sub->style->fontStyle);

	if (subString16 == NULL)
		return NULL;

	SDL_Surface* surface = NULL;
	auto         subFill = new Graphics::LVP_SubTexture(sub);
	
	subFill->textUTF16 = subString16;

	if (subFill->textUTF16[0] != 0)
	{
		bool isBorderStyleBox = ((sub->style != NULL) && (sub->style->borderStyle == SUB_BORDER_STYLE_BOX));

		if (isBorderStyleBox)
			surface = TTF_RenderUNICODE_Shaded(font, subFill->textUTF16, sub->getColor(), sub->style->colorShadow);
		else
			surface = TTF_RenderUNICODE_Blended(font, subFill->textUTF16, sub->getColor());

		if (surface == NULL) {
			DELETE_POINTER(subFill);
			return NULL;
		}

		auto blur    = sub->getBlur();
		auto outline = sub->getOutline(subContext.scale);

		if (!isBorderStyleBox && (outline < 1) && (blur > 0))
			Graphics::LVP_Graphics::Blur(surface, blur);

		subFill->textureData = new Graphics::LVP_Texture(surface, renderer);
		FREE_SURFACE(surface);

		if (subFill->textureData->data == NULL) {
			DELETE_POINTER(subFill);
			return NULL;
		}

		// Scale texture in X/Y dimensions
		if (sub->style != NULL)
		{
			if (sub->style->fontScale.x > MIN_FLOAT_ZERO)
				subFill->textureData->width = (int)((float)subFill->textureData->width * sub->style->fontScale.x);

			if (sub->style->fontScale.y > MIN_FLOAT_ZERO)
				subFill->textureData->height = (int)((float)subFill->textureData->height * sub->style->fontScale.y);
		}
	}

	return subFill;
}

Graphics::LVP_SubTexture* MediaPlayer::LVP_SubTextRenderer::createSubOutline(Graphics::LVP_SubTexture* subFill, SDL_Renderer* renderer, LVP_SubtitleContext &subContext)
{
	if ((subFill == NULL) || (subFill->subtitle == NULL) || (subFill->textureData == NULL))
		return NULL;

	auto font = subFill->subtitle->getFont(subContext);

	if (font == NULL)
		return NULL;

	TTF_SetFontOutline(font, subFill->subtitle->getOutline(subContext.scale));

	if (subFill->subtitle->style != NULL)
		TTF_SetFontStyle(font, subFill->subtitle->style->fontStyle);

	SDL_Surface* surface = TTF_RenderUNICODE_Blended(font, subFill->textUTF16, subFill->subtitle->getColorOutline());

	if (surface == NULL)
		return NULL;

	int blur = subFill->subtitle->getBlur();

	if (blur > 0)
		Graphics::LVP_Graphics::Blur(surface, blur);

	auto subOutline = new Graphics::LVP_SubTexture(*subFill);

	subOutline->textureData = new Graphics::LVP_Texture(surface, renderer);

	FREE_SURFACE(surface);

	if (subOutline->textureData->data == NULL)
		return NULL;

	// Scale texture in X/Y dimensions
	if (subFill->subtitle->style != NULL)
	{
		if (subFill->subtitle->style->fontScale.x > MIN_FLOAT_ZERO)
			subOutline->textureData->width = (int)((float)subOutline->textureData->width * subFill->subtitle->style->fontScale.x);

		if (subFill->subtitle->style->fontScale.y > MIN_FLOAT_ZERO)
			subOutline->textureData->height = (int)((float)subOutline->textureData->height * subFill->subtitle->style->fontScale.y);
	}

	int outline = TTF_GetFontOutline(font);

	subOutline->locationRender.x -= outline;
	subOutline->locationRender.y -= outline;
	subOutline->locationRender.w  = subOutline->textureData->width;
	subOutline->locationRender.h  = subOutline->textureData->height;

	return subOutline;
}

Graphics::LVP_SubTexture* MediaPlayer::LVP_SubTextRenderer::createSubShadow(Graphics::LVP_SubTexture* subFill, SDL_Renderer* renderer, LVP_SubtitleContext &subContext)
{
	if ((subFill == NULL) || (subFill->subtitle == NULL) || (subFill->textureData == NULL))
		return NULL;

	auto font = subFill->subtitle->getFont(subContext);

	if (font == NULL)
		return NULL;

	TTF_SetFontOutline(font, subFill->subtitle->getOutline(subContext.scale));

	if (subFill->subtitle->style != NULL)
		TTF_SetFontStyle(font, subFill->subtitle->style->fontStyle);

	SDL_Surface* surface = TTF_RenderUNICODE_Blended(font, subFill->textUTF16, subFill->subtitle->getColorShadow());

	if (surface == NULL)
		return NULL;

	int blur = subFill->subtitle->getBlur();

	if (blur > 0)
		Graphics::LVP_Graphics::Blur(surface, blur);

	auto subShadow = new Graphics::LVP_SubTexture(*subFill);

	subShadow->textureData = new Graphics::LVP_Texture(surface, renderer);

	FREE_SURFACE(surface);

	if (subShadow->textureData->data == NULL)
		return NULL;

	// Scale texture in X/Y dimensions
	if (subFill->subtitle->style != NULL)
	{
		if (subFill->subtitle->style->fontScale.x > MIN_FLOAT_ZERO)
			subShadow->textureData->width = (int)((float)subShadow->textureData->width * subFill->subtitle->style->fontScale.x);

		if (subFill->subtitle->style->fontScale.y > MIN_FLOAT_ZERO)
			subShadow->textureData->height = (int)((float)subShadow->textureData->height * subFill->subtitle->style->fontScale.y);
	}

	int       outline = TTF_GetFontOutline(font);
	SDL_Point shadow  = subFill->subtitle->getShadow(subContext.scale);

	subShadow->locationRender.x += (shadow.x - outline);
	subShadow->locationRender.y += (shadow.y - outline);
	subShadow->locationRender.w  = subShadow->textureData->width;
	subShadow->locationRender.h  = subShadow->textureData->height;

	return subShadow;
}

bool MediaPlayer::LVP_SubTextRenderer::formatAnimationsContain(const Strings &animations, const std::string &string)
{
	for (const auto &animation : animations)
	{
		auto pos = animation.find(string);

		if (pos != std::string::npos)
		{
			if ((string == "\\fr") || (string == "\\fs")) {
				if (isdigit(animation[pos + 4]))
					return true;
			} else {
				return true;
			}
		}
	}

	return false;
}

std::string MediaPlayer::LVP_SubTextRenderer::getSubText(const std::string &dialogueText, size_t nrStyles)
{
	std::string subText = std::string(dialogueText);

	// ",,,,,,,,text" => "text"
	for (int i = 0; i < SUB_DIALOGUE_TEXT; i++)
		subText = subText.substr(subText.find(",") + 1);

	if (subText.empty())
		return subText;

	subText = System::LVP_Text::Replace(subText, "\\N", "^");
	subText = System::LVP_Text::Replace(subText, "\\n", (subText.find("\\q2") != std::string::npos ? "^" : " "));
	subText = System::LVP_Text::Replace(subText, "\\h", " ");
	subText = System::LVP_Text::Replace(subText, "{*", "{");

	if (nrStyles < 2)
		subText = System::LVP_Text::Replace(subText, "{\\r}", "");

	size_t findPos = subText.rfind("\r\n");

	if (findPos != std::string::npos)
		subText = subText.substr(0, findPos);

	// {=43}{\f} => {\f}
	subText = LVP_SubTextRenderer::removeInvalidFormatting(subText);

	// {\f1}{\f2} => {\f1\f2}
	subText = System::LVP_Text::Replace(subText, "}{", "");

	return subText;
}

void MediaPlayer::LVP_SubTextRenderer::formatDrawCommand(
	const std::string &subText,
	const Strings &subSplit,
	int subID,
	LVP_Subtitles &subs,
	const LVP_SubtitleContext &subContext
)
{
	// Custom draw operation (fill rect)
	if (subText.find("\\p1") != std::string::npos)
	{
		LVP_SubStyle* style    = LVP_SubTextRenderer::getSubStyle(subContext.styles, subSplit);
		SDL_Rect      drawRect = LVP_SubTextRenderer::getDrawRect(subText, style);

		if (!SDL_RectEmpty(&drawRect))
		{
			LVP_Subtitle* sub = new LVP_Subtitle();

			sub->id       = subID;
			sub->drawRect = drawRect;
			sub->style    = new LVP_SubStyle(*style);

			subs.push_back(sub);
		}
	}
}

Strings MediaPlayer::LVP_SubTextRenderer::formatGetAnimations(const std::string &subString)
{
	Strings     animations;
	size_t      findPos1, findPos2;
	std::string tempString1, tempString2;

	std::string formattedString = std::string(subString);

	findPos1 = formattedString.find("\\t(");
	findPos2 = formattedString.rfind(")");

	while ((findPos1 != std::string::npos) && (findPos2 != std::string::npos) && (findPos2 > findPos1))
	{
		tempString1     = formattedString.substr(0, findPos1);
		tempString2     = formattedString.substr(findPos1);
		formattedString = tempString1.append(tempString2.substr(tempString2.find(")") + 1));

		animations.push_back(tempString2.substr(0, tempString2.find(")") + 1));

		findPos1 = formattedString.find("\\t(");
		findPos2 = formattedString.rfind(")");
	}

	return animations;
}

// Style Overriders - Category 1 - Affects the entire sub
void MediaPlayer::LVP_SubTextRenderer::formatOverrideStyleCat1(const Strings &animations, LVP_Subtitle* sub, const LVP_SubStyles &subStyles)
{
	Strings styleProps;
	bool    aligned      = false;
	auto    defaultStyle = LVP_SubStyle::GetStyle(sub->style->name, subStyles);
	size_t  findPos      = sub->text3.find("{\\");
	bool    positioned   = false;

	if (findPos != std::string::npos)
		styleProps = System::LVP_Text::Split(sub->text3, "\\");

	for (const auto &prop : styleProps)
	{
		// ALIGNMENT - Reset to original alignment
		if ((prop == "an") || (prop == "a"))
		{
			if (!aligned && (defaultStyle != NULL)) {
				sub->style->alignment = defaultStyle->alignment;
				aligned = true;
			}
		}
		// ALIGNMENT - Numpad
		else if ((prop.substr(0, 2) == "an") && isdigit(prop[2]))
		{
			if (!aligned) {
				sub->style->alignment = (LVP_SubAlignment)std::atoi(prop.substr(2).c_str());
				aligned = true;
			}
		}
		// ALIGNMENT - Legacy
		else if ((prop[0] == 'a') && isdigit(prop[1]))
		{
			if (!aligned) {
				sub->style->alignment = LVP_SubStyle::ToSubAlignment(std::atoi(prop.substr(1).c_str()));
				aligned = true;
			}
		}
		// CLIP
		else if ((prop.substr(0, 5) == "clip("))
		{
			Strings clipProps = System::LVP_Text::Split(prop.substr(5, (prop.size() - 1)), ",");

			if ((clipProps.size() > 3) && !LVP_SubTextRenderer::formatAnimationsContain(animations, "\\clip"))
			{
				sub->clip.x = std::atoi(clipProps[0].c_str());
				sub->clip.y = std::atoi(clipProps[1].c_str());
				sub->clip.w = (std::atoi(clipProps[2].c_str()) - sub->clip.x);
				sub->clip.h = (std::atoi(clipProps[3].c_str()) - sub->clip.y);

				sub->customClip = true;
			}
		}
		// POSITION
		else if (prop.substr(0, 4) == "pos(")
		{
			if (!positioned)
			{
				sub->position.x = std::atoi(prop.substr(4).c_str());
				sub->position.y = std::atoi(prop.substr(prop.find(",") + 1).c_str());

				sub->customPos = true;
				positioned     = true;
			}
		}
		// MOVE - Position
		else if (prop.substr(0, 5) == "move(")
		{
			Strings moveProps = System::LVP_Text::Split(prop.substr(prop.find("(") + 1), ",");
				
			if (moveProps.size() > 3)
			{
				sub->position.x = std::atoi(moveProps[2].c_str());
				sub->position.y = std::atoi(moveProps[3].c_str());

				sub->customPos = true;
			}
		}
		// ROTATION POINT - Origin
		else if (prop.substr(0, 4) == "org(")
		{
			sub->rotationPoint = {
				std::atoi(prop.substr(4).c_str()),
				std::atoi(prop.substr(prop.find(",") + 1).c_str())
			};

			sub->customRotation = true;
		}
	}
}

// Style Overriders - Category 2 - Affects only preceding text
void MediaPlayer::LVP_SubTextRenderer::formatOverrideStyleCat2(const Strings &animations, LVP_Subtitle* sub, const LVP_SubStyles &subStyles)
{
	Strings styleProps;
	auto    defaultStyle = LVP_SubStyle::GetStyle(sub->style->name, subStyles);
	size_t  findPos      = sub->text2.find("{\\");

	if (findPos == 0)
		styleProps = System::LVP_Text::Split(sub->text2.substr((findPos + 2), (sub->text2.find("}") - (findPos + 2))), "\\");

	for (const auto &prop : styleProps)
	{
		// COLOR - Reset to original color
		if (prop == "c")
		{
			if (defaultStyle != NULL)
				sub->style->colorPrimary = defaultStyle->colorPrimary;
		}
		// COLOR - Primary Fill
		if (prop.substr(0, 3) == "c&H")
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\c&H")) {
				auto color               = Graphics::LVP_Graphics::ToLVPColor(prop.substr(1, 8));
				sub->style->colorPrimary = { color.r, color.g, color.b, sub->style->colorPrimary.a };
			}
		}
		// COLOR - Primary Fill
		else if (prop.substr(0, 4) == "1c&H")
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\1c&H")) {
				auto color               = Graphics::LVP_Graphics::ToLVPColor(prop.substr(2, 8));
				sub->style->colorPrimary = { color.r, color.g, color.b, sub->style->colorPrimary.a };
			}
		}
		// COLOR - Secondary Fill
		else if (prop.substr(0, 4) == "2c&H")
		{
			//
		}
		// COLOR - Outline Border
		else if (prop.substr(0, 4) == "3c&H")
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\3c&H")) {
				auto color               = Graphics::LVP_Graphics::ToLVPColor(prop.substr(2, 8));
				sub->style->colorOutline = { color.r, color.g, color.b, sub->style->colorOutline.a };
			}
		}
		// COLOR - Shadow
		else if (prop.substr(0, 4) == "4c&H")
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\4c&H")) {
				auto color              = Graphics::LVP_Graphics::ToLVPColor(prop.substr(2, 8));
				sub->style->colorShadow = { color.r, color.g, color.b, sub->style->colorShadow.a };
			}
		}
		// ALPHA
		else if (prop.substr(0, 7) == "alpha&H")
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\alpha&H")) {
				sub->style->colorPrimary.a = (0xFF - HEX_STR_TO_UINT(prop.substr(7, 2)));
				sub->style->colorOutline.a = sub->style->colorPrimary.a;
				sub->style->colorShadow.a  = sub->style->colorPrimary.a;
			}
		}
		else if (prop.substr(0, 4) == "1a&H")
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\1a&H"))
				sub->style->colorPrimary.a = (0xFF - HEX_STR_TO_UINT(prop.substr(4, 2)));
		}
		// ALPHA - Secondary Fill
		else if (prop.substr(0, 4) == "2a&H")
		{
			//
		}
		// ALPHA - Outline Border
		else if (prop.substr(0, 4) == "3a&H")
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\3a&H"))
				sub->style->colorOutline.a = (0xFF - HEX_STR_TO_UINT(prop.substr(4, 2)));
		}
		// ALPHA - Shadow
		else if (prop.substr(0, 4) == "4a&H")
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\4a&H"))
				sub->style->colorShadow.a = (0xFF - HEX_STR_TO_UINT(prop.substr(4, 2)));
		}
		// OUTLINE BORDER WIDTH
		else if ((prop.substr(0, 4) == "bord") && isdigit(prop[4]))
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\bord"))
				sub->style->outline = (int)std::round(std::atof(prop.substr(4).c_str()));
		}
		// SHADOW DEPTH
		else if ((prop.substr(0, 4) == "shad") && (isdigit(prop[4]) || (prop[4] == '-')))
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\shad")) {
				int shadow         = (int)std::round(std::atof(prop.substr(4).c_str()));
				sub->style->shadow = { shadow, shadow };
			}
		}
		else if ((prop.substr(0, 5) == "xshad") && (isdigit(prop[5]) || (prop[5] == '-')))
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\xshad"))
				sub->style->shadow.x = (int)std::round(std::atof(prop.substr(5).c_str()));
		}
		else if ((prop.substr(0, 5) == "yshad") && (isdigit(prop[5]) || (prop[5] == '-')))
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\yshad"))
				sub->style->shadow.y = (int)std::round(std::atof(prop.substr(5).c_str()));
		}
		// BLUR EFFECT
		else if ((prop.substr(0, 4) == "blur") && isdigit(prop[4]))
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\blur"))
				sub->style->blur = (int)std::round(std::atof(prop.substr(4).c_str()));
		}
		// FONT - Name
		else if (prop.substr(0, 2) == "fn")
		{
			std::string font = prop.substr(2);

			if (!font.empty() && (font[font.size() - 1] == '}'))
				font = font.substr(0, font.size() - 1);

			#if defined _windows
				auto font16          = (wchar_t*)SDL_iconv_utf8_ucs2(font.c_str());
				sub->style->fontName = std::wstring(font16);

				SDL_free(font16);
			#else
				sub->style->fontName = font;
			#endif
		}
		// FONT - Scale X
		else if ((prop.substr(0, 4) == "fscx") && isdigit(prop[4]))
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\fscx"))
				sub->style->fontScale.x = (float)(std::atof(prop.substr(4).c_str()) * 0.01);
		}
		// FONT - Scale Y
		else if ((prop.substr(0, 4) == "fscy") && isdigit(prop[4]))
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\fscy"))
				sub->style->fontScale.y = (float)(std::atof(prop.substr(4).c_str()) * 0.01);
		}
		// FONT - Letter Spacing
		else if ((prop.substr(0, 3) == "fsp") && isdigit(prop[3]))
		{
			//
		}
		// FONT - Size
		else if ((prop.substr(0, 2) == "fs") && isdigit(prop[2]))
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\fs"))
			{
				int fontSize = std::atoi(prop.substr(2).c_str());

				if ((fontSize > 0) && (fontSize < MAX_FONT_SIZE))
					sub->style->fontSize = fontSize;
			}
		}
		// FONT STYLING - Bold
		else if (prop == "b1")
		{
			sub->style->fontStyle |= TTF_STYLE_BOLD;
		}
		// FONT STYLING - Reset Bold
		else if (prop == "b0")
		{
			sub->style->fontStyle &= ~TTF_STYLE_BOLD;
		}
		// FONT STYLING - Italic
		else if (prop == "i1")
		{
			sub->style->fontStyle |= TTF_STYLE_ITALIC;
		}
		// FONT STYLING - Reset Italic
		else if (prop == "i0")
		{
			sub->style->fontStyle &= ~TTF_STYLE_ITALIC;
		}
		// FONT STYLING - Strikeout
		else if (prop == "s1")
		{
			sub->style->fontStyle |= TTF_STYLE_STRIKETHROUGH;
		}
		// FONT STYLING - Reset Strikeout
		else if (prop == "s0")
		{
			sub->style->fontStyle &= ~TTF_STYLE_STRIKETHROUGH;
		}
		// FONT STYLING - Underline
		else if (prop == "u1")
		{
			sub->style->fontStyle |= TTF_STYLE_UNDERLINE;
		}
		// FONT STYLING - Reset Underline
		else if (prop == "u0")
		{
			sub->style->fontStyle &= ~TTF_STYLE_UNDERLINE;
		}
		// ROTATION - Degrees
		else if ((prop.substr(0, 3) == "frx") && (isdigit(prop[3]) || (prop[3] == '-')))
		{
			//
		}
		else if ((prop.substr(0, 3) == "fry") && (isdigit(prop[3]) || (prop[3] == '-')))
		{
			//
		}
		else if ((prop.substr(0, 3) == "frz") && (isdigit(prop[3]) || (prop[3] == '-')))
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\frz"))
				sub->rotation = (360.0 - (double)(std::atoi(prop.substr(3).c_str()) % 360));
		}
		else if ((prop.substr(0, 2) == "fr") && (isdigit(prop[2]) || (prop[2] == '-')))
		{
			if (!LVP_SubTextRenderer::formatAnimationsContain(animations, "\\fr"))
				sub->rotation = (360.0 - (double)(std::atoi(prop.substr(2).c_str()) % 360));
		}
		// RESET STYLE
		else if (prop[0] == 'r')
		{
			int           fontStyle = sub->style->fontStyle;
			LVP_SubStyle* style     = NULL;

			// Reset to default style: {\r}
			if (prop == "r")
				style = defaultStyle;
			// Reset to specified style: {\rStyleName}
			else
				style = LVP_SubStyle::GetStyle(prop.substr(1), subStyles);

			if (style != NULL)
			{
				DELETE_POINTER(sub->style);

				sub->style = new LVP_SubStyle(*style);
				sub->style->fontStyle = fontStyle;
			}
		}
	}
}

std::string MediaPlayer::LVP_SubTextRenderer::formatRemoveAnimations(const std::string &subString)
{
	size_t      findPos1, findPos2;
	std::string tempString1, tempString2;

	std::string formattedString = std::string(subString);

	findPos1 = formattedString.find("\\t(");
	findPos2 = formattedString.rfind(")");

	while ((findPos1 != std::string::npos) && (findPos2 != std::string::npos) && (findPos2 > findPos1))
	{
		tempString1 = formattedString.substr(0, findPos1);
		tempString2 = formattedString.substr(findPos1);
		tempString2 = tempString2.substr(tempString2.find(")") + 1);

		formattedString = tempString1.append(tempString2);

		findPos1 = formattedString.find("\\t(");
		findPos2 = formattedString.rfind(")");
	}

	return formattedString;
}

Strings MediaPlayer::LVP_SubTextRenderer::formatSplitStyling(const std::string &subText, LVP_SubStyle* subStyle, LVP_SubtitleContext &subContext)
{
	Strings subLines1;

	// Split the sub string by partial formatting ({\f1}t1{\f2}t2)
	if ((subText.find("{") != subText.rfind("{")) && System::LVP_Text::IsValidSubtitle(subText))
	{
		Strings subLines2 = System::LVP_Text::Split(subText, "^", true);

		for (const auto &subLine2 : subLines2)
		{
			Strings subLines3 = System::LVP_Text::Split(subLine2, "{", false);

			if (subLine2.empty())
				subLines1.push_back("^");

			for (auto &subLine3 : subLines3)
			{
				auto subLine4 = std::string(subLine3);
				auto findPos  = subLine3.rfind("}");

				if (findPos != std::string::npos) {
					subLine4 = subLine3.substr(findPos + 1);
					subLine3 = ("{" + subLine3);
				}

				if (subLine4 != "\r\n")
					subLines1.push_back(subLine3);
			}

			if (!subLines1.empty() && !subLines3.empty())
				subLines1[subLines1.size() - 1].append("^");
		}
	}
	// Split the sub string by newlines (\\N => ^)
	else
	{
		Strings subLines2 = System::LVP_Text::Split(subText, "^");

		for (auto &subLine2 : subLines2)
			subLines1.push_back(subLine2.append("^"));
	}

	TTF_Font* font = NULL;

	if ((subStyle != NULL) && !subStyle->fontName.empty())
		font = subStyle->getFont(subContext);

	Strings subLines;

	for (std::string subLine1 : subLines1)
		subLines.push_back(subLine1);

	return subLines;
}

SDL_Rect MediaPlayer::LVP_SubTextRenderer::getDrawRect(const std::string &subLine, LVP_SubStyle* style)
{
	SDL_Rect    drawRect     = {};
	std::string drawLine     = LVP_SubTextRenderer::RemoveFormatting(subLine);
	Strings     drawCommands = System::LVP_Text::Split(drawLine, " ");

	// m  1494  212    l  1494 398     383   411       374 209
	// m  160.5 880.5  l  282  879     283.5 610.5     456 606  459 133.5  157.5 133.5
	// m  1494  212    l  1494 398  l  383   411    l  374 209
	if ((drawCommands.size() < 10) || (drawCommands[0] != "m") || (drawCommands[3] != "l"))
		return drawRect;

	SDL_Point startPosition   = {};
	SDL_Point minDrawPosition = { std::atoi(drawCommands[1].c_str()), std::atoi(drawCommands[2].c_str()) };
	SDL_Point maxDrawPosition = SDL_Point(minDrawPosition);

	for (size_t i = 4; i < drawCommands.size(); i += 2)
	{
		SDL_Point p = { std::atoi(drawCommands[i].c_str()), std::atoi(drawCommands[i + 1].c_str()) };

		if (p.x < minDrawPosition.x)
			minDrawPosition.x = p.x;
		if (p.y < minDrawPosition.y)
			minDrawPosition.y = p.y;

		if (p.x > maxDrawPosition.x)
			maxDrawPosition.x = p.x;
		if (p.y > maxDrawPosition.y)
			maxDrawPosition.y = p.y;

		if ((i + 2 < drawCommands.size()) && !drawCommands[i + 2].empty()) {
			if (drawCommands[i + 2] == "l")
				i++;
			else if (!isdigit(drawCommands[i + 2][0]))
				return drawRect;
		}
	}

	LVP_SubAlignment alignment  = (style != NULL ? style->alignment : SUB_ALIGN_TOP_LEFT);
	SDL_FPoint       fontScale  = (style != NULL ? style->fontScale : SDL_FPoint { 1.0f, 1.0f });
	Strings          styleProps = System::LVP_Text::Split(subLine, "\\");

	// {\pos(0,0)\an7\1c&HD9F0F5&\p1}m 1494 212 l 1494 398 383 411 374 209{\p0}
	for (const auto &prop : styleProps)
	{
		if (prop.substr(0, 7) == "alpha&H") {
			LVP_SubTextRenderer::drawColor.a = (0xFF - HEX_STR_TO_UINT(prop.substr(7, 2)));
		} else if (prop.substr(0, 4) == "1a&H") {
			LVP_SubTextRenderer::drawColor.a = (0xFF - HEX_STR_TO_UINT(prop.substr(4, 2)));
		} else if (prop.substr(0, 4) == "3a&H") {
			style->colorOutline.a = (0xFF - HEX_STR_TO_UINT(prop.substr(4, 2)));
		} else if (prop.substr(0, 3) == "c&H") {
			auto color = Graphics::LVP_Graphics::ToLVPColor(prop.substr(1, 8));

			LVP_SubTextRenderer::drawColor = { color.r, color.g, color.b, LVP_SubTextRenderer::drawColor.a };
		} else if (prop.substr(0, 4) == "1c&H") {
			auto color = Graphics::LVP_Graphics::ToLVPColor(prop.substr(2, 8));

			LVP_SubTextRenderer::drawColor = { color.r, color.g, color.b, LVP_SubTextRenderer::drawColor.a };
		} else if (prop.substr(0, 4) == "3c&H") {
			auto color = Graphics::LVP_Graphics::ToLVPColor(prop.substr(2, 8));

			style->colorOutline = { color.r, color.g, color.b, style->colorOutline.a };
		} else if ((prop.substr(0, 4) == "bord") && isdigit(prop[4])) {
			style->outline = (int)std::round(std::atof(prop.substr(4).c_str()));
		} else if (prop.substr(0, 2) == "an") {
			alignment = (LVP_SubAlignment)std::atoi(prop.substr(2).c_str());
		} else if ((prop[0] == 'a') && isdigit(prop[1])) {
			alignment = LVP_SubStyle::ToSubAlignment(std::atoi(prop.substr(1).c_str()));
		} else if (prop.substr(0, 4) == "fscx") {
			fontScale.x = (float)(std::atof(prop.substr(4).c_str()) * 0.01);
		} else if (prop.substr(0, 4) == "fscy") {
			fontScale.y = (float)(std::atof(prop.substr(4).c_str()) * 0.01);
		} else if (prop.substr(0, 4) == "pos(") {
			startPosition.x = std::atoi(prop.substr(4).c_str());
			startPosition.y = std::atoi(prop.substr(prop.find(",") + 1).c_str());
		}
	}

	if ((startPosition.x != 0) && (startPosition.y != 0))
	{
		drawRect.x = (startPosition.x + minDrawPosition.x);
		drawRect.y = (startPosition.y + minDrawPosition.y);
		drawRect.w = (max(startPosition.x + maxDrawPosition.x, drawRect.x) - drawRect.x);
		drawRect.h = (max(startPosition.y + maxDrawPosition.y, drawRect.y) - drawRect.y);
	}
	else
	{
		drawRect.x = minDrawPosition.x;
		drawRect.y = minDrawPosition.y;
		drawRect.w = (max(maxDrawPosition.x, drawRect.x) - drawRect.x);
		drawRect.h = (max(maxDrawPosition.y, drawRect.y) - drawRect.y);
	}

	if (fontScale.x > MIN_FLOAT_ZERO)
		drawRect.w = (int)((float)drawRect.w * fontScale.x);

	if (fontScale.y > MIN_FLOAT_ZERO)
		drawRect.h = (int)((float)drawRect.h * fontScale.y);

	if (LVP_SubStyle::IsAlignedRight(alignment))
		drawRect.x -= drawRect.w;
	else if (LVP_SubStyle::IsAlignedCenter(alignment))
		drawRect.x -= (drawRect.w / 2);

	if (LVP_SubStyle::IsAlignedBottom(alignment))
		drawRect.y -= drawRect.h;
	else if (LVP_SubStyle::IsAlignedMiddle(alignment))
		drawRect.y -= (drawRect.h / 2);

	return drawRect;
}

MediaPlayer::LVP_SubStyle* MediaPlayer::LVP_SubTextRenderer::getSubStyle(const LVP_SubStyles &subStyles, const Strings &dialogueSplit)
{
	if (dialogueSplit.size() < SUB_DIALOGUE_STYLE)
		return NULL;

	for (auto style : subStyles) {
		if (System::LVP_Text::ToLower(style->name) == System::LVP_Text::ToLower(dialogueSplit[SUB_DIALOGUE_STYLE]))
			return style;
	}

	return (!subStyles.empty() ? subStyles[0] : NULL);
}

void MediaPlayer::LVP_SubTextRenderer::handleSubCollisions(const Graphics::LVP_SubTextureId &subTextures, const Graphics::LVP_SubTexturesId &subs)
{
	const size_t MAX_TRIES = subs.size();

	for (size_t i = 0; i < MAX_TRIES; i++)
	{
		bool collision = false;

		for (const auto &sub : subs)
		{
			if ((subTextures.first == sub.first) || !SDL_HasIntersection(&subTextures.second[0]->total, &sub.second[0]->total))
				continue;

			int offsetY = 0;

			for (auto subTexture : subTextures.second)
			{
				if (subTexture->subtitle->layer >= 0)
					continue;

				if (subTexture->subtitle->isAlignedTop())
					subTexture->locationRender.y = (sub.second[0]->total.y + sub.second[0]->total.h + offsetY);
				else
					subTexture->locationRender.y = (sub.second[0]->total.y - subTexture->total.h + offsetY);

				subTexture->total.y = subTextures.second[0]->locationRender.y;

				offsetY += subTexture->locationRender.h;

				DELETE_POINTER(subTexture->outline);
				DELETE_POINTER(subTexture->shadow);
			}

			collision = true;

			break;
		}

		if (!collision)
			break;
	}
}

void MediaPlayer::LVP_SubTextRenderer::handleSubsOutOfBound(const Graphics::LVP_SubTextureId &subTextures)
{
	if (subTextures.second.empty() || (subTextures.second[0]->subtitle->text3.find("\\q2") != std::string::npos))
		return;

	int  offsetY      = 0;
	bool outOfBoundsY = (subTextures.second[0]->locationRender.y < 0);

	for (auto subTexture : subTextures.second)
	{
		if (subTexture->locationRender.x < 0)
		{
			subTexture->locationRender.x = 0;
			subTexture->total.x          = 0;

			DELETE_POINTER(subTexture->outline);
			DELETE_POINTER(subTexture->shadow);
		}

		if (outOfBoundsY)
		{
			subTexture->locationRender.y = offsetY;
			subTexture->total.y          = 0;

			offsetY += subTexture->locationRender.h;

			DELETE_POINTER(subTexture->outline);
			DELETE_POINTER(subTexture->shadow);
		}
	}
}

std::string MediaPlayer::LVP_SubTextRenderer::RemoveFormatting(const std::string &subString)
{
	std::string newSubString = std::string(subString);

	auto findPos1 = newSubString.find("{");
	auto findPos2 = newSubString.find("}");

	while ((findPos1 != std::string::npos) && (findPos2 != std::string::npos) && (findPos2 > findPos1))
	{
		auto tempString = newSubString.substr(0, findPos1);

		newSubString = tempString.append(newSubString.substr(findPos2 + 1));

		findPos1 = newSubString.find("{");
		findPos2 = newSubString.find("}");
	}

	return newSubString;
}

std::string MediaPlayer::LVP_SubTextRenderer::removeInvalidFormatting(const std::string &subString)
{
	Strings partsResult;
	Strings partsStart = System::LVP_Text::Split(subString, "{");

	for (const auto &start : partsStart)
	{
		auto partsEnd = System::LVP_Text::Split((start.find("}") != std::string::npos ? "{" : "") + start, "}");

		for (const auto &end : partsEnd)
		{
			if (!end.empty())
				partsResult.push_back(end + (end.find("{") != std::string::npos ? "}" : ""));
		}
	}

	std::string newSubString = "";

	for (const auto &part : partsResult) {
		if ((part[0] != '{') || (part.find('\\') != std::string::npos))
			newSubString.append(part);
	}

	return newSubString;
}

void MediaPlayer::LVP_SubTextRenderer::removeSubs(Graphics::LVP_SubTexturesId &subs)
{
	for (auto &subsId : subs) {
		for (auto &sub : subsId.second)
			DELETE_POINTER(sub);
	}

	subs.clear();
}

void MediaPlayer::LVP_SubTextRenderer::RemoveSubs(size_t id)
{
	LVP_SubTextRenderer::removeSubs(LVP_SubTextRenderer::subsBottom,   id);
	LVP_SubTextRenderer::removeSubs(LVP_SubTextRenderer::subsMiddle,   id);
	LVP_SubTextRenderer::removeSubs(LVP_SubTextRenderer::subsTop,      id);
	LVP_SubTextRenderer::removeSubs(LVP_SubTextRenderer::subsPosition, id);
}

void MediaPlayer::LVP_SubTextRenderer::removeSubs(Graphics::LVP_SubTexturesId &subs, size_t id)
{
	if (subs.find(id) != subs.end())
	{
		for (auto &subTexture : subs[id])
			DELETE_POINTER(subTexture);

		subs.erase(id);
	}
}

void MediaPlayer::LVP_SubTextRenderer::RemoveSubsBottom()
{
	for (auto &subsId : LVP_SubTextRenderer::subsBottom) {
		for (auto &sub : subsId.second)
			DELETE_POINTER(sub);
	}

	LVP_SubTextRenderer::subsBottom.clear();
}

void MediaPlayer::LVP_SubTextRenderer::renderSub(Graphics::LVP_SubTexture* subTexture, SDL_Renderer* renderer)
{
	if ((subTexture == NULL) || (subTexture->textureData == NULL) || System::LVP_Text::Trim(subTexture->subtitle->text).empty())
		return;

	SDL_Rect* clip = NULL;

	if (subTexture->subtitle->customClip)
	{
		clip = &subTexture->subtitle->clip;

		subTexture->locationRender.x += subTexture->subtitle->clip.x;
		subTexture->locationRender.y += subTexture->subtitle->clip.y;
		subTexture->locationRender.w  = subTexture->subtitle->clip.w;
		subTexture->locationRender.h  = subTexture->subtitle->clip.h;
	}

	SDL_RenderCopyEx(
		renderer, subTexture->textureData->data, clip, &subTexture->locationRender,
		subTexture->subtitle->rotation, &subTexture->subtitle->rotationPoint, SDL_FLIP_NONE
	);
}

void MediaPlayer::LVP_SubTextRenderer::renderSubBorderShadow(Graphics::LVP_SubTexture* subTexture, SDL_Renderer* renderer, LVP_SubtitleContext &subContext)
{
	if ((subTexture == NULL) || (subTexture->textureData == NULL))
		return;

	// SHADOW DROP
	SDL_Point shadow = subTexture->subtitle->getShadow(subContext.scale);

	if (((shadow.x != 0) || (shadow.y != 0)) && (subTexture->subtitle->getColorShadow().a > 0))
	{
		if (subTexture->shadow == NULL)
			subTexture->shadow = LVP_SubTextRenderer::createSubShadow(subTexture, renderer, subContext);

		LVP_SubTextRenderer::renderSub(subTexture->shadow, renderer);
	}

	// BORDER OUTLINE
	if ((subTexture->subtitle->getOutline(subContext.scale) > 0) && (subTexture->subtitle->getColorOutline().a > 0))
	{
		if (subTexture->outline == NULL)
			subTexture->outline = LVP_SubTextRenderer::createSubOutline(subTexture, renderer, subContext);

		LVP_SubTextRenderer::renderSub(subTexture->outline, renderer);
	}
}

void MediaPlayer::LVP_SubTextRenderer::renderSubs(Graphics::LVP_SubTexturesId &subs, SDL_Renderer* renderer, LVP_SubtitleContext &subContext)
{
	std::list<Graphics::LVP_SubTexture*> layeredSubs;

	// Find layered subs
	for (const auto &subTextures : subs)
	{
		for (auto subTexture : subTextures.second)
			layeredSubs.push_back(subTexture);
	}

	// Sort layered subs by render order
	layeredSubs.sort([](Graphics::LVP_SubTexture* t1, Graphics::LVP_SubTexture* t2) {
		return t1->subtitle->layer < t2->subtitle->layer;
	});
	
	// Render border/shadow subs
	for (auto subTexture : layeredSubs)
		LVP_SubTextRenderer::renderSubBorderShadow(subTexture, renderer, subContext);

	// Render Fill subs
	for (auto subTexture : layeredSubs)
	{
		if (subTexture->subtitle->getColor().a > 0)
			LVP_SubTextRenderer::renderSub(subTexture, renderer);
	}
}

void MediaPlayer::LVP_SubTextRenderer::Render(SDL_Renderer* renderer, LVP_SubtitleContext &subContext)
{
	for (auto sub : subContext.subs)
	{
		if ((LVP_SubTextRenderer::subsPosition.find(sub->id) != LVP_SubTextRenderer::subsPosition.end()) ||
			(LVP_SubTextRenderer::subsTop.find(sub->id)      != LVP_SubTextRenderer::subsTop.end()) ||
			(LVP_SubTextRenderer::subsMiddle.find(sub->id)   != LVP_SubTextRenderer::subsMiddle.end()) ||
			(LVP_SubTextRenderer::subsBottom.find(sub->id)   != LVP_SubTextRenderer::subsBottom.end()) ||
			((sub->subRect != NULL) && !IS_SUB_TEXT(sub->subRect->type)))
		{
			sub->skip = true;
		}
	}

	int subWidth      = 0;
	int subWidthSplit = 0;

	for (auto sub : subContext.subs)
	{
		if (sub->skip)
			continue;

		// CUSTOM DRAW OPERATION - Fill rect
		if (!SDL_RectEmpty(&sub->drawRect))
		{
			SDL_Rect drawRectScaled = {};

			drawRectScaled.x = (int)ceil((float)sub->drawRect.x * subContext.scale.x);
			drawRectScaled.y = (int)ceil((float)sub->drawRect.y * subContext.scale.y);
			drawRectScaled.w = (int)ceil((float)sub->drawRect.w * subContext.scale.x);
			drawRectScaled.h = (int)ceil((float)sub->drawRect.h * subContext.scale.y);

			Graphics::LVP_Graphics::FillArea(LVP_SubTextRenderer::drawColor, drawRectScaled, renderer);

			if (sub->style && (sub->style->outline > 0))
			{
				auto border = Graphics::LVP_Border(sub->getOutline(subContext.scale));

				Graphics::LVP_Graphics::FillBorder(sub->getColorOutline(), drawRectScaled, border, renderer);
			}

			continue;
		}

		auto font = sub->getFont(subContext);

		if (font == NULL)
			continue;

		TTF_SetFontOutline(font, sub->getOutline(subContext.scale));

		int       subStringWidth, h;
		Strings16 subStrings16;

		auto margins  = sub->getMargins({ 1.0f, 1.0f });
		auto maxWidth = (subContext.videoDimensions.w - margins.x);

		// Split the sub if larger than video width
		if (sub->style != NULL)
		{
			TTF_SetFontStyle(font, sub->style->fontStyle);
			TTF_SizeUNICODE(font, sub->textUTF16, &subStringWidth, &h);

			subWidth    += subStringWidth;
			subStrings16 = LVP_SubTextRenderer::splitSub(sub->textUTF16, subStringWidth, sub, font, subWidth, maxWidth);
		} else {
			subStrings16.push_back(sub->textUTF16);
		}

		// Create a new sub surface/texture for each sub line (if the sub was split)
		for (size_t i = 0; i < subStrings16.size(); i++)
		{
			auto subFill = LVP_SubTextRenderer::createSubFill(subStrings16[i], sub, renderer, subContext);

			if (subFill == NULL)
				continue;

			bool offsetY = false;

			if (!System::LVP_Text::EndsWith(sub->text2, '^'))
			{
				TTF_SizeUNICODE(font, subStrings16[i], &subStringWidth, &h);

				int nextWidth = 0;

				if (i + 1 < subStrings16.size())
				{
					TTF_SizeUNICODE(font, subStrings16[i + 1], &nextWidth, &h);

					nextWidth += System::LVP_Text::GetSpaceWidth(font);
				}

				subWidthSplit += subStringWidth;

				if (subWidthSplit + nextWidth > maxWidth)
					offsetY = true;
			} else {
				offsetY = true;
			}

			if (offsetY) {
				subFill->offsetY = true;
				subWidth         = 0;
				subWidthSplit    = 0;
			}

			// Organize subs by alignment for positioning
			if (subFill->subtitle->customPos)
				LVP_SubTextRenderer::subsPosition[subFill->subtitle->id].push_back(subFill);
			else if (subFill->subtitle->isAlignedTop())
				LVP_SubTextRenderer::subsTop[subFill->subtitle->id].push_back(subFill);
			else if (subFill->subtitle->isAlignedMiddle())
				LVP_SubTextRenderer::subsMiddle[subFill->subtitle->id].push_back(subFill);
			else
				LVP_SubTextRenderer::subsBottom[subFill->subtitle->id].push_back(subFill);
		}
	}

	// Calculate and set the total width for split subs
	LVP_SubTextRenderer::setTotalWidth(LVP_SubTextRenderer::subsBottom);
	LVP_SubTextRenderer::setTotalWidth(LVP_SubTextRenderer::subsMiddle);
	LVP_SubTextRenderer::setTotalWidth(LVP_SubTextRenderer::subsTop);
	LVP_SubTextRenderer::setTotalWidth(LVP_SubTextRenderer::subsPosition);

	// Calculate and set the relatively aligned sub positions
	LVP_SubTextRenderer::setSubPositionRelative(LVP_SubTextRenderer::subsBottom, subContext);
	LVP_SubTextRenderer::setSubPositionRelative(LVP_SubTextRenderer::subsMiddle, subContext);
	LVP_SubTextRenderer::setSubPositionRelative(LVP_SubTextRenderer::subsTop,    subContext);

	// Handle sub collisions
	for (const auto &sub : LVP_SubTextRenderer::subsTop)
		LVP_SubTextRenderer::handleSubCollisions(sub, LVP_SubTextRenderer::subsBottom);

	for (const auto &sub : LVP_SubTextRenderer::subsMiddle) {
		LVP_SubTextRenderer::handleSubCollisions(sub, LVP_SubTextRenderer::subsBottom);
		LVP_SubTextRenderer::handleSubCollisions(sub, LVP_SubTextRenderer::subsTop);
	}

	// Calculate and set the absolutely aligned sub positions
	LVP_SubTextRenderer::setSubPositionAbsolute(LVP_SubTextRenderer::subsPosition, subContext);

	// Render the subs
	LVP_SubTextRenderer::renderSubs(LVP_SubTextRenderer::subsPosition, renderer, subContext);
	LVP_SubTextRenderer::renderSubs(LVP_SubTextRenderer::subsMiddle,   renderer, subContext);
	LVP_SubTextRenderer::renderSubs(LVP_SubTextRenderer::subsTop,      renderer, subContext);
	LVP_SubTextRenderer::renderSubs(LVP_SubTextRenderer::subsBottom,   renderer, subContext);
}

void MediaPlayer::LVP_SubTextRenderer::setSubPositionAbsolute(const Graphics::LVP_SubTexturesId &subs, LVP_SubtitleContext &subContext)
{
	for (const auto &subTextures : subs)
	{
		Graphics::LVP_SubTextures subsX;
		Graphics::LVP_SubTexture* prevSub = NULL;

		bool forceNewLine = false;
		int  offsetX      = 0;
		int  offsetY      = 0;
		int  startX       = 0;
		auto subScale     = subContext.scale;
		int  totalHeight  = 0;

		for (auto subTexture : subTextures.second)
		{
			if (subTexture->subtitle->skip)
				continue;

			int rotationPointX = (int)((float)subTexture->subtitle->rotationPoint.x * subScale.x);

			SDL_Point position = {
				(int)((float)subTexture->subtitle->position.x * subScale.x),
				(int)((float)subTexture->subtitle->position.y * subScale.y),
			};

			if (startX == 0)
				startX = position.x;

			// PRIMARY FILL SUB
			subTexture->locationRender.x = position.x;
			subTexture->locationRender.y = position.y;
			subTexture->locationRender.w = (subTexture->textureData != NULL ? subTexture->textureData->width  : 0);
			subTexture->locationRender.h = (subTexture->textureData != NULL ? subTexture->textureData->height : 0);

			// Offset based on previous sub
			if (prevSub != NULL) {
				subTexture->locationRender.x = offsetX;
				subTexture->locationRender.y = offsetY;
			}

			// FORCED BLANK NEW LINES (t1\N\N\Nt2)
			if (subTexture->subtitle->text == " ")
			{
				if (subTexture->subtitle->style != NULL) {
					subTexture->subtitle->style->blur    = 0;
					subTexture->subtitle->style->outline = 0;
					subTexture->subtitle->style->shadow  = {};
				}

				if (subTexture->subtitle->text2.find("\\fs") == std::string::npos)
					subTexture->locationRender.h = (forceNewLine ? subTexture->locationRender.h / 2 : 0);
			} else {
				forceNewLine = true;
			}

			// ROTATION
			if (subTexture->subtitle->customRotation)
				subTexture->subtitle->rotationPoint.x = (rotationPointX - position.x);
			else
				subTexture->subtitle->rotationPoint.x = (subTexture->total.w / 2);

			subTexture->subtitle->rotationPoint.x -= (subTexture->locationRender.x - position.x);

			// CLIP
			if (subTexture->subtitle->customClip)
			{
				int offsetX = 0;
				int offsetY = 0;

				if (subTexture->subtitle->isAlignedRight())
					offsetX = subTexture->locationRender.w;
				else if (subTexture->subtitle->isAlignedCenter())
					offsetX = (subTexture->locationRender.w / 2);

				if (subTexture->subtitle->isAlignedBottom())
					offsetY = subTexture->locationRender.h;
				else if (subTexture->subtitle->isAlignedMiddle())
					offsetY = (subTexture->locationRender.h / 2);

				subTexture->subtitle->clip.x = max((subTexture->subtitle->clip.x - (position.x - offsetX)), 0);
				subTexture->subtitle->clip.y = max((subTexture->subtitle->clip.y - (position.y - offsetY)), 0);
				subTexture->subtitle->clip.w = min(subTexture->subtitle->clip.w, subTexture->locationRender.w);
				subTexture->subtitle->clip.h = min(subTexture->subtitle->clip.h, subTexture->locationRender.h);
			}

			offsetX = (subTexture->locationRender.x + subTexture->locationRender.w + LVP_SubStyle::GetOffsetX(prevSub, subContext));
			offsetY = subTexture->locationRender.y;
			prevSub = subTexture;

			// Add split subs belonging to the same sub line
			subsX.push_back(subTexture);

			if (!subTexture->offsetY)
				continue;

			// HORIZONTAL ALIGN
			int maxY = 0;

			// Offset X by total width including split subs
			for (size_t i = 0; i < subsX.size(); i++)
			{
				if (subTexture->subtitle->isAlignedRight())
					subsX[i]->locationRender.x -= subTexture->total.w;
				else if (subTexture->subtitle->isAlignedCenter())
					subsX[i]->locationRender.x -= (subTexture->total.w / 2);

				subsX[i]->total.x = subsX[0]->locationRender.x;
				subsX[i]->total.w = subTexture->total.w;

				if (subTexture->locationRender.h > maxY)
					maxY = subTexture->locationRender.h;
			}

			offsetX      = startX;
			offsetY     += maxY;
			totalHeight += maxY;

			subsX.clear();
		}

		for (auto subTexture : subTextures.second)
		{
			if (subTexture->subtitle->skip)
				continue;

			int positionY      = (int)((float)subTexture->subtitle->position.y      * subScale.y);
			int rotationPointY = (int)((float)subTexture->subtitle->rotationPoint.y * subScale.y);

			subTexture->total.h = totalHeight;

			// VERTICAL ALIGN
			if (subTexture->subtitle->isAlignedBottom())
				subTexture->locationRender.y -= subTexture->total.h;
			else if (subTexture->subtitle->isAlignedMiddle())
				subTexture->locationRender.y -= (subTexture->total.h / 2);

			if (subTexture->subtitle->customRotation)
				subTexture->subtitle->rotationPoint.y = (rotationPointY - positionY);
			else
				subTexture->subtitle->rotationPoint.y = (subTexture->total.h / 2);

			subTexture->subtitle->rotationPoint.y -= (subTexture->locationRender.y - positionY);
		}

		int minY = subContext.videoDimensions.h;
		int minX = subContext.videoDimensions.w;
		int maxX = 0;

		for (auto subTexture : subTextures.second)
		{
			if (subTexture->subtitle->skip)
				continue;

			if (subTexture->locationRender.y < minY)
				minY = subTexture->locationRender.y;

			if (subTexture->locationRender.x < minX)
				minX = subTexture->locationRender.x;

			if (subTexture->locationRender.x + subTexture->locationRender.w > maxX)
				maxX = (subTexture->locationRender.x + subTexture->locationRender.w);
		}

		for (auto subTexture : subTextures.second)
		{
			if (subTexture->subtitle->skip)
				continue;

			subTexture->total.x = minX;
			subTexture->total.y = minY;
			subTexture->total.w = (maxX - minX);
		}

		LVP_SubTextRenderer::handleSubCollisions(subTextures, LVP_SubTextRenderer::subsBottom);
		LVP_SubTextRenderer::handleSubCollisions(subTextures, LVP_SubTextRenderer::subsMiddle);
		LVP_SubTextRenderer::handleSubCollisions(subTextures, LVP_SubTextRenderer::subsTop);

		LVP_SubTextRenderer::handleSubsOutOfBound(subTextures);
	}
}

void MediaPlayer::LVP_SubTextRenderer::setSubPositionRelative(const Graphics::LVP_SubTexturesId &subs, LVP_SubtitleContext &subContext)
{
	for (const auto &subTextures : subs)
	{
		Graphics::LVP_SubTextures subLine;

		bool forceBlankNewLine = false;
		int  offsetX           = 0;
		int  offsetY           = 0;

		for (size_t i = 0; i < subTextures.second.size(); i++)
		{
			auto subTexture = subTextures.second[i];

			if (subTexture->subtitle->skip)
				continue;

			subTexture->locationRender.x = offsetX;
			subTexture->locationRender.y = offsetY;
			subTexture->locationRender.w = (subTexture->textureData != NULL ? subTexture->textureData->width  : 0);
			subTexture->locationRender.h = (subTexture->textureData != NULL ? subTexture->textureData->height : 0);

			// FORCED BLANK NEW LINES (t1\N\N\Nt2)
			if (subTexture->subtitle->text == " ")
			{
				if (subTexture->subtitle->style != NULL) {
					subTexture->subtitle->style->blur    = 0;
					subTexture->subtitle->style->outline = 0;
					subTexture->subtitle->style->shadow  = {};
				}

				if (subTexture->subtitle->text2.find("\\fs") == std::string::npos)
					subTexture->locationRender.h = (forceBlankNewLine ? subTexture->locationRender.h / 2 : 0);
			} else {
				forceBlankNewLine = true;
			}

			// ROTATION
			subTexture->subtitle->rotationPoint.x = ((subTexture->total.w / 2) - offsetX);

			subLine.push_back(subTexture);

			// Add split subs belonging to the same sub line
			if ((i < subTextures.second.size() - 1) && !subTexture->offsetY)
			{
				if (!System::LVP_Text::Trim(subTexture->subtitle->text).empty())
					offsetX += (subTexture->locationRender.w + LVP_SubStyle::GetOffsetX(subTexture, subContext));

				continue;
			}

			int  maxY    = 0;
			auto margins = subTexture->subtitle->getMargins(subContext.scale);

			// Offset subs horizontally (x) based on alignment
			for (auto subWord : subLine)
			{
				if (subTexture->subtitle->isAlignedLeft())
					subWord->locationRender.x += margins.x;
				else if (subTexture->subtitle->isAlignedRight())
					subWord->locationRender.x += (subContext.videoDimensions.w - subTexture->total.w - margins.y);
				else if (subTexture->subtitle->isAlignedCenter())
					subWord->locationRender.x += ALIGN_CENTER(0, subContext.videoDimensions.w, subTexture->total.w);

				subWord->total.x = subLine[0]->locationRender.x;
				subWord->total.w = subTexture->total.w;

				if (subWord->locationRender.h > maxY)
					maxY = subWord->locationRender.h;
			}

			offsetY += maxY;
			offsetX  = 0;

			subLine.clear();
		}

		for (auto subTexture : subTextures.second)
		{
			if (subTexture->subtitle->skip)
				continue;

			subTexture->total.h = offsetY;

			// Offset subs vertically (y) based on alignment
			int  offsetY = 0;
			auto margins = subTexture->subtitle->getMargins(subContext.scale);

			if (subTexture->subtitle->isAlignedTop())
				offsetY = margins.h;
			else if (subTexture->subtitle->isAlignedMiddle())
				offsetY = ((subContext.videoDimensions.h - subTexture->total.h) / 2);
			else
				offsetY = (subContext.videoDimensions.h - subTexture->total.h - margins.h);

			if (offsetY > 0)
				subTexture->locationRender.y += offsetY;
		}

		int minY = subContext.videoDimensions.h;
		int minX = subContext.videoDimensions.w;
		int maxX = 0;

		for (auto subTexture : subTextures.second)
		{
			if (subTexture->subtitle->skip)
				continue;

			if (subTexture->locationRender.y < minY)
				minY = subTexture->locationRender.y;

			if (subTexture->locationRender.x < minX)
				minX = subTexture->locationRender.x;

			if (subTexture->locationRender.x + subTexture->locationRender.w > maxX)
				maxX = (subTexture->locationRender.x + subTexture->locationRender.w);
		}

		for (auto subTexture : subTextures.second)
		{
			if (subTexture->subtitle->skip)
				continue;

			subTexture->total.x = minX;
			subTexture->total.y = minY;
			subTexture->total.w = (maxX - minX);

			subTexture->subtitle->rotationPoint.y  = (subTexture->total.h / 2);
			subTexture->subtitle->rotationPoint.y -= (subTexture->locationRender.y - subTexture->total.y);
		}

		LVP_SubTextRenderer::handleSubCollisions(subTextures, subs);
	}
}

void MediaPlayer::LVP_SubTextRenderer::setTotalWidth(const Graphics::LVP_SubTexturesId &subs)
{
	for (const auto &subTextures : subs)
	{
		Graphics::LVP_SubTextures subLine;

		int maxWidth   = 0;
		int totalWidth = 0;

		for (size_t i = 0; i < subTextures.second.size(); i++)
		{
			auto subTexture = subTextures.second[i];

			if (subTexture->subtitle->skip)
				continue;

			if ((subTexture->textureData != NULL) && !System::LVP_Text::Trim(subTexture->subtitle->text).empty())
				totalWidth += subTexture->textureData->width;

			subLine.push_back(subTexture);

			if ((i < subTextures.second.size() - 1) && !subTexture->offsetY)
				continue;

			for (auto sub : subLine)
				sub->total.w = totalWidth;

			if (totalWidth > maxWidth)
				maxWidth = totalWidth;

			totalWidth = 0;

			subLine.clear();
		}
	}
}

Strings16 MediaPlayer::LVP_SubTextRenderer::splitSub(uint16_t* subStringUTF16, int subStringWidth, LVP_Subtitle* sub, TTF_Font* font, int subWidth, int maxWidth)
{
	Strings16 subStrings16;

	if ((subStringUTF16 == NULL) || (sub == NULL) || (font == NULL))
		return subStrings16;

	std::string text = System::LVP_Text::Trim(sub->text);

	if ((subWidth > maxWidth) && (sub->text3.find("\\q2") == std::string::npos) && (sub->text3.find("^") == std::string::npos) && !text.empty())
	{
		Strings words = System::LVP_Text::Split(text, " ");

		if (words.size() > 1)
		{
			if (sub->text2.find("} ") != std::string::npos)
				words[0] = std::string(" ").append(words[0]);

			if (System::LVP_Text::GetLastCharacter(sub->text2) == ' ')
				words[words.size() - 1].append(" ");

			if (sub->text3.find("{") == std::string::npos)
			{
				size_t nrLines = LVP_SubTextRenderer::splitSubGetNrLines(words, font, maxWidth);
				subStrings16   = LVP_SubTextRenderer::splitSubDistributeByLines(words, nrLines, font, maxWidth);

				if (subStrings16.empty())
					subStrings16 = LVP_SubTextRenderer::splitSubDistributeByWidth(words, font, maxWidth, maxWidth);
			} else {
				subStrings16 = LVP_SubTextRenderer::splitSubDistributeByWidth(words, font, (maxWidth - (subWidth - subStringWidth)), maxWidth);
			}
		}
		else
		{
			subStrings16.push_back(SDL_iconv_utf8_ucs2(" "));
			subStrings16.push_back(subStringUTF16);
		}

		return subStrings16;
	}

	subStrings16.push_back(subStringUTF16);

	return subStrings16;
}

size_t MediaPlayer::LVP_SubTextRenderer::splitSubGetNrLines(const Strings &words, TTF_Font* font, int maxWidth)
{
	int lineWidth;

	size_t      nrLines     = 0;
	std::string lineString1 = "";
	std::string lineString2 = "";

	for (size_t i = 0; i < words.size(); i++)
	{
		lineString2.append(words[i]);

		lineWidth = System::LVP_Text::GetWidth(lineString2, font);

		if (i == words.size() - 1)
			nrLines++;

		if (lineWidth > maxWidth) {
			nrLines++;

			lineString1 = "";
			lineString2 = (words[i] + " ");
		}

		lineString1.append(words[i] + " ");
		lineString2.append(" ");
	}

	return nrLines;
}

Strings16 MediaPlayer::LVP_SubTextRenderer::splitSubDistributeByLines(const Strings &words, size_t nrLines, TTF_Font* font, int maxWidth)
{
	int       lineWidth, lineHeight;
	Strings16 subStrings16;

	std::string line         = "";
	int         wordsPerLine = (int)ceilf((float)words.size() / (float)nrLines);

	for (size_t i = 0; i < words.size(); i++)
	{
		line.append(words[i]);

		if (((i + 1) % wordsPerLine == 0) || (i == words.size() - 1))
		{
			auto line16 = SDL_iconv_utf8_ucs2(line.c_str());

			TTF_SizeUNICODE(font, line16, &lineWidth, &lineHeight);
			
			if (lineWidth > maxWidth)
			{
				for (auto sub16 : subStrings16)
					DELETE_POINTER(sub16);

				subStrings16.clear();

				return subStrings16;
			}

			subStrings16.push_back(line16);
			line = "";

			continue;
		}

		line.append(" ");
	}

	return subStrings16;
}

Strings16 MediaPlayer::LVP_SubTextRenderer::splitSubDistributeByWidth(const Strings &words, TTF_Font* font, int remainingWidth, int maxWidth)
{
	int       lineWidth;
	Strings16 subStrings16;

	std::string lineString1  = "";
	std::string lineString2  = (!words.empty() ? words[0] : "");
	int         max          = remainingWidth;

	for (size_t i = 0; i < words.size(); i++)
	{
		lineWidth = System::LVP_Text::GetWidth(lineString2, font);

		if (lineWidth > max)
		{
			if (lineString1.empty())
				lineString1 = " ";

			auto line16 = SDL_iconv_utf8_ucs2(lineString1.c_str());

			subStrings16.push_back(line16);

			lineString1 = "";
			lineString2 = words[i];
			max         = maxWidth;
		}

		if (i == words.size() - 1)
		{
			auto endLine16 = SDL_iconv_utf8_ucs2(lineString2.c_str());

			subStrings16.push_back(endLine16);
		}

		lineString1.append(words[i] + " ");

		if (i < words.size() - 1)
			lineString2.append(" " + words[i + 1]);
	}

	return subStrings16;
}

// http://docs.aegisub.org/3.2/ASS_Tags/
// https://en.wikipedia.org/wiki/SubStation_Alpha
MediaPlayer::LVP_Subtitles MediaPlayer::LVP_SubTextRenderer::SplitAndFormatSub(const Strings &subTexts, LVP_SubtitleContext &subContext)
{
	LVP_Subtitles subs;

	if (subTexts.empty())
		return subs;

	for (std::string dialogueLine : subTexts)
	{
		if (dialogueLine.size() > DEFAULT_CHAR_BUFFER_SIZE)
			continue;

		LVP_Subtitle* prevSub = NULL;
		
		// Split and format the dialogue properties:
		//   ReadOrder, Layer, Style, Speaker, MarginL, MarginR, MarginV, Effect, Text
		auto dialogueSplit = System::LVP_Text::Split(dialogueLine, ",");

		if (dialogueSplit.size() < SUB_DIALOGUE_TEXT)
			continue;

		auto subStyle = LVP_SubTextRenderer::getSubStyle(subContext.styles, dialogueSplit);
		auto subText  = LVP_SubTextRenderer::getSubText(dialogueLine, subContext.styles.size());
		auto subID    = std::atoi(dialogueSplit[SUB_DIALOGUE_READORDER].c_str());

		// Split by partial formatting ({\f1}t1{\f2}t2)
		Strings subLines = LVP_SubTextRenderer::formatSplitStyling(subText, subStyle, subContext);

		for (auto &subLine : subLines)
		{
			// CUSTOM DRAW OPERATION - Fill Rect
			LVP_SubTextRenderer::formatDrawCommand(subLine, dialogueSplit, subID, subs, subContext);

			// Skip unsupported draw operations
			if (!System::LVP_Text::IsValidSubtitle(subLine))
				continue;

			auto sub = new LVP_Subtitle();

			sub->id    = subID;
			sub->layer = std::atoi(dialogueSplit[SUB_DIALOGUE_LAYER].c_str());

			if (subLine.empty())
				subLine = " ";

			sub->text2     = std::string(subLine);
			sub->text3     = std::string(subText);
			sub->textSplit = dialogueSplit;

			if (System::LVP_Text::EndsWith(subLine, '^'))
				subLine = subLine.substr(0, subLine.size() - 1);

			// Set sub style
			sub->style = new LVP_SubStyle(*subStyle);

			// Set sub rotation from style
			sub->rotation = sub->style->rotation;

			// Remove formatting tags
			sub->text = LVP_SubTextRenderer::RemoveFormatting(subLine);

			// UTF-16
			sub->textUTF16 = SDL_iconv_utf8_ucs2(sub->text.c_str());

			if (sub->textUTF16 == NULL) {
				DELETE_POINTER(sub);
				continue;
			}
			
			// MULTIPLE_LINE SUB - Inherit from previous sub
			if (prevSub != NULL)
				sub->copy(*prevSub);

			// Remove animations
			Strings animations = LVP_SubTextRenderer::formatGetAnimations(sub->text3);
			sub->text2         = LVP_SubTextRenderer::formatRemoveAnimations(sub->text2);

			LVP_SubTextRenderer::formatOverrideStyleCat1(animations, sub, subContext.styles);
			LVP_SubTextRenderer::formatOverrideStyleCat2(animations, sub, subContext.styles);

			// Add split lines to the list of sub strings
			subs.push_back(sub);

			prevSub = sub;
		}
	}

	return subs;
}
