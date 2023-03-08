#pragma once

#include "core/ring_buffer.h"
#include "core/float3.h"
#include "core/delegate_array.h"

/// <summary>
/// The component in charge of maintaining the acceleration readings,
/// and determining die motion state.
/// </summary>
namespace Modules::Accelerometer
{
	using Core::float3;
	enum RollState : uint8_t
	{
		RollState_Unknown = 0,
		RollState_OnFace,
		RollState_Handling,
		RollState_Rolling,
		RollState_Crooked,
		RollState_Count
	};

	#pragma pack(push, 1)

	/// <summary>
	/// Small struct holding a single frame of accelerometer data
	/// used for both face detection (not that kind) and telemetry
	/// Size is 50-bytes
	/// </summary>
	struct AccelFrame
	{
		float3 acc;
		float3 jerk;
		float3 smoothAcc;
		float sigma;
		float faceConfidence;
		uint32_t time;
		RollState rollState;
		uint8_t face;
	};

	#pragma pack(pop)

	bool init();
	void start();
	void stop();
	void lowPower();
	void wakeUp();

	int currentFace();
	float currentFaceConfidence();
	RollState currentRollState();

	// Returns empty string in release builds so to save space
	const char *getRollStateString(RollState state);

	void readAccelerometer(float3* acc);

	typedef void(*AccelerometerInterruptMethod)(void* param);
	void enableInterrupt(AccelerometerInterruptMethod callback, void* param);
	void disableInterrupt();

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
