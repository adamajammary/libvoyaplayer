#include "LVP_FileSystem.h"

LVP_Strings System::LVP_FileSystem::getDirectoryFiles(const std::string& directoryPath)
{
	LVP_Strings files;

	auto directory = std::filesystem::recursive_directory_iterator(directoryPath);

	for (const auto& dirEntry : directory)
	{
		if (!dirEntry.is_regular_file())
			continue;

		auto filePathUTF8 = SDL_iconv_wchar_utf8(dirEntry.path().generic_wstring().c_str());
		auto filePath     = std::string(filePathUTF8);

		SDL_free(filePathUTF8);

		if (!LVP_FileSystem::IsSystemFile(LVP_FileSystem::GetFile(filePath)))
			files.push_back(filePath);
	}

	return files;
}

System::LVP_File System::LVP_FileSystem::GetFile(const std::string& filePath)
{
	LVP_File file = { .filePath = filePath };

	// BLURAY/DVD: "concat:streamPath|stream1|...|streamN|duration|title|audioTrackCount|subTrackCount|"
	if (LVP_FileSystem::IsConcat(filePath))
	{
		auto fileDetails = LVP_Text::Split(filePath.substr(7), "|");

		if (!fileDetails.empty())
			file.path = fileDetails[0]; // streamPath

		if (fileDetails.size() > 2)
			file.name = fileDetails[fileDetails.size() - 3]; // title

		return file;
	}

	auto lastSeparator = file.filePath.rfind('/');

    if (lastSeparator == std::string::npos)
        lastSeparator = file.filePath.rfind('\\');

    if (lastSeparator == std::string::npos)
        return file;

	file.pathSep = file.filePath[lastSeparator];
	file.path    = file.filePath.substr(0, lastSeparator);
    file.file    = file.filePath.substr(lastSeparator + 1);

    auto extension = file.file.rfind('.');

    if (extension == std::string::npos)
        return file;

    file.name = file.file.substr(0, extension);
    file.ext  = LVP_Text::ToLower(file.file.substr(extension + 1));

    return file;
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

		return fileSize;
	}

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

	return fileSize;
}

LVP_Strings System::LVP_FileSystem::GetSubtitleFilesForVideo(const std::string& videoFilePath)
{
	LVP_Strings subtitleFiles;

	auto videoFile        = LVP_FileSystem::GetFile(videoFilePath);
	auto videoFileName    = LVP_Text::ToLower(videoFile.name);
	auto filesInDirectory = LVP_FileSystem::getDirectoryFiles(videoFile.path);

	std::string idxFile = "";

	for (const auto& filePath : filesInDirectory)
	{
		auto file     = LVP_FileSystem::GetFile(filePath);
		auto fileName = LVP_Text::ToLower(file.name);

		if (!LVP_FileSystem::isSubtitleFile(file) || !fileName.starts_with(videoFileName))
			continue;

		if (!idxFile.empty() && (fileName == idxFile)) {
			idxFile = "";
			continue;
		}

		try
		{
			auto mediaType = MediaPlayer::LVP_Player::GetMediaType(filePath);

			if (mediaType == LVP_MEDIA_TYPE_SUBTITLE)
				subtitleFiles.push_back(filePath);
		}
		catch (const std::exception& e)
		{
			#if defined _DEBUG
				LOG("ERROR: %s\n", e.what());
			#endif

			continue;
		}

		if (file.ext == "idx")
			idxFile = fileName;
	}

	return subtitleFiles;
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

bool System::LVP_FileSystem::isSubtitleFile(const LVP_File& file)
{
	return LVP_Text::VectorContains(SUB_FILE_EXTENSIONS, file.ext);
}

bool System::LVP_FileSystem::IsSystemFile(const LVP_File& file)
{
	return LVP_Text::VectorContains(SYSTEM_FILE_EXTENSIONS, file.ext);
}
