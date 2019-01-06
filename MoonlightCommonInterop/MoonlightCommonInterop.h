#pragma once

#include "Limelight.h"
#include "IVideoRenderer.h"
#include "IAudioRenderer.h"

namespace Moonlight
{
	namespace Xbox
	{
		namespace Binding
		{
			public ref class MoonlightCommonInterop sealed
			{
			public:
				MoonlightCommonInterop(
					IVideoRenderer^ videoRenderer,
					IAudioRenderer^ audioRenderer);

				int StartConnection();

			private:
				// Decoder Renderer callbacks
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

				// Audio Renderer callbacks
				int ArInit(
					int audioConfiguration,
					const POPUS_MULTISTREAM_CONFIGURATION opusConfig,
					void* context,
					int arFlags);
				void ArStart();
				void ArStop();
				void ArCleanup();
				void ArDecodeAndPlaySample(char *sampleData, int sampleLength);

				IVideoRenderer^ _videoRenderer;
				IAudioRenderer^ _audioRenderer;
			};
		}
	}
}