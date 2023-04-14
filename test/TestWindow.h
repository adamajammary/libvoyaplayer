#ifndef TEST_WINDOW_H
#define TEST_WINDOW_H

#include <chrono>
#include <format>
#include <unordered_map>

extern "C" {
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_syswm.h>
}

#if defined _linux
	#include <gtk/gtk.h> // gtk_file_chooser_dialog_new(x), gtk_dialog_run(x), gtk_file_chooser_get_uri(x)
#elif defined _macosx
	#include <AppKit/AppKit.h> // NSOpenPanel
#elif defined _windows
	#include <windows.h>
	#include <windowsx.h>
    #include <CommCtrl.h>
    #include <Commdlg.h> // GetOpenFileNameA(x)
#endif

#include <libvoyaplayer.h>

const enum Bitmap
{
    BITMAP_MUTE,
    BITMAP_PAUSE,
    BITMAP_PLAY,
    BITMAP_STOP,
    BITMAP_VOLUME,
    NR_OF_BITMAPS
};

const enum Control
{
    CONTROL_MUTE,
    CONTROL_PANEL,
    CONTROL_PAUSE,
    CONTROL_PROGRESS,
    CONTROL_SEEK,
    CONTROL_SPEED,
    CONTROL_STOP,
    CONTROL_VOLUME,
    NR_OF_CONTROLS
};

const enum ControlItem
{
    CONTROL_ITEM_INVALID,
    CONTROL_ITEM_MUTE = 100,
    CONTROL_ITEM_PAUSE,
    CONTROL_ITEM_SEEK,
    CONTROL_ITEM_STOP,
    CONTROL_ITEM_VOLUME
};

const enum Menu
{
    MENU,
    MENU_AUDIO,
    MENU_AUDIO_DEVICE,
    MENU_AUDIO_TRACK,
    MENU_FILE,
    MENU_PLAYBACK,
    MENU_PLAYBACK_CHAPTER,
    MENU_PLAYBACK_SPEED,
    MENU_SUBTITLE,
    MENU_SUBTITLE_TRACK,
    MENU_HELP,
    NR_OF_MENUS
};

const enum MenuItem
{
    MENU_ITEM_AUDIO_MUTE = 1000,
    MENU_ITEM_AUDIO_VOLUME_DOWN,
    MENU_ITEM_AUDIO_VOLUME_UP,
    MENU_ITEM_FILE_OPEN,
    MENU_ITEM_FILE_QUIT,
    MENU_ITEM_HELP_ABOUT,
    MENU_ITEM_PLAYBACK_PAUSE,
    MENU_ITEM_PLAYBACK_SEEK_BACK,
    MENU_ITEM_PLAYBACK_SEEK_FORWARD,
    MENU_ITEM_PLAYBACK_STOP,
    MENU_ITEM_PLAYBACK_SPEED_050X = 2000,
    MENU_ITEM_PLAYBACK_SPEED_075X,
    MENU_ITEM_PLAYBACK_SPEED_100X,
    MENU_ITEM_PLAYBACK_SPEED_125X,
    MENU_ITEM_PLAYBACK_SPEED_150X,
    MENU_ITEM_PLAYBACK_SPEED_175X,
    MENU_ITEM_PLAYBACK_SPEED_200X,
    MENU_ITEM_AUDIO_DEVICE1 = 3000,
    MENU_ITEM_AUDIO_TRACK1 = 4000,
    MENU_ITEM_SUBTITLE_TRACK1 = 5000,
    MENU_ITEM_PLAYBACK_CHAPTER1 = 6000
};

const char MENU_LABEL_AUDIO[] = "Audio";
const char MENU_LABEL_AUDIO_DEVICE[] = "Audio Device";
const char MENU_LABEL_AUDIO_DEVICE_DEFAULT[] = "Default";
const char MENU_LABEL_AUDIO_MUTE[] = "Mute\tM";
const char MENU_LABEL_AUDIO_TRACK[] = "Audio Track";
const char MENU_LABEL_AUDIO_UNMUTE[] = "Unmute\tM";
const char MENU_LABEL_AUDIO_VOLUME_DOWN[] = "Decrease Volume\tDOWN";
const char MENU_LABEL_AUDIO_VOLUME_UP[] = "Increase Volume\tUP";
const char MENU_LABEL_FILE[] = "File";
const char MENU_LABEL_FILE_OPEN[] = "Open\tCtrl+O";
const char MENU_LABEL_FILE_QUIT[] = "Quit\tCtrl+Q";
const char MENU_LABEL_HELP[] = "Help";
const char MENU_LABEL_HELP_ABOUT[] = "About";
const char MENU_LABEL_PLAYBACK[] = "Playback";
const char MENU_LABEL_PLAYBACK_CHAPTER[] = "Chapter";
const char MENU_LABEL_PLAYBACK_PAUSE[] = "Pause\tSPACE";
const char MENU_LABEL_PLAYBACK_PLAY[] = "Play\tSPACE";
const char MENU_LABEL_PLAYBACK_SEEK_BACK[] = "Seek Backward\tLEFT";
const char MENU_LABEL_PLAYBACK_SEEK_FORWARD[] = "Seek Forward\tRIGHT";
const char MENU_LABEL_PLAYBACK_SPEED[] = "Speed";
const char MENU_LABEL_PLAYBACK_SPEED_050X[] = "0.5x (Slow)";
const char MENU_LABEL_PLAYBACK_SPEED_075X[] = "0.75x (Slow)";
const char MENU_LABEL_PLAYBACK_SPEED_100X[] = "1x (Normal)";
const char MENU_LABEL_PLAYBACK_SPEED_125X[] = "1.25x (Fast)";
const char MENU_LABEL_PLAYBACK_SPEED_150X[] = "1.5x (Fast)";
const char MENU_LABEL_PLAYBACK_SPEED_175X[] = "1.75x (Fast)";
const char MENU_LABEL_PLAYBACK_SPEED_200X[] = "2x (Fast)";
const char MENU_LABEL_PLAYBACK_STOP[] = "Stop\tS";
const char MENU_LABEL_SUBTITLE[] = "Subtitle";
const char MENU_LABEL_SUBTITLE_TRACK[] = "Subtitle Track";

#if defined _windows
	const char PATH_SEPARATOR = '\\';
#else
    const char PATH_SEPARATOR = '/';
#endif

const int PLAYER_CONTROLS_PANEL_HEIGHT   = 34;
const int PLAYER_CONTROLS_CONTENT_HEIGHT = 30;
const int PLAYER_CONTROLS_PROGRESS_WIDTH = 130;
const int PLAYER_CONTROLS_SPEED_WIDTH    = 54;
const int PLAYER_CONTROLS_VOLUME_WIDTH   = 100;
const int UI_UDPATE_RATE_MS              = 500;

class TestWindow
{
private:
    TestWindow()  {}
    ~TestWindow() {}

private:
    static std::unordered_map<unsigned short, LVP_MediaTrack>   audioTracks;
    static std::unordered_map<int, unsigned short>              audioTrackIds;
    static std::unordered_map<unsigned short, LVP_MediaChapter> chapters;
    static std::unordered_map<unsigned short, LVP_MediaTrack>   subtitleTracks;
    static std::unordered_map<int, unsigned short>              subtitleTrackIds;

    static std::string   about;
    static int           height;
    static SDL_Renderer* renderer;
    static std::string   title;
    static int           width;
    static SDL_Window*   window;

    #if defined _windows
        static HANDLE bitmaps[NR_OF_BITMAPS];
        static HWND   controls[NR_OF_CONTROLS];
        static HMENU  menus[NR_OF_MENUS];
    #endif

public:
    static LVP_MediaTrack   GetAudioTrack(unsigned short id);
    static unsigned short   GetAudioTrackId(int track);
    static LVP_MediaTrack   GetSubtitleTrack(unsigned short id);
    static unsigned short   GetSubtitleTrackId(int track);
    static LVP_MediaChapter GetChapter(unsigned short id);
    static double           GetControlSliderClickPosition(Control control);
    static SDL_Rect         GetDimensions();
    static unsigned short   GetMenuIdPlaybackSpeed(double speed);
    static std::string      GetMenuLabel(unsigned short id, Menu menu);
    static SDL_Renderer*    GetRenderer();
    static void             Init(int width, int height);
    static void             InitSubMenuAudioTracks(const std::vector<LVP_MediaTrack> &tracks);
    static void             InitSubMenuSubtitleTracks(const std::vector<LVP_MediaTrack> &tracks);
    static void             InitSubMenuChapters(const std::vector<LVP_MediaChapter> &chapters);
    static void             InitSubMenuItems(const std::vector<std::string> &items, Menu menu, MenuItem firstItem, const std::string &defaultLabel);
    static void             Quit();
    static void             Resize();
    static void             ShowAbout();
    static void             ToggleMenuChecked(unsigned short id, Menu menu, MenuItem firstItem);
    static void             UpdateUI(uint32_t deltaTime);

    #if defined _windows
        static ControlItem  GetControlId(HWND handle);
        static std::wstring OpenFile();
    #else
        static std::string OpenFile();
    #endif

private:
    static void          clearSubMenuItems(Menu menu, MenuItem firstItem);
    static SDL_SysWMinfo getSysInfo();
    static void          initSubMenuItems(const std::vector<std::string> &items, Menu menu, MenuItem firstItem);
    static void          initSubMenuTracks(const std::vector<LVP_MediaTrack> &tracks, Menu menu, MenuItem firstItem, std::unordered_map<unsigned short, LVP_MediaTrack> &tracksMap, std::unordered_map<int, unsigned short> &trackIdsMap);
    static void          toggleBitmapsEnabled(bool isPlayerActive, bool isPaused, bool isMuted);
    static void          toggleControlsEnabled(bool isPlayerActive);
    static void          toggleMenuEnabled(bool isPlayerActive, bool isPaused, bool isMuted);
    static void          updateBitmap(Control control, Bitmap bitmap);
    static void          updateChapters(int64_t progress);
    static void          updateControlsSlider(Control control, int64_t position);
    static void          updateControlsText(Control control, const std::string &text);
    static void          updateProgress(bool isEnabled, int64_t progress, int64_t duration);
    static void          updateTitle(bool isEnabled, const std::string &filePath);
    static void          updateToggleMenuItem(Menu menu, MenuItem item, const std::string &label);

    #if defined _windows
        static HANDLE getBitmap(const std::string &file);
        static HWND   getControl(LPCSTR className, DWORD style, const SDL_Rect &location, const SDL_SysWMinfo &sysInfo, HMENU id = NULL);
        static HWND   getControlButton(DWORD style, const SDL_Rect &location, const SDL_SysWMinfo &sysInfo, HMENU id = NULL);
        static HWND   getControlStatic(DWORD style, const SDL_Rect &location, const SDL_SysWMinfo &sysInfo);
        static void   initBitmaps();
        static void   initControls(const SDL_SysWMinfo &sysInfo);
        static void   initMenu(const SDL_SysWMinfo &sysInfo);
    #endif

};

#endif
