#include "LVP_Media.h"

std::string MediaPlayer::LVP_Media::GetAudioChannelLayout(const LibFFmpeg::AVChannelLayout &layout)
{
	char buffer[DEFAULT_CHAR_BUFFER_SIZE];

	LibFFmpeg::av_channel_layout_describe(&layout, buffer, DEFAULT_CHAR_BUFFER_SIZE);

	return std::string(buffer);
}

double MediaPlayer::LVP_Media::GetAudioPTS(const LVP_AudioContext &audioContext)
{
	auto pts = (double)audioContext.frame->best_effort_timestamp;

	if (audioContext.stream->start_time != AV_NOPTS_VALUE)
		pts -= (double)audioContext.stream->start_time;

	pts *= LibFFmpeg::av_q2d(audioContext.stream->time_base);

	if (pts < 0)
		pts = (audioContext.lastPogress + audioContext.frameDuration);

	return pts;
}

std::map<std::string, std::string> MediaPlayer::LVP_Media::GetMediaCodecMeta(LibFFmpeg::AVStream* stream)
{
	std::map<std::string, std::string> meta;

	if ((stream == NULL) || (stream->codecpar == NULL))
		return meta;

	auto decoder = LibFFmpeg::avcodec_find_decoder(stream->codecpar->codec_id);

	if (decoder != NULL) {
		meta["codec_name"] = std::string(decoder->long_name);
		meta["codec"]      = std::string(decoder->name);
	}

	switch (stream->codecpar->codec_type) {
		case LibFFmpeg::AVMEDIA_TYPE_AUDIO:    meta["media_type"] = "audio"; break;
		case LibFFmpeg::AVMEDIA_TYPE_SUBTITLE: meta["media_type"] = "subtitle"; break;
		case LibFFmpeg::AVMEDIA_TYPE_VIDEO:    meta["media_type"] = "video"; break;
		default: break;
	}

	if (stream->codecpar->bit_rate > 0)
		meta["bit_rate"] = std::to_string(stream->codecpar->bit_rate);

	if (stream->codecpar->bits_per_coded_sample > 0)
		meta["bits_per_coded_sample"] = std::to_string(stream->codecpar->bits_per_coded_sample);

	if (stream->codecpar->bits_per_raw_sample > 0)
		meta["bits_per_raw_sample"] = std::to_string(stream->codecpar->bits_per_raw_sample);

	if (IS_AUDIO(stream->codecpar->codec_type))
	{
		if (stream->codecpar->ch_layout.nb_channels > 0)
			meta["channel_layout"] = LVP_Media::GetAudioChannelLayout(stream->codecpar->ch_layout);

		auto sampleFormat = LibFFmpeg::av_get_sample_fmt_name((LibFFmpeg::AVSampleFormat)stream->codecpar->format);

		if (sampleFormat != NULL)
			meta["sample_format"] = std::string(sampleFormat);

		if (stream->codecpar->sample_rate > 0)
			meta["sample_rate"] = std::to_string(stream->codecpar->sample_rate);
	}
	else if (IS_VIDEO(stream->codecpar->codec_type))
	{
		auto frameRate = LVP_Media::GetMediaFrameRate(stream);

		if (frameRate > 0)
			meta["frame_rate"] = std::to_string(frameRate);

		auto pixelFormat = LibFFmpeg::av_get_pix_fmt_name((LibFFmpeg::AVPixelFormat)stream->codecpar->format);

		if (pixelFormat != NULL)
			meta["pixel_format"] = std::string(pixelFormat);

		if (stream->codecpar->width > 0)
			meta["video_width"] = std::to_string(stream->codecpar->width);

		if (stream->codecpar->height > 0)
			meta["video_height"] = std::to_string(stream->codecpar->height);
	}

	return meta;
}

int64_t MediaPlayer::LVP_Media::GetMediaDuration(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVStream* audioStream)
{
	if (formatContext == NULL)
		return 0;

	if (formatContext->duration > 0)
		return (size_t)((double)formatContext->duration / AV_TIME_BASE_D);

	if (audioStream == NULL)
		return 0;

	if (audioStream->duration > 0)
		return (size_t)((double)audioStream->duration * LibFFmpeg::av_q2d(audioStream->time_base));

	auto fileSize = System::LVP_FileSystem::GetFileSize(formatContext->url);

	if ((audioStream->codecpar == NULL) || (fileSize == 0))
		return 0;

	auto avRescaleA = (int64_t)(fileSize * 8ll);
	auto avRescaleB = (int64_t)audioStream->time_base.den;
	auto avRescaleC = (int64_t)(audioStream->codecpar->bit_rate * audioStream->codecpar->ch_layout.nb_channels * audioStream->time_base.num);

	return (LibFFmpeg::av_rescale(avRescaleA, avRescaleB, avRescaleC) / AV_TIME_BASE_I64);
}

/**
 * @throws invalid_argument
 * @throws exception
 */
LibFFmpeg::AVFormatContext* MediaPlayer::LVP_Media::GetMediaFormatContext(const std::string &filePath, bool parseStreams, System::LVP_TimeOut* timeOut)
{
	if (filePath.empty())
		throw std::invalid_argument("filePath cannot be empty");

	Strings fileParts;
	auto    file          = std::string(filePath);
	auto    formatContext = LibFFmpeg::avformat_alloc_context();
	bool    isConcat      = System::LVP_FileSystem::IsConcat(filePath);

	// BLURAY / DVD: "concat:streamPath|stream1|stream2|streamN|duration|title|audioTrackCount|subTrackCount|"
	if (isConcat)
	{
		fileParts = System::LVP_Text::Split(std::string(file).substr(7), "|");
		file      = "concat:";

		for (uint32_t i = 1; i < fileParts.size() - 4; i++)
			file.append(fileParts[i] + "|");

		if (chdir(fileParts[0].c_str()) != 0)
			throw std::invalid_argument(std::format("Failed to change directory: {}", fileParts[0]));
	}

	if (timeOut != NULL) {
		formatContext->flags                      |= AVFMT_FLAG_NONBLOCK;
		formatContext->interrupt_callback.callback = System::LVP_TimeOut::InterruptCallback;
		formatContext->interrupt_callback.opaque   = timeOut;
	}

	formatContext->max_analyze_duration = (int64_t)(10 * AV_TIME_BASE);

	int result = avformat_open_input(&formatContext, file.c_str(), NULL, NULL);

	if ((result < 0) || (formatContext == NULL)) {
		FREE_AVFORMAT(formatContext);
		throw std::exception(std::format("[{}] Failed to open input: {}", result, file).c_str());
	}

	result = formatContext->probe_score;

	if (result < AVPROBE_SCORE_RETRY) {
		FREE_AVFORMAT(formatContext);
		throw std::exception(std::format("[{}] Invalid probe score: {}", result, file).c_str());
	}

	if (LVP_Media::isDRM(formatContext->metadata)) {
		FREE_AVFORMAT(formatContext);
		throw std::exception("Media is DRM encrypted.");
	}

	if (!parseStreams)
		return formatContext;

	// Try to fix MP3 files with invalid header and codec type
	if (System::LVP_FileSystem::GetFileExtension(file, true) == "MP3")
	{
		for (uint32_t i = 0; i < formatContext->nb_streams; i++)
		{
			auto stream = formatContext->streams[i];

			if ((stream == NULL) || (stream->codecpar == NULL))
				continue;

			auto codecType = stream->codecpar->codec_type;
			auto codecID   = stream->codecpar->codec_id;

			if ((codecID == LibFFmpeg::AV_CODEC_ID_NONE) && (codecType == LibFFmpeg::AVMEDIA_TYPE_AUDIO))
				stream->codecpar->codec_id = LibFFmpeg::AV_CODEC_ID_MP3;
		}
	}

	if (formatContext->nb_streams == 0) {
		formatContext->max_analyze_duration = (int64_t)(15 * AV_TIME_BASE);
		formatContext->probesize            = (int64_t)(10 * MEGA_BYTE);
	}

	if ((result = LibFFmpeg::avformat_find_stream_info(formatContext, NULL)) < 0) {
		FREE_AVFORMAT(formatContext);
		throw std::exception(std::format("[{}] Failed to find stream info: {}", result, file).c_str());
	}

	#if defined _DEBUG
		LibFFmpeg::av_dump_format(formatContext, -1, file.c_str(), 0);
	#endif

	if (isConcat)
	{
		int64_t duration = std::atoll(fileParts[fileParts.size() - 4].c_str());

		if (duration > 0)
			formatContext->duration = duration;
	}

	return formatContext;
}

double MediaPlayer::LVP_Media::GetMediaFrameRate(LibFFmpeg::AVStream* stream)
{
	if (stream == NULL)
		return 0;

	auto timeBase = LibFFmpeg::av_stream_get_codec_timebase(stream);

	// r_frame_rate is wrong - Needs adjustment
	if ((timeBase.num > 0) && (timeBase.den > 0) &&
		(LibFFmpeg::av_q2d(timeBase) < (LibFFmpeg::av_q2d(stream->r_frame_rate) * 0.7)) &&
		(fabs(1.0 - LibFFmpeg::av_q2d(av_div_q(stream->avg_frame_rate, stream->r_frame_rate))) > 0.1))
	{
		return LibFFmpeg::av_q2d(timeBase);
	// r_frame_rate is valid
	} else if ((stream->r_frame_rate.num > 0) && (stream->r_frame_rate.den > 0)) {
		return LibFFmpeg::av_q2d(stream->r_frame_rate);
	}

	// r_frame_rate is not valid - Use avg_frame_rate
	return LibFFmpeg::av_q2d(stream->avg_frame_rate);
}

std::map<std::string, std::string> MediaPlayer::LVP_Media::GetMediaMeta(LibFFmpeg::AVFormatContext* formatContext)
{
	return LVP_Media::getMeta(formatContext != NULL ? formatContext->metadata : NULL);
}

LibFFmpeg::AVStream* MediaPlayer::LVP_Media::GetMediaTrackBest(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVMediaType mediaType)
{
	if ((formatContext == NULL) || (formatContext->nb_streams == 0))
		return NULL;

	int  index   = LibFFmpeg::av_find_best_stream(formatContext, mediaType, -1, -1, NULL, 0);
	bool isValid = ((index >= 0) && (index < (int)formatContext->nb_streams));
	auto stream  = (isValid ? formatContext->streams[index] : NULL);

	if ((stream == NULL) || (stream->codecpar == NULL) || (stream->codecpar->codec_id == LibFFmpeg::AV_CODEC_ID_NONE))
		return NULL;

	return stream;
}

size_t MediaPlayer::LVP_Media::getMediaTrackCount(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVMediaType mediaType)
{
	if (formatContext == NULL)
		return 0;

	// Perform an extra scan if no streams were found in the initial scan
	if (formatContext->nb_streams == 0) {
		if (LibFFmpeg::avformat_find_stream_info(formatContext, NULL) < 0)
			return 0;
	}

	size_t streamCount = 0;

	for (uint32_t i = 0; i < formatContext->nb_streams; i++)
	{
		LibFFmpeg::AVStream* stream = formatContext->streams[i];

		if ((stream == NULL) ||
			(stream->codecpar == NULL) ||
			(stream->codecpar->codec_id == LibFFmpeg::AV_CODEC_ID_NONE) ||
			(IS_VIDEO(mediaType) && (stream->attached_pic.size > 0))) // AUDIO COVER
		{
			continue;
		}

		if (stream->codecpar->codec_type == mediaType)
			streamCount++;
	}

	return streamCount;
}

std::map<std::string, std::string> MediaPlayer::LVP_Media::GetMediaTrackMeta(LibFFmpeg::AVStream* stream)
{
	return LVP_Media::getMeta(stream != NULL ? stream->metadata : NULL);
}

LibFFmpeg::AVMediaType MediaPlayer::LVP_Media::GetMediaType(LibFFmpeg::AVFormatContext* formatContext)
{
	if (formatContext == NULL)
		return LibFFmpeg::AVMEDIA_TYPE_UNKNOWN;

	size_t audioStreamCount = LVP_Media::getMediaTrackCount(formatContext, LibFFmpeg::AVMEDIA_TYPE_AUDIO);
	size_t subStreamCount   = LVP_Media::getMediaTrackCount(formatContext, LibFFmpeg::AVMEDIA_TYPE_SUBTITLE);
	size_t videoStreamCount = LVP_Media::getMediaTrackCount(formatContext, LibFFmpeg::AVMEDIA_TYPE_VIDEO);

	if ((audioStreamCount > 0) && (videoStreamCount == 0) && (subStreamCount == 0))
		return LibFFmpeg::AVMEDIA_TYPE_AUDIO;
	else if ((subStreamCount > 0) && (audioStreamCount == 0) && (videoStreamCount == 0))
		return LibFFmpeg::AVMEDIA_TYPE_SUBTITLE;
	else if ((audioStreamCount > 0) && (videoStreamCount > 0))
		return LibFFmpeg::AVMEDIA_TYPE_VIDEO;

	return LibFFmpeg::AVMEDIA_TYPE_UNKNOWN;
}

// http://wiki.multimedia.cx/index.php?title=FFmpeg_Metadata
std::map<std::string, std::string> MediaPlayer::LVP_Media::getMeta(LibFFmpeg::AVDictionary* metadata)
{
	std::map<std::string, std::string> meta;

	if (metadata == NULL)
		return meta;

	LibFFmpeg::AVDictionaryEntry* entry = NULL;

	while ((entry = LibFFmpeg::av_dict_get(metadata, "", entry, AV_DICT_IGNORE_SUFFIX)) != NULL)
	{
		if (strcmp(entry->value, "und") != 0)
			meta[System::LVP_Text::ToLower(entry->key)] = entry->value;
	}

	return meta;
}

MediaPlayer::LVP_SubPTS MediaPlayer::LVP_Media::GetSubtitlePTS(LibFFmpeg::AVPacket* packet, LibFFmpeg::AVSubtitle &frame, const LibFFmpeg::AVRational &timeBase, int64_t startTime)
{
	if (packet == NULL)
		return {};

	// START PTS
	auto start = (double)(packet->pts != AV_NOPTS_VALUE ? packet->pts : packet->dts);

	if (startTime != AV_NOPTS_VALUE)
		start -= (double)startTime;

	start *= LibFFmpeg::av_q2d(timeBase);

	if (frame.start_display_time > 0)
		start += (double)((double)frame.start_display_time / (double)ONE_SECOND_MS);

	// END PTS
	double end;

	if (frame.end_display_time == UINT32_MAX)
		end = UINT32_MAX;
	else if (frame.end_display_time > 0)
		end = (double)(start + (double)((double)frame.end_display_time / (double)ONE_SECOND_MS));
	else if (packet->duration > 0)
		end = (double)(start + (double)((double)packet->duration * LibFFmpeg::av_q2d(timeBase)));
	else
		end = (start + MAX_SUB_DURATION);

	return { start, end };
}

double MediaPlayer::LVP_Media::GetSubtitleEndPTS(LibFFmpeg::AVPacket* packet, const LibFFmpeg::AVRational &timeBase)
{
	if (packet == NULL)
		return 0;

	auto end = (double)(packet->pts != AV_NOPTS_VALUE ? packet->pts : packet->dts);

	return (end * LibFFmpeg::av_q2d(timeBase));
}

double MediaPlayer::LVP_Media::GetVideoPTS(LibFFmpeg::AVFrame* frame, const LibFFmpeg::AVRational &timeBase, int64_t startTime)
{
	auto pts = (double)frame->best_effort_timestamp;

	if (startTime != AV_NOPTS_VALUE)
		pts -= (double)startTime;

	pts *= LibFFmpeg::av_q2d(timeBase);

	return pts;
}

bool MediaPlayer::LVP_Media::HasSubtitleTracks(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVFormatContext* formatContextExternal)
{
	auto streamCount         = LVP_Media::getMediaTrackCount(formatContext,         LibFFmpeg::AVMEDIA_TYPE_SUBTITLE);
	auto streamCountExternal = LVP_Media::getMediaTrackCount(formatContextExternal, LibFFmpeg::AVMEDIA_TYPE_SUBTITLE);

	return ((streamCount + streamCountExternal) > 0);
}

bool MediaPlayer::LVP_Media::isDRM(LibFFmpeg::AVDictionary* metaData)
{
	return (LibFFmpeg::av_dict_get(metaData, "encryption", NULL, 0) != NULL);
}

bool MediaPlayer::LVP_Media::IsStreamWithFontAttachments(LibFFmpeg::AVStream* stream)
{
	if ((stream == NULL) || (stream->codecpar == NULL) || (stream->codecpar->extradata_size < 1) || !IS_ATTACHMENT(stream->codecpar->codec_type))
		return false;

	auto mimetype = LibFFmpeg::av_dict_get(stream->metadata, "mimetype", NULL, 0);

	return ((mimetype != NULL) && std::string(mimetype->value).find("font") != std::string::npos);
}

void MediaPlayer::LVP_Media::SetMediaTrackBest(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVMediaType mediaType, LVP_MediaContext &mediaContext)
{
	if (formatContext == NULL)
		return;

	int index = LibFFmpeg::av_find_best_stream(formatContext, mediaType, -1, -1, NULL, 0);

	if (index >= 0)
		LVP_Media::SetMediaTrackByIndex(formatContext, index, mediaContext);
}

void MediaPlayer::LVP_Media::SetMediaTrackByIndex(LibFFmpeg::AVFormatContext* formatContext, int index, LVP_MediaContext &mediaContext, bool isSubsExternal)
{
	if ((formatContext == NULL) || (formatContext->nb_streams == 0))
		return;

	bool isValid = ((index >= 0) && (index < (int)formatContext->nb_streams));
	auto stream  = (isValid ? formatContext->streams[index] : NULL);

	if ((stream == NULL) || (stream->codecpar == NULL) || (stream->codecpar->codec_id == LibFFmpeg::AV_CODEC_ID_NONE))
		return;

	auto codec = LibFFmpeg::avcodec_alloc_context3(NULL);

	if (codec == NULL)
		return;

	int initCodecResult = LibFFmpeg::avcodec_parameters_to_context(codec, stream->codecpar);

	if (initCodecResult < 0) {
		FREE_AVCODEC(codec);
		return;
	}

	codec->pkt_timebase = stream->time_base;

	auto decoder = LibFFmpeg::avcodec_find_decoder(codec->codec_id);

	if (decoder == NULL) {
		FREE_AVCODEC(codec);
		return;
	}

	codec->codec_id = decoder->id;

	// Multi-threading must be disabled for some music cover/thumb types like PNG
	auto codecID = codec->codec_id;
	bool isPNG   = (codecID == LibFFmpeg::AV_CODEC_ID_PNG);
	auto threads = (!isPNG ? "auto" : "1");

	LibFFmpeg::AVDictionary* options = NULL;
	LibFFmpeg::av_dict_set(&options, "threads", threads, 0);

	if (LibFFmpeg::avcodec_open2(codec, decoder, &options) < 0)
	{
		FREE_AVCODEC(codec);
		FREE_AVDICT(options);

		return;
	}

	FREE_AVDICT(options);

	stream->discard = LibFFmpeg::AVDISCARD_DEFAULT;

	mediaContext.codec  = codec;
	mediaContext.index  = (isSubsExternal ? (SUB_STREAM_EXTERNAL + stream->index) : stream->index);
	mediaContext.stream = stream;

	if (codec->pix_fmt != LibFFmpeg::AV_PIX_FMT_NONE)
		return;

	switch (stream->codecpar->codec_type) {
		case LibFFmpeg::AVMEDIA_TYPE_SUBTITLE:
			codec->pix_fmt = LibFFmpeg::AV_PIX_FMT_PAL8;
			break;
		case  LibFFmpeg::AVMEDIA_TYPE_VIDEO:
			codec->pix_fmt = LibFFmpeg::AV_PIX_FMT_YUV420P;
			break;
		default:
			break;
	}
}
