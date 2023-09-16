#include "TestWindow.h"

ButtonIds     TestWindow::buttonIds;
Buttons       TestWindow::buttons;
SDL_Renderer* TestWindow::renderer = nullptr;
std::string   TestWindow::title    = "Voya Player Library";
SDL_Window*   TestWindow::window   = nullptr;

void TestWindow::EnableButton(ButtonId id, bool enabled)
{
	if (TestWindow::buttonIds.contains(id))
		TestWindow::buttonIds[id]->enable(enabled);
}

Button* TestWindow::GetClickedButton(const SDL_Point& clickPosition)
{
	for (const auto& button : TestWindow::buttons) {
		if (button->enabled && SDL_PointInRect(&clickPosition, &button->background))
			return button;
	}

	return nullptr;
}

SDL_Rect TestWindow::GetDimensions()
{
    SDL_Rect dimensions = {};

    SDL_GetWindowPosition(TestWindow::window,       &dimensions.x, &dimensions.y);
	SDL_GetRendererOutputSize(TestWindow::renderer, &dimensions.w, &dimensions.h);

    return dimensions;
}

SDL_Renderer* TestWindow::GetRenderer()
{
    return TestWindow::renderer;
}

void TestWindow::Init(int width, int height)
{
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0)
        throw std::runtime_error(TextFormat("Failed to initialize SDL: %s", SDL_GetError()));

	if (TTF_Init() < 0)
		throw std::runtime_error(TextFormat("Failed to initialize TTF: %s", TTF_GetError()));

	const auto WINDOW_FLAGS = (SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    TestWindow::window = SDL_CreateWindow(TestWindow::title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, WINDOW_FLAGS);

    if (!TestWindow::window)
        throw std::runtime_error(TextFormat("Failed to create a window: %s", SDL_GetError()));

	SDL_SetWindowMinimumSize(TestWindow::window, 640, 480);

    TestWindow::renderer = SDL_CreateRenderer(TestWindow::window, -1, SDL_RENDERER_ACCELERATED);

    if (!TestWindow::renderer)
        TestWindow::renderer = SDL_CreateRenderer(TestWindow::window, -1, SDL_RENDERER_SOFTWARE);

    if (!TestWindow::renderer)
        throw std::runtime_error(TextFormat("Failed to create a renderer: %s", SDL_GetError()));

	TestWindow::initButtons();
}

void TestWindow::initButtons()
{
	auto open = new Button(TestWindow::renderer, BUTTON_ID_OPEN, "OPEN");

	TestWindow::buttonIds[BUTTON_ID_OPEN] = open;
	TestWindow::buttons.push_back(open);

	auto stop = new Button(TestWindow::renderer, BUTTON_ID_STOP, "STOP", false);

	TestWindow::buttonIds[BUTTON_ID_STOP] = stop;
	TestWindow::buttons.push_back(stop);

	auto seekBack = new Button(TestWindow::renderer, BUTTON_ID_SEEK_BACK, "<< SEEK", false);

	TestWindow::buttonIds[BUTTON_ID_SEEK_BACK] = seekBack;
	TestWindow::buttons.push_back(seekBack);

	auto seekForward = new Button(TestWindow::renderer, BUTTON_ID_SEEK_FORWARD, "SEEK >>", false);

	TestWindow::buttonIds[BUTTON_ID_SEEK_FORWARD] = seekForward;
	TestWindow::buttons.push_back(seekForward);

	auto progress = new Button(TestWindow::renderer, BUTTON_ID_PROGRESS, "00:00:00 / 00:00:00", false);

	TestWindow::buttonIds[BUTTON_ID_PROGRESS] = progress;
	TestWindow::buttons.push_back(progress);
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
	for (const auto& button : TestWindow::buttons)
		delete button;

	TestWindow::buttonIds.clear();
	TestWindow::buttons.clear();

	if (TestWindow::renderer) {
		SDL_DestroyRenderer(TestWindow::renderer);
		TestWindow::renderer = nullptr;
	}

	if (TestWindow::window) {
		SDL_DestroyWindow(TestWindow::window);
		TestWindow::window = nullptr;
	}

	TTF_Quit();
    SDL_Quit();
}

void TestWindow::RenderControls(const SDL_Rect& destination)
{
	SDL_Point mousePosition = {};
	SDL_GetMouseState(&mousePosition.x, &mousePosition.y);

	SDL_Color backgroundColor = { 0x10, 0x10, 0x10, 0xFF };
	SDL_Color highlightColor  = { 0x40, 0x40, 0x40, 0xFF };
	SDL_Color lineColor       = { 0x80, 0x80, 0x80, 0xFF };

	auto controlsSize = ((int)TestWindow::buttons.size() * 2 * 10);

	for (const auto& button : TestWindow::buttons)
		controlsSize += button->size.x;

	auto offsetX = std::max(0, (std::max(0, (destination.w - controlsSize)) / 2));

	SDL_SetRenderDrawColor(TestWindow::renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
	SDL_RenderFillRect(TestWindow::renderer, &destination);

	int lineY      = (!TestWindow::buttons.empty() ? TestWindow::buttons[0]->background.y : 0);
	int lineHeight = (!TestWindow::buttons.empty() ? TestWindow::buttons[0]->background.h : 0);

	if (!TestWindow::buttons.empty()) {
		SDL_SetRenderDrawColor(TestWindow::renderer, lineColor.r, lineColor.g, lineColor.b, lineColor.a);
		SDL_RenderDrawLine(TestWindow::renderer, offsetX, lineY, offsetX, (lineY + lineHeight));
	}

	offsetX += 10;

	for (const auto& button : TestWindow::buttons)
	{
		SDL_Rect highlightArea = { button->background.x - 5, button->background.y, button->background.w + 10, button->background.h };

		if (button->enabled && SDL_PointInRect(&mousePosition, &highlightArea)) {
			SDL_SetRenderDrawColor(TestWindow::renderer, highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);
			SDL_RenderFillRect(TestWindow::renderer, &highlightArea);
		}

		button->background = { offsetX, (destination.y + 7), button->size.x, button->size.y };

		SDL_RenderCopy(TestWindow::renderer, button->texture, nullptr, &button->background);

		offsetX += (button->background.w + 10);

		SDL_SetRenderDrawColor(TestWindow::renderer, lineColor.r, lineColor.g, lineColor.b, lineColor.a);
		SDL_RenderDrawLine(TestWindow::renderer, offsetX, lineY, offsetX, (lineY + lineHeight));

		offsetX += 10;
	}
}

void TestWindow::UpdateButton(ButtonId id, const std::string& label)
{
	if (TestWindow::buttonIds.contains(id))
		TestWindow::buttonIds[id]->update(label);
}

void TestWindow::UpdateProgress()
{
	if (LVP_IsStopped())
		return;

	auto durationLabel = TimeFormat(LVP_GetDuration());
	auto progressLabel = TimeFormat(LVP_GetProgress());

	TestWindow::UpdateButton(BUTTON_ID_PROGRESS, TextFormat("%s / %s", progressLabel.c_str(), durationLabel.c_str()));
}
