/***************************************************************************
                          playerportaudio.cpp  -  description
                             -------------------
    begin                : Wed Feb 20 2002
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                : 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "playerportaudio.h"

PlayerPortAudio::PlayerPortAudio(int size, std::vector<EngineObject *> *engines, QString device) : Player(size, engines, device)
{
    PaError err = Pa_Initialize();
    if( err != paNoError )
        qFatal("PortAudio: Initialization error");

    // Default device ID.
    PaDeviceID id = -1;

    // Fill out devices list with info about available devices
    int no = Pa_CountDevices();

    const PaDeviceInfo *devInfo;
    for (int i=0; i<no; i++)
    {
		//	qWarning("dev");
		// Add the device if it is an output device:
        devInfo = Pa_GetDeviceInfo(i);
		if (devInfo->maxOutputChannels != 0)
		{
			// Add new PlayerInfo object to devices list
			Player::Info *p = new Player::Info;
			devices.append(p);
        
	        // Name
		    p->name = QString(devInfo->name);

			// Check for default device
	        if (p->name == device)
		        id = i;

			// Sample rates
			if (devInfo->numSampleRates != -1)
			{
				for (int j=0; j<devInfo->numSampleRates; j++)
					p->sampleRates.append((int)devInfo->sampleRates[j]);
			} 
			else 
			{
				// If we're just given a range of samplerates, then just
				// assume some standard rates:
				p->sampleRates.append(11025);
				p->sampleRates.append(22050);
				p->sampleRates.append(44100);
				p->sampleRates.append(48000);
				p->sampleRates.append(96000);
			}
	
		    // Bits
			if (devInfo->nativeSampleFormats & paInt8)        p->bits.append(8);
	        if (devInfo->nativeSampleFormats & paInt16)       p->bits.append(16);
		    //if (devInfo->nativeSampleFormats & paPackedInt24) p->bits.append(24);
			if (devInfo->nativeSampleFormats & paInt24)       p->bits.append(24);
			if (devInfo->nativeSampleFormats & paInt32)       p->bits.append(32);
		}
    }

    // Get id of default playback device if ID of device was not found in previous loop
    if (id<0)
        id = Pa_GetDefaultOutputDeviceID();
    devInfo = Pa_GetDeviceInfo(id);

	//qWarning("Device %s, num rates %d",devInfo->name, devInfo->numSampleRates);

    // Ensure stereo is supported
    if (devInfo->maxOutputChannels < NO_CHANNELS)
        qFatal("PortAudio: Not enough channels available on default output device: %i",devInfo->maxOutputChannels);
    
    // Set sample rate to 44100 if possible, otherwise highest possible
    /*int temp_sr = 0;
    {for (int i=0; i<=devInfo->numSampleRates; i++)
        if (devInfo->sampleRates[i] == 44100.)
            temp_sr = 44100;
	}
    if (temp_sr == 0)
        temp_sr = (int)devInfo->sampleRates[devInfo->numSampleRates-1];*/

    if (!open(QString(devInfo->name),44100,16,size))
        qFatal("PortAudio: Error opening device");
}

PlayerPortAudio::~PlayerPortAudio()
{
    Pa_Terminate();
}

#define NUM_SECONDS   (1)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (512)
#ifndef M_PI
#define M_PI  (3.14159265)
#endif
#define TABLE_SIZE   (200)
typedef struct {
	float sine[TABLE_SIZE];
	int left_phase;
	int right_phase;
}paTestData;
/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int patestCallback(   void *inputBuffer, void *outputBuffer,
                             unsigned long framesPerBuffer,
                             PaTimestamp outTime, void *userData )
{
	paTestData *data = (paTestData*)userData;
	float *out = (float*)outputBuffer;
	unsigned long i;
	int finished = 0;
	(void) outTime; /* Prevent unused variable warnings. */
	(void) inputBuffer;
	for( i=0; i<framesPerBuffer; i++ )
	{
		*out++ = data->sine[data->left_phase];		/* left */
		*out++ = data->sine[data->right_phase];		/* right */
		data->left_phase += 1;
		if( data->left_phase >= TABLE_SIZE ) data->left_phase -= TABLE_SIZE;
		data->right_phase += 3; /* higher pitch so we can distinguish left and right. */
		if( data->right_phase >= TABLE_SIZE ) data->right_phase -= TABLE_SIZE;
	}
	return finished;
}

bool PlayerPortAudio::open(QString name, int srate, int bits, int bufferSize)
{
    
	
	// Extract bit information
    PaSampleFormat format = 0;
    switch (bits)
    {
        case 8:  format = paInt8; break;
        case 16: format = paInt16; break;
        case 24: format = paInt24; break;
        case 32: format = paInt32; break;
        default: qFatal("PortAudio: Sample format not supported (%i bits)", bits); return false;
    }

    // Extract device information
    unsigned int id;
    for (id=0; id<devices.count(); id++)
        if (name == devices.at(id)->name)
            break;
    // Try to open device 5 times before giving up!
    PaError err = 0;
    for (int i=0; i<5; i++)
    {
      err = Pa_OpenStream(&stream,
                        paNoDevice,         // no input device
                        0,                  // no input
                        format,
                        NULL,
                        id,                 // output device
                        NO_CHANNELS,        // stereo output
                        format,
                        NULL,
                        (double)srate,
                        bufferSize/NO_CHANNELS,   // frames per buffer per channel
                        0,                  // number of buffers, if zero then use default minimum
                        paClipOff,          // we won't output out of range samples so don't bother clipping them
                        paCallback,
                        this );

        if (err == paNoError)
            break;
    }
    if( err != paNoError )
    {
        qFatal("PortAudio: Open stream error: %s", Pa_GetErrorText(err));
        return false;
    }

    // Fill in active config information
    setParams(QString(Pa_GetDeviceInfo(id)->name), srate, 16, bufferSize);
    buffer_size = bufferSize;

    allocate();

    return true;
}

void PlayerPortAudio::close()
{
    PaError err = Pa_CloseStream( stream );
    if( err != paNoError )
        qFatal("PortAudio: Close stream error: %s", Pa_GetErrorText(err));

    deallocate();
}

void PlayerPortAudio::start(EngineObject *_reader)
{
    Player::start(_reader);

    PaError err = Pa_StartStream(stream);
    if (err != paNoError)
        qFatal("PortAudio: Start stream error: %s", Pa_GetErrorText(err));
}

void PlayerPortAudio::wait()
{
}

void PlayerPortAudio::stop()
{
	PaError err = Pa_StopStream( stream );
	if( err != paNoError ) exit(-1);
}

CSAMPLE *PlayerPortAudio::process(const CSAMPLE *, const int)
{
	return 0;
}

/* -------- ------------------------------------------------------
   Purpose: Wrapper function to call processing loop function,
            implemented as a method in a class. Used in PortAudio,
            which knows nothing about C++.
   Input:   .
   Output:  -
   -------- ------------------------------------------------------ */
int paCallback(void *, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      PaTimestamp, void *_player)
{
    Player *player = (Player *)_player;
    SAMPLE *out = (SAMPLE*)outputBuffer;
    player->prepareBuffer();
    for (unsigned int i=0; i<framesPerBuffer*NO_CHANNELS; i++)
        *out++=player->out_buffer[i];
    return 0;
}
