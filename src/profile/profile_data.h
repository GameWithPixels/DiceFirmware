#pragma once

#include "stdint.h"
#include "animations/animation.h"
#include "behaviors/condition.h"
#include "behaviors/action.h"
#include "behaviors/behavior.h"
#include "profile_buffer.h"
#include "animations/animation_parameters.h"

#define PROFILE_VALID_KEY (0x600DF00D) // Good Food ;)
#define PROFILE_VERSION 5


namespace Profile
{
	// The profile data is actually dynamically sized
	// This class sits at the beginning of the profile and 
	// lets us know where to fetch the list of rules and animations
	struct Header
	{
		// Indicates whether there is valid data!!!
		uint32_t headerStartMarker;
		uint32_t version;
		uint16_t bufferSize; // Starting after the header!

        // The animations. Because animations can be one of multiple classes (simple inheritance system)
        // The dataset stores an offset into the animations buffer for each entry. The first member of
        // The animation base class is a type enum indicating what it actually is.
		// The buffer is after these members
		Array<Animations::AnimationPtr> animations;

        // Rules are pairs or conditions and actions
		Array<Behaviors::Rule> rules;

		uint32_t headerEndMarker;
		// Following this is the remainder of the buffer
		// and hash value
	};

	// This describes the entire profile, header + buffer (which is variable size) + hash
	struct Data
	{
	//private:
		// First is the header
		// Next is the data (variable size)
		// Next is the hash value (uint32_t)
		const Header* header;

		// Cached data set on init() and reflash
		BufferDescriptor buffer;
		uint32_t size;
		uint32_t hash;

	public:
		Data();

		uint32_t getSize() const;
		uint32_t getHash() const;
        BufferDescriptor getBuffer() const;

		bool init(const Header* theHeader);
		bool checkValid() const;

		// Animations
		const Animations::Animation* getAnimation(int animationIndex) const;
		uint16_t getAnimationCount() const;

		// Rules
		const Behaviors::Rule* getRule(int ruleIndex) const;
		uint16_t getRuleCount() const;

		// Helpers to fetch conditions and actions
		const Behaviors::Condition* getCondition(Pointer<Behaviors::Condition> conditionPtr) const;
		void TriggerAction(Behaviors::ActionPtr action, Animations::AnimationTag tag) const;
		void TriggerActions(Array<Behaviors::ActionPtr> actions, Animations::AnimationTag tag) const;

		// Helper method to create a default profile data, the returned pointer needs to be freed after use.
		// Typically this is called to create a default profile, then that profile is programmed in flash
		// and can be freed.
		static uint8_t* CreateDefaultProfile(uint32_t* outSize);
		static void DestroyDefaultProfile(uint8_t* ptr);
	};

}