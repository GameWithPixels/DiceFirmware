#include "animation_noise.h"
#include "data_set/data_animation_bits.h"
#include "utils/Utils.h"
#include "config/board_config.h"
#include "nrf_log.h"
#include "drivers_nrf/rng.h"

using namespace DriversNRF;
namespace Animations
{
	/// <summary>
	/// constructor for rainbow animations
	/// Needs to have an associated preset passed in
	/// </summary>
	AnimationInstanceNoise::AnimationInstanceNoise(const AnimationNoise* preset, const DataSet::AnimationBits* bits)
		: AnimationInstance(preset, bits) {
	}

	/// <summary>
	/// destructor
	/// </summary>
	AnimationInstanceNoise::~AnimationInstanceNoise() {
	}

	/// <summary>
	/// Small helper to return the expected size of the preset data
	/// </summary>
	int AnimationInstanceNoise::animationSize() const {
		return sizeof(AnimationNoise);
	}

	/// <summary>
	/// (re)Initializes the instance to animate leds. This can be called on a reused instance.
	/// </summary>
	void AnimationInstanceNoise::start(int _startTime, uint8_t _remapFace, bool _loop) {
		AnimationInstance::start(_startTime, _remapFace, _loop);
		auto preset = getPreset();
        ledCount = Config::BoardManager::getBoard()->ledCount;
		blinkInterValMinMs = 1000000 / (preset->blinkFrequencyTimes1000 + preset->blinkFrequencyVarTimes1000);
		int blinkInterValMaxMs = 1000000 / (preset->blinkFrequencyTimes1000 - preset->blinkFrequencyVarTimes1000);
		blinkInterValDeltaMs = MAX(blinkInterValMaxMs - blinkInterValMinMs, 1);

		// initializing the durations and times of each blink
		for(int i = 0; i < MAX_COUNT; i++){
			blinkStartTimes[i] = 0;
			blinkDurations[i] = 0;
		}

		nextBlinkTime = _startTime + blinkInterValMinMs + (RNG::randomUInt32() % blinkInterValDeltaMs);
		NRF_LOG_INFO("ms: %d -> %d, %d, %d", _startTime, blinkInterValMinMs, blinkInterValDeltaMs, nextBlinkTime);
	}

	/// <summary>
	/// Computes the list of LEDs that need to be on, and what their intensities should be.
	/// </summary>
	/// <param name="ms">The animation time (in milliseconds)</param>
	/// <param name="retIndices">the return list of LED indices to fill, max size should be at least 21, the max number of leds</param>
	/// <param name="retColors">the return list of LED color to fill, max size should be at least 21, the max number of leds</param>
	/// <returns>The number of leds/intensities added to the return array</returns>
	int AnimationInstanceNoise::updateLEDs(int ms, int retIndices[], uint32_t retColors[]) {
		
        auto preset = getPreset();
		int time = ms - startTime;

		// LEDs will pick an initial color from the overall gradient (generally black to white)
		auto& gradientOverall = animationBits->getRGBTrack(preset->overallGradientTrackOffset); 			
		// they will then fade according to the individual gradient
		auto& gradientIndividual = animationBits->getRGBTrack(preset->individualGradientTrackOffset);	

		// gradient time is an x-axis variable used to progress along a gradient and is normalized to range from 0-1000
		// eg: if we have a gradient that goes r->g->b 
        int gradientTime = time*1000/preset->duration;
		uint32_t gradientColor = gradientOverall.evaluateColor(animationBits, gradientTime);

		// Should we start a new blink instance?
		if (ms >= nextBlinkTime) {
			// Yes, pick an led!
			int newLed = RNG::randomUInt32() % ledCount;
			blinkDurations[newLed] = preset->blinkDurationMs;
			blinkStartTimes[newLed] = ms;

			// Setup next blink
			nextBlinkTime = ms + blinkInterValMinMs + (RNG::randomUInt32() % blinkInterValDeltaMs);
		}

		int retCount = 0; // number that indicates how many LEDS to light up in ther current cycle
		for (int i = 0; i < ledCount; ++i) {
			if (blinkDurations[i] > 0) {
				// Update this blink
				int blinkTime = ms - blinkStartTimes[i];
				if (blinkTime > blinkDurations[i]) {
					// This blink is over, return black this one time
					retIndices[retCount] = i;
					retColors[retCount] = 0;
					retCount++;

					// and clear the array entry
					blinkDurations[i] = 0;
					blinkStartTimes[i] = 0;
				} else {
					// Process this blink
					int blinkGradientTime = blinkTime * 1000 / blinkDurations[i];
					uint32_t blinkColor = gradientIndividual.evaluateColor(animationBits, blinkGradientTime);
					retIndices[retCount] = i;
					retColors[retCount] = Utils::mulColors(gradientColor, blinkColor);
					retCount++;
				}
			}
			// Else skip
		}
		return retCount;
	}

	/// <summary>
	/// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
	/// </summary>
	int AnimationInstanceNoise::stop(int retIndices[]) {
		return setIndices(ANIM_FACEMASK_ALL_LEDS, retIndices);
	}

	const AnimationNoise* AnimationInstanceNoise::getPreset() const {
        return static_cast<const AnimationNoise*>(animationPreset);
    }
}
