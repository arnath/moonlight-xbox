#pragma once

// C callback declarations
int DrSetup(
	int videoFormat,
	int width,
	int height,
	int redrawRate,
	void* context,
	int drFlags);
void DrStart();
void DrStop();
void DrCleanup();
int DrSubmitDecodeUnit(PDECODE_UNIT decodeUnit);

namespace Moonlight
{
	namespace Xbox
	{
		namespace Interop
		{
			using namespace Platform;

			public interface class IVideoRenderer
			{
				int Initialize(int videoFormat, int width, int height, int redrawRate);

				void Start();

				void Stop();

				void Cleanup();

				int HandleFrame(const Array<unsigned char>^ frameData, int frameType, int frameNumber, __int64 receiveTimeMs);
			};
		}
	}
}