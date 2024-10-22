#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_FILESYSTEM_H
#define LVP_FILESYSTEM_H

namespace LibVoyaPlayer
{
	namespace System
	{
		class LVP_FileSystem
		{
		private:
			LVP_FileSystem()  {}
			~LVP_FileSystem() {}

		public:
			static std::string GetFileExtension(const std::string& filePath);
			static size_t      GetFileSize(const std::string& filePath);
			static LVP_Strings GetSubtitleFilesForVideo(const std::string& videoFilePath);
			static bool        IsBlurayAACS(const std::string& filePath, size_t fileSize);
			static bool        IsConcat(const std::string& filePath);
			static bool        IsDVDCSS(const std::string& filePath, size_t fileSize);
			static bool        IsSystemFile(const std::string& fileName);

		private:
			static LVP_Strings getDirectoryContent(const std::string& directoryPath, bool returnFiles, bool checkSystemFiles = true);
			static LVP_Strings getDirectoryFiles(const std::string& directoryPath, bool checkSystemFiles = true);
			static std::string getFileName(const std::string& filePath, bool removeExtension);
			static bool        hasFileExtension(const std::string& filePath);
			static bool        isSubtitleFile(const std::string& filePath);
		};
	}
}

#endif
