#pragma once

#include "core/ring_buffer.h"
#include "core/float3.h"
#include "core/delegate_array.h"

namespace Modules
{
	/// <summary>
	/// The component in charge of maintaining the acceleraion readings,
	/// and determining die motion state.
	/// </summary>
	namespace Accelerometer
	{
	    enum RollState : uint8_t
		{
			RollState_Unknown = 0,
			RollState_OnFace,
			RollState_Handling,
			RollState_Rolling,
			RollState_Crooked,
			RollState_Count
		};

		/// <summary>
		/// Small struct holding a single frame of accelerometer data
		/// used for both face detection (not that kind) and telemetry
        /// Size is 50-bytes
		/// </summary>
		struct AccelFrame
		{
			Core::float3 acc;
			Core::float3 jerk;
			Core::float3 smoothAcc;
			float sigma;
			float faceConfidence;
			uint32_t time;
            RollState rollState;
			uint8_t face;
		};

		int determineFace(Core::float3 acc, float* outConfidence = nullptr);

		void init();
		void start();
		void stop();

		int currentFace();
		float currentFaceConfidence();
		RollState currentRollState();

		// Returns empty string in release builds so to save space
		const char *getRollStateString(RollState state);

        void readAccelerometer(Core::float3* acc);
        void enableInterrupt();
        void disableInterrupt();
        void clearInterrupt();
        bool checkIntPin();


		// Notification management
		typedef void(*FrameDataClientMethod)(void* param, const AccelFrame& accelFrame);
		void hookFrameData(FrameDataClientMethod method, void* param);
		void unHookFrameData(FrameDataClientMethod client);
		void unHookFrameDataWithParam(void* param);

		typedef void(*RollStateClientMethod)(void* param, RollState newState, int newFace);
		void hookRollState(RollStateClientMethod method, void* param);
		void unHookRollState(RollStateClientMethod client);
		void unHookRollStateWithParam(void* param);
	}
}


