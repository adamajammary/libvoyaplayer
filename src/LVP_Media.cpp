#include "LVP_Media.h"

double MediaPlayer::LVP_Media::GetAudioPTS(LVP_AudioContext* audioContext, AVFrame* frame)
{
	auto pts = (double)frame->best_effort_timestamp;

	if (audioContext->avStream->start_time != AV_NOPTS_VALUE)
		pts -= (double)audioContext->avStream->start_time;

	pts *= av_q2d(audioContext->avStream->time_base);

	if (pts < 0)
		pts = (audioContext->lastPogress + audioContext->packetDuration);

	return pts;
}

const AVCodecHWConfig* MediaPlayer::LVP_Media::getHardwareConfig(const AVCodec* decoder)
{
	const AVCodecHWConfig* hardwareConfig = NULL;

	for (int i = 0; (hardwareConfig = avcodec_get_hw_config(decoder, i)) != NULL; i++) {
		if (hardwareConfig->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
			return hardwareConfig;
	}

	return NULL;
}

LVP_MapStrStr MediaPlayer::LVP_Media::GetMediaCodecMeta(AVStream* stream)
{
	LVP_MapStrStr meta;

	if ((stream == NULL) || (stream->codecpar == NULL))
		return meta;

	auto decoder = avcodec_find_decoder(stream->codecpar->codec_id);

	if (decoder != NULL) {
		meta["codec_name"] = std::string(decoder->long_name);
		meta["codec"]      = std::string(decoder->name);
	}

	switch (stream->codecpar->codec_type) {
		case AVMEDIA_TYPE_AUDIO:    meta["media_type"] = "audio"; break;
		case AVMEDIA_TYPE_SUBTITLE: meta["media_type"] = "subtitle"; break;
		case AVMEDIA_TYPE_VIDEO:    meta["media_type"] = "video"; break;
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
			meta["channel_layout"] = LVP_AudioSpecs::getChannelLayoutName(stream->codecpar->ch_layout);

		auto sampleFormat = av_get_sample_fmt_name((AVSampleFormat)stream->codecpar->format);

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

		auto pixelFormat = av_get_pix_fmt_name((AVPixelFormat)stream->codecpar->format);

		if (pixelFormat != NULL)
			meta["pixel_format"] = std::string(pixelFormat);

		if (stream->codecpar->width > 0)
			meta["video_width"] = std::to_string(stream->codecpar->width);

		if (stream->codecpar->height > 0)
			meta["video_height"] = std::to_string(stream->codecpar->height);
	}

	return meta;
}

int64_t MediaPlayer::LVP_Media::GetMediaDuration(AVFormatContext* formatContext, AVStream* audioStream)
{
	if (formatContext == NULL)
		return 0;

	if (formatContext->duration < 0)
		LVP_Media::parseStreams(formatContext, formatContext->url);

	if (formatContext->duration > 0)
		return (size_t)((double)formatContext->duration / AV_TIME_BASE_D);

	if (audioStream == NULL)
		return 0;

	if (audioStream->duration > 0)
		return (size_t)((double)audioStream->duration * av_q2d(audioStream->time_base));

	if (audioStream->codecpar == NULL)
		return 0;

	auto fileSize = System::LVP_FileSystem::GetFileSize(formatContext->url);

	if (fileSize == 0)
		return 0;

	auto avRescaleA = (int64_t)(fileSize * 8ll);
	auto avRescaleB = (int64_t)audioStream->time_base.den;
	auto avRescaleC = (int64_t)(audioStream->codecpar->bit_rate * audioStream->codecpar->ch_layout.nb_channels * audioStream->time_base.num);

	if (avRescaleC > 0)
		return (av_rescale(avRescaleA, avRescaleB, avRescaleC) / AV_TIME_BASE_I64);

	return 0;
}

/**
 * @throws invalid_argument
 * @throws runtime_error
 */
AVFormatContext* MediaPlayer::LVP_Media::GetMediaFormatContext(const std::string& filePath, bool parseStreams, System::LVP_TimeOut* timeOut)
{
	if (filePath.empty())
		throw std::invalid_argument("filePath cannot be empty");

	auto fileObject = System::LVP_FileSystem::GetFile(filePath);

	if (System::LVP_FileSystem::IsSystemFile(fileObject))
		throw std::runtime_error(std::format("Invalid media file: {}", filePath));

	auto fileParts     = LVP_Strings();
	auto file          = std::string(filePath);
	auto formatContext = avformat_alloc_context();
	bool isConcat      = System::LVP_FileSystem::IsConcat(filePath);

	// BLURAY/DVD: "concat:streamPath|stream1|...|streamN|duration|title|audioTrackCount|subTrackCount|"
	if (isConcat)
	{
		fileParts = System::LVP_Text::Split(std::string(file).substr(7), "|");
		file      = "concat:";

		for (uint32_t i = 1; i < fileParts.size() - 4; i++)
			file.append(fileParts[i] + "|");

		std::filesystem::current_path(fileParts[0]);

		if (std::filesystem::current_path().generic_string() != fileParts[0])
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
		throw std::runtime_error(std::format("[{}] Failed to open input: {}", result, file));
	}

	auto probeScore = formatContext->probe_score;

	if (probeScore < AVPROBE_SCORE_RETRY) {
		FREE_AVFORMAT(formatContext);
		throw std::runtime_error(std::format("[{}] Invalid probe score: {}", probeScore, file));
	}

	if (LVP_Media::isDRM(formatContext->metadata)) {
		FREE_AVFORMAT(formatContext);
		throw std::runtime_error("Media is DRM encrypted.");
	}

	// Try to fix MP3 files with invalid header and codec type
	if (fileObject.ext == "mp3")
	{
		for (uint32_t i = 0; i < formatContext->nb_streams; i++)
		{
			auto stream = formatContext->streams[i];

			if ((stream == NULL) || (stream->codecpar == NULL))
				continue;

			auto codecType = stream->codecpar->codec_type;
			auto codecID   = stream->codecpar->codec_id;

			if ((codecID == AV_CODEC_ID_NONE) && IS_AUDIO(codecType))
				stream->codecpar->codec_id = AV_CODEC_ID_MP3;
		}
	}

	if (isConcat)
	{
		int64_t duration = std::atoll(fileParts[fileParts.size() - 4].c_str());

		if (duration > 0)
			formatContext->duration = duration;
	}

	if (formatContext->duration == 0)
		formatContext->duration = INT64_MIN;

	if (parseStreams)
		LVP_Media::parseStreams(formatContext, file);

	return formatContext;
}

double MediaPlayer::LVP_Media::GetMediaFrameRate(AVStream* stream)
{
	if (stream == NULL)
		return 0;

	// r_frame_rate is wrong - Needs adjustment
	if ((stream->time_base.num > 0) && (stream->time_base.den > 0) &&
		(av_q2d(stream->time_base) < (av_q2d(stream->r_frame_rate) * 0.7)) &&
		(fabs(1.0 - av_q2d(av_div_q(stream->avg_frame_rate, stream->r_frame_rate))) > 0.1))
	{
		return av_q2d(stream->time_base);
	// r_frame_rate is valid
	} else if ((stream->r_frame_rate.num > 0) && (stream->r_frame_rate.den > 0)) {
		return av_q2d(stream->r_frame_rate);
	}

	// r_frame_rate is not valid - Use avg_frame_rate
	return av_q2d(stream->avg_frame_rate);
}

LVP_MapStrStr MediaPlayer::LVP_Media::GetMediaMeta(AVFormatContext* formatContext)
{
	return LVP_Media::getMeta(formatContext != NULL ? formatContext->metadata : NULL);
}

SDL_Surface* MediaPlayer::LVP_Media::GetMediaThumbnail(AVFormatContext* formatContext)
{
	if (formatContext == NULL)
		return NULL;

	if ((formatContext->duration < 0) || (formatContext->nb_streams == 0))
		LVP_Media::parseStreams(formatContext, formatContext->url);

	auto videoStream = LVP_Media::getMediaTrackThumbnail(formatContext);

	if (videoStream == NULL)
		videoStream = LVP_Media::GetMediaTrackBest(formatContext, AVMEDIA_TYPE_VIDEO);

	if ((videoStream == NULL) || (videoStream->codecpar == NULL))
		return NULL;

	auto decoder = avcodec_find_decoder(videoStream->codecpar->codec_id);
	auto codec   = (decoder != NULL ? avcodec_alloc_context3(decoder) : NULL);

	if (codec != NULL)
		avcodec_parameters_to_context(codec, videoStream->codecpar);

	if ((codec == NULL) || (avcodec_open2(codec, decoder, NULL) < 0)) {
		FREE_AVCODEC(codec);
		return NULL;
	}

	int  result = -1;
	auto frame  = av_frame_alloc();

	if (videoStream->attached_pic.size > 0)
	{
		avcodec_send_packet(codec, &videoStream->attached_pic);

		result = avcodec_receive_frame(codec, frame);
	}
	else
	{
		bool isByteSeek = IS_BYTE_SEEK(formatContext->iformat);
		auto seekPos    = LVP_Media::getMediaThumbnailSeekPos(formatContext, isByteSeek);

		if (seekPos > 0)
			av_seek_frame(formatContext, -1, seekPos, (isByteSeek ? AVSEEK_FLAG_BYTE : 0));

		auto packet = av_packet_alloc();

		for (int i = 0; (av_read_frame(formatContext, packet) == 0) && (i < 100); i++)
		{
			if (packet->stream_index != videoStream->index) {
				av_packet_unref(packet);
				continue;
			}

			avcodec_send_packet(codec, packet);
			result = avcodec_receive_frame(codec, frame);

			if (result != AVERROR(EAGAIN))
				break;

			av_frame_unref(frame);
			av_packet_unref(packet);
		}

		FREE_AVPACKET(packet);
	}

	if (result < 0) {
		FREE_AVFRAME(frame);
		FREE_AVCODEC(codec);
		return NULL;
	}

	auto frameRGB = av_frame_alloc();

	av_image_alloc(frameRGB->data, frameRGB->linesize, frame->width, frame->height, AV_PIX_FMT_RGBA, 1);

	auto contextRGB = sws_getContext(
		frame->width,
		frame->height,
		(AVPixelFormat)frame->format,
		frame->width,
		frame->height,
		AV_PIX_FMT_RGBA,
		DEFAULT_SCALE_FILTER,
		NULL,
		NULL,
		NULL
	);

	SDL_Surface* thumbnail = NULL;

	result = sws_scale_frame(contextRGB, frameRGB, frame);

	if (result > 0)
		thumbnail = SDL_CreateSurface(frame->width, frame->height, SDL_PIXELFORMAT_RGBA32);

	if (thumbnail != NULL)
	{
		thumbnail->pitch = frameRGB->linesize[0];

		auto size = (size_t)(frame->height * frame->width * SDL_BYTESPERPIXEL(thumbnail->format));

		std::memcpy(thumbnail->pixels, frameRGB->data[0], size);
	}

	FREE_AVFRAME(frameRGB);
	FREE_AVFRAME(frame);
	FREE_SWS(contextRGB);
	FREE_AVCODEC(codec);

	return thumbnail;
}

int64_t MediaPlayer::LVP_Media::getMediaThumbnailSeekPos(AVFormatContext* formatContext, bool isByteSeek)
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

	if (!isByteSeek)
		return seekPos;

	auto fileSize = System::LVP_FileSystem::GetFileSize(formatContext->url);
	auto percent  = ((double)seekPos / (double)formatContext->duration);

	return (int64_t)((double)fileSize * percent);
}

AVStream* MediaPlayer::LVP_Media::GetMediaTrackBest(AVFormatContext* formatContext, AVMediaType mediaType)
{
	if ((formatContext == NULL) || (formatContext->nb_streams == 0))
		return NULL;

	AVStream* firstMatch = NULL;
	AVStream* bestMatch  = NULL;

	for (uint32_t i = 0; i < formatContext->nb_streams; i++)
	{
		auto stream = formatContext->streams[i];

		if (stream->codecpar->codec_type != mediaType)
			continue;

		if (firstMatch == NULL)
			firstMatch = stream;

		if ((stream->disposition & AV_DISPOSITION_FORCED) || (stream->disposition & AV_DISPOSITION_DEFAULT)) {
			bestMatch = stream;
			break;
		}
	}

	if ((bestMatch == NULL) && !IS_SUB(mediaType))
		bestMatch = firstMatch;

	return bestMatch;
}

size_t MediaPlayer::LVP_Media::getMediaTrackCount(AVFormatContext* formatContext, AVMediaType mediaType)
{
	if (formatContext == NULL)
		return 0;

	if (formatContext->nb_streams == 0)
		LVP_Media::parseStreams(formatContext, formatContext->url);

	size_t streamCount = 0;

	for (uint32_t i = 0; i < formatContext->nb_streams; i++)
	{
		auto stream = formatContext->streams[i];

		if ((stream == NULL) ||
			(stream->codecpar == NULL) ||
			(stream->codecpar->codec_id == AV_CODEC_ID_NONE) ||
			(IS_VIDEO(mediaType) && (stream->disposition & AV_DISPOSITION_ATTACHED_PIC))) // AUDIO COVER
		{
			continue;
		}

		if (stream->codecpar->codec_type == mediaType)
			streamCount++;
	}

	return streamCount;
}

LVP_MapStrStr MediaPlayer::LVP_Media::GetMediaTrackMeta(AVStream* stream)
{
	return LVP_Media::getMeta(stream != NULL ? stream->metadata : NULL);
}

AVMediaType MediaPlayer::LVP_Media::GetMediaType(AVFormatContext* formatContext)
{
	if (formatContext == NULL)
		return AVMEDIA_TYPE_UNKNOWN;

	auto audioStreamCount = LVP_Media::getMediaTrackCount(formatContext, AVMEDIA_TYPE_AUDIO);
	auto subStreamCount   = LVP_Media::getMediaTrackCount(formatContext, AVMEDIA_TYPE_SUBTITLE);
	auto videoStreamCount = LVP_Media::getMediaTrackCount(formatContext, AVMEDIA_TYPE_VIDEO);

	if ((audioStreamCount > 0) && (videoStreamCount == 0) && (subStreamCount == 0))
		return AVMEDIA_TYPE_AUDIO;
	else if ((subStreamCount > 0) && (audioStreamCount == 0) && (videoStreamCount == 0))
		return AVMEDIA_TYPE_SUBTITLE;
	else if ((audioStreamCount > 0) && (videoStreamCount > 0))
		return AVMEDIA_TYPE_VIDEO;

	return AVMEDIA_TYPE_UNKNOWN;
}

AVStream* MediaPlayer::LVP_Media::getMediaTrackThumbnail(AVFormatContext* formatContext)
{
	for (uint32_t i = 0; i < formatContext->nb_streams; i++) {
		if (IS_VIDEO(formatContext->streams[i]->codecpar->codec_type) && (formatContext->streams[i]->attached_pic.size > 0))
			return formatContext->streams[i];
	}

	return NULL;
}

// http://wiki.multimedia.cx/index.php?title=FFmpeg_Metadata
// https://www.exiftool.org/TagNames/ID3.html

LVP_MapStrStr MediaPlayer::LVP_Media::getMeta(AVDictionary* metadata)
{
	LVP_MapStrStr meta;

	if (metadata == NULL)
		return meta;

	AVDictionaryEntry* entry = NULL;

	while ((entry = av_dict_get(metadata, "", entry, AV_DICT_IGNORE_SUFFIX)) != NULL)
	{
		if (strcmp(entry->value, "und") == 0)
			continue;

		auto key = System::LVP_Text::Trim(System::LVP_Text::ToLower(entry->key));

		if (key.empty() || key.starts_with("id3v2_priv."))
			continue;

		auto value = System::LVP_Text::Trim(entry->value);

		value = System::LVP_Text::Replace(value, "\r", "\\r");
		value = System::LVP_Text::Replace(value, "\n", "\\n");

		if (!value.empty())
			meta[key] = value;
	}

	return meta;
}

MediaPlayer::LVP_PTS MediaPlayer::LVP_Media::GetPacketPTS(AVPacket* packet, const AVRational& timeBase, int64_t startTime)
{
	if (packet == NULL)
		return {};

	LVP_PTS pts = {};

	pts.start = (double)(packet->pts != AV_NOPTS_VALUE ? packet->pts : packet->dts);

	if (startTime != AV_NOPTS_VALUE)
		pts.start -= (double)startTime;

	pts.start *= av_q2d(timeBase);
	pts.end    = (packet->duration > 0 ? (pts.start + ((double)packet->duration * av_q2d(timeBase))) : 0.0);

	return pts;
}

MediaPlayer::LVP_PTS MediaPlayer::LVP_Media::GetSubtitlePTS(AVPacket* packet, AVSubtitle& frame, const AVRational& timeBase, int64_t startTime)
{
	if (packet == NULL)
		return {};

	auto pts = LVP_Media::GetPacketPTS(packet, timeBase, startTime);

	if (frame.start_display_time > 0)
		pts.start += (double)((double)frame.start_display_time / ONE_SECOND_MS_D);

	if (frame.end_display_time == UINT32_MAX)
		pts.end = 0.0;
	else if (frame.end_display_time > 0)
		pts.end = (double)(pts.start + (double)((double)frame.end_display_time / ONE_SECOND_MS_D));
	else if (packet->duration > 0)
		pts.end = (double)(pts.start + (double)((double)packet->duration * av_q2d(timeBase)));
	else
		pts.end = 0.0;

	return pts;
}

double MediaPlayer::LVP_Media::GetSubtitlePGSEndPTS(AVPacket* packet, const AVRational& timeBase)
{
	if (packet == NULL)
		return 0.0;

	auto end = (double)(packet->pts != AV_NOPTS_VALUE ? packet->pts : packet->dts);

	return (end * av_q2d(timeBase));
}

double MediaPlayer::LVP_Media::GetVideoPTS(LVP_VideoContext* videoContext, int64_t startTime)
{
	auto pts = (double)videoContext->frame->best_effort_timestamp;

	if (startTime != AV_NOPTS_VALUE)
		pts -= (double)startTime;

	pts *= av_q2d(videoContext->avStream->time_base);

	return pts;
}

bool MediaPlayer::LVP_Media::isDRM(AVDictionary* metaData)
{
	return (av_dict_get(metaData, "encryption", NULL, 0) != NULL);
}

bool MediaPlayer::LVP_Media::IsStreamWithFontAttachments(AVStream* stream)
{
	if ((stream == NULL) || (stream->codecpar == NULL) || !IS_ATTACHMENT(stream->codecpar->codec_type) || (stream->codecpar->extradata_size <= 0))
		return false;

	if (IS_FONT(stream->codecpar->codec_id))
		return true;

	auto mimeType = av_dict_get(stream->metadata, "mimetype", NULL, 0);

	if ((mimeType == NULL) || (mimeType->value == NULL))
		return false;

	return (strstr(mimeType->value, "font") || strstr(mimeType->value, "ttf") || strstr(mimeType->value, "otf"));
}

void MediaPlayer::LVP_Media::parseStreams(AVFormatContext* formatContext, const std::string filePath)
{
	if (formatContext->duration >= 0)
		return;
	
	if (formatContext->nb_streams == 0) {
		formatContext->max_analyze_duration = (int64_t)(15 * AV_TIME_BASE);
		formatContext->probesize            = (int64_t)(10 * MEGA_BYTE);
	}

	int result = avformat_find_stream_info(formatContext, NULL);

	if (result < 0) {
		FREE_AVFORMAT(formatContext);
		throw std::runtime_error(std::format("[{}] Failed to find stream info: {}", result, filePath));
	}

	#if defined _DEBUG
		av_dump_format(formatContext, -1, filePath.c_str(), 0);
	#endif

	if (formatContext->duration < 0)
		formatContext->duration = 0;
}

void MediaPlayer::LVP_Media::SetMediaTrackBest(AVFormatContext* formatContext, AVMediaType mediaType, LVP_MediaContext* mediaContext)
{
	if (formatContext == NULL)
		return;

	auto stream = LVP_Media::GetMediaTrackBest(formatContext, mediaType);

	if (stream != NULL)
		LVP_Media::SetMediaTrackByIndex(formatContext, stream->index, mediaContext);
}

AVPixelFormat MediaPlayer::LVP_Media::getHardwarePixelFormat(AVCodecContext* codec, const AVPixelFormat* pixelFormats)
{
	const AVPixelFormat* pixelFormat;

	for (pixelFormat = pixelFormats; *pixelFormat != AV_PIX_FMT_NONE; pixelFormat++) {
		if (*pixelFormat == LVP_Player::GetPixelFormatHardware())
			return *pixelFormat;
	}

	return codec->sw_pix_fmt;
}

void MediaPlayer::LVP_Media::SetMediaTrackByIndex(AVFormatContext* formatContext, int index, LVP_MediaContext* mediaContext, int extSubFileIndex)
{
	if ((formatContext == NULL) || (index < 0) || ((int)formatContext->nb_streams <= index))
		return;

	auto stream = formatContext->streams[index];

	if ((stream == NULL) || (stream->codecpar == NULL) || (stream->codecpar->codec_id == AV_CODEC_ID_NONE))
		return;

	auto codec = avcodec_alloc_context3(NULL);

	if (codec == NULL)
		return;

	int initCodecResult = avcodec_parameters_to_context(codec, stream->codecpar);

	if (initCodecResult < 0) {
		FREE_AVCODEC(codec);
		return;
	}

	codec->pkt_timebase = stream->time_base;

	auto decoder = avcodec_find_decoder(codec->codec_id);

	if (decoder == NULL) {
		FREE_AVCODEC(codec);
		return;
	}

	codec->codec_id = decoder->id;

	// Multi-threading must be disabled for some music cover/thumb types like PNG
	bool isPNG   = (codec->codec_id == AV_CODEC_ID_PNG);
	auto threads = (isPNG ? "1" : "auto");

	if (IS_VIDEO(stream->codecpar->codec_type))
	{
		auto videoContext = static_cast<LVP_VideoContext*>(mediaContext);
		auto hwConfig     = LVP_Media::getHardwareConfig(decoder);

		if ((hwConfig != NULL) &&
			(av_hwdevice_ctx_create(&videoContext->hwDeviceContext, hwConfig->device_type, "auto", NULL, 0) == 0))
		{
			videoContext->hwPixelFormat = hwConfig->pix_fmt;

			codec->get_format    = LVP_Media::getHardwarePixelFormat;
			codec->hw_device_ctx = av_buffer_ref(videoContext->hwDeviceContext);
		}
	}

	AVDictionary* options = NULL;

	av_dict_set(&options, "threads", threads, 0);

	auto openResult = avcodec_open2(codec, decoder, &options);

	FREE_AVDICT(options);

	if (openResult < 0) {
		FREE_AVCODEC(codec);
		return;
	}

	stream->discard = AVDISCARD_DEFAULT;

	bool isSubsExternal = (extSubFileIndex >= 0);

	mediaContext->codec     = codec;
	mediaContext->index    = (stream->index + (isSubsExternal ? ((extSubFileIndex + 1) * SUB_STREAM_EXTERNAL) : 0)),
	mediaContext->avStream = stream;

	if (codec->pix_fmt != AV_PIX_FMT_NONE)
		return;

	switch (stream->codecpar->codec_type) {
		case AVMEDIA_TYPE_SUBTITLE: codec->pix_fmt = AV_PIX_FMT_PAL8;    break;
		case AVMEDIA_TYPE_VIDEO:    codec->pix_fmt = AV_PIX_FMT_YUV420P; break;
		default: break;
	}
}
