#ifndef LVP_MAIN_H
#define LVP_MAIN_H

#include <algorithm> // min/max(x)
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#if defined _android
	#include <dirent.h>   // opendir(x)
	#include <unistd.h>   // chdir(x)
	#include <sys/stat.h> // stat64, lstat64(x)
#elif defined _ios
	#include <dirent.h>   // opendir(x)
	#include <unistd.h>   // chdir(x)
	#include <sys/stat.h> // stat64, lstat64(x)
#elif defined _linux
	#include <dirent.h>   // opendir(x)
	#include <sys/stat.h> // stat64, lstat64(x)
#elif defined _macosx
	#include <unistd.h>   // chdir(x)
	#include <sys/dir.h>  // opendir(x)
	#include <sys/stat.h> // stat64, lstat64(x)
#elif defined _windows
	#include <direct.h>   // _chdir(x)
	#include <dirent.h>   // _wopendir(x)
#endif

#ifndef LIB_SDL2_H
#define LIB_SDL2_H
extern "C"
{
	#include <SDL2/SDL.h>
}
#endif

#ifndef LIB_ASS_H
#define LIB_ASS_H
namespace LibASS
{
	extern "C"
	{
		#include <ass/ass.h>
	}
}
#endif

#ifndef LIB_FFMPEG_H
#define LIB_FFMPEG_H
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
#endif

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
		#define fstat       lstat
		#define stat_t      struct stat
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
		#define fstat       _wstat64
		#define stat_t      struct _stat64
		#define fseek       _fseeki64
		#define LOG(x, ...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, x, ##__VA_ARGS__);
	#endif

	#define FREE_POINTER(p) if (p != NULL) { std::free(p); p = NULL; }

	const int DEFAULT_CHAR_BUFFER_SIZE = 1024;

	using LVP_Strings = std::vector<std::string>;

	namespace System
	{
		const int MAX_FILE_PATH = 260;

		#if defined _windows
			const char PATH_SEPARATOR = '\\';
		#else
			const char PATH_SEPARATOR = '/';
		#endif
			
		const LVP_Strings SUB_FILE_EXTENSIONS =
		{
			"ass", "idx", "js", "jss", "mpl2", "pjs", "rt", "sami", "smi", "srt", "ssa", "stl", "sub", "txt", "vtt"
		};

		const LVP_Strings SYSTEM_FILE_EXTENSIONS = {
			"7z", "a", "aab", "apk", "appx", "appxbundle", "bat", "bin", "bz2", "cab", "cmd", "deb", "dll", "dmg", "doc", "docx", "ds_store", "dylib", "epub", "exe", "gz", "gzip", "ini", "ipa", "jar", "json", "lib", "lock", "log", "msi", "pdf", "pkg", "ppt", "pptx", "rar", "rpm", "sh", "so", "tar", "torrent", "ttf", "txt", "xls", "xlsx", "xml", "xz", "zip"
		};

		class LVP_TimeOut;
	}

	namespace MediaPlayer
	{
		#define DELETE_POINTER(p)      if (p != NULL) { delete p; p = NULL; }
		#define FREE_ASS_LIBRARY(l)    if (l != NULL) { LibASS::ass_library_done(l); l = NULL; }
		#define FREE_ASS_RENDERER(r)   if (r != NULL) { LibASS::ass_renderer_done(r); r = NULL; }
		#define FREE_ASS_TRACK(t)      if (t != NULL) { LibASS::ass_free_track(t); t = NULL; }
		#define FREE_AVCODEC(c)        if (c != NULL) { LibFFmpeg::avcodec_free_context(&c); c = NULL; }
		#define FREE_AVDICT(d)         if (d != NULL) { LibFFmpeg::av_dict_free(&d); d = NULL; }
		#define FREE_AVFILTER_GRAPH(g) if (g != NULL) { LibFFmpeg::avfilter_graph_free(&g); }
		#define FREE_AVFORMAT(f)       if (f != NULL) { LibFFmpeg::avformat_close_input(&f); LibFFmpeg::avformat_free_context(f); f = NULL; }
		#define FREE_AVFRAME(f)        if (f != NULL) { LibFFmpeg::av_frame_free(&f); f = NULL; }
		#define FREE_AVPOINTER(p)      if (p != NULL) { LibFFmpeg::av_free(p); p = NULL; }
		#define FREE_AVPACKET(p)       if (p != NULL) { LibFFmpeg::av_packet_free(&p); LibFFmpeg::av_free(p); p = NULL; }
		#define FREE_AVSTREAM(s)       if (s != NULL) { s->discard = LibFFmpeg::AVDISCARD_ALL; s = NULL; }
		#define FREE_RENDERER(r)       if (r != NULL) { SDL_DestroyRenderer(r); r = NULL; }
		#define FREE_SWR(s)            if (s != NULL) { LibFFmpeg::swr_free(&s); s = NULL; }
		#define FREE_SWS(s)            if (s != NULL) { LibFFmpeg::sws_freeContext(s); s = NULL; }
		#define FREE_SUB_DATA(t)       if (t != NULL) { LibFFmpeg::av_freep(&t); }
		#define FREE_SUB_FRAME(f)      if (f.num_rects > 0) { LibFFmpeg::avsubtitle_free(&f); f = {}; }
		#define FREE_SURFACE(s)        if (s != NULL) { SDL_FreeSurface(s); s = NULL; }
		#define FREE_TEXTURE(t)        if (t != NULL) { SDL_DestroyTexture(t); t = NULL; }
		#define FREE_THREAD(t)         if (t != NULL) { SDL_DetachThread(t); t = NULL; }
		#define FREE_THREAD_COND(c)    if (c != NULL) { SDL_DestroyCond(c);c = NULL; }
		#define FREE_THREAD_MUTEX(m)   if (m != NULL) { SDL_DestroyMutex(m); m = NULL; }

		#define IS_ATTACHMENT(t) (t == LibFFmpeg::AVMEDIA_TYPE_ATTACHMENT)
		#define IS_AUDIO(t)      (t == LibFFmpeg::AVMEDIA_TYPE_AUDIO)
		#define IS_BYTE_SEEK(i)  (((i->flags & AVFMT_NO_BYTE_SEEK) == 0) && ((i->flags & AVFMT_TS_DISCONT) != 0) && (strcmp("ogg", i->name) != 0))
		#define IS_FONT(i)       ((i == LibFFmpeg::AV_CODEC_ID_TTF) || (i == LibFFmpeg::AV_CODEC_ID_OTF))
		#define IS_SUB(t)        (t == LibFFmpeg::AVMEDIA_TYPE_SUBTITLE)
		#define IS_SUB_BITMAP(t) (t == LibFFmpeg::SUBTITLE_BITMAP)
		#define IS_VIDEO(t)      (t == LibFFmpeg::AVMEDIA_TYPE_VIDEO)

		const int    DEFAULT_SCALE_FILTER = SWS_LANCZOS;
		const int    MEGA_BYTE            = 1024000;
		const double ONE_SECOND_MS_D      = 1000.0;
		const int    ONE_SECOND_MS        = 1000;
		const int    SUB_STREAM_EXTERNAL  = 1000;

		const int64_t AV_TIME_BASE_I64 = 1000000LL;
		const double  AV_TIME_BASE_D   = 1000000.0;

		const int    DELAY_TIME_DEFAULT   = 15;
		const double DELAY_TIME_DEFAULT_S = 0.015;
		const int    DELAY_TIME_ONE_MS    = 1;

		const int    MAX_255x255   = 65025;
		const int    MAX_ERRORS    = 100;
		const double MAX_SUB_DELAY = -0.1;

		const int    MIN_PACKET_QUEUE_SIZE   = 25;
		const double MIN_SUB_DISPLAY_TIME    = 0.015;
		const int    MIN_SUB_PACKET_SIZE     = 100;
		const double MIN_SUB_TIME_TO_PTS     = (DELAY_TIME_DEFAULT_S + MIN_SUB_DISPLAY_TIME);

		enum LVP_RGBA
		{
			LVP_RGBA_R,
			LVP_RGBA_G,
			LVP_RGBA_B,
			LVP_RGBA_A
		};

		enum LVP_Threads
		{
			LVP_THREAD_AUDIO,
			LVP_THREAD_PACKETS,
			LVP_THREAD_SUBTITLE,
			LVP_THREAD_VIDEO,
			LVP_NR_OF_THREADS
		};

		struct LVP_AudioDevice
		{
			std::string       device        = "";
			SDL_AudioDeviceID deviceID      = 0;
			bool              isDeviceReady = true;

		};

		struct LVP_AudioFilter
		{
			LibFFmpeg::AVFilterContext* bufferSink   = NULL;
			LibFFmpeg::AVFilterContext* bufferSource = NULL;
			LibFFmpeg::AVFilterGraph*   filterGraph  = NULL;
		};

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

		struct LVP_MediaContext
		{
			LibFFmpeg::AVCodecContext*       codec       = NULL;
			int                              index       = -1;
			std::queue<LibFFmpeg::AVPacket*> packets     = {};
			std::mutex                       packetsLock = {};
			LibFFmpeg::AVStream*             stream      = NULL;
		};

		struct LVP_PTS
		{
			double start = 0.0;
			double end   = 0.0;
		};

		struct LVP_Size
		{
			int width  = 0;
			int height = 0;
		};
	}
}

using namespace LibVoyaPlayer;

#include "LVP_AudioSpecs.h"
#include "LVP_PlayerState.h"

#include "LVP_AudioContext.h"
#include "LVP_SubtitleContext.h"
#include "LVP_VideoContext.h"

#include "LVP_FileSystem.h"
#include "LVP_Media.h"
#include "LVP_Player.h"
#include "LVP_Subtitle.h"
#include "LVP_SubtitleBitmap.h"
#include "LVP_SubtitleText.h"
#include "LVP_Text.h"
#include "LVP_TimeOut.h"

#endif
