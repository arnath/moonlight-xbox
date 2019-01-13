#pragma once

// C callback declarations
void ClStageStarting(int stage);
void ClStageComplete(int stage);
void ClStageFailed(int stage, long errorCode);
void ClConnectionStarted();
void ClConnectionTerminated(long errorCode);
void ClDisplayMessage(const char* message);
void ClDisplayTransientMessage(const char* message);
void ClLogMessage(const char* format, ...);

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