#pragma once

#include "animations/Animation.h"

#pragma pack(push, 1)

namespace Animations
{
	/// <summary>
	/// Procedural rainbow animation that cycle faces
	/// </summary>
	struct AnimationCycle
		: public Animation
	{
		uint32_t faceMask;
        uint8_t count;
        uint8_t fade;
		uint8_t intensity;
		uint8_t cyclesTimes10;
		uint16_t gradientTrackOffset;
	};

	/// <summary>
	/// Procedural rainbow animation instance data
	/// </summary>
	class AnimationInstanceCycle
		: public AnimationInstance
	{
	public:
		AnimationInstanceCycle(const AnimationCycle *preset, const DataSet::AnimationBits *bits);
		virtual ~AnimationInstanceCycle();
		virtual int animationSize() const;

		virtual void start(int _startTime, uint8_t _remapFace, uint8_t _loopCount);
		virtual int updateLEDs(int ms, int retIndices[], uint32_t retColors[]);
		virtual int stop(int retIndices[]);

	private:
		const AnimationCycle *getPreset() const;
	};
}

#pragma pack(pop)