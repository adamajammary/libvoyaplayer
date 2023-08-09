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

const LibFFmpeg::AVCodecHWConfig* MediaPlayer::LVP_Media::getHardwareConfig(const LibFFmpeg::AVCodec* decoder)
{
	const LibFFmpeg::AVCodecHWConfig* hardwareConfig = NULL;

	for (int i = 0; (hardwareConfig = LibFFmpeg::avcodec_get_hw_config(decoder, i)) != NULL; i++)
	{
		if (hardwareConfig->methods & LibFFmpeg::AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
			return hardwareConfig;
	}

	return NULL;
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

	// Perform an extra scan if duration was not calculated in the initial scan
	if ((formatContext->duration < 1) && (LibFFmpeg::avformat_find_stream_info(formatContext, NULL) < 0))
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

	if (avRescaleC > 0)
		return (LibFFmpeg::av_rescale(avRescaleA, avRescaleB, avRescaleC) / AV_TIME_BASE_I64);

	return 0;
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

	if (isConcat)
	{
		int64_t duration = std::atoll(fileParts[fileParts.size() - 4].c_str());

		if (duration > 0)
			formatContext->duration = duration;
	}

	if (!parseStreams)
		return formatContext;

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

SDL_Surface* MediaPlayer::LVP_Media::GetMediaThumbnail(LibFFmpeg::AVFormatContext* formatContext)
{
	if (formatContext == NULL)
		return NULL;

	auto videoStream = LVP_Media::GetMediaTrackThumbnail(formatContext);

	if (videoStream == NULL)
		videoStream = LVP_Media::GetMediaTrackBest(formatContext, LibFFmpeg::AVMEDIA_TYPE_VIDEO);

	if ((videoStream == NULL) || (videoStream->codecpar == NULL))
		return NULL;

	auto decoder = LibFFmpeg::avcodec_find_decoder(videoStream->codecpar->codec_id);
	auto codec   = (decoder != NULL ? LibFFmpeg::avcodec_alloc_context3(decoder) : NULL);

	if (codec != NULL)
		LibFFmpeg::avcodec_parameters_to_context(codec, videoStream->codecpar);

	if ((codec == NULL) || (LibFFmpeg::avcodec_open2(codec, decoder, NULL) < 0)) {
		FREE_AVCODEC(codec);
		return NULL;
	}

	int  result = -1;
	auto frame  = LibFFmpeg::av_frame_alloc();

	if (videoStream->attached_pic.size > 0)
	{
		LibFFmpeg::avcodec_send_packet(codec, &videoStream->attached_pic);
		result = LibFFmpeg::avcodec_receive_frame(codec, frame);
	}
	else
	{
		auto seekPos = LVP_Media::GetMediaThumbnailSeekPos(formatContext);

		if (seekPos > 0)
			avformat_seek_file(formatContext, -1, INT64_MIN, seekPos, INT64_MAX, AV_SEEK_FLAGS(formatContext->iformat));

		auto packet = LibFFmpeg::av_packet_alloc();

		for (int i = 0; (LibFFmpeg::av_read_frame(formatContext, packet) == 0) && (i < 100); i++)
		{
			if (packet->stream_index != videoStream->index) {
				LibFFmpeg::av_packet_unref(packet);
				continue;
			}

			LibFFmpeg::avcodec_send_packet(codec, packet);
			result = LibFFmpeg::avcodec_receive_frame(codec, frame);

			if (result != AVERROR(EAGAIN))
				break;

			LibFFmpeg::av_frame_unref(frame);
			LibFFmpeg::av_packet_unref(packet);
		}

		FREE_AVPACKET(packet);
	}

	if (result < 0) {
		FREE_AVFRAME(frame);
		FREE_AVCODEC(codec);
		return NULL;
	}

	auto frameRGB = LibFFmpeg::av_frame_alloc();

	LibFFmpeg::av_image_alloc(frameRGB->data, frameRGB->linesize, frame->width, frame->height, LibFFmpeg::AV_PIX_FMT_RGB24, 24);

	auto contextRGB = LibFFmpeg::sws_getContext(
		frame->width, frame->height, (LibFFmpeg::AVPixelFormat)frame->format,
		frame->width, frame->height, LibFFmpeg::AV_PIX_FMT_RGB24,
		DEFAULT_SCALE_FILTER, NULL, NULL, NULL
	);

	SDL_Surface* thumbnail = NULL;

	result = LibFFmpeg::sws_scale(contextRGB, frame->data, frame->linesize, 0, frame->height, frameRGB->data, frameRGB->linesize);

	if (result > 0)
		thumbnail = SDL_CreateRGBSurfaceWithFormat(0, frame->width, frame->height, 24, SDL_PIXELFORMAT_RGB24);

	if (thumbnail != NULL)
	{
		bool lock = SDL_MUSTLOCK(thumbnail);
		auto size = (size_t)(frame->width * thumbnail->format->BytesPerPixel * frame->height);

		if (lock)
			SDL_LockSurface(thumbnail);

		std::memcpy(thumbnail->pixels, frameRGB->data[0], size);

		if (lock)
			SDL_UnlockSurface(thumbnail);
	}

	FREE_SWS(contextRGB);
	FREE_AVFRAME(frameRGB);
	FREE_AVFRAME(frame);
	FREE_AVCODEC(codec);

	return thumbnail;
}

int64_t MediaPlayer::LVP_Media::GetMediaThumbnailSeekPos(LibFFmpeg::AVFormatContext* formatContext)
{
	if (formatContext == NULL)
		return 0;

	const int64_t AV_TIME_100 = (100ll * AV_TIME_BASE_I64);
	const int64_t AV_TIME_50  = (50ll  * AV_TIME_BASE_I64);
	const int64_t AV_TIME_10  = (10ll  * AV_TIME_BASE_I64);

	int64_t seekPos = 0;

	if (formatContext->duration > AV_TIME_100)
		seekPos = AV_TIME_100;
	else if (formatContext->duration > AV_TIME_50)
		seekPos = AV_TIME_50;
	else if (formatContext->duration > AV_TIME_10)
		seekPos = AV_TIME_10;

	if (AV_SEEK_FLAGS(formatContext->iformat) != AVSEEK_FLAG_BYTE)
		return seekPos;

	auto fileSize = System::LVP_FileSystem::GetFileSize(formatContext->url);
	auto percent  = ((double)seekPos / (double)formatContext->duration);

	return (int64_t)((double)fileSize * percent);
}

LibFFmpeg::AVStream* MediaPlayer::LVP_Media::GetMediaTrackBest(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVMediaType mediaType)
{
	if ((formatContext == NULL) || (formatContext->nb_streams == 0))
		return NULL;

	auto index = LibFFmpeg::av_find_best_stream(formatContext, mediaType, -1, -1, NULL, 0);

	if (index < 0)
	{
		for (uint32_t i = 0; i < formatContext->nb_streams; i++) {
			if (formatContext->streams[i]->codecpar->codec_type == mediaType) {
				index = i;
				break;
			}
		}
	}

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
	if ((formatContext->nb_streams == 0) && (LibFFmpeg::avformat_find_stream_info(formatContext, NULL) < 0))
		return 0;

	size_t streamCount = 0;

	for (uint32_t i = 0; i < formatContext->nb_streams; i++)
	{
		auto stream = formatContext->streams[i];

		if ((stream == NULL) ||
			(stream->codecpar == NULL) ||
			(stream->codecpar->codec_id == LibFFmpeg::AV_CODEC_ID_NONE) ||
			(IS_VIDEO(mediaType) && (stream->disposition & AV_DISPOSITION_ATTACHED_PIC))) // AUDIO COVER
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

	auto audioStreamCount = LVP_Media::getMediaTrackCount(formatContext, LibFFmpeg::AVMEDIA_TYPE_AUDIO);
	auto subStreamCount   = LVP_Media::getMediaTrackCount(formatContext, LibFFmpeg::AVMEDIA_TYPE_SUBTITLE);
	auto videoStreamCount = LVP_Media::getMediaTrackCount(formatContext, LibFFmpeg::AVMEDIA_TYPE_VIDEO);

	if ((audioStreamCount > 0) && (videoStreamCount == 0) && (subStreamCount == 0))
		return LibFFmpeg::AVMEDIA_TYPE_AUDIO;
	else if ((subStreamCount > 0) && (audioStreamCount == 0) && (videoStreamCount == 0))
		return LibFFmpeg::AVMEDIA_TYPE_SUBTITLE;
	else if ((audioStreamCount > 0) && (videoStreamCount > 0))
		return LibFFmpeg::AVMEDIA_TYPE_VIDEO;

	return LibFFmpeg::AVMEDIA_TYPE_UNKNOWN;
}

LibFFmpeg::AVStream* MediaPlayer::LVP_Media::GetMediaTrackThumbnail(LibFFmpeg::AVFormatContext* formatContext)
{
	for (uint32_t i = 0; i < formatContext->nb_streams; i++) {
		if ((formatContext->streams[i]->codecpar->codec_type == LibFFmpeg::AVMEDIA_TYPE_VIDEO) && (formatContext->streams[i]->attached_pic.size > 0))
			return formatContext->streams[i];
	}

	return NULL;
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
		if (strcmp(entry->value, "und") == 0)
			continue;

		auto key   = System::LVP_Text::ToLower(entry->key);
		auto value = System::LVP_Text::Replace(entry->value, "\r", "\\r");
		value      = System::LVP_Text::Replace(value,        "\n", "\\n");

		meta[key] = value;
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

	// stream->disposition
	// AV_DISPOSITION_DEFAULT
	// AV_DISPOSITION_DUB
	// AV_DISPOSITION_ORIGINAL
	// AV_DISPOSITION_COMMENT
	// AV_DISPOSITION_LYRICS
	// AV_DISPOSITION_KARAOKE
	// AV_DISPOSITION_FORCED
	// AV_DISPOSITION_HEARING_IMPAIRED
	// AV_DISPOSITION_VISUAL_IMPAIRED
	// AV_DISPOSITION_CLEAN_EFFECTS
	// AV_DISPOSITION_ATTACHED_PIC
	// AV_DISPOSITION_TIMED_THUMBNAILS
	// AV_DISPOSITION_NON_DIEGETIC
	// AV_DISPOSITION_CAPTIONS
	// AV_DISPOSITION_DESCRIPTIONS
	// AV_DISPOSITION_METADATA
	// AV_DISPOSITION_DEPENDENT
	// AV_DISPOSITION_STILL_IMAGE
	int index = LibFFmpeg::av_find_best_stream(formatContext, mediaType, -1, -1, NULL, 0);

	if (index >= 0)
		LVP_Media::SetMediaTrackByIndex(formatContext, index, mediaContext);
}

LibFFmpeg::AVPixelFormat MediaPlayer::LVP_Media::getHardwarePixelFormat(LibFFmpeg::AVCodecContext* codec, const LibFFmpeg::AVPixelFormat* pixelFormats)
{
	const LibFFmpeg::AVPixelFormat* pixelFormat;

	for (pixelFormat = pixelFormats; *pixelFormat != LibFFmpeg::AV_PIX_FMT_NONE; pixelFormat++)
	{
		if (*pixelFormat == LVP_Player::GetPixelFormatHardware())
			return *pixelFormat;
	}

	return codec->sw_pix_fmt;
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
	bool isPNG   = (codec->codec_id == LibFFmpeg::AV_CODEC_ID_PNG);
	auto threads = (isPNG ? "1" : "auto");

	if (IS_VIDEO(stream->codecpar->codec_type))
	{
		auto hardwareConfig = LVP_Media::getHardwareConfig(decoder);

		if (hardwareConfig != NULL)
		{
			LibFFmpeg::AVBufferRef* hardwareDeviceContext = NULL;

			if (LibFFmpeg::av_hwdevice_ctx_create(&hardwareDeviceContext, hardwareConfig->device_type, "auto", NULL, 0) == 0)
			{
				codec->get_format    = LVP_Media::getHardwarePixelFormat;
				codec->hw_device_ctx = LibFFmpeg::av_buffer_ref(hardwareDeviceContext);

				mediaContext.pixelFormatHardware = hardwareConfig->pix_fmt;
			}
		}
	}

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
		case LibFFmpeg::AVMEDIA_TYPE_VIDEO:
			codec->pix_fmt = LibFFmpeg::AV_PIX_FMT_YUV420P;
			break;
		default:
			break;
	}
}
