#include "animation_worm.h"
#include "utils/utils.h"
#include "utils/rainbow.h"
#include "config/dice_variants.h"
#include "data_set/data_animation_bits.h"

using namespace Config;

namespace Animations
{
    /// <summary>
    /// constructor for simple animations
    /// Needs to have an associated preset passed in
    /// </summary>
    AnimationInstanceWorm::AnimationInstanceWorm(const AnimationWorm* preset, const DataSet::AnimationBits* bits)
        : AnimationInstance(preset, bits) {
    }

    /// <summary>
    /// destructor
    /// </summary>
    AnimationInstanceWorm::~AnimationInstanceWorm() {
    }

    /// <summary>
    /// Small helper to return the expected size of the preset data
    /// </summary>
    int AnimationInstanceWorm::animationSize() const {
        return sizeof(AnimationWorm);
    }

    /// <summary>
    /// (re)Initializes the instance to animate leds. This can be called on a reused instance.
    /// </summary>
    void AnimationInstanceWorm::start(int _startTime, uint8_t _remapFace, uint8_t _loopCount) {
        AnimationInstance::start(_startTime, _remapFace, _loopCount);
    }

    /// <summary>
    /// Computes the list of LEDs that need to be on, and what their intensities should be.
    /// </summary>
    /// <param name="ms">The animation time (in milliseconds)</param>
    /// <param name="retIndices">the return list of LED indices to fill, max size should be at least 21, the max number of leds</param>
    /// <param name="retColors">the return list of LED color to fill, max size should be at least 21, the max number of leds</param>
    /// <returns>The number of leds/intensities added to the return array</returns>
    int AnimationInstanceWorm::updateLEDs(int ms, int retIndices[], uint32_t retColors[]) {
        auto l = DiceVariants::getLayout();
        int c = l->ledCount;

        auto preset = getPreset();

        // Compute color 
        int fadeTime = preset->duration * preset->fade / (255 * 2);
        int time = (ms - startTime);

        uint8_t intensity = preset->intensity;
        if (time <= fadeTime) {
            // Ramp up
            intensity = (uint8_t)(time * preset->intensity / fadeTime);
        } else if (time >= (preset->duration - fadeTime)) {
            // Ramp down
            intensity = (uint8_t)((preset->duration - time) * preset->intensity / fadeTime);
        }

        // Figure out the color from the gradient
        auto& gradient = animationBits->getRGBTrack(preset->gradientTrackOffset);
        int gradientTime = time * preset->count * 1000 / preset->duration;

        // Fill the indices and colors for the anim controller to know how to update leds
        int retCount = 0;
        for (int i = 0; i < c; ++i) {
            if ((preset->faceMask & (1 << i)) != 0) {
                retIndices[retCount] = i;
                int faceTime = (gradientTime + i * 1000 * preset->cyclesTimes10 / (c * 10)) % 1000;
                retColors[retCount] = Utils::modulateColor(gradient.evaluateColor(animationBits, faceTime), intensity);
                retCount++;
            }
        }
        return retCount;
    }

    /// <summary>
    /// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
    /// </summary>
    int AnimationInstanceWorm::stop(int retIndices[]) {
        auto preset = getPreset();
        return setIndices(preset->faceMask, retIndices);
    }

    const AnimationWorm* AnimationInstanceWorm::getPreset() const {
        return static_cast<const AnimationWorm*>(animationPreset);
    }
}
