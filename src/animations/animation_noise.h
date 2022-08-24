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
		uint16_t speedMultiplier256;
		uint16_t overallGradientTrackOffset;		
			
		uint16_t blinkSpeedMultiplier256;
		uint16_t individualGradientTrackOffset;

		uint16_t flashCount;	

		uint8_t flashDuration;	

		uint8_t fade;
		uint32_t faceMask;

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
		int past_time = 0;
		int individualFlashTimes[20];
		uint16_t flashDurations[20];
		uint16_t curRand;

		const uint8_t maxI = 0xFF;
		DataSet::AnimationBits animationBits;
		uint8_t animPalette[21] = {0x00, 0x00, 0x00, maxI, maxI, maxI, 0x00, maxI, 0x00, maxI, 0x00, 0x00, 0x00, 0x00, maxI, maxI, 0x00, maxI, 0x00, maxI, maxI};
		Animations::RGBKeyframe rgbKeyframes[10];
		Animations::RGBTrack rgbTracks[2];
		Animations::Keyframe keyframes[9];
		Animations::Track intensityTracks;
	};
}

#pragma pack(pop)