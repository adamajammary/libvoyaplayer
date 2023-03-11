#include "LVP_TimeOut.h"

System::LVP_TimeOut::LVP_TimeOut(uint32_t timeOut)
{
	this->started   = false;
	this->startTime = 0;
	this->timeOut   = timeOut;
}

int System::LVP_TimeOut::InterruptCallback(void* data)
{
	LVP_TimeOut* timeOut = static_cast<LVP_TimeOut*>(data);

	return ((timeOut != NULL) && timeOut->isTimedOut() ? 1 : 0);
}

bool System::LVP_TimeOut::isTimedOut()
{
	return (this->started && ((SDL_GetTicks() - this->startTime) >= this->timeOut));
}

void System::LVP_TimeOut::start()
{
	this->startTime = SDL_GetTicks();
	this->started   = true;
}

void System::LVP_TimeOut::stop()
{
	this->started = false;
}
