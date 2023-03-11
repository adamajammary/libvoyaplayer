#include "TestWindow.h"

std::unordered_map<unsigned short, LVP_MediaTrack>   TestWindow::audioTracks;
std::unordered_map<int, unsigned short>              TestWindow::audioTrackIds;
std::unordered_map<unsigned short, LVP_MediaChapter> TestWindow::chapters;
std::unordered_map<unsigned short, LVP_MediaTrack>   TestWindow::subtitleTracks;
std::unordered_map<int, unsigned short>              TestWindow::subtitleTrackIds;

std::string   TestWindow::about    = "libvoyaplayer is a free cross-platform media player library that easily plays your music and video files.\n\nCopyright (C) 2021 Adam A. Jammary (Jammary Studio)";
int           TestWindow::height   = 0;
SDL_Renderer* TestWindow::renderer = nullptr;
std::string   TestWindow::title    = "Voya Player Library";
int           TestWindow::width    = 0;
SDL_Window*   TestWindow::window   = nullptr;

#if defined _windows
	HANDLE TestWindow::bitmaps[NR_OF_BITMAPS];
	HWND   TestWindow::controls[NR_OF_CONTROLS];
	HMENU  TestWindow::menus[NR_OF_MENUS];
#endif

void TestWindow::clearSubMenuItems(Menu menu, MenuItem firstItem)
{
	auto itemCount = GetMenuItemCount(TestWindow::menus[menu]);

	for (int i = 0; i < itemCount; i++)
		RemoveMenu(TestWindow::menus[menu], (firstItem + i), MF_BYCOMMAND);
}

LVP_MediaTrack TestWindow::GetAudioTrack(unsigned short id)
{
	return TestWindow::audioTracks[id];
}

unsigned short TestWindow::GetAudioTrackId(int track)
{
	return TestWindow::audioTrackIds[track];
}

LVP_MediaTrack TestWindow::GetSubtitleTrack(unsigned short id)
{
	return TestWindow::subtitleTracks[id];
}

unsigned short TestWindow::GetSubtitleTrackId(int track)
{
	return TestWindow::subtitleTrackIds[track];
}

LVP_MediaChapter TestWindow::GetChapter(unsigned short id)
{
	return TestWindow::chapters[id];
}

#if defined _windows
HANDLE TestWindow::getBitmap(const std::string &file)
{
	return LoadImageA(NULL, file.c_str(), IMAGE_BITMAP, 0, 0, (LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED));
}

HWND TestWindow::getControl(LPCSTR className, DWORD style, const SDL_Rect &location, const SDL_SysWMinfo &sysInfo, HMENU id)
{
	auto hwnd     = sysInfo.info.win.window;
	auto instance = sysInfo.info.win.hinstance;

	return CreateWindowA(className, NULL, style, location.x, location.y, location.w, location.h, hwnd, id, instance, NULL);
}

HWND TestWindow::getControlButton(DWORD style, const SDL_Rect &location, const SDL_SysWMinfo &sysInfo, HMENU id)
{
	return TestWindow::getControl("BUTTON", style, location, sysInfo, id);
}

HWND TestWindow::getControlStatic(DWORD style, const SDL_Rect &location, const SDL_SysWMinfo &sysInfo)
{
	return TestWindow::getControl("STATIC", style, location, sysInfo);
}

ControlItem TestWindow::GetControlId(HWND handle)
{
	if (handle == TestWindow::controls[CONTROL_SEEK])
		return CONTROL_ITEM_SEEK;
	else if (handle == TestWindow::controls[CONTROL_VOLUME])
		return CONTROL_ITEM_VOLUME;

	return CONTROL_ITEM_INVALID;
}

#endif

double TestWindow::GetControlSliderClickPosition(Control control)
{
	#if defined _windows
		POINT mousePosition = {};
		GetCursorPos(&mousePosition);

		RECT controlPosition = {};
		GetWindowRect(TestWindow::controls[control], &controlPosition);

		RECT thumbPosition = {};
		SendMessageA(TestWindow::controls[control], TBM_GETTHUMBRECT, 0, (LPARAM)&thumbPosition);

		auto clickPosition = (mousePosition.x - controlPosition.left);
		auto thumbWidth    = (thumbPosition.right - thumbPosition.left);
		auto controlWidth  = (controlPosition.right - controlPosition.left - thumbWidth);

		return (double)((double)clickPosition / (double)controlWidth);
	#endif

	return 0;
}

SDL_Rect TestWindow::GetDimensions()
{
    SDL_Rect dimensions = {};

    SDL_GetWindowPosition(TestWindow::window,       &dimensions.x, &dimensions.y);
	SDL_GetRendererOutputSize(TestWindow::renderer, &dimensions.w, &dimensions.h);

    return dimensions;
}

unsigned short TestWindow::GetMenuIdPlaybackSpeed(double speed)
{
	if (speed > 1.99)
		return MENU_ITEM_PLAYBACK_SPEED_200X;
	else if (speed > 1.74)
		return MENU_ITEM_PLAYBACK_SPEED_175X;
	else if (speed > 1.49)
		return MENU_ITEM_PLAYBACK_SPEED_150X;
	else if (speed > 1.24)
		return MENU_ITEM_PLAYBACK_SPEED_125X;
	else if (speed > 0.99)
		return MENU_ITEM_PLAYBACK_SPEED_100X;
	else if (speed > 0.74)
		return MENU_ITEM_PLAYBACK_SPEED_075X;

	return MENU_ITEM_PLAYBACK_SPEED_050X;
}

std::string TestWindow::GetMenuLabel(unsigned short id, Menu menu)
{
	char label[256] = {};

	auto result = GetMenuStringA(TestWindow::menus[menu], id, label, 256, MF_BYCOMMAND);

	return (result > 0 ? std::string(label) : "");
}

SDL_Renderer* TestWindow::GetRenderer()
{
    return TestWindow::renderer;
}

SDL_SysWMinfo TestWindow::getSysInfo()
{
	SDL_SysWMinfo sysInfo = {};
	SDL_VERSION(&sysInfo.version);

	SDL_GetWindowWMInfo(TestWindow::window, &sysInfo);

	return sysInfo;
}

void TestWindow::Init(int width, int height)
{
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0)
        throw std::exception(std::format("Failed to initialize SDL: {}", SDL_GetError()).c_str());

	const auto FLAGS = (SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    TestWindow::window = SDL_CreateWindow(TestWindow::title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, FLAGS);

    if (!TestWindow::window)
        throw std::exception(std::format("Failed to create a window: {}", SDL_GetError()).c_str());

	SDL_SetWindowMinimumSize(TestWindow::window, 640, 480);

	auto window = TestWindow::GetDimensions();

	TestWindow::width  = window.w;
	TestWindow::height = window.h;

    TestWindow::renderer = SDL_CreateRenderer(TestWindow::window, -1, SDL_RENDERER_ACCELERATED);

    if (!TestWindow::renderer)
        TestWindow::renderer = SDL_CreateRenderer(TestWindow::window, -1, SDL_RENDERER_SOFTWARE);

    if (!TestWindow::renderer)
        throw std::exception(std::format("Failed to create a renderer: {}", SDL_GetError()).c_str());

	auto sysInfo = TestWindow::getSysInfo();

	#if defined _windows
		TestWindow::initBitmaps();
		TestWindow::initMenu(sysInfo);
		TestWindow::initControls(sysInfo);
	#endif

	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
}

#if defined _windows
void TestWindow::initBitmaps()
{
	TestWindow::bitmaps[BITMAP_MUTE]   = TestWindow::getBitmap("mute-16.bmp");
	TestWindow::bitmaps[BITMAP_PAUSE]  = TestWindow::getBitmap("pause-16.bmp");
	TestWindow::bitmaps[BITMAP_PLAY]   = TestWindow::getBitmap("play-16.bmp");
	TestWindow::bitmaps[BITMAP_STOP]   = TestWindow::getBitmap("stop-16.bmp");
	TestWindow::bitmaps[BITMAP_VOLUME] = TestWindow::getBitmap("volume-16.bmp");
}

void TestWindow::initControls(const SDL_SysWMinfo &sysInfo)
{
	for (auto i = 0; i < NR_OF_CONTROLS; i++)
		DestroyWindow(TestWindow::controls[i]);

	const int   DEFAULT_HEIGHT = PLAYER_CONTROLS_CONTENT_HEIGHT;
	const DWORD DEFAULT_STYLE  = (WS_CHILD | WS_VISIBLE);
	const int   OFFSET_Y       = (TestWindow::height - PLAYER_CONTROLS_PANEL_HEIGHT);

	const DWORD STYLE_DEFPUSH_BUTTON = (DEFAULT_STYLE | BS_DEFPUSHBUTTON | BS_BITMAP);
	const DWORD STYLE_PUSH_BUTTON    = (DEFAULT_STYLE | BS_PUSHBUTTON | BS_BITMAP);
	const DWORD STYLE_SLIDER         = (DEFAULT_STYLE | TBS_ENABLESELRANGE | TBS_FIXEDLENGTH | TBS_HORZ | TBS_NOTICKS | TBS_BOTH);
	const DWORD STYLE_TEXT           = (DEFAULT_STYLE | SS_CENTERIMAGE);

	SDL_Rect panelLocation = { 0, OFFSET_Y, TestWindow::width, PLAYER_CONTROLS_PANEL_HEIGHT };

	TestWindow::controls[CONTROL_PANEL] = TestWindow::getControlStatic(DEFAULT_STYLE, panelLocation, sysInfo);

	// PLAY/PAUSE

	SDL_Rect pauseLocation = { 2, (OFFSET_Y + 2), DEFAULT_HEIGHT, DEFAULT_HEIGHT };

	TestWindow::controls[CONTROL_PAUSE] = TestWindow::getControlButton(STYLE_DEFPUSH_BUTTON, pauseLocation, sysInfo, (HMENU)CONTROL_ITEM_PAUSE);

	// STOP

	SDL_Rect stopLocation = { (pauseLocation.x + pauseLocation.w), (OFFSET_Y + 2), DEFAULT_HEIGHT, DEFAULT_HEIGHT };

	TestWindow::controls[CONTROL_STOP] = TestWindow::getControlButton(STYLE_PUSH_BUTTON, stopLocation, sysInfo, (HMENU)CONTROL_ITEM_STOP);

	// SEEK

	int      seekWidth    = (TestWindow::width - pauseLocation.x - pauseLocation.w - stopLocation.w - PLAYER_CONTROLS_PROGRESS_WIDTH - PLAYER_CONTROLS_SPEED_WIDTH - PLAYER_CONTROLS_VOLUME_WIDTH - DEFAULT_HEIGHT);
	SDL_Rect seekLocation = { (stopLocation.x + stopLocation.w), (OFFSET_Y + 4), seekWidth, DEFAULT_HEIGHT };

	TestWindow::controls[CONTROL_SEEK] = TestWindow::getControl(TRACKBAR_CLASS, STYLE_SLIDER, seekLocation, sysInfo, (HMENU)CONTROL_ITEM_SEEK);

	SendMessageA(TestWindow::controls[CONTROL_SEEK], TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 100));

	// MUTE

	SDL_Rect muteLocation = { (seekLocation.x + seekLocation.w), (OFFSET_Y + 2), DEFAULT_HEIGHT, DEFAULT_HEIGHT };

	TestWindow::controls[CONTROL_MUTE] = TestWindow::getControlButton(STYLE_PUSH_BUTTON, muteLocation, sysInfo, (HMENU)CONTROL_ITEM_MUTE);

	SendMessageA(TestWindow::controls[CONTROL_MUTE], BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)TestWindow::bitmaps[BITMAP_VOLUME]);

	// VOLUME SLIDER

	SDL_Rect volumeLocation = { (muteLocation.x + muteLocation.w), (OFFSET_Y + 4), PLAYER_CONTROLS_VOLUME_WIDTH, DEFAULT_HEIGHT };

	TestWindow::controls[CONTROL_VOLUME] = TestWindow::getControl(TRACKBAR_CLASS, STYLE_SLIDER, volumeLocation, sysInfo, (HMENU)CONTROL_ITEM_VOLUME);

	SendMessageA(TestWindow::controls[CONTROL_VOLUME], TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 100));

	TestWindow::updateControlsSlider(CONTROL_VOLUME, 100);

	// PROGRESS/DURATION

	SDL_Rect progressLocation = { (volumeLocation.x + volumeLocation.w), OFFSET_Y, PLAYER_CONTROLS_PROGRESS_WIDTH, DEFAULT_HEIGHT };

	TestWindow::controls[CONTROL_PROGRESS] = TestWindow::getControlStatic(STYLE_TEXT, progressLocation, sysInfo);

	// PLAYBACK SPEED

	SDL_Rect speedLocation = { (progressLocation.x + progressLocation.w), OFFSET_Y, PLAYER_CONTROLS_SPEED_WIDTH, DEFAULT_HEIGHT };

	TestWindow::controls[CONTROL_SPEED] = TestWindow::getControlStatic(STYLE_TEXT, speedLocation, sysInfo);
}

void TestWindow::initMenu(const SDL_SysWMinfo &sysInfo)
{
	TestWindow::menus[MENU] = CreateMenu();

	TestWindow::menus[MENU_AUDIO]            = CreatePopupMenu();
	TestWindow::menus[MENU_AUDIO_DEVICE]     = CreatePopupMenu();
	TestWindow::menus[MENU_AUDIO_DRIVER]     = CreatePopupMenu();
	TestWindow::menus[MENU_AUDIO_TRACK]      = CreatePopupMenu();
	TestWindow::menus[MENU_FILE]             = CreatePopupMenu();
	TestWindow::menus[MENU_PLAYBACK]         = CreatePopupMenu();
	TestWindow::menus[MENU_PLAYBACK_CHAPTER] = CreatePopupMenu();
	TestWindow::menus[MENU_PLAYBACK_SPEED]   = CreatePopupMenu();
	TestWindow::menus[MENU_SUBTITLE]         = CreatePopupMenu();
	TestWindow::menus[MENU_SUBTITLE_TRACK]   = CreatePopupMenu();
	TestWindow::menus[MENU_HELP]             = CreatePopupMenu();

	// MENU

	AppendMenuA(TestWindow::menus[MENU], MF_POPUP, (UINT_PTR)TestWindow::menus[MENU_FILE],     MENU_LABEL_FILE);
	AppendMenuA(TestWindow::menus[MENU], MF_POPUP, (UINT_PTR)TestWindow::menus[MENU_PLAYBACK], MENU_LABEL_PLAYBACK);
	AppendMenuA(TestWindow::menus[MENU], MF_POPUP, (UINT_PTR)TestWindow::menus[MENU_AUDIO],    MENU_LABEL_AUDIO);
	AppendMenuA(TestWindow::menus[MENU], MF_POPUP, (UINT_PTR)TestWindow::menus[MENU_SUBTITLE], MENU_LABEL_SUBTITLE);
	AppendMenuA(TestWindow::menus[MENU], MF_POPUP, (UINT_PTR)TestWindow::menus[MENU_HELP],     MENU_LABEL_HELP);

	// FILE

	AppendMenuA(TestWindow::menus[MENU_FILE], MF_STRING, MENU_ITEM_FILE_OPEN, MENU_LABEL_FILE_OPEN);
	AppendMenuA(TestWindow::menus[MENU_FILE], MF_SEPARATOR, NULL, NULL);
	AppendMenuA(TestWindow::menus[MENU_FILE], MF_STRING, MENU_ITEM_FILE_QUIT, MENU_LABEL_FILE_QUIT);

	// PLAYBACK

	AppendMenuA(TestWindow::menus[MENU_PLAYBACK], MF_POPUP | MF_DISABLED, (UINT_PTR)TestWindow::menus[MENU_PLAYBACK_CHAPTER], MENU_LABEL_PLAYBACK_CHAPTER);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK], MF_SEPARATOR, NULL, NULL);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK], MF_POPUP | MF_DISABLED, (UINT_PTR)TestWindow::menus[MENU_PLAYBACK_SPEED], MENU_LABEL_PLAYBACK_SPEED);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK], MF_SEPARATOR, NULL, NULL);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK], MF_STRING | MF_DISABLED, MENU_ITEM_PLAYBACK_SEEK_FORWARD, MENU_LABEL_PLAYBACK_SEEK_FORWARD);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK], MF_STRING | MF_DISABLED, MENU_ITEM_PLAYBACK_SEEK_BACK,    MENU_LABEL_PLAYBACK_SEEK_BACK);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK], MF_SEPARATOR, NULL, NULL);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK], MF_STRING | MF_DISABLED, MENU_ITEM_PLAYBACK_PAUSE, MENU_LABEL_PLAYBACK_PLAY);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK], MF_STRING | MF_DISABLED, MENU_ITEM_PLAYBACK_STOP,  MENU_LABEL_PLAYBACK_STOP);

	AppendMenuA(TestWindow::menus[MENU_PLAYBACK_SPEED], MFT_RADIOCHECK | MF_UNCHECKED, MENU_ITEM_PLAYBACK_SPEED_050X, MENU_LABEL_PLAYBACK_SPEED_050X);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK_SPEED], MFT_RADIOCHECK | MF_UNCHECKED, MENU_ITEM_PLAYBACK_SPEED_075X, MENU_LABEL_PLAYBACK_SPEED_075X);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK_SPEED], MFT_RADIOCHECK | MF_CHECKED,   MENU_ITEM_PLAYBACK_SPEED_100X, MENU_LABEL_PLAYBACK_SPEED_100X);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK_SPEED], MFT_RADIOCHECK | MF_UNCHECKED, MENU_ITEM_PLAYBACK_SPEED_125X, MENU_LABEL_PLAYBACK_SPEED_125X);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK_SPEED], MFT_RADIOCHECK | MF_UNCHECKED, MENU_ITEM_PLAYBACK_SPEED_150X, MENU_LABEL_PLAYBACK_SPEED_150X);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK_SPEED], MFT_RADIOCHECK | MF_UNCHECKED, MENU_ITEM_PLAYBACK_SPEED_175X, MENU_LABEL_PLAYBACK_SPEED_175X);
	AppendMenuA(TestWindow::menus[MENU_PLAYBACK_SPEED], MFT_RADIOCHECK | MF_UNCHECKED, MENU_ITEM_PLAYBACK_SPEED_200X, MENU_LABEL_PLAYBACK_SPEED_200X);

	// AUDIO

	AppendMenuA(TestWindow::menus[MENU_AUDIO], MF_POPUP, (UINT_PTR)TestWindow::menus[MENU_AUDIO_TRACK],  MENU_LABEL_AUDIO_TRACK);
	AppendMenuA(TestWindow::menus[MENU_AUDIO], MF_POPUP, (UINT_PTR)TestWindow::menus[MENU_AUDIO_DEVICE], MENU_LABEL_AUDIO_DEVICE);
	AppendMenuA(TestWindow::menus[MENU_AUDIO], MF_POPUP, (UINT_PTR)TestWindow::menus[MENU_AUDIO_DRIVER], MENU_LABEL_AUDIO_DRIVER);
	AppendMenuA(TestWindow::menus[MENU_AUDIO], MF_SEPARATOR, NULL, NULL);
	AppendMenuA(TestWindow::menus[MENU_AUDIO], MF_STRING | MF_DISABLED, MENU_ITEM_AUDIO_VOLUME_UP,   MENU_LABEL_AUDIO_VOLUME_UP);
	AppendMenuA(TestWindow::menus[MENU_AUDIO], MF_STRING | MF_DISABLED, MENU_ITEM_AUDIO_VOLUME_DOWN, MENU_LABEL_AUDIO_VOLUME_DOWN);
	AppendMenuA(TestWindow::menus[MENU_AUDIO], MF_STRING | MF_DISABLED, MENU_ITEM_AUDIO_MUTE,        MENU_LABEL_AUDIO_MUTE);

	// SUBTITLE

	AppendMenuA(TestWindow::menus[MENU_SUBTITLE], MF_POPUP, (UINT_PTR)TestWindow::menus[MENU_SUBTITLE_TRACK], MENU_LABEL_SUBTITLE_TRACK);

	// HELP

	AppendMenuA(TestWindow::menus[MENU_HELP], MF_STRING, MENU_ITEM_HELP_ABOUT, MENU_LABEL_HELP_ABOUT);

	SetMenu(sysInfo.info.win.window, TestWindow::menus[MENU]);
}
#endif

void TestWindow::initSubMenuTracks(
	const std::vector<LVP_MediaTrack> &tracks,
	Menu menu,
	MenuItem firstItem,
	std::unordered_map<unsigned short, LVP_MediaTrack> &tracksMap,
	std::unordered_map<int, unsigned short> &trackIdsMap
)
{
	std::vector<std::string> trackTitles;

	tracksMap.clear();

	for (unsigned short i = 0; i < (unsigned short)tracks.size(); i++)
	{
		LVP_MediaTrack track = tracks[i];

		tracksMap[firstItem + i] = track;
		trackIdsMap[track.track] = (unsigned short)(firstItem + i);

		std::string metaLanguage = track.meta["language"];
		std::string metaTitle    = track.meta["title"];

		auto offset     = (menu == MENU_SUBTITLE_TRACK ? 0 : 1);
		auto title      = (!metaTitle.empty() ? metaTitle : std::format("Track {}", (i + offset)));;
		auto language   = (!metaLanguage.empty() ? std::format("\t{}", metaLanguage) : "");;
		auto trackTitle = std::format("{}{}", title, language);

		trackTitles.push_back(trackTitle);
	}

	TestWindow::initSubMenuItems(trackTitles, menu, firstItem);
}

void TestWindow::InitSubMenuAudioTracks(const std::vector<LVP_MediaTrack> &tracks)
{
	TestWindow::initSubMenuTracks(tracks, MENU_AUDIO_TRACK, MENU_ITEM_AUDIO_TRACK1, TestWindow::audioTracks, TestWindow::audioTrackIds);
}

void TestWindow::InitSubMenuSubtitleTracks(const std::vector<LVP_MediaTrack> &tracks)
{
	TestWindow::initSubMenuTracks(tracks, MENU_SUBTITLE_TRACK, MENU_ITEM_SUBTITLE_TRACK1, TestWindow::subtitleTracks, TestWindow::subtitleTrackIds);
}

void TestWindow::InitSubMenuChapters(const std::vector<LVP_MediaChapter> &chapters)
{
	std::vector<std::string> chapterTitles;

	TestWindow::chapters.clear();

	for (unsigned short i = 0; i < (unsigned short)chapters.size(); i++)
	{
		LVP_MediaChapter chapter = chapters[i];

		TestWindow::chapters[MENU_ITEM_PLAYBACK_CHAPTER1 + i] = chapter;

		auto startChrono  = std::chrono::seconds(chapter.startTime / 1000);
		auto chapterTitle = std::format("{}\t{:%T}", chapter.title, startChrono);

		chapterTitles.push_back(chapterTitle);
	}

	TestWindow::initSubMenuItems(chapterTitles, MENU_PLAYBACK_CHAPTER, MENU_ITEM_PLAYBACK_CHAPTER1);
}

void TestWindow::InitSubMenuItems(const std::vector<std::string> &items, Menu menu, MenuItem firstItem, const std::string &defaultLabel)
{
	#if defined _windows
		TestWindow::clearSubMenuItems(menu, firstItem);

		AppendMenuA(TestWindow::menus[menu], MFT_RADIOCHECK | MF_CHECKED, firstItem, defaultLabel.c_str());

		for (size_t i = 0; i < items.size(); i++)
			AppendMenuA(TestWindow::menus[menu], MFT_RADIOCHECK | MF_UNCHECKED, ((size_t)firstItem + 1 + i), items[i].c_str());
	#endif
}

void TestWindow::initSubMenuItems(const std::vector<std::string> &items, Menu menu, MenuItem firstItem)
{
	#if defined _windows
		TestWindow::clearSubMenuItems(menu, firstItem);

		for (size_t i = 0; i < items.size(); i++)
			AppendMenuA(TestWindow::menus[menu], MFT_RADIOCHECK | (i > 0 ? MF_UNCHECKED : MF_CHECKED), (firstItem + i), items[i].c_str());
	#endif
}

#if defined _linux
std::string TestWindow::OpenFile()
{
	std::string directoryPath = "";

	if (strlen(std::getenv("DISPLAY")) == 0)
		SDL_setenv("DISPLAY", ":0", 1);

	if (!gtk_init_check(0, NULL))
		return directoryPath;

	GtkWidget* dialog = gtk_file_chooser_dialog_new(
		"Select a file", NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL
	);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char* selectedPath = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		if (selectedPath != NULL) {
			directoryPath = std::string(selectedPath);
			g_free(selectedPath);
		}
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));

	while (gtk_events_pending())
		gtk_main_iteration();

	if (directoryPath.substr(0, 7) == "file://")
		directoryPath = directoryPath.substr(7);

	return directoryPath;
}
#elif defined _macosx
std::string TestWindow::OpenFile()
{
	std::string  directoryPath = "";
	NSOpenPanel* panel         = [NSOpenPanel openPanel];

	if (!panel)
		return directoryPath;

	[panel setAllowsMultipleSelection: NO];
	[panel setCanChooseDirectories: NO];
	[panel setCanChooseFiles: YES];

	if ([panel runModal] != NSOKButton)
		return directoryPath;

	CFURLRef selectedURL = (CFURLRef) [[panel URLs]firstObject];

	if (!selectedURL)
		return directoryPath;

	const int MAX_FILE_PATH = 260;
	char selectedPath[MAX_FILE_PATH] = {};

	CFURLGetFileSystemRepresentation(selectedURL, TRUE, (UInt8*)selectedPath, MAX_FILE_PATH);

	directoryPath = std::string(selectedPath);

	if (directoryPath.substr(0, 7) == "file://")
		directoryPath = directoryPath.substr(7);

	return directoryPath;
}
#elif defined _windows
std::wstring TestWindow::OpenFile()
{
	const int MAX_FILE_PATH = 260;
	wchar_t selectedPath[MAX_FILE_PATH] = {};

	OPENFILENAMEW browseDialog;
	memset(&browseDialog, 0, sizeof(browseDialog));

	browseDialog.lStructSize  = sizeof(browseDialog);
	browseDialog.lpstrFile    = selectedPath;
	browseDialog.lpstrFile[0] = '\0';
	browseDialog.nMaxFile     = sizeof(selectedPath);
	browseDialog.lpstrFilter  = L"All\0*.*\0";
	browseDialog.nFilterIndex = 1;
	browseDialog.Flags        = (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NODEREFERENCELINKS | OFN_EXPLORER);

	std::wstring directoryPath = L"";

	if (GetOpenFileNameW(&browseDialog))
		directoryPath = std::wstring(selectedPath);

	if (directoryPath.substr(0, 7) == L"file://")
		directoryPath = directoryPath.substr(7);

	return directoryPath;
}
#endif

void TestWindow::Quit()
{
	#if defined _windows
		for (auto i = 0; i < NR_OF_CONTROLS; i++)
			DestroyWindow(TestWindow::controls[i]);

		for (auto i = 0; i < NR_OF_MENUS; i++)
			DestroyMenu(TestWindow::menus[i]);
	#endif

	if (TestWindow::renderer) {
		SDL_DestroyRenderer(TestWindow::renderer);
		TestWindow::renderer = nullptr;
	}

	if (TestWindow::window) {
		SDL_DestroyWindow(TestWindow::window);
		TestWindow::window = nullptr;
	}

    SDL_Quit();
}

void TestWindow::Resize()
{
	auto sysInfo = TestWindow::getSysInfo();
	auto window  = TestWindow::GetDimensions();

	TestWindow::width  = window.w;
	TestWindow::height = window.h;

	#if defined _windows
		TestWindow::initControls(sysInfo);
	#endif
}

void TestWindow::ShowAbout()
{
	#if defined _windows
		MessageBoxA(nullptr, TestWindow::about.c_str(), TestWindow::title.c_str(), MB_OK);
	#endif
}

void TestWindow::toggleBitmapsEnabled(bool isPlayerActive, bool isPaused, bool isMuted)
{
	auto muteBitmap  = (isMuted ? BITMAP_MUTE : BITMAP_VOLUME);
	auto pauseBitmap = (!isPlayerActive || isPaused ? BITMAP_PLAY : BITMAP_PAUSE);

	TestWindow::updateBitmap(CONTROL_MUTE,  muteBitmap);
	TestWindow::updateBitmap(CONTROL_PAUSE, pauseBitmap);
	TestWindow::updateBitmap(CONTROL_STOP,  BITMAP_STOP);
}

void TestWindow::toggleControlsEnabled(bool isPlayerActive)
{
	#if defined _windows
		for (auto i = 0; i < NR_OF_CONTROLS; i++)
		{
			if ((bool)IsWindowEnabled(TestWindow::controls[i]) != isPlayerActive) {
				EnableWindow(TestWindow::controls[i], isPlayerActive);
				InvalidateRect(TestWindow::controls[i], NULL, 1);
			}
		}
	#endif
}

void TestWindow::ToggleMenuChecked(unsigned short id, Menu menu, MenuItem firstItem)
{
	#if defined _windows
		auto itemCount = GetMenuItemCount(TestWindow::menus[menu]);
		auto lastID    = (firstItem + itemCount - 1);

		CheckMenuRadioItem(TestWindow::menus[menu], firstItem, lastID, id, MF_BYCOMMAND);
	#endif
}

void TestWindow::toggleMenuEnabled(bool isPlayerActive, bool isPaused, bool isMuted)
{
	auto muteLabel  = (isMuted ? MENU_LABEL_AUDIO_UNMUTE : MENU_LABEL_AUDIO_MUTE);
	auto pauseLabel = (!isPlayerActive || isPaused ? MENU_LABEL_PLAYBACK_PLAY : MENU_LABEL_PLAYBACK_PAUSE);

	TestWindow::updateToggleMenuItem(MENU_AUDIO,    MENU_ITEM_AUDIO_MUTE,     muteLabel);
	TestWindow::updateToggleMenuItem(MENU_PLAYBACK, MENU_ITEM_PLAYBACK_PAUSE, pauseLabel);

	#if defined _windows
		UINT AUDIO_TRACKS_ENABLED = (isPlayerActive && !TestWindow::audioTracks.empty()    ? MF_ENABLED : MF_DISABLED);
		UINT SUB_TRACKS_ENABLED   = (isPlayerActive && !TestWindow::subtitleTracks.empty() ? MF_ENABLED : MF_DISABLED);
		UINT CHAPTERS_ENABLED     = (isPlayerActive && !TestWindow::chapters.empty()       ? MF_ENABLED : MF_DISABLED);
		UINT ENABLED              = (isPlayerActive ? MF_ENABLED : MF_DISABLED);

		EnableMenuItem(TestWindow::menus[MENU_AUDIO],    MENU_ITEM_AUDIO_MUTE,            ENABLED);
		EnableMenuItem(TestWindow::menus[MENU_AUDIO],    MENU_ITEM_AUDIO_VOLUME_DOWN,     ENABLED);
		EnableMenuItem(TestWindow::menus[MENU_AUDIO],    MENU_ITEM_AUDIO_VOLUME_UP,       ENABLED);
		EnableMenuItem(TestWindow::menus[MENU_PLAYBACK], MENU_ITEM_PLAYBACK_PAUSE,        ENABLED);
		EnableMenuItem(TestWindow::menus[MENU_PLAYBACK], MENU_ITEM_PLAYBACK_SEEK_BACK,    ENABLED);
		EnableMenuItem(TestWindow::menus[MENU_PLAYBACK], MENU_ITEM_PLAYBACK_SEEK_FORWARD, ENABLED);
		EnableMenuItem(TestWindow::menus[MENU_PLAYBACK], MENU_ITEM_PLAYBACK_STOP,         ENABLED);

		EnableMenuItem(TestWindow::menus[MENU_AUDIO],    (UINT)((UINT_PTR)TestWindow::menus[MENU_AUDIO_TRACK]),      AUDIO_TRACKS_ENABLED);
		EnableMenuItem(TestWindow::menus[MENU_SUBTITLE], (UINT)((UINT_PTR)TestWindow::menus[MENU_SUBTITLE_TRACK]),   SUB_TRACKS_ENABLED);
		EnableMenuItem(TestWindow::menus[MENU_PLAYBACK], (UINT)((UINT_PTR)TestWindow::menus[MENU_PLAYBACK_CHAPTER]), CHAPTERS_ENABLED);
		EnableMenuItem(TestWindow::menus[MENU_PLAYBACK], (UINT)((UINT_PTR)TestWindow::menus[MENU_PLAYBACK_SPEED]),   ENABLED);
	#endif
}

void TestWindow::updateBitmap(Control control, Bitmap bitmap)
{
	#if defined _windows
		auto handle = (HANDLE)SendMessageA(TestWindow::controls[control], BM_GETIMAGE, (WPARAM)IMAGE_BITMAP, 0);

		if (handle != TestWindow::bitmaps[bitmap])
			SendMessageA(TestWindow::controls[control], BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)TestWindow::bitmaps[bitmap]);
	#endif
}

void TestWindow::updateChapters(int64_t progress)
{
	for (const auto &chapter : TestWindow::chapters)
	{
		if ((progress >= chapter.second.startTime) && (progress <= chapter.second.endTime)) {
			TestWindow::ToggleMenuChecked(chapter.first, MENU_PLAYBACK_CHAPTER, MENU_ITEM_PLAYBACK_CHAPTER1);
			break;
		}
	}
}

void TestWindow::updateControlsSlider(Control id, int64_t position)
{
	#if defined _windows
		SendMessageA(TestWindow::controls[id], TBM_SETPOS, (WPARAM)TRUE, (LPARAM)position);

		if (position > 0)
			SendMessageA(TestWindow::controls[id], TBM_SETSEL, (WPARAM)TRUE, (LPARAM)MAKELONG(0, position));
		else
			SendMessageA(TestWindow::controls[id], TBM_CLEARSEL, (WPARAM)TRUE, 0);
	#endif
}

void TestWindow::updateControlsText(Control id, const std::string &text)
{
	#if defined _windows
		char oldText[256];
		GetWindowTextA(TestWindow::controls[id], oldText, 256);

		if (std::string(oldText) != text)
			SendMessageA(TestWindow::controls[id], WM_SETTEXT, 0, (LPARAM)text.c_str());
	#endif
}

void TestWindow::updateProgress(bool isEnabled, int64_t progress, int64_t duration)
{
	if (isEnabled)
	{
		auto durationChrono = std::chrono::seconds(duration / 1000);
		auto progressChrono = std::chrono::seconds(progress / 1000);

		TestWindow::updateControlsText(CONTROL_PROGRESS, std::format("{:%T} / {:%T}", progressChrono, durationChrono));
	} else {
		TestWindow::updateControlsText(CONTROL_PROGRESS, "00:00:00 / 00:00:00");
	}
}

void TestWindow::updateTitle(bool isEnabled, const std::string &filePath)
{
	std::string title;

	if (isEnabled) {
		auto file = filePath.substr(filePath.rfind(PATH_SEPARATOR) + 1);
		title     = std::format("{} - {}", file, TestWindow::title);
	} else {
		title = TestWindow::title;
	}

	if (title != std::string(SDL_GetWindowTitle(TestWindow::window)))
		SDL_SetWindowTitle(TestWindow::window, title.c_str());
}

void TestWindow::updateToggleMenuItem(Menu menu, MenuItem item, const std::string &label)
{
	#if defined _windows
		ModifyMenuA(TestWindow::menus[menu], item, MF_STRING, item, label.c_str());
	#endif
}

void TestWindow::UpdateUI(const LVP_State &state, bool isActive, uint32_t deltaTime)
{
	bool isEnabled = (isActive && !state.filePath.empty());

	if (isEnabled)
		TestWindow::updateControlsSlider(CONTROL_SEEK, (state.progress * 100 / state.duration));

	TestWindow::updateControlsSlider(CONTROL_VOLUME, (int64_t)(state.volume * 100));

	TestWindow::updateControlsText(CONTROL_SPEED, std::format("[{:.2f}x]", state.playbackSpeed));

	TestWindow::toggleControlsEnabled(isEnabled);
	TestWindow::toggleBitmapsEnabled(isEnabled, state.isPaused, state.isMuted);
	TestWindow::toggleMenuEnabled(isEnabled, state.isPaused, state.isMuted);

	if (deltaTime < UI_UDPATE_RATE_MS)
		return;

	if (isEnabled)
		TestWindow::updateChapters(state.progress);
	else
		TestWindow::updateControlsSlider(CONTROL_SEEK, 0);

	TestWindow::updateProgress(isEnabled, state.progress, state.duration);
	TestWindow::updateTitle(isEnabled, state.filePath);
}
