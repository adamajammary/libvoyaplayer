#include "TestWindow.h"

TestButtonIds TestWindow::buttonIds = {};
TestButtons   TestWindow::buttons   = {};
SDL_Renderer* TestWindow::renderer  = nullptr;
std::string   TestWindow::title     = "Voya Player Library";
SDL_Window*   TestWindow::window    = nullptr;

void TestWindow::EnableButton(TestButtonId id, bool enabled)
{
	if (TestWindow::buttonIds.contains(id))
		TestWindow::buttonIds[id]->enable(enabled);
}

TestButton* TestWindow::GetClickedButton(const SDL_MouseButtonEvent& event)
{
	#if defined _ios || defined _macosx
		auto      scale    = TestWindow::GetDPIScale();
		SDL_Point position = { (int)((float)event.x * scale), (int)((float)event.y * scale) };
	#else
		SDL_Point position = { event.x, event.y };
	#endif

	for (const auto& button : TestWindow::buttons) {
		if (button->enabled && SDL_PointInRect(&position, &button->background))
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

float TestWindow::GetDPIScale()
{
	#if defined _android
		float dpi;
		SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(TestWindow::window), &dpi, nullptr, nullptr);

		return (dpi / 160.0f);
	#else
		auto sizeInPixels = TestWindow::GetDimensions();

		SDL_Rect size = {};
		SDL_GetWindowSize(TestWindow::window, &size.w, &size.h);

		return ((float)sizeInPixels.w / (float)size.w);
	#endif
}

SDL_Renderer* TestWindow::GetRenderer()
{
    return TestWindow::renderer;
}

void TestWindow::Init(int width, int height, const char* basePath)
{
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0)
        throw std::runtime_error(TextFormat("Failed to initialize SDL: %s", SDL_GetError()));

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

	#if defined _linux || defined _macosx || defined _windows
	TestWindow::initIcon(basePath);
	#endif
	
	TestWindow::initButtons(basePath);
}

void TestWindow::initButtons(const char* basePath)
{
	auto dpiScale = TestWindow::GetDPIScale();

	auto open = new TestButton(TestWindow::renderer, dpiScale, basePath, TEST_BUTTON_ID_PLAY_PAUSE, "PAUSE");

	TestWindow::buttonIds[TEST_BUTTON_ID_PLAY_PAUSE] = open;
	TestWindow::buttons.push_back(open);

	auto stop = new TestButton(TestWindow::renderer, dpiScale, basePath, TEST_BUTTON_ID_STOP, "STOP", false);

	TestWindow::buttonIds[TEST_BUTTON_ID_STOP] = stop;
	TestWindow::buttons.push_back(stop);

	auto seekBack = new TestButton(TestWindow::renderer, dpiScale, basePath, TEST_BUTTON_ID_SEEK_BACK, "<< SEEK", false);

	TestWindow::buttonIds[TEST_BUTTON_ID_SEEK_BACK] = seekBack;
	TestWindow::buttons.push_back(seekBack);

	auto seekForward = new TestButton(TestWindow::renderer, dpiScale, basePath, TEST_BUTTON_ID_SEEK_FORWARD, "SEEK >>", false);

	TestWindow::buttonIds[TEST_BUTTON_ID_SEEK_FORWARD] = seekForward;
	TestWindow::buttons.push_back(seekForward);

	auto progress = new TestButton(TestWindow::renderer, dpiScale, basePath, TEST_BUTTON_ID_PROGRESS, "00:00:00 / 00:00:00", false);

	TestWindow::buttonIds[TEST_BUTTON_ID_PROGRESS] = progress;
	TestWindow::buttons.push_back(progress);
}

#if defined _linux || defined _macosx || defined _windows
void TestWindow::initIcon(const char* basePath)
{
	auto icon   = TextFormat("%s%s", basePath, "icon.ppm");
	auto file   = std::fopen(icon.c_str(), "rb");
	auto pixels = (uint8_t*)std::malloc(TestAppIcon::Size);

	std::fseek(file, 13, SEEK_SET);

	if (pixels)
		std::fread(pixels, 1, TestAppIcon::Size, file);

	std::fclose(file);

	auto surface = SDL_CreateRGBSurfaceWithFormatFrom(
		pixels,
		TestAppIcon::Width,
		TestAppIcon::Height,
		TestAppIcon::Depth,
		TestAppIcon::Pitch,
		TestAppIcon::Format
	);

	if (surface)
		SDL_SetWindowIcon(TestWindow::window, surface);

	SDL_FreeSurface(surface);
	std::free(pixels);
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

	SDL_Quit();
}

void TestWindow::RenderControls(const SDL_Rect& destination, float dpiScale)
{
	SDL_Point mousePosition = {};
	SDL_GetMouseState(&mousePosition.x, &mousePosition.y);

	SDL_Color backgroundColor = { 0x10, 0x10, 0x10, 0xFF };
	SDL_Color highlightColor  = { 0x40, 0x40, 0x40, 0xFF };
	SDL_Color lineColor       = { 0x80, 0x80, 0x80, 0xFF };

	auto padding10 = (int)(10.0f * dpiScale);
	auto padding5  = (int)(5.0f  * dpiScale);

	auto controlsSize = ((int)TestWindow::buttons.size() * 2 * padding10);

	for (const auto& button : TestWindow::buttons)
		controlsSize += button->size.x;

	auto offsetX = std::max(0, (std::max(0, (destination.w - controlsSize)) / 2));

	SDL_SetRenderDrawColor(TestWindow::renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
	SDL_RenderFillRect(TestWindow::renderer, &destination);

	int lineY      = (destination.y + padding5);
	int lineHeight = (destination.h - padding10);

	if (!TestWindow::buttons.empty()) {
		SDL_SetRenderDrawColor(TestWindow::renderer, lineColor.r, lineColor.g, lineColor.b, lineColor.a);
		SDL_RenderDrawLine(TestWindow::renderer, offsetX, lineY, offsetX, (lineY + lineHeight));
	}

	offsetX += padding10;

	for (const auto& button : TestWindow::buttons)
	{
		#if defined _linux || defined _macosx || defined _windows
		SDL_Rect highlightArea = { (button->background.x - padding5), lineY, (button->background.w + padding10), lineHeight };

		if (button->enabled && SDL_PointInRect(&mousePosition, &highlightArea)) {
			SDL_SetRenderDrawColor(TestWindow::renderer, highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);
			SDL_RenderFillRect(TestWindow::renderer, &highlightArea);
		}
		#endif

		button->background = { offsetX, (lineY + ((lineHeight - button->size.y) / 2)), button->size.x, button->size.y };

		SDL_RenderCopy(TestWindow::renderer, button->texture, nullptr, &button->background);

		offsetX += (button->background.w + padding10);

		SDL_SetRenderDrawColor(TestWindow::renderer, lineColor.r, lineColor.g, lineColor.b, lineColor.a);
		SDL_RenderDrawLine(TestWindow::renderer, offsetX, lineY, offsetX, (lineY + lineHeight));

		offsetX += padding10;
	}
}

void TestWindow::UpdateButton(TestButtonId id, const std::string& label)
{
	if (TestWindow::buttonIds.contains(id))
		TestWindow::buttonIds[id]->update(label);
}

void TestWindow::UpdateProgress()
{
	if (LVP_IsStopped())
		return;

	auto duration = TimeFormat(LVP_GetDuration());
	auto progress = TimeFormat(LVP_GetProgress());

	TestWindow::UpdateButton(TEST_BUTTON_ID_PROGRESS, TextFormat("%s / %s", progress.c_str(), duration.c_str()));
}

void TestWindow::UpdateTitle(const std::string& title)
{
	if (!title.empty())
		SDL_SetWindowTitle(TestWindow::window, TextFormat("%s - %s", TestWindow::title.c_str(), title.c_str()).c_str());
	else
		SDL_SetWindowTitle(TestWindow::window, TestWindow::title.c_str());
}
