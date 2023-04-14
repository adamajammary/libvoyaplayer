#include "TestPlayer.h"

SDL_Texture* TestPlayer::texture = nullptr;
SDL_Surface* TestPlayer::videoFrame = nullptr;
bool         TestPlayer::videoIsAvailable = false;
std::mutex   TestPlayer::videoLock;
bool         TestPlayer::useHardwareRenderer = true;

void TestPlayer::freeResources()
{
	TestPlayer::videoLock.lock();

	if (TestPlayer::texture) {
		SDL_DestroyTexture(TestPlayer::texture);
		TestPlayer::texture = nullptr;
	}

	if (TestPlayer::videoFrame) {
		SDL_FreeSurface(TestPlayer::videoFrame);
		TestPlayer::videoFrame = nullptr;
	}

	TestPlayer::videoLock.unlock();

}

void TestPlayer::handleError(const std::string &errorMessage, const void* data)
{
	fprintf(stderr, "%s\n", errorMessage.c_str());
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "testvoyaplayer", errorMessage.c_str(), NULL);
}

void TestPlayer::handleEvent(LVP_EventType type, const void* data)
{
	switch (type) {
	case LVP_EVENT_AUDIO_MUTED:
		printf("LVP_EVENT_AUDIO_MUTED\n");
		break;
	case LVP_EVENT_AUDIO_UNMUTED:
		printf("LVP_EVENT_AUDIO_UNMUTED\n");
		break;
	case LVP_EVENT_AUDIO_VOLUME_CHANGED:
		printf("LVP_EVENT_AUDIO_VOLUME_CHANGED\n");
		break;
	case LVP_EVENT_MEDIA_COMPLETED:
		printf("LVP_EVENT_MEDIA_COMPLETED\n");
		break;
	case LVP_EVENT_MEDIA_COMPLETED_WITH_ERRORS:
		printf("LVP_EVENT_MEDIA_COMPLETED_WITH_ERRORS\n");
		break;
	case LVP_EVENT_MEDIA_PAUSED:
		printf("LVP_EVENT_MEDIA_PAUSED\n");
		break;
	case LVP_EVENT_MEDIA_PLAYING:
		printf("LVP_EVENT_MEDIA_PLAYING\n");
		break;
	case LVP_EVENT_MEDIA_STOPPED:
		printf("LVP_EVENT_MEDIA_STOPPED\n");
		break;
	case LVP_EVENT_MEDIA_STOPPING:
		TestPlayer::freeResources();
		return;
	case LVP_EVENT_MEDIA_TRACKS_UPDATED:
		printf("LVP_EVENT_MEDIA_TRACKS_UPDATED\n");
		break;
	case LVP_EVENT_METADATA_UPDATED:
		printf("LVP_EVENT_METADATA_UPDATED\n");
		break;
	default:
		break;
	}

	SDL_Event event = {};

	event.type       = SDL_RegisterEvents(1);
	event.user.code  = (Sint32)type;
	event.user.data1 = (void*)data;

	SDL_PushEvent(&event);
}

void TestPlayer::handleVideoIsAvailable(SDL_Surface* videoFrame, const void* data)
{
	if (TestPlayer::useHardwareRenderer)
		return;

	TestPlayer::videoLock.lock();

	if (TestPlayer::videoFrame)
		SDL_FreeSurface(TestPlayer::videoFrame);

	TestPlayer::videoFrame = videoFrame;
	TestPlayer::videoIsAvailable = true;

	TestPlayer::videoLock.unlock();
}

void TestPlayer::Init(SDL_Renderer* renderer, const void* data)
{
	LVP_CallbackContext callbackContext = {
		.errorCB = handleError,
		.eventsCB = handleEvent,
		.videoCB = handleVideoIsAvailable,
		.data = data,
		.hardwareRenderer = (TestPlayer::useHardwareRenderer ? renderer : nullptr)
	};

	LVP_Initialize(callbackContext);
}

void TestPlayer::Quit()
{
	LVP_Quit();

	TestPlayer::freeResources();
}

void TestPlayer::Render(SDL_Renderer* renderer, const SDL_Rect &destination)
{
	// HARDWARE RENDERER
	if (TestPlayer::useHardwareRenderer) {
		LVP_Render(&destination);
		return;
	}

	// SOFTWARE RENDERER
	LVP_Render();

	if (!renderer || !TestPlayer::videoFrame)
		return;

	TestPlayer::videoLock.lock();

	if (!TestPlayer::texture && TestPlayer::videoFrame)
	{
		TestPlayer::texture = SDL_CreateTexture(
			renderer,
			TestPlayer::videoFrame->format->format,
			SDL_TEXTUREACCESS_STREAMING,
			TestPlayer::videoFrame->w,
			TestPlayer::videoFrame->h
		);

		if (!TestPlayer::texture)
			fprintf(stderr, "%s\n", SDL_GetError());
	}

	if (TestPlayer::videoIsAvailable && TestPlayer::texture && TestPlayer::videoFrame)
	{
		TestPlayer::videoIsAvailable = false;

		if (SDL_UpdateTexture(TestPlayer::texture, nullptr, TestPlayer::videoFrame->pixels, TestPlayer::videoFrame->pitch) < 0)
			fprintf(stderr, "%s\n", SDL_GetError());
	}

	if (TestPlayer::texture)
	{
		auto maxWidth     = (int)((double)TestPlayer::videoFrame->w / (double)TestPlayer::videoFrame->h * (double)destination.h);
		auto maxHeight    = (int)((double)TestPlayer::videoFrame->h / (double)TestPlayer::videoFrame->w * (double)destination.w);
		auto scaledWidth  = std::min(destination.w, maxWidth);
		auto scaledHeight = std::min(destination.h, maxHeight);

		SDL_Rect scaledDestination = {
			(destination.x + ((destination.w - scaledWidth)  / 2)),
			(destination.y + ((destination.h - scaledHeight) / 2)),
			scaledWidth,
			scaledHeight
		};

		if (SDL_RenderCopy(renderer, TestPlayer::texture, nullptr, &scaledDestination) < 0)
			fprintf(stderr, "%s\n", SDL_GetError());
	}

	TestPlayer::videoLock.unlock();
}

void TestPlayer::SeekBack()
{
	auto duration = (double)LVP_GetDuration();
	auto progress = (double)(LVP_GetProgress() - SEEK_DIFF);
	auto percent  = (progress / duration);

	LVP_SeekTo(percent);
}

void TestPlayer::SeekForward()
{
	auto duration = (double)LVP_GetDuration();
	auto progress = (double)(LVP_GetProgress() + SEEK_DIFF);
	auto percent  = (progress / duration);

	LVP_SeekTo(percent);
}

void TestPlayer::SeekToChapter(const LVP_MediaChapter &chapter)
{
	auto duration = (double)LVP_GetDuration();
	auto position = (double)chapter.startTime;
	auto percent  = (position / duration);

	LVP_SeekTo(percent);
}

void TestPlayer::SetPlaybackSpeedDown()
{
	auto speed = LVP_GetPlaybackSpeed();

	LVP_SetPlaybackSpeed(speed - SPEED_DIFF);
}

void TestPlayer::SetPlaybackSpeedUp()
{
	auto speed = LVP_GetPlaybackSpeed();

	LVP_SetPlaybackSpeed(speed + SPEED_DIFF);
}

void TestPlayer::SetVolumeDown()
{
	auto volume = LVP_GetVolume();

	LVP_SetVolume(volume - VOLUME_DIFF);
}

void TestPlayer::SetVolumeUp()
{
	auto volume = LVP_GetVolume();

	LVP_SetVolume(volume + VOLUME_DIFF);
}
