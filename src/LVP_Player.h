#ifndef LVP_GLOBAL_H
	#include "LVP_Global.h"
#endif

#ifndef LVP_PLAYER_H
#define LVP_PLAYER_H

#include <libvoyaplayer_events.h>

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		// libavcodec/dvdsubdec.c
		struct LVP_DVDSubContext
		{
			LibFFmpeg::AVClass* av_class;
			uint32_t            palette[16];
			char*               palette_str;
			char*               ifo_str;
			int                 has_palette;
			uint8_t             colormap[4];
			uint8_t             alpha[256];
			uint8_t             buf[0x10000];
			int                 buf_size;
			int                 forced_subs_only;
		};

		struct LVP_PlayerState
		{
			int64_t                duration;
			std::string            filePath;
			size_t                 fileSize;
			bool                   isPaused;
			bool                   isPlaying;
			bool                   isStopped;
			LibFFmpeg::AVMediaType mediaType;
			double                 playbackSpeed;
			double                 progress;
			bool                   quit;
			unsigned int           trackCount;

			LVP_PlayerState()
			{
				this->isStopped = true;
				this->quit      = false;

				this->reset();
			}

			void reset()
			{
				this->duration      = 0;
				this->filePath      = "";
				this->fileSize      = 0;
				this->isPlaying     = false;
				this->isPaused      = false;
				this->mediaType     = LibFFmpeg::AVMEDIA_TYPE_UNKNOWN;
				this->playbackSpeed = 1.0;
				this->progress      = 0.0;
				this->trackCount    = 0;
			}
		};

		struct LVP_RenderContext
		{
			SDL_Renderer*          renderer;
			LibFFmpeg::SwsContext* scaleContextSub;
			LibFFmpeg::SwsContext* scaleContextVideo;
			SDL_Surface*           surface;

			void reset()
			{
				this->renderer          = NULL;
				this->scaleContextSub   = NULL;
				this->scaleContextVideo = NULL;
				this->surface           = NULL;
			}
		};

		class LVP_Player
		{
		private:
			LVP_Player()  {}
			~LVP_Player() {}

		private:
			static LVP_AudioContext            audioContext;
			static std::mutex                  audioFilterLock;
			static LVP_CallbackContext         callbackContext;
			static LibFFmpeg::AVFormatContext* formatContext;
			static LibFFmpeg::AVFormatContext* formatContextExternal;
			static bool                        isStopping;
			static SDL_Thread*                 packetsThread;
			static LVP_RenderContext           renderContext;
			static bool                        seekRequested;
			static bool                        seekRequestedPaused;
			static int64_t                     seekPosition;
			static LVP_PlayerState             state;
			static LVP_SubtitleContext         subContext;
			static bool                        subIsUpdated;
			static System::LVP_TimeOut*        timeOut;
			static LVP_VideoContext            videoContext;

		public:
			static void                          CallbackError(const std::string &errorMessage);
			static void                          Close();
			static Strings                       GetAudioDevices();
			static Strings                       GetAudioDrivers();
			static int                           GetAudioTrack();
			static std::vector<LVP_MediaTrack>   GetAudioTracks();
			static std::vector<LVP_MediaChapter> GetChapters();
			static std::string                   GetFilePath();
			static int64_t                       GetDuration();
			static LVP_MediaMeta                 GetMediaMeta();
			static double                        GetPlaybackSpeed();
			static int64_t                       GetProgress();
			static LVP_State                     GetState();
			static int                           GetSubtitleTrack();
			static std::vector<LVP_MediaTrack>   GetSubtitleTracks();
			static std::vector<LVP_MediaTrack>   GetVideoTracks();
			static double                        GetVolume();
			static void                          Init(const LVP_CallbackContext &callbackContext);
			static bool                          IsMuted();
			static bool                          IsPaused();
			static void                          Open(const std::string &filePath);
			static void                          SeekTo(double percent);
			static bool                          SetAudioDevice(const std::string &device = "");
			static bool                          SetAudioDriver(const std::string &driver);
			static void                          SetPlaybackSpeed(double speed);
			static void                          SetTrack(const LVP_MediaTrack &track);
			static void                          SetVolume(double percent);
			static void                          ToggleMute();
			static void                          TogglePause();

		private:
			static void                        callbackEvents(LVP_EventType type);
			static void                        callbackVideoIsAvailable(SDL_Surface* surface);
			static void                        closeAudio();
			static void                        closePackets();
			static void                        closeStream(LibFFmpeg::AVMediaType streamType);
			static void                        closeSub();
			static void                        closeThreads();
			static void                        closeVideo();
			static LibFFmpeg::AVSampleFormat   getAudioSampleFormat(SDL_AudioFormat sdlFormat);
			static SDL_AudioFormat             getAudioSampleFormat(LibFFmpeg::AVSampleFormat sampleFormat);
			static LVP_FontFaceMap             getFontFaces(LibFFmpeg::AVFormatContext* formatContext);
			static std::vector<LVP_MediaTrack> getMediaTracks(LibFFmpeg::AVMediaType mediaType);
			static std::vector<LVP_MediaTrack> getMediaTracksMeta(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVMediaType mediaType, bool isSubsExternal = false);
			static SDL_YUV_CONVERSION_MODE     getSdlYuvConversionMode(LibFFmpeg::AVFrame* frame);
			static uint32_t                    getVideoPixelFormat(LibFFmpeg::AVPixelFormat pixelFormat);
			static LibFFmpeg::AVPixelFormat    getVideoPixelFormat(uint32_t pixelFormat);
			static void                        handleSeek();
			static bool                        isPacketQueueFull();
			static bool                        isPacketQueueFull(LibFFmpeg::AVMediaType streamType);
			static bool                        isYUV(LibFFmpeg::AVPixelFormat pixelFormat);
			static void                        openAudioDevice(const SDL_AudioSpec &wantedSpecs);
			static void                        openFormatContext(const std::string &filePath);
			static void                        openStreams();
			static void                        openSubExternal(int streamIndex);
			static void                        openThreads();
			static void                        openThreadAudio();
			static void                        openThreadAudioFilter();
			static void                        openThreadSub();
			static void                        openThreadVideo();
			static void                        packetAdd(LibFFmpeg::AVPacket* packet, LVP_MediaContext &mediaContext);
			static LibFFmpeg::AVPacket*        packetGet(LVP_MediaContext &mediaContext);
			static void                        packetsClear(LVP_MediaContext &mediaContext);
			static void                        pause();
			static void                        play();
			static void                        renderSubs();
			static void                        renderSubsBitmap();
			static void                        renderSubsText();
			static void                        renderVideo();
			static void                        renderVideoAndSubs();
			static void                        reset();
			static void                        seekToPosition(int64_t position);
			static void                        stop(const std::string &errorMessage = "");
			static void                        threadAudio(void* userData, Uint8* outputStream, int outputStreamSize);
			static int                         threadPackets(void* userData);
			static int                         threadSub(void* userData);
			static int                         threadVideo(void* userData);

		};
	}
}

#endif
