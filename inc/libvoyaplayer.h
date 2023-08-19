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
 * @throws exception
 */
DLLEXPORT void DLL LVP_Initialize(const LVP_CallbackContext &callbackContext);

/**
 * @returns a list of available audio devices.
 */
DLLEXPORT std::vector<std::string> DLL LVP_GetAudioDevices();

/**
 * @returns a list of chapters in the currently loaded media.
 * @throws exception
 */
DLLEXPORT std::vector<LVP_MediaChapter> DLL LVP_GetChapters();

/**
 * @returns the current audio track index number.
 * @throws exception
 */
DLLEXPORT int DLL LVP_GetAudioTrack();

/**
 * @returns a list of audio tracks in the currently loaded media.
 * @throws exception
 */
DLLEXPORT std::vector<LVP_MediaTrack> DLL LVP_GetAudioTracks();

/**
 * @returns the media duration in milliseconds (one thousandth of a second).
 * @throws exception
*/
DLLEXPORT int64_t DLL LVP_GetDuration();

/**
 * @returns the current media file path.
 * @throws exception
*/
DLLEXPORT std::string DLL LVP_GetFilePath();

/**
 * @returns media details of the currently loaded media.
 * @throws exception
 */
DLLEXPORT LVP_MediaDetails DLL LVP_GetMediaDetails();

/**
 * @returns media details of the provided media file.
 * @param filePath Full path to the media file.
 * @throws exception
 */
DLLEXPORT LVP_MediaDetails DLL LVP_GetMediaDetails(const std::string& filePath);

/**
 * @returns media details of the provided media file.
 * @param filePath Full path to the media file.
 * @throws exception
 */
DLLEXPORT LVP_MediaDetails DLL LVP_GetMediaDetails(const std::wstring& filePath);

/**
 * @returns the media type of the currently loaded media.
 * @throws exception
 */
DLLEXPORT LVP_MediaType DLL LVP_GetMediaType();

/**
 * @returns the media type of the provided media file.
 * @param filePath Full path to the media file.
 * @throws exception
 */
DLLEXPORT LVP_MediaType DLL LVP_GetMediaType(const std::string& filePath);

/**
 * @returns the media type of the provided media file.
 * @param filePath Full path to the media file.
 * @throws exception
 */
DLLEXPORT LVP_MediaType DLL LVP_GetMediaType(const std::wstring& filePath);

/**
 * @returns the current playback speed as a percent between 0.5 and 2.0.
 * @throws exception
 */
DLLEXPORT double DLL LVP_GetPlaybackSpeed();

/**
 * @returns the media playback progress in milliseconds (one thousandth of a second).
 * @throws exception
*/
DLLEXPORT int64_t DLL LVP_GetProgress();

/**
 * @returns the current subtitle track index number.
 * @throws exception
 */
DLLEXPORT int DLL LVP_GetSubtitleTrack();

/**
 * @returns a list of subtitle tracks in the currently loaded media.
 * @throws exception
 */
DLLEXPORT std::vector<LVP_MediaTrack> DLL LVP_GetSubtitleTracks();

/**
 * @returns a list of video tracks in the currently loaded media.
 * @throws exception
 */
DLLEXPORT std::vector<LVP_MediaTrack> DLL LVP_GetVideoTracks();

/**
 * @returns the current audio volume as a percent between 0 and 1.
 * @throws exception
 */
DLLEXPORT double DLL LVP_GetVolume();

/**
 * @returns true if audio volume is muted.
 * @throws exception
 */
DLLEXPORT bool DLL LVP_IsMuted();

/**
 * @returns true if playback is paused.
 * @throws exception
 */
DLLEXPORT bool DLL LVP_IsPaused();

/**
 * @returns true if playback is playing (not paused and not stopped).
 * @throws exception
 */
DLLEXPORT bool DLL LVP_IsPlaying();

/**
 * @returns true if playback is stopped.
 * @throws exception
 */
DLLEXPORT bool DLL LVP_IsStopped();

/**
 * @brief Tries to open and play the given media file.
 * @param filePath Full path to the media file.
 * @throws exception
 */
DLLEXPORT void DLL LVP_Open(const std::string &filePath);

/**
 * @brief Tries to open and play the given media file.
 * @param filePath Full path to the media file.
 * @throws exception
 */
DLLEXPORT void DLL LVP_Open(const std::wstring &filePath);

/**
 * @brief Cleans up allocated resources.
 */
DLLEXPORT void DLL LVP_Quit();

/**
 * @brief Generates and renders a video frame.
 *        If hardware rendering is used, it will copy the texture to the renderer.
 *        If software rendering is used, it will generate a LVP_VideoCallback with an SDL_Surface.
 * @param destination Optional clipping/scaling region used by the hardware renderer.
 */
DLLEXPORT void DLL LVP_Render(const SDL_Rect* destination = nullptr);

/**
 * @brief Should be called whenever the window resizes to tell the player to recreate the video frame context.
 */
DLLEXPORT void DLL LVP_Resize();

/**
 * @brief Seeks to the given position as a percent between 0 and 1.
 * @param percent [0.0-1.0]
 * @throws exception
 */
DLLEXPORT void DLL LVP_SeekTo(double percent);

/**
 * @brief Tries to set the given audio device as the current device if valid.
 * @param device Name of the audio device.
 * @returns true if the audio device is successfully set.
 */
DLLEXPORT bool DLL LVP_SetAudioDevice(const std::string &device);

/**
 * @brief Mutes/unmutes the audio volume.
 * @param muted true to mute or false to unmute
 * @throws exception
 */
DLLEXPORT void DLL LVP_SetMuted(bool muted);

/**
 * @brief Sets the given playback speed as a relative percent between 0.5 and 2.0, where 1.0 is normal/default.
 * @param speed [0.5-2.0]
 * @throws exception
 */
DLLEXPORT void DLL LVP_SetPlaybackSpeed(double speed);

/**
 * @brief Tries to set the given stream as the current stream if valid.
 * @param track -1 to disable subtitles or >= 0 for a valid media track.
 * @throws exception
 */
DLLEXPORT void DLL LVP_SetTrack(const LVP_MediaTrack &track);

/**
 * @brief Sets the given audio volume as a percent between 0 and 1.
 * @param percent [0.0-1.0]
 * @throws exception
 */
DLLEXPORT void DLL LVP_SetVolume(double percent);

/**
 * @brief Stops playback of the currently loaded media.
 * @throws exception
 */
DLLEXPORT void DLL LVP_Stop();

/**
 * @brief Toggles muting audio volume on/off.
 * @throws exception
 */
DLLEXPORT void DLL LVP_ToggleMute();

/**
 * @brief Toggles between pausing and playing.
 * @throws exception
 */
DLLEXPORT void DLL LVP_TogglePause();

#endif
