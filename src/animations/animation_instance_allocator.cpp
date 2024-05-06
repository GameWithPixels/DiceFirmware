#include "animation_instance_allocator.h"

#include "nrf_log.h"
#include "malloc.h"
#include "animation.h"
#include "ledpattern.h"
#include "config/dice_variants.h"
#include "animation_context.h"

#include "animations/animation_flashes.h"
#include "animations/animation_blinkid.h"
#include "animations/animation_pattern.h"
#include "animations/animation_rainbow.h"
#include "animations/animation_sequence.h"

using namespace Config;

namespace Animations
{
    uint16_t AnimationInstanceAllocator::getLEDPatternInstanceSize(LEDPattern const * pattern) const {
        switch (pattern->type) {
        case LEDPatternType_Unknown:
            NRF_LOG_ERROR("Unknown pattern type");
            return sizeof(LEDPatternInstance);
        case LEDPatternType_Keyframes:
            return sizeof(LEDPatternKeyframesInstance);
        case LEDPatternType_Noise:
            return sizeof(LEDPatternNoiseInstance);
        default:
            NRF_LOG_ERROR("Unhandled pattern type");
            return sizeof(LEDPatternInstance);
        }
    }

    uint16_t AnimationInstanceAllocator::getAnimationInstanceSize(Animation const * animation) const {
        uint16_t ret = 0;
        switch (animation->type) {
            case AnimationType_Unknown:
                NRF_LOG_ERROR("Unknown animation type");
                break;
            case AnimationType_Flashes:
                ret = sizeof(AnimationFlashesInstance);
                break;
            case AnimationType_Rainbow:
                ret = sizeof(AnimationRainbowInstance);
                break;
            case AnimationType_BlinkID:
                ret = sizeof(AnimationBlinkIdInstance);
                break;
            case AnimationType_Pattern:
            {
                ret = sizeof(AnimationPatternInstance);
                auto animPattern = (AnimationPattern const*)animation;
                ret += getLEDPatternInstanceSize(context->getParameter(animPattern->pattern));
            }
            case AnimationType_Sequence:
                ret = sizeof(AnimationSequenceInstance);
                break;
            default:
                NRF_LOG_ERROR("Unsupported clip type");
                break;
        }
        return ret;
    }

    LEDPatternInstance* AnimationInstanceAllocator::CreateLEDPatternInstance(LEDPattern const * pattern) {
        switch (pattern->type) {
        case LEDPatternType_Unknown:
            NRF_LOG_ERROR("Unknown pattern type");
            return nullptr;
        case LEDPatternType_Keyframes:
            {
                // "allocate" the animation led pattern instance
                auto ret = instanceBufferAllocate<LEDPatternKeyframesInstance>();

                // Initialize the layer:
                ret->context = context;
                ret->pattern = pattern;
                return ret;
            }
        case LEDPatternType_Noise:
            {
                // "allocate" the animation led pattern instance
                auto ret = instanceBufferAllocate<LEDPatternNoiseInstance>();

                // Initialize the layer:
                ret->context = context;
                ret->pattern = pattern;
                return ret;
            }
        default:
            NRF_LOG_ERROR("Unhandled pattern type");
            return nullptr;
        }
    }

    AnimationInstance* AnimationInstanceAllocator::CreateAnimationInstance(Animation const * animation) {
        AnimationInstance* ret = nullptr;
        switch (animation->type) {
            case AnimationType_Unknown:
                NRF_LOG_ERROR("Unknown clip type");
                break;
            case AnimationType_Flashes:
                // "allocate" the animation clip instance
                ret = instanceBufferAllocate<AnimationFlashesInstance>();
                break;
            case AnimationType_Rainbow:
                // "allocate" the animation clip instance
                ret = instanceBufferAllocate<AnimationRainbowInstance>();
                break;
            case AnimationType_BlinkID:
                // "allocate" the animation clip instance
                ret = instanceBufferAllocate<AnimationBlinkIdInstance>();
                break;
            case AnimationType_Pattern:
                {
                    // "allocate" the animation clip instance
                    auto pi = instanceBufferAllocate<AnimationPatternInstance>();
                    ret = pi;

                    // then "allocate" the led pattern instance
                    auto animPattern = (AnimationPattern const*)animation;
                    auto dp = CreateLEDPatternInstance(context->getParameter(animPattern->pattern));
                    if (dp == nullptr) {
                        ret = nullptr;
                    } else {
                        pi->patternInstance = dp;
                    }
                }
                break;
            case AnimationType_Sequence:
                // "allocate" the animation clip instance
                ret = instanceBufferAllocate<AnimationSequenceInstance>();
                break;
            default:
                NRF_LOG_ERROR("Unsupported clip type");
                break;
        }

        ret->animationPreset = animation;
        ret->context = context;
        return ret;
    }

    AnimationInstanceAllocator::AnimationInstanceAllocator(
        const AnimationContextGlobals* theGlobals,
        Profile::BufferDescriptor theBuffer,
        Profile::BufferDescriptor theOverrideBuffer,
        Profile::Array<ParameterOverride> theOverrides)
        : globals(theGlobals)
        , contextBuffer(theBuffer)
        , contextOverrideBuffer(theOverrideBuffer)
        , contextOverrides(theOverrides)
        , context(nullptr)
        , instanceBuffer(nullptr)
        , instanceCurrent(nullptr)
        , instanceBufferSize(0)
    {
    }

    AnimationInstanceAllocator::AnimationInstanceAllocator(
        const AnimationContextGlobals* theGlobals,
        Profile::BufferDescriptor theBuffer)
        : globals(theGlobals)
        , contextBuffer(theBuffer)
        , contextOverrideBuffer(Profile::BufferDescriptor::nullDescriptor)
        , contextOverrides(Profile::Array<ParameterOverride>::emptyArray())
        , context(nullptr)
        , instanceBuffer(nullptr)
        , instanceCurrent(nullptr)
        , instanceBufferSize(0)
    {
    }


    AnimationInstance* AnimationInstanceAllocator::CreateInstance(Animation const* animation) {
        AnimationInstance* ret = nullptr;

        // Create temporary context to compute necessary memory size
        AnimationContext computeSizeContext(globals, contextBuffer, contextOverrideBuffer, contextOverrides);
        // Temporarily set the stored context to this local variable while we determine the memory necessary
        context = &computeSizeContext;
        uint16_t animationInstanceSize = getAnimationInstanceSize(animation);
        context = nullptr;

        // Allocate enough memory for the instance and associated context
        instanceBufferSize = animationInstanceSize + sizeof(AnimationContext);
        instanceBuffer = (uint8_t*)malloc(instanceBufferSize);
        if (instanceBuffer != nullptr) {
            // Initialize the context part of this buffer
            auto instanceContextAddr = (void*)(instanceBuffer + animationInstanceSize);
            context = new (instanceContextAddr) AnimationContext(globals, contextBuffer, contextOverrideBuffer, contextOverrides);

            // Set the initial pointer to the start of the buffer we just allocated
            instanceCurrent = instanceBuffer;
            // Create the actual animation instance
            ret = CreateAnimationInstance(animation);
        }
        return ret;
    }

    void AnimationInstanceAllocator::DestroyInstance(AnimationInstance* instance) {
        // The instance pointer is the pointer to the entire buffer,
        // for the instance itself and everything associated.
        // So all we need to do is free the memory!
        free(instance);
    }
}
