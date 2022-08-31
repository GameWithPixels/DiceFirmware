#pragma once

#include "animations/Animation.h"
#include "data_set/data_animation_bits.h"
#include "data_set/data_set.h"

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

		uint16_t blinkCount;	

		uint8_t blinkDuration;	

		uint8_t fade;

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
		int previousBlinkTime = 0;				// state keeping track of the last time we turned on a set of faces
		int individualBlinkTimes[MAX_LED_COUNT];// state that keeps track of the start of every individual blink so as to know how to fade it based on the time
		uint16_t blinkDurations[MAX_LED_COUNT]; // keeps track of the duration of each individual blink, so as to add a bit of variation 
		uint16_t curRand;						// rand variable for generating new random variables using nextRand from Utils.h
		int ledCount; 							// int that keeps track of how many led's the circuit board has
	};
}

#pragma pack(pop)