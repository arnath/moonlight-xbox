#pragma once

namespace Moonlight
{
	namespace Xbox
	{
		namespace Binding
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