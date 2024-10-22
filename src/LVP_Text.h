#ifndef LVP_MAIN_H
	#include "main.h"
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
			static char        GetLastCharacter(const std::string& text);
			static std::string Replace(const std::string& text, const std::string& oldSubstring, const std::string& newSubstring);
			static LVP_Strings Split(const std::string& text, const std::string& delimiter, bool returnEmpty = true);
			static std::string ToLower(const std::string& text);
			static uint16_t*   ToUTF16(const std::string& text);

			template<typename... Args>
			static std::string Format(const char* formatString, const Args&... args)
			{
				if (!formatString)
					return "";

				char buffer[DEFAULT_CHAR_BUFFER_SIZE] = {};
				auto result = std::snprintf(buffer, DEFAULT_CHAR_BUFFER_SIZE, formatString, args...);

				if (result < DEFAULT_CHAR_BUFFER_SIZE)
					return std::string(buffer);

				auto bufferSize    = (size_t)(result + 1);
				auto dynamicBuffer = (char*)std::malloc(bufferSize);

				std::snprintf(dynamicBuffer, bufferSize, formatString, args...);

				auto resultString = std::string(dynamicBuffer != NULL ? dynamicBuffer : "");

				FREE_POINTER(dynamicBuffer);

				return resultString;
			}

			template <class T>
			static bool VectorContains(const std::vector<T>& vector, const T& value)
			{
				return (std::find(vector.begin(), vector.end(), value) != vector.end());
			}

		private:
			static std::string getToken(std::string& source, const std::string& delimiter);
		};
	}
}

#endif
