#include "LVP_Player.h"

MediaPlayer::LVP_AudioContext    MediaPlayer::LVP_Player::audioContext;
LVP_CallbackContext              MediaPlayer::LVP_Player::callbackContext;
LibFFmpeg::AVFormatContext*      MediaPlayer::LVP_Player::formatContext;
LibFFmpeg::AVFormatContext*      MediaPlayer::LVP_Player::formatContextExternal;
bool                             MediaPlayer::LVP_Player::isStopping;
std::mutex                       MediaPlayer::LVP_Player::packetsLock;
SDL_Thread*                      MediaPlayer::LVP_Player::packetsThread;
MediaPlayer::LVP_RenderContext   MediaPlayer::LVP_Player::renderContext;
bool                             MediaPlayer::LVP_Player::seekRequested;
bool                             MediaPlayer::LVP_Player::seekRequestedPaused;
int64_t                          MediaPlayer::LVP_Player::seekPosition;
MediaPlayer::LVP_PlayerState     MediaPlayer::LVP_Player::state;
MediaPlayer::LVP_SubtitleContext MediaPlayer::LVP_Player::subContext;
System::LVP_TimeOut*             MediaPlayer::LVP_Player::timeOut;
MediaPlayer::LVP_VideoContext    MediaPlayer::LVP_Player::videoContext;

void MediaPlayer::LVP_Player::Init(const LVP_CallbackContext &callbackContext)
{
	LVP_Player::reset();

	LVP_Player::audioContext.isMuted = false;
	LVP_Player::audioContext.volume  = SDL_MIX_MAXVOLUME;
	LVP_Player::callbackContext      = callbackContext;
	LVP_Player::state.quit           = false;
}

void MediaPlayer::LVP_Player::CallbackError(const std::string &errorMessage)
{
	if (LVP_Player::callbackContext.errorCB != nullptr) {
		std::thread([errorMessage]() {
			LVP_Player::callbackContext.errorCB(errorMessage, LVP_Player::callbackContext.data);
		}).detach();
	}
}

void MediaPlayer::LVP_Player::callbackEvents(LVP_EventType type)
{
	if (LVP_Player::callbackContext.eventsCB != nullptr) {
		std::thread([type]() {
			LVP_Player::callbackContext.eventsCB(type, LVP_Player::callbackContext.data);
		}).detach();
	}
}

void MediaPlayer::LVP_Player::callbackVideoIsAvailable(SDL_Surface* surface)
{
	if (LVP_Player::callbackContext.videoCB != nullptr) {
		std::thread([surface]() {
			LVP_Player::callbackContext.videoCB(surface, LVP_Player::callbackContext.data);
		}).detach();
	}
}

void MediaPlayer::LVP_Player::clearSubTexture(SDL_Texture* texture)
{
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	SDL_SetRenderTarget(LVP_Player::renderContext.renderer, texture);

	SDL_SetRenderDrawBlendMode(LVP_Player::renderContext.renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(LVP_Player::renderContext.renderer, 0, 0, 0, 0);
	SDL_RenderClear(LVP_Player::renderContext.renderer);
}

void MediaPlayer::LVP_Player::clearSubTextures(bool current, bool next)
{
	auto renderTarget = SDL_GetRenderTarget(LVP_Player::renderContext.renderer);

	if (current && IS_VALID_TEXTURE(LVP_Player::subContext.textureCurrent))
		LVP_Player::clearSubTexture(LVP_Player::subContext.textureCurrent->data);

	if (next && IS_VALID_TEXTURE(LVP_Player::subContext.textureNext))
		LVP_Player::clearSubTexture(LVP_Player::subContext.textureNext->data);

	SDL_SetRenderTarget(LVP_Player::renderContext.renderer, renderTarget);
}

void MediaPlayer::LVP_Player::clearSubs()
{
	for (auto sub : LVP_Player::subContext.subs) {
		LVP_SubTextRenderer::RemoveSubs(sub->id);
		DELETE_POINTER(sub);
	}

	LVP_Player::subContext.subs.clear();
}

void MediaPlayer::LVP_Player::Close()
{
	if (LVP_Player::isStopping)
		return;

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_STOPPING);

	LVP_Player::state.quit = true;
	LVP_Player::isStopping = true;

	LVP_Player::pause();

	LVP_Player::state.isStopped = false;
	
	SDL_Delay(DELAY_TIME_BACKGROUND);

	LVP_Player::closeThreads();
	LVP_Player::closePackets();
	LVP_Player::closeAudio();
	LVP_Player::closeSub();
	LVP_Player::closeVideo();
	LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_AUDIO);
	LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_SUBTITLE);
	LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_VIDEO);

	FREE_AVFORMAT(LVP_Player::formatContextExternal);
	DELETE_POINTER(LVP_Player::timeOut);

	LVP_Player::reset();

	LVP_Player::isStopping      = false;
	LVP_Player::state.isStopped = true;

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_STOPPED);
}

void MediaPlayer::LVP_Player::closeAudio()
{
	if ((LVP_Player::audioContext.index < 0) || (LVP_Player::audioContext.stream == NULL))
		return;

	FREE_AVFILTER_GRAPH(LVP_Player::audioContext.filter.filterGraph);
	FREE_AVFRAME(LVP_Player::audioContext.frame);
	FREE_POINTER(LVP_Player::audioContext.frameEncoded);
	FREE_SWR(LVP_Player::audioContext.specs.swrContext);
	FREE_THREAD_COND(LVP_Player::audioContext.condition);
	FREE_THREAD_MUTEX(LVP_Player::audioContext.mutex);
	
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
	LVP_Player::packetsClear(LVP_Player::audioContext);
	LVP_Player::packetsClear(LVP_Player::subContext);
	LVP_Player::packetsClear(LVP_Player::videoContext);
}

void MediaPlayer::LVP_Player::closeStream(LibFFmpeg::AVMediaType streamType)
{
	switch (streamType) {
		case LibFFmpeg::AVMEDIA_TYPE_AUDIO:
			FREE_AVSTREAM(LVP_Player::audioContext.stream);
			FREE_AVCODEC(LVP_Player::audioContext.codec);

			LVP_Player::audioContext.index = -1;
			break;
		case LibFFmpeg::AVMEDIA_TYPE_SUBTITLE:
			FREE_AVSTREAM(LVP_Player::subContext.stream);
			FREE_AVCODEC(LVP_Player::subContext.codec);

			LVP_Player::subContext.index = -1;
			break;
		case LibFFmpeg::AVMEDIA_TYPE_VIDEO:
			FREE_AVSTREAM(LVP_Player::videoContext.stream);
			FREE_AVCODEC(LVP_Player::videoContext.codec);

			LVP_Player::videoContext.index = -1;
			break;
		default:
			break;
	}
}

void MediaPlayer::LVP_Player::closeSub()
{
	if ((LVP_Player::subContext.index < 0) || (LVP_Player::subContext.stream == NULL))
		return;

	SDL_LockMutex(LVP_Player::subContext.subsMutex);
	LVP_Player::subContext.available = false;

	LVP_Player::clearSubs();

	LVP_Player::subContext.available = true;
	SDL_CondSignal(LVP_Player::subContext.subsCondition);
	SDL_UnlockMutex(LVP_Player::subContext.subsMutex);

	for (auto &font : LVP_Player::subContext.styleFonts)
		CLOSE_FONT(font.second);

	LVP_Player::subContext.fontFaces.clear();
	LVP_Player::subContext.styleFonts.clear();
	LVP_Player::subContext.styles.clear();

	DELETE_POINTER(LVP_Player::subContext.textureCurrent);
	DELETE_POINTER(LVP_Player::subContext.textureNext);
	FREE_SWS(LVP_Player::renderContext.scaleContextSub);
	FREE_THREAD_COND(LVP_Player::subContext.condition);
	FREE_THREAD_MUTEX(LVP_Player::subContext.mutex);
}

void MediaPlayer::LVP_Player::closeThreads()
{
	SDL_CloseAudioDevice(LVP_Player::audioContext.deviceID);

	FREE_THREAD(LVP_Player::packetsThread);
	FREE_THREAD(LVP_Player::subContext.thread);
	FREE_THREAD(LVP_Player::videoContext.thread);
}

void MediaPlayer::LVP_Player::closeVideo()
{
	if ((LVP_Player::videoContext.index < 0) || (LVP_Player::videoContext.stream == NULL))
		return;

	FREE_AVFRAME(LVP_Player::videoContext.frameSoftware);
	FREE_AVFRAME(LVP_Player::videoContext.frameHardware);
	FREE_AVFRAME(LVP_Player::videoContext.frameEncoded);
	DELETE_POINTER(LVP_Player::videoContext.texture);
	FREE_SWS(LVP_Player::renderContext.scaleContextVideo);
	FREE_THREAD_COND(LVP_Player::videoContext.condition);
	FREE_THREAD_MUTEX(LVP_Player::videoContext.mutex);
	FREE_TEXTURE(LVP_Player::renderContext.texture);

	if (LVP_Player::callbackContext.hardwareRenderer == NULL)
		FREE_RENDERER(LVP_Player::renderContext.renderer);

	FREE_SURFACE(LVP_Player::renderContext.surface);
}

Strings MediaPlayer::LVP_Player::GetAudioDevices()
{
	Strings devices;

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
	return LVP_Player::audioContext.index;
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::GetAudioTracks()
{
	return LVP_Player::GetAudioTracks(LVP_Player::formatContext);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::GetAudioTracks(LibFFmpeg::AVFormatContext* formatContext)
{
	return LVP_Player::getMediaTracks(LibFFmpeg::AVMEDIA_TYPE_AUDIO, formatContext);
}

std::vector<LVP_MediaChapter> MediaPlayer::LVP_Player::GetChapters()
{
	return LVP_Player::GetChapters(LVP_Player::formatContext);
}

std::vector<LVP_MediaChapter> MediaPlayer::LVP_Player::GetChapters(LibFFmpeg::AVFormatContext* formatContext)
{
	std::vector<LVP_MediaChapter> chapters;

	if (formatContext == NULL)
		return chapters;

	int64_t lastChapterEnd = 0;

	for (unsigned int i = 0; i < formatContext->nb_chapters; i++)
	{
		auto chapter = formatContext->chapters[i];

		if (chapter->start < lastChapterEnd)
			continue;

		lastChapterEnd = chapter->end;

		auto timeBase = LibFFmpeg::av_q2d(chapter->time_base);
		auto end      = (int64_t)((double)chapter->end   * timeBase * 1000.0);
		auto start    = (int64_t)((double)chapter->start * timeBase * 1000.0);
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
	if (LVP_Player::formatContext == NULL)
		return "";

	return std::string(LVP_Player::formatContext->url);
}

MediaPlayer::LVP_FontFaceMap MediaPlayer::LVP_Player::getFontFaces(LibFFmpeg::AVFormatContext* formatContext)
{
	LVP_FontFaceMap fontFaces = {};

	if (formatContext == NULL)
		return fontFaces;

	for (uint32_t i = 0; i < formatContext->nb_streams; i++)
	{
		auto stream = formatContext->streams[i];

		if (!LVP_Media::IsStreamWithFontAttachments(stream))
			continue;

		TTF_Font* font   = NULL;
		auto      fontRW = SDL_RWFromConstMem(stream->codecpar->extradata, stream->codecpar->extradata_size);

		if (fontRW != NULL)
			font = TTF_OpenFontRW(fontRW, 0, DEFAULT_FONT_SIZE);

		if (font == NULL)
			continue;

		auto familyName     = TTF_FontFaceFamilyName(font);
		auto fontStyle      = TTF_FontFaceStyleName(font);
		auto fontStyleLower = System::LVP_Text::ToLower(fontStyle);

		#if defined _windows
			auto font16          = (wchar_t*)System::LVP_Text::ToUTF16(familyName);
			auto familyNameLower = std::wstring(font16);

			SDL_free(font16);
		#else
			auto familyNameLower = System::LVP_Text::ToLower(familyName);
		#endif

		LVP_FontFace fontFace = {
			.streamIndex = i,
			.style       = fontStyleLower
		};

		if (!fontFaces.contains(familyNameLower))
			fontFaces[familyNameLower] = { fontFace };
		else
			fontFaces[familyNameLower].push_back(fontFace);

		CLOSE_FONT(font);
	}

	return fontFaces;
}

LVP_MediaDetails MediaPlayer::LVP_Player::GetMediaDetails()
{
	if (LVP_Player::state.isStopped)
		return {};

	return
	{
		.duration       = LVP_Media::GetMediaDuration(LVP_Player::formatContext, LVP_Player::audioContext.stream),
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

LVP_MediaDetails MediaPlayer::LVP_Player::GetMediaDetails(const std::string &filePath)
{
	auto formatContext  = LVP_Media::GetMediaFormatContext(filePath, false);
	auto audioStream    = LVP_Media::GetMediaTrackBest(formatContext, LibFFmpeg::AVMEDIA_TYPE_AUDIO);
	auto mediaType      = LVP_Media::GetMediaType(formatContext);
	auto extSubFiles    = (IS_VIDEO(mediaType) ? System::LVP_FileSystem::GetSubtitleFilesForVideo(filePath) : Strings());

	LVP_MediaDetails details =
	{
		.duration       = LVP_Media::GetMediaDuration(formatContext, audioStream),
		.fileSize       = System::LVP_FileSystem::GetFileSize(formatContext->url),
		.mediaType      = (LVP_MediaType)mediaType,
		.meta           = LVP_Media::GetMediaMeta(formatContext),
		.thumbnail      = LVP_Media::GetMediaThumbnail(formatContext),
		.chapters       = LVP_Player::GetChapters(formatContext),
		.audioTracks    = LVP_Player::GetAudioTracks(formatContext),
		.subtitleTracks = LVP_Player::GetSubtitleTracks(formatContext, extSubFiles),
		.videoTracks    = LVP_Player::GetVideoTracks(formatContext)
	};

	FREE_AVFORMAT(formatContext);

	return details;
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::getMediaTracks(LibFFmpeg::AVMediaType mediaType, LibFFmpeg::AVFormatContext* formatContext, const Strings& extSubFiles)
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
	std::vector<LVP_MediaTrack> tracks;

	if (formatContext == NULL)
		return tracks;

	bool isSubsExternal = (extSubFileIndex >= 0);

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

LVP_MediaType MediaPlayer::LVP_Player::GetMediaType(const std::string &filePath)
{
	auto formatContext = LVP_Media::GetMediaFormatContext(filePath, false);
	auto mediaType     = (LVP_MediaType)LVP_Media::GetMediaType(formatContext);

	FREE_AVFORMAT(formatContext);

	return mediaType;
}

LibFFmpeg::AVPixelFormat MediaPlayer::LVP_Player::GetPixelFormatHardware()
{
	return LVP_Player::videoContext.pixelFormatHardware;
}

double MediaPlayer::LVP_Player::GetPlaybackSpeed()
{
	return LVP_Player::state.playbackSpeed;
}

int64_t MediaPlayer::LVP_Player::GetProgress()
{
	return (int64_t)(LVP_Player::state.progress * 1000.0);
}

SDL_Rect* MediaPlayer::LVP_Player::getScaledVideoDestination(const SDL_Rect* destination)
{
	if (destination == NULL)
		return NULL;

	int videoWidth   = LVP_Player::videoContext.stream->codecpar->width;
	int videoHeight  = LVP_Player::videoContext.stream->codecpar->height;
	int maxWidth     = (int)((double)videoWidth  / (double)videoHeight * (double)destination->h);
	int maxHeight    = (int)((double)videoHeight / (double)videoWidth  * (double)destination->w);
	int scaledWidth  = std::min(destination->w, maxWidth);
	int scaledHeight = std::min(destination->h, maxHeight);

	auto scaledDestination = new SDL_Rect
	{
		(destination->x + ((destination->w - scaledWidth)  / 2)),
		(destination->y + ((destination->h - scaledHeight) / 2)),
		scaledWidth,
		scaledHeight
	};

	return scaledDestination;
}

SDL_YUV_CONVERSION_MODE MediaPlayer::LVP_Player::getSdlYuvConversionMode(LibFFmpeg::AVFrame* frame)
{
	if (frame == NULL)
		return SDL_YUV_CONVERSION_AUTOMATIC;

	switch (frame->format) {
	case LibFFmpeg::AV_PIX_FMT_YUV420P:
	case LibFFmpeg::AV_PIX_FMT_YUYV422:
	case LibFFmpeg::AV_PIX_FMT_UYVY422:
		if (frame->color_range == LibFFmpeg::AVCOL_RANGE_JPEG)
			return SDL_YUV_CONVERSION_JPEG;

		switch (frame->colorspace) {
		case LibFFmpeg::AVCOL_SPC_BT709:
			return SDL_YUV_CONVERSION_BT709;
		case LibFFmpeg::AVCOL_SPC_BT470BG:
		case LibFFmpeg::AVCOL_SPC_SMPTE170M:
		case LibFFmpeg::AVCOL_SPC_SMPTE240M:
			return SDL_YUV_CONVERSION_BT601;
		default:
			break;
		}

		break;
	default:
		break;
	}

	return SDL_YUV_CONVERSION_AUTOMATIC;
}

int MediaPlayer::LVP_Player::GetSubtitleTrack()
{
	return LVP_Player::subContext.index;
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::GetSubtitleTracks()
{
	return LVP_Player::GetSubtitleTracks(LVP_Player::formatContext, LVP_Player::subContext.external);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::GetSubtitleTracks(LibFFmpeg::AVFormatContext* formatContext, const Strings& extSubFiles)
{
	return LVP_Player::getMediaTracks(LibFFmpeg::AVMEDIA_TYPE_SUBTITLE, formatContext, extSubFiles);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::GetVideoTracks()
{
	return LVP_Player::GetVideoTracks(LVP_Player::formatContext);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::GetVideoTracks(LibFFmpeg::AVFormatContext* formatContext)
{
	return LVP_Player::getMediaTracks(LibFFmpeg::AVMEDIA_TYPE_VIDEO, formatContext);
}

double MediaPlayer::LVP_Player::GetVolume()
{
	return (double)((double)LVP_Player::audioContext.volume / (double)SDL_MIX_MAXVOLUME);
}

void MediaPlayer::LVP_Player::handleSeek()
{
	LVP_Player::packetsLock.lock();

	LVP_Player::clearSubTextures();
	LVP_Player::clearSubs();

	LVP_Player::audioContext.bufferOffset    = 0;
	LVP_Player::audioContext.bufferRemaining = 0;
	LVP_Player::audioContext.writtenToStream = 0;
	LVP_Player::audioContext.decodeFrame     = true;
	LVP_Player::audioContext.lastPogress     = 0.0;
	LVP_Player::subContext.isReadyForRender  = false;
	LVP_Player::subContext.isReadyForPresent = false;
	LVP_Player::subContext.currentPTS        = {};
	LVP_Player::subContext.nextPTS           = {};
	LVP_Player::subContext.pts               = {};
	LVP_Player::videoContext.pts             = 0.0;

	if (LVP_Player::subContext.codec != NULL)
		LibFFmpeg::avcodec_flush_buffers(LVP_Player::subContext.codec);

	int  seekFlags    = 0;
	auto seekPosition = LVP_Player::seekPosition;

	if (IS_BYTE_SEEK(LVP_Player::formatContext->iformat) && (LVP_Player::state.fileSize > 0))
	{
		auto percent = (double)((double)LVP_Player::seekPosition / ((double)LVP_Player::state.duration * AV_TIME_BASE_D));

		seekFlags    = AVSEEK_FLAG_BYTE;
		seekPosition = (int64_t)((double)LVP_Player::state.fileSize * percent);
	}

	if (LibFFmpeg::avformat_seek_file(LVP_Player::formatContext, -1, INT64_MIN, seekPosition, INT64_MAX, seekFlags) >= 0)
	{
		if (LVP_Player::subContext.index >= SUB_STREAM_EXTERNAL)
			LibFFmpeg::avformat_seek_file(LVP_Player::formatContextExternal, -1, INT64_MIN, LVP_Player::seekPosition, INT64_MAX, 0);

		LVP_Player::state.progress = (double)((double)LVP_Player::seekPosition / AV_TIME_BASE_D);
	}

	SDL_Delay(DELAY_TIME_DEFAULT);

	LVP_Player::seekPosition  = -1;
	LVP_Player::seekRequested = false;

	LVP_Player::packetsLock.unlock();
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

	auto channelLayout = LVP_Media::GetAudioChannelLayout(LVP_Player::audioContext.specs.channelLayout);
	auto sampleFormat  = LibFFmpeg::av_get_sample_fmt_name((LibFFmpeg::AVSampleFormat)LVP_Player::audioContext.specs.format);
	auto sampleRate    = LVP_Player::audioContext.specs.sampleRate;
	auto timeBase      = LVP_Player::audioContext.stream->time_base;

	LibFFmpeg::av_opt_set(bufferSource,     "channel_layout", channelLayout.c_str(), AV_OPT_SEARCH_CHILDREN);
	LibFFmpeg::av_opt_set(bufferSource,     "sample_fmt",     sampleFormat,          AV_OPT_SEARCH_CHILDREN);
	LibFFmpeg::av_opt_set_int(bufferSource, "sample_rate",    sampleRate,            AV_OPT_SEARCH_CHILDREN);
	LibFFmpeg::av_opt_set_q(bufferSource,   "time_base",      timeBase,              AV_OPT_SEARCH_CHILDREN);

	LibFFmpeg::av_opt_set_double(filterATempo, "tempo", LVP_Player::audioContext.specs.playbackSpeed, AV_OPT_SEARCH_CHILDREN);

	LibFFmpeg::avfilter_init_str(bufferSource, NULL);
	LibFFmpeg::avfilter_init_str(bufferSink,   NULL);
	LibFFmpeg::avfilter_init_str(filterATempo, NULL);

	LibFFmpeg::avfilter_link(bufferSource, 0, filterATempo, 0);
	LibFFmpeg::avfilter_link(filterATempo, 0, bufferSink, 0);

	if (LibFFmpeg::avfilter_graph_config(filterGraph, NULL) < 0)
		throw std::runtime_error("Failed to initialize a valid audio filter.");

	FREE_AVFILTER_GRAPH(LVP_Player::audioContext.filter.filterGraph);

	LVP_Player::audioContext.filter.filterGraph  = filterGraph;
	LVP_Player::audioContext.filter.bufferSource = bufferSource;
	LVP_Player::audioContext.filter.bufferSink   = bufferSink;
}

void MediaPlayer::LVP_Player::initSubTextures()
{
	if (IS_VALID_TEXTURE(LVP_Player::subContext.textureCurrent) && IS_VALID_TEXTURE(LVP_Player::subContext.textureNext))
		return;

	DELETE_POINTER(LVP_Player::subContext.textureCurrent);
	DELETE_POINTER(LVP_Player::subContext.textureNext);

	auto videoWidth  = LVP_Player::videoContext.stream->codecpar->width;
	auto videoHeight = LVP_Player::videoContext.stream->codecpar->height;

	LVP_Player::subContext.textureCurrent = new Graphics::LVP_Texture(
		SUB_PIXEL_FORMAT_SDL,
		SDL_TEXTUREACCESS_TARGET,
		videoWidth,
		videoHeight,
		LVP_Player::renderContext.renderer
	);

	LVP_Player::subContext.textureNext = new Graphics::LVP_Texture(
		SUB_PIXEL_FORMAT_SDL,
		SDL_TEXTUREACCESS_TARGET,
		videoWidth,
		videoHeight,
		LVP_Player::renderContext.renderer
	);

	LVP_Player::clearSubTextures();
}

bool MediaPlayer::LVP_Player::IsMuted()
{
	return LVP_Player::audioContext.isMuted;
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
		return ((LVP_Player::audioContext.stream != NULL) && (LVP_Player::audioContext.packets.size() >= MIN_PACKET_QUEUE_SIZE));
	case LibFFmpeg::AVMEDIA_TYPE_SUBTITLE:
		return ((LVP_Player::subContext.stream != NULL) && (LVP_Player::subContext.packets.size() >= MIN_PACKET_QUEUE_SIZE));
	case LibFFmpeg::AVMEDIA_TYPE_VIDEO:
		return ((LVP_Player::videoContext.stream != NULL) && (LVP_Player::videoContext.packets.size() >= MIN_PACKET_QUEUE_SIZE));
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
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::Open(const std::string &filePath)
{
	if (filePath.empty())
		throw std::invalid_argument("filePath cannot be empty");

	if (!LVP_Player::state.isStopped)
		LVP_Player::Close();

	LVP_Player::state.quit = false;

	LVP_Player::openFormatContext(filePath);
	LVP_Player::openStreams();
	LVP_Player::openThreads();

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_OPENED);

	LVP_Player::play();
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openAudioDevice(const SDL_AudioSpec &wantedSpecs)
{
	bool isDefault = (LVP_Player::audioContext.device == "Default");
	bool isEmpty   = LVP_Player::audioContext.device.empty();
	auto newDevice = (!isEmpty && !isDefault ? LVP_Player::audioContext.device.c_str() : NULL);

	if (LVP_Player::audioContext.deviceID >= 2)
		SDL_CloseAudioDevice(LVP_Player::audioContext.deviceID);

	if (SDL_GetNumAudioDevices(0) < 1)
	{
		SDL_AudioQuit();
		SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
		SDL_AudioInit("dummy");
	}

	LVP_Player::audioContext.deviceID = SDL_OpenAudioDevice(
		newDevice,
		0,
		&wantedSpecs,
		&LVP_Player::audioContext.deviceSpecs,
		SDL_AUDIO_ALLOW_ANY_CHANGE
	);

	if (LVP_Player::audioContext.deviceID < 2)
	{
		#if defined _DEBUG
			LOG("LVP_Player::openAudioDevice: %s\n", SDL_GetError());
		#endif

		SDL_CloseAudioDevice(LVP_Player::audioContext.deviceID);

		throw std::runtime_error(System::LVP_Text::Format("Failed to open a valid audio device: %u", LVP_Player::audioContext.deviceID).c_str());
	}
}

/**
 * @throws invalid_argument
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openFormatContext(const std::string &filePath)
{
	if (filePath.empty())
		throw std::invalid_argument("filePath cannot be empty");

	LVP_Player::timeOut = new System::LVP_TimeOut();

	if (LVP_Player::formatContext == NULL)
		LVP_Player::formatContext = LVP_Media::GetMediaFormatContext(filePath, true, LVP_Player::timeOut);

	auto mediaType    = LVP_Media::GetMediaType(LVP_Player::formatContext);
	bool isValidMedia = ((mediaType == LibFFmpeg::AVMEDIA_TYPE_AUDIO) || (mediaType == LibFFmpeg::AVMEDIA_TYPE_VIDEO));

	if (!isValidMedia) {
		FREE_AVFORMAT(LVP_Player::formatContext);
		throw std::runtime_error(System::LVP_Text::Format("Invalid media type: %d", (int)mediaType).c_str());
	}

	auto fileExtension = System::LVP_FileSystem::GetFileExtension(filePath);
	auto fileSize      = System::LVP_FileSystem::GetFileSize(filePath);

	if ((fileExtension == "m2ts") && System::LVP_FileSystem::IsBlurayAACS(filePath, fileSize))
		isValidMedia = false;
	else if ((fileExtension == "vob") && System::LVP_FileSystem::IsDVDCSS(filePath, fileSize))
		isValidMedia = false;

	if (!isValidMedia) {
		FREE_AVFORMAT(LVP_Player::formatContext);
		throw std::runtime_error("Media is DRM encrypted.");
	}

	LVP_Player::state.filePath  = filePath;
	LVP_Player::state.fileSize  = fileSize;
	LVP_Player::state.mediaType = mediaType;

	LVP_Player::subContext.formatContext = LVP_Player::formatContext;
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openStreams()
{
	LVP_Player::audioContext.index = -1;
	LVP_Player::subContext.index   = -1;
	LVP_Player::videoContext.index = -1;

	// AUDIO TRACK
	LVP_Media::SetMediaTrackBest(LVP_Player::formatContext, LibFFmpeg::AVMEDIA_TYPE_AUDIO, LVP_Player::audioContext);

	if (LVP_Player::audioContext.stream == NULL)
		throw std::runtime_error("Failed to find a valid audio track.");

	LVP_Player::state.duration = LVP_Media::GetMediaDuration(LVP_Player::formatContext, LVP_Player::audioContext.stream);

	// VIDEO TRACK
	LVP_Media::SetMediaTrackBest(LVP_Player::formatContext, LibFFmpeg::AVMEDIA_TYPE_VIDEO, LVP_Player::videoContext);

	if (IS_VIDEO(LVP_Player::state.mediaType))
	{
		if (LVP_Player::videoContext.stream == NULL)
			throw std::runtime_error("Failed to find a valid video track.");

		// SUB TRACK
		LVP_Media::SetMediaTrackBest(LVP_Player::formatContext, LibFFmpeg::AVMEDIA_TYPE_SUBTITLE, LVP_Player::subContext);

		LVP_Player::subContext.external = System::LVP_FileSystem::GetSubtitleFilesForVideo(LVP_Player::state.filePath);

		if ((LVP_Player::subContext.stream == NULL) && !LVP_Player::subContext.external.empty())
			LVP_Player::openSubExternal();
	}

	LVP_Player::state.trackCount = LVP_Player::formatContext->nb_streams;
}

void MediaPlayer::LVP_Player::openSubExternal(int streamIndex)
{
	if (LVP_Player::subContext.external.empty() || (streamIndex < SUB_STREAM_EXTERNAL))
		return;

	int fileIndex  = ((streamIndex - SUB_STREAM_EXTERNAL) / SUB_STREAM_EXTERNAL);
	int trackIndex = ((streamIndex - SUB_STREAM_EXTERNAL) % SUB_STREAM_EXTERNAL);

	std::string subFile = LVP_Player::subContext.external[fileIndex];

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

	if (LVP_Player::videoContext.stream != NULL) {
		LVP_Player::openThreadVideo();
		LVP_Player::openThreadSub();
	}

	LVP_Player::packetsThread = SDL_CreateThread(LVP_Player::threadPackets, "packetsThread", NULL);

	if (LVP_Player::packetsThread == NULL)
		throw std::runtime_error(System::LVP_Text::Format("Failed to create a packets thread: %s", SDL_GetError()).c_str());
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openThreadAudio()
{
	if ((LVP_Player::audioContext.stream == NULL) || (LVP_Player::audioContext.stream->codecpar == NULL))
		throw std::runtime_error("Audio context stream is missing a valid codec.");

	LVP_Player::audioContext.frame     = LibFFmpeg::av_frame_alloc();
	LVP_Player::audioContext.condition = SDL_CreateCond();
	LVP_Player::audioContext.mutex     = SDL_CreateMutex();
	LVP_Player::audioContext.index     = LVP_Player::audioContext.stream->index;

	if ((LVP_Player::audioContext.frame == NULL) || (LVP_Player::audioContext.condition == NULL) || (LVP_Player::audioContext.mutex == NULL))
		throw std::runtime_error("Failed to allocate an audio context frame.");

	auto channelCount = LVP_Player::audioContext.stream->codecpar->ch_layout.nb_channels;
	auto sampleRate   = LVP_Player::audioContext.stream->codecpar->sample_rate;

	if (channelCount < 1) {
		LibFFmpeg::av_channel_layout_default(&LVP_Player::audioContext.stream->codecpar->ch_layout, 2);
		channelCount = LVP_Player::audioContext.stream->codecpar->ch_layout.nb_channels;
	}

	if ((sampleRate < 1) || (channelCount < 1))
		throw std::runtime_error(System::LVP_Text::Format("Invalid audio: %d channels, %d bps", channelCount, sampleRate).c_str());

	// https://developer.apple.com/documentation/audiotoolbox/kaudiounitproperty_maximumframesperslice

	LVP_Player::audioContext.deviceSpecsWanted.callback = LVP_Player::threadAudio;
	LVP_Player::audioContext.deviceSpecsWanted.channels = channelCount;
	LVP_Player::audioContext.deviceSpecsWanted.format   = LVP_Player::getAudioSampleFormat(LVP_Player::audioContext.codec->sample_fmt);
	LVP_Player::audioContext.deviceSpecsWanted.freq     = sampleRate;
	LVP_Player::audioContext.deviceSpecsWanted.samples  = 4096;

	LVP_Player::openAudioDevice(LVP_Player::audioContext.deviceSpecsWanted);

	LVP_Player::audioContext.bufferOffset    = 0;
	LVP_Player::audioContext.bufferRemaining = 0;
	LVP_Player::audioContext.bufferSize      = AUDIO_BUFFER_SIZE;

	LVP_Player::audioContext.decodeFrame     = true;
	LVP_Player::audioContext.frameEncoded    = (uint8_t*)std::malloc(LVP_Player::audioContext.bufferSize);
	LVP_Player::audioContext.writtenToStream = 0;

	if (LVP_Player::audioContext.frameEncoded == NULL)
		throw std::runtime_error("Failed to allocate an encoded audio context frame.");
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openThreadSub()
{
	if ((LVP_Player::subContext.stream == NULL) || (LVP_Player::subContext.stream->codecpar == NULL))
		return;

	// ASS/SSA STYLES

	int  scriptWidth  = 384;
	int  scriptHeight = 288;
	auto styleVersion = SUB_STYLE_VERSION_UNKNOWN;
	int  subWidth     = LVP_Player::subContext.stream->codecpar->width;
	int  subHeight    = LVP_Player::subContext.stream->codecpar->height;
	int  videoWidth   = (LVP_Player::videoContext.stream != NULL ? LVP_Player::videoContext.stream->codecpar->width  : 0);
	int  videoHeight  = (LVP_Player::videoContext.stream != NULL ? LVP_Player::videoContext.stream->codecpar->height : 0);

	auto header    = LVP_Player::subContext.codec->subtitle_header;
	auto subHeader = (header != NULL ? std::string(reinterpret_cast<char*>(header)) : "");

	if (!subHeader.empty())
	{
		#if defined _DEBUG
			printf("\n%s\n", subHeader.c_str());
		#endif

		auto posResX = subHeader.find("PlayResX:");
		auto posResY = subHeader.find("PlayResY:");

		if (posResX != std::string::npos)
			scriptWidth = std::atoi(subHeader.substr(posResX + 9).c_str());

		if (posResY != std::string::npos)
			scriptHeight = std::atoi(subHeader.substr(posResY + 9).c_str());

		auto posStyle   = subHeader.find("Format: Name,");
		auto styleProps = 0;

		if (posStyle != std::string::npos) {
			auto format = subHeader.substr(posStyle);
			styleProps  = (int)System::LVP_Text::Split(format.substr(0, format.find("\n")), ",").size();
		}

		switch (styleProps) {
			case NR_OF_V4PLUS_SUB_STYLES: styleVersion = SUB_STYLE_VERSION_4PLUS_ASS; break;
			case NR_OF_V4_SUB_STYLES:     styleVersion = SUB_STYLE_VERSION_4_SSA; break;
			default: break;
		}
	}

	if ((scriptWidth > 0) && (scriptHeight > 0)) {
		LVP_Player::subContext.size = { scriptWidth, scriptHeight };
	} else if ((scriptHeight > 0) && (videoWidth > 0) && (videoHeight > 0)) {
		float scriptVideoScale = ((float)scriptHeight / (float)videoHeight);
		LVP_Player::subContext.size = { (int)((float)videoWidth * scriptVideoScale), scriptHeight };
	} else if ((subWidth > 0) && (subHeight > 0)) {
		LVP_Player::subContext.size = { subWidth, subHeight };
	} else {
		LVP_Player::subContext.size = { videoWidth, videoHeight };
	}

	// SUB SCALING RELATIVE TO VIDEO

	if ((LVP_Player::subContext.size.x > 0) && (LVP_Player::subContext.size.y > 0))
	{
		LVP_Player::subContext.scale = {
			(float)((float)videoWidth  / (float)LVP_Player::subContext.size.x),
			(float)((float)videoHeight / (float)LVP_Player::subContext.size.y)
		};
	}

	LVP_Player::subContext.videoDimensions = { 0, 0, videoWidth, videoHeight };

	// SUB STYLES

	if (styleVersion != SUB_STYLE_VERSION_UNKNOWN)
	{
		LVP_Player::subContext.fontFaces = LVP_Player::getFontFaces(LVP_Player::formatContext);

		auto subHeaderStream = std::stringstream(subHeader);

		std::string subHeaderLine;

		while (std::getline(subHeaderStream, subHeaderLine))
		{
			if (subHeaderLine.substr(0, 7) != "Style: ")
				continue;

			auto subHeaderParts  = System::LVP_Text::Split(subHeaderLine.substr(7), ",");
			bool isValid4SSA     = ((styleVersion == SUB_STYLE_VERSION_4_SSA)     && (subHeaderParts.size() >= NR_OF_V4_SUB_STYLES));
			bool isValid4PlusSSA = ((styleVersion == SUB_STYLE_VERSION_4PLUS_ASS) && (subHeaderParts.size() >= NR_OF_V4PLUS_SUB_STYLES));

			if (!isValid4SSA && !isValid4PlusSSA)
				continue;

			auto subStyle = new LVP_SubStyle(subHeaderParts, styleVersion);

			LVP_Player::subContext.styles.push_back(subStyle);
		}
	}

	// SUB THREAD

	if (LVP_Player::subContext.mutex == NULL) {
		LVP_Player::subContext.mutex     = SDL_CreateMutex();
		LVP_Player::subContext.condition = SDL_CreateCond();
	}

	if ((LVP_Player::subContext.mutex == NULL) || (LVP_Player::subContext.condition == NULL))
		throw std::runtime_error(System::LVP_Text::Format("Failed to create a subtitle context mutex: %s", SDL_GetError()).c_str());

	if (LVP_Player::subContext.subsMutex == NULL) {
		LVP_Player::subContext.subsMutex     = SDL_CreateMutex();
		LVP_Player::subContext.subsCondition = SDL_CreateCond();
	}

	if ((LVP_Player::subContext.subsMutex == NULL) || (LVP_Player::subContext.subsCondition == NULL))
		throw std::runtime_error(System::LVP_Text::Format("Failed to create a subtitles context mutex: %s", SDL_GetError()).c_str());

	if (LVP_Player::subContext.thread == NULL)
		LVP_Player::subContext.thread = SDL_CreateThread(LVP_Player::threadSub, "subContext.thread", NULL);

	if (LVP_Player::subContext.thread == NULL)
		throw std::runtime_error(System::LVP_Text::Format("Failed to create a subtitles context thread: %s", SDL_GetError()).c_str());
}

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::openThreadVideo()
{
	if ((LVP_Player::videoContext.stream == NULL) || (LVP_Player::videoContext.stream->codecpar == NULL))
		throw std::runtime_error("Video context stream is missing a valid codec.");

	// VIDEO RENDERER

	int videoWidth  = LVP_Player::videoContext.stream->codecpar->width;
	int videoHeight = LVP_Player::videoContext.stream->codecpar->height;

	if (LVP_Player::callbackContext.hardwareRenderer != NULL) {
		LVP_Player::renderContext.renderer = LVP_Player::callbackContext.hardwareRenderer;
	} else {
	   LVP_Player::renderContext.surface  = SDL_CreateRGBSurfaceWithFormat(0, videoWidth, videoHeight, 24, SDL_PIXELFORMAT_RGB24);
	   LVP_Player::renderContext.renderer = SDL_CreateSoftwareRenderer(LVP_Player::renderContext.surface);
	}

	if (LVP_Player::renderContext.renderer == NULL)
		throw std::runtime_error(System::LVP_Text::Format("Failed to create a renderer: %s", SDL_GetError()).c_str());

	// VIDEO TARGET TEXTURE

	LVP_Player::renderContext.texture = SDL_CreateTexture(
		LVP_Player::renderContext.renderer,
		SDL_PIXELFORMAT_RGB24,
		SDL_TEXTUREACCESS_TARGET,
		videoWidth,
		videoHeight
	);

	LVP_Player::videoContext.frameHardware = LibFFmpeg::av_frame_alloc();
	LVP_Player::videoContext.frameSoftware = LibFFmpeg::av_frame_alloc();
	LVP_Player::videoContext.frameRate     = (1.0 / LVP_Media::GetMediaFrameRate(LVP_Player::videoContext.stream));

	if ((LVP_Player::videoContext.frameHardware == NULL) ||
		(LVP_Player::videoContext.frameSoftware == NULL) ||
		(LVP_Player::videoContext.frameRate <= 0))
	{
		throw std::runtime_error("Failed to allocate a video context frame.");
	}

	if (LVP_Player::videoContext.frameEncoded != NULL)
		LVP_Player::videoContext.frameEncoded->linesize[0] = 0;

	// VIDEO THREAD

	LVP_Player::videoContext.condition = SDL_CreateCond();
	LVP_Player::videoContext.mutex     = SDL_CreateMutex();
	LVP_Player::videoContext.thread    = SDL_CreateThread(LVP_Player::threadVideo, "videoContext.thread", NULL);

	if ((LVP_Player::videoContext.thread == NULL) || (LVP_Player::videoContext.mutex == NULL) || (LVP_Player::videoContext.condition == NULL))
		throw std::runtime_error(System::LVP_Text::Format("Failed to create a video thread: %s", SDL_GetError()).c_str());

	LVP_Player::videoContext.index = LVP_Player::videoContext.stream->index;
}

void MediaPlayer::LVP_Player::packetAdd(LibFFmpeg::AVPacket* packet, LVP_MediaContext &mediaContext)
{
	if ((packet == NULL) || (mediaContext.mutex == NULL) || (mediaContext.condition == NULL))
		return;

	SDL_LockMutex(mediaContext.mutex);

	mediaContext.packetsAvailable = false;
	mediaContext.packets.push(packet);
	mediaContext.packetsAvailable = true;
	
	SDL_CondSignal(mediaContext.condition);
	SDL_UnlockMutex(mediaContext.mutex);
}

LibFFmpeg::AVPacket* MediaPlayer::LVP_Player::packetGet(LVP_MediaContext &mediaContext)
{
	if ((mediaContext.mutex == NULL) || (mediaContext.condition == NULL) || mediaContext.packets.empty())
		return NULL;

	SDL_LockMutex(mediaContext.mutex);

	if (!mediaContext.packetsAvailable)
		SDL_CondWait(mediaContext.condition, mediaContext.mutex);

	auto packet = mediaContext.packets.front();
	mediaContext.packets.pop();

	SDL_UnlockMutex(mediaContext.mutex);
	
	return packet;
}

void MediaPlayer::LVP_Player::packetsClear(LVP_MediaContext &mediaContext)
{
	if ((mediaContext.mutex == NULL) || (mediaContext.condition == NULL))
		return;

	SDL_LockMutex(mediaContext.mutex);
	
	mediaContext.packetsAvailable = false;

	while (!mediaContext.packets.empty()) {
		FREE_AVPACKET(mediaContext.packets.front());
		mediaContext.packets.pop();
	}
	
	mediaContext.packetsAvailable = true;

	SDL_CondSignal(mediaContext.condition);
	SDL_UnlockMutex(mediaContext.mutex);
}

void MediaPlayer::LVP_Player::pause()
{
	if (LVP_Player::state.isPaused || LVP_Player::state.filePath.empty())
		return;

	LVP_Player::state.isPaused  = true;
	LVP_Player::state.isPlaying = false;
	LVP_Player::state.isStopped = false;

	SDL_PauseAudioDevice(LVP_Player::audioContext.deviceID, 1);

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_PAUSED);
}

void MediaPlayer::LVP_Player::play()
{
	if (LVP_Player::state.isPlaying || LVP_Player::state.filePath.empty())
		return;

	LVP_Player::state.isPlaying = true;
	LVP_Player::state.isPaused  = false;
	LVP_Player::state.isStopped = false;

	SDL_PauseAudioDevice(LVP_Player::audioContext.deviceID, 0);

	LVP_Player::callbackEvents(LVP_EVENT_MEDIA_PLAYING);
}

void MediaPlayer::LVP_Player::present()
{
	if (!IS_VALID_TEXTURE(LVP_Player::videoContext.texture))
		return;

	auto renderTarget = SDL_GetRenderTarget(LVP_Player::renderContext.renderer);

	SDL_SetRenderTarget(LVP_Player::renderContext.renderer, LVP_Player::renderContext.texture);

	SDL_SetYUVConversionMode(LVP_Player::getSdlYuvConversionMode(LVP_Player::videoContext.frame));
	SDL_RenderClear(LVP_Player::renderContext.renderer);
	SDL_RenderCopy(LVP_Player::renderContext.renderer, LVP_Player::videoContext.texture->data, NULL, NULL);
	SDL_SetYUVConversionMode(SDL_YUV_CONVERSION_AUTOMATIC);

	if (LVP_Player::subContext.isReadyForPresent)
	{
		LVP_Player::subContext.isReadyForPresent = false;

		auto current = LVP_Player::subContext.textureCurrent;

		LVP_Player::subContext.textureCurrent = LVP_Player::subContext.textureNext;
		LVP_Player::subContext.textureNext    = current;
	}

	if ((LVP_Player::subContext.textureCurrent != NULL) && (LVP_Player::subContext.index >= 0))
		SDL_RenderCopy(LVP_Player::renderContext.renderer, LVP_Player::subContext.textureCurrent->data, NULL, NULL);

	SDL_SetRenderTarget(LVP_Player::renderContext.renderer, renderTarget);

	if (LVP_Player::callbackContext.hardwareRenderer == NULL)
		LVP_Player::presentSoftwareRenderer();
}

void MediaPlayer::LVP_Player::presentSoftwareRenderer()
{
	SDL_RenderCopy(LVP_Player::renderContext.renderer, LVP_Player::renderContext.texture, NULL, NULL);

	SDL_Rect renderViewport;
	SDL_RenderGetViewport(LVP_Player::renderContext.renderer, &renderViewport);

	auto surface = SDL_CreateRGBSurfaceWithFormat(0, renderViewport.w, renderViewport.h, 24, SDL_PIXELFORMAT_RGB24);

	if (surface == NULL)
		return;

	auto readResult = SDL_RenderReadPixels(LVP_Player::renderContext.renderer, NULL, surface->format->format, surface->pixels, surface->pitch);

	if ((readResult == 0) && !LVP_Player::state.quit)
		LVP_Player::callbackVideoIsAvailable(surface);
}

void MediaPlayer::LVP_Player::removeExpiredSubs()
{
	bool isReadyForRender  = false;
	bool isReadyForPresent = false;

	for (auto subIter = LVP_Player::subContext.subs.begin(); subIter != LVP_Player::subContext.subs.end();)
	{
		auto sub = *subIter;

		bool isExpired = sub->isExpiredPTS(LVP_Player::subContext, LVP_Player::state.progress);
		bool isSeeked  = sub->isSeekedPTS(LVP_Player::subContext);

		if (!isExpired && !isSeeked && (LVP_Player::subContext.index >= 0)) {
			subIter++;
			continue;
		}

		LVP_SubTextRenderer::RemoveSubs(sub->id);
		DELETE_POINTER(sub);

		subIter = LVP_Player::subContext.subs.erase(subIter);

		isReadyForRender = true;

		if (isSeeked)
			isReadyForPresent = true;
	}

	if (isReadyForRender)
		LVP_Player::subContext.isReadyForRender = true;

	if (isReadyForPresent || LVP_Player::subContext.subs.empty())
	{
		LVP_Player::clearSubTextures();

		LVP_Player::subContext.isReadyForPresent = true;
	}
}

void MediaPlayer::LVP_Player::renderSubs()
{
	if ((LVP_Player::subContext.index < 0) || LVP_Player::state.isStopped || LVP_Player::state.quit)
		return;

	SDL_LockMutex(LVP_Player::subContext.subsMutex);

	if (!LVP_Player::subContext.available)
		SDL_CondWait(LVP_Player::subContext.subsCondition, LVP_Player::subContext.subsMutex);

	if (LVP_Player::seekRequested) {
		LVP_Player::clearSubs();
		SDL_UnlockMutex(LVP_Player::subContext.subsMutex);
		return;
	}

	LVP_Player::initSubTextures();

	if (!IS_VALID_TEXTURE(LVP_Player::subContext.textureCurrent) || !IS_VALID_TEXTURE(LVP_Player::subContext.textureNext))
	{
		LVP_Player::subContext.isReadyForRender = true;
		SDL_UnlockMutex(LVP_Player::subContext.subsMutex);
		return;
	}

	switch (LVP_Player::subContext.type) {
	case LibFFmpeg::SUBTITLE_ASS:
	case LibFFmpeg::SUBTITLE_TEXT:
		LVP_Player::renderSubsText();
		break;
	case LibFFmpeg::SUBTITLE_BITMAP:
		LVP_Player::renderSubsBitmap();
		break;
	default:
		break;
	}

	SDL_UnlockMutex(LVP_Player::subContext.subsMutex);
}

void MediaPlayer::LVP_Player::renderSubsBitmap()
{
	auto renderTarget = SDL_GetRenderTarget(LVP_Player::renderContext.renderer);

	LVP_Player::clearSubTexture(LVP_Player::subContext.textureNext->data);

	auto& subs = LVP_Player::subContext.subs;

	for (auto subIter = subs.begin(); subIter != subs.end(); subIter++)
	{
		LVP_Subtitle* sub = *subIter;

		if ((sub == NULL) || (sub->subRect->w <= 0) || (sub->subRect->h <= 0))
			continue;

		bool skipSub = false;

		for (LVP_Subtitles::iterator subIter2 = subIter; subIter2 != subs.end(); subIter2++)
		{
			LVP_Subtitle* sub2     = *subIter2;
			SDL_Rect      sub2Rect = { sub2->subRect->x, sub2->subRect->y, sub2->subRect->w, sub2->subRect->h };
			SDL_Rect      subRect  = { sub->subRect->x,  sub->subRect->y,  sub->subRect->w,  sub->subRect->h };

			if ((subIter2 != subIter) && SDL_HasIntersection(&sub2Rect, &subRect))
			{
				skipSub      = true;
				sub->pts.end = LVP_Player::state.progress;

				break;
			}
		}

		// Skip if there are collisions/overlap
		if (skipSub)
			continue;

		auto& subStream = LVP_Player::subContext.stream;

		LVP_Player::renderContext.scaleContextSub = sws_getCachedContext(
			LVP_Player::renderContext.scaleContextSub,
			sub->subRect->w, sub->subRect->h, LVP_Player::subContext.codec->pix_fmt,
			sub->subRect->w, sub->subRect->h, SUB_PIXEL_FORMAT_FFMPEG, DEFAULT_SCALE_FILTER,
			NULL, NULL, NULL
		);

		auto frameEncoded = LibFFmpeg::av_frame_alloc();

		auto frameAllocResult = av_image_alloc(
			frameEncoded->data,
			frameEncoded->linesize,
			sub->subRect->w,
			sub->subRect->h,
			SUB_PIXEL_FORMAT_FFMPEG,
			32
		);

		if ((LVP_Player::renderContext.scaleContextSub == NULL) || (frameAllocResult <= 0)) {
			FREE_AVFRAME(frameEncoded);
			continue;
		}

		auto scaleResult = sws_scale(
			LVP_Player::renderContext.scaleContextSub,
			sub->subRect->data, sub->subRect->linesize, 0, sub->subRect->h,
			frameEncoded->data, frameEncoded->linesize
		);

		if (scaleResult <= 0) {
			FREE_AVFRAME(frameEncoded);
			continue;
		}

		auto texture = new Graphics::LVP_Texture(
			SUB_PIXEL_FORMAT_SDL,
			SDL_TEXTUREACCESS_STATIC,
			sub->subRect->w,
			sub->subRect->h,
			LVP_Player::renderContext.renderer
		);

		if (!IS_VALID_TEXTURE(texture)) {
			FREE_AVFRAME(frameEncoded);
			continue;
		}

		SDL_UpdateTexture(texture->data, NULL, frameEncoded->data[0], frameEncoded->linesize[0]);

		auto renderLocation = sub->subRect->getSdlRect();

		SDL_RenderCopy(LVP_Player::renderContext.renderer, texture->data, NULL, &renderLocation);

		DELETE_POINTER(texture);
		FREE_AVFRAME(frameEncoded);
	}

	SDL_SetRenderTarget(LVP_Player::renderContext.renderer, renderTarget);
}

void MediaPlayer::LVP_Player::renderSubsText()
{
	auto renderTarget = SDL_GetRenderTarget(LVP_Player::renderContext.renderer);

	LVP_Player::clearSubTexture(LVP_Player::subContext.textureNext->data);

	if (!LVP_Player::subContext.subs.empty())
		LVP_SubTextRenderer::Render(LVP_Player::renderContext.renderer, LVP_Player::subContext);
	else
		LVP_Player::subContext.isReadyForPresent = true;

	SDL_SetRenderTarget(LVP_Player::renderContext.renderer, renderTarget);
}

void MediaPlayer::LVP_Player::renderVideo()
{
	if ((LVP_Player::videoContext.index < 0) || (LVP_Player::videoContext.codec == NULL))
		return;

	auto videoWidth  = LVP_Player::videoContext.stream->codecpar->width;
	auto videoHeight = LVP_Player::videoContext.stream->codecpar->height;

	if (!IS_VALID_TEXTURE(LVP_Player::videoContext.texture))
	{
		LVP_Player::videoContext.texture = new Graphics::LVP_Texture(
			VIDEO_PIXEL_FORMAT_SDL,
			SDL_TEXTUREACCESS_STREAMING,
			videoWidth,
			videoHeight,
			LVP_Player::renderContext.renderer
		);
	}

	if (!IS_VALID_TEXTURE(LVP_Player::videoContext.texture)) {
		DELETE_POINTER(LVP_Player::videoContext.texture);
		return;
	}

	if (LVP_Player::videoContext.frame == NULL)
		return;

	auto pixelFormat = (LibFFmpeg::AVPixelFormat)LVP_Player::videoContext.frame->format;
	bool isYUV420P   = (pixelFormat == VIDEO_PIXEL_FORMAT_FFMPEG);

	// YUV420P VIDEOS - Copy the frame directly to the texture
	if (isYUV420P)
	{
		SDL_UpdateYUVTexture(
			LVP_Player::videoContext.texture->data, NULL,
			LVP_Player::videoContext.frame->data[LVP_YUV_Y], LVP_Player::videoContext.frame->linesize[LVP_YUV_Y],
			LVP_Player::videoContext.frame->data[LVP_YUV_U], LVP_Player::videoContext.frame->linesize[LVP_YUV_U],
			LVP_Player::videoContext.frame->data[LVP_YUV_V], LVP_Player::videoContext.frame->linesize[LVP_YUV_V]
		);

		return;
	}

	// NON-YUV420P VIDEOS - Convert the pixel format to YUV420P

	if (LVP_Player::videoContext.frameEncoded == NULL)
	{
		LVP_Player::videoContext.frameEncoded = LibFFmpeg::av_frame_alloc();

		auto frameEncodedResult = LibFFmpeg::av_image_alloc(
			LVP_Player::videoContext.frameEncoded->data,
			LVP_Player::videoContext.frameEncoded->linesize,
			videoWidth,
			videoHeight,
			VIDEO_PIXEL_FORMAT_FFMPEG,
			24
		);

		if (frameEncodedResult <= 0)
			FREE_AVFRAME(LVP_Player::videoContext.frameEncoded);
	}

	if ((LVP_Player::videoContext.frameEncoded == NULL) || (pixelFormat == LibFFmpeg::AV_PIX_FMT_NONE))
		return;

	LVP_Player::renderContext.scaleContextVideo = LibFFmpeg::sws_getCachedContext(
		LVP_Player::renderContext.scaleContextVideo,
		videoWidth, videoHeight, pixelFormat,
		videoWidth, videoHeight, VIDEO_PIXEL_FORMAT_FFMPEG,
		DEFAULT_SCALE_FILTER, NULL, NULL, NULL
	);

	auto scaleResult = LibFFmpeg::sws_scale(
		LVP_Player::renderContext.scaleContextVideo,
		LVP_Player::videoContext.frame->data,
		LVP_Player::videoContext.frame->linesize,
		0,
		LVP_Player::videoContext.frame->height,
		LVP_Player::videoContext.frameEncoded->data,
		LVP_Player::videoContext.frameEncoded->linesize
	);

	if (scaleResult <= 0)
		return;

	// Copy the converted frame to the texture
	SDL_UpdateYUVTexture(
		LVP_Player::videoContext.texture->data, NULL,
		LVP_Player::videoContext.frameEncoded->data[LVP_YUV_Y], LVP_Player::videoContext.frameEncoded->linesize[LVP_YUV_Y],
		LVP_Player::videoContext.frameEncoded->data[LVP_YUV_U], LVP_Player::videoContext.frameEncoded->linesize[LVP_YUV_U],
		LVP_Player::videoContext.frameEncoded->data[LVP_YUV_V], LVP_Player::videoContext.frameEncoded->linesize[LVP_YUV_V]
	);
}

void MediaPlayer::LVP_Player::Render(const SDL_Rect* destination)
{
	if (LVP_Player::state.isStopped || LVP_Player::state.quit)
		return;

	if (LVP_Player::seekRequested) {
		LVP_Player::handleSeek();
		return;
	}

	if (LVP_Player::videoContext.isReadyForRender) {
		LVP_Player::videoContext.isReadyForRender = false;
		LVP_Player::renderVideo();
		LVP_Player::removeExpiredSubs();
	}

	if ((LVP_Player::subContext.currentPTS.end > 0) &&
		(LVP_Player::state.progress > LVP_Player::subContext.currentPTS.end) &&
		(LVP_Player::state.progress < LVP_Player::subContext.nextPTS.start))
	{
		LVP_Player::clearSubTextures(true, false);
	}

	if (LVP_Player::subContext.isReadyForRender) {
		LVP_Player::subContext.isReadyForRender = false;
		LVP_Player::renderSubs();
	}

	if (LVP_Player::videoContext.isReadyForPresent) {
		LVP_Player::videoContext.isReadyForPresent = false;
		LVP_Player::present();
	}

	if (LVP_Player::callbackContext.hardwareRenderer != NULL)
		LVP_Player::renderHardwareRenderer(destination);
}

void MediaPlayer::LVP_Player::renderHardwareRenderer(const SDL_Rect* destination)
{
	if ((LVP_Player::renderContext.renderer == NULL) || (LVP_Player::renderContext.texture == NULL))
		return;

	SDL_Rect* scaledDestination = LVP_Player::getScaledVideoDestination(destination);

	SDL_RenderCopy(LVP_Player::renderContext.renderer, LVP_Player::renderContext.texture, NULL, scaledDestination);

	DELETE_POINTER(scaledDestination);
}

void MediaPlayer::LVP_Player::reset()
{
	LVP_Player::formatContext         = NULL;
	LVP_Player::formatContextExternal = NULL;
	LVP_Player::packetsThread         = NULL;
	LVP_Player::seekRequestedPaused   = false;
	LVP_Player::seekRequested         = false;
	LVP_Player::seekPosition          = -1;
	LVP_Player::timeOut               = NULL;

	LVP_Player::audioContext.reset();
	LVP_Player::renderContext.reset();
	LVP_Player::state.reset();
	LVP_Player::subContext.reset();
	LVP_Player::videoContext.reset();
}

void MediaPlayer::LVP_Player::Resize()
{
	if (LVP_Player::state.isStopped)
		return;

	LVP_Player::videoContext.isReadyForRender  = true;
	LVP_Player::videoContext.isReadyForPresent = true;

	if ((LVP_Player::state.progress >= LVP_Player::subContext.currentPTS.start) && (LVP_Player::state.progress < LVP_Player::subContext.currentPTS.end)) {
		LVP_Player::subContext.isReadyForRender  = true;
		LVP_Player::subContext.isReadyForPresent = true;
	}
}

void MediaPlayer::LVP_Player::SeekTo(double percent)
{
	if ((LVP_Player::formatContext == NULL) || LVP_Player::seekRequested)
		return;
	
	auto validPercent = std::max(0.0, std::min(1.0, percent));
	auto seekPosition = (int64_t)((double)((double)LVP_Player::state.duration * AV_TIME_BASE_D) * validPercent);

	LVP_Player::seekToPosition(seekPosition);
}

void MediaPlayer::LVP_Player::seekToPosition(int64_t position)
{
	if ((LVP_Player::formatContext == NULL) || (position == LVP_Player::seekPosition))
		return;

	LVP_Player::seekRequested = true;
	LVP_Player::seekPosition  = position;

	if (IS_VIDEO(LVP_Player::state.mediaType) && LVP_Player::state.isPaused)
		LVP_Player::seekRequestedPaused = true;
}

bool MediaPlayer::LVP_Player::SetAudioDevice(const std::string &device)
{
	LVP_Player::audioContext.isDeviceReady = false;

	bool isSuccess = true;
	bool isPaused  = LVP_Player::state.isPaused;

	if (!isPaused)
		SDL_PauseAudioDevice(LVP_Player::audioContext.deviceID, 1);

	auto currentDevice = std::string(LVP_Player::audioContext.device);

	LVP_Player::audioContext.device = std::string(device);

	if (!LVP_Player::state.isStopped)
	{
		try {
			LVP_Player::openAudioDevice(LVP_Player::audioContext.deviceSpecsWanted);
		} catch (const std::exception &e) {
			isSuccess = false;
			LVP_Player::CallbackError(e.what());
		}
	}

	if (!isPaused)
		SDL_PauseAudioDevice(LVP_Player::audioContext.deviceID, 0);

	LVP_Player::audioContext.isDeviceReady = true;

	return isSuccess;
}

void MediaPlayer::LVP_Player::SetMuted(bool muted)
{
	LVP_Player::audioContext.isMuted = muted;

	if (LVP_Player::audioContext.isMuted)
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

/**
 * @throws runtime_error
 */
void MediaPlayer::LVP_Player::SetTrack(const LVP_MediaTrack &track)
{
	if (LVP_Player::formatContext == NULL)
		throw std::runtime_error(System::LVP_Text::Format("Failed to set track %d:\n- No media has been loaded.", track.track).c_str());

	auto mediaType        = (LibFFmpeg::AVMediaType)track.mediaType;
	bool isSubtitle       = IS_SUB(mediaType);
	bool isSameAudioTrack = (IS_AUDIO(mediaType) && (track.track == LVP_Player::audioContext.index));
	bool isSameSubTrack   = (isSubtitle          && (track.track == LVP_Player::subContext.index));

	// Nothing to do, track is already set.
	if (isSameAudioTrack || isSameSubTrack)
		return;

	// Disable subs
	if (isSubtitle && (track.track < 0)) {
		LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_SUBTITLE);
		return;
	}

	bool isPaused     = LVP_Player::state.isPaused;
	auto lastProgress = (double)(LVP_Player::state.progress / (double)LVP_Player::state.duration);

	LVP_Player::audioContext.isDeviceReady = false;

	switch (mediaType) {
	case LibFFmpeg::AVMEDIA_TYPE_AUDIO:
		if (!isPaused)
			SDL_PauseAudioDevice(LVP_Player::audioContext.deviceID, 1);

		LVP_Player::closeAudio();
		LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_AUDIO);

		LVP_Media::SetMediaTrackByIndex(LVP_Player::formatContext, track.track, LVP_Player::audioContext);

		try {
			LVP_Player::openThreadAudio();
		} catch (const std::exception &e) {
			LVP_Player::CallbackError(e.what());
		}

		LVP_Player::SeekTo(lastProgress);

		if (!isPaused)
			SDL_PauseAudioDevice(LVP_Player::audioContext.deviceID, 0);

		break;
	case LibFFmpeg::AVMEDIA_TYPE_SUBTITLE:
		LVP_Player::closeSub();
		LVP_Player::closeStream(LibFFmpeg::AVMEDIA_TYPE_SUBTITLE);

		if (track.track >= SUB_STREAM_EXTERNAL)
			LVP_Player::openSubExternal(track.track);
		else
			LVP_Media::SetMediaTrackByIndex(LVP_Player::formatContext, track.track, LVP_Player::subContext);

		LVP_Player::openThreadSub();
		LVP_Player::SeekTo(lastProgress);

		break;
	default:
		break;
	}

	LVP_Player::audioContext.isDeviceReady = true;
}

void MediaPlayer::LVP_Player::SetVolume(double percent)
{
	auto validPercent = std::max(0.0, std::min(1.0, percent));
	auto volume       = (int)((double)SDL_MIX_MAXVOLUME * validPercent);

	LVP_Player::audioContext.volume = std::max(0, std::min(SDL_MIX_MAXVOLUME, volume));

	LVP_Player::callbackEvents(LVP_EVENT_AUDIO_VOLUME_CHANGED);
}

void MediaPlayer::LVP_Player::stop(const std::string &errorMessage)
{
	LVP_Player::state.quit = true;
	
	if (!errorMessage.empty())
		LVP_Player::CallbackError(errorMessage);
}

void MediaPlayer::LVP_Player::threadAudio(void* userData, Uint8* stream, int streamSize)
{
	if (!LVP_Player::audioContext.isDeviceReady || !LVP_Player::audioContext.isDriverReady)
		return;

	while (LVP_Player::state.isPaused && !LVP_Player::state.quit)
		SDL_Delay(DELAY_TIME_DEFAULT);

	while ((streamSize > 0) && LVP_Player::state.isPlaying && !LVP_Player::seekRequested && !LVP_Player::state.quit)
	{
		if (LVP_Player::audioContext.decodeFrame)
		{
			// Get packet from queue
			auto packet = LVP_Player::packetGet(LVP_Player::audioContext);

			if (packet == NULL)
				break;

			// Send packet to get decoded
			auto result = LibFFmpeg::avcodec_send_packet(LVP_Player::audioContext.codec, packet);

			if ((result < 0) || LVP_Player::state.quit) {
				FREE_AVPACKET(packet);
				break;
			}

			// Get decoded frames (some packets contain multiple frames)
			while (result == 0)
			{
				result = LibFFmpeg::avcodec_receive_frame(LVP_Player::audioContext.codec, LVP_Player::audioContext.frame);

				if (result < 0)
				{
					if (result != AVERROR(EAGAIN))
					{
						#if defined _DEBUG
							char strerror[AV_ERROR_MAX_STRING_SIZE];
							LibFFmpeg::av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);
							LOG("%s\n", strerror);
						#endif

						LVP_Player::audioContext.bufferRemaining = 0;
					}

					LVP_Player::audioContext.bufferOffset = 0;

					break;
				}

				// Apply atempo (speed) filter to frame

				bool specsHaveChanged = LVP_Player::audioContext.specs.hasChanged(LVP_Player::audioContext.frame, LVP_Player::state.playbackSpeed);

				if (specsHaveChanged || (LVP_Player::audioContext.filter.filterGraph == NULL)) {
					LVP_Player::audioContext.specs.init(LVP_Player::audioContext.frame, LVP_Player::state.playbackSpeed);
					LVP_Player::initAudioFilter();
				}

				LibFFmpeg::av_buffersrc_add_frame(LVP_Player::audioContext.filter.bufferSource, LVP_Player::audioContext.frame);
				
				auto filterResult = LibFFmpeg::av_buffersink_get_frame(LVP_Player::audioContext.filter.bufferSink, LVP_Player::audioContext.frame);

				// Get more frames if needed
				if (filterResult == AVERROR(EAGAIN))
					continue;

				// FRAME DURATION

				if (packet->duration > 0)
				{
					auto packetDuration = (double)packet->duration;
					auto streamTimeBase = LibFFmpeg::av_q2d(LVP_Player::audioContext.stream->time_base);

					LVP_Player::audioContext.frameDuration = (packetDuration * streamTimeBase);
				}

				// PROGRESS

				if (LVP_Player::state.progress > 0)
					LVP_Player::audioContext.lastPogress = LVP_Player::state.progress;

				LVP_Player::state.progress = LVP_Media::GetAudioPTS(LVP_Player::audioContext);

				if ((LVP_Player::audioContext.frameDuration <= 0) &&
					(LVP_Player::state.progress > 0) &&
					(LVP_Player::audioContext.lastPogress > 0) &&
					(LVP_Player::state.progress != LVP_Player::audioContext.lastPogress))
				{
					LVP_Player::audioContext.frameDuration = (LVP_Player::state.progress - LVP_Player::audioContext.lastPogress);
					LVP_Player::audioContext.lastPogress   = LVP_Player::state.progress;
				}

				// RESAMPLE CONTEXT

				LibFFmpeg::AVChannelLayout outChannelLayout, inChannelLayout;

				LibFFmpeg::av_channel_layout_default(&outChannelLayout, LVP_Player::audioContext.deviceSpecs.channels);
				LibFFmpeg::av_channel_layout_default(&inChannelLayout,  LVP_Player::audioContext.frame->ch_layout.nb_channels);

				auto inSampleFormat = (LibFFmpeg::AVSampleFormat)LVP_Player::audioContext.frame->format;
				auto inSampleRate   = LVP_Player::audioContext.frame->sample_rate;

				auto outSampleFormat = LVP_Player::getAudioSampleFormat(LVP_Player::audioContext.deviceSpecs.format);
				auto outSampleRate   = LVP_Player::audioContext.deviceSpecs.freq;

				if (LVP_Player::audioContext.specs.initContext || (LVP_Player::audioContext.specs.swrContext == NULL))
				{
					LVP_Player::audioContext.specs.initContext = false;

					FREE_SWR(LVP_Player::audioContext.specs.swrContext);

					auto allocResult = LibFFmpeg::swr_alloc_set_opts2(
						&LVP_Player::audioContext.specs.swrContext,
						&outChannelLayout, outSampleFormat, outSampleRate,
						&inChannelLayout,  inSampleFormat,  inSampleRate,
						0, NULL
					);

					if ((allocResult < 0) || (LVP_Player::audioContext.specs.swrContext == NULL)) {
						LVP_Player::stop("Failed to allocate an audio resample context.");
						break;
					}

					if (LibFFmpeg::swr_is_initialized(LVP_Player::audioContext.specs.swrContext) < 1)
					{
						if (LibFFmpeg::swr_init(LVP_Player::audioContext.specs.swrContext) < 0) {
							LVP_Player::stop("Failed to initialize the audio resample context.");
							break;
						}
					}
				}

				if (LVP_Player::state.quit)
					break;

				// Calculate needed frame buffer size for resampling
				
				auto bytesPerSample  = (LVP_Player::audioContext.deviceSpecs.channels * LibFFmpeg::av_get_bytes_per_sample(outSampleFormat));
				auto nextSampleCount = LibFFmpeg::swr_get_out_samples(LVP_Player::audioContext.specs.swrContext, LVP_Player::audioContext.frame->nb_samples);
				auto nextBufferSize  = (nextSampleCount * bytesPerSample);

				// Increase the frame buffer size if needed
				
				if (nextBufferSize > LVP_Player::audioContext.bufferSize)
				{
					auto frameEncoded = (uint8_t*)realloc(LVP_Player::audioContext.frameEncoded, nextBufferSize);

					LVP_Player::audioContext.frameEncoded = frameEncoded;
					LVP_Player::audioContext.bufferSize   = nextBufferSize;
				}

				// Resample the audio frame

				auto samplesResampled = LibFFmpeg::swr_convert(
					LVP_Player::audioContext.specs.swrContext,
					&LVP_Player::audioContext.frameEncoded, nextSampleCount,
					(const uint8_t**)LVP_Player::audioContext.frame->extended_data, LVP_Player::audioContext.frame->nb_samples
				);

				if (samplesResampled < 0) {
					LVP_Player::stop("Failed to resample the audio frame.");
					break;
				}

				if (LVP_Player::state.quit)
					break;

				// Calculate the actual frame buffer size after resampling

				LVP_Player::audioContext.bufferRemaining = (samplesResampled * bytesPerSample);
				LVP_Player::audioContext.bufferOffset    = 0;
			}

			FREE_AVPACKET(packet);
		}

		if (LVP_Player::seekRequested || LVP_Player::state.quit)
			break;

		// Calculate how much to write to the stream

		int writeSize;

		if (LVP_Player::audioContext.writtenToStream > 0)
			writeSize = (streamSize - LVP_Player::audioContext.writtenToStream);
		else if (LVP_Player::audioContext.bufferRemaining > streamSize)
			writeSize = streamSize;
		else
			writeSize = LVP_Player::audioContext.bufferRemaining;

		if (writeSize > LVP_Player::audioContext.bufferRemaining)
			writeSize = LVP_Player::audioContext.bufferRemaining;

		// Write the data from the buffer to the stream

		memset(stream, 0, writeSize);

		auto buffer = static_cast<const Uint8*>(LVP_Player::audioContext.frameEncoded + LVP_Player::audioContext.bufferOffset);
		auto volume = (!LVP_Player::audioContext.isMuted ? LVP_Player::audioContext.volume : 0);

		SDL_MixAudioFormat(stream, buffer, LVP_Player::audioContext.deviceSpecs.format, writeSize, volume);

		// Update counters with how much was written

		stream                                   += writeSize;
		LVP_Player::audioContext.writtenToStream += writeSize;
		LVP_Player::audioContext.bufferRemaining -= writeSize;

		// STREAM IS FULL - BUFFER IS NOT Get a new stream, don't decode frame.
		if (((LVP_Player::audioContext.writtenToStream % streamSize) == 0) && (LVP_Player::audioContext.bufferRemaining > 0))
		{
			LVP_Player::audioContext.decodeFrame     = false;
			LVP_Player::audioContext.writtenToStream = 0;
			LVP_Player::audioContext.bufferOffset   += writeSize;

			break;
		}
		// STREAM IS FULL - BUFFER IS FULL - Get a new stream, decode frame.
		else if (((LVP_Player::audioContext.writtenToStream % streamSize) == 0) && (LVP_Player::audioContext.bufferRemaining <= 0))
		{
			LVP_Player::audioContext.decodeFrame     = true;
			LVP_Player::audioContext.writtenToStream = 0;

			break;
		}
		// STREAM IS NOT FULL - BUFFER IS FULL - Don't get a new stream, decode frame.
		else if (((LVP_Player::audioContext.writtenToStream % streamSize) != 0) && (LVP_Player::audioContext.bufferRemaining <= 0))
		{
			LVP_Player::audioContext.decodeFrame = true;
		}
		// STREAM IS NOT FULL - BUFFER IS NOT FULL - Don't get a new stream, don't decode frame.
		else if (((LVP_Player::audioContext.writtenToStream % streamSize) != 0) && (LVP_Player::audioContext.bufferRemaining > 0))
		{
			LVP_Player::audioContext.decodeFrame   = false;
			LVP_Player::audioContext.bufferOffset += writeSize;
		}
	}
}

int MediaPlayer::LVP_Player::threadPackets(void* userData)
{
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

		// Read and fill in the packet

		LVP_Player::packetsLock.lock();

		auto result = LibFFmpeg::av_read_frame(LVP_Player::formatContext, packet);

		LVP_Player::packetsLock.unlock();

		if ((result < 0) || LVP_Player::state.quit)
		{
			if (LVP_Player::timeOut != NULL)
				LVP_Player::timeOut->stop();

			// Is the media file completed (EOF) or did an error occur?

			if ((result == AVERROR_EOF) || LibFFmpeg::avio_feof(formatContext->pb))
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

				while (!LVP_Player::audioContext.packets.empty() && !LVP_Player::state.quit)
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

		// AUDIO PACKET
		if (packet->stream_index == LVP_Player::audioContext.index)
			LVP_Player::packetAdd(packet, LVP_Player::audioContext);
		// VIDEO PACKET
		else if (packet->stream_index == LVP_Player::videoContext.index)
			LVP_Player::packetAdd(packet, LVP_Player::videoContext);
		// SUB PACKET - INTERNAL
		else if ((packet->stream_index == LVP_Player::subContext.index) && (LVP_Player::subContext.index < SUB_STREAM_EXTERNAL))
			LVP_Player::packetAdd(packet, LVP_Player::subContext);
		// UNKNOWN DATA PACKET
		else
			FREE_AVPACKET(packet);

		// EXTARNAL SUB PACKET
		if ((LVP_Player::subContext.index >= SUB_STREAM_EXTERNAL) && (LVP_Player::subContext.packets.size() < MIN_PACKET_QUEUE_SIZE))
		{
			auto packet = LibFFmpeg::av_packet_alloc();
			
			if (packet != NULL)
			{
				int extSubStream = ((LVP_Player::subContext.index - SUB_STREAM_EXTERNAL) % SUB_STREAM_EXTERNAL);

				LVP_Player::packetsLock.lock();

				if ((av_read_frame(LVP_Player::formatContextExternal, packet) == 0) && (packet->stream_index == extSubStream))
					LVP_Player::packetAdd(packet, LVP_Player::subContext);
				else
					FREE_AVPACKET(packet);

				LVP_Player::packetsLock.unlock();
			}
		}

		if (LVP_Player::state.quit)
			break;

		// MEDIA TRACKS UPDATED
		if (LVP_Player::formatContext->nb_streams != LVP_Player::state.trackCount) {
			LVP_Player::state.trackCount = LVP_Player::formatContext->nb_streams;
			LVP_Player::callbackEvents(LVP_EVENT_MEDIA_TRACKS_UPDATED);
		}

		// METADATA UPDATED
		if (LVP_Player::formatContext->event_flags & AVFMT_EVENT_FLAG_METADATA_UPDATED) {
			LVP_Player::formatContext->event_flags &= ~AVFMT_EVENT_FLAG_METADATA_UPDATED;
			LVP_Player::callbackEvents(LVP_EVENT_METADATA_UPDATED);
		}

		// Wait while the packet queues are full
		while (LVP_Player::isPacketQueueFull() && !LVP_Player::seekRequested && !LVP_Player::state.quit)
			SDL_Delay(DELAY_TIME_ONE_MS);
	}

	if (endOfFile || (errorCount >= MAX_ERRORS))
	{
		LVP_Player::Close();

		if (errorCount >= MAX_ERRORS)
			LVP_Player::callbackEvents(LVP_EVENT_MEDIA_COMPLETED_WITH_ERRORS);
		else
			LVP_Player::callbackEvents(LVP_EVENT_MEDIA_COMPLETED);
	}

	return 0;
}

int MediaPlayer::LVP_Player::threadSub(void* userData)
{
	int frameDecoded;

	LibFFmpeg::AVSubtitle subFrame = {};

	while (!LVP_Player::state.quit)
	{
		// Wait until subtitle packets are available
		while (!LVP_Player::state.quit &&
			(LVP_Player::subContext.packets.empty() ||
			(LVP_Player::state.isPaused && !LVP_Player::seekRequestedPaused) ||
			LVP_Player::seekRequested ||
			(LVP_Player::subContext.index < 0) ||
			(LVP_Player::audioContext.index < 0)))
		{
			SDL_Delay(DELAY_TIME_DEFAULT);
		}

		if (LVP_Player::state.quit)
			break;

		// Get packet from queue
		auto packet = LVP_Player::packetGet(LVP_Player::subContext);

		if (packet == NULL)
			continue;

		// Decode packet to frame
		auto decodeResult = LibFFmpeg::avcodec_decode_subtitle2(LVP_Player::subContext.codec, &subFrame, &frameDecoded, packet);

		// INVALID PACKET
		if ((decodeResult < 0) || !frameDecoded) {
			FREE_AVPACKET(packet);
			continue;
		}

		if (LVP_Player::state.quit)
			break;

		SDL_LockMutex(LVP_Player::subContext.subsMutex);
		LVP_Player::subContext.available = false;

		// Replace yellow DVD subs with white subs (if it's missing the color palette)
		if (strcmp(LVP_Player::subContext.codec->codec->name, "dvdsub") == 0)
		{
			auto dvdSubContext = static_cast<LVP_DVDSubContext*>(LVP_Player::subContext.codec->priv_data);

			if ((dvdSubContext != NULL) && !dvdSubContext->has_palette)
			{
				dvdSubContext->has_palette = 1;

				dvdSubContext->palette[dvdSubContext->colormap[1]] = 0xFFFFFFFF; // TEXT
				dvdSubContext->palette[dvdSubContext->colormap[3]] = 0xFF000000; // BORDER
			}
		}

		// Some sub types, like PGS, come in pairs:
		// - The first one with data but without duration or end PTS
		// - The second one has no data, but contains the end PTS
		bool isPGSSub = (strcmp(LVP_Player::subContext.codec->codec->name, "pgssub") == 0);

		// Update End PTS for previous subs.
		if (isPGSSub && (packet->size < MIN_SUB_PACKET_SIZE) && !LVP_Player::subContext.subs.empty())
		{
			double ptsEnd = LVP_Media::GetSubtitleEndPTS(packet, LVP_Player::subContext.stream->time_base);

			for (auto sub : LVP_Player::subContext.subs) {
				if (sub->pts.end == UINT32_MAX)
					sub->pts.end = ptsEnd;
			}
		}

		LVP_Player::subContext.available = true;
		SDL_CondSignal(LVP_Player::subContext.subsCondition);
		SDL_UnlockMutex(LVP_Player::subContext.subsMutex);

		// NO FRAMES DECODED
		if ((subFrame.num_rects == 0) || ((packet->duration == 0) && (subFrame.end_display_time == 0))) {
			FREE_AVPACKET(packet);
			continue;
		}

		// SUB PTS
		auto pts = LVP_Media::GetSubtitlePTS(packet, subFrame, LVP_Player::subContext.stream->time_base, LVP_Player::audioContext.stream->start_time);

		FREE_AVPACKET(packet);

		// Extract sub type (text/bitmap) from frame

		Strings       subTexts;
		LVP_Subtitle* bitmapSub = NULL;

		for (uint32_t i = 0; i < subFrame.num_rects; i++)
		{
			if (subFrame.rects[i] == NULL)
				continue;

			LVP_Player::subContext.type = subFrame.rects[i]->type;

			switch (LVP_Player::subContext.type) {
			case LibFFmpeg::SUBTITLE_ASS:
				if ((subFrame.rects[i]->ass != NULL) && (strlen(subFrame.rects[i]->ass) > 0)) {
					subTexts.push_back(subFrame.rects[i]->ass);
					FREE_SUB_TEXT(subFrame.rects[i]->ass);
				}
				break;
			case LibFFmpeg::SUBTITLE_TEXT:
				if ((subFrame.rects[i]->text != NULL) && (strlen(subFrame.rects[i]->text) > 0)) {
					subTexts.push_back(subFrame.rects[i]->text);
					FREE_SUB_TEXT(subFrame.rects[i]->text);
				}
				break;
			case LibFFmpeg::SUBTITLE_BITMAP:
				bitmapSub          = new LVP_Subtitle();
				bitmapSub->pts     = LVP_SubPTS(pts);
				bitmapSub->subRect = new LVP_SubRect(*subFrame.rects[i]);
				break;
			default:
				break;
			}
		}

		FREE_SUB_FRAME(subFrame);

		// Split and format the text subs

		LVP_Subtitles textSubs;

		if (IS_SUB_TEXT(LVP_Player::subContext.type))
		{
			SDL_LockMutex(LVP_Player::subContext.subsMutex);

			if (!LVP_Player::subContext.available)
				SDL_CondWait(LVP_Player::subContext.subsCondition, LVP_Player::subContext.subsMutex);

			textSubs = LVP_SubTextRenderer::SplitAndFormatSub(subTexts, LVP_Player::subContext);

			SDL_UnlockMutex(LVP_Player::subContext.subsMutex);

			for (auto textSub : textSubs)
				textSub->pts = LVP_SubPTS(pts);

			#if _DEBUG
			for (const auto &subText : subTexts)
				printf("[%.3f,%.3f] %s\n", pts.start, pts.end, subText.c_str());
			#endif
		}

		SDL_LockMutex(LVP_Player::subContext.subsMutex);
		LVP_Player::subContext.available = false;

		if (bitmapSub != NULL)
			LVP_Player::subContext.subs.push_back(bitmapSub);

		for (auto textSub : textSubs)
			LVP_Player::subContext.subs.push_back(textSub);

		LVP_Player::subContext.available = true;
		SDL_CondSignal(LVP_Player::subContext.subsCondition);
		SDL_UnlockMutex(LVP_Player::subContext.subsMutex);

		bool isNextPTS = ((pts.start >= LVP_Player::subContext.pts.end) && (pts.end > LVP_Player::subContext.pts.end));

		LVP_Player::subContext.nextPTS = pts;

		while (LVP_Player::subContext.isReadyForPresent && isNextPTS && !LVP_Player::seekRequested && !LVP_Player::state.quit)
			SDL_Delay(DELAY_TIME_ONE_MS);

		if (LVP_Player::seekRequested || LVP_Player::state.quit)
			continue;

		LVP_Player::subContext.pts = pts;

		auto timeToSleep = (LVP_Player::subContext.pts.start - LVP_Player::state.progress);

		// Sub is ahead of audio, skip or speed up.

		if (timeToSleep < 0)
		{
			if (!isNextPTS) {
				LVP_Player::subContext.isReadyForRender  = true;
				LVP_Player::subContext.isReadyForPresent = true;
			}

			continue;
		}

		// Prepare subs by rendering to background texture buffer

		LVP_Player::subContext.isReadyForPresent = false;
		LVP_Player::subContext.isReadyForRender  = true;

		// Sub is behind audio, wait or slow down.

		auto timeUntilPTS = (LVP_Player::subContext.pts.start - LVP_Player::state.progress);

		while ((timeUntilPTS >= DELAY_TIME_DEFAULT_S) && !LVP_Player::seekRequested && !LVP_Player::state.quit) {
			SDL_Delay(DELAY_TIME_DEFAULT);
			timeUntilPTS = (LVP_Player::subContext.pts.start - LVP_Player::state.progress);
		}

		if (LVP_Player::seekRequested || LVP_Player::state.quit)
			continue;

		// Make the subs available for presentation

		LVP_Player::subContext.isReadyForPresent = true;

		LVP_Player::subContext.currentPTS = pts;
	}

	FREE_SUB_FRAME(subFrame);

	SDL_LockMutex(LVP_Player::subContext.subsMutex);
	LVP_Player::subContext.available = false;

	LVP_Player::clearSubs();

	LVP_Player::subContext.available = true;
	SDL_CondSignal(LVP_Player::subContext.subsCondition);
	SDL_UnlockMutex(LVP_Player::subContext.subsMutex);

	return 0;
}

int MediaPlayer::LVP_Player::threadVideo(void* userData)
{
	auto& videoFrame         = LVP_Player::videoContext.frame;
	auto& videoFrameHardware = LVP_Player::videoContext.frameHardware;
	auto& videoFrameSoftware = LVP_Player::videoContext.frameSoftware;

	auto videoStream = LVP_Player::videoContext.stream;

	int  errorCount                  = 0;
	auto videoFrameDuration2x        = (int)(LVP_Player::videoContext.frameRate * 2.0 * (double)ONE_SECOND_MS);
	auto videoFrameDuration2xSeconds = (LVP_Player::videoContext.frameRate * 2.0);

	while (!LVP_Player::state.quit)
	{
		// Wait until video packets are available
		while ((LVP_Player::videoContext.packets.empty() || LVP_Player::seekRequested) && !LVP_Player::state.quit)
			SDL_Delay(DELAY_TIME_DEFAULT);

		if (LVP_Player::state.quit)
			break;

		// Get packet from queue
		auto packet = LVP_Player::packetGet(LVP_Player::videoContext);

		if (packet == NULL)
			continue;

		// Send packet to get decoded
		auto result = LibFFmpeg::avcodec_send_packet(LVP_Player::videoContext.codec, packet);

		if ((result < 0) || LVP_Player::state.quit) {
			FREE_AVPACKET(packet);
			continue;
		}

		// Get decoded frame
		result = LibFFmpeg::avcodec_receive_frame(LVP_Player::videoContext.codec, videoFrameHardware);

		FREE_AVPACKET(packet);

		if (LVP_Player::state.quit)
			break;

		if ((LibFFmpeg::AVPixelFormat)videoFrameHardware->format == LVP_Player::videoContext.pixelFormatHardware)
		{
			result = LibFFmpeg::av_hwframe_transfer_data(videoFrameSoftware, videoFrameHardware, 0);

			videoFrameSoftware->best_effort_timestamp = videoFrameHardware->best_effort_timestamp;

			videoFrame = videoFrameSoftware;

		} else {
			videoFrame = videoFrameHardware;
		}

		if ((result < 0) || !AVFRAME_IS_VALID(videoFrame))
		{
			if (result != AVERROR(EAGAIN))
			{
				#if defined _DEBUG
					char strerror[AV_ERROR_MAX_STRING_SIZE];
					LibFFmpeg::av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);
					LOG("%s\n", strerror);
				#endif
			}

			if (errorCount > MAX_ERRORS) {
				LVP_Player::stop("Too many errors occured while decoding the video.");
				break;
			}
			
			errorCount++;

			if (LVP_Player::state.progress > 1.0) {
				LibFFmpeg::av_frame_unref(videoFrame);
				videoFrame = LibFFmpeg::av_frame_alloc();
			}

			continue;
		}

		errorCount = 0;

		// AUDIO - Always display audio/music cover image
		if (IS_AUDIO(LVP_Player::state.mediaType) && (videoStream->attached_pic.size > 0))
			LVP_Player::videoContext.pts = LVP_Player::state.progress;
		// VIDEO - Calculate video present time (pts)
		else
			LVP_Player::videoContext.pts = LVP_Media::GetVideoPTS(videoFrame, videoStream->time_base, LVP_Player::audioContext.stream->start_time);

		// Wait while paused (unless a seek is requested)
		while (LVP_Player::state.isPaused && !LVP_Player::seekRequested && !LVP_Player::seekRequestedPaused && !LVP_Player::state.quit)
			SDL_Delay(DELAY_TIME_DEFAULT);

		if (LVP_Player::state.quit)
			break;

		// Update video if seek was requested while paused
		if (LVP_Player::seekRequestedPaused)
		{
			// Keep seeking until the video and audio catch up
			if (fabs(LVP_Player::state.progress - LVP_Player::videoContext.pts) > videoFrameDuration2xSeconds)
				continue;

			LVP_Player::seekRequestedPaused = false;

			LVP_Player::videoContext.isReadyForRender  = true;
			LVP_Player::videoContext.isReadyForPresent = true;

			LVP_Player::clearSubTextures(true, false);

			for (auto sub : LVP_Player::subContext.subs)
			{
				if ((LVP_Player::state.progress < sub->pts.start) || (LVP_Player::state.progress >= sub->pts.end))
					continue;

				LVP_Player::subContext.isReadyForRender  = true;
				LVP_Player::subContext.isReadyForPresent = true;

				break;
			}
		}

		auto sleepTime = (int)((LVP_Player::videoContext.pts - LVP_Player::state.progress) * (double)ONE_SECOND_MS);

		LVP_Player::videoContext.isReadyForRender = true;

		// Video is behind audio - wait or slow down
		if (sleepTime > 0)
			SDL_Delay(std::min(sleepTime, videoFrameDuration2x));

		LVP_Player::videoContext.isReadyForPresent = true;
	}

	return 0;
}

void MediaPlayer::LVP_Player::ToggleMute()
{
	LVP_Player::SetMuted(!LVP_Player::audioContext.isMuted);
}

void MediaPlayer::LVP_Player::TogglePause()
{
	if (!LVP_Player::state.isPaused)
		LVP_Player::pause();
	else
		LVP_Player::play();
}
