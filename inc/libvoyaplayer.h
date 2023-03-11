#ifndef LIBVOYAPLAYER_H
#define LIBVOYAPLAYER_H

#define DLL __stdcall

#ifdef libvoyaplayer_EXPORTS
	#define DLLEXPORT __declspec(dllexport)
#else
	#define DLLEXPORT __declspec(dllimport)
#endif

#include <libvoyaplayer_events.h>

/**
 * @brief Tries to initialize the library and other dependencies.
 * @param videoCB  Called every time a new video frame is available.
 * @param errorCB  Called every time an error occurs.
 * @param eventsCB Called every time an event of type LVP_EventType occurs.
 * @param data     Custom data context, will be available in all callbacks.
 */
DLLEXPORT void DLL LVP_Initialize(LVP_VideoCallback videoCB, LVP_ErrorCallback errorCB, LVP_EventsCallback eventsCB, const void* data = nullptr);

/**
 * @returns a list of available audio devices.
 */
DLLEXPORT std::vector<std::string> DLL LVP_GetAudioDevices();

/**
 * @returns a list of available audio drivers.
 */
DLLEXPORT std::vector<std::string> DLL LVP_GetAudioDrivers();

/**
 * @returns a list of chapters in the currently loaded media.
 */
DLLEXPORT std::vector<LVP_MediaChapter> DLL LVP_GetChapters();

/**
 * @returns the current audio track index number.
 */
DLLEXPORT int DLL LVP_GetAudioTrack();

/**
 * @returns a list of audio tracks in the currently loaded media.
 */
DLLEXPORT std::vector<LVP_MediaTrack> DLL LVP_GetAudioTracks();

/**
 * @returns the media duration as milliseconds (one thousandth of a second).
*/
DLLEXPORT int64_t DLL LVP_GetDuration();

/**
 * @returns the current media file path.
*/
DLLEXPORT std::string DLL LVP_GetFilePath();

/**
 * @returns metadata for the currently loaded media including all tracks.
 */
DLLEXPORT LVP_MediaMeta DLL LVP_GetMediaMeta();

/**
 * @returns the current playback speed as a percent between 0.5 and 2.0.
 */
DLLEXPORT double DLL LVP_GetPlaybackSpeed();

/**
 * @returns the media playback progress as milliseconds (one thousandth of a second).
*/
DLLEXPORT int64_t DLL LVP_GetProgress();

/**
 * @returns the current state of the player.
 */
DLLEXPORT LVP_State DLL LVP_GetState();

/**
 * @returns the current subtitle track index number.
 */
DLLEXPORT int DLL LVP_GetSubtitleTrack();

/**
 * @returns a list of subtitle tracks in the currently loaded media.
 */
DLLEXPORT std::vector<LVP_MediaTrack> DLL LVP_GetSubtitleTracks();

/**
 * @returns a list of video tracks in the currently loaded media.
 */
DLLEXPORT std::vector<LVP_MediaTrack> DLL LVP_GetVideoTracks();

/**
 * @returns the current audio volume as a percent between 0 and 1.
 */
DLLEXPORT double DLL LVP_GetVolume();

/**
 * @returns true if audio volume is muted.
 */
DLLEXPORT bool DLL LVP_IsMuted();

/**
 * @returns true if playback is paused.
 */
DLLEXPORT bool DLL LVP_IsPaused();

/**
 * @brief Tries to open and play the given media file.
 * @param filePath Full path to the media file.
 */
DLLEXPORT void DLL LVP_Open(const std::string &filePath);

/**
 * @brief Tries to open and play the given media file.
 * @param filePath Full path to the media file.
 */
DLLEXPORT void DLL LVP_Open(const std::wstring &filePath);

/**
 * @brief Cleans up allocated resources.
 */
DLLEXPORT void DLL LVP_Quit();

/**
 * @brief Seeks to the given position as a percent between 0 and 1.
 * @param percent [0.0-1.0]
 */
DLLEXPORT void DLL LVP_SeekTo(double percent);

/**
 * @brief Tries to set the given audio device as the current device if valid.
 * @param device Name of the audio device.
 * @returns true if the audio device is successfully set.
 */
DLLEXPORT bool DLL LVP_SetAudioDevice(const std::string &device);

/**
 * @brief Tries to set the given audio driver as the current output if valid.
 * @param device Name of the audio driver.
 * @returns true if the audio driver is successfully set.
 */
DLLEXPORT bool DLL LVP_SetAudioDriver(const std::string &driver);

/**
 * @brief Sets the given playback speed as a relative percent between 0.5 and 2.0, where 1.0 is normal/default.
 * @param speed [0.5-2.0]
 */
DLLEXPORT void DLL LVP_SetPlaybackSpeed(double speed);

/**
 * @brief Tries to set the given stream as the current stream if valid.
 * @param LVP_MediaTrack with track of -1 to disable subtitles, or >= 0 for a valid media track.
 */
DLLEXPORT void DLL LVP_SetTrack(const LVP_MediaTrack &track);

/**
 * @brief Sets the given audio volume as a percent between 0 and 1.
 * @param percent [0.0-1.0]
 */
DLLEXPORT void DLL LVP_SetVolume(double percent);

/**
 * @brief Stops playback of the currently loaded media.
 */
DLLEXPORT void DLL LVP_Stop();

/**
 * @brief Toggles muting audio volume on/off.
 */
DLLEXPORT void DLL LVP_ToggleMute();

/**
 * @brief Toggles between pausing and playing.
 */
DLLEXPORT void DLL LVP_TogglePause();

#endif
