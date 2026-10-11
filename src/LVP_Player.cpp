#include "LVP_Player.h"

LVP_CallbackContext             MediaPlayer::LVP_Player::callback   = {};
MediaPlayer::LVP_PlayerContext* MediaPlayer::LVP_Player::context    = NULL;
std::mutex                      MediaPlayer::LVP_Player::packetLock = {};
MediaPlayer::LVP_PlayerState    MediaPlayer::LVP_Player::state      = {};

void MediaPlayer::LVP_Player::Init(const LVP_CallbackContext& callbackContext)
{
	LVP_Player::callback   = callbackContext;
	LVP_Player::state.quit = false;

	LVP_Player::SetAudioDevices();
}

void MediaPlayer::LVP_Player::CallbackError(const std::string& errorMessage)
{
	if (LVP_Player::callback.errorCB == nullptr)
		return;

	std::thread([errorMessage]() {
		LVP_Player::callback.errorCB(errorMessage, LVP_Player::callback.data);
	}).detach();
}

void MediaPlayer::LVP_Player::callbackEvents(LVP_EventType type)
{
	if (LVP_Player::callback.eventsCB == nullptr)
		return;

	std::thread([type]() {
		LVP_Player::callback.eventsCB(type, LVP_Player::callback.data);
	}).detach();
}

void MediaPlayer::LVP_Player::callbackVideoIsAvailable(SDL_Surface* surface)
{
	if (LVP_Player::callback.videoCB == nullptr)
		return;

	std::thread([surface]() {
		LVP_Player::callback.videoCB(surface, LVP_Player::callback.data);
	}).detach();
}

void MediaPlayer::LVP_Player::Close()
{
	if (!LVP_Player::state.isStopped && !LVP_Player::state.isStopping)
		LVP_Player::state.quit = true;
}

void MediaPlayer::LVP_Player::close()
{
	if (LVP_Player::state.isStopped || LVP_Player::state.isStopping)
		return;

	LVP_Player::state.isStopping = true;

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_STOPPING);

	LVP_Player::Pause();

	LVP_Player::state.quit = true;

	while (
		LVP_Player::state.threads[LVP_THREAD_AUDIO] ||
		LVP_Player::state.threads[LVP_THREAD_AUDIO_CALLBACK] ||
		LVP_Player::state.threads[LVP_THREAD_PACKETS] ||
		LVP_Player::state.threads[LVP_THREAD_SUBTITLE] ||
		LVP_Player::state.threads[LVP_THREAD_VIDEO]
	) {
		SDL_Delay(DELAY_TIME_DEFAULT);
	}

	LVP_Player::closePackets();

	LVP_Player::closeStream(AVMEDIA_TYPE_AUDIO);
	LVP_Player::closeStream(AVMEDIA_TYPE_SUBTITLE);
	LVP_Player::closeStream(AVMEDIA_TYPE_VIDEO);

	LVP_Player::closeAudioStream();
	LVP_Player::closeSubContext();

	DELETE_POINTER(LVP_Player::context);

	LVP_Player::state.reset();

	LVP_Player::state.isCompleted = false;
	LVP_Player::state.isStopped   = true;
	LVP_Player::state.quit        = false;

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_STOPPED);

	LVP_Player::state.isStopping = false;
}

void MediaPlayer::LVP_Player::closeAudioStream()
{
	if ((LVP_Player::context == NULL) || (LVP_Player::context->audio == NULL) || (LVP_Player::context->audio->audioStream == NULL))
		return;

	SDL_DestroyAudioStream(LVP_Player::context->audio->audioStream);

	LVP_Player::context->audio->audioStream = NULL;
}

void MediaPlayer::LVP_Player::closePackets(LVP_MediaContext* context)
{
	while (!context->packets.empty()) {
		FREE_AVPACKET(context->packets.front());
		context->packets.pop();
	}
}

void MediaPlayer::LVP_Player::closePackets()
{
	LVP_Player::closePackets(LVP_Player::context->audio);
	LVP_Player::closePackets(LVP_Player::context->subtitle);
	LVP_Player::closePackets(LVP_Player::context->video);
}

void MediaPlayer::LVP_Player::closeStream(AVMediaType streamType)
{
	switch (streamType) {
		case AVMEDIA_TYPE_AUDIO:    LVP_Player::closeStream(LVP_Player::context->audio);    break;
		case AVMEDIA_TYPE_SUBTITLE: LVP_Player::closeStream(LVP_Player::context->subtitle); break;
		case AVMEDIA_TYPE_VIDEO:    LVP_Player::closeStream(LVP_Player::context->video);    break;
		default: break;
	}
}

void MediaPlayer::LVP_Player::closeStream(LVP_MediaContext* mediaContext)
{
	if (mediaContext == NULL)
		return;

	if (mediaContext->codec != NULL)
		avcodec_flush_buffers(mediaContext->codec);

	FREE_AVSTREAM(mediaContext->avStream);
	FREE_AVCODEC(mediaContext->codec);

	mediaContext->index = -1;
}

void MediaPlayer::LVP_Player::closeSubContext()
{
	LVP_SubtitleBitmap::Remove();

	#if defined _ENABLE_VIDEO_AV1_AND_SUBS_ASS
		LVP_SubtitleText::Remove();
		LVP_SubtitleText::Quit();
	#endif
}

int MediaPlayer::LVP_Player::decodeAudioFrame()
{
    if (LVP_Player::state.isPaused)
        return -1;

	auto frame = LVP_Player::getAudioFrame();

	if (frame == NULL)
		return -1;

	auto bufferSize = av_samples_get_buffer_size(
		NULL,
		frame->ch_layout.nb_channels,
		frame->nb_samples,
		(AVSampleFormat)frame->format,
		1
	);

	// Allocate initial buffer
	if (LVP_Player::context->audio->buffer == NULL)
	{
		LVP_Player::context->audio->buffer = (uint8_t*)std::malloc(bufferSize);
	}
	// Increase size of buffer if needed
	else if (bufferSize > LVP_Player::context->audio->bufferSize)
	{
		auto newBuffer = (uint8_t*)std::realloc(LVP_Player::context->audio->buffer, bufferSize);

		if (newBuffer != NULL)
			LVP_Player::context->audio->buffer = newBuffer;
		else
			FREE_POINTER(LVP_Player::context->audio->buffer);
	}

	LVP_Player::context->audio->bufferSize = bufferSize;

	if (LVP_Player::context->audio->buffer == NULL)
	{
		FREE_AVFRAME(frame);
		LVP_Player::stop("Failed to create an audio buffer.");
		
		return AVERROR(ENOMEM);
	}

	std::memcpy(LVP_Player::context->audio->buffer, frame->extended_data[0], bufferSize);

	LVP_Player::setAudioProgress(frame);

	FREE_AVFRAME(frame);

	return bufferSize;
}

void MediaPlayer::LVP_Player::decodeAudioFrames()
{
	int result;

	while (true)
	{
		// Decode frame

		LVP_Player::packetLock.lock();

		auto frame = av_frame_alloc();

		result = avcodec_receive_frame(LVP_Player::context->audio->codec, frame);

		LVP_Player::packetLock.unlock();

		if (result < 0) {
			FREE_AVFRAME(frame);
			break;
		}

		// Apply filter to frame

		if (!LVP_Player::context->audio->filterSpecs.equals(frame, LVP_Player::state.playbackSpeed) ||
			(LVP_Player::context->audio->filter.filterGraph == NULL))
		{
			LVP_Player::initAudioFilter(frame);

			LVP_Player::context->audio->filterSpecs = LVP_AudioSpecs(frame, LVP_Player::state.playbackSpeed);
		}

		result = av_buffersrc_add_frame(LVP_Player::context->audio->filter.bufferSource, frame);

		if (result < 0) {
			FREE_AVFRAME(frame);
			break;
		}

		bool isFrameFiltered = false;

		while (result >= 0)
		{
			result = av_buffersink_get_frame(LVP_Player::context->audio->filter.bufferSink, frame);

			if (result >= 0)
				isFrameFiltered = true;
		}

		if ((result < 0) && (result != AVERROR(EAGAIN))) {
			FREE_AVFRAME(frame);
			break;
		}

		if (!isFrameFiltered) {
			FREE_AVFRAME(frame);
			continue;
		}

		// Add filtered frame

		LVP_Player::context->audio->framesLock.lock();

		LVP_Player::context->audio->frames.push(frame);

		LVP_Player::context->audio->framesLock.unlock();
	}

	if ((result < 0) && (result != AVERROR(EAGAIN)))
	{
		LVP_Player::logAVError(result, "LVP_Player::decodeAudioFrames failed");
		LVP_Player::stop("Failed to decode audio.");
	}
	else if (result == AVERROR_EOF)
	{
		LVP_Player::packetLock.lock();

		avcodec_flush_buffers(LVP_Player::context->audio->codec);

		LVP_Player::packetLock.unlock();
	}
}

void MediaPlayer::LVP_Player::decodeAudioPacket(AVPacket* packet)
{
	LVP_Player::packetLock.lock();

	auto result = avcodec_send_packet(LVP_Player::context->audio->codec, packet);

	LVP_Player::packetLock.unlock();

	if ((result < 0) && (result != AVERROR(EAGAIN)))
		LVP_Player::logAVError(result, "LVP_Player::decodeAudioPacket failed");
}

std::string MediaPlayer::LVP_Player::GetAudioDevice()
{
	return LVP_Player::state.audioDevice.name;
}

LVP_Strings MediaPlayer::LVP_Player::GetAudioDevices()
{
	LVP_Strings devices;

	devices.reserve(LVP_Player::state.audioDevices.size());
	devices.push_back("Default");

	for (const auto& device : LVP_Player::state.audioDevices) {
		if (device.first != "Default")
			devices.push_back(device.first);
	}

	return devices;
}

AVFrame* MediaPlayer::LVP_Player::getAudioFrame()
{
	AVFrame* frame = NULL;

	LVP_Player::context->audio->framesLock.lock();

	if (!LVP_Player::context->audio->frames.empty())
	{
		frame = LVP_Player::context->audio->frames.front();

		LVP_Player::context->audio->frames.pop();
	}

	LVP_Player::context->audio->framesLock.unlock();

	return frame;
}

int MediaPlayer::LVP_Player::GetAudioTrack()
{
	return (LVP_Player::context->audio ? LVP_Player::context->audio->index : -1);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::GetAudioTracks()
{
	return LVP_Player::getAudioTracks(LVP_Player::context->format);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::getAudioTracks(AVFormatContext* formatContext)
{
	return LVP_Player::getMediaTracks(AVMEDIA_TYPE_AUDIO, formatContext);
}

std::vector<LVP_MediaChapter> MediaPlayer::LVP_Player::GetChapters()
{
	return LVP_Player::getChapters(LVP_Player::context->format);
}

std::vector<LVP_MediaChapter> MediaPlayer::LVP_Player::getChapters(AVFormatContext* formatContext)
{
	if (formatContext == NULL)
		return {};

	int64_t lastChapterEnd = 0;

	std::vector<LVP_MediaChapter> chapters;

	for (unsigned int i = 0; i < formatContext->nb_chapters; i++)
	{
		auto chapter = formatContext->chapters[i];

		if (chapter->start < lastChapterEnd)
			continue;

		lastChapterEnd = chapter->end;

		auto timeBase = av_q2d(chapter->time_base);
		auto end      = (int64_t)((double)chapter->end   * timeBase * ONE_SECOND_MS_D);
		auto start    = (int64_t)((double)chapter->start * timeBase * ONE_SECOND_MS_D);
		auto title    = av_dict_get(chapter->metadata, "title", NULL, 0);

		chapters.push_back({
			.title     = (title ? std::string(title->value) : ""),
			.startTime = start,
			.endTime   = end,
		});
	}

	return chapters;
}

int64_t MediaPlayer::LVP_Player::GetDuration()
{
	return (LVP_Player::state.duration * 1000);
}

std::string MediaPlayer::LVP_Player::GetFilePath()
{
	return (LVP_Player::context->format != NULL ? std::string(LVP_Player::context->format->url) : "");
}

LVP_MediaDetails MediaPlayer::LVP_Player::GetMediaDetails(bool skipThumbnail)
{
	if (LVP_Player::state.isStopped)
		return {};

	return
	{
		.duration       = LVP_Media::GetMediaDuration(LVP_Player::context->format, LVP_Player::context->audio->avStream),
		.fileSize       = System::LVP_FileSystem::GetFileSize(LVP_Player::context->format->url),
		.mediaType      = (LVP_MediaType)LVP_Player::state.mediaType,
		.meta           = LVP_Media::GetMediaMeta(LVP_Player::context->format),
		.thumbnail      = (!skipThumbnail ? LVP_Media::GetMediaThumbnail(LVP_Player::context->format) : NULL),
		.chapters       = LVP_Player::GetChapters(),
		.audioTracks    = LVP_Player::GetAudioTracks(),
		.subtitleTracks = LVP_Player::GetSubtitleTracks(),
		.videoTracks    = LVP_Player::GetVideoTracks()
	};
}

LVP_MediaDetails MediaPlayer::LVP_Player::GetMediaDetails(const std::string& filePath, bool skipThumbnail)
{
	auto formatContext = LVP_Media::GetMediaFormatContext(filePath, false);

	if (formatContext == NULL)
		return {};

	auto audioStream = LVP_Media::GetMediaTrackBest(formatContext, AVMEDIA_TYPE_AUDIO);
	auto mediaType   = LVP_Media::GetMediaType(formatContext);
	auto extSubFiles = (IS_VIDEO(mediaType) ? System::LVP_FileSystem::GetSubtitleFilesForVideo(filePath) : LVP_Strings());

	LVP_MediaDetails details =
	{
		.duration       = LVP_Media::GetMediaDuration(formatContext, audioStream),
		.fileSize       = System::LVP_FileSystem::GetFileSize(formatContext->url),
		.mediaType      = (LVP_MediaType)mediaType,
		.meta           = LVP_Media::GetMediaMeta(formatContext),
		.thumbnail      = (!skipThumbnail ? LVP_Media::GetMediaThumbnail(formatContext) : NULL),
		.chapters       = LVP_Player::getChapters(formatContext),
		.audioTracks    = LVP_Player::getAudioTracks(formatContext),
		.subtitleTracks = LVP_Player::getSubtitleTracks(formatContext, extSubFiles),
		.videoTracks    = LVP_Player::getVideoTracks(formatContext)
	};

	FREE_AVFORMAT(formatContext);

	return details;
}

AVPacket* MediaPlayer::LVP_Player::getMediaPacket(LVP_MediaContext* context)
{
	AVPacket* packet = NULL;

	context->packetsLock.lock();

	if (!context->packets.empty())
	{
		packet = context->packets.front();

		context->packets.pop();
	}

	context->packetsLock.unlock();

	if ((packet != NULL) && (packet->data == NULL))
	{
		FREE_AVPACKET(packet);

		LVP_Player::packetLock.lock();

		avcodec_flush_buffers(context->codec);

		LVP_Player::packetLock.unlock();
	}

	return packet;
}

SDL_Surface* MediaPlayer::LVP_Player::GetMediaThumbnail()
{
	if (LVP_Player::state.isStopped)
		return nullptr;

	return LVP_Media::GetMediaThumbnail(LVP_Player::context->format);
}

SDL_Surface* MediaPlayer::LVP_Player::GetMediaThumbnail(const std::string& filePath)
{
	auto formatContext = LVP_Media::GetMediaFormatContext(filePath, false);

	if (formatContext == NULL)
		return nullptr;

	auto thumbnail = LVP_Media::GetMediaThumbnail(formatContext);

	FREE_AVFORMAT(formatContext);

	return thumbnail;
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::getMediaTracks(
	AVMediaType        mediaType,
	AVFormatContext*   formatContext,
	const LVP_Strings& extSubFiles
)
{
	// INTERNAL MEDIA TRACKS

	auto tracks = LVP_Player::getMediaTracksMeta(formatContext, mediaType);

	if (!IS_SUB(mediaType))
		return tracks;

	// EXTERNAL SUB TRACKS

	for (int i = 0; i < (int)extSubFiles.size(); i++)
	{
		auto extFormatContext = LVP_Media::GetMediaFormatContext(extSubFiles[i], true);
		auto extSubTracks     = LVP_Player::getMediaTracksMeta(extFormatContext, AVMEDIA_TYPE_SUBTITLE, i);

		if (!extSubTracks.empty())
			tracks.insert(tracks.end(), extSubTracks.begin(), extSubTracks.end());
	}

	int subCount = 0;

	for (const auto& track : tracks) {
		if (track.mediaType == LVP_MEDIA_TYPE_SUBTITLE)
			subCount++;
	}

	if (subCount <= 0)
		return tracks;

	// DISABLE SUBS

	tracks.insert(tracks.begin(), {
		.mediaType = LVP_MEDIA_TYPE_SUBTITLE,
		.track     = -1,
		.meta      = {{ "title", "Disable" }}
	});

	return tracks;
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::getMediaTracksMeta(AVFormatContext* formatContext, AVMediaType mediaType, int extSubFileIndex)
{
	if (formatContext == NULL)
		return {};

	bool isSubsExternal = (extSubFileIndex >= 0);

	std::vector<LVP_MediaTrack> tracks;

	for (unsigned int i = 0; i < formatContext->nb_streams; i++)
	{
		auto stream = formatContext->streams[i];

		if ((stream == NULL) || (stream->codecpar == NULL) || (stream->codecpar->codec_type != mediaType))
			continue;

		tracks.push_back({
			.mediaType  = (LVP_MediaType)mediaType,
			.track      = (stream->index + (isSubsExternal ? ((extSubFileIndex + 1) * SUB_STREAM_EXTERNAL) : 0)),
			.meta       = LVP_Media::GetMediaTrackMeta(stream),
			.codec      = LVP_Media::GetMediaCodecMeta(stream)
		});
	}

	return tracks;
}

LVP_MediaMeta MediaPlayer::LVP_Player::GetMediaMeta()
{
	LVP_MediaMeta mediaMeta =
	{
		.meta      = LVP_Media::GetMediaMeta(LVP_Player::context->format),
		.mediaType = (LVP_MediaType)LVP_Media::GetMediaType(LVP_Player::context->format)
	};

	return mediaMeta;
}

LVP_MediaMeta MediaPlayer::LVP_Player::GetMediaMeta(const std::string& filePath)
{
	auto formatContext = LVP_Media::GetMediaFormatContext(filePath, false);

	LVP_MediaMeta mediaMeta =
	{
		.meta      = LVP_Media::GetMediaMeta(formatContext),
		.mediaType = (LVP_MediaType)LVP_Media::GetMediaType(formatContext)
	};

	FREE_AVFORMAT(formatContext);

	return mediaMeta;
}

LVP_MediaType MediaPlayer::LVP_Player::GetMediaType()
{
	return (LVP_MediaType)LVP_Player::state.mediaType;
}

LVP_MediaType MediaPlayer::LVP_Player::GetMediaType(const std::string& filePath)
{
	auto formatContext = LVP_Media::GetMediaFormatContext(filePath, false);
	auto mediaType     = (LVP_MediaType)LVP_Media::GetMediaType(formatContext);

	FREE_AVFORMAT(formatContext);

	return mediaType;
}

AVPixelFormat MediaPlayer::LVP_Player::GetPixelFormatHardware()
{
	return (LVP_Player::context->video != NULL ? LVP_Player::context->video->hwPixelFormat : AV_PIX_FMT_NONE);
}

double MediaPlayer::LVP_Player::GetPlaybackSpeed()
{
	return LVP_Player::state.playbackSpeed;
}

int64_t MediaPlayer::LVP_Player::GetProgress()
{
	return (int64_t)(LVP_Player::state.progress * ONE_SECOND_MS_D);
}

SDL_Rect MediaPlayer::LVP_Player::getScaledVideoDestination(const SDL_Rect& destination)
{
	if (SDL_RectEmpty(&destination) || (LVP_Player::context->video->codec == NULL))
		return {};

	int videoWidth   = LVP_Player::context->video->codec->width;
	int videoHeight  = LVP_Player::context->video->codec->height;
	int maxWidth     = (int)((double)videoWidth  / (double)videoHeight * (double)destination.h);
	int maxHeight    = (int)((double)videoHeight / (double)videoWidth  * (double)destination.w);
	int scaledWidth  = std::min(destination.w, maxWidth);
	int scaledHeight = std::min(destination.h, maxHeight);

	SDL_Rect scaledDestination = {
		(destination.x + ((destination.w - scaledWidth)  / 2)),
		(destination.y + ((destination.h - scaledHeight) / 2)),
		scaledWidth,
		scaledHeight
	};

	return scaledDestination;
}

int MediaPlayer::LVP_Player::GetSubtitleTrack()
{
	return (LVP_Player::context->subtitle ? LVP_Player::context->subtitle->index : -1);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::GetSubtitleTracks()
{
	auto extSubFiles = (LVP_Player::context->subtitle ? LVP_Player::context->subtitle->external : LVP_Strings());

	return LVP_Player::getSubtitleTracks(LVP_Player::context->format, extSubFiles);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::getSubtitleTracks(
	AVFormatContext*   formatContext,
	const LVP_Strings& extSubFiles
)
{
	return LVP_Player::getMediaTracks(AVMEDIA_TYPE_SUBTITLE, formatContext, extSubFiles);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::GetVideoTracks()
{
	return LVP_Player::getVideoTracks(LVP_Player::context->format);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::getVideoTracks(AVFormatContext* formatContext)
{
	return LVP_Player::getMediaTracks(AVMEDIA_TYPE_VIDEO, formatContext);
}

double MediaPlayer::LVP_Player::GetVolume()
{
	return (double)LVP_Player::state.volume;
}

void MediaPlayer::LVP_Player::handleSeek()
{
	if (!LVP_Player::state.seek.isRequested)
		return;

	LVP_Player::packetLock.lock();

	LVP_Player::context->audio->packetsLock.lock();
	LVP_Player::context->subtitle->packetsLock.lock();
	LVP_Player::context->video->packetsLock.lock();

	LVP_Player::state.seek.isRequestedBack = false;

	int64_t duration    = (LVP_Player::state.duration * AV_TIME_BASE_I64);
	bool    seekByBytes = (IS_BYTE_SEEK(LVP_Player::context->format->iformat) && (LVP_Player::state.fileSize > 0));
	double  seekPercent = LVP_Player::state.seek.to;

	int64_t seekPosition         = 0;
	int64_t seekPositionExternal = 0;

	if (LVP_Player::state.seek.by != 0)
	{
		auto newPosition = (LVP_Player::state.progress + (double)LVP_Player::state.seek.by);

		if (seekByBytes) {
			seekPercent  = (newPosition / (double)LVP_Player::state.duration);
			seekPosition = (int64_t)((double)LVP_Player::state.fileSize * seekPercent);
		} else {
			seekPosition = (int64_t)(newPosition * AV_TIME_BASE_D);
		}

		if (LVP_Player::context->subtitle->index >= SUB_STREAM_EXTERNAL)
			seekPositionExternal = (int64_t)(newPosition * av_q2d(LVP_Player::context->subtitle->avStream->time_base));

		if (LVP_Player::state.seek.by < 0)
			LVP_Player::state.seek.isRequestedBack = true;
	}
	else
	{
		if (seekByBytes)
			seekPosition = (int64_t)((double)LVP_Player::state.fileSize * seekPercent);
		else
			seekPosition = (int64_t)((double)duration * seekPercent);

		if (LVP_Player::context->subtitle->index >= SUB_STREAM_EXTERNAL)
		{
			auto duration = (double)LVP_Player::state.duration;
			auto timeBase = av_q2d(LVP_Player::context->subtitle->avStream->time_base);

			seekPositionExternal = (int64_t)(duration * timeBase * seekPercent);
		}

		if (seekPercent < (LVP_Player::state.progress / (double)LVP_Player::state.duration))
			LVP_Player::state.seek.isRequestedBack = true;
	}

	auto seekMax = (seekByBytes ? (int64_t)LVP_Player::state.fileSize : duration);

	if (seekPosition < 0)
		seekPosition = 0;
	else if (seekPosition > seekMax)
		seekPosition = seekMax;

	int seekFlags = (LVP_Player::state.seek.isRequestedBack ? AVSEEK_FLAG_BACKWARD : 0);

	if (seekByBytes)
		seekFlags |= AVSEEK_FLAG_BYTE;

	auto result = av_seek_frame(LVP_Player::context->format, -1, seekPosition, seekFlags);

	if (result >= 0)
	{
		if (LVP_Player::context->subtitle->index >= SUB_STREAM_EXTERNAL)
		{
			auto formatContext = LVP_Player::context->formatExternal;
			auto streamIndex   = LVP_Player::context->subtitle->avStream->index;

			avformat_seek_file(formatContext, streamIndex, INT64_MIN, seekPositionExternal, INT64_MAX, 0);
		}

		LVP_SubtitleBitmap::Remove();

		#if defined _ENABLE_VIDEO_AV1_AND_SUBS_ASS
			LVP_SubtitleText::Remove();
		#endif

		LVP_Player::closePackets();

		if (LVP_Player::context->audio->index >= 0)
			LVP_Player::context->audio->packets.push(av_packet_alloc());

		if (LVP_Player::context->subtitle->index >= 0)
			LVP_Player::context->subtitle->packets.push(av_packet_alloc());

		if (LVP_Player::context->video->index >= 0)
			LVP_Player::context->video->packets.push(av_packet_alloc());

		LVP_Player::context->audio->bufferOffset = 0;
		LVP_Player::context->video->pts          = 0;

		if (seekByBytes)
			LVP_Player::state.seek.pts = ((double)LVP_Player::state.duration * seekPercent);
		else
			LVP_Player::state.seek.pts = ((double)seekPosition / AV_TIME_BASE_D);

		if (LVP_Player::state.isPaused)
			LVP_Player::state.progress = LVP_Player::state.seek.pts;
	} else {
		LVP_Player::logAVError(result, "LVP_Player::handleSeek failed");
	}

	LVP_Player::state.seek.by = 0;
	LVP_Player::state.seek.to = 0.0;

	LVP_Player::state.seek.isRequested = false;

	LVP_Player::context->video->packetsLock.unlock();
	LVP_Player::context->subtitle->packetsLock.unlock();
	LVP_Player::context->audio->packetsLock.unlock();

	LVP_Player::packetLock.unlock();
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::handleTrack()
{
	if (!LVP_Player::state.track.requested)
		return;

	LVP_Player::packetLock.lock();

	while (!LVP_Player::state.track.requests.empty())
	{
		const auto& trackRequest = LVP_Player::state.track.requests.front();

		LVP_Player::state.track.requests.pop();

		auto mediaType = (AVMediaType)trackRequest.mediaType;
		bool isAudio   = IS_AUDIO(mediaType);
		bool isSub     = IS_SUB(mediaType);

		bool isSameAudioTrack = (isAudio && (trackRequest.track == LVP_Player::context->audio->index));
		bool isSameSubTrack   = (isSub   && (trackRequest.track == LVP_Player::context->subtitle->index));

		if (isSameAudioTrack || isSameSubTrack)
			continue;

		// DISABLE SUBS

		if (isSub && (trackRequest.track < 0))
		{
			LVP_Player::closeStream(AVMEDIA_TYPE_SUBTITLE);

			LVP_SubtitleBitmap::Remove();

			#if defined _ENABLE_VIDEO_AV1_AND_SUBS_ASS
				LVP_SubtitleText::Remove();
			#endif

			continue;
		}

		bool isPlaying    = LVP_Player::state.isPlaying;
		auto lastProgress = (double)(LVP_Player::state.progress / (double)LVP_Player::state.duration);

		switch (mediaType) {
		case AVMEDIA_TYPE_AUDIO:
			if (isPlaying)
				SDL_PauseAudioDevice(LVP_Player::state.audioDevice.id);

			LVP_Player::closeStream(AVMEDIA_TYPE_AUDIO);

			LVP_Player::context->audio->free();

			LVP_Media::SetMediaTrackByIndex(LVP_Player::context->format, trackRequest.track, LVP_Player::context->audio);

			try {
				LVP_Player::openThreadAudio();
			} catch (const std::exception& e) {
				LVP_Player::CallbackError(e.what());
			}

			if (!LVP_Player::state.seek.isRequested)
				LVP_Player::seekTo(lastProgress);

			if (isPlaying)
				SDL_ResumeAudioDevice(LVP_Player::state.audioDevice.id);

			break;
		case AVMEDIA_TYPE_SUBTITLE:
			LVP_Player::closeStream(AVMEDIA_TYPE_SUBTITLE);

			LVP_SubtitleBitmap::Remove();

			#if defined _ENABLE_VIDEO_AV1_AND_SUBS_ASS
				LVP_SubtitleText::Remove();
			#endif

			if (trackRequest.track >= SUB_STREAM_EXTERNAL)
				LVP_Player::openSubExternal(trackRequest.track);
			else
				LVP_Media::SetMediaTrackByIndex(LVP_Player::context->format, trackRequest.track, LVP_Player::context->subtitle);

			LVP_Player::openThreadSub();

			if (!LVP_Player::state.seek.isRequested)
				LVP_Player::seekTo(lastProgress);

			break;
		default:
			break;
		}
	}

	LVP_Player::state.track.requested = false;

	LVP_Player::packetLock.unlock();
}

bool MediaPlayer::LVP_Player::hasValidAudioDevice()
{
	int nrOfAudioDevices;
	SDL_GetAudioPlaybackDevices(&nrOfAudioDevices);

	return (nrOfAudioDevices > 0);
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::initAudioFilter(AVFrame* frame)
{
	auto abuffer     = avfilter_get_by_name("abuffer");
	auto abuffersink = avfilter_get_by_name("abuffersink");
	auto aresample   = avfilter_get_by_name("aresample");

	auto filterGraph    = avfilter_graph_alloc();
	auto bufferSource   = avfilter_graph_alloc_filter(filterGraph, abuffer,     "src");
	auto bufferSink     = avfilter_graph_alloc_filter(filterGraph, abuffersink, "sink");
	auto filterResample = avfilter_graph_alloc_filter(filterGraph, aresample,   "aresample");

	// https://ffmpeg.org/ffmpeg-filters.html#abuffer

	auto inChannelLayout = LVP_AudioSpecs::getChannelLayoutName(frame->ch_layout);
	auto inSampleFormat  = av_get_sample_fmt_name((AVSampleFormat)frame->format);

	av_opt_set(bufferSource,     "channel_layout", inChannelLayout.c_str(), AV_OPT_SEARCH_CHILDREN);
	av_opt_set(bufferSource,     "sample_fmt",     inSampleFormat,          AV_OPT_SEARCH_CHILDREN);
	av_opt_set_int(bufferSource, "sample_rate",    frame->sample_rate,      AV_OPT_SEARCH_CHILDREN);

	// https://ffmpeg.org/ffmpeg-resampler.html#Resampler-Options

	auto deviceSpecs = LVP_Player::context->audio->deviceSpecs;

	auto outChannelLayout = LVP_AudioSpecs::getChannelLayoutName(LVP_AudioSpecs::getChannelLayout(deviceSpecs.channels));
	auto outSampleFormat  = av_get_sample_fmt_name(LVP_AudioSpecs::getSampleFormat(deviceSpecs.format));
	auto outSampleRate    = LVP_AudioSpecs::getSampleRate(deviceSpecs.freq, LVP_Player::state.playbackSpeed);

	av_opt_set(filterResample,     "out_chlayout",    outChannelLayout.c_str(), AV_OPT_SEARCH_CHILDREN);
	av_opt_set(filterResample,     "out_sample_fmt",  outSampleFormat,          AV_OPT_SEARCH_CHILDREN);
	av_opt_set_int(filterResample, "out_sample_rate", outSampleRate,            AV_OPT_SEARCH_CHILDREN);

	avfilter_init_str(bufferSource,   NULL);
	avfilter_init_str(bufferSink,     NULL);
	avfilter_init_str(filterResample, NULL);

	avfilter_link(bufferSource,   0, filterResample, 0);
	avfilter_link(filterResample, 0, bufferSink, 0);

	if (avfilter_graph_config(filterGraph, NULL) < 0)
		throw std::runtime_error("Failed to initialize a valid audio filter.");

	FREE_AVFILTER_GRAPH(LVP_Player::context->audio->filter.filterGraph);

	LVP_Player::context->audio->filter.filterGraph  = filterGraph;
	LVP_Player::context->audio->filter.bufferSource = bufferSource;
	LVP_Player::context->audio->filter.bufferSink   = bufferSink;
}

bool MediaPlayer::LVP_Player::isHardwarePixelFormat(int frameFormat)
{
	auto format = (AVPixelFormat)frameFormat;

	return ((format != AV_PIX_FMT_NONE) && (format == LVP_Player::context->video->hwPixelFormat));
}

bool MediaPlayer::LVP_Player::IsMuted()
{
	return LVP_Player::state.isMuted;
}

bool MediaPlayer::LVP_Player::IsPaused()
{
	return LVP_Player::state.isPaused;
}

bool MediaPlayer::LVP_Player::IsPlaying()
{
	return LVP_Player::state.isPlaying;
}

bool MediaPlayer::LVP_Player::isPacketQueueFull()
{
	switch (LVP_Player::state.mediaType) {
	case AVMEDIA_TYPE_AUDIO:
		return LVP_Player::isPacketQueueFull(AVMEDIA_TYPE_AUDIO);
	case AVMEDIA_TYPE_VIDEO:
		return (LVP_Player::isPacketQueueFull(AVMEDIA_TYPE_AUDIO) && LVP_Player::isPacketQueueFull(AVMEDIA_TYPE_VIDEO));
	default:
		break;
	}

	return false;
}

bool MediaPlayer::LVP_Player::isPacketQueueFull(AVMediaType streamType)
{
	size_t packetsSize;
	int    streamIndex;

	switch (streamType) {
	case AVMEDIA_TYPE_AUDIO:
		streamIndex = LVP_Player::context->audio->index;
		packetsSize = LVP_Player::context->audio->packets.size();
		break;
	case AVMEDIA_TYPE_SUBTITLE:
		streamIndex = LVP_Player::context->subtitle->index;
		packetsSize = LVP_Player::context->subtitle->packets.size();
		break;
	case AVMEDIA_TYPE_VIDEO:
		streamIndex = LVP_Player::context->video->index;
		packetsSize = LVP_Player::context->video->packets.size();
		break;
	default:
		return false;
	}

	return ((streamIndex >= 0) && (packetsSize >= MIN_PACKET_QUEUE_SIZE));
}

bool MediaPlayer::LVP_Player::IsStopped()
{
	return LVP_Player::state.isStopped;
}

void MediaPlayer::LVP_Player::logAVError(int result, const std::string& message)
{
	char strerror[AV_ERROR_MAX_STRING_SIZE];

	av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);

	LOG("%s: %s\n", message.c_str(), strerror);
}

/**
 * @throws invalid_argument
 */
void MediaPlayer::LVP_Player::Open(const std::string& filePath)
{
	if (!LVP_Player::state.openFilePath.empty() || LVP_Player::state.isOpening)
		return;

	if (filePath.empty())
		throw std::invalid_argument("filePath cannot be empty");

	LVP_Player::state.openFilePath = filePath;
}

void MediaPlayer::LVP_Player::open()
{
	if (LVP_Player::state.openFilePath.empty() || LVP_Player::state.isOpening)
		return;

	LVP_Player::state.isOpening = true;

	LVP_Player::close();

	LVP_Player::context = new LVP_PlayerContext();

	try {
		LVP_Player::openFormatContext();
		LVP_Player::openStreams();
		LVP_Player::openThreads();

		LVP_Player::callbackEvents(LVP_EVENT_MEDIA_OPENED);

		LVP_Player::Play();
	} catch (const std::exception& e) {
		LVP_Player::CallbackError(std::format("Failed to open media file:\n{}", e.what()));
		LVP_Player::close();
	}

	LVP_Player::state.openFilePath = "";
	LVP_Player::state.isOpening    = false;
}

void MediaPlayer::LVP_Player::openAudioDevice(const std::string& name)
{
	if (!LVP_Player::hasValidAudioDevice()) {
		LOG("LVP_Player::openAudioDevice(%s) failed: Couldn't find a valid audio playback device.\n", name.c_str());
		return;
	}

	LVP_Player::closeAudioStream();
	LVP_Player::SetAudioDevices();

	auto deviceId = (name == "Default" ? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK : LVP_Player::state.audioDevices[name]);
		
	LVP_Player::context->audio->audioStream = SDL_OpenAudioDeviceStream(
		deviceId,
		&LVP_Player::context->audio->deviceSpecs,
		LVP_Player::threadAudioCallback,
		NULL
	);

	auto currentDeviceId = SDL_GetAudioStreamDevice(LVP_Player::context->audio->audioStream);

	if (currentDeviceId == 0) {
		LOG("LVP_Player::openAudioDevice(%s) failed: %s\n", name.c_str(), SDL_GetError());
		return;
	}

	LVP_Player::state.audioDevices[name] = currentDeviceId;

	LVP_Player::state.audioDevice = { .id = currentDeviceId, .name = name };
}

/**
 * @throws invalid_argument
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openFormatContext()
{
	if (LVP_Player::state.openFilePath.empty())
		throw std::invalid_argument("filePath cannot be empty");

	if (LVP_Player::context->format == NULL)
	{
		LVP_Player::context->format = LVP_Media::GetMediaFormatContext(
			LVP_Player::state.openFilePath,
			true,
			LVP_Player::context->timeOut
		);
	}

	auto mediaType    = LVP_Media::GetMediaType(LVP_Player::context->format);
	bool isValidMedia = (IS_AUDIO(mediaType) || IS_VIDEO(mediaType));

	if (!isValidMedia) {
		FREE_AVFORMAT(LVP_Player::context->format);
		throw std::runtime_error(std::format("Invalid media type: {}", (int)mediaType));
	}

	auto file     = System::LVP_FileSystem::GetFile(LVP_Player::state.openFilePath);
	auto fileSize = System::LVP_FileSystem::GetFileSize(LVP_Player::state.openFilePath);

	if ((file.ext == "m2ts") && System::LVP_FileSystem::IsBlurayAACS(LVP_Player::state.openFilePath, fileSize))
		isValidMedia = false;
	else if ((file.ext == "vob") && System::LVP_FileSystem::IsDVDCSS(LVP_Player::state.openFilePath, fileSize))
		isValidMedia = false;

	if (!isValidMedia) {
		FREE_AVFORMAT(LVP_Player::context->format);
		throw std::runtime_error("Media is DRM encrypted.");
	}

	LVP_Player::state.filePath     = LVP_Player::state.openFilePath;
	LVP_Player::state.fileSize     = fileSize;
	LVP_Player::state.mediaType    = mediaType;
	LVP_Player::state.openFilePath = "";
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openStreams()
{
	LVP_Player::context->audio->index    = -1;
	LVP_Player::context->subtitle->index = -1;
	LVP_Player::context->video->index    = -1;

	// AUDIO TRACK
	LVP_Media::SetMediaTrackBest(LVP_Player::context->format, AVMEDIA_TYPE_AUDIO, LVP_Player::context->audio);

	if (LVP_Player::context->audio->avStream == NULL)
		throw std::runtime_error("Failed to find a valid audio track.");

	LVP_Player::state.duration = LVP_Media::GetMediaDuration(LVP_Player::context->format, LVP_Player::context->audio->avStream);

	if (IS_VIDEO(LVP_Player::state.mediaType))
	{
		// VIDEO TRACK
		LVP_Media::SetMediaTrackBest(LVP_Player::context->format, AVMEDIA_TYPE_VIDEO, LVP_Player::context->video);

		if (LVP_Player::context->video->avStream == NULL)
			throw std::runtime_error("Failed to find a valid video track.");

		// SUBTITLE TRACK
		LVP_Media::SetMediaTrackBest(LVP_Player::context->format, AVMEDIA_TYPE_SUBTITLE, LVP_Player::context->subtitle);

		auto protocols = std::string(LVP_Player::context->format->protocol_whitelist);

		if (protocols.starts_with("file"))
			LVP_Player::context->subtitle->external = System::LVP_FileSystem::GetSubtitleFilesForVideo(LVP_Player::state.filePath);

		if ((LVP_Player::context->subtitle->avStream == NULL) && !LVP_Player::context->subtitle->external.empty())
			LVP_Player::openSubExternal();
	}

	LVP_Player::state.track.count = LVP_Player::context->format->nb_streams;
}

void MediaPlayer::LVP_Player::openSubExternal(int streamIndex)
{
	if (LVP_Player::context->subtitle->external.empty() || (streamIndex < SUB_STREAM_EXTERNAL))
		return;

	int fileIndex  = ((streamIndex - SUB_STREAM_EXTERNAL) / SUB_STREAM_EXTERNAL);
	int trackIndex = ((streamIndex - SUB_STREAM_EXTERNAL) % SUB_STREAM_EXTERNAL);

	std::string subFile = LVP_Player::context->subtitle->external[fileIndex];

	FREE_AVFORMAT(LVP_Player::context->formatExternal);

	LVP_Player::context->formatExternal = LVP_Media::GetMediaFormatContext(subFile, true);

	if (LVP_Player::context->formatExternal != NULL)
		LVP_Media::SetMediaTrackByIndex(LVP_Player::context->formatExternal, trackIndex, LVP_Player::context->subtitle, fileIndex);
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openThreads()
{
	LVP_Player::openThreadAudio();

	if (LVP_Player::context->video->avStream != NULL)
		LVP_Player::openThreadVideo();

	if (LVP_Player::context->subtitle->avStream != NULL)
		LVP_Player::openThreadSub();

	std::thread(LVP_Player::threadPackets).detach();

	SDL_ResumeAudioDevice(LVP_Player::state.audioDevice.id);
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openThreadAudio()
{
	if (LVP_Player::context->audio->codec == NULL)
		throw std::runtime_error("Audio stream is missing a valid codec.");

	LVP_Player::context->audio->index = LVP_Player::context->audio->avStream->index;

	auto channelCount = LVP_Player::context->audio->codec->ch_layout.nb_channels;
	auto sampleRate   = LVP_Player::context->audio->codec->sample_rate;

	if (channelCount <= 0)
		channelCount = LVP_AudioSpecs::getChannelLayout(2).nb_channels;

	if ((sampleRate <= 0) || (channelCount <= 0))
		throw std::runtime_error(std::format("Invalid audio: {} channels, {} bps", channelCount, sampleRate));

	auto sampleFormat = LVP_AudioSpecs::getSampleFormat(LVP_Player::context->audio->codec->sample_fmt);

	LVP_Player::context->audio->deviceSpecs = {
		.format   = (SDL_AudioFormat)(sampleFormat > 0 ? sampleFormat : SDL_AUDIO_S16),
		.channels = (uint8_t)channelCount,
		.freq     = sampleRate
	};

	// https://stackoverflow.com/questions/11085007/ios-background-audio-stops-when-screen-is-locked

	const int DEFAULT_SAMPLE_COUNT = 4096;

	#if defined _ios && !defined _ENABLE_VIDEO_AV1_AND_SUBS_ASS
		auto sampleCount = DEFAULT_SAMPLE_COUNT;
	#else
		auto sampleCount = LVP_Player::context->audio->codec->frame_size;
	#endif

	auto samples = (sampleCount > 0 ? sampleCount : DEFAULT_SAMPLE_COUNT);

	SDL_SetHintWithPriority(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, std::to_string(samples).c_str(), SDL_HINT_OVERRIDE);

	LVP_Player::openAudioDevice(LVP_Player::state.audioDevice.name);

	if (!LVP_Player::state.threads[LVP_THREAD_AUDIO])
		std::thread(LVP_Player::threadAudio).detach();
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openThreadSub()
{
	#if defined _DEBUG
		auto extradata = LVP_Player::context->subtitle->codec->extradata;
		auto header    = LVP_Player::context->subtitle->codec->subtitle_header;
		auto subHeader = (header != NULL ? std::string(reinterpret_cast<char*>(header)) : "");

		if (subHeader.empty())
			subHeader = (extradata != NULL ? std::string(reinterpret_cast<char*>(extradata)) : "");

		LOG("\n%s\n", subHeader.c_str());
	#endif

	if (!LVP_Player::state.threads[LVP_THREAD_SUBTITLE])
		std::thread(LVP_Player::threadSub).detach();

	LVP_Player::context->subtitle->formatContext = LVP_Player::context->format;

	LVP_Player::context->subtitle->videoSize = {
		LVP_Player::context->video->codec->width,
		LVP_Player::context->video->codec->height
	};

	#if defined _ENABLE_VIDEO_AV1_AND_SUBS_ASS
		LVP_SubtitleText::Init(LVP_Player::context->subtitle);
	#endif
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openThreadVideo()
{
	if (LVP_Player::context->video->codec == NULL)
		throw std::runtime_error("Video stream is missing a valid codec.");

	int videoWidth  = LVP_Player::context->video->codec->width;
	int videoHeight = LVP_Player::context->video->codec->height;

	// SURFACE

	LVP_Player::context->video->surface = SDL_CreateSurface(videoWidth, videoHeight, SDL_PIXELFORMAT_RGBA32);

	// RENDERER

	LVP_Player::context->video->isSoftwareRenderer = (LVP_Player::callback.hardwareRenderer == NULL);

	if (LVP_Player::context->video->isSoftwareRenderer)
		LVP_Player::context->video->renderer = SDL_CreateSoftwareRenderer(LVP_Player::context->video->surface);
	else
		LVP_Player::context->video->renderer = LVP_Player::callback.hardwareRenderer;

	if (LVP_Player::context->video->renderer == NULL)
		throw std::runtime_error(std::format("Failed to create a renderer: {}", SDL_GetError()));

	// TEXTURE

	LVP_Player::context->video->texture = SDL_CreateTexture(
		LVP_Player::context->video->renderer,
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING,
		videoWidth,
		videoHeight
	);

	SDL_SetTextureBlendMode(LVP_Player::context->video->texture, SDL_BLENDMODE_NONE);

	// FRAME

	LVP_Player::context->video->frameHardware = av_frame_alloc();
	LVP_Player::context->video->frameSoftware = av_frame_alloc();

	if ((LVP_Player::context->video->frameHardware == NULL) || (LVP_Player::context->video->frameSoftware == NULL))
		throw std::runtime_error("Failed to allocate a video context frame.");

	if (LVP_Player::context->video->frameEncoded != NULL)
		LVP_Player::context->video->frameEncoded->linesize[0] = 0;

	// THREAD

	if (!LVP_Player::state.threads[LVP_THREAD_VIDEO])
		std::thread(LVP_Player::threadVideo).detach();

	// STREAM/TRACK

	LVP_Player::context->video->index = LVP_Player::context->video->avStream->index;
}

void MediaPlayer::LVP_Player::Pause()
{
	if (LVP_Player::state.isPaused || LVP_Player::state.filePath.empty())
		return;

	LVP_Player::state.isPaused  = true;
	LVP_Player::state.isPlaying = false;
	LVP_Player::state.isStopped = false;

	SDL_PauseAudioDevice(LVP_Player::state.audioDevice.id);

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_PAUSED);
}

void MediaPlayer::LVP_Player::Play()
{
	if (LVP_Player::state.isPlaying || LVP_Player::state.filePath.empty())
		return;

	LVP_Player::state.isPlaying = true;
	LVP_Player::state.isPaused  = false;
	LVP_Player::state.isStopped = false;

	SDL_ResumeAudioDevice(LVP_Player::state.audioDevice.id);

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_PLAYING);
}

void MediaPlayer::LVP_Player::Quit()
{
	LVP_Player::close();
}

void MediaPlayer::LVP_Player::renderVideo()
{
	if ((LVP_Player::context->video->index < 0) ||
		(LVP_Player::context->video->codec == NULL) ||
		(LVP_Player::context->video->frame == NULL))
	{
		return;
	}

	if (LVP_Player::context->video->frameEncoded == NULL)
	{
		LVP_Player::context->video->frameEncoded = av_frame_alloc();

		auto result = av_image_alloc(
			LVP_Player::context->video->frameEncoded->data,
			LVP_Player::context->video->frameEncoded->linesize,
			LVP_Player::context->video->codec->width,
			LVP_Player::context->video->codec->height,
			AV_PIX_FMT_RGBA,
			1
		);

		if (result <= 0)
			FREE_AVFRAME(LVP_Player::context->video->frameEncoded);
	}

	if (LVP_Player::context->video->frameEncoded == NULL)
		return;

	LVP_Player::packetLock.lock();

	auto pixelFormat = (AVPixelFormat)LVP_Player::context->video->frame->format;

	LVP_Player::packetLock.unlock();

	if (pixelFormat == AV_PIX_FMT_NONE)
		return;

	LVP_Player::context->video->scaleContext = sws_getCachedContext(
		LVP_Player::context->video->scaleContext,
		LVP_Player::context->video->codec->width,
		LVP_Player::context->video->codec->height,
		pixelFormat,
		LVP_Player::context->video->codec->width,
		LVP_Player::context->video->codec->height,
		AV_PIX_FMT_RGBA,
		DEFAULT_SCALE_FILTER,
		NULL,
		NULL,
		NULL
	);

	if (LVP_Player::context->video->scaleContext == NULL)
		return;

	LVP_Player::packetLock.lock();

	auto scaleResult = sws_scale_frame(
		LVP_Player::context->video->scaleContext,
		LVP_Player::context->video->frameEncoded,
		LVP_Player::context->video->frame
	);

	LVP_Player::packetLock.unlock();

	if (scaleResult <= 0)
		return;

	auto size = (size_t)(scaleResult * LVP_Player::context->video->frameEncoded->linesize[0]);

	std::memcpy(LVP_Player::context->video->surface->pixels, LVP_Player::context->video->frameEncoded->data[0], size);
}

void MediaPlayer::LVP_Player::Resize()
{
	if (LVP_Player::state.isStopped)
		return;

	LVP_Player::context->video->isReadyForRender  = true;
	LVP_Player::context->video->isReadyForPresent = true;
}

void MediaPlayer::LVP_Player::Run(const SDL_Rect& destination)
{
	if (LVP_Player::state.isCompleted || LVP_Player::state.quit)
		LVP_Player::close();

	if (LVP_Player::state.isStopping)
		return;

	if (!LVP_Player::state.openFilePath.empty())
		LVP_Player::open();

	if (LVP_Player::state.isStopped)
		return;

	LVP_Player::handleTrack();
	LVP_Player::handleSeek();

	if (LVP_Player::context->video->isReadyForRender)
	{
		LVP_Player::context->video->isReadyForRender = false;

		LVP_Player::renderVideo();

		LVP_SubtitleBitmap::RemoveExpired(LVP_Player::state.progress);

		LVP_SubtitleBitmap::Render(LVP_Player::context->video->surface, LVP_Player::context->subtitle, LVP_Player::state.progress);
		
		#if defined _ENABLE_VIDEO_AV1_AND_SUBS_ASS
			LVP_SubtitleText::Render(LVP_Player::context->video->surface, LVP_Player::state.progress);
		#endif
	}

	if (LVP_Player::context->video->isReadyForPresent && (LVP_Player::context->video->surface != NULL))
	{
		LVP_Player::context->video->isReadyForPresent = false;

		auto surface = LVP_Player::context->video->surface;

		if (!LVP_Player::context->video->isSoftwareRenderer)
			SDL_UpdateTexture(LVP_Player::context->video->texture, NULL, surface->pixels, surface->pitch);
		else
			LVP_Player::callbackVideoIsAvailable(surface);
	}

	if (!LVP_Player::context->video->isSoftwareRenderer && (LVP_Player::context->video->texture != NULL))
	{
		auto scaledDest = LVP_Player::getScaledVideoDestination(destination);

		if (!SDL_RectEmpty(&scaledDest))
		{
			SDL_FRect scaledDestF;
			SDL_RectToFRect(&scaledDest, &scaledDestF);

			SDL_RenderTexture(LVP_Player::context->video->renderer, LVP_Player::context->video->texture, NULL, &scaledDestF);
		}
	}
}

void MediaPlayer::LVP_Player::SeekBy(int seconds)
{
	if (LVP_Player::context->format == NULL)
		return;

	LVP_Player::packetLock.lock();

	LVP_Player::state.seek.by += seconds;

	LVP_Player::state.seek.isRequested  = true;

	if (IS_VIDEO(LVP_Player::state.mediaType) && LVP_Player::state.isPaused)
		LVP_Player::state.seek.isRequestedPaused = true;

	LVP_Player::packetLock.unlock();
}

void MediaPlayer::LVP_Player::SeekTo(double percent)
{
	if (LVP_Player::context->format == NULL)
		return;

	auto validPercent = std::max(0.0, std::min(1.0, percent));

	LVP_Player::packetLock.lock();

	LVP_Player::seekTo(validPercent);

	LVP_Player::packetLock.unlock();
}

void MediaPlayer::LVP_Player::seekTo(double percent)
{
	LVP_Player::state.seek.to = percent;

	LVP_Player::state.seek.isRequested = true;

	if (IS_VIDEO(LVP_Player::state.mediaType) && LVP_Player::state.isPaused)
		LVP_Player::state.seek.isRequestedPaused = true;
}

void MediaPlayer::LVP_Player::SetAudioDevice(const std::string& name)
{
	if (LVP_Player::state.isStopped)
	{
		LVP_Player::state.audioDevice = {
			.id   = LVP_Player::state.audioDevices[name],
			.name = name
		};

		return;
	}

	bool isPlaying = LVP_Player::state.isPlaying;

	if (isPlaying)
		SDL_PauseAudioDevice(LVP_Player::state.audioDevice.id);

	LVP_Player::openAudioDevice(name);

	if (isPlaying)
		SDL_ResumeAudioDevice(LVP_Player::state.audioDevice.id);
}

void MediaPlayer::LVP_Player::SetAudioDevices()
{
	LVP_Player::state.audioDevices.clear();

	LVP_Player::state.audioDevices["Default"] = 0;

	int  nrOfAudioDevices;
	auto deviceIds = SDL_GetAudioPlaybackDevices(&nrOfAudioDevices);

	for (int i = 0; i < nrOfAudioDevices; i++)
		LVP_Player::state.audioDevices[SDL_GetAudioDeviceName(deviceIds[i])] = deviceIds[i];
}

void MediaPlayer::LVP_Player::setAudioPacketDuration(AVPacket* packet)
{
	if ((packet == NULL) || (packet->duration <= 0))
		return;

	auto packetDuration = ((double)packet->duration / LVP_Player::state.playbackSpeed);
	auto streamTimeBase = av_q2d(LVP_Player::context->audio->avStream->time_base);

	LVP_Player::context->audio->packetDuration = (packetDuration * streamTimeBase);
}

void MediaPlayer::LVP_Player::setAudioProgress(AVFrame* frame)
{
	if (frame == NULL)
		return;

	// Save last valid progress
	
	if (LVP_Player::state.progress > 0)
		LVP_Player::context->audio->lastPogress = LVP_Player::state.progress;
	
	// Calculate current progress from timestamp (pts) or last progress and packet duration

	LVP_Player::state.progress = LVP_Media::GetAudioPTS(LVP_Player::context->audio, frame);
	
	// Manually calculate packet duration (if not available) based on last progress

	if ((LVP_Player::context->audio->packetDuration <= 0) &&
		(LVP_Player::state.progress > 0) &&
		(LVP_Player::context->audio->lastPogress > 0) &&
		(LVP_Player::state.progress != LVP_Player::context->audio->lastPogress))
	{
		LVP_Player::context->audio->packetDuration = (LVP_Player::state.progress - LVP_Player::context->audio->lastPogress);
		LVP_Player::context->audio->lastPogress    = LVP_Player::state.progress;
	}
	
	// Correct invalid progress when seeking backwards

	if (IS_INVALID_PTS())
		LVP_Player::state.seek.pts += LVP_Player::context->audio->packetDuration;
}

void MediaPlayer::LVP_Player::SetMuted(bool muted)
{
	LVP_Player::state.isMuted = muted;

	if (LVP_Player::state.isMuted)
		LVP_Player::callbackEvents(LVP_EVENT_AUDIO_MUTED);
	else
		LVP_Player::callbackEvents(LVP_EVENT_AUDIO_UNMUTED);
}

void MediaPlayer::LVP_Player::SetPlaybackSpeed(double speed)
{
	auto validSpeed = std::max(0.5, std::min(2.0, speed));

	LVP_Player::state.playbackSpeed = validSpeed;

	LVP_Player::callbackEvents(LVP_EVENT_PLAYBACK_SPEED_CHANGED);
}

void MediaPlayer::LVP_Player::SetTrack(const LVP_MediaTrack& track)
{
	if (LVP_Player::context->format == NULL)
		return;

	LVP_Player::packetLock.lock();

	LVP_Player::state.track.requests.push(track);

	LVP_Player::state.track.requested = true;

	LVP_Player::packetLock.unlock();
}

void MediaPlayer::LVP_Player::SetVolume(double percent)
{
	LVP_Player::state.volume = std::max(0.0f, std::min(1.0f, (float)percent));

	LVP_Player::callbackEvents(LVP_EVENT_AUDIO_VOLUME_CHANGED);
}

void MediaPlayer::LVP_Player::stop(const std::string& errorMessage)
{
	LVP_Player::state.quit = true;
	
	if (!errorMessage.empty())
		LVP_Player::CallbackError(errorMessage);
}

int MediaPlayer::LVP_Player::threadAudio()
{
	LVP_Player::state.threads[LVP_THREAD_AUDIO] = true;

	while (!LVP_Player::state.quit)
	{
		while (!LVP_Player::state.quit && (LVP_Player::state.audioDevice.id > 0) && (
			LVP_Player::state.isPaused ||
			LVP_Player::state.seek.isRequested ||
			LVP_Player::state.track.requested ||
			LVP_Player::context->audio->packets.empty() ||
			(LVP_Player::context->audio->frames.size() >= MIN_PACKET_QUEUE_SIZE)))
		{
			SDL_Delay(DELAY_TIME_DEFAULT);
		}

		if (LVP_Player::state.quit)
			break;

		auto packet = LVP_Player::getMediaPacket(LVP_Player::context->audio);

		if (packet == NULL)
			continue;

		LVP_Player::setAudioPacketDuration(packet);

		if (LVP_Player::state.audioDevice.id > 0)
		{
			LVP_Player::decodeAudioPacket(packet);
			LVP_Player::decodeAudioFrames();
		}
		else
		{
			LVP_Player::context->audio->free();

			auto duration = (uint32_t)(LVP_Player::context->audio->packetDuration * ONE_SECOND_MS_D);
			auto timeBase = av_q2d(LVP_Player::context->audio->avStream->time_base);

			if (packet->pts >= 0)
				LVP_Player::state.progress = ((double)packet->pts * timeBase);

			SDL_Delay(duration);
		}

		FREE_AVPACKET(packet);
	}

	LVP_Player::state.threads[LVP_THREAD_AUDIO] = false;

	return 0;
}

void MediaPlayer::LVP_Player::threadAudioCallback(void* userData, SDL_AudioStream* stream, int remaining, int total)
{
	if (remaining <= 0)
		return;

	LVP_Player::state.threads[LVP_THREAD_AUDIO_CALLBACK] = true;

	auto streamData  = (uint8_t*)std::malloc(total);
	auto streamStart = streamData;


	while (remaining > 0)
	{
		if (LVP_Player::context->audio->bufferOffset >= LVP_Player::context->audio->dataSize)
		{
			auto dataSize = LVP_Player::decodeAudioFrame();

			if (dataSize < 0)
			{
				auto sampleFormat = LVP_AudioSpecs::getSampleFormat(LVP_Player::context->audio->deviceSpecs.format);
				auto sampleSize   = av_samples_get_buffer_size(NULL, LVP_Player::context->audio->deviceSpecs.channels, 1, sampleFormat, 1);

				FREE_POINTER(LVP_Player::context->audio->buffer);

				LVP_Player::context->audio->dataSize = (512 / sampleSize * sampleSize);
			} else {
				LVP_Player::context->audio->dataSize = dataSize;
			}

			LVP_Player::context->audio->bufferOffset = 0;
		}

		int writeSize = (LVP_Player::context->audio->dataSize - LVP_Player::context->audio->bufferOffset);

		if (writeSize > remaining)
			writeSize = remaining;

		if (LVP_Player::context->audio->buffer != NULL)
		{
			auto buffer = (LVP_Player::context->audio->buffer + LVP_Player::context->audio->bufferOffset);
			auto volume = (!LVP_Player::state.isMuted ? LVP_Player::state.volume : 0.0f);

			if (volume < 1.0f) {
				std::memset(streamData, 0, writeSize);
				SDL_MixAudio(streamData, buffer, LVP_Player::context->audio->deviceSpecs.format, writeSize, volume);
			} else {
				std::memcpy(streamData, buffer, writeSize);
			}

			SDL_PutAudioStreamData(stream, streamData, writeSize);
		}

		remaining  -= writeSize;
		streamData += writeSize;

		LVP_Player::context->audio->bufferOffset += writeSize;
	}

	std::free(streamStart);

	LVP_Player::state.threads[LVP_THREAD_AUDIO_CALLBACK] = false;
}

int MediaPlayer::LVP_Player::threadPackets()
{
	LVP_Player::state.threads[LVP_THREAD_PACKETS] = true;

	bool endOfFile  = false;
	int  errorCount = 0;

	while (!LVP_Player::state.quit)
	{
		while (LVP_Player::state.seek.isRequested && !LVP_Player::state.quit)
			SDL_Delay(DELAY_TIME_DEFAULT);

		if (LVP_Player::state.quit)
			break;

		// Initialize a new packet

		auto packet = av_packet_alloc();

		if (packet == NULL)
		{
			errorCount = MAX_ERRORS;

			LVP_Player::stop("Failed to allocate packet.");
			break;
		}

		if (LVP_Player::context->timeOut != NULL)
			LVP_Player::context->timeOut->start();

		// Read and fill the packet

		LVP_Player::packetLock.lock();

		auto result = av_read_frame(LVP_Player::context->format, packet);

		LVP_Player::packetLock.unlock();

		if (LVP_Player::state.quit) {
			FREE_AVPACKET(packet);
			break;
		}

		if (result < 0)
		{
			FREE_AVPACKET(packet);

			if (LVP_Player::context->timeOut != NULL)
				LVP_Player::context->timeOut->stop();

			while (LVP_Player::state.isPaused && !LVP_Player::state.seek.isRequested && !LVP_Player::state.quit)
				SDL_Delay(DELAY_TIME_DEFAULT);

			if (LVP_Player::state.seek.isRequested)
				continue;

			// Media file has completed (EOF) or an error occured

			if ((result == AVERROR_EOF) || avio_feof(LVP_Player::context->format->pb))
			{
				if (!endOfFile) {
					endOfFile = true;
					continue;
				}
			}
			else
			{
				LVP_Player::logAVError(result, "LVP_Player::threadPackets failed");

				errorCount++;

				if (errorCount < MAX_ERRORS)
					continue;
			}

			if (LVP_Player::state.quit)
				break;

			// Wait until all packets have been consumed

			while (!LVP_Player::context->audio->packets.empty() && !LVP_Player::state.quit)
				SDL_Delay(DELAY_TIME_DEFAULT);

			if (LVP_Player::state.quit)
				break;

			// Stop media playback

			if (errorCount >= MAX_ERRORS)
				LVP_Player::stop("Too many errors occured while reading packets.");
			else
				LVP_Player::stop();

			break;
		}
		else
		{
			endOfFile  = false;
			errorCount = 0;
		}

		if (LVP_Player::context->timeOut != NULL)
			LVP_Player::context->timeOut->stop();

		// INTERNAL STREAMS

		bool isSubExternal = (LVP_Player::context->subtitle->index >= SUB_STREAM_EXTERNAL);

		if (packet->stream_index == LVP_Player::context->audio->index)
		{
			LVP_Player::context->audio->packetsLock.lock();
			LVP_Player::context->audio->packets.push(packet);
			LVP_Player::context->audio->packetsLock.unlock();
		}
		else if (packet->stream_index == LVP_Player::context->video->index)
		{
			LVP_Player::context->video->packetsLock.lock();
			LVP_Player::context->video->packets.push(packet);
			LVP_Player::context->video->packetsLock.unlock();
		}
		else if ((packet->stream_index == LVP_Player::context->subtitle->index) && !isSubExternal)
		{
			LVP_Player::context->subtitle->packetsLock.lock();
			LVP_Player::context->subtitle->packets.push(packet);
			LVP_Player::context->subtitle->packetsLock.unlock();
		} else {
			FREE_AVPACKET(packet);
		}

		// EXTERNAL SUBTITLE FILES

		if (isSubExternal && (LVP_Player::context->subtitle->packets.size() < MIN_PACKET_QUEUE_SIZE))
		{
			auto packetExternal = av_packet_alloc();
			
			LVP_Player::packetLock.lock();

			auto result = av_read_frame(LVP_Player::context->formatExternal, packetExternal);

			LVP_Player::packetLock.unlock();

			int streamExternalIndex = ((LVP_Player::context->subtitle->index - SUB_STREAM_EXTERNAL) % SUB_STREAM_EXTERNAL);

			if ((result >= 0) && (packetExternal->stream_index == streamExternalIndex)) {
				LVP_Player::context->subtitle->packetsLock.lock();
				LVP_Player::context->subtitle->packets.push(packetExternal);
				LVP_Player::context->subtitle->packetsLock.unlock();
			} else {
				FREE_AVPACKET(packetExternal);
			}
		}

		if (LVP_Player::state.quit)
			break;

		// Number of media tracks have changed

		if (LVP_Player::context->format->nb_streams != LVP_Player::state.track.count)
		{
			LVP_Player::state.track.count = LVP_Player::context->format->nb_streams;

			LVP_Player::callbackEvents(LVP_EVENT_MEDIA_TRACKS_UPDATED);
		}

		// Metadata has changed

		if (LVP_Player::context->format->event_flags & AVFMT_EVENT_FLAG_METADATA_UPDATED)
		{
			LVP_Player::context->format->event_flags &= ~AVFMT_EVENT_FLAG_METADATA_UPDATED;

			LVP_Player::callbackEvents(LVP_EVENT_METADATA_UPDATED);
		}

		// Wait while packet queues are full

		while (LVP_Player::isPacketQueueFull() && !LVP_Player::state.seek.isRequested && !LVP_Player::state.quit)
			SDL_Delay(DELAY_TIME_DEFAULT);
	}

	if (endOfFile || (errorCount >= MAX_ERRORS))
	{
		if (errorCount >= MAX_ERRORS)
			LVP_Player::callbackEvents(LVP_EVENT_MEDIA_COMPLETED_WITH_ERRORS);
		else
			LVP_Player::callbackEvents(LVP_EVENT_MEDIA_COMPLETED);

		LVP_Player::state.isCompleted = true;
	}

	LVP_Player::state.threads[LVP_THREAD_PACKETS] = false;

	return 0;
}

int MediaPlayer::LVP_Player::threadSub()
{
	LVP_Player::state.threads[LVP_THREAD_SUBTITLE] = true;

	AVSubtitle subFrame = {};

	while (!LVP_Player::state.quit)
	{
		// Wait until subtitle packets are available

		while (!LVP_Player::state.quit && (
			(LVP_Player::state.isPaused && !LVP_Player::state.seek.isRequestedPaused) ||
			LVP_Player::state.seek.isRequested ||
			LVP_Player::state.track.requested ||
			(LVP_Player::context->subtitle->index < 0) ||
			LVP_Player::context->subtitle->packets.empty()
		)) {
			SDL_Delay(DELAY_TIME_DEFAULT);
		}

		if (LVP_Player::state.quit)
			break;

		// Get subtitle packet from queue

		auto packet = LVP_Player::getMediaPacket(LVP_Player::context->subtitle);

		if (packet == NULL)
			continue;

		auto trackIndex = LVP_Player::context->subtitle->index;

		auto packetPTS = LVP_Media::GetPacketPTS(
			packet,
			LVP_Player::context->subtitle->avStream->time_base,
			LVP_Player::context->audio->avStream->start_time
		);

		// Sub is behind audio, skip packet to catch up.

		auto packetDelayStart = (packetPTS.start - LVP_Player::state.progress);
		auto packetDelayEnd   = (packetPTS.end   - LVP_Player::state.progress);

		if ((packetPTS.end > 0.0) && ((packetDelayStart <= MAX_SUB_DELAY) || (packetDelayEnd <= 0.0)))
		{
			#if defined _DEBUG
				LOG("SUB_PACKET_DELAY: %.3fs [%.3fs]\n", packetDelayStart, packetDelayEnd);
			#endif

			if (packetDelayEnd <= 0.0)
				continue;
		}

		bool isExternal = (LVP_Player::context->subtitle->index >= SUB_STREAM_EXTERNAL);

		while (isExternal &&
			((packetPTS.start - LVP_Player::state.progress) > 1.0) &&
			!LVP_Player::state.seek.isRequested &&
			!LVP_Player::state.quit)
		{
			SDL_Delay(1);
		}

		if (LVP_Player::state.quit) {
			FREE_AVPACKET(packet);
			break;
		}

		// Decode subtitle packet to frame

		LVP_Player::packetLock.lock();

		if (LVP_Player::context->subtitle->index != trackIndex) {
			FREE_AVPACKET(packet);
			LVP_Player::packetLock.unlock();
			continue;
		}

		int  frameDecoded  = 0;
		auto subtitleCodec = LVP_Player::context->subtitle->codec;

		auto decodeResult = avcodec_decode_subtitle2(subtitleCodec, &subFrame, &frameDecoded, packet);

		LVP_Player::packetLock.unlock();

		if ((decodeResult < 0) || !frameDecoded) {
			FREE_SUB_FRAME(subFrame);
			FREE_AVPACKET(packet);
			continue;
		}

		if (LVP_Player::state.quit)
			break;

		auto subStreamTimeBase = LVP_Player::context->subtitle->avStream->time_base;

		switch (subtitleCodec->codec->id) {
		case AV_CODEC_ID_DVD_SUBTITLE:
			LVP_SubtitleBitmap::UpdateDVDColorPalette(subtitleCodec->priv_data);
			break;
		case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
			if (subFrame.num_rects == 0)
				LVP_SubtitleBitmap::UpdatePGSEndPTS(LVP_Media::GetSubtitlePGSEndPTS(packet, subStreamTimeBase));
			break;
		default:
			break;
		}

		if (subFrame.num_rects == 0)
		{
			FREE_AVPACKET(packet);
			FREE_SUB_FRAME(subFrame);

			continue;
		}

		auto audioStreamTimeBase = LVP_Player::context->audio->avStream->start_time;

		auto framePTS = LVP_Media::GetSubtitlePTS(packet, subFrame, subStreamTimeBase, audioStreamTimeBase);

		FREE_AVPACKET(packet);

		if (framePTS.end == 0.0)
			LVP_SubtitleBitmap::UpdatePGSEndPTS(framePTS.start);

		// Sub is behind audio, skip frame to catch up.

		auto frameDelayStart = (framePTS.start - LVP_Player::state.progress);
		auto frameDelayEnd   = (framePTS.end   - LVP_Player::state.progress);

		if ((framePTS.end > 0.0) && ((frameDelayStart <= MAX_SUB_DELAY) || (frameDelayEnd <= 0.0)))
		{
			#if defined _DEBUG
				LOG("SUB_FRAME_DELAY: %.3fs [%.3fs]\n", frameDelayStart, frameDelayEnd);
			#endif

			if (frameDelayEnd <= 0.0)
				continue;
		}

		// Process subtitle frame content

		for (uint32_t i = 0; i < subFrame.num_rects; i++)
		{
			if (subFrame.rects[i] == NULL)
				continue;

			auto sub = new LVP_Subtitle(*subFrame.rects[i], framePTS);

			switch (sub->type) {
			case SUBTITLE_BITMAP:
				#if defined _DEBUG
				if (sub->pts.end != 0.0)
					LOG("[%.3f,%.3f] %d,%d %dx%d\n", sub->pts.start, sub->pts.end, sub->bitmap.x, sub->bitmap.y, sub->bitmap.w, sub->bitmap.h);
				#endif

				LVP_SubtitleBitmap::ProcessEvent(sub, *subFrame.rects[i]);

				LVP_SubtitleBitmap::queueLock.lock();

				LVP_SubtitleBitmap::queue.push(sub);

				LVP_SubtitleBitmap::queueLock.unlock();
				break;
			case SUBTITLE_ASS:
			case SUBTITLE_TEXT:
				#if defined _DEBUG
				if (sub->dialogue.find("\\vr") == std::string::npos)
					LOG("[%.3f,%.3f] %s\n", sub->pts.start, sub->pts.end, sub->dialogue.c_str());
				#endif

				#if defined _ENABLE_VIDEO_AV1_AND_SUBS_ASS
					LVP_SubtitleText::ProcessEvent(sub);
				#endif

				DELETE_POINTER(sub);
				break;
			default:
				break;
			}
		}

		FREE_SUB_FRAME(subFrame);
	}

	FREE_SUB_FRAME(subFrame);

	LVP_Player::state.threads[LVP_THREAD_SUBTITLE] = false;

	return 0;
}

int MediaPlayer::LVP_Player::threadVideo()
{
	LVP_Player::state.threads[LVP_THREAD_VIDEO] = true;

	int errorCount = 0;

	while (!LVP_Player::state.quit)
	{
		// Wait until video packets are available

		while ((LVP_Player::context->video->packets.empty() || LVP_Player::state.seek.isRequested) && !LVP_Player::state.quit)
			SDL_Delay(DELAY_TIME_DEFAULT);

		if (LVP_Player::state.quit)
			break;

		// Get video packet from queue

		auto packet = LVP_Player::getMediaPacket(LVP_Player::context->video);

		if (packet == NULL)
			continue;

		// Decode video packet to frame

		LVP_Player::packetLock.lock();

		auto result = avcodec_send_packet(LVP_Player::context->video->codec, packet);

		LVP_Player::packetLock.unlock();

		if ((result < 0) || LVP_Player::state.quit)
		{
			if ((result == AVERROR(EAGAIN)) || (result == AVERROR_EOF))
				continue;

			FREE_AVPACKET(packet);

			LVP_Player::logAVError(result, "LVP_Player::threadVideo failed");

			errorCount++;

			if (errorCount >= MAX_ERRORS) {
				LVP_Player::stop("Failed to decode video.");
				break;
			}

			continue;
		}

		while (result >= 0)
		{
			LVP_Player::packetLock.lock();

			result = avcodec_receive_frame(LVP_Player::context->video->codec, LVP_Player::context->video->frameHardware);

			LVP_Player::packetLock.unlock();

			if ((result == AVERROR(EAGAIN)) || (result == AVERROR_EOF))
				break;

			if (result < 0) {
				LVP_Player::logAVError(result, "LVP_Player::threadVideo failed");
				errorCount++;
				break;
			}

			// Process video frame content

			LVP_Player::packetLock.lock();

			auto frameHardware = LVP_Player::context->video->frameHardware;

			if (LVP_Player::isHardwarePixelFormat(frameHardware->format))
			{
				av_hwframe_transfer_data(LVP_Player::context->video->frameSoftware, frameHardware, 0);

				LVP_Player::context->video->frameSoftware->best_effort_timestamp = frameHardware->best_effort_timestamp;

				LVP_Player::context->video->frameSoftware->pts     = frameHardware->pts;
				LVP_Player::context->video->frameSoftware->pkt_dts = frameHardware->pkt_dts;

				LVP_Player::context->video->frame = LVP_Player::context->video->frameSoftware;
			} else {
				LVP_Player::context->video->frame = frameHardware;
			}

			// Calculate video PTS (present time)

			if (IS_AUDIO(LVP_Player::state.mediaType) && (LVP_Player::context->video->avStream->attached_pic.size > 0))
				LVP_Player::context->video->pts = LVP_Player::state.progress;
			else
				LVP_Player::context->video->pts = LVP_Media::GetVideoPTS(LVP_Player::context->video, LVP_Player::context->audio->avStream->start_time);

			LVP_Player::packetLock.unlock();

			// Wait while paused (unless a seek is requested)

			while (LVP_Player::state.isPaused &&
				!LVP_Player::state.seek.isRequested &&
				!LVP_Player::state.seek.isRequestedPaused &&
				!LVP_Player::state.quit)
			{
				SDL_Delay(DELAY_TIME_DEFAULT);
			}

			if (LVP_Player::state.seek.isRequested || LVP_Player::state.quit)
				break;

			// Update video if seek was requested while paused

			if (LVP_Player::state.seek.isRequestedPaused)
			{
				// Keep seeking until the video catches up to the audio

				if (LVP_Player::context->video->getTimeUntilPTS(LVP_Player::state.progress) > ONE_SECOND_MS)
					break;

				LVP_Player::state.seek.isRequestedPaused = false;
			}

			// Start preparing the video frame for rendering

			LVP_Player::context->video->isReadyForRender = true;

			// Wait before presenting the video frame

			auto progress = (IS_INVALID_PTS() ? LVP_Player::state.seek.pts : LVP_Player::state.progress);

			while ((LVP_Player::context->video->getTimeUntilPTS(progress) >= DELAY_TIME_DEFAULT) &&
				!LVP_Player::state.isPaused &&
				!LVP_Player::state.quit)
			{
				SDL_Delay(DELAY_TIME_DEFAULT);

				progress = (IS_INVALID_PTS() ? LVP_Player::state.seek.pts : LVP_Player::state.progress);
			}

			if (!IS_INVALID_PTS())
				LVP_Player::state.seek.isRequestedBack = false;

			LVP_Player::context->video->isReadyForPresent = true;
		}

		FREE_AVPACKET(packet);

		if ((result == AVERROR(EAGAIN)) || (result == AVERROR_EOF) || (result >= 0))
			errorCount = 0;
	}

	LVP_Player::state.threads[LVP_THREAD_VIDEO] = false;

	return 0;
}

void MediaPlayer::LVP_Player::ToggleMute()
{
	LVP_Player::SetMuted(!LVP_Player::state.isMuted);
}

void MediaPlayer::LVP_Player::TogglePause()
{
	if (!LVP_Player::state.isPaused)
		LVP_Player::Pause();
	else
		LVP_Player::Play();
}

void MediaPlayer::LVP_Player::UpdateAudioDevices()
{
	bool isPlaying = LVP_Player::state.isPlaying;

	if (isPlaying)
		LVP_Player::Pause();

	if ((LVP_Player::context != NULL) && (LVP_Player::context->audio != NULL) && (LVP_Player::context->audio->audioStream == NULL))
		LVP_Player::openAudioDevice(LVP_Player::state.audioDevice.name);
	else
		LVP_Player::SetAudioDevices();

	if (!LVP_Player::hasValidAudioDevice())
	{
		LVP_Player::closeAudioStream();

		LVP_Player::state.audioDevice = {};
	}

	if (isPlaying)
		LVP_Player::Play();
}
