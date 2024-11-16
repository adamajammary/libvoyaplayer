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
			static LVP_AudioContext*           audioContext;
			static LVP_AudioDevice             audioDevice;
			static LVP_CallbackContext         callbackContext;
			static LibFFmpeg::AVFormatContext* formatContext;
			static LibFFmpeg::AVFormatContext* formatContextExternal;
			static bool                        isStopping;
			static bool                        isOpening;
			static std::mutex                  packetLock;
			static double                      seekPTS;
			static bool                        seekRequested;
			static bool                        seekRequestedBack;
			static bool                        seekRequestedPaused;
			static int                         seekByRequest;
			static double                      seekToRequest;
			static LVP_PlayerState             state;
			static LVP_SubtitleContext*        subContext;
			static System::LVP_TimeOut*        timeOut;
			static bool                        trackRequested;
			static LVP_MediaTrack              trackRequest;
			static LVP_VideoContext*           videoContext;

		public:
			static void                          CallbackError(const std::string& errorMessage);
			static void                          Close();
			static LVP_Strings                   GetAudioDevices();
			static int                           GetAudioTrack();
			static std::vector<LVP_MediaTrack>   GetAudioTracks();
			static std::vector<LVP_MediaChapter> GetChapters();
			static std::string                   GetFilePath();
			static int64_t                       GetDuration();
			static LVP_MediaDetails              GetMediaDetails();
			static LVP_MediaDetails              GetMediaDetails(const std::string& filePath);
			static LVP_MediaType                 GetMediaType();
			static LVP_MediaType                 GetMediaType(const std::string& filePath);
			static LibFFmpeg::AVPixelFormat      GetPixelFormatHardware();
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
			static void                          Quit();
			static void                          Render(const SDL_Rect& destination = {});
			static void                          Resize();
			static void                          SeekBy(int seconds);
			static void                          SeekTo(double percent);
			static bool                          SetAudioDevice(const std::string& device = "");
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
			static void                          closePackets(LVP_MediaContext* context);
			static void                          closePackets();
			static void                          closeStream(LibFFmpeg::AVMediaType streamType);
			static void                          closeSubContext();
			static void                          closeThreads();
			static void                          closeVideoContext();
			static LibFFmpeg::AVSampleFormat     getAudioSampleFormat(SDL_AudioFormat sdlFormat);
			static SDL_AudioFormat               getAudioSampleFormat(LibFFmpeg::AVSampleFormat sampleFormat);
			static std::vector<LVP_MediaTrack>   getAudioTracks(LibFFmpeg::AVFormatContext* formatContext);
			static std::vector<LVP_MediaChapter> getChapters(LibFFmpeg::AVFormatContext* formatContext);
			static std::vector<LVP_MediaTrack>   getMediaTracks(LibFFmpeg::AVMediaType mediaType, LibFFmpeg::AVFormatContext* formatContext, const LVP_Strings& extSubFiles = {});
			static std::vector<LVP_MediaTrack>   getMediaTracksMeta(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVMediaType mediaType, int extSubFileIndex = -1);
			static SDL_Rect                      getScaledVideoDestination(const SDL_Rect& destination);
			static std::vector<LVP_MediaTrack>   getSubtitleTracks(LibFFmpeg::AVFormatContext* formatContext, const LVP_Strings& extSubFiles);
			static std::vector<LVP_MediaTrack>   getVideoTracks(LibFFmpeg::AVFormatContext* formatContext);
			static void                          handleSeek();
			static void                          handleTrack();
			static void                          initAudioFilter();
			static bool                          isHardwarePixelFormat(int frameFormat);
			static bool                          isPacketQueueFull();
			static bool                          isPacketQueueFull(LibFFmpeg::AVMediaType streamType);
			static void                          open();
			static void                          openAudioDevice(const SDL_AudioSpec& wantedSpecs);
			static void                          openFormatContext();
			static void                          openStreams();
			static void                          openSubExternal(int streamIndex = SUB_STREAM_EXTERNAL);
			static void                          openThreads();
			static void                          openThreadAudio();
			static void                          openThreadSub();
			static void                          openThreadVideo();
			static void                          pause();
			static void                          play();
			static void                          renderVideo();
			static void                          seekTo(double percent);
			static void                          stop(const std::string& errorMessage = "");
			static void                          threadAudio(void* userData, Uint8* outputStream, int outputStreamSize);
			static int                           threadPackets();
			static int                           threadSub();
			static int                           threadVideo();
		};
	}
}

#endif
