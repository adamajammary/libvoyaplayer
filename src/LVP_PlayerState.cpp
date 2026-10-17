#include "LVP_PlayerState.h"

MediaPlayer::LVP_PlayerState::LVP_PlayerState()
{
	this->isCompleted = false;
	this->isMuted     = false;
	this->isOpening   = false;
	this->isPaused    = false;
	this->isPlaying   = false;
	this->isStopped   = true;
	this->isStopping  = false;

	this->audioDevice  = {};
	this->audioDevices = {};

	this->filePath = "";
	this->fileSize = 0;

	this->duration      = 0;
	this->mediaType     = AVMEDIA_TYPE_UNKNOWN;
	this->openFilePath  = "";
	this->playbackSpeed = 1.0;
	this->progress      = 0.0;
	this->quit          = false;
	this->seek          = {};
	this->track         = {};
	this->volume        = 1.0f;

	for (int i = 0; i < LVP_NR_OF_THREADS; i++)
		this->threads[i] = false;
}

void MediaPlayer::LVP_PlayerState::reset()
{
	this->duration  = 0;
	this->filePath  = "";
	this->fileSize  = 0;
	this->isPaused  = false;
	this->isPlaying = false;
	this->mediaType = AVMEDIA_TYPE_UNKNOWN;
	this->progress  = 0.0;

	this->track.count = 0;
}
