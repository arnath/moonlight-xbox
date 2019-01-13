#include <string>
#include <opus_multistream.h>
#include "Limelight.h"
#include "MoonlightCommonInterop.h"

using namespace Platform;
using namespace Moonlight::Xbox::Interop;

static IVideoRenderer^ g_VideoRenderer;
static IAudioRenderer^ g_AudioRenderer;
static IConnectionListener^ g_ConnectionListener;

#define INITIAL_FRAME_BUFFER_SIZE 32768
static int g_VideoFrameBufferSize = 0;
static char* g_VideoFrameBuffer = NULL;

#define PCM_FRAME_SIZE 240
#define CHANNEL_COUNT 2
static int g_AudioFrameBufferSize = 0;
static char* g_AudioFrameBuffer = NULL;
static OpusMSDecoder* g_OpusDecoder = NULL;

int DrSetup(
	int videoFormat,
	int width,
	int height,
	int redrawRate,
	void* context,
	int drFlags)
{
	return g_VideoRenderer->Initialize(videoFormat, width, height, redrawRate);
}

void DrStart()
{
	g_VideoRenderer->Start();
}

void DrStop()
{
	g_VideoRenderer->Stop();
}

void DrCleanup()
{
	if (g_VideoFrameBuffer != NULL)
	{
		free(g_VideoFrameBuffer);
		g_VideoFrameBuffer = NULL;
		g_VideoFrameBufferSize = 0;
	}

	g_VideoRenderer->Cleanup();
}

int DrSubmitDecodeUnit(PDECODE_UNIT decodeUnit)
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
				g_VideoRenderer->HandleFrame(
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
		g_VideoRenderer->HandleFrame(
			ArrayReference<unsigned char>((unsigned char*)g_VideoFrameBuffer, offset),
			BUFFER_TYPE_PICDATA,
			decodeUnit->frameNumber,
			decodeUnit->receiveTimeMs);
}

int ArInit(
	int audioConfiguration,
	const POPUS_MULTISTREAM_CONFIGURATION opusConfig,
	void* context,
	int arFlags)
{
	int err = g_AudioRenderer->Initialize(audioConfiguration);
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

void ArStart()
{
	g_AudioRenderer->Start();
}

void ArStop()
{
	g_AudioRenderer->Stop();
}

void ArCleanup()
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

	g_AudioRenderer->Cleanup();
}

void ArDecodeAndPlaySample(char *sampleData, int sampleLength)
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
		g_AudioRenderer->HandleFrame(ArrayReference<unsigned char>((unsigned char *)g_AudioFrameBuffer, g_AudioFrameBufferSize));
	}
}

int MoonlightCommonInterop::StartConnection(
	IVideoRenderer^ videoRenderer,
	IAudioRenderer^ audioRenderer,
	IConnectionListener^ connectionListener)
{
	g_VideoRenderer = videoRenderer;
	g_AudioRenderer = audioRenderer;

	return 0;
}