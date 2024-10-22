#include "LVP_Text.h"

char System::LVP_Text::GetLastCharacter(const std::string& text)
{
	return (!text.empty() ? text[text.size() - 1] : '\0');
}

// NOTE! Updates source by removing token from the start of the string
std::string System::LVP_Text::getToken(std::string& source, const std::string& delimiter)
{
	auto delim = source.find(delimiter);
	auto token = source.substr(0, delim);

	source = (delim != std::string::npos ? source.substr(delim + delimiter.size()) : "");

	return token;
}

std::string System::LVP_Text::Replace(const std::string& text, const std::string& oldSubstring, const std::string& newSubstring)
{
	std::string formattedString = std::string(text);
	size_t      lastPosition    = formattedString.find(oldSubstring);

	while (lastPosition != std::string::npos) {
		formattedString = formattedString.replace(lastPosition, oldSubstring.size(), newSubstring);
		lastPosition    = formattedString.find(oldSubstring, (lastPosition + newSubstring.size()));
	}

	return formattedString;
}

LVP_Strings System::LVP_Text::Split(const std::string& text, const std::string& delimiter, bool returnEmpty)
{
	LVP_Strings parts;
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

std::string System::LVP_Text::ToLower(const std::string& text)
{
	std::string lower = std::string(text);

	for (int i = 0; i < (int)lower.size(); i++)
		lower[i] = std::tolower(lower[i]);

	return lower;
}

uint16_t* System::LVP_Text::ToUTF16(const std::string& text)
{
	#if defined _linux
		return (uint16_t*)SDL_iconv_string("UCS-2", "UTF-8", text.c_str(), text.size() + 1);
	#else
		return (uint16_t*)SDL_iconv_string("UCS-2-INTERNAL", "UTF-8", text.c_str(), text.size() + 1);
	#endif
}
