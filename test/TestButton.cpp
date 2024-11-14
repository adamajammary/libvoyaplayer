#include "TestButton.h"

TestButton::TestButton(int fontSize, TestButtonId id, const std::string& label, bool enabled)
{
    this->background = {};
    this->enabled    = enabled;
    this->fontSize   = fontSize;
    this->id         = id;
    this->label      = label;
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
    auto renderer = TestWindow::GetRenderer();
    auto surface  = TestText::GetSurface(this);

    this->size    = { surface->w, surface->h };
    this->texture = SDL_CreateTextureFromSurface(renderer, surface);

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
