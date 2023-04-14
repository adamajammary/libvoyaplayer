#define SDL_MAIN_HANDLED

#include "TestPlayer.h"
#include "TestWindow.h"

bool QUIT = false;

void handleOpenUI()
{
    TestWindow::InitSubMenuAudioTracks(LVP_GetAudioTracks());
    TestWindow::InitSubMenuSubtitleTracks(LVP_GetSubtitleTracks());
    TestWindow::InitSubMenuChapters(LVP_GetChapters());

    auto audioTrack    = LVP_GetAudioTrack();
    auto subtitleTrack = LVP_GetSubtitleTrack();

    TestWindow::ToggleMenuChecked(TestWindow::GetAudioTrackId(audioTrack),       MENU_AUDIO_TRACK,    MENU_ITEM_AUDIO_TRACK1);
    TestWindow::ToggleMenuChecked(TestWindow::GetSubtitleTrackId(subtitleTrack), MENU_SUBTITLE_TRACK, MENU_ITEM_SUBTITLE_TRACK1);
}

void handleOpenFile()
{
    auto file = TestWindow::OpenFile();

    if (!file.empty()) {
        LVP_Open(file);
        handleOpenUI();
    }
}

void handleSeekToChapter(unsigned short id, Menu menu, MenuItem firstItem)
{
    TestPlayer::SeekToChapter(TestWindow::GetChapter(id));
    TestWindow::ToggleMenuChecked(id, menu, firstItem);
}

void handleSetAudioDevice(unsigned short id, Menu menu, MenuItem firstItem)
{
    auto device = TestWindow::GetMenuLabel(id, menu);

    if (LVP_SetAudioDevice(device))
        TestWindow::ToggleMenuChecked(id, menu, firstItem);
}

void handleSetPlaybackSpeed(unsigned short id)
{
    auto speedLabel = TestWindow::GetMenuLabel(id, MENU_PLAYBACK_SPEED);
    auto speed      = std::atof(speedLabel.c_str());

    LVP_SetPlaybackSpeed(speed);
    TestWindow::ToggleMenuChecked(id, MENU_PLAYBACK_SPEED, MENU_ITEM_PLAYBACK_SPEED_050X);
}

void handleSetPlaybackSpeed()
{
    auto speed = LVP_GetPlaybackSpeed();
    auto id    = TestWindow::GetMenuIdPlaybackSpeed(speed);

    TestWindow::ToggleMenuChecked(id, MENU_PLAYBACK_SPEED, MENU_ITEM_PLAYBACK_SPEED_050X);
}

void handleSetTrack(LVP_MediaTrack track, unsigned short id, Menu menu, MenuItem firstItem)
{
    LVP_SetTrack(track);
    TestWindow::ToggleMenuChecked(id, menu, firstItem);
}

void handleStop()
{
    LVP_Stop();
    TestWindow::ToggleMenuChecked(MENU_ITEM_PLAYBACK_SPEED_100X, MENU_PLAYBACK_SPEED, MENU_ITEM_PLAYBACK_SPEED_050X);
}

void handleDropFileEvent(const SDL_Event &event)
{
    LVP_Open(event.drop.file);
    SDL_free(event.drop.file);

    handleOpenUI();
}

void handleKeyDownEvent(const SDL_KeyboardEvent &event)
{
    if (LVP_IsStopped())
        return;

    switch (event.keysym.sym) {
    case SDLK_MINUS: case SDLK_KP_MINUS:
        TestPlayer::SetPlaybackSpeedDown();
        handleSetPlaybackSpeed();
        break;
    case SDLK_PLUS: case SDLK_KP_PLUS:
        TestPlayer::SetPlaybackSpeedUp();
        handleSetPlaybackSpeed();
        break;
    case SDLK_LEFT: case SDLK_AUDIOREWIND:
        TestPlayer::SeekBack();
        break;
    case SDLK_RIGHT: case SDLK_AUDIOFASTFORWARD:
        TestPlayer::SeekForward();
        break;
    case SDLK_DOWN:
        TestPlayer::SetVolumeDown();
        break;
    case SDLK_UP:
        TestPlayer::SetVolumeUp();
        break;
    default:
        break;
    }
}

void handleKeyUpEvent(const SDL_KeyboardEvent &event)
{
    // https://wiki.libsdl.org/SDL2/SDL_Keymod
    if (event.keysym.mod & (KMOD_LCTRL | KMOD_RCTRL))
    {
        switch (event.keysym.sym) {
        case SDLK_o:
            handleOpenFile();
            break;
        case SDLK_q:
            QUIT = true; 
            break;
        default:
            break;
        }

        return;
    }

    if (LVP_IsStopped())
        return;

    switch (event.keysym.sym) {
    case SDLK_m:
        LVP_ToggleMute();
        break;
    case SDLK_s: case SDLK_AUDIOSTOP:
        handleStop();
        break;
    case SDLK_SPACE: case SDLK_AUDIOPLAY:
        LVP_TogglePause();
        break;
    default:
        break;
    }
}

void handleMouseScrollEvent(const SDL_MouseWheelEvent &event)
{
    if (LVP_IsStopped())
        return;

    int scrollDirection = (event.direction == SDL_MOUSEWHEEL_FLIPPED ? event.y * -1 : event.y);

    if (scrollDirection > 0)
        TestPlayer::SetVolumeUp();
    else
        TestPlayer::SetVolumeDown();
}

#if defined _windows
void handleSystemCommandEvent(SDL_SysWMmsg* msg)
{
    auto id = LOWORD(msg->msg.win.wParam);

    switch (id) {
    case MENU_ITEM_AUDIO_MUTE: case CONTROL_ITEM_MUTE:
        LVP_ToggleMute();
        break;
    case MENU_ITEM_AUDIO_VOLUME_DOWN:
        TestPlayer::SetVolumeDown();
        break;
    case MENU_ITEM_AUDIO_VOLUME_UP:
        TestPlayer::SetVolumeUp();
        break;
    case MENU_ITEM_FILE_OPEN:
        handleOpenFile();
        break;
    case MENU_ITEM_FILE_QUIT:
        QUIT = true;
        break;
    case MENU_ITEM_HELP_ABOUT:
        TestWindow::ShowAbout();
        break;
    case MENU_ITEM_PLAYBACK_PAUSE: case CONTROL_ITEM_PAUSE:
        LVP_TogglePause();
        break;
    case MENU_ITEM_PLAYBACK_SEEK_BACK:
        TestPlayer::SeekBack();
        break;
    case MENU_ITEM_PLAYBACK_SEEK_FORWARD:
        TestPlayer::SeekForward();
        break;
    case MENU_ITEM_PLAYBACK_STOP: case CONTROL_ITEM_STOP:
        handleStop();
        break;
    default:
        if (id >= MENU_ITEM_PLAYBACK_CHAPTER1)
            handleSeekToChapter(id, MENU_PLAYBACK_CHAPTER, MENU_ITEM_PLAYBACK_CHAPTER1);
        else if (id >= MENU_ITEM_SUBTITLE_TRACK1)
            handleSetTrack(TestWindow::GetSubtitleTrack(id), id, MENU_SUBTITLE_TRACK, MENU_ITEM_SUBTITLE_TRACK1);
        else if (id >= MENU_ITEM_AUDIO_TRACK1)
            handleSetTrack(TestWindow::GetAudioTrack(id), id, MENU_AUDIO_TRACK, MENU_ITEM_AUDIO_TRACK1);
        else if (id >= MENU_ITEM_AUDIO_DEVICE1)
            handleSetAudioDevice(id, MENU_AUDIO_DEVICE, MENU_ITEM_AUDIO_DEVICE1);
        else if (id >= MENU_ITEM_PLAYBACK_SPEED_050X)
            handleSetPlaybackSpeed(id);
        break;
    }

    SetFocus(msg->msg.win.hwnd);
}

void handleSystemSliderEvent(SDL_SysWMmsg* msg)
{
    auto scrollType = LOWORD(msg->msg.win.wParam);
    auto controlId  = TestWindow::GetControlId((HWND)msg->msg.win.lParam);

    switch (scrollType) {
    case TB_PAGEDOWN: case TB_PAGEUP:
        switch (controlId) {
        case CONTROL_ITEM_SEEK:
            LVP_SeekTo(TestWindow::GetControlSliderClickPosition(CONTROL_SEEK));
            break;
        case CONTROL_ITEM_VOLUME:
            LVP_SetVolume(TestWindow::GetControlSliderClickPosition(CONTROL_VOLUME));
            break;
        default:
            break;
        }
        break;
    case TB_THUMBTRACK:
        switch (controlId) {
        case CONTROL_ITEM_SEEK:
            LVP_SeekTo((double)HIWORD(msg->msg.win.wParam) * 0.01);
            break;
        case CONTROL_ITEM_VOLUME:
            LVP_SetVolume((double)HIWORD(msg->msg.win.wParam) * 0.01);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    SetFocus(msg->msg.win.hwnd);
}
#endif

void handleSystemEvent(const SDL_SysWMEvent &event)
{
    #if defined _windows
        switch (event.msg->msg.win.msg) {
        case WM_COMMAND:
            handleSystemCommandEvent(event.msg);
            break;
        case WM_HSCROLL: case WM_VSCROLL:
            handleSystemSliderEvent(event.msg);
            break;
        case WM_DESTROY: case WM_QUERYENDSESSION: case WM_ENDSESSION:
            QUIT = true;
            break;
        default:
            break;
        }
    #endif
}

void handleUserEvent(const SDL_UserEvent &event)
{
    auto eventType = (LVP_EventType)event.code;

    switch (eventType) {
    case LVP_EVENT_MEDIA_TRACKS_UPDATED: case LVP_EVENT_METADATA_UPDATED:
        handleOpenUI();
        break;
    default:
        break;
    }
}

void handleWindowEvent(const SDL_WindowEvent &event)
{
    switch (event.event) {
        case SDL_WINDOWEVENT_CLOSE:
            QUIT = true;
            break;
        case SDL_WINDOWEVENT_MOVED: case SDL_WINDOWEVENT_SIZE_CHANGED:
            TestWindow::Resize();
            LVP_Resize();
            break;
		default:
            break;
    }
}

void handleEvents()
{
    SDL_Event event = {};

    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_DROPFILE:
            handleDropFileEvent(event);
            break;
        case SDL_KEYDOWN:
            handleKeyDownEvent(event.key);
            break;
        case SDL_KEYUP:
            handleKeyUpEvent(event.key);
            break;
        case SDL_MOUSEWHEEL:
            handleMouseScrollEvent(event.wheel);
            break;
        case SDL_SYSWMEVENT:
            handleSystemEvent(event.syswm);
            break;
        case SDL_WINDOWEVENT:
            handleWindowEvent(event.window);
            break;
        default:
            if (event.type >= SDL_USEREVENT)
                handleUserEvent(event.user);
            break;
        }
    }
}

void init() {
    TestWindow::Init(800, 600);
    TestPlayer::Init(TestWindow::GetRenderer());

    TestWindow::InitSubMenuItems(LVP_GetAudioDevices(), MENU_AUDIO_DEVICE, MENU_ITEM_AUDIO_DEVICE1, MENU_LABEL_AUDIO_DEVICE_DEFAULT);
}

void quit() {
    TestPlayer::Quit();
    TestWindow::Quit();
}

void render()
{
    bool     isPlayerActive = !LVP_IsStopped();
    auto     renderer       = TestWindow::GetRenderer();
    auto     window         = TestWindow::GetDimensions();
    SDL_Rect player         = { 0, 0, window.w, window.h - PLAYER_CONTROLS_PANEL_HEIGHT };

    SDL_SetRenderTarget(renderer, nullptr);

    if (isPlayerActive)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    else
        SDL_SetRenderDrawColor(renderer, 32, 32, 32, 0xFF);

    SDL_RenderClear(renderer);

    if (isPlayerActive)
        TestPlayer::Render(renderer, player);

    SDL_RenderPresent(renderer);
}
#if defined _windows && defined _DEBUG
int wmain(int argc, wchar_t* argv[])
#elif defined _windows && defined NDEBUG
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int SDL_main(int argc, char* argv[])
#endif
{
    try {
        init();
    } catch (const std::exception &e) {
        std::fprintf(stderr, e.what());

        quit();

        return 1;
    }

    const int MS_PER_FRAME_FPS60 = (1000 / 60);
    const int MS_PER_FRAME_IDLE  = 200;

    auto startTime = SDL_GetTicks();

    while (!QUIT)
    {
        auto frameStart = SDL_GetTicks();

        handleEvents();

        auto deltaTime = (SDL_GetTicks() - startTime);

        TestWindow::UpdateUI(deltaTime);

        if (deltaTime >= UI_UDPATE_RATE_MS)
            startTime = SDL_GetTicks();

        if (QUIT)
            break;

        render();

        auto timeToRenderFrame = (SDL_GetTicks() - frameStart);
        bool use60FPS          = (LVP_IsPlaying() && (LVP_GetMediaType() == LVP_MEDIA_TYPE_VIDEO));
        auto timePerFrame      = (use60FPS ? MS_PER_FRAME_FPS60 : MS_PER_FRAME_IDLE);
        auto sleepTime         = (int)(timePerFrame - (int)timeToRenderFrame);

        if (sleepTime > 0)
            SDL_Delay(sleepTime);
    }

    quit();

    return 0;
}

#if !defined _windows
#if defined __cplusplus
extern "C"
#endif
int main(int argc, char* argv[])
{
#if defined _ios
	return SDL_UIKitRunApp(argc, argv, SDL_main);
#else
	return SDL_main(argc, argv);
#endif
}
#endif
