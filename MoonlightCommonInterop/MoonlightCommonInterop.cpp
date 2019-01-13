#include <stdarg.h>
#include <string>
#include <opus_multistream.h>
#include "Limelight.h"
#include "MoonlightCommonInterop.h"

using namespace Platform;
using namespace Moonlight::Xbox::Interop;

// C callback declarations
static int DrSetup(int videoFormat, int width, int height, int redrawRate, void* context, int drFlags);
static void DrStart();
static void DrStop();
static void DrCleanup();
static int DrSubmitDecodeUnit(PDECODE_UNIT decodeUnit);
static int ArInit(int audioConfiguration, const POPUS_MULTISTREAM_CONFIGURATION opusConfig, void* context, int arFlags);
static void ArStart();
static void ArStop();
static void ArCleanup();
static void ArDecodeAndPlaySample(char *sampleData, int sampleLength);
static void ClStageStarting(int stage);
static void ClStageComplete(int stage);
static void ClStageFailed(int stage, long errorCode);
static void ClConnectionStarted();
static void ClConnectionTerminated(long errorCode);
static void ClDisplayMessage(const char* message);
static void ClDisplayTransientMessage(const char* message);
static void ClLogMessage(const char* format, ...);

static IVideoRenderer^ s_VideoRenderer;
static IAudioRenderer^ s_AudioRenderer;
static IConnectionListener^ s_ConnectionListener;

#define INITIAL_FRAME_BUFFER_SIZE 32768
static int s_VideoFrameBufferSize = 0;
static char* s_VideoFrameBuffer = NULL;

#define PCM_FRAME_SIZE 240
#define CHANNEL_COUNT 2
static int s_AudioFrameBufferSize = 0;
static char* s_AudioFrameBuffer = NULL;
static OpusMSDecoder* s_OpusDecoder = NULL;

inline String^ CStringToPlatformString(const char* string)
{
	std::string stdString = std::string(string);
	std::wstring wString = std::wstring(stdString.begin(), stdString.end());
	return ref new String(wString.c_str());
}

int DrSetup(
	int videoFormat,
	int width,
	int height,
	int redrawRate,
	void* context,
	int drFlags)
{
	return s_VideoRenderer->Initialize(videoFormat, width, height, redrawRate);
}

void DrStart()
{
	s_VideoRenderer->Start();
}

void DrStop()
{
	s_VideoRenderer->Stop();
}

void DrCleanup()
{
	if (s_VideoFrameBuffer != NULL)
	{
		free(s_VideoFrameBuffer);
		s_VideoFrameBuffer = NULL;
		s_VideoFrameBufferSize = 0;
	}

	s_VideoRenderer->Cleanup();
}

int DrSubmitDecodeUnit(PDECODE_UNIT decodeUnit)
{
	// Resize the frame buffer if the current frame is too big.
	// This is safe without locking because this function is
	// called only from a single thread.
	if (s_VideoFrameBufferSize < decodeUnit->fullLength)
	{
		s_VideoFrameBufferSize = decodeUnit->fullLength;
		s_VideoFrameBuffer = (char*)malloc(s_VideoFrameBufferSize);
	}

	if (s_VideoFrameBuffer == NULL)
	{
		s_VideoFrameBufferSize = 0;
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
			memcpy(&s_VideoFrameBuffer[0], currentEntry->data, currentEntry->length);

			int ret =
				s_VideoRenderer->HandleFrame(
					ArrayReference<unsigned char>((unsigned char*)s_VideoFrameBuffer, currentEntry->length),
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
			memcpy(&s_VideoFrameBuffer[offset], currentEntry->data, currentEntry->length);
			offset += currentEntry->length;
			currentEntry = currentEntry->next;
		}
	}

	return
		s_VideoRenderer->HandleFrame(
			ArrayReference<unsigned char>((unsigned char*)s_VideoFrameBuffer, offset),
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
	int err = s_AudioRenderer->Initialize(audioConfiguration);
	if (err != 0)
	{
		return err;
	}

	s_OpusDecoder =
		opus_multistream_decoder_create(
			opusConfig->sampleRate,
			opusConfig->channelCount,
			opusConfig->streams,
			opusConfig->coupledStreams,
			opusConfig->mapping,
			&err);
	if (s_OpusDecoder == NULL)
	{
		ArCleanup();
		return -1;
	}

	// We know ahead of time what the buffer size will be for decoded audio, so pre-allocate it
	s_AudioFrameBufferSize = opusConfig->channelCount * PCM_FRAME_SIZE * sizeof(opus_int16);
	s_AudioFrameBuffer = (char *)malloc(s_AudioFrameBufferSize);

	return err;
}

void ArStart()
{
	s_AudioRenderer->Start();
}

void ArStop()
{
	s_AudioRenderer->Stop();
}

void ArCleanup()
{
	if (s_OpusDecoder != NULL)
	{
		opus_multistream_decoder_destroy(s_OpusDecoder);
		s_OpusDecoder = NULL;
	}

	if (s_AudioFrameBuffer != NULL)
	{
		free(s_AudioFrameBuffer);
		s_AudioFrameBuffer = NULL;
		s_AudioFrameBufferSize = 0;
	}

	s_AudioRenderer->Cleanup();
}

void ArDecodeAndPlaySample(char *sampleData, int sampleLength)
{
	int decodeLen =
		opus_multistream_decode(
			s_OpusDecoder,
			(const unsigned char*)sampleData,
			sampleLength,
			(opus_int16*)s_AudioFrameBuffer,
			PCM_FRAME_SIZE,
			0);
	if (decodeLen > 0)
	{
		s_AudioRenderer->HandleFrame(ArrayReference<unsigned char>((unsigned char *)s_AudioFrameBuffer, s_AudioFrameBufferSize));
	}
}

void ClStageStarting(int stage)
{
	String^ stageName = CStringToPlatformString(LiGetStageName(stage));
	s_ConnectionListener->StageStarting(stageName);
}

void ClStageComplete(int stage)
{
	String^ stageName = CStringToPlatformString(LiGetStageName(stage));
	s_ConnectionListener->StageComplete(stageName);
}

void ClStageFailed(int stage, long errorCode)
{
	String^ stageName = CStringToPlatformString(LiGetStageName(stage));
	s_ConnectionListener->StageFailed(stageName, errorCode);
}

void ClConnectionStarted()
{
	s_ConnectionListener->ConnectionStarted();
}

void ClConnectionTerminated(long errorCode)
{
	s_ConnectionListener->ConnectionTerminated(errorCode);
}

void ClDisplayMessage(const char* message)
{
	s_ConnectionListener->DisplayMessage(CStringToPlatformString(message));
}

void ClDisplayTransientMessage(const char* message)
{
	s_ConnectionListener->DisplayTransientMessage(CStringToPlatformString(message));
}

void ClLogMessage(const char* format, ...)
{
	char message[1024];
	va_list va;
	va_start(va, format);
	vsnprintf(message, 1024, format, va);
	va_end(va);

	s_ConnectionListener->LogMessage(CStringToPlatformString(message));
}

int MoonlightCommonInterop::StartConnection(
	IVideoRenderer^ videoRenderer,
	IAudioRenderer^ audioRenderer,
	IConnectionListener^ connectionListener)
{
	s_VideoRenderer = videoRenderer;
	s_AudioRenderer = audioRenderer;
	s_ConnectionListener = connectionListener;

	return 0;
}