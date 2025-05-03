#include "LVP_AudioContext.h"

MediaPlayer::LVP_AudioContext::LVP_AudioContext()
{
	this->buffer            = NULL;
	this->bufferOffset      = 0;
	this->bufferSize        = 0;
	this->dataSize          = 0;
	this->deviceSpecs       = {};
	this->deviceSpecsWanted = {};
	this->filter            = {};
	this->filterSpecs       = {};
	this->frames            = {};
	this->lastPogress       = 0.0;
	this->packetDuration    = 0.0;
}

MediaPlayer::LVP_AudioContext::~LVP_AudioContext()
{
	this->free();
}

void MediaPlayer::LVP_AudioContext::clearFrames()
{
	this->framesLock.lock();

	while (!this->frames.empty()) {
		FREE_AVFRAME(this->frames.front());
		this->frames.pop();
	}

	this->framesLock.unlock();
}

void MediaPlayer::LVP_AudioContext::free()
{
	FREE_POINTER(this->buffer);
	FREE_AVFILTER_GRAPH(this->filter.filterGraph);

	LVP_AudioContext::clearFrames();
}
