#ifndef TEST_WINDOW_H
#define TEST_WINDOW_H

#include <algorithm> // min/max(x)
#include <unordered_map>

extern "C" {
    #include <SDL2/SDL_ttf.h>
}

#if defined _linux
	#include <gtk/gtk.h>       // gtk_file_chooser_dialog_new(x), gtk_dialog_run(x), gtk_file_chooser_get_uri(x)
#elif defined _macosx
	#include <AppKit/AppKit.h> // NSOpenPanel
#elif defined _windows
	#include <windows.h>
    #include <Commdlg.h>       // GetOpenFileNameW(x)
#endif

#include <libvoyaplayer.h>

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
    BUTTON_ID_OPEN,
    BUTTON_ID_PROGRESS,
    BUTTON_ID_SEEK_BACK,
    BUTTON_ID_SEEK_FORWARD,
    BUTTON_ID_STOP
};

struct Button
{
    SDL_Rect      background = {};
    bool          enabled    = true;
    ButtonId      id         = BUTTON_ID_UNKNOWN;
    std::string   label      = "";
    SDL_Renderer* renderer   = nullptr;
    SDL_Point     size       = {};
    SDL_Texture*  texture    = nullptr;

    Button(SDL_Renderer* renderer, ButtonId id, const std::string& label, bool enabled = true)
    {
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
	        const auto FONT_PATH = "/system/fonts/DroidSans.ttf";
        #elif defined _ios
	        const auto FONT_PATH = "/System/Library/Fonts/Cache/arialuni.ttf";
        #elif defined _linux
	        const auto FONT_PATH = "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf";
        #elif defined  _macosx
	        const auto FONT_PATH = "/System/Library/Fonts/Supplemental/Arial Unicode.ttf";
        #elif defined _windows
	        const auto FONT_PATH = "C:\\Windows\\Fonts\\ARIALUNI.TTF";
        #endif

		auto font = TTF_OpenFont(FONT_PATH, 14);

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

using Buttons = std::unordered_map<ButtonId, Button*>;

class TestWindow
{
private:
    TestWindow()  {}
    ~TestWindow() {}

private:
    static Buttons       buttons;
    static SDL_Renderer* renderer;
    static std::string   title;
    static SDL_Window*   window;

public:
    static void          EnableButton(ButtonId id, bool enabled = true);
    static Button*       GetClickedButton(const SDL_Point& clickPosition);
    static SDL_Rect      GetDimensions();
    static SDL_Renderer* GetRenderer();
    static void          Init(int width, int height);
    static void          Quit();
    static void          RenderControls(const SDL_Rect& destination);
    static void          UpdateButton(ButtonId id, const std::string& label);
    static void          UpdateProgress();

    #if defined _windows
        static std::wstring OpenFile();
    #elif defined _linux || defined _macosx
        static std::string OpenFile();
    #endif

private:
    static void initButtons();

};

#endif
