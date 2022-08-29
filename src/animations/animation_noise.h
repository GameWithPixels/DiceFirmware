#pragma once

#include "animations/Animation.h"
#include "data_set/data_animation_bits.h"

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

		uint16_t flashCount;	

		uint8_t flashDuration;	

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
		int previousFlashTime = 0;				// state keeping track of the last time we turned on a set of faces
		int individualFlashTimes[20];			// state that keeps track of the start of every individual flash so as to know how to fade it based on the time
		uint16_t flashDurations[20];			// keeps track of the duration of each individual flash, so as to add a bit of variation 
		uint16_t curRand;						// rand variable for generating new random variables using nextRand from Utils.h
	};
}

#pragma pack(pop)