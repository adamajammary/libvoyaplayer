#include "LVP_Text.h"

bool System::LVP_Text::EndsWith(const std::string &text, char character) {
	return (!text.empty() && (text[text.size() - 1] == character));
}

char System::LVP_Text::GetLastCharacter(const std::string &text)
{
	return (!text.empty() ? text[text.size() - 1] : '\0');
}

int System::LVP_Text::GetSpaceWidth(TTF_Font* font)
{
	int minx, maxx, miny, maxy, advance;

	TTF_GlyphMetrics(font, ' ', &minx, &maxx, &miny, &maxy, &advance);

	return (advance * 2);
}

// NOTE! Updates source by removing token from the start of the string
std::string System::LVP_Text::getToken(std::string &source, const std::string &delimiter)
{
	size_t      delimPos = strcspn(source.c_str(), delimiter.c_str());
	std::string token    = source.substr(0, delimPos);

	source = source.substr(delimPos + (delimPos < source.size() ? 1 : 0));

	return token;
}

int System::LVP_Text::GetWidth(const std::string &text, TTF_Font* font)
{
	if (!text.empty())
	{
		auto text16 = SDL_iconv_utf8_ucs2(text.c_str());

		int textWidth, h;
		TTF_SizeUNICODE(font, text16, &textWidth, &h);

		SDL_free(text16);

		return textWidth;
	}

	return 0;
}

bool System::LVP_Text::IsValidSubtitle(const std::string &subtitle)
{
	return ((subtitle.rfind("\\p0")  == std::string::npos) && (subtitle.rfind("\\p1") == std::string::npos) &&
			(subtitle.rfind("\\p2")  == std::string::npos) && (subtitle.rfind("\\p4") == std::string::npos) &&
			(subtitle.rfind("\\pbo") == std::string::npos));
}

std::string System::LVP_Text::Replace(const std::string &text, const std::string &oldSubstring, const std::string &newSubstring)
{
	std::string formattedString = std::string(text);
	size_t      lastPosition    = formattedString.find(oldSubstring);

	while (lastPosition != std::string::npos) {
		formattedString = formattedString.replace(lastPosition, oldSubstring.size(), newSubstring);
		lastPosition    = formattedString.find(oldSubstring, (lastPosition + newSubstring.size()));
	}

	return formattedString;
}

Strings System::LVP_Text::Split(const std::string &text, const std::string &delimiter, bool returnEmpty)
{
	Strings     parts;
	std::string fullString = std::string(text);

	if (fullString.empty())
		return parts;

	if (fullString.find(delimiter) == std::string::npos) {
		parts.push_back(fullString);
		return parts;
	}

	while (!fullString.empty())
	{
		std::string part = LVP_Text::getToken(fullString, delimiter);

		if (!part.empty() || returnEmpty)
			parts.push_back(part);
	}

	return parts;
}

std::string System::LVP_Text::ToLower(const std::string &text)
{
	std::string lower = std::string(text);

	for (int i = 0; i < (int)lower.size(); i++)
		lower[i] = tolower(lower[i]);

	return lower;
}

#if defined _windows
std::wstring System::LVP_Text::ToLower(const std::wstring &text)
{
	std::wstring lower = std::wstring(text);

	for (int i = 0; i < (int)lower.size(); i++)
		lower[i] = tolower(lower[i]);

	return lower;
}
#endif

std::string System::LVP_Text::ToUpper(const std::string &text)
{
	std::string upper = std::string(text);

	for (int i = 0; i < (int)upper.size(); i++)
		upper[i] = toupper(upper[i]);

	return upper;
}

std::string System::LVP_Text::Trim(const std::string &text)
{
	auto stringTrimmed = std::string(text);

	// TRIM FRONT
	while (!stringTrimmed.empty() && std::isspace(stringTrimmed[0], std::locale()))
		stringTrimmed = stringTrimmed.substr(1);

	// TRIM END
	while (!stringTrimmed.empty() && std::isspace(stringTrimmed[stringTrimmed.size() - 1], std::locale()))
		stringTrimmed = stringTrimmed.substr(0, stringTrimmed.size() - 1);
	
	return stringTrimmed;
}
