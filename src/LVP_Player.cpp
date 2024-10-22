#include "LVP_Player.h"

MediaPlayer::LVP_AudioContext*    MediaPlayer::LVP_Player::audioContext          = NULL;
MediaPlayer::LVP_AudioDevice      MediaPlayer::LVP_Player::audioDevice           = {};
LVP_CallbackContext               MediaPlayer::LVP_Player::callbackContext       = {};
LibFFmpeg::AVFormatContext*       MediaPlayer::LVP_Player::formatContext         = NULL;
LibFFmpeg::AVFormatContext*       MediaPlayer::LVP_Player::formatContextExternal = NULL;
bool                              MediaPlayer::LVP_Player::isOpening             = false;
bool                              MediaPlayer::LVP_Player::isStopping            = false;
std::mutex                        MediaPlayer::LVP_Player::packetLock            = {};
bool                              MediaPlayer::LVP_Player::seekRequested         = false;
bool                              MediaPlayer::LVP_Player::seekRequestedPaused   = false;
double                            MediaPlayer::LVP_Player::seekRequest           = 0.0;
MediaPlayer::LVP_PlayerState      MediaPlayer::LVP_Player::state                 = {};
MediaPlayer::LVP_SubtitleContext* MediaPlayer::LVP_Player::subContext            = {};
System::LVP_TimeOut*              MediaPlayer::LVP_Player::timeOut               = NULL;
bool                              MediaPlayer::LVP_Player::trackRequested        = false;
LVP_MediaTrack                    MediaPlayer::LVP_Player::trackRequest          = {};
MediaPlayer::LVP_VideoContext*    MediaPlayer::LVP_Player::videoContext          = {};

void MediaPlayer::LVP_Player::Init(const LVP_CallbackContext& callbackContext)
{
	LVP_Player::callbackContext = callbackContext;
	LVP_Player::state.quit      = false;
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

	LVP_Player::pause();

	LVP_Player::state.quit = true;

	while (
		LVP_Player::state.threads[LVP_THREAD_AUDIO]    ||
		LVP_Player::state.threads[LVP_THREAD_PACKETS]  ||
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
	
	auto audioDriver = SDL_GetCurrentAudioDriver();

	if ((audioDriver != NULL) && strcmp(audioDriver, "dummy") == 0)
	{
		SDL_AudioQuit();
		SDL_setenv("SDL_AUDIODRIVER", "", 1);
		SDL_AudioInit(NULL);
	}
}

void MediaPlayer::LVP_Player::closePackets()
{
	while (!LVP_Player::audioContext->packets.empty()) {
		FREE_AVPACKET(LVP_Player::audioContext->packets.front());
		LVP_Player::audioContext->packets.pop();
	}

	while (!LVP_Player::subContext->packets.empty()) {
		FREE_AVPACKET(LVP_Player::subContext->packets.front());
		LVP_Player::subContext->packets.pop();
	}

	while (!LVP_Player::videoContext->packets.empty()) {
		FREE_AVPACKET(LVP_Player::videoContext->packets.front());
		LVP_Player::videoContext->packets.pop();
	}
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
	LVP_SubtitleText::Remove();

	LVP_SubtitleText::Quit();

	DELETE_POINTER(LVP_Player::subContext);
}

void MediaPlayer::LVP_Player::closeThreads()
{
	SDL_CloseAudioDevice(LVP_Player::audioDevice.deviceID);

	LVP_Player::videoContext->renderer = NULL;
}

void MediaPlayer::LVP_Player::closeVideoContext()
{
	DELETE_POINTER(LVP_Player::videoContext);
}

LVP_Strings MediaPlayer::LVP_Player::GetAudioDevices()
{
	LVP_Strings devices;

	for (int i = 0; i < SDL_GetNumAudioDevices(0); i++)
		devices.push_back(SDL_GetAudioDeviceName(i, 0));

	return devices;
}

LibFFmpeg::AVSampleFormat MediaPlayer::LVP_Player::getAudioSampleFormat(SDL_AudioFormat sampleFormat)
{
	switch (sampleFormat) {
		case AUDIO_U8:     return LibFFmpeg::AV_SAMPLE_FMT_U8;
		case AUDIO_S16SYS: return LibFFmpeg::AV_SAMPLE_FMT_S16;
		case AUDIO_S32SYS: return LibFFmpeg::AV_SAMPLE_FMT_S32;
		case AUDIO_F32SYS: return LibFFmpeg::AV_SAMPLE_FMT_FLT;
		default: break;
	}

	return LibFFmpeg::AV_SAMPLE_FMT_NONE;
}

SDL_AudioFormat MediaPlayer::LVP_Player::getAudioSampleFormat(LibFFmpeg::AVSampleFormat sampleFormat)
{
	switch (sampleFormat) {
		case LibFFmpeg::AV_SAMPLE_FMT_U8:
		case LibFFmpeg::AV_SAMPLE_FMT_U8P:
			return AUDIO_U8;
		case LibFFmpeg::AV_SAMPLE_FMT_S16:
		case LibFFmpeg::AV_SAMPLE_FMT_S16P:
			return AUDIO_S16SYS;
		case LibFFmpeg::AV_SAMPLE_FMT_S32:
		case LibFFmpeg::AV_SAMPLE_FMT_S32P:
			return AUDIO_S32SYS;
		case LibFFmpeg::AV_SAMPLE_FMT_FLT:
		case LibFFmpeg::AV_SAMPLE_FMT_FLTP:
			return AUDIO_F32SYS;
		default:
			break;
	}

	return LibFFmpeg::AV_SAMPLE_FMT_NONE;
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
		auto end      = (int64_t)((double)chapter->end   * timeBase * ONE_SECOND_MS);
		auto start    = (int64_t)((double)chapter->start * timeBase * ONE_SECOND_MS);
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

LVP_MediaDetails MediaPlayer::LVP_Player::GetMediaDetails()
{
	if (LVP_Player::state.isStopped)
		return {};

	return
	{
		.duration       = LVP_Media::GetMediaDuration(LVP_Player::formatContext, LVP_Player::audioContext->stream),
		.fileSize       = System::LVP_FileSystem::GetFileSize(LVP_Player::formatContext->url),
		.mediaType      = (LVP_MediaType)LVP_Player::state.mediaType,
		.meta           = LVP_Media::GetMediaMeta(LVP_Player::formatContext),
		.thumbnail      = LVP_Media::GetMediaThumbnail(LVP_Player::formatContext),
		.chapters       = LVP_Player::GetChapters(),
		.audioTracks    = LVP_Player::GetAudioTracks(),
		.subtitleTracks = LVP_Player::GetSubtitleTracks(),
		.videoTracks    = LVP_Player::GetVideoTracks()
	};
}

LVP_MediaDetails MediaPlayer::LVP_Player::GetMediaDetails(const std::string& filePath)
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
		.thumbnail      = LVP_Media::GetMediaThumbnail(formatContext),
		.chapters       = LVP_Player::getChapters(formatContext),
		.audioTracks    = LVP_Player::getAudioTracks(formatContext),
		.subtitleTracks = LVP_Player::getSubtitleTracks(formatContext, extSubFiles),
		.videoTracks    = LVP_Player::getVideoTracks(formatContext)
	};

	FREE_AVFORMAT(formatContext);

	return details;
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

	if (subCount < 1)
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
	return (int64_t)(LVP_Player::state.progress * ONE_SECOND_MS);
}

SDL_Rect MediaPlayer::LVP_Player::getScaledVideoDestination(const SDL_Rect& destination)
{
	if (SDL_RectEmpty(&destination))
		return {};

	int videoWidth   = LVP_Player::videoContext->stream->codecpar->width;
	int videoHeight  = LVP_Player::videoContext->stream->codecpar->height;
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

	LVP_SubtitleBitmap::Remove();
	LVP_SubtitleText::Remove();

	LVP_Player::audioContext->bufferOffset    = 0;
	LVP_Player::audioContext->bufferRemaining = 0;
	LVP_Player::audioContext->writtenToStream = 0;
	LVP_Player::audioContext->decodeFrame     = true;
	LVP_Player::audioContext->lastPogress     = 0.0;

	int     seekFlags    = 0;
	int64_t seekPosition = -1;

	if (IS_BYTE_SEEK(LVP_Player::formatContext->iformat) && (LVP_Player::state.fileSize > 0)) {
		seekFlags    = AVSEEK_FLAG_BYTE;
		seekPosition = (int64_t)((double)LVP_Player::state.fileSize * LVP_Player::seekRequest);
	} else {
		seekPosition = (int64_t)(((double)LVP_Player::state.duration * AV_TIME_BASE_D) * LVP_Player::seekRequest);
	}

	if (LibFFmpeg::avformat_seek_file(LVP_Player::formatContext, -1, INT64_MIN, seekPosition, INT64_MAX, seekFlags) >= 0)
	{
		if (LVP_Player::subContext->index >= SUB_STREAM_EXTERNAL)
			LibFFmpeg::avformat_seek_file(LVP_Player::formatContextExternal, -1, INT64_MIN, seekPosition, INT64_MAX, 0);

		LVP_Player::state.progress = ((double)seekPosition / AV_TIME_BASE_D);
	}

	if (LVP_Player::subContext->codec != NULL)
		LibFFmpeg::avcodec_flush_buffers(LVP_Player::subContext->codec);

	if (LVP_Player::videoContext->codec != NULL)
		LibFFmpeg::avcodec_flush_buffers(LVP_Player::videoContext->codec);

	LVP_Player::seekRequested = false;

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

	auto mediaType        = (LibFFmpeg::AVMediaType)LVP_Player::trackRequest.mediaType;
	bool isSameAudioTrack = (IS_AUDIO(mediaType) && (LVP_Player::trackRequest.track == LVP_Player::audioContext->index));
	bool isSameSubTrack   = (IS_SUB(mediaType)   && (LVP_Player::trackRequest.track == LVP_Player::subContext->index));

	if (isSameAudioTrack || isSameSubTrack)
	{
		LVP_Player::trackRequested = false;

		LVP_Player::packetLock.unlock();

		return;
	}

	// DISABLE SUBS

	if (IS_SUB(mediaType) && (LVP_Player::trackRequest.track < 0))
	{
		LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_SUBTITLE);

		LVP_SubtitleBitmap::Remove();
		LVP_SubtitleText::Remove();

		LVP_Player::trackRequested = false;

		LVP_Player::packetLock.unlock();

		return;
	}

	bool isPaused     = LVP_Player::state.isPaused;
	auto lastProgress = (double)(LVP_Player::state.progress / (double)LVP_Player::state.duration);

	LVP_Player::audioDevice.isDeviceReady = false;

	switch (mediaType) {
	case LibFFmpeg::AVMEDIA_TYPE_AUDIO:
		if (!isPaused)
			SDL_PauseAudioDevice(LVP_Player::audioDevice.deviceID, 1);

		LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_AUDIO);

		LVP_Media::SetMediaTrackByIndex(LVP_Player::formatContext, LVP_Player::trackRequest.track, LVP_Player::audioContext);

		try {
			LVP_Player::openThreadAudio();
		} catch (const std::exception& e) {
			LVP_Player::CallbackError(e.what());
		}

		if (!LVP_Player::seekRequested)
			LVP_Player::seekTo(lastProgress);

		if (!isPaused)
			SDL_PauseAudioDevice(LVP_Player::audioDevice.deviceID, 0);

		break;
	case LibFFmpeg::AVMEDIA_TYPE_SUBTITLE:
		LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_SUBTITLE);

		LVP_SubtitleBitmap::Remove();
		LVP_SubtitleText::Remove();

		if (LVP_Player::trackRequest.track >= SUB_STREAM_EXTERNAL)
			LVP_Player::openSubExternal(LVP_Player::trackRequest.track);
		else
			LVP_Media::SetMediaTrackByIndex(LVP_Player::formatContext, LVP_Player::trackRequest.track, LVP_Player::subContext);

		LVP_Player::openThreadSub();

		if (!LVP_Player::seekRequested)
			LVP_Player::seekTo(lastProgress);

		break;
	default:
		break;
	}

	LVP_Player::audioDevice.isDeviceReady = true;

	LVP_Player::trackRequested = false;

	LVP_Player::packetLock.unlock();
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::initAudioFilter()
{
	auto abuffer     = LibFFmpeg::avfilter_get_by_name("abuffer");
	auto abuffersink = LibFFmpeg::avfilter_get_by_name("abuffersink");
	auto atempo      = LibFFmpeg::avfilter_get_by_name("atempo");

	auto filterGraph  = LibFFmpeg::avfilter_graph_alloc();
	auto bufferSource = LibFFmpeg::avfilter_graph_alloc_filter(filterGraph, abuffer,     "src");
	auto bufferSink   = LibFFmpeg::avfilter_graph_alloc_filter(filterGraph, abuffersink, "sink");
	auto filterATempo = LibFFmpeg::avfilter_graph_alloc_filter(filterGraph, atempo,      "atempo");

	auto channelLayout = LVP_Media::GetAudioChannelLayout(LVP_Player::audioContext->specs.channelLayout);
	auto sampleFormat  = LibFFmpeg::av_get_sample_fmt_name((LibFFmpeg::AVSampleFormat)LVP_Player::audioContext->specs.format);
	auto sampleRate    = LVP_Player::audioContext->specs.sampleRate;
	auto timeBase      = LVP_Player::audioContext->stream->time_base;

	LibFFmpeg::av_opt_set(bufferSource,     "channel_layout", channelLayout.c_str(), AV_OPT_SEARCH_CHILDREN);
	LibFFmpeg::av_opt_set(bufferSource,     "sample_fmt",     sampleFormat,          AV_OPT_SEARCH_CHILDREN);
	LibFFmpeg::av_opt_set_int(bufferSource, "sample_rate",    sampleRate,            AV_OPT_SEARCH_CHILDREN);
	LibFFmpeg::av_opt_set_q(bufferSource,   "time_base",      timeBase,              AV_OPT_SEARCH_CHILDREN);

	LibFFmpeg::av_opt_set_double(filterATempo, "tempo", LVP_Player::audioContext->specs.playbackSpeed, AV_OPT_SEARCH_CHILDREN);

	LibFFmpeg::avfilter_init_str(bufferSource, NULL);
	LibFFmpeg::avfilter_init_str(bufferSink,   NULL);
	LibFFmpeg::avfilter_init_str(filterATempo, NULL);

	LibFFmpeg::avfilter_link(bufferSource, 0, filterATempo, 0);
	LibFFmpeg::avfilter_link(filterATempo, 0, bufferSink, 0);

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
		return ((LVP_Player::audioContext->stream != NULL) && (LVP_Player::audioContext->packets.size() >= MIN_PACKET_QUEUE_SIZE));
	case LibFFmpeg::AVMEDIA_TYPE_SUBTITLE:
		return ((LVP_Player::subContext->stream != NULL) && (LVP_Player::subContext->packets.size() >= MIN_PACKET_QUEUE_SIZE));
	case LibFFmpeg::AVMEDIA_TYPE_VIDEO:
		return ((LVP_Player::videoContext->stream != NULL) && (LVP_Player::videoContext->packets.size() >= MIN_PACKET_QUEUE_SIZE));
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

		LVP_Player::play();

		LVP_Player::isOpening = false;
	} catch (const std::exception& e) {
		MediaPlayer::LVP_Player::CallbackError(System::LVP_Text::Format("Failed to open media file:\n%s", e.what()));
		LVP_Player::close();
	}
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openAudioDevice(const SDL_AudioSpec& wantedSpecs)
{
	bool isDefault = (LVP_Player::audioDevice.device == "Default");
	bool isEmpty   = LVP_Player::audioDevice.device.empty();
	auto newDevice = (!isEmpty && !isDefault ? LVP_Player::audioDevice.device.c_str() : NULL);

	if (LVP_Player::audioDevice.deviceID >= 2)
		SDL_CloseAudioDevice(LVP_Player::audioDevice.deviceID);

	if (SDL_GetNumAudioDevices(0) < 1)
	{
		SDL_AudioQuit();
		SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
		SDL_AudioInit("dummy");
	}

	LVP_Player::audioDevice.deviceID = SDL_OpenAudioDevice(
		newDevice,
		0,
		&wantedSpecs,
		&LVP_Player::audioContext->deviceSpecs,
		SDL_AUDIO_ALLOW_ANY_CHANGE
	);

	if (LVP_Player::audioDevice.deviceID < 2)
	{
		#if defined _DEBUG
			LOG("%s\n", SDL_GetError());
		#endif

		SDL_CloseAudioDevice(LVP_Player::audioDevice.deviceID);

		throw std::runtime_error(System::LVP_Text::Format("Failed to open a valid audio device: %u", LVP_Player::audioDevice.deviceID).c_str());
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
	if ((LVP_Player::audioContext->stream == NULL) || (LVP_Player::audioContext->stream->codecpar == NULL))
		throw std::runtime_error("Audio context stream is missing a valid codec.");

	LVP_Player::audioContext->frame = LibFFmpeg::av_frame_alloc();
	LVP_Player::audioContext->index = LVP_Player::audioContext->stream->index;

	if (LVP_Player::audioContext->frame == NULL)
		throw std::runtime_error("Failed to allocate an audio context frame.");

	auto channelCount = LVP_Player::audioContext->stream->codecpar->ch_layout.nb_channels;
	auto sampleRate   = LVP_Player::audioContext->stream->codecpar->sample_rate;

	if (channelCount < 1) {
		LibFFmpeg::av_channel_layout_default(&LVP_Player::audioContext->stream->codecpar->ch_layout, 2);
		channelCount = LVP_Player::audioContext->stream->codecpar->ch_layout.nb_channels;
	}

	if ((sampleRate < 1) || (channelCount < 1))
		throw std::runtime_error(System::LVP_Text::Format("Invalid audio: %d channels, %d bps", channelCount, sampleRate).c_str());

	// https://developer.apple.com/documentation/audiotoolbox/kaudiounitproperty_maximumframesperslice

	LVP_Player::audioContext->deviceSpecsWanted.callback = LVP_Player::threadAudio;
	LVP_Player::audioContext->deviceSpecsWanted.channels = channelCount;
	LVP_Player::audioContext->deviceSpecsWanted.format   = LVP_Player::getAudioSampleFormat(LVP_Player::audioContext->codec->sample_fmt);
	LVP_Player::audioContext->deviceSpecsWanted.freq     = sampleRate;
	LVP_Player::audioContext->deviceSpecsWanted.samples  = 4096;

	LVP_Player::openAudioDevice(LVP_Player::audioContext->deviceSpecsWanted);

	LVP_Player::audioContext->bufferOffset    = 0;
	LVP_Player::audioContext->bufferRemaining = 0;
	LVP_Player::audioContext->bufferSize      = 768000;

	LVP_Player::audioContext->decodeFrame     = true;
	LVP_Player::audioContext->frameEncoded    = (uint8_t*)std::malloc(LVP_Player::audioContext->bufferSize);
	LVP_Player::audioContext->writtenToStream = 0;

	if (LVP_Player::audioContext->frameEncoded == NULL)
		throw std::runtime_error("Failed to allocate an encoded audio context frame.");
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

	std::thread(LVP_Player::threadSub).detach();

	LVP_Player::subContext->formatContext = LVP_Player::formatContext;

	LVP_Player::subContext->videoSize = {
		LVP_Player::videoContext->stream->codecpar->width,
		LVP_Player::videoContext->stream->codecpar->height
	};

	LVP_SubtitleText::Init(LVP_Player::subContext);
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openThreadVideo()
{
	if ((LVP_Player::videoContext->stream == NULL) || (LVP_Player::videoContext->stream->codecpar == NULL))
		throw std::runtime_error("Video context stream is missing a valid codec.");

	int videoWidth  = LVP_Player::videoContext->stream->codecpar->width;
	int videoHeight = LVP_Player::videoContext->stream->codecpar->height;

	// SURFACE

	LVP_Player::videoContext->surface = SDL_CreateRGBSurfaceWithFormat(0, videoWidth, videoHeight, 24, SDL_PIXELFORMAT_RGB24);

	// RENDERER

	LVP_Player::videoContext->isSoftwareRenderer = (LVP_Player::callbackContext.hardwareRenderer == NULL);

	if (LVP_Player::videoContext->isSoftwareRenderer)
		LVP_Player::videoContext->renderer = SDL_CreateSoftwareRenderer(LVP_Player::videoContext->surface);
	else
		LVP_Player::videoContext->renderer = LVP_Player::callbackContext.hardwareRenderer;

	if (LVP_Player::videoContext->renderer == NULL)
		throw std::runtime_error(System::LVP_Text::Format("Failed to create a renderer: %s", SDL_GetError()).c_str());

	// TEXTURE

	LVP_Player::videoContext->texture = SDL_CreateTexture(LVP_Player::videoContext->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, videoWidth, videoHeight);

	SDL_SetTextureBlendMode(LVP_Player::videoContext->texture, SDL_BLENDMODE_NONE);

	// FRAME

	LVP_Player::videoContext->frameHardware = LibFFmpeg::av_frame_alloc();
	LVP_Player::videoContext->frameSoftware = LibFFmpeg::av_frame_alloc();
	LVP_Player::videoContext->frameRate     = (1.0 / LVP_Media::GetMediaFrameRate(LVP_Player::videoContext->stream));

	if ((LVP_Player::videoContext->frameHardware == NULL) ||
		(LVP_Player::videoContext->frameSoftware == NULL) ||
		(LVP_Player::videoContext->frameRate <= 0))
	{
		throw std::runtime_error("Failed to allocate a video context frame.");
	}

	if (LVP_Player::videoContext->frameEncoded != NULL)
		LVP_Player::videoContext->frameEncoded->linesize[0] = 0;

	// THREAD

	std::thread(LVP_Player::threadVideo).detach();

	// STREAM/TRACK

	LVP_Player::videoContext->index = LVP_Player::videoContext->stream->index;
}

void MediaPlayer::LVP_Player::pause()
{
	if (LVP_Player::state.isPaused || LVP_Player::state.filePath.empty())
		return;

	LVP_Player::state.isPaused  = true;
	LVP_Player::state.isPlaying = false;
	LVP_Player::state.isStopped = false;

	SDL_PauseAudioDevice(LVP_Player::audioDevice.deviceID, 1);

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_PAUSED);
}

void MediaPlayer::LVP_Player::play()
{
	if (LVP_Player::state.isPlaying || LVP_Player::state.filePath.empty())
		return;

	LVP_Player::state.isPlaying = true;
	LVP_Player::state.isPaused  = false;
	LVP_Player::state.isStopped = false;

	SDL_PauseAudioDevice(LVP_Player::audioDevice.deviceID, 0);

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_PLAYING);
}

void MediaPlayer::LVP_Player::Quit()
{
	LVP_Player::close();
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

	if (LVP_Player::trackRequested)
		LVP_Player::handleTrack();

	if (LVP_Player::seekRequested)
		LVP_Player::handleSeek();

	if (LVP_Player::videoContext->isReadyForRender)
	{
		LVP_Player::videoContext->isReadyForRender = false;

		LVP_Player::renderVideo();

		LVP_SubtitleBitmap::RemoveExpired(LVP_Player::state.progress);

		LVP_SubtitleBitmap::Render(LVP_Player::videoContext->surface, LVP_Player::subContext, LVP_Player::state.progress);
		
		LVP_SubtitleText::Render(LVP_Player::videoContext->surface, LVP_Player::state.progress);
	}

	if (LVP_Player::videoContext->isReadyForPresent)
	{
		LVP_Player::videoContext->isReadyForPresent = false;

		auto surface = LVP_Player::videoContext->surface;

		if (!LVP_Player::videoContext->isSoftwareRenderer)
			SDL_UpdateTexture(LVP_Player::videoContext->texture, NULL, surface->pixels, surface->pitch);
		else
			LVP_Player::callbackVideoIsAvailable(surface);
	}

	if (!LVP_Player::videoContext->isSoftwareRenderer)
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

	auto videoWidth  = LVP_Player::videoContext->stream->codecpar->width;
	auto videoHeight = LVP_Player::videoContext->stream->codecpar->height;

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
			LibFFmpeg::AV_PIX_FMT_RGB24,
			1
		);

		if (result <= 0)
			FREE_AVFRAME(LVP_Player::videoContext->frameEncoded);
	}

	auto pixelFormat = (LibFFmpeg::AVPixelFormat)LVP_Player::videoContext->frame->format;

	if ((LVP_Player::videoContext->frameEncoded == NULL) || (pixelFormat == LibFFmpeg::AV_PIX_FMT_NONE))
		return;

	LVP_Player::videoContext->scaleContext = LibFFmpeg::sws_getCachedContext(
		LVP_Player::videoContext->scaleContext,
		videoWidth,
		videoHeight,
		pixelFormat,
		videoWidth,
		videoHeight,
		LibFFmpeg::AV_PIX_FMT_RGB24,
		DEFAULT_SCALE_FILTER,
		NULL,
		NULL,
		NULL
	);

	if (LVP_Player::videoContext->scaleContext == NULL)
		return;

	auto scaleResult = LibFFmpeg::sws_scale(
		LVP_Player::videoContext->scaleContext,
		LVP_Player::videoContext->frame->data,
		LVP_Player::videoContext->frame->linesize,
		0,
		LVP_Player::videoContext->frame->height,
		LVP_Player::videoContext->frameEncoded->data,
		LVP_Player::videoContext->frameEncoded->linesize
	);

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
	LVP_Player::seekRequest   = percent;
	LVP_Player::seekRequested = true;

	if (IS_VIDEO(LVP_Player::state.mediaType) && LVP_Player::state.isPaused)
		LVP_Player::seekRequestedPaused = true;
}

bool MediaPlayer::LVP_Player::SetAudioDevice(const std::string& device)
{
	LVP_Player::audioDevice.isDeviceReady = false;

	bool isSuccess = true;
	bool isPaused  = LVP_Player::state.isPaused;

	if (!isPaused)
		SDL_PauseAudioDevice(LVP_Player::audioDevice.deviceID, 1);

	auto currentDevice = std::string(LVP_Player::audioDevice.device);

	LVP_Player::audioDevice.device = std::string(device);

	if (!LVP_Player::state.isStopped)
	{
		try {
			LVP_Player::openAudioDevice(LVP_Player::audioContext->deviceSpecsWanted);
		} catch (const std::exception& e) {
			isSuccess = false;
			LVP_Player::CallbackError(e.what());
		}
	}

	if (!isPaused)
		SDL_PauseAudioDevice(audioDevice.deviceID, 0);

	LVP_Player::audioDevice.isDeviceReady = true;

	return isSuccess;
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
	auto newSpeed = std::max(0.5, std::min(2.0, speed));

	if (!ARE_DIFFERENT_DOUBLES(newSpeed, LVP_Player::state.playbackSpeed))
		return;

	LVP_Player::state.playbackSpeed = newSpeed;

	LVP_Player::callbackEvents(LVP_EVENT_PLAYBACK_SPEED_CHANGED);
}

void MediaPlayer::LVP_Player::SetTrack(const LVP_MediaTrack& track)
{
	if (LVP_Player::formatContext == NULL)
		return;

	LVP_Player::packetLock.lock();

	LVP_Player::trackRequest   = track;
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

void MediaPlayer::LVP_Player::threadAudio(void* userData, Uint8* stream, int streamSize)
{
	if (!LVP_Player::audioDevice.isDeviceReady || !LVP_Player::audioContext->isDriverReady)
		return;

	LVP_Player::state.threads[LVP_THREAD_AUDIO] = true;

	while (LVP_Player::state.isPaused && !LVP_Player::state.quit)
		SDL_Delay(DELAY_TIME_DEFAULT);

	while ((streamSize > 0) && LVP_Player::state.isPlaying && !LVP_Player::seekRequested && !LVP_Player::state.quit)
	{
		if (LVP_Player::audioContext->decodeFrame)
		{
			// Get audio packet from queue

			LibFFmpeg::AVPacket* packet = NULL;

			LVP_Player::audioContext->packetsLock.lock();

			if (!LVP_Player::audioContext->packets.empty()) {
				packet = LVP_Player::audioContext->packets.front();
				LVP_Player::audioContext->packets.pop();
			}

			LVP_Player::audioContext->packetsLock.unlock();

			if (packet == NULL)
				break;

			// Decode audio packet to frame

			auto result = LibFFmpeg::avcodec_send_packet(LVP_Player::audioContext->codec, packet);

			if ((result < 0) || LVP_Player::state.quit) {
				FREE_AVPACKET(packet);
				break;
			}

			// Some packets contain multiple frames

			while (result == 0)
			{
				result = LibFFmpeg::avcodec_receive_frame(LVP_Player::audioContext->codec, LVP_Player::audioContext->frame);

				if (result < 0)
				{
					if (result != AVERROR(EAGAIN))
					{
						#if defined _DEBUG
							char strerror[AV_ERROR_MAX_STRING_SIZE];
							LibFFmpeg::av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);
							LOG("%s\n", strerror);
						#endif

						LVP_Player::audioContext->bufferRemaining = 0;
					}

					LVP_Player::audioContext->bufferOffset = 0;

					break;
				}

				// Apply atempo (speed) filter to frame

				bool specsHaveChanged = LVP_Player::audioContext->specs.hasChanged(LVP_Player::audioContext->frame, LVP_Player::state.playbackSpeed);

				if (specsHaveChanged || (LVP_Player::audioContext->filter.filterGraph == NULL)) {
					LVP_Player::audioContext->specs.init(LVP_Player::audioContext->frame, LVP_Player::state.playbackSpeed);
					LVP_Player::initAudioFilter();
				}

				LibFFmpeg::av_buffersrc_add_frame(LVP_Player::audioContext->filter.bufferSource, LVP_Player::audioContext->frame);
				
				auto filterResult = LibFFmpeg::av_buffersink_get_frame(LVP_Player::audioContext->filter.bufferSink, LVP_Player::audioContext->frame);

				// Get more frames if needed

				if (filterResult == AVERROR(EAGAIN))
					continue;

				// FRAME DURATION

				if (packet->duration > 0)
				{
					auto packetDuration = (double)packet->duration;
					auto streamTimeBase = LibFFmpeg::av_q2d(LVP_Player::audioContext->stream->time_base);

					LVP_Player::audioContext->frameDuration = (packetDuration * streamTimeBase);
				}

				// PROGRESS

				if (LVP_Player::state.progress > 0)
					LVP_Player::audioContext->lastPogress = LVP_Player::state.progress;

				LVP_Player::state.progress = LVP_Media::GetAudioPTS(LVP_Player::audioContext);

				if ((LVP_Player::audioContext->frameDuration <= 0) &&
					(LVP_Player::state.progress > 0) &&
					(LVP_Player::audioContext->lastPogress > 0) &&
					(LVP_Player::state.progress != LVP_Player::audioContext->lastPogress))
				{
					LVP_Player::audioContext->frameDuration = (LVP_Player::state.progress - LVP_Player::audioContext->lastPogress);
					LVP_Player::audioContext->lastPogress   = LVP_Player::state.progress;
				}

				// RESAMPLE CONTEXT

				LibFFmpeg::AVChannelLayout outChannelLayout, inChannelLayout;

				LibFFmpeg::av_channel_layout_default(&outChannelLayout, LVP_Player::audioContext->deviceSpecs.channels);
				LibFFmpeg::av_channel_layout_default(&inChannelLayout,  LVP_Player::audioContext->frame->ch_layout.nb_channels);

				auto inSampleFormat = (LibFFmpeg::AVSampleFormat)LVP_Player::audioContext->frame->format;
				auto inSampleRate   = LVP_Player::audioContext->frame->sample_rate;

				auto outSampleFormat = LVP_Player::getAudioSampleFormat(LVP_Player::audioContext->deviceSpecs.format);
				auto outSampleRate   = LVP_Player::audioContext->deviceSpecs.freq;

				if (LVP_Player::audioContext->specs.initContext || (LVP_Player::audioContext->specs.swrContext == NULL))
				{
					LVP_Player::audioContext->specs.initContext = false;

					FREE_SWR(LVP_Player::audioContext->specs.swrContext);

					auto result = LibFFmpeg::swr_alloc_set_opts2(
						&LVP_Player::audioContext->specs.swrContext,
						&outChannelLayout,
						outSampleFormat,
						outSampleRate,
						&inChannelLayout,
						inSampleFormat,
						inSampleRate,
						0,
						NULL
					);

					if ((result < 0) || (LVP_Player::audioContext->specs.swrContext == NULL)) {
						LVP_Player::stop("Failed to allocate an audio resample context.");
						break;
					}

					if (LibFFmpeg::swr_is_initialized(LVP_Player::audioContext->specs.swrContext) < 1)
					{
						if (LibFFmpeg::swr_init(LVP_Player::audioContext->specs.swrContext) < 0) {
							LVP_Player::stop("Failed to initialize the audio resample context.");
							break;
						}
					}
				}

				if (LVP_Player::state.quit)
					break;

				// Calculate needed frame buffer size for resampling
				
				auto bytesPerSample  = (LVP_Player::audioContext->deviceSpecs.channels * LibFFmpeg::av_get_bytes_per_sample(outSampleFormat));
				auto nextSampleCount = LibFFmpeg::swr_get_out_samples(LVP_Player::audioContext->specs.swrContext, LVP_Player::audioContext->frame->nb_samples);
				auto nextBufferSize  = (nextSampleCount * bytesPerSample);

				// Increase the frame buffer size if needed
				
				if (nextBufferSize > LVP_Player::audioContext->bufferSize)
				{
					auto frameEncoded = (uint8_t*)std::realloc(LVP_Player::audioContext->frameEncoded, nextBufferSize);

					LVP_Player::audioContext->frameEncoded = frameEncoded;
					LVP_Player::audioContext->bufferSize   = nextBufferSize;
				}

				// Resample the audio frame

				auto samplesResampled = LibFFmpeg::swr_convert(
					LVP_Player::audioContext->specs.swrContext,
					&LVP_Player::audioContext->frameEncoded,
					nextSampleCount,
					(const uint8_t**)LVP_Player::audioContext->frame->extended_data,
					LVP_Player::audioContext->frame->nb_samples
				);

				if (samplesResampled < 0) {
					LVP_Player::stop("Failed to resample the audio frame.");
					break;
				}

				if (LVP_Player::state.quit)
					break;

				// Calculate the actual frame buffer size after resampling

				LVP_Player::audioContext->bufferRemaining = (samplesResampled * bytesPerSample);
				LVP_Player::audioContext->bufferOffset    = 0;
			}

			FREE_AVPACKET(packet);
		}

		if (LVP_Player::seekRequested || LVP_Player::state.quit)
			break;

		// Calculate how much to write to the stream

		int writeSize;

		if (LVP_Player::audioContext->writtenToStream > 0)
			writeSize = (streamSize - LVP_Player::audioContext->writtenToStream);
		else if (LVP_Player::audioContext->bufferRemaining > streamSize)
			writeSize = streamSize;
		else
			writeSize = LVP_Player::audioContext->bufferRemaining;

		if (writeSize > LVP_Player::audioContext->bufferRemaining)
			writeSize = LVP_Player::audioContext->bufferRemaining;

		// Write the data from the buffer to the stream

		std::memset(stream, 0, writeSize);

		auto buffer = static_cast<const Uint8*>(LVP_Player::audioContext->frameEncoded + LVP_Player::audioContext->bufferOffset);
		auto volume = (!LVP_Player::state.isMuted ? LVP_Player::state.volume : 0);

		SDL_MixAudioFormat(stream, buffer, LVP_Player::audioContext->deviceSpecs.format, writeSize, volume);

		// Update counters with how much was written

		stream                                    += writeSize;
		LVP_Player::audioContext->writtenToStream += writeSize;
		LVP_Player::audioContext->bufferRemaining -= writeSize;

		// Stream is full - Buffer is not full - Get a new stream, don't decode frame.
		if (((LVP_Player::audioContext->writtenToStream % streamSize) == 0) && (LVP_Player::audioContext->bufferRemaining > 0))
		{
			LVP_Player::audioContext->decodeFrame     = false;
			LVP_Player::audioContext->writtenToStream = 0;
			LVP_Player::audioContext->bufferOffset   += writeSize;

			break;
		}
		// Stream is full - Buffer is full - Get a new stream, decode frame.
		else if (((LVP_Player::audioContext->writtenToStream % streamSize) == 0) && (LVP_Player::audioContext->bufferRemaining <= 0))
		{
			LVP_Player::audioContext->decodeFrame     = true;
			LVP_Player::audioContext->writtenToStream = 0;

			break;
		}
		// Stream is not full - Buffer is full - Don't get a new stream, decode frame.
		else if (((LVP_Player::audioContext->writtenToStream % streamSize) != 0) && (LVP_Player::audioContext->bufferRemaining <= 0))
		{
			LVP_Player::audioContext->decodeFrame = true;
		}
		// Stream is not full - Buffer is not full - Don't get a new stream, don't decode frame.
		else if (((LVP_Player::audioContext->writtenToStream % streamSize) != 0) && (LVP_Player::audioContext->bufferRemaining > 0))
		{
			LVP_Player::audioContext->decodeFrame   = false;
			LVP_Player::audioContext->bufferOffset += writeSize;
		}
	}

	LVP_Player::state.threads[LVP_THREAD_AUDIO] = false;
}

int MediaPlayer::LVP_Player::threadPackets()
{
	LVP_Player::state.threads[LVP_THREAD_PACKETS] = true;

	bool endOfFile  = false;
	int  errorCount = 0;

	while (!LVP_Player::state.quit)
	{
		if (LVP_Player::seekRequested)
			LVP_Player::closePackets();

		while (LVP_Player::seekRequested && !LVP_Player::state.quit)
			SDL_Delay(DELAY_TIME_ONE_MS);

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

		if ((result < 0) || LVP_Player::state.quit)
		{
			if (LVP_Player::timeOut != NULL)
				LVP_Player::timeOut->stop();

			// Media file has completed (EOF) or an error occured

			if ((result == AVERROR_EOF) || LibFFmpeg::avio_feof(LVP_Player::formatContext->pb))
				endOfFile = true;
			else
				errorCount++;

			#if defined _DEBUG
			if (!endOfFile) {
				char strerror[AV_ERROR_MAX_STRING_SIZE];
				LibFFmpeg::av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);
				LOG("%s\n", strerror);
			}
			#endif

			if (LVP_Player::state.quit)
				break;

			if (endOfFile || (errorCount >= MAX_ERRORS))
			{
				FREE_AVPACKET(packet);

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
		} else if (errorCount > 0) {
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
			auto packet = LibFFmpeg::av_packet_alloc();
			
			if (packet != NULL)
			{
				LVP_Player::packetLock.lock();

				auto result = av_read_frame(LVP_Player::formatContextExternal, packet);

				LVP_Player::packetLock.unlock();

				int extSubStream = ((LVP_Player::subContext->index - SUB_STREAM_EXTERNAL) % SUB_STREAM_EXTERNAL);

				if ((result == 0) && (packet->stream_index == extSubStream))
				{
					LVP_Player::subContext->packetsLock.lock();
					LVP_Player::subContext->packets.push(packet);
					LVP_Player::subContext->packetsLock.unlock();
				} else {
					FREE_AVPACKET(packet);
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
			SDL_Delay(DELAY_TIME_ONE_MS);
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
			LVP_Player::subContext->packets.empty() ||
			(LVP_Player::state.isPaused && !LVP_Player::seekRequestedPaused) ||
			LVP_Player::seekRequested ||
			LVP_Player::trackRequested ||
			(LVP_Player::subContext->index < 0)
			)) {
			SDL_Delay(DELAY_TIME_DEFAULT);
		}

		if (LVP_Player::state.quit)
			break;

		// Get subtitle packet from queue

		LibFFmpeg::AVPacket* packet = NULL;

		LVP_Player::subContext->packetsLock.lock();

		if (!LVP_Player::subContext->packets.empty()) {
			packet = LVP_Player::subContext->packets.front();
			LVP_Player::subContext->packets.pop();
		}

		LVP_Player::subContext->packetsLock.unlock();

		if (packet == NULL)
			continue;

		// Decode subtitle packet to frame

		int  frameDecoded;
		auto decodeResult = LibFFmpeg::avcodec_decode_subtitle2(LVP_Player::subContext->codec, &subFrame, &frameDecoded, packet);

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

		auto delayTimeStart = (framePTS.start - LVP_Player::state.progress);
		auto delayTimeEnd   = (framePTS.end   - LVP_Player::state.progress);

		if ((framePTS.end > 0.0) && ((delayTimeStart <= MAX_SUB_DELAY) || (delayTimeEnd <= 0.0)))
		{
			#if defined _DEBUG
				printf("DELAY: %.3fs [%.3fs]\n", delayTimeStart, delayTimeEnd);
			#endif

			if (delayTimeEnd <= 0.0)
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

				LVP_SubtitleText::ProcessEvent(sub);

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

	int  errorCount         = 0;
	auto maxFrameDurationS  = (LVP_Player::videoContext->frameRate * 2.0);
	auto maxFrameDurationMS = (int)(maxFrameDurationS * ONE_SECOND_MS);

	while (!LVP_Player::state.quit)
	{
		// Wait until video packets are available

		while ((LVP_Player::videoContext->packets.empty() || LVP_Player::seekRequested) && !LVP_Player::state.quit)
			SDL_Delay(DELAY_TIME_DEFAULT);

		if (LVP_Player::state.quit)
			break;

		// Get video packet from queue

		LibFFmpeg::AVPacket* packet = NULL;

		LVP_Player::videoContext->packetsLock.lock();

		if (!LVP_Player::videoContext->packets.empty()) {
			packet = LVP_Player::videoContext->packets.front();
			LVP_Player::videoContext->packets.pop();
		}

		LVP_Player::videoContext->packetsLock.unlock();

		if (packet == NULL)
			continue;

		// Decode video packet to frame

		LVP_Player::packetLock.lock();

		auto result = LibFFmpeg::avcodec_send_packet(LVP_Player::videoContext->codec, packet);

		LVP_Player::packetLock.unlock();

		if ((result < 0) || LVP_Player::state.quit) {
			FREE_AVPACKET(packet);
			continue;
		}

		result = LibFFmpeg::avcodec_receive_frame(LVP_Player::videoContext->codec, LVP_Player::videoContext->frameHardware);

		FREE_AVPACKET(packet);

		if (LVP_Player::state.quit)
			break;

		// Process video frame content

		if (LVP_Player::isHardwarePixelFormat(LVP_Player::videoContext->frameHardware->format))
		{
			result = LibFFmpeg::av_hwframe_transfer_data(
				LVP_Player::videoContext->frameSoftware,
				LVP_Player::videoContext->frameHardware,
				0
			);

			LVP_Player::videoContext->frameSoftware->best_effort_timestamp = LVP_Player::videoContext->frameHardware->best_effort_timestamp;
			LVP_Player::videoContext->frameSoftware->pts                   = LVP_Player::videoContext->frameHardware->pts;
			LVP_Player::videoContext->frameSoftware->pkt_dts               = LVP_Player::videoContext->frameHardware->pkt_dts;

			LVP_Player::videoContext->frame = LVP_Player::videoContext->frameSoftware;
		} else {
			LVP_Player::videoContext->frame = LVP_Player::videoContext->frameHardware;
		}

		if ((result < 0) || !AVFRAME_IS_VALID(LVP_Player::videoContext->frame))
		{
			if (result != AVERROR(EAGAIN))
			{
				#if defined _DEBUG
					char strerror[AV_ERROR_MAX_STRING_SIZE];
					LibFFmpeg::av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);
					LOG("%s\n", strerror);
				#endif

				errorCount++;
			}
					
			if (errorCount >= MAX_ERRORS) {
				LVP_Player::stop("Too many errors occured while decoding the video.");
				break;
			}

			if (LVP_Player::state.progress > 1.0) {
				LibFFmpeg::av_frame_unref(LVP_Player::videoContext->frame);
				LVP_Player::videoContext->frame = LibFFmpeg::av_frame_alloc();
			}

			continue;
		}

		errorCount = 0;

		// Always display music cover image
		if (IS_AUDIO(LVP_Player::state.mediaType) && (LVP_Player::videoContext->stream->attached_pic.size > 0))
		{
			LVP_Player::videoContext->pts = LVP_Player::state.progress;
		}
		// Calculate video PTS (present time)
		else
		{
			LVP_Player::videoContext->pts = LVP_Media::GetVideoPTS(
				LVP_Player::videoContext->frame,
				LVP_Player::videoContext->stream->time_base,
				LVP_Player::audioContext->stream->start_time
			);
		}

		// Wait while paused (unless a seek is requested)

		while (LVP_Player::state.isPaused && !LVP_Player::seekRequested && !LVP_Player::seekRequestedPaused && !LVP_Player::state.quit)
			SDL_Delay(DELAY_TIME_DEFAULT);

		if (LVP_Player::state.quit)
			break;

		// Update video if seek was requested while paused

		if (LVP_Player::seekRequestedPaused)
		{
			// Keep seeking until the video and audio catch up

			if (std::abs(LVP_Player::state.progress - LVP_Player::videoContext->pts) > maxFrameDurationS)
				continue;

			LVP_Player::seekRequestedPaused = false;

			LVP_Player::videoContext->isReadyForRender  = true;
			LVP_Player::videoContext->isReadyForPresent = true;
		}

		auto sleepTime = (int)((LVP_Player::videoContext->pts - LVP_Player::state.progress) * ONE_SECOND_MS);

		LVP_Player::videoContext->isReadyForRender = true;

		// Video is ahead of audio, wait or slow down.

		if (sleepTime > 0)
			SDL_Delay(std::min(sleepTime, maxFrameDurationMS));

		LVP_Player::videoContext->isReadyForPresent = true;
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
		LVP_Player::pause();
	else
		LVP_Player::play();
}
