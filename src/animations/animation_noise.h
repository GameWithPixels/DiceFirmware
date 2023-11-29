#pragma once

#include "animations/Animation.h"
#include "data_set/data_animation_bits.h"
#include "settings.h"

#pragma pack(push, 1)

namespace Animations
{
	/// <summary>
	/// Procedural noise animation
	/// </summary>
	struct AnimationNoise
		: public Animation
	{
		uint16_t overallGradientTrackOffset;
		uint16_t individualGradientTrackOffset;
		uint16_t blinkFrequencyTimes1000; // per seconds
		uint16_t blinkFrequencyVarTimes1000; // per seconds
		uint16_t blinkDurationMs;
	};

	/// <summary>
	/// Procedural noise animation instance data
	/// </summary>
	class AnimationInstanceNoise
		: public AnimationInstance
	{
	public:
		AnimationInstanceNoise(const AnimationNoise* preset, const DataSet::AnimationBits* bits);
		virtual ~AnimationInstanceNoise();
		virtual int animationSize() const;

		virtual void start(int _startTime, uint8_t _remapFace, bool _loop);
		virtual int updateLEDs(int ms, int retIndices[], uint32_t retColors[]);
		virtual int stop(int retIndices[]);

	private:
		
		const AnimationNoise* getPreset() const;
		int nextBlinkTime;
		int blinkStartTimes[MAX_COUNT];		// state that keeps track of the start of every individual blink so as to know how to fade it based on the time
		int blinkDurations[MAX_COUNT];	// keeps track of the duration of each individual blink, so as to add a bit of variation 
		int ledCount; 					// int that keeps track of how many led's the circuit board has
		int blinkInterValMinMs;
		int blinkInterValDeltaMs;
	};
}

#pragma pack(pop)