#ifndef LIBVOYAPLAYER_EVENTS_H
#define LIBVOYAPLAYER_EVENTS_H

#include <functional>
#include <mutex>
#include <unordered_map>

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
	LVP_EVENT_MEDIA_COMPLETED,
	LVP_EVENT_MEDIA_COMPLETED_WITH_ERRORS,
	LVP_EVENT_MEDIA_PAUSED,
	LVP_EVENT_MEDIA_PLAYING,
	LVP_EVENT_MEDIA_STOPPED,
	LVP_EVENT_MEDIA_STOPPING,
	LVP_EVENT_MEDIA_TRACKS_UPDATED,
	LVP_EVENT_METADATA_UPDATED
};

typedef std::function<void(const std::string &errorMessage, const void* data)> LVP_ErrorCallback;

typedef std::function<void(LVP_EventType type, const void* data)> LVP_EventsCallback;

typedef std::function<void(SDL_Surface* videoFrame, const void* data)> LVP_VideoCallback;

struct LVP_CallbackContext
{
	const void*        data     = nullptr;
	LVP_ErrorCallback  errorCB  = nullptr;
	LVP_EventsCallback eventsCB = nullptr;
	LVP_VideoCallback  videoCB  = nullptr;
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
	 * Media type of the track, like video (0), audio (1) or subtitle (3).
	 */
	int mediaType = -1;

	/**
	 * Track index number (position of the track in the media file).
	 */
	int track = -1;

	/**
	 * Track metadata, like title, language etc.
	 */
	std::unordered_map<std::string, std::string> meta;

	/**
	 * Codec specs, like codec_name, bit_rate etc.
	 */
	std::unordered_map<std::string, std::string> codec;
};

struct LVP_MediaMeta
{
	std::vector<LVP_MediaTrack> audioTracks;
	std::vector<LVP_MediaTrack> subtitleTracks;
	std::vector<LVP_MediaTrack> videoTracks;

	std::unordered_map<std::string, std::string> meta;
};

struct LVP_State
{
	int64_t     duration      = 0;
	std::string filePath      = "";
	bool        isMuted       = false;
	bool        isPaused      = false;
	double      playbackSpeed = 0.0;
	int64_t     progress      = 0;
	double      volume        = 0.0;
};

#endif
