#ifndef LVP_GLOBAL_H
	#include "LVP_Global.h"
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
			static std::string                        GetAudioChannelLayout(const LibFFmpeg::AVChannelLayout &layout);
			static double                             GetAudioPTS(const LVP_AudioContext &audioContext);
			static std::map<std::string, std::string> GetMediaCodecMeta(LibFFmpeg::AVStream* stream);
			static int64_t                            GetMediaDuration(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVStream* audioStream);
			static LibFFmpeg::AVFormatContext*        GetMediaFormatContext(const std::string &filePath, bool parseStreams, System::LVP_TimeOut* timeOut = NULL);
			static double                             GetMediaFrameRate(LibFFmpeg::AVStream* stream);
			static std::map<std::string, std::string> GetMediaMeta(LibFFmpeg::AVFormatContext* formatContext);
			static std::map<std::string, std::string> GetMediaTrackMeta(LibFFmpeg::AVStream* stream);
			static LibFFmpeg::AVMediaType             GetMediaType(LibFFmpeg::AVFormatContext* formatContext);
			static LVP_SubPTS                         GetSubtitlePTS(LibFFmpeg::AVPacket* packet, LibFFmpeg::AVSubtitle &frame, const LibFFmpeg::AVRational &timeBase, int64_t startTime);
			static double                             GetSubtitleEndPTS(LibFFmpeg::AVPacket* packet, const LibFFmpeg::AVRational &timeBase);
			static double                             GetVideoPTS(LibFFmpeg::AVFrame* frame, const LibFFmpeg::AVRational &timeBase, int64_t startTime);
			static bool                               HasSubtitleTracks(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVFormatContext* formatContextExternal);
			static bool                               IsStreamWithFontAttachments(LibFFmpeg::AVStream* stream);
			static void                               SetMediaTrackBest(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVMediaType mediaType, LVP_MediaContext &mediaContext);
			static void                               SetMediaTrackByIndex(LibFFmpeg::AVFormatContext* formatContext, int index, LVP_MediaContext &mediaContext, bool isSubsExternal = false);

		private:
			static size_t                             getMediaTrackCount(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVMediaType mediaType);
			static std::map<std::string, std::string> getMeta(LibFFmpeg::AVDictionary* metadata);
			static bool                               isDRM(LibFFmpeg::AVDictionary* metaData);

		};
	}
}

#endif
