#include "LVP_Player.h"

MediaPlayer::LVP_AudioContext    MediaPlayer::LVP_Player::audioContext;
std::mutex                       MediaPlayer::LVP_Player::audioFilterLock;
LVP_CallbackContext              MediaPlayer::LVP_Player::callbackContext;
LibFFmpeg::AVFormatContext*      MediaPlayer::LVP_Player::formatContext;
LibFFmpeg::AVFormatContext*      MediaPlayer::LVP_Player::formatContextExternal;
bool                             MediaPlayer::LVP_Player::isStopping;
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

	SDL_SetRenderDrawColor(LVP_Player::renderContext.renderer, 0, 0, 0, 0);
	SDL_RenderClear(LVP_Player::renderContext.renderer);
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
	if (LVP_Player::isStopping || LVP_Player::state.isStopped)
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
	FREE_SWR(LVP_Player::audioContext.resampleContext);
	FREE_THREAD_COND(LVP_Player::audioContext.condition);
	FREE_THREAD_MUTEX(LVP_Player::audioContext.mutex);
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

	FREE_AVFRAME(LVP_Player::videoContext.frame);
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
	return LVP_Player::getMediaTracks(LibFFmpeg::AVMEDIA_TYPE_AUDIO);
}

std::vector<LVP_MediaChapter> MediaPlayer::LVP_Player::GetChapters()
{
	std::vector<LVP_MediaChapter> chapters;

	if (LVP_Player::formatContext == NULL)
		return chapters;

	int64_t lastChapterEnd = 0;

	for (unsigned int i = 0; i < LVP_Player::formatContext->nb_chapters; i++)
	{
		auto chapter = LVP_Player::formatContext->chapters[i];

		if (chapter->start < lastChapterEnd)
			continue;

		lastChapterEnd = chapter->end;
			
		auto title = LibFFmpeg::av_dict_get(chapter->metadata, "title", NULL, 0);

		if (title == NULL)
			continue;

		auto timeBase = LibFFmpeg::av_q2d(chapter->time_base);
		auto end      = (double)((double)chapter->end   * timeBase);
		auto start    = (double)((double)chapter->start * timeBase);

		chapters.push_back({
			.title     = std::string(title->value),
			.startTime = (int64_t)(start * 1000.0),
			.endTime   = (int64_t)(end   * 1000.0),
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
			auto font16          = (wchar_t*)SDL_iconv_utf8_ucs2(familyName);
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

LVP_MediaMeta MediaPlayer::LVP_Player::GetMediaMeta()
{
	return
	{
		.audioTracks    = LVP_Player::GetAudioTracks(),
		.subtitleTracks = LVP_Player::GetSubtitleTracks(),
		.videoTracks    = LVP_Player::GetVideoTracks(),
		.meta           = LVP_Media::GetMediaMeta(LVP_Player::formatContext)
	};
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::getMediaTracks(LibFFmpeg::AVMediaType mediaType)
{
	std::vector<LVP_MediaTrack> tracks;

	// DISABLE SUBS

	if (IS_SUB(mediaType) && LVP_Media::HasSubtitleTracks(LVP_Player::formatContext, LVP_Player::formatContextExternal))
	{
		std::unordered_map<std::string, std::string> trackMeta;

		trackMeta["title"] = "Disable";

		tracks.push_back({
			.mediaType = (LVP_MediaType)LibFFmpeg::AVMEDIA_TYPE_SUBTITLE,
			.track     = -1,
			.meta      = trackMeta
		});
	}

	// MEDIA TRACKS

	auto mediaTracks = LVP_Player::getMediaTracksMeta(LVP_Player::formatContext, mediaType);

	tracks.insert(tracks.end(), mediaTracks.begin(), mediaTracks.end());

	// EXTERNAL SUB TRACKS

	if (IS_SUB(mediaType))
	{
		auto externalSubTracks = LVP_Player::getMediaTracksMeta(LVP_Player::formatContextExternal, LibFFmpeg::AVMEDIA_TYPE_SUBTITLE, true);

		tracks.insert(tracks.end(), externalSubTracks.begin(), externalSubTracks.end());
	}

	return tracks;
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::getMediaTracksMeta(LibFFmpeg::AVFormatContext* formatContext, LibFFmpeg::AVMediaType mediaType, bool isSubsExternal)
{
	std::vector<LVP_MediaTrack> tracks;

	if (formatContext == NULL)
		return tracks;

	for (unsigned int i = 0; i < formatContext->nb_streams; i++)
	{
		auto stream = formatContext->streams[i];

		if ((stream == NULL) || (stream->codecpar == NULL) || (stream->codecpar->codec_type != mediaType))
			continue;

		tracks.push_back({
			.mediaType = (LVP_MediaType)mediaType,
			.track     = (int)(isSubsExternal ? (SUB_STREAM_EXTERNAL + i) : i),
			.meta      = LVP_Media::GetMediaTrackMeta(stream),
			.codec     = LVP_Media::GetMediaCodecMeta(stream)
		});
	}

	return tracks;
}

LVP_MediaType MediaPlayer::LVP_Player::GetMediaType()
{
	return (LVP_MediaType)LVP_Player::state.mediaType;
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
	int scaledWidth  = min(destination->w, maxWidth);
	int scaledHeight = min(destination->h, maxHeight);

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
	return LVP_Player::getMediaTracks(LibFFmpeg::AVMEDIA_TYPE_SUBTITLE);
}

std::vector<LVP_MediaTrack> MediaPlayer::LVP_Player::GetVideoTracks()
{
	return LVP_Player::getMediaTracks(LibFFmpeg::AVMEDIA_TYPE_VIDEO);
}

uint32_t MediaPlayer::LVP_Player::getVideoPixelFormat(LibFFmpeg::AVPixelFormat pixelFormat)
{
	// https://wiki.videolan.org/YUV/
	switch (pixelFormat) {
	case LibFFmpeg::AV_PIX_FMT_YUV420P:
	case LibFFmpeg::AV_PIX_FMT_YUVJ420P:
		return SDL_PIXELFORMAT_YV12;
	case LibFFmpeg::AV_PIX_FMT_YUYV422:
		return SDL_PIXELFORMAT_YUY2;
	case LibFFmpeg::AV_PIX_FMT_UYVY422:
		return SDL_PIXELFORMAT_UYVY;
	case LibFFmpeg::AV_PIX_FMT_YVYU422:
		return SDL_PIXELFORMAT_YVYU;
	case LibFFmpeg::AV_PIX_FMT_NV12:
		return SDL_PIXELFORMAT_NV12;
	case LibFFmpeg::AV_PIX_FMT_NV21:
		return SDL_PIXELFORMAT_NV21;
	default:
		break;
	}

	return SDL_PIXELFORMAT_YV12;
}

LibFFmpeg::AVPixelFormat MediaPlayer::LVP_Player::getVideoPixelFormat(uint32_t pixelFormat)
{
	switch (pixelFormat) {
		case SDL_PIXELFORMAT_YV12: return LibFFmpeg::AV_PIX_FMT_YUV420P;
		case SDL_PIXELFORMAT_YUY2: return LibFFmpeg::AV_PIX_FMT_YUYV422;
		case SDL_PIXELFORMAT_UYVY: return LibFFmpeg::AV_PIX_FMT_UYVY422;
		case SDL_PIXELFORMAT_YVYU: return LibFFmpeg::AV_PIX_FMT_YVYU422;
		case SDL_PIXELFORMAT_NV12: return LibFFmpeg::AV_PIX_FMT_NV12;
		case SDL_PIXELFORMAT_NV21: return LibFFmpeg::AV_PIX_FMT_NV21;
		default: break;
	}

	return LibFFmpeg::AV_PIX_FMT_YUV420P;
}

double MediaPlayer::LVP_Player::GetVolume()
{
	return (double)((double)LVP_Player::audioContext.volume / (double)SDL_MIX_MAXVOLUME);
}

void MediaPlayer::LVP_Player::handleSeek()
{
	const int SEEK_FLAGS = AV_SEEK_FLAGS(LVP_Player::formatContext->iformat);

	if (LibFFmpeg::avformat_seek_file(LVP_Player::formatContext, -1, INT64_MIN, LVP_Player::seekPosition, INT64_MAX, SEEK_FLAGS) >= 0)
	{
		if (LVP_Player::subContext.index >= SUB_STREAM_EXTERNAL)
			LibFFmpeg::avformat_seek_file(LVP_Player::formatContextExternal, -1, INT64_MIN, LVP_Player::seekPosition, INT64_MAX, SEEK_FLAGS);

		LVP_Player::closePackets();

		LVP_Player::audioContext.bufferOffset    = 0;
		LVP_Player::audioContext.bufferRemaining = 0;
		LVP_Player::audioContext.writtenToStream = 0;
		LVP_Player::audioContext.decodeFrame     = true;
		LVP_Player::audioContext.lastPogress     = 0;
		LVP_Player::videoContext.pts             = 0.0;

		if (AV_SEEK_BYTES(LVP_Player::formatContext->iformat, LVP_Player::state.fileSize))
		{
			auto percent = (double)((double)LVP_Player::seekPosition / (double)LVP_Player::state.fileSize);

			LVP_Player::state.progress = (double)((double)LVP_Player::state.duration * percent);
		} else {
			LVP_Player::state.progress = (double)((double)LVP_Player::seekPosition / AV_TIME_BASE_D);
		}
	}

	LVP_Player::seekRequested = false;
	LVP_Player::seekPosition  = -1;
}

void MediaPlayer::LVP_Player::initSubTextures()
{
	if (IS_VALID_TEXTURE(LVP_Player::subContext.textureCurrent) && IS_VALID_TEXTURE(LVP_Player::subContext.textureNext))
		return;

	DELETE_POINTER(LVP_Player::subContext.textureCurrent);
	DELETE_POINTER(LVP_Player::subContext.textureNext);

	LVP_Player::subContext.textureCurrent = new Graphics::LVP_Texture(
		SUB_PIXEL_FORMAT_SDL,
		SDL_TEXTUREACCESS_TARGET,
		LVP_Player::videoContext.frame->width,
		LVP_Player::videoContext.frame->height,
		LVP_Player::renderContext.renderer
	);

	LVP_Player::subContext.textureNext = new Graphics::LVP_Texture(
		SUB_PIXEL_FORMAT_SDL,
		SDL_TEXTUREACCESS_TARGET,
		LVP_Player::videoContext.frame->width,
		LVP_Player::videoContext.frame->height,
		LVP_Player::renderContext.renderer
	);

	auto renderTarget = SDL_GetRenderTarget(LVP_Player::renderContext.renderer);

	LVP_Player::clearSubTexture(LVP_Player::subContext.textureCurrent->data);
	LVP_Player::clearSubTexture(LVP_Player::subContext.textureNext->data);

	SDL_SetRenderTarget(LVP_Player::renderContext.renderer, renderTarget);
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

bool MediaPlayer::LVP_Player::isYUV(LibFFmpeg::AVPixelFormat pixelFormat)
{
	switch (pixelFormat) {
	case LibFFmpeg::AV_PIX_FMT_YUV420P:
	case LibFFmpeg::AV_PIX_FMT_YUVJ420P:
	case LibFFmpeg::AV_PIX_FMT_YUYV422:
	case LibFFmpeg::AV_PIX_FMT_UYVY422:
	case LibFFmpeg::AV_PIX_FMT_YVYU422:
	case LibFFmpeg::AV_PIX_FMT_NV12:
	case LibFFmpeg::AV_PIX_FMT_NV21:
		return true;
	default:
		break;
	}

	return false;
}

/**
 * @throws invalid_argument
 * @throws exception
 */
void MediaPlayer::LVP_Player::Open(const std::string &filePath)
{
	if (filePath.empty())
		throw std::invalid_argument("filePath cannot be empty");

	if (!System::LVP_FileSystem::IsMediaFile(filePath))
		throw std::exception("Invalid media file.");

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
 * @throws exception
 */
void MediaPlayer::LVP_Player::openAudioDevice(const SDL_AudioSpec &wantedSpecs)
{
	bool isDefault = (LVP_Player::audioContext.device == "Default");
	bool isEmpty   = LVP_Player::audioContext.device.empty();
	auto newDevice = (!isEmpty && !isDefault ? LVP_Player::audioContext.device.c_str() : NULL);

	if (LVP_Player::audioContext.deviceID >= 2)
		SDL_CloseAudioDevice(LVP_Player::audioContext.deviceID);

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

		throw std::exception(std::format("Failed to open a valid audio device: {}", LVP_Player::audioContext.deviceID).c_str());
	}
}

/**
 * @throws invalid_argument
 * @throws exception
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
		throw std::exception(std::format("Invalid media type: {}", (int)mediaType).c_str());
	}

	auto fileExtension = System::LVP_FileSystem::GetFileExtension(filePath, true);
	auto fileSize      = System::LVP_FileSystem::GetFileSize(filePath);

	if ((fileExtension == "M2TS") && System::LVP_FileSystem::IsBlurayAACS(filePath, fileSize))
		isValidMedia = false;
	else if ((fileExtension == "VOB") && System::LVP_FileSystem::IsDVDCSS(filePath, fileSize))
		isValidMedia = false;

	if (!isValidMedia) {
		FREE_AVFORMAT(LVP_Player::formatContext);
		throw std::exception("Media is DRM encrypted.");
	}

	LVP_Player::state.filePath  = filePath;
	LVP_Player::state.fileSize  = fileSize;
	LVP_Player::state.mediaType = mediaType;

	LVP_Player::subContext.formatContext = LVP_Player::formatContext;
}

/**
 * @throws exception
 */
void MediaPlayer::LVP_Player::openStreams()
{
	LVP_Player::audioContext.index = -1;
	LVP_Player::subContext.index   = -1;
	LVP_Player::videoContext.index = -1;

	// AUDIO TRACK
	LVP_Media::SetMediaTrackBest(LVP_Player::formatContext, LibFFmpeg::AVMEDIA_TYPE_AUDIO, LVP_Player::audioContext);

	if (LVP_Player::audioContext.stream == NULL)
		throw std::exception("Failed to find a valid audio track.");

	LVP_Player::state.duration = LVP_Media::GetMediaDuration(LVP_Player::formatContext, LVP_Player::audioContext.stream);

	// VIDEO TRACK
	LVP_Media::SetMediaTrackBest(LVP_Player::formatContext, LibFFmpeg::AVMEDIA_TYPE_VIDEO, LVP_Player::videoContext);

	if (IS_VIDEO(LVP_Player::state.mediaType))
	{
		if (LVP_Player::videoContext.stream == NULL)
			throw std::exception("Failed to find a valid video track.");

		// SUB TRACK
		LVP_Player::subContext.external = System::LVP_FileSystem::GetSubtitleFilesForVideo(LVP_Player::state.filePath);

		if (!LVP_Player::subContext.external.empty())
			LVP_Player::openSubExternal(SUB_STREAM_EXTERNAL);
		else
			LVP_Media::SetMediaTrackBest(LVP_Player::formatContext, LibFFmpeg::AVMEDIA_TYPE_SUBTITLE, LVP_Player::subContext);
	}

	LVP_Player::state.trackCount = LVP_Player::formatContext->nb_streams;
}

void MediaPlayer::LVP_Player::openSubExternal(int streamIndex)
{
	if (LVP_Player::subContext.external.empty())
		return;

	int fileIndex       = ((streamIndex - SUB_STREAM_EXTERNAL) / SUB_STREAM_EXTERNAL);
	int fileStreamIndex = ((streamIndex - SUB_STREAM_EXTERNAL) % SUB_STREAM_EXTERNAL);

	std::string subFile = LVP_Player::subContext.external[fileIndex];

	FREE_AVFORMAT(LVP_Player::formatContextExternal);

	LVP_Player::formatContextExternal = LVP_Media::GetMediaFormatContext(subFile, true);

	if (LVP_Player::formatContextExternal == NULL)
		return;

	LVP_Media::SetMediaTrackByIndex(LVP_Player::formatContextExternal, fileStreamIndex, LVP_Player::subContext, true);
}

/**
 * @throws exception
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
		throw std::exception(std::format("Failed to create a packets thread: {}", SDL_GetError()).c_str());
}

/**
 * @throws exception
 */
void MediaPlayer::LVP_Player::openThreadAudio()
{
	if ((LVP_Player::audioContext.stream == NULL) || (LVP_Player::audioContext.stream->codecpar == NULL))
		throw std::exception("Audio context stream is missing a valid codec.");

	LVP_Player::audioContext.frame     = LibFFmpeg::av_frame_alloc();
	LVP_Player::audioContext.condition = SDL_CreateCond();
	LVP_Player::audioContext.mutex     = SDL_CreateMutex();
	LVP_Player::audioContext.index     = LVP_Player::audioContext.stream->index;

	if ((LVP_Player::audioContext.frame == NULL) || (LVP_Player::audioContext.condition == NULL) || (LVP_Player::audioContext.mutex == NULL))
		throw std::exception("Failed to allocate an audio context frame.");

	auto channelCount = LVP_Player::audioContext.stream->codecpar->ch_layout.nb_channels;
	auto sampleRate   = LVP_Player::audioContext.stream->codecpar->sample_rate;

	if (channelCount < 1)
		LibFFmpeg::av_channel_layout_default(&LVP_Player::audioContext.stream->codecpar->ch_layout, 2);

	if ((sampleRate < 1) || (channelCount < 1))
		throw std::exception(std::format("Invalid audio: {} channels, {} bps", channelCount, sampleRate).c_str());

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
	LVP_Player::audioContext.frameEncoded    = (uint8_t*)malloc(LVP_Player::audioContext.bufferSize);
	LVP_Player::audioContext.writtenToStream = 0;

	if (LVP_Player::audioContext.frameEncoded == NULL)
		throw std::exception("Failed to allocate an encoded audio context frame.");

	LVP_Player::openThreadAudioFilter();
}

/**
 * @throws exception
 */
void MediaPlayer::LVP_Player::openThreadAudioFilter()
{
	if ((LVP_Player::audioContext.stream == NULL) || (LVP_Player::audioContext.stream->codecpar == NULL))
		return;

	auto abuffer     = LibFFmpeg::avfilter_get_by_name("abuffer");
	auto abuffersink = LibFFmpeg::avfilter_get_by_name("abuffersink");
	auto atempo      = LibFFmpeg::avfilter_get_by_name("atempo");

	auto filterGraph  = LibFFmpeg::avfilter_graph_alloc();
	auto bufferSource = LibFFmpeg::avfilter_graph_alloc_filter(filterGraph, abuffer,     "src");
	auto bufferSink   = LibFFmpeg::avfilter_graph_alloc_filter(filterGraph, abuffersink, "sink");
	auto filterATempo = LibFFmpeg::avfilter_graph_alloc_filter(filterGraph, atempo,      "atempo");

	auto channelLayout = LVP_Media::GetAudioChannelLayout(LVP_Player::audioContext.stream->codecpar->ch_layout);
	auto sampleFormat  = LibFFmpeg::av_get_sample_fmt_name((LibFFmpeg::AVSampleFormat)LVP_Player::audioContext.stream->codecpar->format);
	auto sampleRate    = LVP_Player::audioContext.stream->codecpar->sample_rate;
	auto timeBase      = LVP_Player::audioContext.stream->time_base;

	LibFFmpeg::av_opt_set(bufferSource,     "channel_layout", channelLayout.c_str(), AV_OPT_SEARCH_CHILDREN);
	LibFFmpeg::av_opt_set(bufferSource,     "sample_fmt",     sampleFormat,          AV_OPT_SEARCH_CHILDREN);
	LibFFmpeg::av_opt_set_int(bufferSource, "sample_rate",    sampleRate,            AV_OPT_SEARCH_CHILDREN);
	LibFFmpeg::av_opt_set_q(bufferSource,   "time_base",      timeBase,              AV_OPT_SEARCH_CHILDREN);

	LibFFmpeg::av_opt_set_double(filterATempo, "tempo", LVP_Player::state.playbackSpeed, AV_OPT_SEARCH_CHILDREN);

	LibFFmpeg::avfilter_init_str(bufferSource, NULL);
	LibFFmpeg::avfilter_init_str(bufferSink,   NULL);
	LibFFmpeg::avfilter_init_str(filterATempo, NULL);

	LibFFmpeg::avfilter_link(bufferSource, 0, filterATempo, 0);
	LibFFmpeg::avfilter_link(filterATempo, 0, bufferSink, 0);

	if (LibFFmpeg::avfilter_graph_config(filterGraph, NULL) < 0)
		throw std::exception("Failed to initialize a valid audio filter.");

	LVP_Player::audioFilterLock.lock();

	FREE_AVFILTER_GRAPH(LVP_Player::audioContext.filter.filterGraph);

	LVP_Player::audioContext.filter.filterGraph  = filterGraph;
	LVP_Player::audioContext.filter.bufferSource = bufferSource;
	LVP_Player::audioContext.filter.bufferSink   = bufferSink;

	LVP_Player::audioFilterLock.unlock();
}

/**
 * @throws exception
 */
void MediaPlayer::LVP_Player::openThreadSub()
{
	if ((LVP_Player::subContext.stream == NULL) || (LVP_Player::subContext.stream->codecpar == NULL))
		return;

	// ASS/SSA STYLES

	int  scriptWidth  = 0;
	int  scriptHeight = 0;
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

		scriptWidth  = (posResX != std::string::npos ? std::atoi(subHeader.substr(posResX + 9).c_str()) : 0);
		scriptHeight = (posResY != std::string::npos ? std::atoi(subHeader.substr(posResY + 9).c_str()) : 0);

		if (subHeader.find("[V4+ Styles]") != std::string::npos)
			styleVersion = SUB_STYLE_VERSION_4PLUS_ASS;
		else if (subHeader.find("[V4 Styles]") != std::string::npos)
			styleVersion = SUB_STYLE_VERSION_4_SSA;
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
		throw std::exception(std::format("Failed to create a subtitle context mutex: {}", SDL_GetError()).c_str());

	if (LVP_Player::subContext.subsMutex == NULL) {
		LVP_Player::subContext.subsMutex     = SDL_CreateMutex();
		LVP_Player::subContext.subsCondition = SDL_CreateCond();
	}

	if ((LVP_Player::subContext.subsMutex == NULL) || (LVP_Player::subContext.subsCondition == NULL))
		throw std::exception(std::format("Failed to create a subtitles context mutex: {}", SDL_GetError()).c_str());

	if (LVP_Player::subContext.thread == NULL)
		LVP_Player::subContext.thread = SDL_CreateThread(LVP_Player::threadSub, "subContext.thread", NULL);

	if (LVP_Player::subContext.thread == NULL)
		throw std::exception(std::format("Failed to create a subtitles context thread: {}", SDL_GetError()).c_str());
}

/**
 * @throws exception
 */
void MediaPlayer::LVP_Player::openThreadVideo()
{
	if ((LVP_Player::videoContext.stream == NULL) || (LVP_Player::videoContext.stream->codecpar == NULL))
		throw std::exception("Video context stream is missing a valid codec.");

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
		throw std::exception(std::format("Failed to create a renderer: {}", SDL_GetError()).c_str());

	// VIDEO TARGET TEXTURE

	LVP_Player::renderContext.texture = SDL_CreateTexture(
		LVP_Player::renderContext.renderer,
		SDL_PIXELFORMAT_RGB24,
		SDL_TEXTUREACCESS_TARGET,
		videoWidth,
		videoHeight
	);

	LVP_Player::videoContext.frame     = LibFFmpeg::av_frame_alloc();
	LVP_Player::videoContext.frameRate = (1.0 / LVP_Media::GetMediaFrameRate(LVP_Player::videoContext.stream));

	if ((LVP_Player::videoContext.frame == NULL) || (LVP_Player::videoContext.frameRate <= 0))
		throw std::exception("Failed to allocate a video context frame.");

	if (LVP_Player::videoContext.frameEncoded != NULL)
		LVP_Player::videoContext.frameEncoded->linesize[0] = 0;

	// VIDEO THREAD

	LVP_Player::videoContext.condition = SDL_CreateCond();
	LVP_Player::videoContext.mutex     = SDL_CreateMutex();
	LVP_Player::videoContext.thread    = SDL_CreateThread(LVP_Player::threadVideo, "videoContext.thread", NULL);

	if ((LVP_Player::videoContext.thread == NULL) || (LVP_Player::videoContext.mutex == NULL) || (LVP_Player::videoContext.condition == NULL))
		throw std::exception(std::format("Failed to create a video thread: {}", SDL_GetError()).c_str());

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

void MediaPlayer::LVP_Player::removeExpiredSubs()
{
	for (auto subIter = LVP_Player::subContext.subs.begin(); subIter != LVP_Player::subContext.subs.end();)
	{
		auto sub = *subIter;
		
		if (!sub->isExpired(LVP_Player::subContext.presentTime, LVP_Player::state.progress)) {
			subIter++;
			continue;
		}

		bool isBottomAligned = (IS_SUB_TEXT(LVP_Player::subContext.type) && sub->isAlignedBottom());

		LVP_SubTextRenderer::RemoveSubs(sub->id);
		DELETE_POINTER(sub);

		subIter = LVP_Player::subContext.subs.erase(subIter);

		if (isBottomAligned)
			LVP_SubTextRenderer::RemoveSubsBottom();

		LVP_Player::subContext.isReadyForRender = true;
	}
}

void MediaPlayer::LVP_Player::renderSubs()
{
	if (LVP_Player::subContext.subs.empty() || (LVP_Player::subContext.index < 0) || LVP_Player::state.isStopped || LVP_Player::state.quit)
		return;

	SDL_LockMutex(LVP_Player::subContext.subsMutex);

	if (!LVP_Player::subContext.available)
		SDL_CondWait(LVP_Player::subContext.subsCondition, LVP_Player::subContext.subsMutex);

	if (LVP_Player::seekRequested) {
		LVP_Player::clearSubs();
		SDL_UnlockMutex(LVP_Player::subContext.subsMutex);
		return;
	}

	LVP_Player::removeExpiredSubs();

	LVP_Player::initSubTextures();

	if (!IS_VALID_TEXTURE(LVP_Player::subContext.textureCurrent) || !IS_VALID_TEXTURE(LVP_Player::subContext.textureNext)) {
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

	LVP_SubTextRenderer::Render(LVP_Player::renderContext.renderer, LVP_Player::subContext);

	SDL_SetRenderTarget(LVP_Player::renderContext.renderer, renderTarget);
}

void MediaPlayer::LVP_Player::renderVideo()
{
	auto& frame        = LVP_Player::videoContext.frame;
	auto& frameEncoded = LVP_Player::videoContext.frameEncoded;
	auto& texture      = LVP_Player::videoContext.texture;

	auto pixelFormat = LVP_Player::videoContext.codec->pix_fmt;

	if (!IS_VALID_TEXTURE(texture))
	{
		auto sdlPixelFormat    = LVP_Player::getVideoPixelFormat(pixelFormat);
		auto ffmpegPixelFormat = LVP_Player::getVideoPixelFormat(sdlPixelFormat);

		texture = new Graphics::LVP_Texture(
			sdlPixelFormat,
			SDL_TEXTUREACCESS_STREAMING,
			frame->width,
			frame->height,
			LVP_Player::renderContext.renderer
		);

		// SUB SCALING RELATIVE TO VIDEO

		if ((LVP_Player::subContext.size.x > 0) && (LVP_Player::subContext.size.y > 0))
		{
			LVP_Player::subContext.scale = {
				(float)((float)frame->width  / (float)LVP_Player::subContext.size.x),
				(float)((float)frame->height / (float)LVP_Player::subContext.size.y)
			};
		}

		LVP_Player::subContext.videoDimensions = { 0, 0, frame->width, frame->height };

		if ((pixelFormat == LibFFmpeg::AV_PIX_FMT_YUV420P) || !IS_VALID_TEXTURE(texture))
			return;

		// Create a scaling context for non-YUV420P videos

		frameEncoded = LibFFmpeg::av_frame_alloc();

		int result = LibFFmpeg::av_image_alloc(
			frameEncoded->data,
			frameEncoded->linesize,
			frame->width,
			frame->height,
			ffmpegPixelFormat,
			24
		);

		LVP_Player::renderContext.scaleContextVideo = sws_getCachedContext(
			LVP_Player::renderContext.scaleContextVideo,
			frame->width, frame->height, pixelFormat,
			frame->width, frame->height, ffmpegPixelFormat,
			DEFAULT_SCALE_FILTER, NULL, NULL, NULL
		);
					
		if ((result <= 0) || (LVP_Player::renderContext.scaleContextVideo == NULL)) {
			FREE_AVFRAME(frameEncoded);
			DELETE_POINTER(texture);
		}
	}

	if (!IS_VALID_TEXTURE(texture))
		return;

	// YUV420P VIDEOS - Copy the frame directly to the texture
	if (LVP_Player::isYUV(pixelFormat))
	{
		SDL_UpdateYUVTexture(
			texture->data, NULL,
			frame->data[0], frame->linesize[0],
			frame->data[1], frame->linesize[1],
			frame->data[2], frame->linesize[2]
		);
	}
	// NON-YUV420P VIDEOS - Convert the pixel format to YUV420P
	else
	{
		auto result = sws_scale(
			LVP_Player::renderContext.scaleContextVideo,
			frame->data, frame->linesize, 0, frame->height,
			frameEncoded->data, frameEncoded->linesize
		);

		// Copy the converted frame to the texture
		if (result > 0) {
			SDL_UpdateYUVTexture(
				texture->data, NULL,
				frameEncoded->data[0], frameEncoded->linesize[0],
				frameEncoded->data[1], frameEncoded->linesize[1],
				frameEncoded->data[2], frameEncoded->linesize[2]
			);
		}
	}
}

void MediaPlayer::LVP_Player::present()
{
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

	if (LVP_Player::subContext.textureCurrent != NULL)
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

void MediaPlayer::LVP_Player::Render(const SDL_Rect* destination)
{
	if (LVP_Player::state.quit)
		return;

	if (LVP_Player::videoContext.isReadyForRender) {
		LVP_Player::videoContext.isReadyForRender = false;
		LVP_Player::renderVideo();
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
	LVP_Player::videoContext.isReadyForRender  = true;
	LVP_Player::videoContext.isReadyForPresent = true;

	LVP_Player::subContext.isReadyForRender  = true;
	LVP_Player::subContext.isReadyForPresent = true;
}

void MediaPlayer::LVP_Player::SeekTo(double percent)
{
	if ((LVP_Player::formatContext == NULL) || LVP_Player::seekRequested)
		return;
	
	int64_t position;
	auto    validPercent = max(0.0, min(1.0, percent));

	if (AV_SEEK_BYTES(LVP_Player::formatContext->iformat, LVP_Player::state.fileSize))
		position = (int64_t)((double)LVP_Player::state.fileSize * validPercent);
	else
		position = (int64_t)((double)((double)LVP_Player::state.duration * AV_TIME_BASE_D) * validPercent);

	LVP_Player::seekToPosition(position);
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

void MediaPlayer::LVP_Player::SetPlaybackSpeed(double speed)
{
	auto newSpeed = max(0.5, min(2.0, speed));

	if (ARE_DIFFERENT_DOUBLES(newSpeed, LVP_Player::state.playbackSpeed))
	{
		LVP_Player::state.playbackSpeed = newSpeed;

		LVP_Player::openThreadAudioFilter();
	}
}

/**
 * @throws exception
 */
void MediaPlayer::LVP_Player::SetTrack(const LVP_MediaTrack &track)
{
	auto mediaType  = (LibFFmpeg::AVMediaType)track.mediaType;
	auto trackIndex = track.track;

	if (LVP_Player::formatContext == NULL)
		throw std::exception(std::format("Failed to set track {}:\n- No media has been loaded.", trackIndex).c_str());

	bool isSameAudioTrack = (IS_AUDIO(mediaType) && (trackIndex == LVP_Player::audioContext.index));
	bool isSameSubTrack   = (IS_SUB(mediaType)   && (trackIndex == LVP_Player::subContext.index));

	// Nothing to do, track is already set
	if (isSameAudioTrack || isSameSubTrack)
		return;

	// Disable subs
	if (IS_SUB(mediaType) && (trackIndex < 0)) {
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

		LVP_Media::SetMediaTrackByIndex(LVP_Player::formatContext, trackIndex, LVP_Player::audioContext);

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

		if (trackIndex >= SUB_STREAM_EXTERNAL)
			LVP_Player::openSubExternal(trackIndex);
		else
			LVP_Media::SetMediaTrackByIndex(LVP_Player::formatContext, trackIndex, LVP_Player::subContext);

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
	auto validPercent = max(0.0, min(1.0, percent));
	auto volume       = (int)((double)SDL_MIX_MAXVOLUME * validPercent);

	LVP_Player::audioContext.volume = max(0, min(SDL_MIX_MAXVOLUME, volume));

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

				LVP_Player::audioFilterLock.lock();

				LibFFmpeg::av_buffersrc_add_frame(LVP_Player::audioContext.filter.bufferSource, LVP_Player::audioContext.frame);
				auto filterResult = LibFFmpeg::av_buffersink_get_frame(LVP_Player::audioContext.filter.bufferSink, LVP_Player::audioContext.frame);

				LVP_Player::audioFilterLock.unlock();

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

				auto allocResult = LibFFmpeg::swr_alloc_set_opts2(
					&LVP_Player::audioContext.resampleContext,
					&outChannelLayout, outSampleFormat, outSampleRate,
					&inChannelLayout,  inSampleFormat,  inSampleRate,
					0, NULL
				);

				if ((allocResult < 0) || (LVP_Player::audioContext.resampleContext == NULL)) {
					LVP_Player::stop("Failed to allocate an audio resample context.");
					break;
				}

				if (LibFFmpeg::swr_is_initialized(LVP_Player::audioContext.resampleContext) < 1)
				{
					if (LibFFmpeg::swr_init(LVP_Player::audioContext.resampleContext) < 0) {
						LVP_Player::stop("Failed to initialize the audio resample context.");
						break;
					}
				}

				if (LVP_Player::state.quit)
					break;

				// Calculate needed frame buffer size for resampling
				
				auto bytesPerSample  = (LVP_Player::audioContext.deviceSpecs.channels * LibFFmpeg::av_get_bytes_per_sample(outSampleFormat));
				auto nextSampleCount = LibFFmpeg::swr_get_out_samples(LVP_Player::audioContext.resampleContext, LVP_Player::audioContext.frame->nb_samples);
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
					LVP_Player::audioContext.resampleContext,
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
		// Seek if requested
		if (LVP_Player::seekRequested)
			LVP_Player::handleSeek();

		if (LVP_Player::state.quit)
			break;

		// Initialize a new packet
		auto packet = LibFFmpeg::av_packet_alloc();

		if (packet == NULL) {
			LVP_Player::stop("Failed to allocate packet.");
			break;
		}

		LVP_Player::timeOut->start();

		// Read and fill in the packet
		auto result = LibFFmpeg::av_read_frame(LVP_Player::formatContext, packet);

		if ((result < 0) || LVP_Player::state.quit)
		{
			LVP_Player::timeOut->stop();

			#if defined _DEBUG
				char strerror[AV_ERROR_MAX_STRING_SIZE];
				LibFFmpeg::av_strerror(result, strerror, AV_ERROR_MAX_STRING_SIZE);
				LOG("%s\n", strerror);
			#endif

			if (LVP_Player::state.quit)
				break;

			// Is the media file completed (EOF) or did an error occur?

			if ((result == AVERROR_EOF) && LibFFmpeg::avio_feof(formatContext->pb))
				endOfFile = true;
			else
				errorCount++;

			if (endOfFile || (errorCount >= MAX_ERRORS))
			{
				FREE_AVPACKET(packet);

				// Wait until all packets have been consumed
				while (!LVP_Player::audioContext.packets.empty() && !LVP_Player::state.quit)
					SDL_Delay(DELAY_TIME_DEFAULT);

				if (LVP_Player::state.quit)
					break;

				// Stop media playback
				if (errorCount >= MAX_ERRORS) {
					LVP_Player::stop("Too many errors occured while reading packets.");
					LVP_Player::callbackEvents(LVP_EVENT_MEDIA_COMPLETED_WITH_ERRORS);
				} else {
					LVP_Player::stop();
					LVP_Player::callbackEvents(LVP_EVENT_MEDIA_COMPLETED);
				}

				break;
			}
		} else if (errorCount > 0) {
			errorCount = 0;
		}

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
				int extSubIntIdx = ((LVP_Player::subContext.index - SUB_STREAM_EXTERNAL) % SUB_STREAM_EXTERNAL);

				if ((av_read_frame(LVP_Player::formatContextExternal, packet) == 0) && (packet->stream_index == extSubIntIdx))
					LVP_Player::packetAdd(packet, LVP_Player::subContext);
				else
					FREE_AVPACKET(packet);
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

	LVP_Player::Close();

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

		if ((packet == NULL))
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
				printf("[%.2f,%.2f] %s\n", pts.start, pts.end, subText.c_str());
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

		auto timeUntilPresent = (pts.start - LVP_Player::state.progress);
		auto timeToSleepMs    = (int)(timeUntilPresent * (double)ONE_SECOND_MS);

		LVP_Player::subContext.presentTime = (LVP_Player::state.progress + timeUntilPresent);

		// Sub is ahead of audio, skip or speed up.
		if (timeToSleepMs < 0)
			continue;

		LVP_Player::subContext.isReadyForRender = true;

		// Sub is behind audio, wait or slow down.
		SDL_Delay(min(timeToSleepMs, MAX_SUB_DURATION_MS));

		// Make the subs available for rendering
		LVP_Player::subContext.isReadyForPresent = true;
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
	auto& videoFrame     = LVP_Player::videoContext.frame;
	auto  videoStream    = LVP_Player::videoContext.stream;

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
		result = LibFFmpeg::avcodec_receive_frame(LVP_Player::videoContext.codec, videoFrame);

		FREE_AVPACKET(packet);

		if (LVP_Player::state.quit)
			break;

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
				av_frame_unref(videoFrame);
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
			// Keep seeking until we video and audio catch up.
			if (fabs(LVP_Player::state.progress - LVP_Player::videoContext.pts) > videoFrameDuration2xSeconds)
				continue;

			LVP_Player::seekRequestedPaused = false;

			LVP_Player::videoContext.isReadyForRender  = true;
			LVP_Player::videoContext.isReadyForPresent = true;

			LVP_Player::subContext.isReadyForRender  = true;
			LVP_Player::subContext.isReadyForPresent = true;
		}

		auto sleepTime = (int)((LVP_Player::videoContext.pts - LVP_Player::state.progress) * (double)ONE_SECOND_MS);

		// Video is ahead of audio, skip or speed up.
		if (sleepTime < 0)
			continue;

		LVP_Player::videoContext.isReadyForRender = true;

		// Video is behind audio, wait or slow down.
		SDL_Delay(min(sleepTime, videoFrameDuration2x));

		LVP_Player::videoContext.isReadyForPresent = true;
	}

	return 0;
}

void MediaPlayer::LVP_Player::ToggleMute()
{
	LVP_Player::audioContext.isMuted = !LVP_Player::audioContext.isMuted;

	if (LVP_Player::audioContext.isMuted)
		LVP_Player::callbackEvents(LVP_EVENT_AUDIO_MUTED);
	else
		LVP_Player::callbackEvents(LVP_EVENT_AUDIO_UNMUTED);
}

void MediaPlayer::LVP_Player::TogglePause()
{
	if (!LVP_Player::state.isPaused)
		LVP_Player::pause();
	else
		LVP_Player::play();
}
