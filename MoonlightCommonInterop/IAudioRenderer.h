#pragma once

namespace Moonlight
{
	namespace Xbox
	{
		namespace Binding
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