#include "LVP_PlayerState.h"

MediaPlayer::LVP_PlayerState::LVP_PlayerState()
{
	this->completed     = false;
	this->duration      = 0;
	this->filePath      = "";
	this->fileSize      = 0;
	this->isMuted       = false;
	this->isPaused      = false;
	this->isPlaying     = false;
	this->isStopped     = true;
	this->mediaType     = LibFFmpeg::AVMEDIA_TYPE_UNKNOWN;
	this->openFilePath  = "";
	this->playbackSpeed = 1.0;
	this->progress      = 0.0;
	this->quit          = false;
	this->trackCount    = 0;
	this->volume        = SDL_MIX_MAXVOLUME;

	for (int i = 0; i < LVP_NR_OF_THREADS; i++)
		this->threads[i] = false;
}

void MediaPlayer::LVP_PlayerState::reset()
{
	this->duration      = 0;
	this->filePath      = "";
	this->fileSize      = 0;
	this->isPaused      = false;
	this->isPlaying     = false;
	this->mediaType     = LibFFmpeg::AVMEDIA_TYPE_UNKNOWN;
	this->playbackSpeed = 1.0;
	this->progress      = 0.0;
	this->trackCount    = 0;
}
