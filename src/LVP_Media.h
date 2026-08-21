#ifndef LVP_MAIN_H
	#include "main.h"
#endif

#ifndef LVP_MEDIA_H
#define LVP_MEDIA_H

namespace LibVoyaPlayer
{
	namespace MediaPlayer
	{
		class LVP_Media
		{
		private:
			LVP_Media()  {}
			~LVP_Media() {}

		public:
			static double           GetAudioPTS(LVP_AudioContext* audioContext, AVFrame* frame);
			static LVP_MapStrStr    GetMediaCodecMeta(AVStream* stream);
			static int64_t          GetMediaDuration(AVFormatContext* formatContext, AVStream* audioStream);
			static AVFormatContext* GetMediaFormatContext(const std::string& filePath, bool parseStreams, System::LVP_TimeOut* timeOut = NULL);
			static double           GetMediaFrameRate(AVStream* stream);
			static LVP_MapStrStr    GetMediaMeta(AVFormatContext* formatContext);
			static SDL_Surface*     GetMediaThumbnail(AVFormatContext* formatContext);
			static AVStream*        GetMediaTrackBest(AVFormatContext* formatContext, AVMediaType mediaType);
			static LVP_MapStrStr    GetMediaTrackMeta(AVStream* stream);
			static AVMediaType      GetMediaType(AVFormatContext* formatContext);
			static LVP_PTS          GetPacketPTS(AVPacket* packet, const AVRational& timeBase, int64_t startTime);
			static LVP_PTS          GetSubtitlePTS(AVPacket* packet, AVSubtitle& frame, const AVRational& timeBase, int64_t startTime);
			static double           GetSubtitlePGSEndPTS(AVPacket* packet, const AVRational& timeBase);
			static double           GetVideoPTS(LVP_VideoContext* videoContext, int64_t startTime);
			static bool             IsStreamWithFontAttachments(AVStream* stream);
			static void             SetMediaTrackBest(AVFormatContext* formatContext, AVMediaType mediaType, LVP_MediaContext* mediaContext);
			static void             SetMediaTrackByIndex(AVFormatContext* formatContext, int index, LVP_MediaContext* mediaContext, int extSubFileIndex = -1);

		private:
			static const AVCodecHWConfig* getHardwareConfig(const AVCodec* decoder);
			static AVPixelFormat          getHardwarePixelFormat(AVCodecContext* codec, const AVPixelFormat* pixelFormats);
			static int64_t                getMediaThumbnailSeekPos(AVFormatContext* formatContext, bool isByteSeek);
			static size_t                 getMediaTrackCount(AVFormatContext* formatContext, AVMediaType mediaType);
			static AVStream*              getMediaTrackThumbnail(AVFormatContext* formatContext);
			static LVP_MapStrStr          getMeta(AVDictionary* metadata);
			static bool                   isDRM(AVDictionary* metaData);
			static void                   parseStreams(AVFormatContext* formatContext, const std::string filePath);
		};
	}
}

#endif
