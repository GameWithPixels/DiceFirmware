#include "animation_noise.h"
#include "data_set/data_animation_bits.h"
#include "utils/Utils.h"
#include "config/board_config.h"
#include "nrf_log.h"
#include "drivers_nrf/rng.h"
#include "modules/accelerometer.h"
#include "dice_variants.h"
#include "utils/Rainbow.h"

using namespace DriversNRF;
using namespace Modules;

#define MAX_RETRIES 5
namespace Animations
{
    int computeBaseParam(NoiseColorOverrideType type) {
        switch (type) {
            case NoiseColorOverrideType_FaceToGradient:
        		return (Accelerometer::currentFace() * 1000) / Config::DiceVariants::getLayout()->faceCount;
            case NoiseColorOverrideType_FaceToRainbowWheel:
        		return (Accelerometer::currentFace() * 256) / Config::DiceVariants::getLayout()->faceCount;
            case NoiseColorOverrideType_RandomFromGradient:
            case NoiseColorOverrideType_None:
            default:
                return 0;
        }
    }

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
	void AnimationInstanceNoise::start(int _startTime, uint8_t _remapFace, uint8_t _loopCount) {
		AnimationInstance::start(_startTime, _remapFace, _loopCount);
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
		baseColorParam = computeBaseParam(preset->overallGradientColorType);
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
		int fadeTime = preset->duration * preset->fade / (255 * 2);

		// LEDs will pick an initial color from the overall gradient (generally black to white)
		auto& gradientOverall = animationBits->getRGBTrack(preset->overallGradientTrackOffset); 			
		// they will then fade according to the individual gradient
		auto& gradientIndividual = animationBits->getRGBTrack(preset->individualGradientTrackOffset);	

		uint8_t intensity = 255;
		if (time <= fadeTime) {
			// Ramp up
			intensity = (uint8_t)(time * 255 / fadeTime);
		} else if (time >= (preset->duration - fadeTime)) {
			// Ramp down
			intensity = (uint8_t)((preset->duration - time) * 255 / fadeTime);
		}

		// Should we start a new blink instance?
		if (ms >= nextBlinkTime) {
			// Yes, pick an led!
			int newLed = RNG::randomUInt32() % ledCount;
			for (int retries = 0; blinkDurations[newLed] != 0 && retries < MAX_RETRIES; ++retries) {
				newLed = RNG::randomUInt32() % ledCount;
			}

			// Setup next blink
			blinkDurations[newLed] = preset->blinkDurationMs;
			blinkStartTimes[newLed] = ms;

			uint32_t gradientColor = 0;
			switch (preset->overallGradientColorType) {
				case NoiseColorOverrideType_RandomFromGradient:
					// Ignore instance gradient parameter, each blink gets a random value
					gradientColor = gradientOverall.evaluateColor(animationBits, RNG::randomUInt32() % 1000);
					break;
				case NoiseColorOverrideType_FaceToGradient:
					{
						// use the current face (set at start()) + variance
						int var = (int)(RNG::randomUInt32() % MAX(1, (2 * preset->overallGradientColorVar))) - preset->overallGradientColorVar;
						int param = baseColorParam + var;
						if (param < 0) {
							param = 0;
						} else if (param > 1000) {
							param = 1000;
						}
						gradientColor = gradientOverall.evaluateColor(animationBits, param);
					}
					break;
				case NoiseColorOverrideType_FaceToRainbowWheel:
					{
						// use the current face (set at start()) + variance
						int var = (int)(RNG::randomUInt32() % MAX(1, (2 * preset->overallGradientColorVar))) - preset->overallGradientColorVar;
						int param = baseColorParam + var * 255 / 1000;
						gradientColor = Rainbow::wheel(param);
					}
					break;
				case NoiseColorOverrideType_None:
				default:
					{
					int gradientTime = time * 1000 / preset->duration;
					gradientColor = gradientOverall.evaluateColor(animationBits, gradientTime);
					}
					break;
			}

			blinkColors[newLed] = gradientColor;
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
					retColors[retCount] = Utils::modulateColor(Utils::mulColors(blinkColors[i], blinkColor), intensity);
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
