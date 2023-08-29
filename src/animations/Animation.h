#pragma once

#include <stdint.h>
#include "animation_tag.h"
#include "animation_context.h"
#include "profile/profile_buffer.h"

#pragma pack(push, 1)

#define ANIM_FACEMASK_ALL_LEDS 0xFFFFFFFF

namespace Animations
{
    enum AnimationType : uint8_t
    {
        AnimationType_Unknown = 0,
		AnimationType_Simple,
        AnimationType_Rainbow,
        AnimationType_BlinkID,
        AnimationType_Pattern,
        AnimationType_Sequence,
        // etc...
    };

	/// <summary>
	/// Flags for the animations, they can be combined.
	/// </summary>
	enum AnimationFlags : uint8_t
	{
		AnimationFlags_None,
		AnimationFlags_Traveling = 1,
		AnimationFlags_UseLedIndices = 2,
	};

	/// <summary>
	/// Base struct for animation presets. All presets have a few properties in common.
	/// Presets are stored in flash, so do not have methods or vtables or anything like that.
	/// </summary>
	struct Animation
	{
		AnimationType type;
		uint8_t animFlags; // Combination of AnimationFlags
		uint16_t duration; // in ms
    };
    typedef Profile::Pointer<Animation> AnimationPtr;

	/// <summary>
	/// Animation instance data, refers to an animation preset but stores the instance data and
	/// (derived classes) implements logic for displaying the animation.
	/// </summary>
    struct AnimationInstance
    {
        // Setup byt the context at creation time
        Animation const * animationPreset;
		AnimationContext* context;

        // Instance-specific data
        uint8_t remapFace;
        AnimationTag tag; // used to identify where the animation came from / what system triggered it
        bool loop;
		int startTime; //ms
		int forceFadeTime; //ms, used when fading out (because anim is being replaced), -1 otherwise

	public:
		// starts the animation, with the option of repeating it if _loop is set
		virtual void start(int _startTime, uint8_t _remapFace, bool _loop);
		// method used to set which faces to turn on as well as the color of their LEDs
		// retIndices is one to one with retColors and keeps track of which face to turn on as well as its corresponding color
		// return value of the method is the number of faces to turn on
		virtual int updateLEDs(int ms, int retIndices[], uint32_t retColors[]) = 0;
		virtual int stop(int retIndices[]) = 0;
		// Set the animation source tag
		void setTag(AnimationTag _tag);
		// sets all of the LEDs that satisfy the face mask (eg: all LEDs on = 0x000FFFFF) to the given color and then stores this information in retIndices and retColors
		int setColor(uint32_t color, uint32_t faceMask, int retIndices[], uint32_t retColors[]);
		// sets all indices that satisfy the facemask and stores the info in retIndices
		int setIndices(uint32_t faceMask, int retIndices[]);
		void forceFadeOut(int fadeOutTime);
	};
}

#pragma pack(pop)
