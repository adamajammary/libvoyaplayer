#include "LVP_AudioContext.h"

MediaPlayer::LVP_AudioContext::LVP_AudioContext()
{
	this->bufferOffset      = 0;
	this->bufferRemaining   = 0;
	this->bufferSize        = 0;
	this->decodeFrame       = true;
	this->deviceSpecs       = {};
	this->deviceSpecsWanted = {};
	this->filter            = {};
	this->frame             = NULL;
	this->frameDuration     = 0.0;
	this->frameEncoded      = NULL;
	this->isDriverReady     = true;
	this->lastPogress       = 0.0;
	this->specs             = {};
	this->writtenToStream   = 0;
}

MediaPlayer::LVP_AudioContext::~LVP_AudioContext()
{
	FREE_AVFILTER_GRAPH(this->filter.filterGraph);
	FREE_AVFRAME(this->frame);
	FREE_POINTER(this->frameEncoded);
	FREE_SWR(this->specs.swrContext);
}
