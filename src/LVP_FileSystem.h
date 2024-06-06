#ifndef LVP_GLOBAL_H
	#include "LVP_Global.h"
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

		private:
			static Strings subFileExtensions;
			static Strings systemFileExtensions;

		public:
			static std::string GetFileExtension(const std::string &filePath);
			static size_t      GetFileSize(const std::string &filePath);
			static Strings     GetSubtitleFilesForVideo(const std::string& videoFilePath);
			static bool        IsBlurayAACS(const std::string &filePath, size_t fileSize);
			static bool        IsConcat(const std::string &filePath);
			static bool        IsDVDCSS(const std::string &filePath, size_t fileSize);
			static bool        IsSystemFile(const std::string &fileName);

			#if defined _windows
				static SDL_RWops* FileOpenSDLRWops(FILE* file);
			#endif

		private:
			static Strings     getDirectoryContent(const std::string &directoryPath, bool returnFiles, bool checkSystemFiles = true);
			static Strings     getDirectoryFiles(const std::string &directoryPath, bool checkSystemFiles = true);
			static std::string getFileName(const std::string &filePath, bool removeExtension);
			static bool        hasFileExtension(const std::string &filePath);
			static bool        isSubtitleFile(const std::string &filePath);

			#if defined _windows
				static int SDLCALL    SDL_RW_Close(SDL_RWops* rwops);
				static size_t SDLCALL SDL_RW_Read(SDL_RWops*  rwops, void* ptr, size_t size, size_t count);
				static Sint64 SDLCALL SDL_RW_Seek(SDL_RWops*  rwops, Sint64 offset, int whence);
				static Sint64 SDLCALL SDL_RW_Size(SDL_RWops*  rwops);
				static size_t SDLCALL SDL_RW_Write(SDL_RWops* rwops, const void* ptr, size_t size, size_t count);
			#endif

		};
	}
}

#endif
