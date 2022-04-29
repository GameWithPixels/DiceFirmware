#pragma once

#include "animations/Animation.h"

#pragma pack(push, 1)

#define NAME_FACEMASK	0xFFFFF
#define NAME_COUNT		32

namespace Animations
{
	/// <summary>
	/// Procedural on off animation
	/// </summary>
	struct AnimationName
		: public Animation
	{
		uint8_t preamble_count;
		uint8_t brightness;
	};

	/// <summary>
	/// Procedural on off animation instance data
	/// </summary>
	class AnimationInstanceName
		: public AnimationInstance
	{
	private:
		uint8_t counter;
		uint16_t last_bit;
		bool skip;
	public:
		AnimationInstanceName(const AnimationName* preset, const DataSet::AnimationBits* bits);
		virtual ~AnimationInstanceName();
		virtual int animationSize() const;

		virtual void start(int _startTime, uint8_t _remapFace, bool _loop);
		virtual int updateLEDs(int ms, int retIndices[], uint32_t retColors[]);
		virtual int stop(int retIndices[]);

	private:
		const AnimationName* getPreset() const;
	};
}

#pragma pack(pop)