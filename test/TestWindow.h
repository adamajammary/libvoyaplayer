#ifndef TEST_WINDOW_H
#define TEST_WINDOW_H

#include <algorithm> // min/max(x)
#include <vector>
#include <unordered_map>

extern "C" {
    #include <SDL2/SDL_ttf.h>
}

#if defined _android
    #include <android/asset_manager_jni.h> // AAsset*, JNI*, j*
    #include <sys/stat.h>                  // mkdir(x)
#elif defined _windows
    #include <windows.h> // WinMain(x)
#endif

#include <libvoyaplayer.h>

struct Icon {
    static const int      depth  = 24;
    static const uint32_t format = SDL_PIXELFORMAT_RGB24;
    static const int      height = 64;
    static const int      pitch  = 192;
    static const size_t   size   = 12288;
    static const int      width  = 64;
};

template<typename... Args>
static std::string TextFormat(const char* formatString, const Args&... args)
{
    if (!formatString)
        return "";

    char buffer[1024] = {};
    std::snprintf(buffer, 1024, formatString, args...);

    return std::string(buffer);
}

static std::string TimeFormat(int64_t milliSeconds)
{
    auto totSecs = (milliSeconds / 1000);
    auto hours   = (totSecs / 3600);
    auto remSecs = (totSecs % 3600);
    auto minutes = (remSecs / 60);
    auto seconds = (remSecs % 60);

    return TextFormat("%02d:%02d:%02d", hours, minutes, seconds);
}

enum ButtonId
{
    BUTTON_ID_UNKNOWN = -1,
    BUTTON_ID_PLAY_PAUSE,
    BUTTON_ID_PROGRESS,
    BUTTON_ID_SEEK_BACK,
    BUTTON_ID_SEEK_FORWARD,
    BUTTON_ID_STOP
};

struct Button
{
    SDL_Rect      background = {};
    const char*   basePath   = nullptr;
    float         dpiScale   = 1.0f;
    bool          enabled    = true;
    ButtonId      id         = BUTTON_ID_UNKNOWN;
    std::string   label      = "";
    SDL_Renderer* renderer   = nullptr;
    SDL_Point     size       = {};
    SDL_Texture*  texture    = nullptr;

    Button(SDL_Renderer* renderer, float dpiScale, const char* basePath, ButtonId id, const std::string& label, bool enabled = true)
    {
        this->basePath = basePath;
        this->dpiScale = dpiScale;
        this->enabled  = enabled;
        this->id       = id;
        this->label    = label;
        this->renderer = renderer;

        this->create();
    }

    ~Button()
    {
        this->destroy();
    }

    void create()
    {
        #if defined _android
    		const auto FONT_PATH = "/system/fonts/NotoSansCJK-Regular.ttc";
        #elif defined _ios
		    auto       fullPath  = TextFormat("%s%s", this->basePath, "Arial Unicode.ttf");
		    const auto FONT_PATH = fullPath.c_str();
        #elif defined _linux
    		const auto FONT_PATH = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc";
        #elif defined  _macosx
	        const auto FONT_PATH = "/System/Library/Fonts/Supplemental/Arial Unicode.ttf";
        #elif defined _windows
	        const auto FONT_PATH = "C:\\Windows\\Fonts\\ARIALUNI.TTF";
        #endif

        const auto FONT_SIZE = (int)(14.0f * this->dpiScale);

        auto font = TTF_OpenFont(FONT_PATH, FONT_SIZE);

        if (!font)
            throw std::runtime_error(TextFormat("Failed to open font '%s': %s", FONT_PATH, TTF_GetError()));

        const SDL_Color WHITE = { 0xFF, 0xFF, 0xFF, 0xFF };
        const SDL_Color GRAY  = { 0xAA, 0xAA, 0xAA, 0xFF };

        auto surface = TTF_RenderUTF8_Blended(font, this->label.c_str(), (this->enabled ? WHITE : GRAY));

        TTF_CloseFont(font);

        this->size    = { surface->w, surface->h };
        this->texture = SDL_CreateTextureFromSurface(this->renderer, surface);

        SDL_FreeSurface(surface);
    }

    void destroy()
    {
        if (this->texture) {
            SDL_DestroyTexture(this->texture);
            this->texture = nullptr;
        }
    }

    void enable(bool enabled = true)
    {
        this->enabled = enabled;

        this->destroy();
        this->create();
    }

    void update(const std::string& label)
    {
        this->label = label;

        this->destroy();
        this->create();
    }
};

using ButtonIds = std::unordered_map<ButtonId, Button*>;
using Buttons   = std::vector<Button*>;

class TestWindow
{
private:
    TestWindow()  {}
    ~TestWindow() {}

private:
    static ButtonIds     buttonIds;
    static Buttons       buttons;
    static SDL_Renderer* renderer;
    static std::string   title;
    static SDL_Window*   window;

public:
    static void          EnableButton(ButtonId id, bool enabled = true);
    static Button*       GetClickedButton(const SDL_MouseButtonEvent& event);
    static SDL_Rect      GetDimensions();
    static float         GetDPIScale();
    static SDL_Renderer* GetRenderer();
    static void          Init(int width, int height, const char* basePath);
    static void          Quit();
    static void          RenderControls(const SDL_Rect& destination, float dpiScale);
    static void          UpdateButton(ButtonId id, const std::string& label);
    static void          UpdateProgress();
    static void          UpdateTitle(const std::string& title = "");

private:
    static void initButtons(const char* basePath);

    #if defined _linux || defined _macosx || defined _windows
    static void initIcon(const char* basePath);
    #endif

};

#endif
