#include "animation_sequence.h"
#include "utils/utils.h"
#include "config/board_config.h"
#include "data_set/data_animation_bits.h"

#include "data_set/data_set.h"
#include "modules/anim_controller.h"
#include "nrf_log.h"
#include "drivers_nrf/scheduler.h"

using namespace Modules;
using namespace DriversNRF;

namespace Animations
{
    /// <summary>
    /// constructor for sequence
    /// Needs to have an associated preset passed in
    /// </summary>
    AnimationInstanceSequence::AnimationInstanceSequence(const AnimationSequence* preset, const DataSet::AnimationBits* bits)
        : AnimationInstance(preset, bits) {
    }

    /// <summary>
    /// destructor
    /// </summary>
    AnimationInstanceSequence::~AnimationInstanceSequence() {
    }

    /// <summary>
    /// Small helper to return the expected size of the preset data
    /// </summary>
    int AnimationInstanceSequence::animationSize() const {
        return sizeof(AnimationSequence);
    }

    /// <summary>
    /// (re)Initializes the instance to animate LEDs. This can be called on a reused instance.
    /// </summary>
    void AnimationInstanceSequence::start(int _startTime, uint8_t _remapFace, uint8_t _loopCount) {
        AnimationInstance::start(_startTime, _remapFace, _loopCount);
        lastMillis = -1;
        processAnimations(_startTime);
    }

    /// <summary>
    /// Computes the list of LEDs that need to be on, and what their intensities should be.
    /// </summary>
    /// <param name="ms">The animation time (in milliseconds)</param>
    /// <param name="retIndices">the return list of LED indices to fill, max size should be at least 21, the max number of LEDs</param>
    /// <param name="retColors">the return list of LED color to fill, max size should be at least 21, the max number of LEDs</param>
    /// <returns>The number of LEDs/intensities added to the return array</returns>
    int AnimationInstanceSequence::updateLEDs(int ms, int retIndices[], uint32_t retColors[]) {
        processAnimations(ms);
        return 0;
    }

    /// <summary>
    /// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
    /// </summary>
    int AnimationInstanceSequence::stop(int retIndices[]) {
        return 0;
    }

    const AnimationSequence* AnimationInstanceSequence::getPreset() const {
        return static_cast<const AnimationSequence*>(animationPreset);
    }

    void AnimationInstanceSequence::processAnimations(int ms) {
        auto preset = getPreset();
        int lastMs = lastMillis - startTime;
        int thisMs = ms - startTime;
        lastMillis = ms;
        for (int i = 0; i < preset->animationCount; i++) {
            int delay = preset->animations[i].animationDelay;
            if (delay > lastMs && delay <= thisMs) {

                NRF_LOG_INFO("Starting animation %d", preset->animations[i].animationIndex);

                struct TriggeredAnimation
                {
                    const DataSet::AnimationBits* bits;
                    uint16_t animIndex;
                    uint8_t remapFace;
                };

                TriggeredAnimation triggeredAnimation = 
                {
                    animationBits,
                    preset->animations[i].animationIndex,
                    remapFace
                };
                Scheduler::push(&triggeredAnimation, sizeof(TriggeredAnimation), [](void *p_event_data, uint16_t event_size) {
                    auto ta = (TriggeredAnimation*)p_event_data;
                    // Start the animation
                    auto anim = ta->bits->getAnimation(ta->animIndex);
                    AnimController::play(anim, ta->bits, ta->remapFace, 1);
                });
            }
        }
    }
}
