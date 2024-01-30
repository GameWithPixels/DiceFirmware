#pragma once

#include "animations/Animation.h"
#include "data_set/data_animation_bits.h"
#include "settings.h"

#pragma pack(push, 1)

namespace Animations
{
	enum NoiseColorOverrideType : uint8_t
	{
		NoiseColorOverrideType_None = 0,
		NoiseColorOverrideType_RandomFromGradient,
		NoiseColorOverrideType_FaceToGradient,
		NoiseColorOverrideType_FaceToRainbowWheel,
	};

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
		uint8_t fade; // 0 - 255
		NoiseColorOverrideType overallGradientColorType; // boolean
		uint16_t overallGradientColorVar; // 0 - 1000
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

		virtual void start(int _startTime, uint8_t _remapFace, uint8_t _loopCount);
		virtual int updateLEDs(int ms, int retIndices[], uint32_t retColors[]);
		virtual int stop(int retIndices[]);

	private:
		
		const AnimationNoise* getPreset() const;
		int nextBlinkTime;
		int blinkStartTimes[MAX_COUNT];		// state that keeps track of the start of every individual blink so as to know how to fade it based on the time
		int blinkDurations[MAX_COUNT];	// keeps track of the duration of each individual blink, so as to add a bit of variation 
		uint32_t blinkColors[MAX_COUNT];
		int ledCount; 					// int that keeps track of how many led's the circuit board has
		int blinkInterValMinMs;
		int blinkInterValDeltaMs;
		int baseColorParam;
	};
}

#pragma pack(pop)