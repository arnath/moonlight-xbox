#include <string>
#include <opus_multistream.h>
#include "Limelight.h"
#include "MoonlightCommonInterop.h"
#include "IVideoRenderer.h"

using namespace Platform;
using namespace Moonlight::Xbox::Binding;

#define INITIAL_FRAME_BUFFER_SIZE 32768
static int g_VideoFrameBufferSize = 0;
static char* g_VideoFrameBuffer = NULL;

#define PCM_FRAME_SIZE 240
#define CHANNEL_COUNT 2
static int g_AudioFrameBufferSize = 0;
static char* g_AudioFrameBuffer = NULL;
static OpusMSDecoder* g_OpusDecoder = NULL;

int MoonlightCommonInterop::DrSetup(
	int videoFormat,
	int width,
	int height,
	int redrawRate,
	void* context,
	int drFlags)
{
	return _videoRenderer->Initialize(videoFormat, width, height, redrawRate);
}

void MoonlightCommonInterop::DrStart()
{
	_videoRenderer->Start();
}

void MoonlightCommonInterop::DrStop()
{
	_videoRenderer->Stop();
}

void MoonlightCommonInterop::DrCleanup()
{
	if (g_VideoFrameBuffer != NULL)
	{
		free(g_VideoFrameBuffer);
		g_VideoFrameBuffer = NULL;
		g_VideoFrameBufferSize = 0;
	}

	_videoRenderer->Cleanup();
}

int MoonlightCommonInterop::DrSubmitDecodeUnit(PDECODE_UNIT decodeUnit)
{
	// Resize the frame buffer if the current frame is too big.
	// This is safe without locking because this function is
	// called only from a single thread.
	if (g_VideoFrameBufferSize < decodeUnit->fullLength)
	{
		g_VideoFrameBufferSize = decodeUnit->fullLength;
		g_VideoFrameBuffer = (char*)malloc(g_VideoFrameBufferSize);
	}

	if (g_VideoFrameBuffer == NULL)
	{
		g_VideoFrameBufferSize = 0;
		return DR_NEED_IDR;
	}

	PLENTRY currentEntry = decodeUnit->bufferList;
	int offset = 0;
	while (currentEntry != NULL)
	{
		// Submit parameter set NALUs separately from picture data
		if (currentEntry->bufferType != BUFFER_TYPE_PICDATA)
		{
			// Use the beginning of the buffer each time since this is a separate
			// invocation of the decoder each time.
			memcpy(&g_VideoFrameBuffer[0], currentEntry->data, currentEntry->length);

			int ret =
				_videoRenderer->HandleFrame(
					ArrayReference<unsigned char>((unsigned char*)g_VideoFrameBuffer, currentEntry->length),
					currentEntry->bufferType,
					decodeUnit->frameNumber,
					decodeUnit->receiveTimeMs);
			if (ret != DR_OK)
			{
				return ret;
			}
		}
		else
		{
			memcpy(&g_VideoFrameBuffer[offset], currentEntry->data, currentEntry->length);
			offset += currentEntry->length;
			currentEntry = currentEntry->next;
		}
	}

	return
		_videoRenderer->HandleFrame(
			ArrayReference<unsigned char>((unsigned char*)g_VideoFrameBuffer, offset),
			BUFFER_TYPE_PICDATA,
			decodeUnit->frameNumber,
			decodeUnit->receiveTimeMs);
}

int MoonlightCommonInterop::ArInit(
	int audioConfiguration,
	const POPUS_MULTISTREAM_CONFIGURATION opusConfig,
	void* context,
	int arFlags)
{
	int err = _audioRenderer->Initialize(audioConfiguration);
	if (err != 0)
	{
		return err;
	}

	g_OpusDecoder =
		opus_multistream_decoder_create(
			opusConfig->sampleRate,
			opusConfig->channelCount,
			opusConfig->streams,
			opusConfig->coupledStreams,
			opusConfig->mapping,
			&err);
	if (g_OpusDecoder == NULL)
	{
		ArCleanup();
		return -1;
	}

	// We know ahead of time what the buffer size will be for decoded audio, so pre-allocate it
	g_AudioFrameBufferSize = opusConfig->channelCount * PCM_FRAME_SIZE * sizeof(opus_int16);
	g_AudioFrameBuffer = (char *)malloc(g_AudioFrameBufferSize);

	return err;
}

void MoonlightCommonInterop::ArStart()
{
	_audioRenderer->Start();
}

void MoonlightCommonInterop::ArStop()
{
	_audioRenderer->Stop();
}

void MoonlightCommonInterop::ArCleanup()
{
	if (g_OpusDecoder != NULL)
	{
		opus_multistream_decoder_destroy(g_OpusDecoder);
		g_OpusDecoder = NULL;
	}

	if (g_AudioFrameBuffer != NULL)
	{
		free(g_AudioFrameBuffer);
		g_AudioFrameBuffer = NULL;
		g_AudioFrameBufferSize = 0;
	}

	_audioRenderer->Cleanup();
}

void MoonlightCommonInterop::ArDecodeAndPlaySample(char *sampleData, int sampleLength)
{
	int decodeLen =
		opus_multistream_decode(
			g_OpusDecoder,
			(const unsigned char*)sampleData,
			sampleLength,
			(opus_int16*)g_AudioFrameBuffer,
			PCM_FRAME_SIZE,
			0);
	if (decodeLen > 0)
	{
		_audioRenderer->HandleFrame(ArrayReference<unsigned char>((unsigned char *)g_AudioFrameBuffer, g_AudioFrameBufferSize));
	}
}

MoonlightCommonInterop::MoonlightCommonInterop(
	IVideoRenderer^ videoRenderer,
	IAudioRenderer^ audioRenderer)
{
	_videoRenderer = videoRenderer;
	_audioRenderer = audioRenderer;
}

int MoonlightCommonInterop::StartConnection()
{
	return 0;
}