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
			static std::string                        GetAudioChannelLayout(const LibFFmpeg::AVChannelLayout& layout);
			static double                             GetAudioPTS(LVP_AudioContext* audioContext);
			static std::map<std::string, std::string> GetMediaCodecMeta(LibFFmpeg::AVStream* stream);
			static int64_t                            GetMediaDuration(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVStream* audioStream);
			static LibFFmpeg::AVFormatContext*        GetMediaFormatContext(const std::string& filePath, bool parseStreams, System::LVP_TimeOut* timeOut = NULL);
			static double                             GetMediaFrameRate(LibFFmpeg::AVStream* stream);
			static std::map<std::string, std::string> GetMediaMeta(LibFFmpeg::AVFormatContext* formatContext);
			static SDL_Surface*                       GetMediaThumbnail(LibFFmpeg::AVFormatContext* formatContext);
			static LibFFmpeg::AVStream*               GetMediaTrackBest(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVMediaType mediaType);
			static std::map<std::string, std::string> GetMediaTrackMeta(LibFFmpeg::AVStream* stream);
			static LibFFmpeg::AVMediaType             GetMediaType(LibFFmpeg::AVFormatContext* formatContext);
			static LVP_PTS                            GetPacketPTS(LibFFmpeg::AVPacket* packet, const LibFFmpeg::AVRational& timeBase, int64_t startTime);
			static LVP_PTS                            GetSubtitlePTS(LibFFmpeg::AVPacket* packet, LibFFmpeg::AVSubtitle& frame, const LibFFmpeg::AVRational& timeBase, int64_t startTime);
			static double                             GetSubtitleEndPTS(LibFFmpeg::AVPacket* packet, const LibFFmpeg::AVRational& timeBase);
			static double                             GetVideoPTS(LibFFmpeg::AVFrame* frame, const LibFFmpeg::AVRational& timeBase, int64_t startTime);
			static bool                               IsStreamWithFontAttachments(LibFFmpeg::AVStream* stream);
			static void                               SetMediaTrackBest(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVMediaType mediaType, LVP_MediaContext* mediaContext);
			static void                               SetMediaTrackByIndex(LibFFmpeg::AVFormatContext* formatContext, int index, LVP_MediaContext* mediaContext, int extSubFileIndex = -1);

		private:
			static const LibFFmpeg::AVCodecHWConfig*  getHardwareConfig(const LibFFmpeg::AVCodec* decoder);
			static LibFFmpeg::AVPixelFormat           getHardwarePixelFormat(LibFFmpeg::AVCodecContext* codec, const LibFFmpeg::AVPixelFormat* pixelFormats);
			static int64_t                            getMediaThumbnailSeekPos(LibFFmpeg::AVFormatContext* formatContext);
			static size_t                             getMediaTrackCount(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVMediaType mediaType);
			static LibFFmpeg::AVStream*               getMediaTrackThumbnail(LibFFmpeg::AVFormatContext* formatContext);
			static std::map<std::string, std::string> getMeta(LibFFmpeg::AVDictionary* metadata);
			static bool                               isDRM(LibFFmpeg::AVDictionary* metaData);
		};
	}
}

#endif
