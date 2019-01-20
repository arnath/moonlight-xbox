#pragma once

#include "IVideoRenderer.h"
#include "IAudioRenderer.h"
#include "IConnectionListener.h"
#include "StreamConfiguration.h"

namespace Moonlight
{
	namespace Xbox
	{
		namespace Interop
		{
			public ref class MoonlightCommonInterop sealed
			{
			public:
				int StartConnection(
					String^ address,
					String^ appVersion,
					String^ gfeVersion,
					StreamConfiguration^ streamConfiguration,
					IVideoRenderer^ videoRenderer,
					IAudioRenderer^ audioRenderer,
					IConnectionListener^ connectionListener);
			};
		}
	}
}