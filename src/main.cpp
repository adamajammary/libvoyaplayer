#include <libvoyaplayer.h>

#include "main.h"

const char ERROR_NO_INIT[] = "libvoyaplayer has not been initialized.";

bool isInitialized = false;

void LVP_Initialize(const LVP_CallbackContext &callbackContext)
{
	try
	{
		if (isInitialized)
			LVP_Quit();

		#if defined _android
			SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "0");
		#elif defined _ios
			SDL_SetHint(SDL_HINT_AUDIO_CATEGORY, "AVAudioSessionCategoryPlayback");
		#endif

		SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE, "3");
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,  "2");

		if ((SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) ||
			(SDL_AudioInit(NULL) < 0) ||
			(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0))
		{
			throw std::runtime_error(System::LVP_Text::Format("Failed to initialize SDL2: %s", SDL_GetError()));
		}

		#if defined _DEBUG
			LibFFmpeg::av_log_set_level(AV_LOG_VERBOSE);
		#else
			LibFFmpeg::av_log_set_level(AV_LOG_QUIET);
		#endif

		if ((LibFFmpeg::av_version_info() == NULL) ||
			(LibFFmpeg::avcodec_version() == 0) ||
			(LibFFmpeg::avformat_version() == 0))
		{
			throw std::runtime_error("Failed to initialize FFmpeg.");
		}

		MediaPlayer::LVP_Player::Init(callbackContext);

		isInitialized = true;
	}
	catch (const std::exception &e)
	{
		if (callbackContext.errorCB != nullptr)
			callbackContext.errorCB(System::LVP_Text::Format("Failed to initialize libvoyaplayer:\n%s", e.what()), callbackContext.data);
		else
			throw e;
	}
}

std::vector<std::string> LVP_GetAudioDevices()
{
	return MediaPlayer::LVP_Player::GetAudioDevices();
}

int LVP_GetAudioTrack()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetAudioTrack();
}

std::vector<LVP_MediaTrack> LVP_GetAudioTracks()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetAudioTracks();
}

std::vector<LVP_MediaChapter> LVP_GetChapters()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetChapters();
}

int64_t LVP_GetDuration()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetDuration();
}

std::string LVP_GetFilePath()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetFilePath();
}

LVP_MediaDetails LVP_GetMediaDetails()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetMediaDetails();
}

LVP_MediaDetails LVP_GetMediaDetails(const std::string &filePath)
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	LVP_MediaDetails mediaDetails = {};

	try {
		mediaDetails = MediaPlayer::LVP_Player::GetMediaDetails(filePath);
	} catch (const std::exception& e) {
		MediaPlayer::LVP_Player::CallbackError(System::LVP_Text::Format("Failed to parse media file:\n%s", e.what()));
	}

	return mediaDetails;
}

LVP_MediaDetails LVP_GetMediaDetails(const std::wstring &filePath)
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	LVP_MediaDetails mediaDetails = {};

	try {
		auto filePathUTF8 = SDL_iconv_wchar_utf8(filePath.c_str());
		mediaDetails      = MediaPlayer::LVP_Player::GetMediaDetails(filePathUTF8);

		SDL_free(filePathUTF8);
	} catch (const std::exception& e) {
		MediaPlayer::LVP_Player::CallbackError(System::LVP_Text::Format("Failed to parse media file:\n%s", e.what()));
	}

	return mediaDetails;
}

LVP_MediaType LVP_GetMediaType()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetMediaType();
}

LVP_MediaType LVP_GetMediaType(const std::string &filePath)
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	LVP_MediaType mediaType = LVP_MEDIA_TYPE_UNKNOWN;

	try {
		mediaType = MediaPlayer::LVP_Player::GetMediaType(filePath);
	} catch (const std::exception& e) {
		MediaPlayer::LVP_Player::CallbackError(System::LVP_Text::Format("Failed to parse media file:\n%s", e.what()));
	}

	return mediaType;
}

LVP_MediaType LVP_GetMediaType(const std::wstring &filePath)
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	LVP_MediaType mediaType = LVP_MEDIA_TYPE_UNKNOWN;

	try {
		auto filePathUTF8 = SDL_iconv_wchar_utf8(filePath.c_str());
		mediaType = MediaPlayer::LVP_Player::GetMediaType(filePathUTF8);

		SDL_free(filePathUTF8);
	} catch (const std::exception& e) {
		MediaPlayer::LVP_Player::CallbackError(System::LVP_Text::Format("Failed to parse media file:\n%s", e.what()));
	}

	return mediaType;
}

double LVP_GetPlaybackSpeed()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetPlaybackSpeed();
}

int64_t LVP_GetProgress()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetProgress();
}

int LVP_GetSubtitleTrack()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetSubtitleTrack();
}

std::vector<LVP_MediaTrack> LVP_GetSubtitleTracks()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetSubtitleTracks();
}

std::vector<LVP_MediaTrack> LVP_GetVideoTracks()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetVideoTracks();
}

double LVP_GetVolume()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetVolume();
}

bool LVP_IsMuted()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::IsMuted();
}

bool LVP_IsPaused()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::IsPaused();
}

bool LVP_IsPlaying()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::IsPlaying();
}

bool LVP_IsStopped()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::IsStopped();
}

void LVP_Open(const std::string &filePath)
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::Open(filePath);
}

void LVP_Open(const std::wstring &filePath)
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	auto filePathUTF8 = SDL_iconv_wchar_utf8(filePath.c_str());

	MediaPlayer::LVP_Player::Open(filePathUTF8);

	SDL_free(filePathUTF8);
}

void LVP_Quit()
{
	if (!isInitialized)
		return;

	isInitialized = false;

	MediaPlayer::LVP_Player::Quit();

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_AudioQuit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void LVP_Render(const SDL_Rect& destination)
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::Render(destination);
}

void LVP_Resize()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::Resize();
}

void LVP_SeekTo(double percent)
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::SeekTo(percent);
}

bool LVP_SetAudioDevice(const std::string &device)
{
	return MediaPlayer::LVP_Player::SetAudioDevice(device);
}

void LVP_SetMuted(bool muted)
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::SetMuted(muted);
}

void LVP_SetPlaybackSpeed(double speed)
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::SetPlaybackSpeed(speed);
}

void LVP_SetTrack(const LVP_MediaTrack &track)
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::SetTrack(track);
}

void LVP_SetVolume(double percent)
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::SetVolume(percent);
}

void LVP_Stop()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::Close();
}

void LVP_ToggleMute()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::ToggleMute();
}

void LVP_TogglePause()
{
	if (!isInitialized)
		throw std::runtime_error(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::TogglePause();
}

#if defined _windows
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    return TRUE;
}
#else
int entry()
{
	return 0;
}
#endif
