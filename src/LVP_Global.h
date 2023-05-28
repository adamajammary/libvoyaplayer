#ifndef LVP_GLOBAL_H
#define LVP_GLOBAL_H

#include <chrono>
#include <format>
#include <list>
#include <map>
#include <queue>
#include <sstream> // stringstream, to_string(x)
#include <thread>
#include <unordered_map>

extern "C"
{
	#include <SDL2/SDL_ttf.h>
}

#if defined _android
	#include <dirent.h> // mkdir(x), opendir(x)
	#include <unistd.h> // chdir(x)
#elif defined _ios
	#include <dirent.h> // mkdir(x),  opendir(x)
#elif defined _linux
	#include <dirent.h> // opendir(x)
#elif defined _macosx
	#include <sys/dir.h> // opendir(x)
#elif defined _windows
	#include <direct.h> // _chdir(x), mkdir(x)
	#include <dirent.h> // opendir(x)
#endif

namespace LibFFmpeg
{
	extern "C"
	{
		#include <libavcodec/avcodec.h>
		#include <libavfilter/avfilter.h>
		#include <libavfilter/buffersink.h>
		#include <libavfilter/buffersrc.h>
		#include <libavformat/avformat.h>
		#include <libavutil/imgutils.h>
		#include <libavutil/opt.h>
		#include <libavutil/time.h>
		#include <libswresample/swresample.h>
		#include <libswscale/swscale.h>
	}
}

namespace LibVoyaPlayer
{
	#if defined _android
		#define fstat       lstat64
		#define stat_t      struct stat64
		#define fseek       fseeko
		#define LOG(x, ...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, x, ##__VA_ARGS__);
	#elif defined _ios
		#define fstat       lstat
		#define stat_t      struct stat
		#define fseek       fseeko
		#define LOG(x, ...) os_log_error(OS_LOG_DEFAULT, x, ##__VA_ARGS__);
	#elif defined _macosx
		#define fstat       lstat64
		#define stat_t      struct stat64
		#define fseek       fseeko
		#define LOG(x, ...) NSLog(@x, ##__VA_ARGS__);
	#elif defined _linux
		#define fstat       lstat64
		#define stat_t      struct stat64
		#define fopen       fopen64
		#define fseek       fseeko64
		#define LOG(x, ...) fprintf(stderr, x, ##__VA_ARGS__);
	#elif defined _windows
		#define chdir       _chdir
		#define dirent      _wdirent
		#define DIR         _WDIR
		#define closedir    _wclosedir
		#define opendir     _wopendir
		#define readdir     _wreaddir
		#define fstat       _stat64
		#define fstatw      _wstat64
		#define stat_t      struct _stat64
		#define fseek       _fseeki64
		#define LOG(x, ...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, x, ##__VA_ARGS__);
	#endif

	#define DELETE_POINTER(p) if (p != NULL) { delete p; p = NULL; }

	#define FREE_AVFORMAT(f) if (f != NULL) { LibFFmpeg::avformat_close_input(&f); LibFFmpeg::avformat_free_context(f); f = NULL; }
	#define FREE_POINTER(p)  if (p != NULL) { free(p); p = NULL; }

	#define LVP_COLOR_BLACK Graphics::LVP_Color(0, 0, 0, 0xFF)
    #define LVP_COLOR_WHITE Graphics::LVP_Color(0xFF, 0xFF, 0xFF, 0xFF)

	const int64_t AV_TIME_BASE_I64 = 1000000ll;
	const double  AV_TIME_BASE_D   = 1000000.0;

	const int MEGA_BYTE = 1024000;

	const int DEFAULT_CHAR_BUFFER_SIZE = 1024;

	typedef std::vector<std::string>  Strings;
	typedef std::vector<uint16_t*>    Strings16;
	typedef std::vector<std::wstring> WStrings;

	namespace Graphics
	{
		#define FREE_TEXTURE(t) if (t != NULL) { SDL_DestroyTexture(t); t = NULL; }

		class LVP_SubTexture;
		class LVP_Texture;

		typedef std::vector<LVP_SubTexture*>                LVP_SubTextures;
		typedef std::pair<size_t, LVP_SubTextures>          LVP_SubTextureId;
		typedef std::unordered_map<size_t, LVP_SubTextures> LVP_SubTexturesId;

		struct LVP_Border
		{
			int bottom, left, right, top;

			LVP_Border()
			{
				this->bottom = this->left = this->right = this->top = 0;
			}

			LVP_Border(int borderWidth)
			{
				this->bottom = this->left = this->right = this->top = borderWidth;
			}
		};
	}

	namespace MediaPlayer
	{
		#define ALIGN_CENTER(o, ts, s)      max(0, ((o) + (max(0, ((ts) - (s))) / 2)))
		#define ARE_DIFFERENT_DOUBLES(a, b) ((a > (b + 0.01)) || (a < (b - 0.01)))
		#define HEX_STR_TO_UINT(h)          (uint8_t)std::strtoul(std::string("0x" + h).c_str(), NULL, 16)

		#define AV_SEEK_FLAGS(i)    (((i->flags & AVFMT_TS_DISCONT) || !i->read_seek) ? AVSEEK_FLAG_BYTE : 0)
		#define AV_SEEK_BYTES(i, s) ((AV_SEEK_FLAGS(i) == AVSEEK_FLAG_BYTE) && (s > 0))
		#define AVFRAME_IS_VALID(f) ((f != NULL) && (f->data[0] != NULL) && (f->linesize[0] > 0) && (f->width > 0) && (f->height > 0))

		#define FREE_AVCODEC(c)        if (c != NULL) { LibFFmpeg::avcodec_free_context(&c); c = NULL; }
		#define FREE_AVDICT(d)         if (d != NULL) { LibFFmpeg::av_dict_free(&d); d = NULL; }
		#define FREE_AVFILTER_GRAPH(g) if (g != NULL) { LibFFmpeg::avfilter_graph_free(&g); }
		#define FREE_AVFRAME(f)        if (f != NULL) { LibFFmpeg::av_frame_free(&f); f = NULL; }
		#define FREE_AVPOINTER(p)      if (p != NULL) { LibFFmpeg::av_free(p); p = NULL; }
		#define FREE_AVPACKET(p)       if (p != NULL) { LibFFmpeg::av_packet_free(&p); LibFFmpeg::av_free(p); p = NULL; }
		#define FREE_AVSTREAM(s)       if (s != NULL) { s->discard = LibFFmpeg::AVDISCARD_ALL; s = NULL; }
		#define FREE_RENDERER(r)       if (r != NULL) { SDL_DestroyRenderer(r); r = NULL; }
		#define FREE_SWR(s)            if (s != NULL) { LibFFmpeg::swr_free(&s); s = NULL; }
		#define FREE_SWS(s)            if (s != NULL) { LibFFmpeg::sws_freeContext(s); s = NULL; }
		#define FREE_SUB_FRAME(f)      if (f.num_rects > 0) { LibFFmpeg::avsubtitle_free(&f); f = { 0, 0, 0, 0 }; }
		#define FREE_SUB_TEXT(t)       if (t != NULL) { LibFFmpeg::av_freep(&t); }
		#define FREE_SURFACE(s)        if (s != NULL) { SDL_FreeSurface(s); s = NULL; }
		#define FREE_THREAD(t)         if (t != NULL) { SDL_DetachThread(t); t = NULL; }
		#define FREE_THREAD_COND(c)    if (c != NULL) { SDL_DestroyCond(c);c = NULL; }
		#define FREE_THREAD_MUTEX(m)   if (m != NULL) { SDL_DestroyMutex(m); m = NULL; }

		#define IS_ATTACHMENT(t)    (t == LibFFmpeg::AVMEDIA_TYPE_ATTACHMENT)
		#define IS_AUDIO(t)         (t == LibFFmpeg::AVMEDIA_TYPE_AUDIO)
		#define IS_SUB(t)           (t == LibFFmpeg::AVMEDIA_TYPE_SUBTITLE)
		#define IS_SUB_TEXT(t)      ((t == LibFFmpeg::SUBTITLE_ASS) || (t == LibFFmpeg::SUBTITLE_TEXT))
		#define IS_VALID_TEXTURE(t) ((t != NULL) && (t->data != NULL))
		#define IS_VIDEO(t)         (t == LibFFmpeg::AVMEDIA_TYPE_VIDEO)

		#if defined _windows
			#define FONT_NAME(f, s) std::format(L"{}_{}", f, s)
			#define OPEN_FONT(f, s) TTF_OpenFontRW(System::LVP_FileSystem::FileOpenSDLRWops(_wfopen(f, L"rb")), 1, s)
		#else
			#define FONT_NAME(f, s) std::format("{}_{}", f, s)
			#define OPEN_FONT(f, s) TTF_OpenFont(f, s)
		#endif

		#define CLOSE_FONT(f) if (f != NULL) { TTF_CloseFont(f); f = NULL; }

		const int AUDIO_BUFFER_SIZE = 768000;

		const int DEFAULT_FONT_SIZE    = 16;
		const int DEFAULT_SCALE_FILTER = SWS_LANCZOS;

		const int    DELAY_TIME_BACKGROUND = 200;
		const int    DELAY_TIME_DEFAULT    = 15;
		const int    DELAY_TIME_ONE_MS     = 1;
		
		const double FONT_DPI_SCALE = (76.0 / 96.0);

		#if defined _windows
			const wchar_t FONT_ARIAL[] = L"Arial";
		#else
			const char FONT_ARIAL[] = "Arial";
		#endif

		const int    MAX_ERRORS          = 100;
		const int    MAX_FONT_SIZE       = 200;
		const double MAX_SUB_DURATION    = 20.0;
		const int    MAX_SUB_DURATION_MS = 20000;

		const float MIN_FLOAT_ZERO        = 0.01f;
		const int   MIN_PACKET_QUEUE_SIZE = 25;
		const int   MIN_SUB_PACKET_SIZE   = 100;

		const int ONE_SECOND_MS = 1000;

		const auto     SUB_PIXEL_FORMAT_FFMPEG = LibFFmpeg::AV_PIX_FMT_BGRA;
		const uint32_t SUB_PIXEL_FORMAT_SDL    = SDL_PIXELFORMAT_ARGB8888;
		const int      SUB_STREAM_EXTERNAL     = 1000;

		class LVP_Subtitle;
		class LVP_SubStyle;

		struct LVP_FontFace {
			uint32_t    streamIndex;
			std::string style;
		};
		
		typedef std::vector<LVP_FontFace>         LVP_FontFaces;
		typedef std::queue<LibFFmpeg::AVPacket*>  LVP_Packets;
		typedef std::list<LVP_Subtitle*>          LVP_Subtitles;
		typedef std::vector<LVP_SubStyle*>        LVP_SubStyles;

		#if defined _windows
			typedef std::unordered_map<std::wstring, TTF_Font*>     LVP_FontMap;
			typedef std::unordered_map<std::wstring, LVP_FontFaces> LVP_FontFaceMap;
		#else
			typedef std::unordered_map<std::string, TTF_Font*>     LVP_FontMap;
			typedef std::unordered_map<std::string, LVP_FontFaces> LVP_FontFaceMap;
		#endif

		struct LVP_AudioFilter
		{
			LibFFmpeg::AVFilterContext* bufferSink;
			LibFFmpeg::AVFilterContext* bufferSource;
			LibFFmpeg::AVFilterGraph*   filterGraph;

			LVP_AudioFilter()
			{
				this->reset();
			}

			void reset()
			{
				this->bufferSink   = NULL;
				this->bufferSource = NULL;
				this->filterGraph  = NULL;
			}
		};

		struct LVP_MediaContext
		{
			LibFFmpeg::AVCodecContext* codec;
			SDL_cond*                  condition;
			int                        index;
			SDL_mutex*                 mutex;
			LVP_Packets                packets;
			bool                       packetsAvailable;
			LibFFmpeg::AVPixelFormat   pixelFormatHardware;
			LibFFmpeg::AVStream*       stream;

			LVP_MediaContext()
			{
				this->reset();
			}

			void reset()
			{
				this->codec               = NULL;
				this->condition           = NULL;
				this->index               = -1;
				this->mutex               = NULL;
				this->packetsAvailable    = true;
				this->pixelFormatHardware = LibFFmpeg::AV_PIX_FMT_NONE;
				this->stream              = NULL;
			}
		};

		struct LVP_AudioContext : LVP_MediaContext
		{
			int                    bufferOffset;
			int                    bufferRemaining;
			int                    bufferSize;
			bool                   decodeFrame;
			std::string            device;
			SDL_AudioDeviceID      deviceID;
			SDL_AudioSpec          deviceSpecs;
			SDL_AudioSpec          deviceSpecsWanted;
			LVP_AudioFilter        filter;
			LibFFmpeg::AVFrame*    frame;
			double                 frameDuration;
			uint8_t*               frameEncoded;
			bool                   isMuted;
			bool                   isDeviceReady;
			bool                   isDriverReady;
			double                 lastPogress;
			LibFFmpeg::SwrContext* resampleContext;
			int                    volume;
			int                    writtenToStream;

			LVP_AudioContext()
			{
				this->reset();
			}

			void reset()
			{
				this->bufferOffset      = 0;
				this->bufferRemaining   = 0;
				this->bufferSize        = AUDIO_BUFFER_SIZE;
				this->decodeFrame       = true;
				this->device            = "";
				this->deviceID          = 0;
				this->deviceSpecs       = {};
				this->deviceSpecsWanted = {};
				this->filter            = {};
				this->frame             = NULL;
				this->frameDuration     = 0.0;
				this->frameEncoded      = NULL;
				this->isDeviceReady     = true;
				this->isDriverReady     = true;
				this->lastPogress       = 0.0;
				this->resampleContext   = NULL;
				this->writtenToStream   = 0;
			}
		};

		struct LVP_SubPTS
		{
			double start, end;

			LVP_SubPTS()
			{
				this->start = 0.0;
				this->end   = 0.0;
			}

			LVP_SubPTS(double start, double end)
			{
				this->start = start;
				this->end   = end;
			}

			LVP_SubPTS(const LVP_SubPTS &pts)
			{
				this->start = pts.start;
				this->end   = pts.end;
			}
		};

		struct LVP_SubtitleContext : LVP_MediaContext
		{
			bool                        available;
			Strings                     external;
			LVP_FontFaceMap             fontFaces;
			LibFFmpeg::AVFormatContext* formatContext;
			bool                        isReadyForRender;
			bool                        isReadyForPresent;
			LVP_SubPTS                  pts;
			SDL_FPoint                  scale;
			SDL_Point                   size;
			std::vector<LVP_SubStyle*>  styles;
			LVP_FontMap                 styleFonts;
			LVP_Subtitles               subs;
			SDL_cond*                   subsCondition;
			SDL_mutex*                  subsMutex;
			Graphics::LVP_Texture*      textureCurrent;
			Graphics::LVP_Texture*      textureNext;
			SDL_Thread*                 thread;
			double                      timeToSleep;
			LibFFmpeg::AVSubtitleType   type;
			SDL_Rect                    videoDimensions;

			LVP_SubtitleContext()
			{
				this->reset();
			}

			void reset()
			{
				this->available         = true;
				this->formatContext     = NULL;
				this->isReadyForRender  = false;
				this->isReadyForPresent = false;
				this->pts               = {};
				this->scale             = { 1.0f, 1.0f };
				this->size              = {};
				this->subsCondition     = NULL;
				this->subsMutex         = NULL;
				this->textureCurrent    = NULL;
				this->textureNext       = NULL;
				this->thread            = NULL;
				this->type              = LibFFmpeg::SUBTITLE_NONE;
				this->timeToSleep       = 0.0;
				this->videoDimensions   = {};

				this->external.clear();
			}
		};

		struct LVP_VideoContext : LVP_MediaContext
		{
			LibFFmpeg::AVFrame*    frame;
			LibFFmpeg::AVFrame*    frameEncoded;
			LibFFmpeg::AVFrame*    frameHardware;
			LibFFmpeg::AVFrame*    frameSoftware;
			double                 frameRate;
			bool                   isReadyForRender;
			bool                   isReadyForPresent;
			double                 pts;
			Graphics::LVP_Texture* texture;
			SDL_Thread*            thread;

			LVP_VideoContext()
			{
				this->reset();
			}

			void reset()
			{
				this->frame             = NULL;
				this->frameEncoded      = NULL;
				this->frameHardware     = NULL;
				this->frameSoftware     = NULL;
				this->frameRate         = 0.0;
				this->isReadyForRender  = false;
				this->isReadyForPresent = false;
				this->pts               = 0.0;
				this->texture           = NULL;
				this->thread            = NULL;
			}
		};
	}

	namespace System
	{
		const int MAX_FILE_PATH = 260;

		#if defined _windows
			const char PATH_SEPARATOR[] = "\\";
		#else
			const char PATH_SEPARATOR[] = "/";
		#endif
	}
}

using namespace LibVoyaPlayer;

#include "LVP_Color.h"
#include "LVP_TimeOut.h"
#include "LVP_FileSystem.h"
#include "LVP_Graphics.h"
#include "LVP_Media.h"
#include "LVP_Text.h"
#include "LVP_Texture.h"
#include "LVP_SubRect.h"
#include "LVP_Subtitle.h"
#include "LVP_SubStyle.h"
#include "LVP_SubTexture.h"
#include "LVP_Player.h"
#include "LVP_SubTextRenderer.h"

#endif
