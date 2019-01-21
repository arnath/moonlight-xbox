#pragma once

namespace Moonlight
{
	namespace Xbox
	{
		namespace Interop
		{
			using namespace Platform;

			public interface class IConnectionListener
			{
				void StageStarting(String^ stage);

				void StageComplete(String^ stage);

				void StageFailed(String^ stage, __int64 errorCode);

				void ConnectionStarted();

				void ConnectionTerminated(__int64 errorCode);

				void DisplayMessage(String^ message);

				void DisplayTransientMessage(String^ message);

				void LogMessage(String^ message);
			};
		}
	}
}