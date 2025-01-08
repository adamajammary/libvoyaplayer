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
	FREE_POINTER(this->buffer);
	FREE_AVFILTER_GRAPH(this->filter.filterGraph);
}
