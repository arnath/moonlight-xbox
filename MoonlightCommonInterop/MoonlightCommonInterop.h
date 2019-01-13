#pragma once

#include "IVideoRenderer.h"
#include "IAudioRenderer.h"
#include "IConnectionListener.h"

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
					IVideoRenderer^ videoRenderer,
					IAudioRenderer^ audioRenderer,
					IConnectionListener^ connectionListener);
			};
		}
	}
}