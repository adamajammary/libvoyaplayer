#ifndef LIBVOYAPLAYER_EVENTS_H
#define LIBVOYAPLAYER_EVENTS_H

#include <functional>
#include <map>
#include <mutex>

#ifndef LIB_SDL2_H
#define LIB_SDL2_H
	extern "C" {
		#include <SDL2/SDL.h>
	}
#endif

enum LVP_EventType
{
	LVP_EVENT_AUDIO_MUTED,
	LVP_EVENT_AUDIO_UNMUTED,
	LVP_EVENT_AUDIO_VOLUME_CHANGED,
	LVP_EVENT_MEDIA_COMPLETED,
	LVP_EVENT_MEDIA_COMPLETED_WITH_ERRORS,
	LVP_EVENT_MEDIA_OPENED,
	LVP_EVENT_MEDIA_PAUSED,
	LVP_EVENT_MEDIA_PLAYING,
	LVP_EVENT_MEDIA_STOPPED,
	LVP_EVENT_MEDIA_STOPPING,
	LVP_EVENT_MEDIA_TRACKS_UPDATED,
	LVP_EVENT_METADATA_UPDATED,
	LVP_EVENT_PLAYBACK_SPEED_CHANGED
};

enum LVP_MediaType
{
	LVP_MEDIA_TYPE_UNKNOWN = -1,
	LVP_MEDIA_TYPE_VIDEO = 0,
	LVP_MEDIA_TYPE_AUDIO = 1,
	LVP_MEDIA_TYPE_SUBTITLE = 3
};

typedef std::function<void(const std::string &errorMessage, const void* data)> LVP_ErrorCallback;

typedef std::function<void(LVP_EventType type, const void* data)> LVP_EventsCallback;

typedef std::function<void(SDL_Surface* videoFrame, const void* data)> LVP_VideoCallback;

struct LVP_CallbackContext
{
	/**
	 * @brief Called every time an error occurs.
	 */
	LVP_ErrorCallback errorCB  = nullptr;

	/**
	 * @brief Called every time an event of type LVP_EventType occurs.
	 */
	LVP_EventsCallback eventsCB = nullptr;

	/**
	 * @brief Called every time a new video frame is available.
	 */
	LVP_VideoCallback videoCB  = nullptr;

	/**
	 * @brief Custom data context, will be available in all callbacks.
	 */
	const void* data = nullptr;

	/**
	 * @brief Use an existing SDL hardware renderer to process the video frames,
	 *        otherwise software rendering will be used.
	 */
	SDL_Renderer* hardwareRenderer = nullptr;
};

struct LVP_MediaChapter
{
	std::string title = "";

	/**
	 * @brief Chapter start time in milliseconds (one thousandth of a second).
	 */
	int64_t startTime = 0;

	/**
	 * @brief Chapter end time in milliseconds (one thousandth of a second).
	 */
	int64_t endTime = 0;
};

struct LVP_MediaTrack
{
	/**
	 * @brief Media type of the track, like video (0), audio (1) or subtitle (3).
	 */
	LVP_MediaType mediaType = LVP_MEDIA_TYPE_UNKNOWN;

	/**
	 * @brief Track index number (position of the track in the media file).
	 */
	int track = -1;

	/**
	 * @brief Track metadata, like title, language etc.
	 */
	std::map<std::string, std::string> meta;

	/**
	 * @brief Codec specs, like codec_name, bit_rate etc.
	 */
	std::map<std::string, std::string> codec;
};

struct LVP_MediaMeta
{
	std::vector<LVP_MediaTrack> audioTracks;
	std::vector<LVP_MediaTrack> subtitleTracks;
	std::vector<LVP_MediaTrack> videoTracks;

	std::map<std::string, std::string> meta;
};

#endif
