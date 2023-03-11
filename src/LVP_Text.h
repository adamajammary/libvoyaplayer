#ifndef LVP_GLOBAL_H
	#include "LVP_Global.h"
#endif

#ifndef LVP_TEXT_H
#define LVP_TEXT_H

namespace LibVoyaPlayer
{
	namespace System
	{
		class LVP_Text
		{
		private:
			LVP_Text()  {}
			~LVP_Text() {}

		public:
			static bool         EndsWith(const std::string &text, char character);
			static char         GetLastCharacter(const std::string &text);
			static int          GetSpaceWidth(TTF_Font* font);
			static int          GetWidth(const std::string &text, TTF_Font* font);
			static bool         IsValidSubtitle(const std::string &subtitle);
			static std::string  Replace(const std::string &text, const std::string &oldSubstring, const std::string &newSubstring);
			static Strings      Split(const std::string &text, const std::string &delimiter, bool returnEmpty = true);
			static std::string  ToLower(const std::string &text);
			static std::string  ToUpper(const std::string &text);
			static std::string  Trim(const std::string &text);

			template <class T>
			static bool VectorContains(const std::vector<T> &vector, const T &value)
			{
				return (std::find(vector.begin(), vector.end(), value) != vector.end());
			}

			#if defined _windows
				static std::wstring ToLower(const std::wstring &text);
			#endif

		private:
			static std::string getToken(std::string &source, const std::string &delimiter);
		};
	}
}

#endif
