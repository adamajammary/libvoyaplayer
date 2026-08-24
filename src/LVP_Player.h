#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_PLAYER_H
#define LVP_PLAYER_H

#include <libvoyaplayer_events.h>

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		class LVP_Player
		{
		private:
			LVP_Player()  {}
			~LVP_Player() {}

		private:
			static LVP_AudioContext*          audioContext;
			static LVP_CallbackContext        callbackContext;
			static AVFormatContext*           formatContext;
			static AVFormatContext*           formatContextExternal;
			static bool                       isStopping;
			static bool                       isOpening;
			static std::mutex                 packetLock;
			static double                     seekPTS;
			static bool                       seekRequested;
			static bool                       seekRequestedBack;
			static bool                       seekRequestedPaused;
			static int                        seekByRequest;
			static double                     seekToRequest;
			static LVP_PlayerState            state;
			static LVP_SubtitleContext*       subContext;
			static System::LVP_TimeOut*       timeOut;
			static bool                       trackRequested;
			static std::queue<LVP_MediaTrack> trackRequests;
			static LVP_VideoContext*          videoContext;

		public:
			static void                          AddAudioDevice(SDL_AudioDeviceID id);
			static void                          CallbackError(const std::string& errorMessage);
			static void                          Close();
			static std::string                   GetAudioDevice();
			static LVP_Strings                   GetAudioDevices();
			static int                           GetAudioTrack();
			static std::vector<LVP_MediaTrack>   GetAudioTracks();
			static std::vector<LVP_MediaChapter> GetChapters();
			static std::string                   GetFilePath();
			static int64_t                       GetDuration();
			static LVP_MediaDetails              GetMediaDetails(bool skipThumbnail);
			static LVP_MediaDetails              GetMediaDetails(const std::string& filePath, bool skipThumbnail);
			static LVP_MediaMeta                 GetMediaMeta();
			static LVP_MediaMeta                 GetMediaMeta(const std::string& filePath);
			static SDL_Surface*                  GetMediaThumbnail();
			static SDL_Surface*                  GetMediaThumbnail(const std::string& filePath);
			static LVP_MediaType                 GetMediaType();
			static LVP_MediaType                 GetMediaType(const std::string& filePath);
			static AVPixelFormat                 GetPixelFormatHardware();
			static double                        GetPlaybackSpeed();
			static int64_t                       GetProgress();
			static int                           GetSubtitleTrack();
			static std::vector<LVP_MediaTrack>   GetSubtitleTracks();
			static std::vector<LVP_MediaTrack>   GetVideoTracks();
			static double                        GetVolume();
			static void                          Init(const LVP_CallbackContext& callbackContext);
			static bool                          IsMuted();
			static bool                          IsPaused();
			static bool                          IsPlaying();
			static bool                          IsStopped();
			static void                          Open(const std::string& filePath);
			static void                          Pause();
			static void                          Play();
			static void                          Quit();
			static void                          RemoveAudioDevice(SDL_AudioDeviceID id);
			static void                          Resize();
			static void                          Run(const SDL_Rect& destination = {});
			static void                          SeekBy(int seconds);
			static void                          SeekTo(double percent);
			static void                          SetAudioDevice(const std::string& name);
			static void                          SetMuted(bool muted = true);
			static void                          SetPlaybackSpeed(double speed);
			static void                          SetTrack(const LVP_MediaTrack& track);
			static void                          SetVolume(double percent);
			static void                          ToggleMute();
			static void                          TogglePause();

		private:
			static void                          callbackEvents(LVP_EventType type);
			static void                          callbackVideoIsAvailable(SDL_Surface* surface);
			static void                          close();
			static void                          closeAudioContext();
			static void                          closeAudioStream();
			static void                          closePackets(LVP_MediaContext* context);
			static void                          closePackets();
			static void                          closeStream(AVMediaType streamType);
			static void                          closeStream(LVP_MediaContext* mediaContext);
			static void                          closeSubContext();
			static void                          closeVideoContext();
			static int                           decodeAudioFrame();
			static void                          decodeAudioFrames();
			static void                          decodeAudioPacket(AVPacket* packet);
			static AVFrame*                      getAudioFrame();
			static std::vector<LVP_MediaTrack>   getAudioTracks(AVFormatContext* formatContext);
			static std::vector<LVP_MediaChapter> getChapters(AVFormatContext* formatContext);
			static AVPacket*                     getMediaPacket(LVP_MediaContext* context);
			static std::vector<LVP_MediaTrack>   getMediaTracks(AVMediaType mediaType, AVFormatContext* formatContext, const LVP_Strings& extSubFiles = {});
			static std::vector<LVP_MediaTrack>   getMediaTracksMeta(AVFormatContext* formatContext, AVMediaType mediaType, int extSubFileIndex = -1);
			static SDL_Rect                      getScaledVideoDestination(const SDL_Rect& destination);
			static std::vector<LVP_MediaTrack>   getSubtitleTracks(AVFormatContext* formatContext, const LVP_Strings& extSubFiles);
			static std::vector<LVP_MediaTrack>   getVideoTracks(AVFormatContext* formatContext);
			static void                          handleSeek();
			static void                          handleTrack();
			static void                          initAudioFilter(AVFrame* frame);
			static bool                          isHardwarePixelFormat(int frameFormat);
			static bool                          isPacketQueueFull();
			static bool                          isPacketQueueFull(AVMediaType streamType);
			static void                          open();
			static void                          openAudioDevice(const std::string& name);
			static void                          openFormatContext();
			static void                          openStreams();
			static void                          openSubExternal(int streamIndex = SUB_STREAM_EXTERNAL);
			static void                          openThreads();
			static void                          openThreadAudio();
			static void                          openThreadSub();
			static void                          openThreadVideo();
			static void                          renderVideo();
			static void                          seekTo(double percent);
			static void                          setAudioDevices();
			static void                          setAudioPacketDuration(AVPacket* packet);
			static void                          setAudioProgress(AVFrame* frame);
			static void                          stop(const std::string& errorMessage = "");
			static int                           threadAudio();
			static void                          threadAudioCallback(void* userData, SDL_AudioStream* stream, int streamSize, int total_amount);
			static int                           threadPackets();
			static int                           threadSub();
			static int                           threadVideo();
		};
	}
}

#endif
