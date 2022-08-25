#pragma once

#include <stdint.h>

#pragma pack(push, 1)

namespace DataSet
{
	struct AnimationBits;
}

#define ANIM_FACEMASK_ALL_LEDS 0xFFFFF

namespace Animations
{
	/// <summary>
	/// Defines the types of Animation Presets we have/support
	/// </summary>
	enum AnimationType : uint8_t
	{
		Animation_Unknown = 0,
		Animation_Simple,
		Animation_Rainbow,
		Animation_Keyframed,
		Animation_GradientPattern,
		Animation_Gradient,
		Animation_Noise,
		Animation_Cycle,
		Animation_Name,
	};

	/// <summary>
	/// Base struct for animation presets. All presets have a few properties in common.
	/// Presets are stored in flash, so do not have methods or vtables or anything like that.
	/// </summary>
	struct Animation
	{
		AnimationType type;
		uint8_t padding_type; // to keep duration 16-bit aligned
		uint16_t duration; // in ms
	};

	/// <summary>
	/// Animation instance data, refers to an animation preset but stores the instance data and
	/// (derived classes) implements logic for displaying the animation.
	/// </summary>
	class AnimationInstance
	{
	public:
		Animation const * animationPreset;
		const DataSet::AnimationBits* animationBits;
		int startTime; //ms
		uint8_t remapFace;
		bool loop;

	protected:
		AnimationInstance(const Animation* preset, const DataSet::AnimationBits* bits);

	public:
		virtual ~AnimationInstance();
		// starts the animation, with the option of repeating it if _loop is set
		virtual void start(int _startTime, uint8_t _remapFace, bool _loop);
		virtual int animationSize() const = 0;
		// method used to set which faces to turn on as well as the color of their LEDS
		// retIndices is one to one with retColors and keeps track of which face to turn on as well as its corresponding color
		// return value of the method is the number of faces to turn on
		virtual int updateLEDs(int ms, int retIndices[], uint32_t retColors[]) = 0;
		virtual int stop(int retIndices[]) = 0;
		// sets all of the LEDS that satisfy the face mask (eg: all LEDS on = 0x000FFFFF) to the given color and then stores this information in retIndices and retColors
		int setColor(uint32_t color, uint32_t faceMask, int retIndices[], uint32_t retColors[]);
		// sets all indices that satisfy the facemask and stores the info in retIndices
		int setIndices(uint32_t faceMask, int retIndices[]);
	};

	Animations::AnimationInstance* createAnimationInstance(int animationIndex);
	Animations::AnimationInstance* createAnimationInstance(const Animations::Animation* preset, const DataSet::AnimationBits* bits);
	void destroyAnimationInstance(Animations::AnimationInstance* animationInstance);

}

#pragma pack(pop)
