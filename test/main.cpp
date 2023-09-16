#define SDL_MAIN_HANDLED

#include "TestPlayer.h"
#include "TestWindow.h"

const int MS_PER_FRAME_FPS60 = (1000 / 60);
const int MS_PER_FRAME_IDLE  = 200;

bool QUIT = false;

void handleOpenFile()
{
    #if defined _linux || defined _macosx || defined _windows
        auto file = TestWindow::OpenFile();
    #else
        auto file = std::string("");
    #endif

    if (!file.empty())
        LVP_Open(file);
}

void handleDropFileEvent(const SDL_Event &event)
{
    LVP_Open(event.drop.file);
    SDL_free(event.drop.file);
}

void handleKeyDownEvent(const SDL_KeyboardEvent &event)
{
    if (LVP_IsStopped())
        return;

    switch (event.keysym.sym) {
    case SDLK_LEFT: case SDLK_AUDIOREWIND:
        TestPlayer::SeekBack();
        break;
    case SDLK_RIGHT: case SDLK_AUDIOFASTFORWARD:
        TestPlayer::SeekForward();
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
    case SDLK_s: case SDLK_AUDIOSTOP:
        LVP_Stop();
        break;
    case SDLK_SPACE: case SDLK_AUDIOPLAY:
        LVP_TogglePause();
        break;
    default:
        break;
    }
}

void handleMouseUpEvent(const SDL_MouseButtonEvent& event)
{
    SDL_Point clickPosition = { event.x, event.y };
    auto      button        = TestWindow::GetClickedButton(clickPosition);

    if (!button)
        return;

    switch (button->id) {
	case BUTTON_ID_OPEN:
        if (LVP_IsStopped())
            handleOpenFile();
        else
            LVP_TogglePause();
        break;
	case BUTTON_ID_SEEK_BACK:
        TestPlayer::SeekBack();
        break;
	case BUTTON_ID_SEEK_FORWARD:
        TestPlayer::SeekForward();
        break;
    case BUTTON_ID_STOP:
        LVP_Stop();
        break;
    default:
		break;
	}
}

void handleUserEvent(const SDL_UserEvent &event)
{
    auto eventType = (LVP_EventType)event.code;

    switch (eventType) {
    case LVP_EVENT_MEDIA_OPENED:
        TestWindow::UpdateButton(BUTTON_ID_OPEN, "PAUSE");

        TestWindow::EnableButton(BUTTON_ID_SEEK_BACK,    true);
        TestWindow::EnableButton(BUTTON_ID_SEEK_FORWARD, true);
        TestWindow::EnableButton(BUTTON_ID_STOP,         true);
        break;
    case LVP_EVENT_MEDIA_PAUSED:
        TestWindow::UpdateButton(BUTTON_ID_OPEN, "PLAY");
        break;
    case LVP_EVENT_MEDIA_PLAYING:
        TestWindow::UpdateButton(BUTTON_ID_OPEN, "PAUSE");
        break;
    case LVP_EVENT_MEDIA_STOPPED:
        TestWindow::UpdateButton(BUTTON_ID_OPEN,     "OPEN");
        TestWindow::UpdateButton(BUTTON_ID_PROGRESS, "00:00:00 / 00:00:00");

        TestWindow::EnableButton(BUTTON_ID_SEEK_BACK,    false);
        TestWindow::EnableButton(BUTTON_ID_SEEK_FORWARD, false);
        TestWindow::EnableButton(BUTTON_ID_STOP,         false);
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
        LVP_Resize();
        break;
	default:
        break;
    }
}

int getSleepTime(uint32_t frameStart)
{
    auto timeToRender = (int)(SDL_GetTicks() - frameStart);
    bool use60FPS     = (LVP_IsPlaying() && (LVP_GetMediaType() == LVP_MEDIA_TYPE_VIDEO));
    auto timePerFrame = (use60FPS ? MS_PER_FRAME_FPS60 : MS_PER_FRAME_IDLE);
    auto sleepTime    = (timePerFrame - timeToRender);

    return sleepTime;
}

void handleEvents()
{
    SDL_Event event = {};

    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
            QUIT = true;
            break;
        case SDL_DROPFILE:
            handleDropFileEvent(event);
            break;
        case SDL_KEYDOWN:
            handleKeyDownEvent(event.key);
            break;
        case SDL_KEYUP:
            handleKeyUpEvent(event.key);
            break;
        case SDL_MOUSEBUTTONUP:
            handleMouseUpEvent(event.button);
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
}

void quit() {
    TestPlayer::Quit();
    TestWindow::Quit();
}

void render()
{
    const int CONTROLS_HEIGHT = 34;

    bool     isPlayerActive = !LVP_IsStopped();
    auto     renderer       = TestWindow::GetRenderer();
    auto     window         = TestWindow::GetDimensions();
    SDL_Rect player         = { 0, 0, window.w, (window.h - CONTROLS_HEIGHT) };
    SDL_Rect controls       = { 0, (window.h - CONTROLS_HEIGHT), window.w, CONTROLS_HEIGHT };

    SDL_SetRenderTarget(renderer, nullptr);

    if (isPlayerActive)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    else
        SDL_SetRenderDrawColor(renderer, 32, 32, 32, 0xFF);

    SDL_RenderClear(renderer);

    if (isPlayerActive)
        TestPlayer::Render(renderer, player);

    TestWindow::RenderControls(controls);

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
    try
    {
        init();

        while (!QUIT)
        {
            auto frameStart = SDL_GetTicks();

            handleEvents();
            TestWindow::UpdateProgress();

            if (QUIT)
                break;

            render();

            auto sleepTime = getSleepTime(frameStart);

            if (sleepTime > 0)
                SDL_Delay(sleepTime);
        }

        quit();
    }
    catch (const std::exception& e)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "testvoyaplayer", e.what(), NULL);
        quit();

        return 1;
    }

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
