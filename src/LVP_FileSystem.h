#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_FILESYSTEM_H
#define LVP_FILESYSTEM_H

namespace LibVoyaPlayer
{
	namespace System
	{
		struct LVP_File
		{
			std::string ext      = "";
			std::string file     = "";
			std::string filePath = "";
			std::string name     = "";
			std::string path     = "";
			char        pathSep  = '/';
		};

		class LVP_FileSystem
		{
		private:
			LVP_FileSystem()  {}
			~LVP_FileSystem() {}

		public:
			static LVP_File    GetFile(const std::string& filePath);
			static size_t      GetFileSize(const std::string& filePath);
			static LVP_Strings GetSubtitleFilesForVideo(const std::string& videoFilePath);
			static bool        IsBlurayAACS(const std::string& filePath, size_t fileSize);
			static bool        IsConcat(const std::string& filePath);
			static bool        IsDVDCSS(const std::string& filePath, size_t fileSize);
			static bool        IsSystemFile(const LVP_File& file);

		private:
			static LVP_Strings getDirectoryFiles(const std::string& directoryPath);
			static bool        isSubtitleFile(const LVP_File& file);
		};
	}
}

#endif
