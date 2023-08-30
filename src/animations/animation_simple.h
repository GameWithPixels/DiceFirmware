#pragma once

#include "animations/animation.h"

#pragma pack(push, 1)

namespace Animations
{
	enum AnimationSimpleFlags : uint8_t
	{
		AnimationSimpleFlags_None			= 0,
		AnimationSimpleFlags_CaptureColor	= 1 << 0,
	};

	/// <summary>
	/// Procedural on off animation
	/// </summary>
	struct AnimationSimple
		: public Animation
	{
		uint8_t colorFlags;
		uint32_t faceMask;
        DColorPtr color;
        uint8_t count;
        uint8_t fade;
	};

	/// <summary>
	/// Procedural on off animation instance data
	/// </summary>
	struct AnimationSimpleInstance
		: public AnimationInstance
	{
	private:
		virtual void start(int _startTime, uint8_t _remapFace, bool _loop);
		virtual int updateLEDs(int ms, int retIndices[], uint32_t retColors[]);
		virtual int stop(int retIndices[]);

	private:
		const AnimationSimple* getPreset() const;

		uint32_t capturedColor;
	};
}

#pragma pack(pop)
