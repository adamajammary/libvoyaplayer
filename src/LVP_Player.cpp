#include "LVP_Player.h"

MediaPlayer::LVP_AudioContext*    MediaPlayer::LVP_Player::audioContext          = NULL;
MediaPlayer::LVP_AudioDevice      MediaPlayer::LVP_Player::audioDevice           = {};
LVP_CallbackContext               MediaPlayer::LVP_Player::callbackContext       = {};
LibFFmpeg::AVFormatContext*       MediaPlayer::LVP_Player::formatContext         = NULL;
LibFFmpeg::AVFormatContext*       MediaPlayer::LVP_Player::formatContextExternal = NULL;
bool                              MediaPlayer::LVP_Player::isOpening             = false;
bool                              MediaPlayer::LVP_Player::isStopping            = false;
std::mutex                        MediaPlayer::LVP_Player::packetLock            = {};
double                            MediaPlayer::LVP_Player::seekPTS               = 0.0;
bool                              MediaPlayer::LVP_Player::seekRequested         = false;
bool                              MediaPlayer::LVP_Player::seekRequestedBack     = false;
bool                              MediaPlayer::LVP_Player::seekRequestedPaused   = false;
int                               MediaPlayer::LVP_Player::seekByRequest         = 0;
double                            MediaPlayer::LVP_Player::seekToRequest         = 0.0;
MediaPlayer::LVP_PlayerState      MediaPlayer::LVP_Player::state                 = {};
MediaPlayer::LVP_SubtitleContext* MediaPlayer::LVP_Player::subContext            = {};
System::LVP_TimeOut*              MediaPlayer::LVP_Player::timeOut               = NULL;
bool                              MediaPlayer::LVP_Player::trackRequested        = false;
std::queue<LVP_MediaTrack>        MediaPlayer::LVP_Player::trackRequests         = {};
MediaPlayer::LVP_VideoContext*    MediaPlayer::LVP_Player::videoContext          = {};

void MediaPlayer::LVP_Player::Init(const LVP_CallbackContext& callbackContext)
{
	LVP_Player::callbackContext = callbackContext;
	LVP_Player::state.quit      = false;
}

void MediaPlayer::LVP_Player::AddAudioDevice(const SDL_AudioDeviceEvent& adevice)
{
	if (LVP_Player::state.isStopped || (LVP_Player::audioDevice.id >= MIN_VALID_AUDIO_DEVICE_ID))
		return;

	#if defined _DEBUG
		printf("Audio device connected: %s\n", SDL_GetAudioDeviceName(adevice.which, adevice.iscapture));
	#endif

	bool isPaused = LVP_Player::state.isPaused;

	LVP_Player::openAudioDevice();

	if (!isPaused)
		SDL_PauseAudioDevice(LVP_Player::audioDevice.id, 0);
}

void MediaPlayer::LVP_Player::CallbackError(const std::string& errorMessage)
{
	if (LVP_Player::callbackContext.errorCB == nullptr)
		return;

	std::thread([errorMessage]() {
		LVP_Player::callbackContext.errorCB(errorMessage, LVP_Player::callbackContext.data);
	}).detach();
}

void MediaPlayer::LVP_Player::callbackEvents(LVP_EventType type)
{
	if (LVP_Player::callbackContext.eventsCB == nullptr)
		return;

	std::thread([type]() {
		LVP_Player::callbackContext.eventsCB(type, LVP_Player::callbackContext.data);
	}).detach();
}

void MediaPlayer::LVP_Player::callbackVideoIsAvailable(SDL_Surface* surface)
{
	if (LVP_Player::callbackContext.videoCB == nullptr)
		return;

	std::thread([surface]() {
		LVP_Player::callbackContext.videoCB(surface, LVP_Player::callbackContext.data);
	}).detach();
}

void MediaPlayer::LVP_Player::Close()
{
	if (!LVP_Player::state.isStopped && !LVP_Player::isStopping)
		LVP_Player::state.quit = true;
}

void MediaPlayer::LVP_Player::close()
{
	if (LVP_Player::state.isStopped || LVP_Player::isStopping)
		return;

	LVP_Player::isStopping = true;

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

	LVP_Player::closeThreads();

	LVP_Player::closePackets();

	LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_AUDIO);
	LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_SUBTITLE);
	LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_VIDEO);

	LVP_Player::closeAudioContext();
	LVP_Player::closeSubContext();
	LVP_Player::closeVideoContext();

	FREE_AVFORMAT(LVP_Player::formatContext);
	FREE_AVFORMAT(LVP_Player::formatContextExternal);
	DELETE_POINTER(LVP_Player::timeOut);

	LVP_Player::state.reset();

	LVP_Player::state.completed = false;
	LVP_Player::state.isStopped = true;
	LVP_Player::state.quit      = false;

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_STOPPED);

	LVP_Player::isStopping = false;
}

void MediaPlayer::LVP_Player::closeAudioContext()
{
	DELETE_POINTER(LVP_Player::audioContext);

	LVP_Player::closeAudioDevice();
}

void MediaPlayer::LVP_Player::closeAudioDevice()
{
	if (LVP_Player::audioDevice.id >= MIN_VALID_AUDIO_DEVICE_ID)
		SDL_CloseAudioDevice(LVP_Player::audioDevice.id);

	LVP_Player::audioDevice.id = 0;
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
	LVP_Player::closePackets(LVP_Player::audioContext);
	LVP_Player::closePackets(LVP_Player::subContext);
	LVP_Player::closePackets(LVP_Player::videoContext);
}

void MediaPlayer::LVP_Player::closeStream(LibFFmpeg::AVMediaType streamType)
{
	switch (streamType) {
		case LibFFmpeg::AVMEDIA_TYPE_AUDIO:
			FREE_AVSTREAM(LVP_Player::audioContext->stream);
			FREE_AVCODEC(LVP_Player::audioContext->codec);

			LVP_Player::audioContext->index = -1;
			break;
		case LibFFmpeg::AVMEDIA_TYPE_SUBTITLE:
			FREE_AVSTREAM(LVP_Player::subContext->stream);
			FREE_AVCODEC(LVP_Player::subContext->codec);

			LVP_Player::subContext->index = -1;
			break;
		case LibFFmpeg::AVMEDIA_TYPE_VIDEO:
			FREE_AVSTREAM(LVP_Player::videoContext->stream);
			FREE_AVCODEC(LVP_Player::videoContext->codec);

			LVP_Player::videoContext->index = -1;
			break;
		default:
			break;
	}
}

void MediaPlayer::LVP_Player::closeSubContext()
{
	LVP_SubtitleBitmap::Remove();

	#if defined _ENABLE_LIBASS
		LVP_SubtitleText::Remove();
		LVP_SubtitleText::Quit();
	#endif

	DELETE_POINTER(LVP_Player::subContext);
}

void MediaPlayer::LVP_Player::closeThreads()
{
	SDL_CloseAudioDevice(LVP_Player::audioDevice.id);

	LVP_Player::videoContext->renderer = NULL;
}

void MediaPlayer::LVP_Player::closeVideoContext()
{
	DELETE_POINTER(LVP_Player::videoContext);
}

int MediaPlayer::LVP_Player::decodeAudioFrame()
{
    if (LVP_Player::state.isPaused)
        return -1;

	auto frame = LVP_Player::getAudioFrame();

	if (frame == NULL)
		return -1;

	auto bufferSize = LibFFmpeg::av_samples_get_buffer_size(
		NULL,
		frame->ch_layout.nb_channels,
		frame->nb_samples,
		(LibFFmpeg::AVSampleFormat)frame->format,
		1
	);

	if (LVP_Player::audioContext->buffer == NULL)
		LVP_Player::audioContext->buffer = (uint8_t*)std::malloc(bufferSize);
	else if (bufferSize > LVP_Player::audioContext->bufferSize)
		LVP_Player::audioContext->buffer = (uint8_t*)std::realloc(LVP_Player::audioContext->buffer, bufferSize);

	LVP_Player::audioContext->bufferSize = bufferSize;

	if (LVP_Player::audioContext->buffer == NULL)
	{
		FREE_AVFRAME(frame);

		LVP_Player::stop("Failed to create an audio buffer.");
		
		return AVERROR(ENOMEM);
	}

	std::memcpy(LVP_Player::audioContext->buffer, frame->extended_data[0], bufferSize);

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

		auto frame = LibFFmpeg::av_frame_alloc();

		result = LibFFmpeg::avcodec_receive_frame(LVP_Player::audioContext->codec, frame);

		LVP_Player::packetLock.unlock();

		if (result < 0) {
			FREE_AVFRAME(frame);
			break;
		}

		// Apply filter to frame

		if (!LVP_Player::audioContext->filterSpecs.equals(frame, LVP_Player::state.playbackSpeed) || (LVP_Player::audioContext->filter.filterGraph == NULL))
		{
			LVP_Player::initAudioFilter(frame);

			LVP_Player::audioContext->filterSpecs = LVP_AudioSpecs(frame, LVP_Player::state.playbackSpeed);
		}

		result = LibFFmpeg::av_buffersrc_add_frame(LVP_Player::audioContext->filter.bufferSource, frame);

		if (result < 0) {
			FREE_AVFRAME(frame);
			break;
		}

		bool isFrameFiltered = false;

		while (result >= 0)
		{
			result = LibFFmpeg::av_buffersink_get_frame(LVP_Player::audioContext->filter.bufferSink, frame);

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

		LVP_Player::audioContext->framesLock.lock();

		LVP_Player::audioContext->frames.push(frame);

		LVP_Player::audioContext->framesLock.unlock();
	}

	if ((result < 0) && (result != AVERROR(EAGAIN)))
	{
		#if defined _DEBUG
			char strerror[AV_ERROR_MAX_STRING_SIZE];
			LibFFmpeg::av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);
			LOG("AUDIO_DECODE_FRAME: %s\n", strerror);
		#endif

		LVP_Player::stop("Failed to decode audio.");
	}
	else if (result == AVERROR_EOF)
	{
		LVP_Player::packetLock.lock();

		LibFFmpeg::avcodec_flush_buffers(LVP_Player::audioContext->codec);

		LVP_Player::packetLock.unlock();
	}
}

void MediaPlayer::LVP_Player::decodeAudioPacket(LibFFmpeg::AVPacket* packet)
{
	LVP_Player::packetLock.lock();

	auto result = LibFFmpeg::avcodec_send_packet(LVP_Player::audioContext->codec, packet);

	LVP_Player::packetLock.unlock();

	if ((result < 0) && (result != AVERROR(EAGAIN)))
	{
		#if defined _DEBUG
			char strerror[AV_ERROR_MAX_STRING_SIZE];
			LibFFmpeg::av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);
			LOG("AUDIO_SEND_PACKET: %s\n", strerror);
		#endif
	}
}

LVP_Strings MediaPlayer::LVP_Player::GetAudioDevices()
{
	LVP_Strings devices;

	for (int i = 0; i < SDL_GetNumAudioDevices(0); i++)
		devices.push_back(SDL_GetAudioDeviceName(i, 0));

	return devices;
}

LibFFmpeg::AVFrame* MediaPlayer::LVP_Player::getAudioFrame()
{
	LibFFmpeg::AVFrame* frame = NULL;

	LVP_Player::audioContext->framesLock.lock();

	if (!LVP_Player::audioContext->frames.empty())
	{
		frame = LVP_Player::audioContext->frames.front();

		LVP_Player::audioContext->frames.pop();
	}

	LVP_Player::audioContext->framesLock.unlock();

	return frame;
}

int MediaPlayer::LVP_Player::GetAudioTrack()
{
	if (LVP_Player::audioContext == NULL)
		return -1;

	return LVP_Player::audioContext->index;
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::GetAudioTracks()
{
	return LVP_Player::getAudioTracks(LVP_Player::formatContext);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::getAudioTracks(LibFFmpeg::AVFormatContext* formatContext)
{
	return LVP_Player::getMediaTracks(LibFFmpeg::AVMEDIA_TYPE_AUDIO, formatContext);
}

std::vector<LVP_MediaChapter> MediaPlayer::LVP_Player::GetChapters()
{
	return LVP_Player::getChapters(LVP_Player::formatContext);
}

std::vector<LVP_MediaChapter> MediaPlayer::LVP_Player::getChapters(LibFFmpeg::AVFormatContext* formatContext)
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

		auto timeBase = LibFFmpeg::av_q2d(chapter->time_base);
		auto end      = (int64_t)((double)chapter->end   * timeBase * ONE_SECOND_MS_D);
		auto start    = (int64_t)((double)chapter->start * timeBase * ONE_SECOND_MS_D);
		auto title    = LibFFmpeg::av_dict_get(chapter->metadata, "title", NULL, 0);

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
	return (LVP_Player::formatContext != NULL ? std::string(LVP_Player::formatContext->url) : "");
}

LVP_MediaDetails MediaPlayer::LVP_Player::GetMediaDetails(bool skipThumbnail)
{
	if (LVP_Player::state.isStopped)
		return {};

	return
	{
		.duration       = LVP_Media::GetMediaDuration(LVP_Player::formatContext, LVP_Player::audioContext->stream),
		.fileSize       = System::LVP_FileSystem::GetFileSize(LVP_Player::formatContext->url),
		.mediaType      = (LVP_MediaType)LVP_Player::state.mediaType,
		.meta           = LVP_Media::GetMediaMeta(LVP_Player::formatContext),
		.thumbnail      = (!skipThumbnail ? LVP_Media::GetMediaThumbnail(LVP_Player::formatContext) : NULL),
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

	auto audioStream = LVP_Media::GetMediaTrackBest(formatContext, LibFFmpeg::AVMEDIA_TYPE_AUDIO);
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

LibFFmpeg::AVPacket* MediaPlayer::LVP_Player::getMediaPacket(LVP_MediaContext* context)
{
	LibFFmpeg::AVPacket* packet = NULL;

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

		LibFFmpeg::avcodec_flush_buffers(context->codec);

		LVP_Player::packetLock.unlock();
	}

	return packet;
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::getMediaTracks(LibFFmpeg::AVMediaType mediaType, LibFFmpeg::AVFormatContext* formatContext, const LVP_Strings& extSubFiles)
{
	// INTERNAL MEDIA TRACKS

	auto tracks = LVP_Player::getMediaTracksMeta(formatContext, mediaType);

	if (!IS_SUB(mediaType))
		return tracks;

	// EXTERNAL SUB TRACKS

	for (int i = 0; i < (int)extSubFiles.size(); i++)
	{
		auto extFormatContext = LVP_Media::GetMediaFormatContext(extSubFiles[i], true);
		auto extSubTracks     = LVP_Player::getMediaTracksMeta(extFormatContext, LibFFmpeg::AVMEDIA_TYPE_SUBTITLE, i);

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

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::getMediaTracksMeta(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVMediaType mediaType, int extSubFileIndex)
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
		.meta      = LVP_Media::GetMediaMeta(LVP_Player::formatContext),
		.mediaType = (LVP_MediaType)LVP_Media::GetMediaType(LVP_Player::formatContext)
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

LibFFmpeg::AVPixelFormat MediaPlayer::LVP_Player::GetPixelFormatHardware()
{
	return (LVP_Player::videoContext != NULL ? LVP_Player::videoContext->hardwareFormat : LibFFmpeg::AV_PIX_FMT_NONE);
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
	if (SDL_RectEmpty(&destination) || (LVP_Player::videoContext->codec == NULL))
		return {};

	int videoWidth   = LVP_Player::videoContext->codec->width;
	int videoHeight  = LVP_Player::videoContext->codec->height;
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
	return (LVP_Player::subContext != NULL ? LVP_Player::subContext->index : -1);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::GetSubtitleTracks()
{
	return LVP_Player::getSubtitleTracks(LVP_Player::formatContext, LVP_Player::subContext->external);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::getSubtitleTracks(LibFFmpeg::AVFormatContext* formatContext, const LVP_Strings& extSubFiles)
{
	return LVP_Player::getMediaTracks(LibFFmpeg::AVMEDIA_TYPE_SUBTITLE, formatContext, extSubFiles);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::GetVideoTracks()
{
	return LVP_Player::getVideoTracks(LVP_Player::formatContext);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::getVideoTracks(LibFFmpeg::AVFormatContext* formatContext)
{
	return LVP_Player::getMediaTracks(LibFFmpeg::AVMEDIA_TYPE_VIDEO, formatContext);
}

double MediaPlayer::LVP_Player::GetVolume()
{
	return (double)((double)LVP_Player::state.volume / (double)SDL_MIX_MAXVOLUME);
}

void MediaPlayer::LVP_Player::handleSeek()
{
	if (!LVP_Player::seekRequested)
		return;

	LVP_Player::packetLock.lock();

	LVP_Player::audioContext->packetsLock.lock();
	LVP_Player::subContext->packetsLock.lock();
	LVP_Player::videoContext->packetsLock.lock();

	LVP_Player::seekRequestedBack = false;

	int64_t duration    = (LVP_Player::state.duration * AV_TIME_BASE_I64);
	bool    seekByBytes = (IS_BYTE_SEEK(LVP_Player::formatContext->iformat) && (LVP_Player::state.fileSize > 0));
	double  seekPercent = LVP_Player::seekToRequest;

	int64_t seekPosition         = 0;
	int64_t seekPositionExternal = 0;

	if (LVP_Player::seekByRequest != 0)
	{
		auto newPosition = (LVP_Player::state.progress + (double)LVP_Player::seekByRequest);

		if (seekByBytes) {
			seekPercent  = (newPosition / (double)LVP_Player::state.duration);
			seekPosition = (int64_t)((double)LVP_Player::state.fileSize * seekPercent);
		} else {
			seekPosition = (int64_t)(newPosition * AV_TIME_BASE_D);
		}

		if (LVP_Player::subContext->index >= SUB_STREAM_EXTERNAL)
			seekPositionExternal = (int64_t)(newPosition * LibFFmpeg::av_q2d(LVP_Player::subContext->stream->time_base));

		if (LVP_Player::seekByRequest < 0)
			LVP_Player::seekRequestedBack = true;
	}
	else
	{
		if (seekByBytes)
			seekPosition = (int64_t)((double)LVP_Player::state.fileSize * seekPercent);
		else
			seekPosition = (int64_t)((double)duration * seekPercent);

		if (LVP_Player::subContext->index >= SUB_STREAM_EXTERNAL)
			seekPositionExternal = (int64_t)((double)LVP_Player::state.duration * LibFFmpeg::av_q2d(LVP_Player::subContext->stream->time_base) * seekPercent);

		if (seekPercent < (LVP_Player::state.progress / (double)LVP_Player::state.duration))
			LVP_Player::seekRequestedBack = true;
	}

	auto seekMax = (seekByBytes ? (int64_t)LVP_Player::state.fileSize : duration);

	if (seekPosition < 0)
		seekPosition = 0;
	else if (seekPosition > seekMax)
		seekPosition = seekMax;

	int seekFlags = (LVP_Player::seekRequestedBack ? AVSEEK_FLAG_BACKWARD : 0);

	if (seekByBytes)
		seekFlags |= AVSEEK_FLAG_BYTE;

	auto result = LibFFmpeg::av_seek_frame(LVP_Player::formatContext, -1, seekPosition, seekFlags);

	if (result >= 0)
	{
		if (LVP_Player::subContext->index >= SUB_STREAM_EXTERNAL)
			LibFFmpeg::avformat_seek_file(LVP_Player::formatContextExternal, LVP_Player::subContext->stream->index, INT64_MIN, seekPositionExternal, INT64_MAX, 0);

		LVP_SubtitleBitmap::Remove();

		#if defined _ENABLE_LIBASS
			LVP_SubtitleText::Remove();
		#endif

		LVP_Player::closePackets();

		if (LVP_Player::audioContext->index >= 0)
			LVP_Player::audioContext->packets.push(LibFFmpeg::av_packet_alloc());

		if (LVP_Player::subContext->index >= 0)
			LVP_Player::subContext->packets.push(LibFFmpeg::av_packet_alloc());

		if (LVP_Player::videoContext->index >= 0)
			LVP_Player::videoContext->packets.push(LibFFmpeg::av_packet_alloc());

		LVP_Player::audioContext->bufferOffset = 0;
		LVP_Player::videoContext->pts          = 0;

		if (seekByBytes)
			LVP_Player::seekPTS = ((double)LVP_Player::state.duration * seekPercent);
		else
			LVP_Player::seekPTS = ((double)seekPosition / AV_TIME_BASE_D);

		if (LVP_Player::state.isPaused)
			LVP_Player::state.progress = LVP_Player::seekPTS;
	}
	#if defined _DEBUG
	else
	{
		char strerror[AV_ERROR_MAX_STRING_SIZE];
		LibFFmpeg::av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);
		LOG("SEEK: %s\n", strerror);
	}
	#endif

	LVP_Player::seekByRequest = 0;
	LVP_Player::seekToRequest = 0.0;
	LVP_Player::seekRequested = false;

	LVP_Player::videoContext->packetsLock.unlock();
	LVP_Player::subContext->packetsLock.unlock();
	LVP_Player::audioContext->packetsLock.unlock();

	LVP_Player::packetLock.unlock();
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::handleTrack()
{
	if (!LVP_Player::trackRequested)
		return;

	LVP_Player::packetLock.lock();

	while (!LVP_Player::trackRequests.empty())
	{
		const auto& trackRequest = LVP_Player::trackRequests.front();

		LVP_Player::trackRequests.pop();

		auto mediaType = (LibFFmpeg::AVMediaType)trackRequest.mediaType;
		bool isAudio   = IS_AUDIO(mediaType);
		bool isSub     = IS_SUB(mediaType);

		bool isSameAudioTrack = (isAudio && (trackRequest.track == LVP_Player::audioContext->index));
		bool isSameSubTrack   = (isSub   && (trackRequest.track == LVP_Player::subContext->index));

		if (isSameAudioTrack || isSameSubTrack)
			continue;

		// DISABLE SUBS

		if (isSub && (trackRequest.track < 0))
		{
			LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_SUBTITLE);

			LVP_SubtitleBitmap::Remove();

			#if defined _ENABLE_LIBASS
				LVP_SubtitleText::Remove();
			#endif

			continue;
		}

		bool isPaused     = LVP_Player::state.isPaused;
		auto lastProgress = (double)(LVP_Player::state.progress / (double)LVP_Player::state.duration);

		switch (mediaType) {
		case LibFFmpeg::AVMEDIA_TYPE_AUDIO:
			if (!isPaused)
				LVP_Player::Pause();

			LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_AUDIO);

			LVP_Player::audioContext->free();

			LVP_Media::SetMediaTrackByIndex(LVP_Player::formatContext, trackRequest.track, LVP_Player::audioContext);

			try {
				LVP_Player::openThreadAudio();
			} catch (const std::exception& e) {
				LVP_Player::CallbackError(e.what());
			}

			if (!LVP_Player::seekRequested)
				LVP_Player::seekTo(lastProgress);

			if (!isPaused)
				LVP_Player::Play();

			break;
		case LibFFmpeg::AVMEDIA_TYPE_SUBTITLE:
			LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_SUBTITLE);

			LVP_SubtitleBitmap::Remove();

			#if defined _ENABLE_LIBASS
				LVP_SubtitleText::Remove();
			#endif

			if (trackRequest.track >= SUB_STREAM_EXTERNAL)
				LVP_Player::openSubExternal(trackRequest.track);
			else
				LVP_Media::SetMediaTrackByIndex(LVP_Player::formatContext, trackRequest.track, LVP_Player::subContext);

			LVP_Player::openThreadSub();

			if (!LVP_Player::seekRequested)
				LVP_Player::seekTo(lastProgress);

			break;
		default:
			break;
		}
	}

	LVP_Player::trackRequested = false;

	LVP_Player::packetLock.unlock();
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::initAudioFilter(LibFFmpeg::AVFrame* frame)
{
	auto abuffer     = LibFFmpeg::avfilter_get_by_name("abuffer");
	auto abuffersink = LibFFmpeg::avfilter_get_by_name("abuffersink");
	auto aresample   = LibFFmpeg::avfilter_get_by_name("aresample");

	auto filterGraph    = LibFFmpeg::avfilter_graph_alloc();
	auto bufferSource   = LibFFmpeg::avfilter_graph_alloc_filter(filterGraph, abuffer,     "src");
	auto bufferSink     = LibFFmpeg::avfilter_graph_alloc_filter(filterGraph, abuffersink, "sink");
	auto filterResample = LibFFmpeg::avfilter_graph_alloc_filter(filterGraph, aresample,   "aresample");

	// https://ffmpeg.org/ffmpeg-filters.html#abuffer

	auto inChannelLayout = LVP_AudioSpecs::getChannelLayoutName(frame->ch_layout);
	auto inSampleFormat  = LibFFmpeg::av_get_sample_fmt_name((LibFFmpeg::AVSampleFormat)frame->format);

	LibFFmpeg::av_opt_set(bufferSource,     "channel_layout", inChannelLayout.c_str(), AV_OPT_SEARCH_CHILDREN);
	LibFFmpeg::av_opt_set(bufferSource,     "sample_fmt",     inSampleFormat,          AV_OPT_SEARCH_CHILDREN);
	LibFFmpeg::av_opt_set_int(bufferSource, "sample_rate",    frame->sample_rate,      AV_OPT_SEARCH_CHILDREN);

	// https://ffmpeg.org/ffmpeg-resampler.html#Resampler-Options

	auto outChannelLayout = LVP_AudioSpecs::getChannelLayoutName(LVP_AudioSpecs::getChannelLayout(LVP_Player::audioContext->deviceSpecs.channels));
	auto outSampleFormat  = LibFFmpeg::av_get_sample_fmt_name(LVP_AudioSpecs::getSampleFormat(LVP_Player::audioContext->deviceSpecs.format));
	auto outSampleRate    = LVP_AudioSpecs::getSampleRate(LVP_Player::audioContext->deviceSpecs.freq, LVP_Player::state.playbackSpeed);

	LibFFmpeg::av_opt_set(filterResample,     "out_chlayout",    outChannelLayout.c_str(), AV_OPT_SEARCH_CHILDREN);
	LibFFmpeg::av_opt_set(filterResample,     "out_sample_fmt",  outSampleFormat,          AV_OPT_SEARCH_CHILDREN);
	LibFFmpeg::av_opt_set_int(filterResample, "out_sample_rate", outSampleRate,            AV_OPT_SEARCH_CHILDREN);

	LibFFmpeg::avfilter_init_str(bufferSource,   NULL);
	LibFFmpeg::avfilter_init_str(bufferSink,     NULL);
	LibFFmpeg::avfilter_init_str(filterResample, NULL);

	LibFFmpeg::avfilter_link(bufferSource,   0, filterResample, 0);
	LibFFmpeg::avfilter_link(filterResample, 0, bufferSink, 0);

	if (LibFFmpeg::avfilter_graph_config(filterGraph, NULL) < 0)
		throw std::runtime_error("Failed to initialize a valid audio filter.");

	FREE_AVFILTER_GRAPH(LVP_Player::audioContext->filter.filterGraph);

	LVP_Player::audioContext->filter.filterGraph  = filterGraph;
	LVP_Player::audioContext->filter.bufferSource = bufferSource;
	LVP_Player::audioContext->filter.bufferSink   = bufferSink;
}

bool MediaPlayer::LVP_Player::isHardwarePixelFormat(int frameFormat)
{
	auto format = (LibFFmpeg::AVPixelFormat)frameFormat;

	if (format == LibFFmpeg::AV_PIX_FMT_NONE)
		return false;

	return (format == LVP_Player::videoContext->hardwareFormat);
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
	case LibFFmpeg::AVMEDIA_TYPE_AUDIO:
		return LVP_Player::isPacketQueueFull(LibFFmpeg::AVMEDIA_TYPE_AUDIO);
	case LibFFmpeg::AVMEDIA_TYPE_VIDEO:
		return (LVP_Player::isPacketQueueFull(LibFFmpeg::AVMEDIA_TYPE_AUDIO) && LVP_Player::isPacketQueueFull(LibFFmpeg::AVMEDIA_TYPE_VIDEO));
	default:
		break;
	}

	return false;
}

bool MediaPlayer::LVP_Player::isPacketQueueFull(LibFFmpeg::AVMediaType streamType)
{
	switch (streamType) {
	case LibFFmpeg::AVMEDIA_TYPE_AUDIO:
		return ((LVP_Player::audioContext->index >= 0) && (LVP_Player::audioContext->packets.size() >= MIN_PACKET_QUEUE_SIZE));
	case LibFFmpeg::AVMEDIA_TYPE_SUBTITLE:
		return ((LVP_Player::subContext->index >= 0) && (LVP_Player::subContext->packets.size() >= MIN_PACKET_QUEUE_SIZE));
	case LibFFmpeg::AVMEDIA_TYPE_VIDEO:
		return ((LVP_Player::videoContext->index >= 0) && (LVP_Player::videoContext->packets.size() >= MIN_PACKET_QUEUE_SIZE));
	default:
		break;
	}

	return false;
}

bool MediaPlayer::LVP_Player::IsStopped()
{
	return LVP_Player::state.isStopped;
}

/**
 * @throws invalid_argument
 */
void MediaPlayer::LVP_Player::Open(const std::string& filePath)
{
	if (!LVP_Player::state.openFilePath.empty() || LVP_Player::isOpening)
		return;

	if (filePath.empty())
		throw std::invalid_argument("filePath cannot be empty");

	LVP_Player::state.openFilePath = filePath;
}

void MediaPlayer::LVP_Player::open()
{
	if (LVP_Player::state.openFilePath.empty() || LVP_Player::isOpening)
		return;

	try {
		LVP_Player::isOpening = true;

		LVP_Player::close();

		LVP_Player::audioContext = new LVP_AudioContext();
		LVP_Player::subContext   = new LVP_SubtitleContext();
		LVP_Player::videoContext = new LVP_VideoContext();

		LVP_Player::openFormatContext();
		LVP_Player::openStreams();
		LVP_Player::openThreads();

		LVP_Player::callbackEvents(LVP_EVENT_MEDIA_OPENED);

		LVP_Player::Play();

		LVP_Player::isOpening = false;
	} catch (const std::exception& e) {
		MediaPlayer::LVP_Player::CallbackError(System::LVP_Text::Format("Failed to open media file:\n%s", e.what()));
		LVP_Player::close();
	}
}

void MediaPlayer::LVP_Player::openAudioDevice()
{
	bool isDefault = (LVP_Player::audioDevice.device == "Default");
	bool isEmpty   = LVP_Player::audioDevice.device.empty();
	auto newDevice = (!isEmpty && !isDefault ? LVP_Player::audioDevice.device.c_str() : NULL);

	LVP_Player::closeAudioDevice();

	if (SDL_GetNumAudioDevices(0) <= 0)
		return;

	LVP_Player::audioDevice.id = SDL_OpenAudioDevice(
		newDevice,
		0,
		&LVP_Player::audioContext->deviceSpecsWanted,
		&LVP_Player::audioContext->deviceSpecs,
		(SDL_AUDIO_ALLOW_CHANNELS_CHANGE | SDL_AUDIO_ALLOW_FORMAT_CHANGE | SDL_AUDIO_ALLOW_FREQUENCY_CHANGE)
	);

	if (LVP_Player::audioDevice.id < MIN_VALID_AUDIO_DEVICE_ID)
	{
		#if defined _DEBUG
			LOG("%s\n", SDL_GetError());
		#endif

		LVP_Player::closeAudioDevice();
	}
}

/**
 * @throws invalid_argument
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openFormatContext()
{
	if (LVP_Player::state.openFilePath.empty())
		throw std::invalid_argument("filePath cannot be empty");

	LVP_Player::timeOut = new System::LVP_TimeOut();

	if (LVP_Player::formatContext == NULL)
		LVP_Player::formatContext = LVP_Media::GetMediaFormatContext(LVP_Player::state.openFilePath, true, LVP_Player::timeOut);

	auto mediaType    = LVP_Media::GetMediaType(LVP_Player::formatContext);
	bool isValidMedia = (IS_AUDIO(mediaType) || IS_VIDEO(mediaType));

	if (!isValidMedia) {
		FREE_AVFORMAT(LVP_Player::formatContext);
		throw std::runtime_error(System::LVP_Text::Format("Invalid media type: %d", (int)mediaType).c_str());
	}

	auto fileExtension = System::LVP_FileSystem::GetFileExtension(LVP_Player::state.openFilePath);
	auto fileSize      = System::LVP_FileSystem::GetFileSize(LVP_Player::state.openFilePath);

	if ((fileExtension == "m2ts") && System::LVP_FileSystem::IsBlurayAACS(LVP_Player::state.openFilePath, fileSize))
		isValidMedia = false;
	else if ((fileExtension == "vob") && System::LVP_FileSystem::IsDVDCSS(LVP_Player::state.openFilePath, fileSize))
		isValidMedia = false;

	if (!isValidMedia) {
		FREE_AVFORMAT(LVP_Player::formatContext);
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
	LVP_Player::audioContext->index = -1;
	LVP_Player::subContext->index   = -1;
	LVP_Player::videoContext->index = -1;

	// AUDIO TRACK
	LVP_Media::SetMediaTrackBest(LVP_Player::formatContext, LibFFmpeg::AVMEDIA_TYPE_AUDIO, LVP_Player::audioContext);

	if (LVP_Player::audioContext->stream == NULL)
		throw std::runtime_error("Failed to find a valid audio track.");

	LVP_Player::state.duration = LVP_Media::GetMediaDuration(LVP_Player::formatContext, LVP_Player::audioContext->stream);

	// VIDEO TRACK
	LVP_Media::SetMediaTrackBest(LVP_Player::formatContext, LibFFmpeg::AVMEDIA_TYPE_VIDEO, LVP_Player::videoContext);

	if (IS_VIDEO(LVP_Player::state.mediaType))
	{
		if (LVP_Player::videoContext->stream == NULL)
			throw std::runtime_error("Failed to find a valid video track.");

		// SUBTITLE TRACK
		LVP_Media::SetMediaTrackBest(LVP_Player::formatContext, LibFFmpeg::AVMEDIA_TYPE_SUBTITLE, LVP_Player::subContext);

		LVP_Player::subContext->external = System::LVP_FileSystem::GetSubtitleFilesForVideo(LVP_Player::state.filePath);

		if ((LVP_Player::subContext->stream == NULL) && !LVP_Player::subContext->external.empty())
			LVP_Player::openSubExternal();
	}

	LVP_Player::state.trackCount = LVP_Player::formatContext->nb_streams;
}

void MediaPlayer::LVP_Player::openSubExternal(int streamIndex)
{
	if (LVP_Player::subContext->external.empty() || (streamIndex < SUB_STREAM_EXTERNAL))
		return;

	int fileIndex  = ((streamIndex - SUB_STREAM_EXTERNAL) / SUB_STREAM_EXTERNAL);
	int trackIndex = ((streamIndex - SUB_STREAM_EXTERNAL) % SUB_STREAM_EXTERNAL);

	std::string subFile = LVP_Player::subContext->external[fileIndex];

	FREE_AVFORMAT(LVP_Player::formatContextExternal);

	LVP_Player::formatContextExternal = LVP_Media::GetMediaFormatContext(subFile, true);

	if (LVP_Player::formatContextExternal != NULL)
		LVP_Media::SetMediaTrackByIndex(LVP_Player::formatContextExternal, trackIndex, LVP_Player::subContext, fileIndex);
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openThreads()
{
	LVP_Player::openThreadAudio();

	if (LVP_Player::videoContext->stream != NULL)
		LVP_Player::openThreadVideo();

	if (LVP_Player::subContext->stream != NULL)
		LVP_Player::openThreadSub();

	std::thread(LVP_Player::threadPackets).detach();
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openThreadAudio()
{
	if (LVP_Player::audioContext->codec == NULL)
		throw std::runtime_error("Audio stream is missing a valid codec.");

	LVP_Player::audioContext->index = LVP_Player::audioContext->stream->index;

	auto channelCount = LVP_Player::audioContext->codec->ch_layout.nb_channels;
	auto sampleRate   = LVP_Player::audioContext->codec->sample_rate;

	if (channelCount <= 0)
		channelCount = LVP_AudioSpecs::getChannelLayout(2).nb_channels;

	if ((sampleRate <= 0) || (channelCount <= 0))
		throw std::runtime_error(System::LVP_Text::Format("Invalid audio: %d channels, %d bps", channelCount, sampleRate).c_str());

	auto sampleCount  = LVP_Player::audioContext->codec->frame_size;
	auto sampleFormat = LVP_AudioSpecs::getSampleFormat(LVP_Player::audioContext->codec->sample_fmt);

	LVP_Player::audioContext->deviceSpecsWanted = {
		.freq     = sampleRate,
		.format   = (SDL_AudioFormat)(sampleFormat > 0 ? sampleFormat : AUDIO_S16SYS),
		.channels = (uint8_t)channelCount,
		.silence  = 0,
		.samples  = (uint16_t)(sampleCount > 0 ? sampleCount : 4096),
		.padding  = 0,
		.size     = 0,
		.callback = LVP_Player::threadAudioCallback,
		.userdata = NULL
	};

	LVP_Player::openAudioDevice();

	if (!LVP_Player::state.threads[LVP_THREAD_AUDIO])
		std::thread(LVP_Player::threadAudio).detach();
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openThreadSub()
{
	#if defined _DEBUG
		auto extradata = LVP_Player::subContext->codec->extradata;
		auto header    = LVP_Player::subContext->codec->subtitle_header;
		auto subHeader = (header != NULL ? std::string(reinterpret_cast<char*>(header)) : "");

		if (subHeader.empty())
			subHeader = (extradata != NULL ? std::string(reinterpret_cast<char*>(extradata)) : "");

		printf("\n%s\n", subHeader.c_str());
	#endif

	if (!LVP_Player::state.threads[LVP_THREAD_SUBTITLE])
		std::thread(LVP_Player::threadSub).detach();

	LVP_Player::subContext->formatContext = LVP_Player::formatContext;

	LVP_Player::subContext->videoSize = {
		LVP_Player::videoContext->codec->width,
		LVP_Player::videoContext->codec->height
	};

	#if defined _ENABLE_LIBASS
		LVP_SubtitleText::Init(LVP_Player::subContext);
	#endif
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openThreadVideo()
{
	if (LVP_Player::videoContext->codec == NULL)
		throw std::runtime_error("Video stream is missing a valid codec.");

	int videoWidth  = LVP_Player::videoContext->codec->width;
	int videoHeight = LVP_Player::videoContext->codec->height;

	// SURFACE

	LVP_Player::videoContext->surface = SDL_CreateRGBSurfaceWithFormat(0, videoWidth, videoHeight, 32, SDL_PIXELFORMAT_RGBA32);

	// RENDERER

	LVP_Player::videoContext->isSoftwareRenderer = (LVP_Player::callbackContext.hardwareRenderer == NULL);

	if (LVP_Player::videoContext->isSoftwareRenderer)
		LVP_Player::videoContext->renderer = SDL_CreateSoftwareRenderer(LVP_Player::videoContext->surface);
	else
		LVP_Player::videoContext->renderer = LVP_Player::callbackContext.hardwareRenderer;

	if (LVP_Player::videoContext->renderer == NULL)
		throw std::runtime_error(System::LVP_Text::Format("Failed to create a renderer: %s", SDL_GetError()).c_str());

	// TEXTURE

	LVP_Player::videoContext->texture = SDL_CreateTexture(
		LVP_Player::videoContext->renderer,
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING,
		videoWidth,
		videoHeight
	);

	SDL_SetTextureBlendMode(LVP_Player::videoContext->texture, SDL_BLENDMODE_NONE);

	// FRAME

	LVP_Player::videoContext->frameHardware = LibFFmpeg::av_frame_alloc();
	LVP_Player::videoContext->frameSoftware = LibFFmpeg::av_frame_alloc();

	if ((LVP_Player::videoContext->frameHardware == NULL) || (LVP_Player::videoContext->frameSoftware == NULL))
		throw std::runtime_error("Failed to allocate a video context frame.");

	if (LVP_Player::videoContext->frameEncoded != NULL)
		LVP_Player::videoContext->frameEncoded->linesize[0] = 0;

	// THREAD

	if (!LVP_Player::state.threads[LVP_THREAD_VIDEO])
		std::thread(LVP_Player::threadVideo).detach();

	// STREAM/TRACK

	LVP_Player::videoContext->index = LVP_Player::videoContext->stream->index;
}

void MediaPlayer::LVP_Player::Pause()
{
	if (LVP_Player::state.isPaused || LVP_Player::state.filePath.empty())
		return;

	LVP_Player::state.isPaused  = true;
	LVP_Player::state.isPlaying = false;
	LVP_Player::state.isStopped = false;

	SDL_PauseAudioDevice(LVP_Player::audioDevice.id, 1);

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_PAUSED);
}

void MediaPlayer::LVP_Player::Play()
{
	if (LVP_Player::state.isPlaying || LVP_Player::state.filePath.empty())
		return;

	LVP_Player::state.isPlaying = true;
	LVP_Player::state.isPaused  = false;
	LVP_Player::state.isStopped = false;

	SDL_PauseAudioDevice(LVP_Player::audioDevice.id, 0);

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_PLAYING);
}

void MediaPlayer::LVP_Player::Quit()
{
	LVP_Player::close();
}

void MediaPlayer::LVP_Player::RemoveAudioDevice(const SDL_AudioDeviceEvent& adevice)
{
	if (LVP_Player::state.isStopped || (adevice.which != LVP_Player::audioDevice.id))
		return;

	#if defined _DEBUG
		printf("Audio device disconnected: %s\n", SDL_GetAudioDeviceName(adevice.which, adevice.iscapture));
	#endif

	LVP_Player::openAudioDevice();
}

void MediaPlayer::LVP_Player::Render(const SDL_Rect& destination)
{
	if (LVP_Player::state.completed || LVP_Player::state.quit)
		LVP_Player::close();

	if (LVP_Player::isStopping)
		return;

	if (!LVP_Player::state.openFilePath.empty())
		LVP_Player::open();

	if (LVP_Player::state.isStopped)
		return;

	LVP_Player::handleTrack();
	LVP_Player::handleSeek();

	if (LVP_Player::videoContext->isReadyForRender)
	{
		LVP_Player::videoContext->isReadyForRender = false;

		LVP_Player::renderVideo();

		LVP_SubtitleBitmap::RemoveExpired(LVP_Player::state.progress);

		LVP_SubtitleBitmap::Render(LVP_Player::videoContext->surface, LVP_Player::subContext, LVP_Player::state.progress);
		
		#if defined _ENABLE_LIBASS
			LVP_SubtitleText::Render(LVP_Player::videoContext->surface, LVP_Player::state.progress);
		#endif
	}

	if (LVP_Player::videoContext->isReadyForPresent && (LVP_Player::videoContext->surface != NULL))
	{
		LVP_Player::videoContext->isReadyForPresent = false;

		auto surface = LVP_Player::videoContext->surface;

		if (!LVP_Player::videoContext->isSoftwareRenderer)
			SDL_UpdateTexture(LVP_Player::videoContext->texture, NULL, surface->pixels, surface->pitch);
		else
			LVP_Player::callbackVideoIsAvailable(surface);
	}

	if (!LVP_Player::videoContext->isSoftwareRenderer && (LVP_Player::videoContext->texture != NULL))
	{
		auto      scaledDest = LVP_Player::getScaledVideoDestination(destination);
		SDL_Rect* destRect   = (!SDL_RectEmpty(&scaledDest) ? &scaledDest : NULL);

		SDL_RenderCopy(LVP_Player::videoContext->renderer, LVP_Player::videoContext->texture, NULL, destRect);
	}
}

void MediaPlayer::LVP_Player::renderVideo()
{
	if ((LVP_Player::videoContext->index < 0) || (LVP_Player::videoContext->codec == NULL))
		return;

	auto videoWidth  = LVP_Player::videoContext->codec->width;
	auto videoHeight = LVP_Player::videoContext->codec->height;

	if (LVP_Player::videoContext->frame == NULL)
		return;

	if (LVP_Player::videoContext->frameEncoded == NULL)
	{
		LVP_Player::videoContext->frameEncoded = LibFFmpeg::av_frame_alloc();

		auto result = LibFFmpeg::av_image_alloc(
			LVP_Player::videoContext->frameEncoded->data,
			LVP_Player::videoContext->frameEncoded->linesize,
			videoWidth,
			videoHeight,
			LibFFmpeg::AV_PIX_FMT_RGBA,
			1
		);

		if (result <= 0)
			FREE_AVFRAME(LVP_Player::videoContext->frameEncoded);
	}

	LVP_Player::packetLock.lock();

	auto pixelFormat = (LibFFmpeg::AVPixelFormat)LVP_Player::videoContext->frame->format;

	LVP_Player::packetLock.unlock();

	if ((LVP_Player::videoContext->frameEncoded == NULL) || (pixelFormat == LibFFmpeg::AV_PIX_FMT_NONE))
		return;

	LVP_Player::videoContext->scaleContext = LibFFmpeg::sws_getCachedContext(
		LVP_Player::videoContext->scaleContext,
		videoWidth,
		videoHeight,
		pixelFormat,
		videoWidth,
		videoHeight,
		LibFFmpeg::AV_PIX_FMT_RGBA,
		DEFAULT_SCALE_FILTER,
		NULL,
		NULL,
		NULL
	);

	if (LVP_Player::videoContext->scaleContext == NULL)
		return;

	LVP_Player::packetLock.lock();

	auto scaleResult = LibFFmpeg::sws_scale_frame(
		LVP_Player::videoContext->scaleContext,
		LVP_Player::videoContext->frameEncoded,
		LVP_Player::videoContext->frame
	);

	LVP_Player::packetLock.unlock();

	if (scaleResult <= 0)
		return;

	auto size = (size_t)(scaleResult * LVP_Player::videoContext->frameEncoded->linesize[0]);

	std::memcpy(LVP_Player::videoContext->surface->pixels, LVP_Player::videoContext->frameEncoded->data[0], size);
}

void MediaPlayer::LVP_Player::Resize()
{
	if (LVP_Player::state.isStopped)
		return;

	LVP_Player::videoContext->isReadyForRender  = true;
	LVP_Player::videoContext->isReadyForPresent = true;
}

void MediaPlayer::LVP_Player::SeekBy(int seconds)
{
	if (LVP_Player::formatContext == NULL)
		return;

	LVP_Player::packetLock.lock();

	LVP_Player::seekByRequest += seconds;
	LVP_Player::seekRequested  = true;

	if (IS_VIDEO(LVP_Player::state.mediaType) && LVP_Player::state.isPaused)
		LVP_Player::seekRequestedPaused = true;

	LVP_Player::packetLock.unlock();
}

void MediaPlayer::LVP_Player::SeekTo(double percent)
{
	if (LVP_Player::formatContext == NULL)
		return;

	auto validPercent = std::max(0.0, std::min(1.0, percent));

	LVP_Player::packetLock.lock();

	LVP_Player::seekTo(validPercent);

	LVP_Player::packetLock.unlock();
}

void MediaPlayer::LVP_Player::seekTo(double percent)
{
	LVP_Player::seekToRequest = percent;
	LVP_Player::seekRequested = true;

	if (IS_VIDEO(LVP_Player::state.mediaType) && LVP_Player::state.isPaused)
		LVP_Player::seekRequestedPaused = true;
}

bool MediaPlayer::LVP_Player::SetAudioDevice(const std::string& device)
{
	bool isPaused = LVP_Player::state.isPaused;

	if (!isPaused)
		LVP_Player::Pause();

	LVP_Player::audioDevice.device = std::string(device);

	if (!LVP_Player::state.isStopped)
		LVP_Player::openAudioDevice();

	if (!isPaused)
		LVP_Player::Play();

	return (LVP_Player::audioDevice.id >= MIN_VALID_AUDIO_DEVICE_ID);
}

void MediaPlayer::LVP_Player::setAudioPacketDuration(LibFFmpeg::AVPacket* packet)
{
	if ((packet == NULL) || (packet->duration <= 0))
		return;

	auto packetDuration = ((double)packet->duration / LVP_Player::state.playbackSpeed);
	auto streamTimeBase = LibFFmpeg::av_q2d(LVP_Player::audioContext->stream->time_base);

	LVP_Player::audioContext->packetDuration = (packetDuration * streamTimeBase);
}

void MediaPlayer::LVP_Player::setAudioProgress(LibFFmpeg::AVFrame* frame)
{
	if (frame == NULL)
		return;

	// Save last valid progress
	
	if (LVP_Player::state.progress > 0)
		LVP_Player::audioContext->lastPogress = LVP_Player::state.progress;
	
	// Calculate current progress from timestamp (pts) or last progress and packet duration

	LVP_Player::state.progress = LVP_Media::GetAudioPTS(LVP_Player::audioContext, frame);
	
	// Manually calculate packet duration (if not available) based on last progress

	if ((LVP_Player::audioContext->packetDuration <= 0) &&
		(LVP_Player::state.progress > 0) &&
		(LVP_Player::audioContext->lastPogress > 0) &&
		(LVP_Player::state.progress != LVP_Player::audioContext->lastPogress))
	{
		LVP_Player::audioContext->packetDuration = (LVP_Player::state.progress - LVP_Player::audioContext->lastPogress);
		LVP_Player::audioContext->lastPogress    = LVP_Player::state.progress;
	}
	
	// Correct invalid progress when seeking backwards

	if (IS_INVALID_PTS())
		LVP_Player::seekPTS += LVP_Player::audioContext->packetDuration;
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
	if (LVP_Player::formatContext == NULL)
		return;

	LVP_Player::packetLock.lock();

	LVP_Player::trackRequests.push(track);

	LVP_Player::trackRequested = true;

	LVP_Player::packetLock.unlock();
}

void MediaPlayer::LVP_Player::SetVolume(double percent)
{
	auto validPercent = std::max(0.0, std::min(1.0, percent));
	auto volume       = (int)((double)SDL_MIX_MAXVOLUME * validPercent);

	LVP_Player::state.volume = std::max(0, std::min(SDL_MIX_MAXVOLUME, volume));

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
		while (!LVP_Player::state.quit && (LVP_Player::audioDevice.id >= MIN_VALID_AUDIO_DEVICE_ID) && (
			LVP_Player::state.isPaused ||
			LVP_Player::seekRequested ||
			LVP_Player::trackRequested ||
			LVP_Player::audioContext->packets.empty() ||
			(LVP_Player::audioContext->frames.size() >= MIN_PACKET_QUEUE_SIZE)))
		{
			SDL_Delay(DELAY_TIME_DEFAULT);
		}

		if (LVP_Player::state.quit)
			break;

		auto packet = LVP_Player::getMediaPacket(LVP_Player::audioContext);

		if (packet == NULL)
			continue;

		LVP_Player::setAudioPacketDuration(packet);

		if (LVP_Player::audioDevice.id >= MIN_VALID_AUDIO_DEVICE_ID)
		{
			LVP_Player::decodeAudioPacket(packet);
			LVP_Player::decodeAudioFrames();
		}
		else
		{
			LVP_Player::audioContext->free();

			auto duration = (uint32_t)(LVP_Player::audioContext->packetDuration * ONE_SECOND_MS_D);
			auto timeBase = LibFFmpeg::av_q2d(LVP_Player::audioContext->stream->time_base);

			if (packet->pts >= 0)
				LVP_Player::state.progress = ((double)packet->pts * timeBase);

			SDL_Delay(duration);
		}

		FREE_AVPACKET(packet);
	}

	LVP_Player::state.threads[LVP_THREAD_AUDIO] = false;

	return 0;
}

void MediaPlayer::LVP_Player::threadAudioCallback(void* userData, uint8_t* stream, int streamSize)
{
	LVP_Player::state.threads[LVP_THREAD_AUDIO_CALLBACK] = true;

	while (streamSize > 0)
	{
		if (LVP_Player::audioContext->bufferOffset >= LVP_Player::audioContext->dataSize)
		{
			auto dataSize = LVP_Player::decodeAudioFrame();

			if (dataSize < 0)
			{
				auto sampleFormat = LVP_AudioSpecs::getSampleFormat(LVP_Player::audioContext->deviceSpecs.format);
				auto sampleSize   = LibFFmpeg::av_samples_get_buffer_size(NULL, LVP_Player::audioContext->deviceSpecs.channels, 1, sampleFormat, 1);

				FREE_POINTER(LVP_Player::audioContext->buffer);

				LVP_Player::audioContext->dataSize = (512 / sampleSize * sampleSize);
			} else {
				LVP_Player::audioContext->dataSize = dataSize;
			}

			LVP_Player::audioContext->bufferOffset = 0;
		}

		int writeSize = (LVP_Player::audioContext->dataSize - LVP_Player::audioContext->bufferOffset);

		if (writeSize > streamSize)
			writeSize = streamSize;

		auto buffer = (LVP_Player::audioContext->buffer != NULL ? (const uint8_t*)(LVP_Player::audioContext->buffer + LVP_Player::audioContext->bufferOffset) : NULL);
		auto volume = (!LVP_Player::state.isMuted ? LVP_Player::state.volume : 0);

		if ((volume == SDL_MIX_MAXVOLUME) && (buffer != NULL))
		{
			std::memcpy(stream, buffer, writeSize);
		}
		else
		{
			std::memset(stream, 0, writeSize);

			if ((volume > 0) && (buffer != NULL))
				SDL_MixAudioFormat(stream, buffer, LVP_Player::audioContext->deviceSpecs.format, writeSize, volume);
		}

		streamSize -= writeSize;
		stream     += writeSize;

		LVP_Player::audioContext->bufferOffset += writeSize;
	}

	LVP_Player::state.threads[LVP_THREAD_AUDIO_CALLBACK] = false;
}

int MediaPlayer::LVP_Player::threadPackets()
{
	LVP_Player::state.threads[LVP_THREAD_PACKETS] = true;

	bool endOfFile  = false;
	int  errorCount = 0;

	while (!LVP_Player::state.quit)
	{
		while (LVP_Player::seekRequested && !LVP_Player::state.quit)
			SDL_Delay(DELAY_TIME_DEFAULT);

		if (LVP_Player::state.quit)
			break;

		// Initialize a new packet

		auto packet = LibFFmpeg::av_packet_alloc();

		if (packet == NULL)
		{
			errorCount = MAX_ERRORS;

			LVP_Player::stop("Failed to allocate packet.");
			break;
		}

		if (LVP_Player::timeOut != NULL)
			LVP_Player::timeOut->start();

		// Read and fill the packet

		LVP_Player::packetLock.lock();

		auto result = LibFFmpeg::av_read_frame(LVP_Player::formatContext, packet);

		LVP_Player::packetLock.unlock();

		if (LVP_Player::state.quit) {
			FREE_AVPACKET(packet);
			break;
		}

		if (result < 0)
		{
			FREE_AVPACKET(packet);

			if (LVP_Player::timeOut != NULL)
				LVP_Player::timeOut->stop();

			while (LVP_Player::state.isPaused && !LVP_Player::seekRequested && !LVP_Player::state.quit)
				SDL_Delay(DELAY_TIME_DEFAULT);

			if (LVP_Player::seekRequested)
				continue;

			// Media file has completed (EOF) or an error occured

			if ((result == AVERROR_EOF) || LibFFmpeg::avio_feof(LVP_Player::formatContext->pb))
			{
				if (!endOfFile) {
					endOfFile = true;
					continue;
				}
			}
			else
			{
				errorCount++;

				#if defined _DEBUG
					char strerror[AV_ERROR_MAX_STRING_SIZE];
					LibFFmpeg::av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);
					LOG("PACKET_READ: %s\n", strerror);
				#endif

				if (errorCount < MAX_ERRORS)
					continue;
			}

			if (LVP_Player::state.quit)
				break;

			// Wait until all packets have been consumed

			while (!LVP_Player::audioContext->packets.empty() && !LVP_Player::state.quit)
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

		if (LVP_Player::timeOut != NULL)
			LVP_Player::timeOut->stop();

		// INTERNAL STREAMS

		bool isSubExternal = (LVP_Player::subContext->index >= SUB_STREAM_EXTERNAL);

		if (packet->stream_index == LVP_Player::audioContext->index)
		{
			LVP_Player::audioContext->packetsLock.lock();
			LVP_Player::audioContext->packets.push(packet);
			LVP_Player::audioContext->packetsLock.unlock();
		}
		else if (packet->stream_index == LVP_Player::videoContext->index)
		{
			LVP_Player::videoContext->packetsLock.lock();
			LVP_Player::videoContext->packets.push(packet);
			LVP_Player::videoContext->packetsLock.unlock();
		}
		else if ((packet->stream_index == LVP_Player::subContext->index) && !isSubExternal)
		{
			LVP_Player::subContext->packetsLock.lock();
			LVP_Player::subContext->packets.push(packet);
			LVP_Player::subContext->packetsLock.unlock();
		} else {
			FREE_AVPACKET(packet);
		}

		// EXTERNAL SUBTITLE FILES

		if (isSubExternal && (LVP_Player::subContext->packets.size() < MIN_PACKET_QUEUE_SIZE))
		{
			auto packetExternal = LibFFmpeg::av_packet_alloc();
			
			if (packetExternal != NULL)
			{
				LVP_Player::packetLock.lock();

				auto result = LibFFmpeg::av_read_frame(LVP_Player::formatContextExternal, packetExternal);

				LVP_Player::packetLock.unlock();

				int extSubStream = ((LVP_Player::subContext->index - SUB_STREAM_EXTERNAL) % SUB_STREAM_EXTERNAL);

				if ((result >= 0) && (packetExternal->stream_index == extSubStream)) {
					LVP_Player::subContext->packetsLock.lock();
					LVP_Player::subContext->packets.push(packetExternal);
					LVP_Player::subContext->packetsLock.unlock();
				} else {
					FREE_AVPACKET(packetExternal);
				}
			}
		}

		if (LVP_Player::state.quit)
			break;

		// Number of media tracks have changed

		if (LVP_Player::formatContext->nb_streams != LVP_Player::state.trackCount) {
			LVP_Player::state.trackCount = LVP_Player::formatContext->nb_streams;
			LVP_Player::callbackEvents(LVP_EVENT_MEDIA_TRACKS_UPDATED);
		}

		// Metadata has changed

		if (LVP_Player::formatContext->event_flags & AVFMT_EVENT_FLAG_METADATA_UPDATED) {
			LVP_Player::formatContext->event_flags &= ~AVFMT_EVENT_FLAG_METADATA_UPDATED;
			LVP_Player::callbackEvents(LVP_EVENT_METADATA_UPDATED);
		}

		// Wait while packet queues are full

		while (LVP_Player::isPacketQueueFull() && !LVP_Player::seekRequested && !LVP_Player::state.quit)
			SDL_Delay(DELAY_TIME_DEFAULT);
	}

	if (endOfFile || (errorCount >= MAX_ERRORS))
	{
		if (errorCount >= MAX_ERRORS)
			LVP_Player::callbackEvents(LVP_EVENT_MEDIA_COMPLETED_WITH_ERRORS);
		else
			LVP_Player::callbackEvents(LVP_EVENT_MEDIA_COMPLETED);

		LVP_Player::state.completed = true;
	}

	LVP_Player::state.threads[LVP_THREAD_PACKETS] = false;

	return 0;
}

int MediaPlayer::LVP_Player::threadSub()
{
	LVP_Player::state.threads[LVP_THREAD_SUBTITLE] = true;

	LibFFmpeg::AVSubtitle subFrame = {};

	while (!LVP_Player::state.quit)
	{
		// Wait until subtitle packets are available

		while (!LVP_Player::state.quit && (
			(LVP_Player::state.isPaused && !LVP_Player::seekRequestedPaused) ||
			LVP_Player::seekRequested ||
			LVP_Player::trackRequested ||
			(LVP_Player::subContext->index < 0) ||
			LVP_Player::subContext->packets.empty()
		)) {
			SDL_Delay(DELAY_TIME_DEFAULT);
		}

		if (LVP_Player::state.quit)
			break;

		// Get subtitle packet from queue

		auto packet = LVP_Player::getMediaPacket(LVP_Player::subContext);

		if (packet == NULL)
			continue;

		auto packetPTS = LVP_Media::GetPacketPTS(
			packet,
			LVP_Player::subContext->stream->time_base,
			LVP_Player::audioContext->stream->start_time
		);

		// Sub is behind audio, skip packet to catch up.

		auto packetDelayStart = (packetPTS.start - LVP_Player::state.progress);
		auto packetDelayEnd   = (packetPTS.end   - LVP_Player::state.progress);

		if ((packetPTS.end > 0.0) && ((packetDelayStart <= MAX_SUB_DELAY) || (packetDelayEnd <= 0.0)))
		{
			#if defined _DEBUG
				printf("SUB_PACKET_DELAY: %.3fs [%.3fs]\n", packetDelayStart, packetDelayEnd);
			#endif

			if (packetDelayEnd <= 0.0)
				continue;
		}

		bool isExternal = (LVP_Player::subContext->index >= SUB_STREAM_EXTERNAL);

		while (isExternal && ((packetPTS.start - LVP_Player::state.progress) > 1.0) && !LVP_Player::seekRequested && !LVP_Player::state.quit)
			SDL_Delay(1);

		if (LVP_Player::state.quit) {
			FREE_AVPACKET(packet);
			break;
		}

		// Decode subtitle packet to frame

		LVP_Player::packetLock.lock();

		int  frameDecoded;
		auto decodeResult = LibFFmpeg::avcodec_decode_subtitle2(LVP_Player::subContext->codec, &subFrame, &frameDecoded, packet);

		LVP_Player::packetLock.unlock();

		if ((decodeResult < 0) || !frameDecoded) {
			FREE_SUB_FRAME(subFrame);
			FREE_AVPACKET(packet);
			continue;
		}

		if (LVP_Player::state.quit)
			break;

		switch (LVP_Player::subContext->codec->codec->id) {
		case LibFFmpeg::AV_CODEC_ID_DVD_SUBTITLE:
			LVP_SubtitleBitmap::UpdateDVDColorPalette(LVP_Player::subContext->codec->priv_data);
			break;
		case LibFFmpeg::AV_CODEC_ID_HDMV_PGS_SUBTITLE:
			if (subFrame.num_rects == 0)
				LVP_SubtitleBitmap::UpdatePGSEndPTS(packet, LVP_Player::subContext->stream->time_base);
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

		auto framePTS = LVP_Media::GetSubtitlePTS(
			packet,
			subFrame,
			LVP_Player::subContext->stream->time_base,
			LVP_Player::audioContext->stream->start_time
		);

		FREE_AVPACKET(packet);

		// Sub is behind audio, skip frame to catch up.

		auto frameDelayStart = (framePTS.start - LVP_Player::state.progress);
		auto frameDelayEnd   = (framePTS.end   - LVP_Player::state.progress);

		if ((framePTS.end > 0.0) && ((frameDelayStart <= MAX_SUB_DELAY) || (frameDelayEnd <= 0.0)))
		{
			#if defined _DEBUG
				printf("SUB_FRAME_DELAY: %.3fs [%.3fs]\n", frameDelayStart, frameDelayEnd);
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
			case LibFFmpeg::SUBTITLE_BITMAP:
				#if defined _DEBUG
				if (sub->pts.end != 0.0)
					printf("[%.3f,%.3f] %d,%d %dx%d\n", sub->pts.start, sub->pts.end, sub->bitmap.x, sub->bitmap.y, sub->bitmap.w, sub->bitmap.h);
				#endif

				LVP_SubtitleBitmap::ProcessEvent(sub, *subFrame.rects[i]);

				LVP_SubtitleBitmap::queueLock.lock();

				LVP_SubtitleBitmap::queue.push(sub);

				LVP_SubtitleBitmap::queueLock.unlock();
				break;
			case LibFFmpeg::SUBTITLE_ASS:
			case LibFFmpeg::SUBTITLE_TEXT:
				#if defined _DEBUG
				if (sub->dialogue.find("\\vr") == std::string::npos)
					printf("[%.3f,%.3f] %s\n", sub->pts.start, sub->pts.end, sub->dialogue.c_str());
				#endif

				#if defined _ENABLE_LIBASS
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

		while ((LVP_Player::videoContext->packets.empty() || LVP_Player::seekRequested) && !LVP_Player::state.quit)
			SDL_Delay(DELAY_TIME_DEFAULT);

		if (LVP_Player::state.quit)
			break;

		// Get video packet from queue

		auto packet = LVP_Player::getMediaPacket(LVP_Player::videoContext);

		if (packet == NULL)
			continue;

		// Decode video packet to frame

		LVP_Player::packetLock.lock();

		auto result = LibFFmpeg::avcodec_send_packet(LVP_Player::videoContext->codec, packet);

		LVP_Player::packetLock.unlock();

		if ((result < 0) || LVP_Player::state.quit)
		{
			if ((result == AVERROR(EAGAIN)) || (result == AVERROR_EOF))
				continue;

			errorCount++;

			FREE_AVPACKET(packet);

			#if defined _DEBUG
				char strerror[AV_ERROR_MAX_STRING_SIZE];
				LibFFmpeg::av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);
				LOG("VIDEO_SEND_PACKET: %s\n", strerror);
			#endif

			if (errorCount >= MAX_ERRORS) {
				LVP_Player::stop("Failed to decode video.");
				break;
			}

			continue;
		}

		while (result >= 0)
		{
			LVP_Player::packetLock.lock();

			result = LibFFmpeg::avcodec_receive_frame(LVP_Player::videoContext->codec, LVP_Player::videoContext->frameHardware);

			LVP_Player::packetLock.unlock();

			if ((result == AVERROR(EAGAIN)) || (result == AVERROR_EOF))
				break;

			if (result < 0)
			{
				errorCount++;

				#if defined _DEBUG
					char strerror[AV_ERROR_MAX_STRING_SIZE];
					LibFFmpeg::av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);
					LOG("VIDEO_DECODE_FRAME: %s\n", strerror);
				#endif

				break;
			}

			// Process video frame content

			LVP_Player::packetLock.lock();

			if (LVP_Player::isHardwarePixelFormat(LVP_Player::videoContext->frameHardware->format))
			{
				LibFFmpeg::av_hwframe_transfer_data(LVP_Player::videoContext->frameSoftware, LVP_Player::videoContext->frameHardware, 0);

				LVP_Player::videoContext->frameSoftware->best_effort_timestamp = LVP_Player::videoContext->frameHardware->best_effort_timestamp;
				LVP_Player::videoContext->frameSoftware->pts                   = LVP_Player::videoContext->frameHardware->pts;
				LVP_Player::videoContext->frameSoftware->pkt_dts               = LVP_Player::videoContext->frameHardware->pkt_dts;

				LVP_Player::videoContext->frame = LVP_Player::videoContext->frameSoftware;
			} else {
				LVP_Player::videoContext->frame = LVP_Player::videoContext->frameHardware;
			}

			// Calculate video PTS (present time)

			if (IS_AUDIO(LVP_Player::state.mediaType) && (LVP_Player::videoContext->stream->attached_pic.size > 0))
				LVP_Player::videoContext->pts = LVP_Player::state.progress;
			else
				LVP_Player::videoContext->pts = LVP_Media::GetVideoPTS(LVP_Player::videoContext, LVP_Player::audioContext->stream->start_time);

			LVP_Player::packetLock.unlock();

			// Wait while paused (unless a seek is requested)

			while (LVP_Player::state.isPaused && !LVP_Player::seekRequested && !LVP_Player::seekRequestedPaused && !LVP_Player::state.quit)
				SDL_Delay(DELAY_TIME_DEFAULT);

			if (LVP_Player::seekRequested || LVP_Player::state.quit)
				break;

			// Update video if seek was requested while paused

			if (LVP_Player::seekRequestedPaused)
			{
				// Keep seeking until the video catches up to the audio

				if (LVP_Player::videoContext->getTimeUntilPTS(LVP_Player::state.progress) > ONE_SECOND_MS)
					break;

				LVP_Player::seekRequestedPaused = false;
			}

			// Start preparing the video frame for rendering

			LVP_Player::videoContext->isReadyForRender = true;

			// Wait before presenting the video frame

			auto progress = (IS_INVALID_PTS() ? LVP_Player::seekPTS : LVP_Player::state.progress);

			while ((LVP_Player::videoContext->getTimeUntilPTS(progress) >= DELAY_TIME_DEFAULT) && !LVP_Player::state.isPaused && !LVP_Player::state.quit)
			{
				SDL_Delay(DELAY_TIME_DEFAULT);

				progress = (IS_INVALID_PTS() ? LVP_Player::seekPTS : LVP_Player::state.progress);
			}

			if (!IS_INVALID_PTS())
				LVP_Player::seekRequestedBack = false;

			LVP_Player::videoContext->isReadyForPresent = true;
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
