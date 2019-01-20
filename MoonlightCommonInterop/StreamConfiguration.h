#pragma once

namespace Moonlight
{
	namespace Xbox
	{
		namespace Interop
		{
			using namespace Platform;

			public ref class StreamConfiguration sealed
			{
			public:
				property int Width;

				property int Height;

				property int Fps;

				property int Bitrate;

				property int PacketSize;

				property int StreamingRemotely;

				property int AudioConfiguration;

				property bool SupportsHevc;

				property bool EnableHdr;

				property int HevcBitratePercentageMultiplier;

				property int ClientRefreshRateX100;

				property Array<unsigned char>^ RemoteInputAesKey;

				property Array<unsigned char>^ RemoteInputAesIv;
			};
		}
	}
}