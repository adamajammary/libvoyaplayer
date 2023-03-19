#include <libvoyaplayer.h>

#include "LVP_Global.h"

const char ERROR_NO_INIT[] = "libvoyaplayer has not been initialized.";

bool isInitialized = false;

void initLibraries()
{
	#if defined _android
		SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "0");
	#elif defined _ios
		SDL_SetHint(SDL_HINT_AUDIO_CATEGORY, "AVAudioSessionCategoryPlayback");
	#endif

	SDL_setenv("SDL_VIDEO_YUV_DIRECT",  "1", 1);
	SDL_setenv("SDL_VIDEO_YUV_HWACCEL", "1", 1);

	SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE, "3");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,  "2");

	if ((SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) ||
		(SDL_AudioInit(NULL) < 0) ||
		(SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) ||
		(SDL_InitSubSystem(SDL_INIT_TIMER) < 0))
	{
		throw std::exception(std::format("Failed to initialize SDL: {}", SDL_GetError()).c_str());
	}

	if (TTF_Init() < 0)
		throw std::exception(std::format("Failed to initialize TTF: {}", TTF_GetError()).c_str());

	#if defined _DEBUG
		LibFFmpeg::av_log_set_level(AV_LOG_VERBOSE);
	#else
		LibFFmpeg::av_log_set_level(AV_LOG_QUIET);
	#endif

	if ((LibFFmpeg::av_version_info() == NULL) ||
		(LibFFmpeg::avcodec_version() == 0) ||
		(LibFFmpeg::avformat_version() == 0))
	{
		throw std::exception("Failed to initialize FFMPEG.");
	}
}

void LVP_Initialize(LVP_VideoCallback videoCB, LVP_ErrorCallback errorCB, LVP_EventsCallback eventsCB, const void* data)
{
	try {
		initLibraries();

		LVP_CallbackContext callbackContext = {
			.data     = data,
			.errorCB  = errorCB,
			.eventsCB = eventsCB,
			.videoCB  = videoCB
		};

		MediaPlayer::LVP_Player::Init(callbackContext);

		isInitialized = true;
	}
	catch (const std::exception &e)
	{
		if (errorCB != nullptr)
			errorCB(std::format("Failed to initialize libvoyaplayer:\n- {}", e.what()), data);
		else
			throw e;
	}
}

Strings LVP_GetAudioDevices()
{
	return MediaPlayer::LVP_Player::GetAudioDevices();
}

Strings LVP_GetAudioDrivers()
{
	return MediaPlayer::LVP_Player::GetAudioDrivers();
}

int LVP_GetAudioTrack()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetAudioTrack();
}

std::vector<LVP_MediaTrack> LVP_GetAudioTracks()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetAudioTracks();
}

std::vector<LVP_MediaChapter> LVP_GetChapters()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetChapters();
}

int64_t LVP_GetDuration()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetDuration();
}

std::string LVP_GetFilePath()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetFilePath();
}

LVP_MediaMeta LVP_GetMediaMeta()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetMediaMeta();
}

double LVP_GetPlaybackSpeed()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetPlaybackSpeed();
}

int64_t LVP_GetProgress()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetProgress();
}

LVP_State LVP_GetState()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetState();
}

int LVP_GetSubtitleTrack()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetSubtitleTrack();
}

std::vector<LVP_MediaTrack> LVP_GetSubtitleTracks()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetSubtitleTracks();
}

std::vector<LVP_MediaTrack> LVP_GetVideoTracks()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetVideoTracks();
}

double LVP_GetVolume()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::GetVolume();
}

bool LVP_IsMuted()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::IsMuted();
}

bool LVP_IsPaused()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	return MediaPlayer::LVP_Player::IsPaused();
}

void LVP_Open(const std::string &filePath)
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	try {
		MediaPlayer::LVP_Player::Open(filePath);
	} catch (const std::exception &e) {
		MediaPlayer::LVP_Player::CallbackError(std::format("Failed to open media file:\n- {}", e.what()));
	}
}

void LVP_Open(const std::wstring &filePath)
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	try {
		auto filePathUTF8 = SDL_iconv_wchar_utf8(filePath.c_str());

		MediaPlayer::LVP_Player::Open(filePathUTF8);

		SDL_free(filePathUTF8);
	} catch (const std::exception &e) {
		MediaPlayer::LVP_Player::CallbackError(std::format("Failed to open media file:\n- {}", e.what()));
	}
}

void LVP_Quit()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::Close();

	TTF_Quit();

	SDL_QuitSubSystem(SDL_INIT_TIMER);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_AudioQuit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void LVP_SeekTo(double percent)
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::SeekTo(percent);
}

bool LVP_SetAudioDevice(const std::string &device)
{
	return MediaPlayer::LVP_Player::SetAudioDevice(device);
}

bool LVP_SetAudioDriver(const std::string &driver)
{
	return MediaPlayer::LVP_Player::SetAudioDriver(driver);
}

void LVP_SetVolume(double percent)
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::SetVolume(percent);
}

void LVP_SetPlaybackSpeed(double speed)
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::SetPlaybackSpeed(speed);
}

void LVP_SetTrack(const LVP_MediaTrack &track)
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	try {
		MediaPlayer::LVP_Player::SetTrack(track);
	} catch (const std::exception &e) {
		MediaPlayer::LVP_Player::CallbackError(e.what());
	}
}

void LVP_Stop()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::Close();
}

void LVP_ToggleMute()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::ToggleMute();
}

void LVP_TogglePause()
{
	if (!isInitialized)
		throw std::exception(ERROR_NO_INIT);

	MediaPlayer::LVP_Player::TogglePause();
}

#if defined _windows
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason) {
        case DLL_PROCESS_ATTACH: // Initialize once for each new process. Return FALSE to fail DLL load.
			break;
        case DLL_PROCESS_DETACH:
            //if (reserved == nullptr) // do not do cleanup if process termination scenario
			break;
        case DLL_THREAD_ATTACH: // Do thread-specific initialization.
            break;
        case DLL_THREAD_DETACH: // Do thread-specific cleanup.
			break;
    }

    return TRUE;
}
#else
int entry()
{
	return 0;
}
#endif
