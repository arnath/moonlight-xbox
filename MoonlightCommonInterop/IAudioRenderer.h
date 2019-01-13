#pragma once

// C callback declarations
int ArInit(
	int audioConfiguration,
	const POPUS_MULTISTREAM_CONFIGURATION opusConfig,
	void* context,
	int arFlags);
void ArStart();
void ArStop();
void ArCleanup();
void ArDecodeAndPlaySample(char *sampleData, int sampleLength);

namespace Moonlight
{
	namespace Xbox
	{
		namespace Interop
		{
			using namespace Platform;

			public interface class IAudioRenderer
			{
				int Initialize(int audioFormat);

				void Start();

				void Stop();

				void Cleanup();

				void HandleFrame(const Array<unsigned char>^ frameData);
			};
		}
	}
}