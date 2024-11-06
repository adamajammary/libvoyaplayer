#include "LVP_FileSystem.h"

LVP_Strings System::LVP_FileSystem::getDirectoryContent(const std::string& directoryPath, bool returnFiles, bool checkSystemFiles)
{
	LVP_Strings directoyContent;

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

LVP_Strings System::LVP_FileSystem::getDirectoryFiles(const std::string& directoryPath, bool checkSystemFiles)
{
	return LVP_FileSystem::getDirectoryContent(directoryPath, true, checkSystemFiles);
}

std::string System::LVP_FileSystem::GetFileExtension(const std::string& filePath)
{
	if ((filePath.rfind(".") != std::string::npos) && (LVP_Text::GetLastCharacter(filePath) != '.'))
		return LVP_Text::ToLower(filePath.substr(filePath.rfind(".") + 1));

	return "";
}

std::string System::LVP_FileSystem::getFileName(const std::string& filePath, bool removeExtension)
{
	auto fileDetails = LVP_Strings();
	auto fileName    = std::string(filePath);

	// BLURAY/DVD: "concat:streamPath|stream1|...|streamN|duration|title|audioTrackCount|subTrackCount|"
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

size_t System::LVP_FileSystem::GetFileSize(const std::string& filePath)
{
	size_t fileSize = 0;
	bool   isConcat = LVP_FileSystem::IsConcat(filePath);

	// BLURAY/DVD: "concat:streamPath|stream1|...|streamN|duration|title|audioTrackCount|subTrackCount|"
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

LVP_Strings System::LVP_FileSystem::GetSubtitleFilesForVideo(const std::string& videoFilePath)
{
	auto directory        = videoFilePath.substr(0, videoFilePath.rfind(PATH_SEPARATOR));
	auto filesInDirectory = LVP_FileSystem::getDirectoryFiles(directory);
	auto videoFileName    = LVP_Text::ToLower(LVP_FileSystem::getFileName(videoFilePath, true));

	LVP_Strings subtitleFiles;
	std::string idxFile = "";

	for (const auto& file : filesInDirectory)
	{
		auto fileName = LVP_Text::ToLower(LVP_FileSystem::getFileName(file, true));

		if (!LVP_FileSystem::isSubtitleFile(file) || !fileName.starts_with(videoFileName))
			continue;

		if (!idxFile.empty() && (fileName == idxFile)) {
			idxFile = "";
			continue;
		}

		try
		{
			auto subtitleFile = LVP_Text::Format("%s%c%s", directory.c_str(), PATH_SEPARATOR, file.c_str());
			auto mediaType    = MediaPlayer::LVP_Player::GetMediaType(subtitleFile);

			if (mediaType == LVP_MEDIA_TYPE_SUBTITLE)
				subtitleFiles.push_back(subtitleFile);
		}
		catch (const std::exception& e)
		{
			#if defined _DEBUG
				LOG("ERROR: %s\n", e.what());
			#endif

			continue;
		}

		if (LVP_FileSystem::GetFileExtension(file) == "idx")
			idxFile = fileName;
	}

	return subtitleFiles;
}

bool System::LVP_FileSystem::hasFileExtension(const std::string& filePath)
{
	return (!LVP_FileSystem::GetFileExtension(filePath).empty());
}

bool System::LVP_FileSystem::IsBlurayAACS(const std::string& filePath, size_t fileSize)
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

bool System::LVP_FileSystem::IsConcat(const std::string& filePath)
{
	return (filePath.substr(0, 7) == "concat:");
}

bool System::LVP_FileSystem::IsDVDCSS(const std::string& filePath, size_t fileSize)
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

bool System::LVP_FileSystem::isSubtitleFile(const std::string& filePath)
{
	if (!LVP_FileSystem::hasFileExtension(filePath) || (filePath.size() >= MAX_FILE_PATH))
		return false;

	auto extension = LVP_FileSystem::GetFileExtension(filePath);

	if (LVP_Text::VectorContains(SUB_FILE_EXTENSIONS, extension))
		return true;

	return false;
}

bool System::LVP_FileSystem::IsSystemFile(const std::string& fileName)
{
	if (fileName.empty())
		return true;

	auto extension = LVP_FileSystem::GetFileExtension(fileName);

	if (LVP_Text::VectorContains(SYSTEM_FILE_EXTENSIONS, extension))
		return true;

	return false;
}
