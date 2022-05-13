#pragma once

#include "animations/Animation.h"

#pragma pack(push, 1)

namespace Animations
{
	/// <summary>
	/// Procedural animation that starts with a white preamble and then
	/// blinks a message on all LEDs using a color scheme.
	/// The message is composed of a header, a CRC and the Device Id.
	/// Each blink lasts for the given number of frames.
	/// Use setDuration() to compute the correct duration.
	/// </summary>
	struct AnimationName
		: public Animation
	{
		uint8_t framesPerBlink;
		uint8_t brightness;

		void setDuration(uint16_t preambleDuration);
	};

	/// <summary>
	/// Procedural on off animation instance data
	/// </summary>
	class AnimationInstanceName
		: public AnimationInstance
	{
	public:
		AnimationInstanceName(const AnimationName* preset, const DataSet::AnimationBits* bits);
		virtual ~AnimationInstanceName();
		virtual int animationSize() const;

		virtual void start(int _startTime, uint8_t _remapFace, bool _loop);
		virtual int updateLEDs(int ms, int retIndices[], uint32_t retColors[]);
		virtual int stop(int retIndices[]);

	private:
		const AnimationName* getPreset() const;
		static uint64_t getMessage();
		const uint64_t message;
	};
}

#pragma pack(pop)