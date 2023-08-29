#include "profile_data.h"
#include "assert.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "app_error.h"

using namespace Animations;
using namespace Behaviors;

namespace Profile
{
	Data::Data()
	: buffer(BufferDescriptor::nullDescriptor)
	, size(0)
	, hash(0)
	{
	}

	uint32_t Data::getSize() const {
		return size;
	}

	uint32_t Data::getHash() const {
		return hash;
	}

    BufferDescriptor Data::getBuffer() const {
		return buffer;
    }

	bool Data::init(const Header* theHeader) {
		// Check if the Profile is valid!
		header = theHeader;
		bool valid = header->headerStartMarker == PROFILE_VALID_KEY && header->headerEndMarker == PROFILE_VALID_KEY;
		if (valid) {
			// Data seems valid, check the hash
			uint32_t headerAddress = reinterpret_cast<uint32_t>(header);
			uint32_t bufferAddress = headerAddress + sizeof(Header);
			uint32_t hashAddress = bufferAddress + header->bufferSize;
			hash = *((uint32_t const *)hashAddress);
			uint32_t headerAndBufferSize = sizeof(Header) + header->bufferSize;
			uint32_t headerHash = Utils::computeHash((const uint8_t*)header, headerAndBufferSize);
			valid = headerHash == hash;
			if (valid) {
				// Compute total flash size
				size = headerAndBufferSize + sizeof(uint32_t); // header + buffer + hash

				// tail seems valid too, check the hash
				buffer.start = reinterpret_cast<uint8_t const*>(bufferAddress);
				buffer.size = header->bufferSize;

				// Finally, check the version number
				valid = (header->version == PROFILE_VERSION);
			}
		}

		if (!valid) {
			// Just to be sure, reset hash and size
			header = nullptr;
			buffer = BufferDescriptor::nullDescriptor;
			size = 0;
			hash = 0;
		}
		return valid;
	}

	bool Data::checkValid() const {
		return header != nullptr;
	}

    uint8_t Data::getBrightness() const {
        return 255; // TODO implement brightness
    }

    const Animation* Data::getAnimation(int animationIndex) const {
		auto animPtr = header->animations.getAt(getBuffer(), animationIndex);
		return animPtr->get(getBuffer());
	}

	uint16_t Data::getAnimationCount() const{
		return header->animations.length;
	}

	const Rule* Data::getRule(int ruleIndex) const {
		return header->rules.getAt(getBuffer(), ruleIndex);
	}

	uint16_t Data::getRuleCount() const {
		return header->rules.length;
	}

	const Condition* Data::getCondition(Pointer<Condition> conditionPtr) const {
		return conditionPtr.get(getBuffer());
	}

    void Data::triggerAction(Behaviors::ActionPtr action, Animations::AnimationTag tag) const {
    	Behaviors::triggerAction(getBuffer(), action, tag);
	}

    void Data::triggerActions(Array<Behaviors::ActionPtr> actions, Animations::AnimationTag tag) const {
		for (int i = 0; i < actions.length; ++i) {
    		Behaviors::triggerAction(getBuffer(), *actions.getAt(getBuffer(), i), tag);
		}
	}

}