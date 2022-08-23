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
		uint8_t flashSpeed;	
		uint32_t faceMask;				
		uint16_t overallGradientTrackOffset;	
		uint16_t individualGradientTrackOffset;
		uint16_t flashDelay;			// constant that decides the time between new flashes, aka the time at which we light up a new set of 1-2 faces on the dice

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
		const uint8_t maxI = 0xFF;
		const AnimationNoise* getPreset() const;
		struct DataSet::AnimationBits animationBits;
		uint8_t animPalette[21] = {0x00, 0x00, 0x00, maxI, maxI, maxI, 0x00, maxI, 0x00, maxI, 0x00, 0x00, 0x00, 0x00, maxI, maxI, 0x00, maxI, 0x00, maxI, maxI};
		Animations::RGBKeyframe rgbKeyframes[10];
		Animations::RGBTrack rgbTracks[2];
		int past_time = 0;
		uint8_t individualFlashTimes[20];
		Animations::Keyframe keyframes[9];
		Animations::Track intensityTracks;
		//uint8_t gradientTimes[20];
	};
}

#pragma pack(pop)