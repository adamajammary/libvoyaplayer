#include "LVP_FileSystem.h"
#include "LVP_FS_Extensions.h"

#if defined _windows
int SDLCALL System::LVP_FileSystem::SDL_RW_Close(SDL_RWops* rwops)
{
	int status = 0;

	if (rwops != NULL)
	{
		if (std::fclose(static_cast<FILE*>(rwops->hidden.windowsio.buffer.data)) != 0)
			status = SDL_Error(SDL_EFWRITE);

		SDL_FreeRW(rwops);
	}

	return status;
}

size_t SDLCALL System::LVP_FileSystem::SDL_RW_Read(SDL_RWops* rwops, void* ptr, size_t size, size_t count)
{
	auto readSize = std::fread(ptr, size, count, static_cast<FILE*>(rwops->hidden.windowsio.buffer.data));
	bool isError  = (readSize == 0 && std::ferror(static_cast<FILE*>(rwops->hidden.windowsio.buffer.data)) != 0);

	return (isError ? SDL_Error(SDL_EFREAD) : readSize);
}

Sint64 SDLCALL System::LVP_FileSystem::SDL_RW_Seek(SDL_RWops* rwops, Sint64 offset, int whence)
{
	auto result = fseek(static_cast<FILE*>(rwops->hidden.windowsio.buffer.data), offset, whence);

	return (result != 0 ? SDL_Error(SDL_EFSEEK) : std::ftell(static_cast<FILE*>(rwops->hidden.windowsio.buffer.data)));
}

Sint64 SDLCALL System::LVP_FileSystem::SDL_RW_Size(SDL_RWops* rwops)
{
	auto position = SDL_RWseek(rwops, 0, RW_SEEK_CUR);

	if (position < 0)
		return -1;

	auto size = SDL_RWseek(rwops, 0, RW_SEEK_END);

	SDL_RWseek(rwops, position, RW_SEEK_SET);

	return size;
}

size_t SDLCALL System::LVP_FileSystem::SDL_RW_Write(SDL_RWops* rwops, const void* ptr, size_t size, size_t count)
{
	auto writeSize = std::fwrite(ptr, size, count, static_cast<FILE*>(rwops->hidden.windowsio.buffer.data));
	bool isError   = (writeSize == 0 && std::ferror(static_cast<FILE*>(rwops->hidden.windowsio.buffer.data)) != 0);

	return (isError ? SDL_Error(SDL_EFWRITE) : writeSize);
}

SDL_RWops* System::LVP_FileSystem::FileOpenSDLRWops(FILE* file)
{
	if (file == NULL)
		return NULL;

	auto rwops = SDL_AllocRW();

	if (rwops == NULL)
		return NULL;

	rwops->close = LVP_FileSystem::SDL_RW_Close;
	rwops->read  = LVP_FileSystem::SDL_RW_Read;
	rwops->seek  = LVP_FileSystem::SDL_RW_Seek;
	rwops->size  = LVP_FileSystem::SDL_RW_Size;
	rwops->write = LVP_FileSystem::SDL_RW_Write;
	rwops->type  = SDL_RWOPS_STDFILE;

	rwops->hidden.windowsio.buffer.data = file;

	return rwops;
}
#endif

Strings System::LVP_FileSystem::getDirectoryContent(const std::string &directoryPath, bool returnFiles, bool checkSystemFiles)
{
	Strings directoyContent;

	if (directoryPath.size() >= MAX_FILE_PATH)
		return directoyContent;

	#if defined _windows
		auto directoryPathUTF16 = (wchar_t*)LVP_Text::ToUTF16(directoryPath.c_str());
		DIR* directory          = opendir(directoryPathUTF16);

		SDL_free(directoryPathUTF16);
	#else
		DIR* directory = opendir(directoryPath.c_str());
	#endif

	if (directory == NULL)
		return directoyContent;

	dirent* file;
	int     fileType = (returnFiles ? DT_REG : DT_DIR);

	while ((file = readdir(directory)) != NULL) {
		#if defined _windows
			auto fileNameUTF8 = SDL_iconv_wchar_utf8(file->d_name);
			auto fileName     = std::string(fileNameUTF8);

			SDL_free(fileNameUTF8);
		#else
			auto fileName = std::string(file->d_name);
		#endif

		if ((file->d_type == fileType) && (!checkSystemFiles || !LVP_FileSystem::IsSystemFile(fileName)))
			directoyContent.push_back(fileName);
	}

	closedir(directory);

	return directoyContent;
}

Strings System::LVP_FileSystem::getDirectoryFiles(const std::string &directoryPath, bool checkSystemFiles)
{
	return LVP_FileSystem::getDirectoryContent(directoryPath, true, checkSystemFiles);
}

std::string System::LVP_FileSystem::GetFileExtension(const std::string &filePath, bool upperCase)
{
	std::string fileExtension = "";

	if ((filePath.rfind(".") == std::string::npos) || (LVP_Text::GetLastCharacter(filePath) == '.'))
		return "";

	fileExtension = std::string(filePath.substr(filePath.rfind(".") + 1));
	fileExtension = (upperCase ? LVP_Text::ToUpper(fileExtension) : LVP_Text::ToLower(fileExtension));

	return fileExtension;
}

std::string System::LVP_FileSystem::getFileName(const std::string &filePath, bool removeExtension)
{
	Strings fileDetails;
	auto    fileName = std::string(filePath);

	// BLURAY / DVD: "concat:streamPath|stream1|stream2|streamN|duration|title|audioTrackCount|subTrackCount|"
	if (LVP_FileSystem::IsConcat(filePath)) {
		fileDetails = LVP_Text::Split(filePath, "|");

		if (fileDetails.size() > 2)
			fileName = fileDetails[fileDetails.size() - 3];
	// FILES
	} else if (filePath.rfind(PATH_SEPARATOR) != std::string::npos) {
		fileName = filePath.substr(filePath.rfind(PATH_SEPARATOR) + 1);
	}

	if (removeExtension)
		fileName = fileName.substr(0, fileName.rfind('.'));

	return fileName;
}

size_t System::LVP_FileSystem::GetFileSize(const std::string &filePath)
{
	size_t fileSize = 0;
	bool   isConcat = LVP_FileSystem::IsConcat(filePath);

	// BLURAY / DVD: "concat:streamPath|stream1|...|streamN|duration|title|audioTrackCount|subTrackCount|"
	if (isConcat)
	{
		auto parts = LVP_Text::Split(filePath.substr(7), "|");

		for (uint32_t i = 1; i < parts.size() - 4; i++)
			fileSize += LVP_FileSystem::GetFileSize(parts[0] + parts[i]);
	}
	else
	{
		stat_t fileStruct;

		#if defined _windows
			auto filePath16 = (wchar_t*)LVP_Text::ToUTF16(filePath.c_str());
			int  result     = fstat(filePath16, &fileStruct);

			SDL_free(filePath16);
		#else
			int result = fstat(filePath.c_str(), &fileStruct);
		#endif

		if (result == 0)
			fileSize = (size_t)fileStruct.st_size;
	}

	return fileSize;
}

Strings System::LVP_FileSystem::GetSubtitleFilesForVideo(const std::string &videoFilePath)
{
	auto directory        = videoFilePath.substr(0, videoFilePath.rfind(PATH_SEPARATOR));
	auto filesInDirectory = LVP_FileSystem::getDirectoryFiles(directory);
	auto videoFileName    = LVP_Text::ToUpper(LVP_FileSystem::getFileName(videoFilePath, true));

	Strings     subtitleFiles;
	std::string idxFile = "";

	for (const auto &file : filesInDirectory)
	{
		if (!LVP_FileSystem::isSubtitleFile(file) || (LVP_Text::ToUpper(file).find(videoFileName) == std::string::npos))
			continue;

		if (!idxFile.empty() && (LVP_Text::ToUpper(LVP_FileSystem::getFileName(file, true)) == idxFile)) {
			idxFile = "";
			continue;
		}

		subtitleFiles.push_back(LVP_Text::Format("%s%c%s", directory.c_str(), PATH_SEPARATOR, file.c_str()));

		if (LVP_FileSystem::GetFileExtension(file, true) == "IDX")
			idxFile = LVP_Text::ToUpper(LVP_FileSystem::getFileName(file, true));
	}

	return subtitleFiles;
}

bool System::LVP_FileSystem::hasFileExtension(const std::string &filePath)
{
	return (!LVP_FileSystem::GetFileExtension(filePath, false).empty());
}

bool System::LVP_FileSystem::IsBlurayAACS(const std::string &filePath, size_t fileSize)
{
	if (filePath.empty() || (fileSize == 0))
		return false;

	const size_t SECTOR_SIZE = 6144;
	uint8_t      buffer[SECTOR_SIZE];

	size_t nrSectors = std::min((size_t)1000, fileSize / SECTOR_SIZE);
	auto   file      = fopen(filePath.c_str(), "rb");
	size_t result    = SECTOR_SIZE;
	size_t sector    = 0;

	std::rewind(file);

	while ((sector < nrSectors) && (result == SECTOR_SIZE))
	{
		result = std::fread(&buffer, 1, SECTOR_SIZE, file);

		if (((buffer[0] & 0xC0) != 0) || (buffer[4] != 0x47))
		{
			for (int i = 0; i < 4; i++)
			{
				if (buffer[4 + (i * 0xC0)] != 0x47) {
					result = 0;
					break;
				}
			}
		}

		sector++;
	}

	std::fclose(file);

	return (sector < nrSectors);
}

bool System::LVP_FileSystem::IsConcat(const std::string &filePath)
{
	return (filePath.substr(0, 7) == "concat:");
}

bool System::LVP_FileSystem::IsDVDCSS(const std::string &filePath, size_t fileSize)
{
	if (filePath.empty() || (fileSize == 0))
		return false;

	const size_t SECTOR_SIZE = 2048;
	char         buffer[SECTOR_SIZE];

	size_t nrSectors = std::min((size_t)1000, fileSize / SECTOR_SIZE);
	auto   file      = fopen(filePath.c_str(), "rb");
	size_t result    = SECTOR_SIZE;
	size_t sector    = 0;

	std::rewind(file);

	while ((sector < nrSectors) && (result == SECTOR_SIZE))
	{
		result = std::fread(&buffer, 1, SECTOR_SIZE, file);

		//if ((buffer[(buffer[13] & 0x07) + 20] & 0x30) != 0)
		if ((buffer[0x14] & 0x30) != 0)
			break;

		sector++;
	}

	std::fclose(file);

	return (sector < nrSectors);
}

bool System::LVP_FileSystem::isSubtitleFile(const std::string &filePath)
{
	if (!LVP_FileSystem::hasFileExtension(filePath) || (filePath.size() >= MAX_FILE_PATH))
		return false;

	auto extension = LVP_FileSystem::GetFileExtension(filePath, true);

	if (LVP_Text::VectorContains(LVP_FileSystem::subFileExtensions, extension))
		return true;

	return false;
}

bool System::LVP_FileSystem::IsSystemFile(const std::string &fileName)
{
	if (fileName.empty())
		return true;

	auto extension = LVP_FileSystem::GetFileExtension(fileName, true);

	if (LVP_Text::VectorContains(LVP_FileSystem::systemFileExtensions, extension))
		return true;

	auto file = LVP_Text::ToUpper(fileName);

	return ((file[0] == '.') || (file[0] == '$') || (file == "RECYCLER") || (file == "WINSXS"));
}
