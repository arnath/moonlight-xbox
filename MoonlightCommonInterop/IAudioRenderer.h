#pragma once

namespace Moonlight
{
	namespace Xbox
	{
		namespace Interop
		{
			using namespace Platform;

			public interface class IAudioRenderer
			{
				property int Capabilities;

				int Initialize(int audioFormat);

				void Start();

				void Stop();

				void Cleanup();

				void HandleFrame(const Array<unsigned char>^ frameData);
			};
		}
	}
}