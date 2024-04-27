#define SDL_MAIN_HANDLED

#include "TestPlayer.h"
#include "TestWindow.h"

const int MS_PER_FRAME_FPS60 = (1000 / 60);
const int MS_PER_FRAME_IDLE  = 200;

char*       BASE_PATH  = nullptr;
bool        QUIT       = false;
const char* VIDEO_FILE = "caminandes_1_llama_drama_2013_300p.ogv";

#if defined _android
static jclass getAndroidJniClass(const std::string& classPath, JNIEnv* environment)
{
    auto jniClass = environment->FindClass(classPath.c_str());

    if (!jniClass)
        throw std::runtime_error(TextFormat("Failed to find Android JNI class: '%s'", classPath.c_str()));

    return jniClass;
}

static JNIEnv* getAndroidJniEnvironment()
{
    auto jniEnvironment = (JNIEnv*)SDL_AndroidGetJNIEnv();

    if (!jniEnvironment)
        throw std::runtime_error("Failed to get a valid Android JNI Environment.");

    return jniEnvironment;
}

static AAssetManager* getAndroidJniAssetManager()
{
	auto jniEnvironment    = getAndroidJniEnvironment();
	auto jniObjectActivity = (jobject)SDL_AndroidGetActivity();

	if (!jniObjectActivity)
		throw std::runtime_error("Failed to get a valid Android JNI Activity.");

	auto jniClassActivity  = getAndroidJniClass("android/app/Activity",          jniEnvironment);
	auto jniClassResources = getAndroidJniClass("android/content/res/Resources", jniEnvironment);

	auto jniMethodGetAssets    = jniEnvironment->GetMethodID(jniClassResources, "getAssets",    "()Landroid/content/res/AssetManager;");
	auto jniMethodGetResources = jniEnvironment->GetMethodID(jniClassActivity,  "getResources", "()Landroid/content/res/Resources;");

	auto jniObjectResources    = jniEnvironment->CallObjectMethod(jniObjectActivity,  jniMethodGetResources);
	auto jniObjectAssetManager = jniEnvironment->CallObjectMethod(jniObjectResources, jniMethodGetAssets);

	auto jniAssetManager = AAssetManager_fromJava(jniEnvironment, jniObjectAssetManager);

	if (!jniAssetManager)
		throw std::runtime_error("Failed to get a valid Android JNI Asset Manager.");

	jniEnvironment->DeleteLocalRef(jniObjectAssetManager);
	jniEnvironment->DeleteLocalRef(jniObjectResources);
	jniEnvironment->DeleteLocalRef(jniObjectActivity);
	jniEnvironment->DeleteLocalRef(jniClassResources);
	jniEnvironment->DeleteLocalRef(jniClassActivity);

	return jniAssetManager;
}

static void initBasePath()
{
    BASE_PATH = SDL_GetPrefPath(nullptr, nullptr);

	if (!BASE_PATH)
		throw std::runtime_error("Failed to get an app-specific location where files can be written.");

    auto jniAssetManager = getAndroidJniAssetManager();
	auto videoAsset      = AAssetManager_open(jniAssetManager, VIDEO_FILE, AASSET_MODE_STREAMING);

	if (!videoAsset)
		throw std::runtime_error(TextFormat("Failed to open asset: %s", VIDEO_FILE));

	auto destinationPath = TextFormat("%s%s", BASE_PATH, VIDEO_FILE);
	auto destinationFile = SDL_RWFromFile(destinationPath.c_str(), "w");

	if (!destinationFile)
		throw std::runtime_error(TextFormat("Failed to write file '%s': %s", destinationPath.c_str(), SDL_GetError()));

	char destinationBuffer[BUFSIZ] = {};
	int  fileReadSize = 0;

	while ((fileReadSize = AAsset_read(videoAsset, destinationBuffer, BUFSIZ)) > 0)
		SDL_RWwrite(destinationFile, destinationBuffer, fileReadSize, 1);

	SDL_RWclose(destinationFile);
	AAsset_close(videoAsset);
}
#else
static void initBasePath()
{
    BASE_PATH = SDL_GetBasePath();

    if (!BASE_PATH)
        throw std::runtime_error("Failed to get the runtime location.");
}
#endif

static void openVideo()
{
    LVP_Open(TextFormat("%s%s", BASE_PATH, VIDEO_FILE));
}

static void handleKeyDownEvent(const SDL_KeyboardEvent &event)
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

static void handleKeyUpEvent(const SDL_KeyboardEvent &event)
{
    switch (event.keysym.sym) {
    case SDLK_SPACE: case SDLK_AUDIOPLAY:
        if (LVP_IsStopped())
            openVideo();
        else
            LVP_TogglePause();
        break;
    case SDLK_s: case SDLK_AUDIOSTOP:
        if (!LVP_IsStopped())
            LVP_Stop();
        break;
    default:
        break;
    }
}

static void handleMouseUpEvent(const SDL_MouseButtonEvent& event)
{
    auto button = TestWindow::GetClickedButton(event);

    if (!button)
        return;

    switch (button->id) {
	case BUTTON_ID_PLAY_PAUSE:
        if (LVP_IsStopped())
            openVideo();
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

static void handleUserEvent(const SDL_UserEvent &event)
{
    auto eventType = (LVP_EventType)event.code;

    switch (eventType) {
    case LVP_EVENT_MEDIA_OPENED:
        TestWindow::UpdateTitle("Caminandes 1: Llama Drama (2013)");

        TestWindow::UpdateButton(BUTTON_ID_PLAY_PAUSE, "PAUSE");

        TestWindow::EnableButton(BUTTON_ID_SEEK_BACK,    true);
        TestWindow::EnableButton(BUTTON_ID_SEEK_FORWARD, true);
        TestWindow::EnableButton(BUTTON_ID_STOP,         true);
        break;
    case LVP_EVENT_MEDIA_PAUSED:
        TestWindow::UpdateButton(BUTTON_ID_PLAY_PAUSE, "PLAY");
        break;
    case LVP_EVENT_MEDIA_PLAYING:
        TestWindow::UpdateButton(BUTTON_ID_PLAY_PAUSE, "PAUSE");
        break;
    case LVP_EVENT_MEDIA_STOPPED:
        TestWindow::UpdateTitle();

        TestWindow::UpdateButton(BUTTON_ID_PLAY_PAUSE, "PLAY");
        TestWindow::UpdateButton(BUTTON_ID_PROGRESS,   "00:00:00 / 00:00:00");

        TestWindow::EnableButton(BUTTON_ID_SEEK_BACK,    false);
        TestWindow::EnableButton(BUTTON_ID_SEEK_FORWARD, false);
        TestWindow::EnableButton(BUTTON_ID_STOP,         false);
        break;
    default:
        break;
    }
}

static void handleWindowEvent(const SDL_WindowEvent &event)
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

static int getSleepTime(uint32_t frameStart)
{
    auto timeToRender = (int)(SDL_GetTicks() - frameStart);
    bool use60FPS     = (LVP_IsPlaying() && (LVP_GetMediaType() == LVP_MEDIA_TYPE_VIDEO));
    auto timePerFrame = (use60FPS ? MS_PER_FRAME_FPS60 : MS_PER_FRAME_IDLE);
    auto sleepTime    = (timePerFrame - timeToRender);

    return sleepTime;
}

static void handleEvents()
{
    SDL_Event event = {};

    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
            QUIT = true;
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

static void init() {
    initBasePath();

    TestWindow::Init(800, 600, BASE_PATH);
    TestPlayer::Init(TestWindow::GetRenderer());

    openVideo();
}

static void quit() {
    TestPlayer::Quit();
    TestWindow::Quit();
}

static void render()
{
    bool isPlayerActive = !LVP_IsStopped();
    auto renderer       = TestWindow::GetRenderer();
    auto window         = TestWindow::GetDimensions();
    auto windowDPIScale = TestWindow::GetDPIScale();

    const auto CONTROLS_HEIGHT = (int)(40.0f * windowDPIScale);

    SDL_Rect player   = { 0, 0, window.w, (window.h - CONTROLS_HEIGHT) };
    SDL_Rect controls = { 0, (window.h - CONTROLS_HEIGHT), window.w, CONTROLS_HEIGHT };

    SDL_SetRenderTarget(renderer, nullptr);

    if (isPlayerActive)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    else
        SDL_SetRenderDrawColor(renderer, 32, 32, 32, 0xFF);

    SDL_RenderClear(renderer);

    if (isPlayerActive)
        TestPlayer::Render(renderer, player);

    TestWindow::RenderControls(controls, windowDPIScale);

    SDL_RenderPresent(renderer);
}

#if defined _windows && defined _DEBUG
int wmain(int argc, wchar_t* argv[])
#elif defined _windows && defined NDEBUG
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
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
