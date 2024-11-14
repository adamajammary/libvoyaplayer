#include "TestButton.h"

TestButton::TestButton(SDL_Renderer* renderer, float dpiScale, const char* basePath, TestButtonId id, const std::string& label, bool enabled)
{
    this->background = {};
    this->basePath   = basePath;
    this->dpiScale   = dpiScale;
    this->enabled    = enabled;
    this->fontSize   = 14.0f;
    this->id         = id;
    this->label      = label;
    this->renderer   = renderer;
    this->size       = {};
    this->texture    = nullptr;

    this->create();
}

TestButton::~TestButton()
{
    this->destroy();
}

void TestButton::create()
{
    auto surface = TestText::GetSurface(this);

    this->size    = { surface->w, surface->h };
    this->texture = SDL_CreateTextureFromSurface(this->renderer, surface);

	SDL_FreeSurface(surface);
}

void TestButton::destroy()
{
    if (this->texture) {
        SDL_DestroyTexture(this->texture);
        this->texture = nullptr;
    }
}

void TestButton::enable(bool enabled)
{
    this->enabled = enabled;

    this->destroy();
    this->create();
}

void TestButton::update(const std::string& label)
{
    this->label = label;

    this->destroy();
    this->create();
}
